#ifndef LIB_FILE_CPP
#define LIB_FILE_CPP

#include "file.hpp"
#include "frontend/ACompiler.hpp"
#include "shared/DefaultOperator.hpp"
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <regex>
#include <sys/stat.h>

#ifdef _WIN32
#define STAT_STRUCT struct _stat64
#define STAT_FUNC _stat64
#define FSEEK _fseeki64
#define FTELL _ftelli64
#else
#define STAT_STRUCT struct stat
#define STAT_FUNC stat
#define FSEEK fseeko
#define FTELL ftello
#endif

namespace Autolang {
class ACompiler;

namespace Libs {
namespace file {

struct AFileHandle {
	FILE *fp = nullptr;
};

static void destroyFile(ANotifier &notifier, void *fileData) {
	auto handle = static_cast<AFileHandle *>(fileData);
	if (handle->fp) {
		fclose(handle->fp);
	}
	delete handle;
}

static bool checkFilePathSecurity(const std::string &path, ANotifier &notifier) {
	if (path.length() > 512) {
		return false;
	}
	if (!notifier.vm->allowedFilePathsRegex) {
		return true;
	}
	std::error_code ec;
	std::string absPath = std::filesystem::absolute(std::filesystem::path(path), ec).string();
	if (ec) {
		return false;
	}
	std::string normalizedPath = absPath;
	for (char &c : normalizedPath) {
		if (c == '\\') {
			c = '/';
		}
	}
	return std::regex_match(normalizedPath, *(notifier.vm->allowedFilePathsRegex));
}

static std::string resolveFilePath(const std::string &rawPath, ANotifier &notifier) {
	if (notifier.vm->fileBasePath.empty()) {
		return rawPath;
	}
	std::filesystem::path p(rawPath);
	if (p.is_relative()) {
		return (std::filesystem::path(notifier.vm->fileBasePath) / p).string();
	}
	return rawPath;
}

inline AObject *constructor(NativeFuncInData) {
	ClassId classId = args[0]->i;
	const std::string &rawPath = args[1]->str->data;
	std::string path = resolveFilePath(rawPath, notifier);
	int64_t modeInt = args[2]->i;

	if (!checkFilePathSecurity(path, notifier)) {
		notifier.throwException("SecurityError: File path is not allowed.");
		return nullptr;
	}

	bool requiresRead = (modeInt == 0 || modeInt == 3 || modeInt == 4 || modeInt == 5);
	bool requiresWrite = (modeInt == 1 || modeInt == 2 || modeInt == 3 || modeInt == 4 || modeInt == 5);

	if (requiresRead && !notifier.vm->allowFileRead) {
		notifier.throwException("SecurityError: File read operation is not allowed.");
		return nullptr;
	}
	if (requiresWrite && !notifier.vm->allowFileWrite) {
		notifier.throwException("SecurityError: File write operation is not allowed.");
		return nullptr;
	}

	const char *cMode;
	switch (modeInt) {
		case 0:
			cMode = "rb";
			break;
		case 1:
			cMode = "wb";
			break;
		case 2:
			cMode = "ab";
			break;
		case 3:
			cMode = "r+b";
			break;
		case 4:
			cMode = "w+b";
			break;
		case 5:
			cMode = "a+b";
			break;
		default:
			notifier.throwException("Invalid FileMode");
			return nullptr;
	}

	FILE *fp = fopen(path.c_str(), cMode);
	if (!fp) {
		notifier.throwException("Cannot open file: " + path);
		return nullptr;
	}

	auto handle = new AFileHandle{fp};
	return notifier.createNativeData(classId, handle, destroyFile);
}

inline AObject *read_text(NativeFuncInData) {
	if (!notifier.vm->allowFileRead) {
		notifier.throwException("SecurityError: File read operation is not allowed.");
		return nullptr;
	}
	auto handle = static_cast<AFileHandle *>(args[0]->data->data);
	if (!handle->fp) {
		notifier.throwException("File is closed");
		return nullptr;
	}

	FSEEK(handle->fp, 0, SEEK_END);
	int64_t size = FTELL(handle->fp);
	FSEEK(handle->fp, 0, SEEK_SET);

	std::string buffer;
	if (size > 0) {

		buffer.resize(size);
		fread(buffer.data(), 1, size, handle->fp);
	}

	return notifier.createString(buffer);
}

inline AObject *for_each_line(NativeFuncInData) {
	if (!notifier.vm->allowFileRead) {
		notifier.throwException("SecurityError: File read operation is not allowed.");
		return nullptr;
	}
	auto handle = static_cast<AFileHandle *>(args[0]->data->data);
	auto funcObject = args[1];

	if (!handle->fp) {
		notifier.throwException("File is closed");
		return nullptr;
	}

	FSEEK(handle->fp, 0, SEEK_SET);

	char buf[4096];
	std::string line;

	while (fgets(buf, sizeof(buf), handle->fp)) {
		size_t len = strlen(buf);

		if (len > 0 && buf[len - 1] == '\n') {
			line.append(buf, len - 1);
			if (!line.empty() && line.back() == '\r') {
				line.pop_back();
			}

			auto lineObj = notifier.createString(line);
			auto value = notifier.callFunctionObject(funcObject, lineObj);
			if (notifier.hasException())
				return nullptr;

			line.clear();
		} else {

			line.append(buf, len);
		}
	}

	if (!line.empty()) {
		auto lineObj = notifier.createString(line);
		auto value = notifier.callFunctionObject(funcObject, lineObj);
	}

	return nullptr;
}

inline AObject *write(NativeFuncInData) {
	if (!notifier.vm->allowFileWrite) {
		notifier.throwException("SecurityError: File write operation is not allowed.");
		return nullptr;
	}
	auto handle = static_cast<AFileHandle *>(args[0]->data->data);
	if (!handle->fp) {
		notifier.throwException("File is closed");
		return nullptr;
	}

	const std::string &data = args[1]->str->data;
	if (!data.empty()) {
		fwrite(data.data(), 1, data.size(), handle->fp);
	}
	return nullptr;
}

inline AObject *seek(NativeFuncInData) {
	auto handle = static_cast<AFileHandle *>(args[0]->data->data);
	if (!handle->fp)
		return nullptr;

	int64_t pos = args[1]->i;
	FSEEK(handle->fp, pos, SEEK_SET);
	return nullptr;
}

inline AObject *close(NativeFuncInData) {
	auto handle = static_cast<AFileHandle *>(args[0]->data->data);
	if (handle->fp) {
		fclose(handle->fp);
		handle->fp = nullptr;
	}
	return nullptr;
}

inline AObject *exists(NativeFuncInData) {
	if (!notifier.vm->allowFileRead) {
		notifier.throwException("SecurityError: File read operation is not allowed.");
		return nullptr;
	}
	const std::string &rawPath = args[0]->str->data;
	std::string path = resolveFilePath(rawPath, notifier);
	if (!checkFilePathSecurity(path, notifier)) {
		notifier.throwException("SecurityError: File path is not allowed.");
		return nullptr;
	}
	STAT_STRUCT stat_buf;
	bool result = (STAT_FUNC(path.c_str(), &stat_buf) == 0);
	return notifier.createBool(result);
}

inline AObject *delete_file(NativeFuncInData) {
	if (!notifier.vm->allowFileWrite || !notifier.vm->allowFileDelete) {
		notifier.throwException("SecurityError: File delete operation is not allowed.");
		return nullptr;
	}
	const std::string &rawPath = args[0]->str->data;
	std::string path = resolveFilePath(rawPath, notifier);
	if (!checkFilePathSecurity(path, notifier)) {
		notifier.throwException("SecurityError: File path is not allowed.");
		return nullptr;
	}
	bool success = (std::remove(path.c_str()) == 0);
	return notifier.createBool(success);
}

inline AObject *get_parent(NativeFuncInData) {
	const std::string &rawPath = args[0]->str->data;
	std::string path = resolveFilePath(rawPath, notifier);
	size_t sep_pos = path.find_last_of("/\\");

	if (sep_pos == std::string::npos) {
		return notifier.createString("");
	}
	if (sep_pos == 0) {
		return notifier.createString(path.substr(0, 1));
	}
	return notifier.createString(path.substr(0, sep_pos));
}

inline AObject *get_absolute_path(NativeFuncInData) {
	if (!notifier.vm->allowFileRead) {
		notifier.throwException("SecurityError: File read operation is not allowed.");
		return nullptr;
	}
	const std::string &rawPath = args[0]->str->data;
	std::string path = resolveFilePath(rawPath, notifier);
	if (!checkFilePathSecurity(path, notifier)) {
		notifier.throwException("SecurityError: File path is not allowed.");
		return nullptr;
	}
	std::error_code ec;
	std::string absPath =
	    std::filesystem::absolute(std::filesystem::path(path), ec).string();
	if (ec) {
		notifier.throwException("Invalid path: " + path);
		return nullptr;
	}
	return notifier.createString(absPath);
}

inline AObject *is_directory(NativeFuncInData) {
	if (!notifier.vm->allowFileRead) {
		notifier.throwException("SecurityError: File read operation is not allowed.");
		return nullptr;
	}
	const std::string &rawPath = args[0]->str->data;
	std::string path = resolveFilePath(rawPath, notifier);
	if (!checkFilePathSecurity(path, notifier)) {
		notifier.throwException("SecurityError: File path is not allowed.");
		return nullptr;
	}
	STAT_STRUCT stat_buf;
	int rc = STAT_FUNC(path.c_str(), &stat_buf);
#ifdef _WIN32
	bool result = (rc == 0) && ((stat_buf.st_mode & _S_IFDIR) != 0);
#else
	bool result = (rc == 0) && S_ISDIR(stat_buf.st_mode);
#endif
	return notifier.createBool(result);
}

inline AObject *is_file(NativeFuncInData) {
	if (!notifier.vm->allowFileRead) {
		notifier.throwException("SecurityError: File read operation is not allowed.");
		return nullptr;
	}
	const std::string &rawPath = args[0]->str->data;
	std::string path = resolveFilePath(rawPath, notifier);
	if (!checkFilePathSecurity(path, notifier)) {
		notifier.throwException("SecurityError: File path is not allowed.");
		return nullptr;
	}
	STAT_STRUCT stat_buf;
	int rc = STAT_FUNC(path.c_str(), &stat_buf);
#ifdef _WIN32
	bool result = (rc == 0) && ((stat_buf.st_mode & _S_IFREG) != 0);
#else
	bool result = (rc == 0) && S_ISREG(stat_buf.st_mode);
#endif
	return notifier.createBool(result);
}

inline AObject *get_all_files(NativeFuncInData) {
	if (!notifier.vm->allowFileRead) {
		notifier.throwException("SecurityError: File read operation is not allowed.");
		return nullptr;
	}
	const std::string &rawPath = args[0]->str->data;
	std::string path = resolveFilePath(rawPath, notifier);
	if (!checkFilePathSecurity(path, notifier)) {
		notifier.throwException("SecurityError: File path is not allowed.");
		return nullptr;
	}
	ClassId arrayClassId = args[1]->i;

	auto newArr = notifier.createArray(arrayClassId);
	std::error_code ec;

	if (std::filesystem::exists(path, ec) &&
	    std::filesystem::is_directory(path, ec)) {
		for (const auto &entry :
		     std::filesystem::directory_iterator(path, ec)) {
			if (ec) {
				notifier.throwException(
				    "Filesystem error while reading directory: " +
				    ec.message());
				return nullptr;
			}
			notifier.arrayAdd(newArr,
			                  notifier.createString(entry.path().string()));
		}
	} else {
		if (ec) {
			notifier.throwException("Filesystem error: " + ec.message());
		} else {
			notifier.throwException("Path is not a valid directory: " + path);
		}
		return nullptr;
	}

	return newArr;
}

inline AObject *get_name(NativeFuncInData) {
	const std::string &rawPath = args[0]->str->data;
	std::string path = resolveFilePath(rawPath, notifier);
	size_t sep_pos = path.find_last_of("/\\");
	if (sep_pos == std::string::npos) {
		return notifier.createString(path);
	}
	if (sep_pos == path.length() - 1) {
		return notifier.createString("");
	}
	return notifier.createString(path.substr(sep_pos + 1));
}

inline AObject *get_size(NativeFuncInData) {
	if (!notifier.vm->allowFileRead) {
		notifier.throwException("SecurityError: File read operation is not allowed.");
		return nullptr;
	}
	const std::string &rawPath = args[0]->str->data;
	std::string path = resolveFilePath(rawPath, notifier);
	if (!checkFilePathSecurity(path, notifier)) {
		notifier.throwException("SecurityError: File path is not allowed.");
		return nullptr;
	}
	STAT_STRUCT stat_buf;

	if (STAT_FUNC(path.c_str(), &stat_buf) == 0) {
		return notifier.createInt(static_cast<int64_t>(stat_buf.st_size));
	}

	notifier.throwException(
	    "Cannot get file size (might be a directory or not exist): " + path);
	return nullptr;
}

inline AObject *get_extension(NativeFuncInData) {
	const std::string &rawPath = args[0]->str->data;
	std::string path = resolveFilePath(rawPath, notifier);
	size_t dot_pos = path.find_last_of('.');
	size_t sep_pos = path.find_last_of("/\\");

	if (dot_pos == std::string::npos ||
	    (sep_pos != std::string::npos && dot_pos < sep_pos)) {
		return notifier.createString("");
	}
	if (dot_pos == sep_pos + 1 ||
	    (sep_pos == std::string::npos && dot_pos == 0)) {
		return notifier.createString(path.substr(dot_pos));
	}
	return notifier.createString(path.substr(dot_pos));
}

inline AObject *get_last_modified(NativeFuncInData) {
	if (!notifier.vm->allowFileRead) {
		notifier.throwException("SecurityError: File read operation is not allowed.");
		return nullptr;
	}
	const std::string &rawPath = args[0]->str->data;
	std::string path = resolveFilePath(rawPath, notifier);
	if (!checkFilePathSecurity(path, notifier)) {
		notifier.throwException("SecurityError: File path is not allowed.");
		return nullptr;
	}
	STAT_STRUCT stat_buf;

	if (STAT_FUNC(path.c_str(), &stat_buf) == 0) {
		return notifier.createInt(static_cast<int64_t>(stat_buf.st_mtime));
	}

	notifier.throwException("Cannot get last modified time for: " + path);
	return nullptr;
}

void init(ACompiler &compiler) {
	compiler.registerBuiltInLibrary(
	    "std/file", R"###(
enum FileMode {
    READ,          
    WRITE,         
    APPEND,        
    READ_WRITE,    
    WRITE_READ,    
    APPEND_READ;   

    fun getId(): Int = when (this) {
        READ -> 0
        WRITE -> 1
        APPEND -> 2
        READ_WRITE -> 3
        WRITE_READ -> 4
        APPEND_READ -> 5
        else -> -1
    }

    fun toString(): String = when (this) {
        READ -> "READ"
        WRITE -> "WRITE"
        APPEND -> "APPEND"
        READ_WRITE -> "READ_WRITE"
        WRITE_READ -> "WRITE_READ"
        APPEND_READ -> "APPEND_READ"
        else -> "UNKNOWN"
    }
}

@no_constructor
@no_extends
class File {
    
    @native("file_constructor")
    private static fun __CLASS__(classId: Int, path: String, modeId: Int): File

    static fun __CLASS__(path: String, mode: FileMode): File = File(getClassId(File), path, mode.getId())

    @native("file_read_text")
    fun readText(): String

    @native("file_for_each_line")
    fun forEachLine(fn: (String) -> Void)

    @native("file_write")
    fun write(text: String)

    @native("file_close")
    fun close()

    @native("file_seek")
    fun seek(position: Int)

    @native("file_exists")
    static fun exists(path: String): Bool

    @native("file_delete")
    static fun delete(path: String): Bool

    @native("file_get_parent")
    static fun getParent(path: String): String

    @native("file_get_absolute_path")
    static fun getAbsolutePath(path: String): String

    @native("file_is_directory")
    static fun isDirectory(path: String): Bool

    @native("file_is_file")
    static fun isFile(path: String): Bool
    
    @native("file_get_all_files")
    static fun getAllFiles(dirPath: String, arrayClassId: Int = getClassId(Array<String>)): Array<String>

    @native("file_get_name")
    static fun getName(path: String): String

    @native("file_get_size")
    static fun getSize(path: String): Int

    @native("file_get_extension")
    static fun getExtension(path: String): String
    
    
    @native("file_get_last_modified")
    static fun getLastModified(path: String): Int
}
    )###",
	    LibraryConfig(),
	    ANativeMap({
	        {"file_constructor", &file::constructor},
	        {"file_read_text", &file::read_text},
	        {"file_for_each_line", &file::for_each_line},
	        {"file_write", &file::write},
	        {"file_close", &file::close},
	        {"file_get_parent", &file::get_parent},
	        {"file_get_absolute_path", &file::get_absolute_path},
	        {"file_get_all_files", &file::get_all_files},
	        {"file_is_directory", &file::is_directory},
	        {"file_get_name", &file::get_name},
	        {"file_get_size", &file::get_size},
	        {"file_get_last_modified", &file::get_last_modified},
	        {"file_is_file", &file::is_file},
	        {"file_seek", &file::seek},
	        {"file_get_extension", &file::get_extension},
	        {"file_exists", &file::exists},
	        {"file_delete", &file::delete_file},
	    }));
}

} // namespace file
} // namespace Libs
} // namespace Autolang
#endif
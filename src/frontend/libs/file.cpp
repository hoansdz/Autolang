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
	std::string path;
	bool isClosed = false;
};

static void destroyFile(ANotifier &notifier, void *fileData) {
	auto handle = static_cast<AFileHandle *>(fileData);
	if (handle->fp) {
		fclose(handle->fp);
	}
	delete handle;
}

static inline std::string extractPath(AObject *obj) {
	if (obj->flags & AObject::Flags::OBJ_IS_NATIVE_DATA) {
		return static_cast<AFileHandle *>(obj->data->data)->path;
	}
	return std::string(obj->str->data, obj->str->size);
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

AObject *constructor(NativeFuncInData) {
	ClassId classId = notifier.callFrame->func->returnId;
	const std::string &rawPath = args[0]->str->data;
	std::string path = resolveFilePath(rawPath, notifier);
	int64_t modeInt = args[1]->i;

	if (!checkFilePathSecurity(path, notifier)) {
		notifier.throwException("SecurityError: File path is not allowed.");
		return nullptr;
	}

	if (modeInt == -1) {
		auto handle = new AFileHandle{nullptr, path};
		return notifier.createNativeData(classId, handle, destroyFile);
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

	auto handle = new AFileHandle{fp, path};
	return notifier.createNativeData(classId, handle, destroyFile);
}

AObject *get_file_path(NativeFuncInData) {
	auto handle = static_cast<AFileHandle *>(args[0]->data->data);
	return notifier.createString(handle->path);
}

AObject *static_read_text(NativeFuncInData) {
	if (!notifier.vm->allowFileRead) {
		notifier.throwException("SecurityError: File read operation is not allowed.");
		return nullptr;
	}
	const std::string &rawPath = extractPath(args[0]);
	std::string path = resolveFilePath(rawPath, notifier);
	if (!checkFilePathSecurity(path, notifier)) {
		notifier.throwException("SecurityError: File path is not allowed.");
		return nullptr;
	}
	FILE *fp = fopen(path.c_str(), "rb");
	if (!fp) {
		notifier.throwException("Cannot open file: " + path);
		return nullptr;
	}
	FSEEK(fp, 0, SEEK_END);
	int64_t size = FTELL(fp);
	FSEEK(fp, 0, SEEK_SET);
	std::string buffer;
	if (size > 0) {
		buffer.resize(size);
		fread(buffer.data(), 1, size, fp);
	}
	fclose(fp);
	return notifier.createString(buffer);
}

AObject *static_write_text(NativeFuncInData) {
	if (!notifier.vm->allowFileWrite) {
		notifier.throwException("SecurityError: File write operation is not allowed.");
		return nullptr;
	}
	const std::string &rawPath = extractPath(args[0]);
	std::string path = resolveFilePath(rawPath, notifier);
	if (!checkFilePathSecurity(path, notifier)) {
		notifier.throwException("SecurityError: File path is not allowed.");
		return nullptr;
	}
	const std::string &text = args[1]->str->data;
	FILE *fp = fopen(path.c_str(), "wb");
	if (!fp) {
		notifier.throwException("Cannot open file for writing: " + path);
		return nullptr;
	}
	if (!text.empty()) {
		fwrite(text.data(), 1, text.size(), fp);
	}
	fclose(fp);
	return nullptr;
}

AObject *static_append_text(NativeFuncInData) {
	if (!notifier.vm->allowFileWrite) {
		notifier.throwException("SecurityError: File write operation is not allowed.");
		return nullptr;
	}
	const std::string &rawPath = extractPath(args[0]);
	std::string path = resolveFilePath(rawPath, notifier);
	if (!checkFilePathSecurity(path, notifier)) {
		notifier.throwException("SecurityError: File path is not allowed.");
		return nullptr;
	}
	const std::string &text = args[1]->str->data;
	FILE *fp = fopen(path.c_str(), "ab");
	if (!fp) {
		notifier.throwException("Cannot open file for appending: " + path);
		return nullptr;
	}
	if (!text.empty()) {
		fwrite(text.data(), 1, text.size(), fp);
	}
	fclose(fp);
	return nullptr;
}

AObject *static_read_lines(NativeFuncInData) {
	if (!notifier.vm->allowFileRead) {
		notifier.throwException("SecurityError: File read operation is not allowed.");
		return nullptr;
	}
	const std::string &rawPath = extractPath(args[0]);
	std::string path = resolveFilePath(rawPath, notifier);
	if (!checkFilePathSecurity(path, notifier)) {
		notifier.throwException("SecurityError: File path is not allowed.");
		return nullptr;
	}
	FILE *fp = fopen(path.c_str(), "rb");
	if (!fp) {
		notifier.throwException("Cannot open file: " + path);
		return nullptr;
	}
	ClassId classId = notifier.callFrame->func->returnId;
	AObject *arrayObj = notifier.createArray(classId);
	char buf[4096];
	std::string line;
	while (fgets(buf, sizeof(buf), fp)) {
		size_t len = strlen(buf);
		if (len > 0 && buf[len - 1] == '\n') {
			line.append(buf, len - 1);
			if (!line.empty() && line.back() == '\r') {
				line.pop_back();
			}
			notifier.arrayAdd(arrayObj, notifier.createString(line));
			line.clear();
		} else {
			line.append(buf, len);
		}
	}
	if (!line.empty()) {
		notifier.arrayAdd(arrayObj, notifier.createString(line));
	}
	fclose(fp);
	return arrayObj;
}

AObject *read_text(NativeFuncInData) {
	if (!notifier.vm->allowFileRead) {
		notifier.throwException("SecurityError: File read operation is not allowed.");
		return nullptr;
	}
	auto handle = static_cast<AFileHandle *>(args[0]->data->data);
	if (handle->isClosed) {
		notifier.throwException("File is closed");
		return nullptr;
	}
	if (!handle->fp) {
		return static_read_text(notifier, args, argSize);
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

AObject *static_read_bytes(NativeFuncInData) {
	if (!notifier.vm->allowFileRead) {
		notifier.throwException("SecurityError: File read operation is not allowed.");
		return nullptr;
	}
	const std::string &rawPath = extractPath(args[0]);
	std::string path = resolveFilePath(rawPath, notifier);
	if (!checkFilePathSecurity(path, notifier)) {
		notifier.throwException("SecurityError: File path is not allowed.");
		return nullptr;
	}
	FILE *fp = fopen(path.c_str(), "rb");
	if (!fp) {
		notifier.throwException("Cannot open file: " + path);
		return nullptr;
	}
	FSEEK(fp, 0, SEEK_END);
	int64_t size = FTELL(fp);
	FSEEK(fp, 0, SEEK_SET);
	AObject *bytesObj = notifier.createBytes(size);
	if (size > 0) {
		fread(bytesObj->bytes->data, 1, size, fp);
		bytesObj->bytes->size = size;
	}
	fclose(fp);
	return bytesObj;
}

AObject *read_bytes(NativeFuncInData) {
	if (!notifier.vm->allowFileRead) {
		notifier.throwException("SecurityError: File read operation is not allowed.");
		return nullptr;
	}
	auto handle = static_cast<AFileHandle *>(args[0]->data->data);
	if (handle->isClosed) {
		notifier.throwException("File is closed");
		return nullptr;
	}
	if (!handle->fp) {
		return static_read_bytes(notifier, args, argSize);
	}
	FSEEK(handle->fp, 0, SEEK_END);
	int64_t size = FTELL(handle->fp);
	FSEEK(handle->fp, 0, SEEK_SET);
	AObject *bytesObj = notifier.createBytes(size);
	if (size > 0) {
		fread(bytesObj->bytes->data, 1, size, handle->fp);
		bytesObj->bytes->size = size;
	}
	return bytesObj;
}

AObject *static_write_bytes(NativeFuncInData) {
	if (!notifier.vm->allowFileWrite) {
		notifier.throwException("SecurityError: File write operation is not allowed.");
		return nullptr;
	}
	const std::string &rawPath = extractPath(args[0]);
	std::string path = resolveFilePath(rawPath, notifier);
	if (!checkFilePathSecurity(path, notifier)) {
		notifier.throwException("SecurityError: File path is not allowed.");
		return nullptr;
	}
	ABytes *b = args[1]->bytes;
	FILE *fp = fopen(path.c_str(), "wb");
	if (!fp) {
		notifier.throwException("Cannot open file for writing: " + path);
		return nullptr;
	}
	if (b && b->size > 0) {
		fwrite(b->data, 1, b->size, fp);
	}
	fclose(fp);
	return nullptr;
}

AObject *write_bytes(NativeFuncInData) {
	if (!notifier.vm->allowFileWrite) {
		notifier.throwException("SecurityError: File write operation is not allowed.");
		return nullptr;
	}
	auto handle = static_cast<AFileHandle *>(args[0]->data->data);
	if (handle->isClosed) {
		notifier.throwException("File is closed");
		return nullptr;
	}
	if (!handle->fp) {
		return static_write_bytes(notifier, args, argSize);
	}
	ABytes *b = args[1]->bytes;
	if (b && b->size > 0) {
		fwrite(b->data, 1, b->size, handle->fp);
	}
	return nullptr;
}

AObject *copy_to(NativeFuncInData) {
	if (!notifier.vm->allowFileRead || !notifier.vm->allowFileWrite) {
		notifier.throwException("SecurityError: File copy operation is not allowed.");
		return nullptr;
	}
	const std::string &rawSrc = extractPath(args[0]);
	const std::string &rawDst = extractPath(args[1]);
	std::string src = resolveFilePath(rawSrc, notifier);
	std::string dst = resolveFilePath(rawDst, notifier);
	if (!checkFilePathSecurity(src, notifier) || !checkFilePathSecurity(dst, notifier)) {
		notifier.throwException("SecurityError: File path is not allowed.");
		return nullptr;
	}
	bool overwrite = (argSize >= 3 && args[2]->type == DefaultClass::boolClassId) ? args[2]->b : false;

	std::error_code ec;
	auto options = overwrite ? std::filesystem::copy_options::overwrite_existing : std::filesystem::copy_options::none;
	std::filesystem::copy_file(src, dst, options, ec);
	if (ec) {
		notifier.throwException("Failed to copy file: " + ec.message());
		return nullptr;
	}
	return args[1];
}

AObject *copy_recursively(NativeFuncInData) {
	if (!notifier.vm->allowFileRead || !notifier.vm->allowFileWrite) {
		notifier.throwException("SecurityError: File copy operation is not allowed.");
		return nullptr;
	}
	const std::string &rawSrc = extractPath(args[0]);
	const std::string &rawDst = extractPath(args[1]);
	std::string src = resolveFilePath(rawSrc, notifier);
	std::string dst = resolveFilePath(rawDst, notifier);
	if (!checkFilePathSecurity(src, notifier) || !checkFilePathSecurity(dst, notifier)) {
		notifier.throwException("SecurityError: File path is not allowed.");
		return nullptr;
	}
	bool overwrite = (argSize >= 3 && args[2]->type == DefaultClass::boolClassId) ? args[2]->b : false;

	std::error_code ec;
	auto options = std::filesystem::copy_options::recursive;
	if (overwrite) options |= std::filesystem::copy_options::overwrite_existing;
	std::filesystem::copy(src, dst, options, ec);
	if (ec) {
		notifier.throwException("Failed to copy recursively: " + ec.message());
		return nullptr;
	}
	return notifier.createBool(true);
}

AObject *for_each_line(NativeFuncInData) {
	if (!notifier.vm->allowFileRead) {
		notifier.throwException("SecurityError: File read operation is not allowed.");
		return nullptr;
	}
	auto handle = static_cast<AFileHandle *>(args[0]->data->data);
	auto funcObject = args[1];

	FILE *fp = handle->fp;
	bool closeFp = false;
	if (!fp) {
		std::string path = resolveFilePath(handle->path, notifier);
		if (!checkFilePathSecurity(path, notifier)) {
			notifier.throwException("SecurityError: File path is not allowed.");
			return nullptr;
		}
		fp = fopen(path.c_str(), "rb");
		if (!fp) {
			notifier.throwException("Cannot open file: " + path);
			return nullptr;
		}
		closeFp = true;
	}

	FSEEK(fp, 0, SEEK_SET);

	char buf[4096];
	std::string line;

	while (fgets(buf, sizeof(buf), fp)) {
		size_t len = strlen(buf);

		if (len > 0 && buf[len - 1] == '\n') {
			line.append(buf, len - 1);
			if (!line.empty() && line.back() == '\r') {
				line.pop_back();
			}

			auto lineObj = notifier.createString(line);
			auto value = notifier.callFunctionObject(funcObject, lineObj);
			if (notifier.hasException()) {
				if (closeFp)
					fclose(fp);
				return nullptr;
			}

			line.clear();
		} else {
			line.append(buf, len);
		}
	}

	if (!line.empty()) {
		auto lineObj = notifier.createString(line);
		auto value = notifier.callFunctionObject(funcObject, lineObj);
	}

	if (closeFp)
		fclose(fp);

	return nullptr;
}

AObject *write(NativeFuncInData) {
	if (!notifier.vm->allowFileWrite) {
		notifier.throwException("SecurityError: File write operation is not allowed.");
		return nullptr;
	}
	auto handle = static_cast<AFileHandle *>(args[0]->data->data);
	if (!handle->fp || handle->isClosed) {
		notifier.throwException("File is closed");
		return nullptr;
	}

	const std::string &data = args[1]->str->data;
	if (!data.empty()) {
		fwrite(data.data(), 1, data.size(), handle->fp);
	}
	return nullptr;
}

AObject *seek(NativeFuncInData) {
	auto handle = static_cast<AFileHandle *>(args[0]->data->data);
	if (!handle->fp)
		return nullptr;

	int64_t pos = args[1]->i;
	FSEEK(handle->fp, pos, SEEK_SET);
	return nullptr;
}

AObject *close(NativeFuncInData) {
	auto handle = static_cast<AFileHandle *>(args[0]->data->data);
	if (handle->fp) {
		fclose(handle->fp);
		handle->fp = nullptr;
	}
	handle->isClosed = true;
	return nullptr;
}

AObject *exists(NativeFuncInData) {
	if (!notifier.vm->allowFileRead) {
		notifier.throwException("SecurityError: File read operation is not allowed.");
		return nullptr;
	}
	const std::string &rawPath = extractPath(args[0]);
	std::string path = resolveFilePath(rawPath, notifier);
	if (!checkFilePathSecurity(path, notifier)) {
		notifier.throwException("SecurityError: File path is not allowed.");
		return nullptr;
	}
	STAT_STRUCT stat_buf;
	bool result = (STAT_FUNC(path.c_str(), &stat_buf) == 0);
	return notifier.createBool(result);
}

AObject *delete_file(NativeFuncInData) {
	if (!notifier.vm->allowFileWrite || !notifier.vm->allowFileDelete) {
		notifier.throwException("SecurityError: File delete operation is not allowed.");
		return nullptr;
	}
	const std::string &rawPath = extractPath(args[0]);
	std::string path = resolveFilePath(rawPath, notifier);
	if (!checkFilePathSecurity(path, notifier)) {
		notifier.throwException("SecurityError: File path is not allowed.");
		return nullptr;
	}
	bool success = (std::remove(path.c_str()) == 0);
	return notifier.createBool(success);
}

AObject *get_parent(NativeFuncInData) {
	const std::string &rawPath = extractPath(args[0]);
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

AObject *get_parent_file(NativeFuncInData) {
	ClassId classId = notifier.callFrame->func->returnId;
	const std::string &rawPath = extractPath(args[0]);
	std::string path = resolveFilePath(rawPath, notifier);
	size_t sep_pos = path.find_last_of("/\\");
	if (sep_pos == std::string::npos) {
		return nullptr;
	}
	std::string parentStr = (sep_pos == 0) ? path.substr(0, 1) : path.substr(0, sep_pos);
	auto handle = new AFileHandle{nullptr, parentStr};
	return notifier.createNativeData(classId, handle, destroyFile);
}

AObject *get_absolute_path(NativeFuncInData) {
	if (!notifier.vm->allowFileRead) {
		notifier.throwException("SecurityError: File read operation is not allowed.");
		return nullptr;
	}
	const std::string &rawPath = extractPath(args[0]);
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

AObject *is_directory(NativeFuncInData) {
	if (!notifier.vm->allowFileRead) {
		notifier.throwException("SecurityError: File read operation is not allowed.");
		return nullptr;
	}
	const std::string &rawPath = extractPath(args[0]);
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

AObject *is_file(NativeFuncInData) {
	if (!notifier.vm->allowFileRead) {
		notifier.throwException("SecurityError: File read operation is not allowed.");
		return nullptr;
	}
	const std::string &rawPath = extractPath(args[0]);
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

AObject *get_all_files(NativeFuncInData) {
	if (!notifier.vm->allowFileRead) {
		notifier.throwException("SecurityError: File read operation is not allowed.");
		return nullptr;
	}
	const std::string &rawPath = extractPath(args[0]);
	std::string path = resolveFilePath(rawPath, notifier);
	if (!checkFilePathSecurity(path, notifier)) {
		notifier.throwException("SecurityError: File path is not allowed.");
		return nullptr;
	}
	ClassId arrayClassId = notifier.callFrame->func->returnId;

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

AObject *get_name(NativeFuncInData) {
	const std::string &rawPath = extractPath(args[0]);
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

AObject *get_name_without_extension(NativeFuncInData) {
	const std::string &rawPath = extractPath(args[0]);
	std::string path = resolveFilePath(rawPath, notifier);
	size_t sep_pos = path.find_last_of("/\\");
	std::string name = (sep_pos == std::string::npos) ? path : path.substr(sep_pos + 1);
	size_t dot_pos = name.find_last_of('.');
	if (dot_pos != std::string::npos && dot_pos > 0) {
		return notifier.createString(name.substr(0, dot_pos));
	}
	return notifier.createString(name);
}

AObject *get_size(NativeFuncInData) {
	if (!notifier.vm->allowFileRead) {
		notifier.throwException("SecurityError: File read operation is not allowed.");
		return nullptr;
	}
	const std::string &rawPath = extractPath(args[0]);
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

AObject *get_extension(NativeFuncInData) {
	const std::string &rawPath = extractPath(args[0]);
	std::string path = resolveFilePath(rawPath, notifier);
	size_t dot_pos = path.find_last_of('.');
	size_t sep_pos = path.find_last_of("/\\");

	if (dot_pos == std::string::npos ||
	    (sep_pos != std::string::npos && dot_pos < sep_pos)) {
		return notifier.createString("");
	}
	if (dot_pos == sep_pos + 1 ||
	    (sep_pos == std::string::npos && dot_pos == 0)) {
		return notifier.createString("");
	}
	return notifier.createString(path.substr(dot_pos + 1));
}

AObject *get_last_modified(NativeFuncInData) {
	if (!notifier.vm->allowFileRead) {
		notifier.throwException("SecurityError: File read operation is not allowed.");
		return nullptr;
	}
	const std::string &rawPath = extractPath(args[0]);
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

AObject *create_new_file(NativeFuncInData) {
	if (!notifier.vm->allowFileWrite) {
		notifier.throwException("SecurityError: File write operation is not allowed.");
		return nullptr;
	}
	const std::string &rawPath = extractPath(args[0]);
	std::string path = resolveFilePath(rawPath, notifier);
	if (!checkFilePathSecurity(path, notifier)) {
		notifier.throwException("SecurityError: File path is not allowed.");
		return nullptr;
	}
	std::error_code ec;
	if (std::filesystem::exists(std::filesystem::path(path), ec)) {
		return notifier.createBool(false);
	}
	FILE *fp = fopen(path.c_str(), "wb");
	if (!fp) {
		return notifier.createBool(false);
	}
	fclose(fp);
	return notifier.createBool(true);
}

AObject *make_dir(NativeFuncInData) {
	if (!notifier.vm->allowFileWrite) {
		notifier.throwException("SecurityError: File write operation is not allowed.");
		return nullptr;
	}
	const std::string &rawPath = extractPath(args[0]);
	std::string path = resolveFilePath(rawPath, notifier);
	if (!checkFilePathSecurity(path, notifier)) {
		notifier.throwException("SecurityError: File path is not allowed.");
		return nullptr;
	}
	std::error_code ec;
	bool ok = std::filesystem::create_directories(std::filesystem::path(path), ec);
	return notifier.createBool(ok && !ec);
}

AObject *delete_recursively(NativeFuncInData) {
	if (!notifier.vm->allowFileWrite || !notifier.vm->allowFileDelete) {
		notifier.throwException("SecurityError: File delete operation is not allowed.");
		return nullptr;
	}
	const std::string &rawPath = extractPath(args[0]);
	std::string path = resolveFilePath(rawPath, notifier);
	if (!checkFilePathSecurity(path, notifier)) {
		notifier.throwException("SecurityError: File path is not allowed.");
		return nullptr;
	}
	std::error_code ec;
	uintmax_t count = std::filesystem::remove_all(std::filesystem::path(path), ec);
	return notifier.createBool(count > 0 && !ec);
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
    private static fun File(path: String, modeId: Int): File

    static fun File(path: String): File = File(path, -1)
    static fun File(path: String, mode: FileMode): File = File(path, mode.getId())
    static fun File(parent: String, child: String): File = File(parent + "/" + child)
    static fun File(parent: File, child: String): File = File(parent.getPath() + "/" + child)

    @native("file_get_path")
    fun getPath(): String

    @native("file_get_path")
    fun path(): String

    @native("file_exists")
    fun exists(): Bool

    @native("file_delete")
    fun delete(): Bool

    @native("file_delete_recursively")
    fun deleteRecursively(): Bool

    @native("file_create_new_file")
    fun createNewFile(): Bool

    @native("file_mkdir")
    fun mkdir(): Bool

    @native("file_mkdir")
    fun mkdirs(): Bool

    @native("file_get_name")
    fun name(): String

    @native("file_get_name")
    fun getName(): String

    @native("file_get_extension")
    fun extension(): String

    @native("file_get_extension")
    fun getExtension(): String

    @native("file_get_name_without_extension")
    fun nameWithoutExtension(): String

    @native("file_get_size")
    fun size(): Int

    @native("file_get_size")
    fun length(): Int

    @native("file_get_parent")
    fun parent(): String

    @native("file_get_parent")
    fun getParent(): String

    @native("file_get_parent_file")
    fun parentFile(): File?

    @native("file_get_parent_file")
    fun getParentFile(): File?

    @native("file_get_absolute_path")
    fun absolutePath(): String

    @native("file_get_absolute_path")
    fun getAbsolutePath(): String

    @native("file_is_directory")
    fun isDirectory(): Bool

    @native("file_is_file")
    fun isFile(): Bool

    fun resolve(relative: String): File = File(this.getPath() + "/" + relative)
    fun resolve(relative: File): File = File(this.getPath() + "/" + relative.getPath())
    fun resolveSibling(relative: String): File {
        val p = this.getParent()
        if (p.length() == 0) return File(relative)
        return File(p + "/" + relative)
    }

    @native("file_static_write_text")
    fun writeText(text: String)

    @native("file_static_append_text")
    fun appendText(text: String)

    @native("file_static_read_lines")
    fun readLines(): Array<String>

    @native("file_static_read_text")
    static fun readText(path: String): String

    @native("file_static_write_text")
    static fun writeText(path: String, text: String)

    @native("file_static_append_text")
    static fun appendText(path: String, text: String)

    @native("file_static_read_lines")
    static fun readLines(path: String): Array<String>

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

    @native("file_delete_recursively")
    static fun deleteRecursively(path: String): Bool

    @native("file_create_new_file")
    static fun createNewFile(path: String): Bool

    @native("file_mkdir")
    static fun mkdir(path: String): Bool

    @native("file_get_parent")
    static fun getParent(path: String): String

    @native("file_get_absolute_path")
    static fun getAbsolutePath(path: String): String

    @native("file_is_directory")
    static fun isDirectory(path: String): Bool

    @native("file_is_file")
    static fun isFile(path: String): Bool
    
    @native("file_get_all_files")
    static fun getAllFiles(dirPath: String): Array<String>

    fun list(): Array<String> = File.getAllFiles(this.getPath())
    fun listFiles(): Array<File> {
        val arr = File.getAllFiles(this.getPath())
        val res = <File>[]
        for (f in arr) {
            res.add(File(f))
        }
        return res
    }

    @native("file_get_name")
    static fun getName(path: String): String

    @native("file_get_size")
    static fun getSize(path: String): Int

    @native("file_get_extension")
    static fun getExtension(path: String): String
    
    @native("file_get_last_modified")
    static fun getLastModified(path: String): Int

    @native("file_read_bytes")
    fun readBytes(): Bytes

    @native("file_write_bytes")
    fun writeBytes(bytes: Bytes)

    @native("file_copy_to")
    fun copyTo(target: File, overwrite: Bool = false): File

    @native("file_copy_to")
    fun copyTo(target: String, overwrite: Bool = false): String

    @native("file_copy_recursively")
    fun copyRecursively(target: File, overwrite: Bool = false): Bool

    @native("file_copy_recursively")
    fun copyRecursively(target: String, overwrite: Bool = false): Bool

    @native("file_static_read_bytes")
    static fun readBytes(path: String): Bytes

    @native("file_static_write_bytes")
    static fun writeBytes(path: String, bytes: Bytes)

    @native("file_copy_to")
    static fun copyTo(source: String, target: String, overwrite: Bool = false): String

    @native("file_copy_recursively")
    static fun copyRecursively(source: String, target: String, overwrite: Bool = false): Bool
}

fun String.toFile(): File = File(this)

@native("file_static_read_text")
fun readFile(path: String): String

@native("file_static_read_lines")
fun readLines(path: String): Array<String>

@native("file_static_read_bytes")
fun readBytes(path: String): Bytes

@native("file_static_write_text")
fun writeFile(path: String, text: String)

@native("file_static_write_bytes")
fun writeBytes(path: String, bytes: Bytes)

@native("file_static_append_text")
fun appendFile(path: String, text: String)
    )###",
	    LibraryConfig(true),
	    ANativeMap({
	        {"file_constructor", &file::constructor},
	        {"file_get_path", &file::get_file_path},
	        {"file_static_read_text", &file::static_read_text},
	        {"file_static_write_text", &file::static_write_text},
	        {"file_static_append_text", &file::static_append_text},
	        {"file_static_read_lines", &file::static_read_lines},
	        {"file_read_text", &file::read_text},
	        {"file_for_each_line", &file::for_each_line},
	        {"file_write", &file::write},
	        {"file_close", &file::close},
	        {"file_get_parent", &file::get_parent},
	        {"file_get_parent_file", &file::get_parent_file},
	        {"file_get_absolute_path", &file::get_absolute_path},
	        {"file_get_all_files", &file::get_all_files},
	        {"file_is_directory", &file::is_directory},
	        {"file_get_name", &file::get_name},
	        {"file_get_name_without_extension", &file::get_name_without_extension},
	        {"file_get_size", &file::get_size},
	        {"file_get_last_modified", &file::get_last_modified},
	        {"file_is_file", &file::is_file},
	        {"file_seek", &file::seek},
	        {"file_get_extension", &file::get_extension},
	        {"file_exists", &file::exists},
	        {"file_delete", &file::delete_file},
	        {"file_delete_recursively", &file::delete_recursively},
	        {"file_create_new_file", &file::create_new_file},
	        {"file_mkdir", &file::make_dir},
	        {"file_read_bytes", &file::read_bytes},
	        {"file_static_read_bytes", &file::static_read_bytes},
	        {"file_write_bytes", &file::write_bytes},
	        {"file_static_write_bytes", &file::static_write_bytes},
	        {"file_copy_to", &file::copy_to},
	        {"file_copy_recursively", &file::copy_recursively},
	    }));
}

} // namespace file
} // namespace Libs
} // namespace Autolang
#endif

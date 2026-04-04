#ifndef LIB_FILE_CPP
#define LIB_FILE_CPP

#include "file.hpp"
#include "frontend/ACompiler.hpp"
#include "shared/DefaultOperator.hpp"
#include <filesystem>
#include <sys/stat.h>
#include <cstdio>
#include <cstring>

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

namespace AutoLang {
class ACompiler;

namespace Libs {
namespace file {

struct AFileHandle {
    FILE* fp = nullptr;
};

static void destroyFile(ANotifier &notifier, void *fileData) {
    auto handle = static_cast<AFileHandle *>(fileData);
    if (handle->fp) {
        fclose(handle->fp);
    }
    delete handle;
}

inline AObject *constructor(NativeFuncInData) {
    ClassId classId = args[0]->i;
    const std::string& path = args[1]->str->data;
    int64_t modeInt = args[2]->i;

    const char* cMode;
    switch (modeInt) {
        case 0: cMode = "rb"; break;
        case 1: cMode = "wb"; break;
        case 2: cMode = "ab"; break;
        case 3: cMode = "r+b"; break;
        case 4: cMode = "w+b"; break;
        case 5: cMode = "a+b"; break;
        default:
            notifier.throwException("Invalid FileMode");
            return nullptr;
    }

    FILE* fp = fopen(path.c_str(), cMode);
    if (!fp) {
        notifier.throwException("Cannot open file: " + path);
        return nullptr;
    }

    auto handle = new AFileHandle{fp};
    return notifier.createNativeData(classId, handle, destroyFile);
}

inline AObject *read_text(NativeFuncInData) {
    auto handle = static_cast<AFileHandle *>(args[0]->data->data);
    if (!handle->fp) {
        notifier.throwException("File is closed");
        return nullptr;
    }

    // Tối ưu: Tính toán size chính xác bằng hàm 64-bit
    FSEEK(handle->fp, 0, SEEK_END);
    int64_t size = FTELL(handle->fp);
    FSEEK(handle->fp, 0, SEEK_SET);

    std::string buffer;
    if (size > 0) {
        // Tối ưu: Chỉ resize 1 lần, fread block trực tiếp vào memory
        buffer.resize(size);
        fread(buffer.data(), 1, size, handle->fp);
    }
    
    return notifier.createString(buffer);
}

inline AObject *for_each_line(NativeFuncInData) {
    auto handle = static_cast<AFileHandle *>(args[0]->data->data);
    auto funcObject = args[1];

    if (!handle->fp) {
        notifier.throwException("File is closed");
        return nullptr;
    }

    FSEEK(handle->fp, 0, SEEK_SET);

    // Tối ưu: Dùng buffer tĩnh và tránh tạo String rác liên tục
    char buf[4096];
    std::string line;
    
    while (fgets(buf, sizeof(buf), handle->fp)) {
        size_t len = strlen(buf);
        
        // Cắt bỏ ký tự xuống dòng
        if (len > 0 && buf[len - 1] == '\n') {
            line.append(buf, len - 1);
            if (!line.empty() && line.back() == '\r') {
                line.pop_back(); // Hỗ trợ định dạng CRLF của Windows
            }
            
            auto lineObj = notifier.createString(line);
            notifier.callFunctionObject(funcObject, lineObj);
            if (notifier.hasException()) return nullptr;
            
            line.clear();
        } else {
            // Trường hợp dòng dài hơn 4096 byte
            line.append(buf, len);
        }
    }

    // Xử lý dòng cuối không có ký tự kết thúc \n
    if (!line.empty()) {
        auto lineObj = notifier.createString(line);
        notifier.callFunctionObject(funcObject, lineObj);
    }

    return nullptr;
}

inline AObject *write(NativeFuncInData) {
    auto handle = static_cast<AFileHandle *>(args[0]->data->data);
    if (!handle->fp) {
        notifier.throwException("File is closed");
        return nullptr;
    }

    // Tối ưu cực hạn: Viết trực tiếp, KHÔNG flush liên tục để tận dụng OS Buffer
    const std::string& data = args[1]->str->data;
    if (!data.empty()) {
        fwrite(data.data(), 1, data.size(), handle->fp);
    }
    return nullptr;
}

inline AObject *seek(NativeFuncInData) {
    auto handle = static_cast<AFileHandle *>(args[0]->data->data);
    if (!handle->fp) return nullptr;

    int64_t pos = args[1]->i;
    FSEEK(handle->fp, pos, SEEK_SET);
    return nullptr;
}

inline AObject *close(NativeFuncInData) {
    auto handle = static_cast<AFileHandle *>(args[0]->data->data);
    if (handle->fp) {
        fclose(handle->fp);
        handle->fp = nullptr; // Đảm bảo an toàn tránh use-after-close
    }
    return nullptr;
}

inline AObject *exists(NativeFuncInData) {
    const std::string &path = args[0]->str->data;
    STAT_STRUCT stat_buf;
    bool result = (STAT_FUNC(path.c_str(), &stat_buf) == 0);
    return notifier.createBool(result);
}

inline AObject *delete_file(NativeFuncInData) {
    const std::string &path = args[0]->str->data;
    bool success = (std::remove(path.c_str()) == 0);
    return notifier.createBool(success);
}

inline AObject *get_parent(NativeFuncInData) {
    const std::string &path = args[0]->str->data;
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
    const std::string &path = args[0]->str->data;
    std::error_code ec;
    std::string absPath = std::filesystem::absolute(std::filesystem::path(path), ec).string();
    if (ec) {
        notifier.throwException("Invalid path: " + path);
        return nullptr;
    }
    return notifier.createString(absPath);
}

inline AObject *is_directory(NativeFuncInData) {
    const std::string &path = args[0]->str->data;
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
    const std::string &path = args[0]->str->data;
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
    const std::string &path = args[0]->str->data;
    ClassId arrayClassId = args[1]->i;

    auto newArr = notifier.createArray(arrayClassId);
    std::error_code ec;

    if (std::filesystem::exists(path, ec) && std::filesystem::is_directory(path, ec)) {
        for (const auto& entry : std::filesystem::directory_iterator(path, ec)) {
            if (ec) {
                notifier.throwException("Filesystem error while reading directory: " + ec.message());
                return nullptr;
            }
            notifier.arrayAdd(newArr, notifier.createString(entry.path().string()));
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
    const std::string &path = args[0]->str->data;
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
    const std::string &path = args[0]->str->data;
    STAT_STRUCT stat_buf;
    
    if (STAT_FUNC(path.c_str(), &stat_buf) == 0) {
        return notifier.createInt(static_cast<int64_t>(stat_buf.st_size));
    }

    notifier.throwException("Cannot get file size (might be a directory or not exist): " + path);
    return nullptr;
}

inline AObject *get_extension(NativeFuncInData) {
    const std::string &path = args[0]->str->data;
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
    const std::string &path = args[0]->str->data;
    STAT_STRUCT stat_buf;

    if (STAT_FUNC(path.c_str(), &stat_buf) == 0) {
        return notifier.createInt(static_cast<int64_t>(stat_buf.st_mtime));
    }

    notifier.throwException("Cannot get last modified time for: " + path);
    return nullptr;
}

void init(ACompiler &compiler) {
    compiler.registerFromSource(
        "std/file", R"###(
enum FileMode {
    READ,          
    WRITE,         
    APPEND,        
    READ_WRITE,    
    WRITE_READ,    
    APPEND_READ;   

    func getId(): Int = when (this) {
        READ -> 0
        WRITE -> 1
        APPEND -> 2
        READ_WRITE -> 3
        WRITE_READ -> 4
        APPEND_READ -> 5
        else -> -1
    }

    func toString(): String = when (this) {
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
    private static func __CLASS__(classId: Int, path: String, modeId: Int): File

    static func __CLASS__(path: String, mode: FileMode): File = File(getClassId(File), path, mode.getId())

    @native("file_read_text")
    func readText(): String

    @native("file_for_each_line")
    func forEachLine(fn: (String) -> Void)

    @native("file_write")
    func write(text: String)

    @native("file_close")
    func close()

    @native("file_seek")
    func seek(position: Int)

    @native("file_exists")
    static func exists(path: String): Bool

    @native("file_delete")
    static func delete(path: String): Bool

    @native("file_get_parent")
    static func getParent(path: String): String

    @native("file_get_absolute_path")
    static func getAbsolutePath(path: String): String

    @native("file_is_directory")
    static func isDirectory(path: String): Bool

    @native("file_is_file")
    static func isFile(path: String): Bool
    
    @native("file_get_all_files")
    static func getAllFiles(dirPath: String, arrayClassId: Int = getClassId(Array<String>)): Array<String>

    @native("file_get_name")
    static func getName(path: String): String

    @native("file_get_size")
    static func getSize(path: String): Int

    @native("file_get_extension")
    static func getExtension(path: String): String
    
    //Since 1970
    @native("file_get_last_modified")
    static func getLastModified(path: String): Int
}
    )###",
        false,
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
} // namespace AutoLang
#endif
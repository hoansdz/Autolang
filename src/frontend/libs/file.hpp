#ifndef LIB_FILE_HPP
#define LIB_FILE_HPP

#include "shared/Type.hpp"

namespace Autolang {
class ACompiler;

namespace Libs {
namespace file {

void init(Autolang::ACompiler &compiler);
AObject *constructor(NativeFuncInData);
AObject *read_text(NativeFuncInData);
AObject *for_each_line(NativeFuncInData);
AObject *write(NativeFuncInData);
AObject *close(NativeFuncInData);
AObject *seek(NativeFuncInData);
AObject *exists(NativeFuncInData);
AObject *delete_file(NativeFuncInData);
AObject *get_parent(NativeFuncInData);
AObject *get_absolute_path(NativeFuncInData);
AObject *is_directory(NativeFuncInData);
AObject *is_file(NativeFuncInData);
AObject *get_all_files(NativeFuncInData);
AObject *get_name(NativeFuncInData);
AObject *get_size(NativeFuncInData);
AObject *get_last_modified(NativeFuncInData);
AObject *read_bytes(NativeFuncInData);
AObject *static_read_bytes(NativeFuncInData);
AObject *write_bytes(NativeFuncInData);
AObject *static_write_bytes(NativeFuncInData);
AObject *copy_to(NativeFuncInData);
AObject *copy_recursively(NativeFuncInData);


} // namespace file
} // namespace Libs
} // namespace Autolang
#endif
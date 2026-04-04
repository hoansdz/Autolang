#ifndef LIBS_VM_HPP
#define LIBS_VM_HPP

#include "shared/Type.hpp"

namespace AutoLang {
class ACompiler;
namespace Libs {
namespace vm {

void init(ACompiler &compiler);
}
} // namespace Libs
} // namespace AutoLang

#endif
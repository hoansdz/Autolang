#ifndef LIBS_DEBUGGER_HPP
#define LIBS_DEBUGGER_HPP

#include "shared/Type.hpp"

namespace AutoLang {
class ACompiler;
namespace Libs {
namespace Debugger {
void init(ACompiler &compiler);

AObject *getClassName(NativeFuncInData);
AObject *getClassId(NativeFuncInData);

} // namespace Debugger
} // namespace Libs
} // namespace AutoLang

#endif
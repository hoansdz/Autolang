#ifndef LIBS_STDLIB_HPP
#define LIBS_STDLIB_HPP

namespace AutoLang {
class ACompiler;
namespace Libs {
namespace stdlib {
void init(ACompiler &compiler);
int64_t now_ns() {
    auto now = Clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();
}
}
} // namespace Libs
} // namespace AutoLang

#endif
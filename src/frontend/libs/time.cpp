#ifndef LIBS_TIME_CPP
#define LIBS_TIME_CPP

#include "frontend/ACompiler.hpp"
#include "array.hpp"
#include <chrono>
#include <string>

namespace AutoLang {
class ACompiler;
namespace Libs {
namespace time {

AObject *now(NativeFuncInData) {
	auto now = std::chrono::system_clock::now();
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch())
	              .count();

	return notifier.createInt(ms);
}

void init(ACompiler &compiler) {
	compiler.registerFromSource("std/time", R"###(
@no_constructor
class Time {
	@native("now")
	static func now(): Int
}
	)###",
	                            true,
	                            ANativeMap({
	                                {"now", &now},
	                            }));
}

} // namespace time
} // namespace Libs
} // namespace AutoLang

#endif

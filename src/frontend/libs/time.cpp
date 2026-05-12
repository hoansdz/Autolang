#ifndef LIBS_TIME_CPP
#define LIBS_TIME_CPP

#include "frontend/ACompiler.hpp"
#include "shared/DefaultOperator.hpp"
#include <chrono>
#include <ctime>
#include <string>

namespace AutoLang {
class ACompiler;
namespace Libs {
namespace time {

inline AObject *now(NativeFuncInData) {
	auto now = std::chrono::system_clock::now();
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
	              now.time_since_epoch())
	              .count();

	return notifier.createInt(ms);
}

inline AObject *format(NativeFuncInData) {

	int64_t ms = args[0]->i;

	std::time_t time_sec = static_cast<std::time_t>(ms / 1000);

	std::tm *tm_info = std::localtime(&time_sec);

	char buffer[80];
	std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_info);

	return notifier.createString(std::string(buffer));
}

void init(ACompiler &compiler) {
	compiler.registerBuiltInLibrary("std/time", R"###(
@no_constructor
class Time {
    @native("now")
    static func now(): Int

    @native("time_format")
    static func format(timestamp: Int): String
}
    )###",
	                                LibraryConfig(),
	                                ANativeMap({
	                                    {"now", &now},
	                                    {"time_format", &format},
	                                }));
}

} // namespace time
} // namespace Libs
} // namespace AutoLang

#endif
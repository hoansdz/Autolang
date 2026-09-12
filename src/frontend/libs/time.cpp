#ifndef LIBS_TIME_CPP
#define LIBS_TIME_CPP

#include "frontend/ACompiler.hpp"
#include "shared/DefaultOperator.hpp"
#include <chrono>
#include <ctime>
#include <string>
#include <thread>
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace Autolang {
class ACompiler;
namespace Libs {
namespace time {

AObject *now(NativeFuncInData) {
	auto now = std::chrono::system_clock::now();
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
	              now.time_since_epoch())
	              .count();

	return notifier.createInt(ms);
}

AObject *now_seconds(NativeFuncInData) {
	auto now = std::chrono::system_clock::now();
	auto sec = std::chrono::duration_cast<std::chrono::seconds>(
	               now.time_since_epoch())
	               .count();

	return notifier.createInt(sec);
}

AObject *now_nanos(NativeFuncInData) {
	auto now = std::chrono::high_resolution_clock::now();
	auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
	              now.time_since_epoch())
	              .count();

	return notifier.createInt(ns);
}

AObject *sleep_ms(NativeFuncInData) {
	int64_t ms = args[0]->i;
	if (ms > 0) {
#ifdef _WIN32
		Sleep(static_cast<DWORD>(ms));
#else
		std::this_thread::sleep_for(std::chrono::milliseconds(ms));
#endif
	}
	return nullptr;
}

AObject *format(NativeFuncInData) {
	int64_t ms = args[0]->i;
	const char *pattern = "%Y-%m-%d %H:%M:%S";
	if (argSize > 1 && args[1] && args[1]->str) {
		pattern = args[1]->str->data;
	}

	std::time_t time_sec = static_cast<std::time_t>(ms / 1000);
	std::tm tm_info;
#ifdef _WIN32
	localtime_s(&tm_info, &time_sec);
#else
	localtime_r(&time_sec, &tm_info);
#endif

	char buffer[128];
	std::strftime(buffer, sizeof(buffer), pattern, &tm_info);

	return notifier.createString(std::string(buffer));
}

void init(ACompiler &compiler) {
	compiler.registerBuiltInLibrary("std/time", R"###(
@no_constructor
class Time {
    @native("now")
    static fun now(): Int

    @native("now")
    static fun nowMs(): Int

    @native("now_seconds")
    static fun nowSeconds(): Int

    @native("now")
    static fun currentTimeMillis(): Int

    @native("now_nanos")
    static fun nowNanos(): Int

    @native("now_nanos")
    static fun nanoTime(): Int

    @native("time_sleep")
    static fun sleep(ms: Int)

    @native("time_format")
    static fun format(timestamp: Int, pattern: String = "%Y-%m-%d %H:%M:%S"): String

    static fun measureTimeMillis(block: () -> Void): Int {
        val start = Time.now()
        block()
        return Time.now() - start
    }
}

@native("time_sleep")
fun sleep(ms: Int)

fun measureTimeMillis(block: () -> Void): Int {
    val start = Time.now()
    block()
    return Time.now() - start
}
    )###",
	                                LibraryConfig(),
	                                ANativeMap({
	                                    {"now", &now},
	                                    {"now_seconds", &now_seconds},
	                                    {"now_nanos", &now_nanos},
	                                    {"time_sleep", &sleep_ms},
	                                    {"time_format", &format},
	                                }));
}

} // namespace time
} // namespace Libs
} // namespace Autolang

#endif

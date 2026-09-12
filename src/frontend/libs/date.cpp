#ifndef LIB_DATE_CPP
#define LIB_DATE_CPP

#include "date.hpp"
#include "frontend/ACompiler.hpp"
#include "shared/DefaultOperator.hpp"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace Autolang {
class ACompiler;

namespace Libs {
namespace date {

constexpr int64_t MS_PER_SECOND = 1000LL;
constexpr int64_t MS_PER_MINUTE = 60LL * MS_PER_SECOND;
constexpr int64_t MS_PER_HOUR = 60LL * MS_PER_MINUTE;
constexpr int64_t MS_PER_DAY = 24LL * MS_PER_HOUR;

struct ADateHandle {
	int64_t timestamp_ms = 0;
};

static void destroyDate(ANotifier &notifier, void *dateData) {
	auto handle = static_cast<ADateHandle *>(dateData);
	if (handle) {
		delete handle;
	}
}

inline bool getTm(int64_t timestamp_ms, std::tm &out_tm) {
	std::time_t t = timestamp_ms / 1000;
#ifdef _WIN32
	return localtime_s(&out_tm, &t) == 0;
#else
	return localtime_r(&t, &out_tm) != nullptr;
#endif
}

inline int64_t getCurrentTimeMs() {
	auto now = std::chrono::system_clock::now();
	return std::chrono::duration_cast<std::chrono::milliseconds>(
	           now.time_since_epoch())
	    .count();
}

AObject *constructor_now(NativeFuncInData) {
	ClassId classId = notifier.callFrame->func->returnId;
	auto handle = new ADateHandle{getCurrentTimeMs()};
	return notifier.createNativeData(classId, handle, destroyDate);
}

AObject *constructor_ts(NativeFuncInData) {
	int64_t ts = args[0]->i;
	ClassId classId = notifier.callFrame->func->returnId;
	auto handle = new ADateHandle{ts};
	return notifier.createNativeData(classId, handle, destroyDate);
}

#define GET_VALID_TM_OR_RETURN_NULL(handle_ptr, tm_var)                        \
	auto handle = static_cast<ADateHandle *>(handle_ptr);                      \
	if (!handle) {                                                             \
		notifier.throwException("Date instance is null or uninitialized");     \
		return nullptr;                                                        \
	}                                                                          \
	std::tm tm_var;                                                            \
	if (!getTm(handle->timestamp_ms, tm_var)) {                                \
		notifier.throwException("Invalid or out-of-range timestamp");          \
		return nullptr;                                                        \
	}

AObject *get_year(NativeFuncInData) {
	GET_VALID_TM_OR_RETURN_NULL(args[0]->data->data, tm);
	return notifier.createInt(tm.tm_year + 1900);
}

AObject *get_month(NativeFuncInData) {
	GET_VALID_TM_OR_RETURN_NULL(args[0]->data->data, tm);
	return notifier.createInt(tm.tm_mon + 1);
}

AObject *get_day(NativeFuncInData) {
	GET_VALID_TM_OR_RETURN_NULL(args[0]->data->data, tm);
	return notifier.createInt(tm.tm_mday);
}

AObject *get_hours(NativeFuncInData) {
	GET_VALID_TM_OR_RETURN_NULL(args[0]->data->data, tm);
	return notifier.createInt(tm.tm_hour);
}

AObject *get_minutes(NativeFuncInData) {
	GET_VALID_TM_OR_RETURN_NULL(args[0]->data->data, tm);
	return notifier.createInt(tm.tm_min);
}

AObject *get_seconds(NativeFuncInData) {
	GET_VALID_TM_OR_RETURN_NULL(args[0]->data->data, tm);
	return notifier.createInt(tm.tm_sec);
}

AObject *get_time(NativeFuncInData) {
	auto handle = static_cast<ADateHandle *>(args[0]->data->data);
	if (!handle) {
		notifier.throwException("Date instance is null or uninitialized");
		return nullptr;
	}
	return notifier.createInt(handle->timestamp_ms);
}

AObject *format(NativeFuncInData) {
	GET_VALID_TM_OR_RETURN_NULL(args[0]->data->data, tm);
	const char *pattern = "%Y-%m-%d %H:%M:%S";
	if (argSize > 1 && args[1] && args[1]->str) {
		pattern = args[1]->str->data;
	}

	std::ostringstream ss;
	ss << std::put_time(&tm, pattern);

	if (ss.fail()) {
		notifier.throwException("Failed to format date with provided pattern");
		return nullptr;
	}

	return notifier.createString(ss.str());
}

AObject *get_day_of_week(NativeFuncInData) {
	GET_VALID_TM_OR_RETURN_NULL(args[0]->data->data, tm);
	return notifier.createInt(tm.tm_wday);
}

AObject *get_day_of_year(NativeFuncInData) {
	GET_VALID_TM_OR_RETURN_NULL(args[0]->data->data, tm);
	return notifier.createInt(tm.tm_yday + 1);
}

AObject *date_parse(NativeFuncInData) {
	const std::string &dateStr = args[0]->str->data;
	const char *pattern = "%Y-%m-%d %H:%M:%S";
	if (argSize > 1 && args[1] && args[1]->str) {
		pattern = args[1]->str->data;
	}

	std::tm tm{};
	std::istringstream ss(dateStr);
	ss >> std::get_time(&tm, pattern);
	if (ss.fail()) {
		notifier.throwException("Failed to parse date string: " + dateStr);
		return nullptr;
	}

	std::time_t t = std::mktime(&tm);
	if (t == -1) {
		notifier.throwException("Failed to convert date to timestamp: " + dateStr);
		return nullptr;
	}

	ClassId classId = notifier.callFrame->func->returnId;
	auto handle = new ADateHandle{static_cast<int64_t>(t) * 1000};
	return notifier.createNativeData(classId, handle, destroyDate);
}

AObject *current_time_millis(NativeFuncInData) {
	return notifier.createInt(getCurrentTimeMs());
}

AObject *add_days(NativeFuncInData) {
	auto handle = static_cast<ADateHandle *>(args[0]->data->data);
	if (!handle) {
		notifier.throwException("Date instance is null");
		return nullptr;
	}
	int64_t days = args[1]->i;
	handle->timestamp_ms += days * MS_PER_DAY;
	return args[0];
}

AObject *add_hours(NativeFuncInData) {
	auto handle = static_cast<ADateHandle *>(args[0]->data->data);
	if (!handle) {
		notifier.throwException("Date instance is null");
		return nullptr;
	}
	int64_t hours = args[1]->i;
	handle->timestamp_ms += hours * MS_PER_HOUR;
	return args[0];
}

AObject *add_minutes(NativeFuncInData) {
	auto handle = static_cast<ADateHandle *>(args[0]->data->data);
	if (!handle) {
		notifier.throwException("Date instance is null");
		return nullptr;
	}
	int64_t minutes = args[1]->i;
	handle->timestamp_ms += minutes * MS_PER_MINUTE;
	return args[0];
}

AObject *add_seconds(NativeFuncInData) {
	auto handle = static_cast<ADateHandle *>(args[0]->data->data);
	if (!handle) {
		notifier.throwException("Date instance is null");
		return nullptr;
	}
	int64_t seconds = args[1]->i;
	handle->timestamp_ms += seconds * MS_PER_SECOND;
	return args[0];
}

AObject *is_leap_year(NativeFuncInData) {
	GET_VALID_TM_OR_RETURN_NULL(args[0]->data->data, tm);
	int year = tm.tm_year + 1900;

	bool isLeap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
	return notifier.createBool(isLeap);
}

void init(ACompiler &compiler) {
	compiler.registerBuiltInLibrary(
	    "std/date", R"###(
@no_constructor
@no_extends
class Date {
    
    @native("date_constructor_now")
    static fun now(): Date

    @native("date_constructor_ts")
    static fun fromTimestamp(timestamp: Int): Date

    static fun Date(): Date = Date.now()
    static fun Date(timestamp: Int): Date = Date.fromTimestamp(timestamp)

    @native("date_get_year")
    fun getYear(): Int

    @native("date_get_year")
    fun year(): Int

    @native("date_get_month")
    fun getMonth(): Int

    @native("date_get_month")
    fun month(): Int

    @native("date_get_day")
    fun getDay(): Int

    @native("date_get_day")
    fun day(): Int

    @native("date_get_hours")
    fun getHours(): Int

    @native("date_get_hours")
    fun hours(): Int

    @native("date_get_hours")
    fun hour(): Int

    @native("date_get_minutes")
    fun getMinutes(): Int

    @native("date_get_minutes")
    fun minutes(): Int

    @native("date_get_minutes")
    fun minute(): Int

    @native("date_get_seconds")
    fun getSeconds(): Int

    @native("date_get_seconds")
    fun seconds(): Int

    @native("date_get_seconds")
    fun second(): Int

    @native("date_get_time")
    fun getTime(): Int

    @native("date_get_time")
    fun timestamp(): Int

    @native("date_get_time")
    fun time(): Int

    @native("date_get_time")
    fun toEpochMilli(): Int
    
    @native("date_get_day_of_week")
    fun getDayOfWeek(): Int

    @native("date_get_day_of_week")
    fun dayOfWeek(): Int

    @native("date_get_day_of_year")
    fun getDayOfYear(): Int

    @native("date_get_day_of_year")
    fun dayOfYear(): Int

    @native("date_parse")
    static fun parse(dateStr: String, pattern: String = "%Y-%m-%d %H:%M:%S"): Date

    @native("date_format")
    fun format(pattern: String = "%Y-%m-%d %H:%M:%S"): String

    fun toString(): String = this.format("%Y-%m-%d %H:%M:%S")
    fun toISOString(): String = this.format("%Y-%m-%dT%H:%M:%S")
    fun toDateString(): String = this.format("%Y-%m-%d")
    fun toTimeString(): String = this.format("%H:%M:%S")

    fun diff(other: Date): Int = this.getTime() - other.getTime()
    fun isBefore(other: Date): Bool = this.getTime() < other.getTime()
    fun isAfter(other: Date): Bool = this.getTime() > other.getTime()

    fun copy(): Date = Date.fromTimestamp(this.getTime())
    fun clone(): Date = Date.fromTimestamp(this.getTime())

    @native("date_current_time_millis")
    static fun currentTimeMillis(): Int

    @native("date_current_time_millis")
    static fun nowMillis(): Int

    @native("date_current_time_millis")
    static fun nowMs(): Int

    @native("date_add_days")
    fun addDays(days: Int): Date

    @native("date_add_hours")
    fun addHours(hours: Int): Date

    @native("date_add_minutes")
    fun addMinutes(minutes: Int): Date

    @native("date_add_seconds")
    fun addSeconds(seconds: Int): Date

    @native("date_is_leap_year")
    fun isLeapYear(): Bool

    @native("date_is_leap_year")
    fun leapYear(): Bool
}
    )###",
	    LibraryConfig(),
	    ANativeMap({
	        {"date_constructor_now", &date::constructor_now},
	        {"date_constructor_ts", &date::constructor_ts},
	        {"date_get_year", &date::get_year},
	        {"date_get_month", &date::get_month},
	        {"date_get_day", &date::get_day},
	        {"date_get_day_of_week", &date::get_day_of_week},
	        {"date_get_day_of_year", &date::get_day_of_year},
	        {"date_get_hours", &date::get_hours},
	        {"date_get_minutes", &date::get_minutes},
	        {"date_get_seconds", &date::get_seconds},
	        {"date_get_time", &date::get_time},
	        {"date_format", &date::format},
	        {"date_parse", &date::date_parse},
	        {"date_current_time_millis", &date::current_time_millis},
	        {"date_add_days", &date::add_days},
	        {"date_add_hours", &date::add_hours},
	        {"date_add_minutes", &date::add_minutes},
	        {"date_add_seconds", &date::add_seconds},
	        {"date_is_leap_year", &date::is_leap_year},
	    }));
}

} // namespace date
} // namespace Libs
} // namespace Autolang
#endif

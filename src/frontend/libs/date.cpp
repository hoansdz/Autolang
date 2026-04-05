#ifndef LIB_DATE_CPP
#define LIB_DATE_CPP

#include "date.hpp"
#include "frontend/ACompiler.hpp"
#include "shared/DefaultOperator.hpp"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace AutoLang {
class ACompiler;

namespace Libs {
namespace date {

// Các hằng số thời gian (milliseconds)
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

// Hàm hỗ trợ: Chuyển đổi an toàn, trả về false nếu timestamp không hợp lệ
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

inline AObject *constructor_now(NativeFuncInData) {
	ClassId classId = args[0]->i;
	auto handle = new ADateHandle{getCurrentTimeMs()};
	return notifier.createNativeData(classId, handle, destroyDate);
}

inline AObject *constructor_ts(NativeFuncInData) {
	int64_t ts = args[0]->i;
	ClassId classId = args[1]->i;
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

inline AObject *get_year(NativeFuncInData) {
	GET_VALID_TM_OR_RETURN_NULL(args[0]->data->data, tm);
	return notifier.createInt(tm.tm_year + 1900);
}

inline AObject *get_month(NativeFuncInData) {
	GET_VALID_TM_OR_RETURN_NULL(args[0]->data->data, tm);
	return notifier.createInt(tm.tm_mon + 1);
}

inline AObject *get_day(NativeFuncInData) {
	GET_VALID_TM_OR_RETURN_NULL(args[0]->data->data, tm);
	return notifier.createInt(tm.tm_mday);
}

inline AObject *get_hours(NativeFuncInData) {
	GET_VALID_TM_OR_RETURN_NULL(args[0]->data->data, tm);
	return notifier.createInt(tm.tm_hour);
}

inline AObject *get_minutes(NativeFuncInData) {
	GET_VALID_TM_OR_RETURN_NULL(args[0]->data->data, tm);
	return notifier.createInt(tm.tm_min);
}

inline AObject *get_seconds(NativeFuncInData) {
	GET_VALID_TM_OR_RETURN_NULL(args[0]->data->data, tm);
	return notifier.createInt(tm.tm_sec);
}

inline AObject *get_time(NativeFuncInData) {
	auto handle = static_cast<ADateHandle *>(args[0]->data->data);
	if (!handle) {
		notifier.throwException("Date instance is null or uninitialized");
		return nullptr;
	}
	return notifier.createInt(handle->timestamp_ms);
}

inline AObject *format(NativeFuncInData) {
	GET_VALID_TM_OR_RETURN_NULL(args[0]->data->data, tm);
	const std::string &pattern = args[1]->str->data;

	std::ostringstream ss;
	ss << std::put_time(&tm, pattern.c_str());

	if (ss.fail()) {
		notifier.throwException("Failed to format date with provided pattern");
		return nullptr;
	}

	return notifier.createString(ss.str());
}

inline AObject *current_time_millis(NativeFuncInData) {
	return notifier.createInt(getCurrentTimeMs());
}

// --- CÁC HÀM TÍNH TOÁN THỜI GIAN ---

inline AObject *add_days(NativeFuncInData) {
	auto handle = static_cast<ADateHandle *>(args[0]->data->data);
	if (!handle) {
		notifier.throwException("Date instance is null");
		return nullptr;
	}
	int64_t days = args[1]->i;
	handle->timestamp_ms += days * MS_PER_DAY;
	return args[0]; // Trả về chính object Date để chaining
}

inline AObject *add_hours(NativeFuncInData) {
	auto handle = static_cast<ADateHandle *>(args[0]->data->data);
	if (!handle) {
		notifier.throwException("Date instance is null");
		return nullptr;
	}
	int64_t hours = args[1]->i;
	handle->timestamp_ms += hours * MS_PER_HOUR;
	return args[0];
}

inline AObject *add_minutes(NativeFuncInData) {
	auto handle = static_cast<ADateHandle *>(args[0]->data->data);
	if (!handle) {
		notifier.throwException("Date instance is null");
		return nullptr;
	}
	int64_t minutes = args[1]->i;
	handle->timestamp_ms += minutes * MS_PER_MINUTE;
	return args[0];
}

inline AObject *add_seconds(NativeFuncInData) {
	auto handle = static_cast<ADateHandle *>(args[0]->data->data);
	if (!handle) {
		notifier.throwException("Date instance is null");
		return nullptr;
	}
	int64_t seconds = args[1]->i;
	handle->timestamp_ms += seconds * MS_PER_SECOND;
	return args[0];
}

inline AObject *is_leap_year(NativeFuncInData) {
	GET_VALID_TM_OR_RETURN_NULL(args[0]->data->data, tm);
	int year = tm.tm_year + 1900;
	// Năm nhuận: chia hết cho 4 nhưng không chia hết cho 100, HOẶC chia hết cho
	// 400
	bool isLeap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
	return notifier.createBool(isLeap);
}

void init(ACompiler &compiler) {
	compiler.registerFromSource(
	    "std/date", R"###(
@no_constructor
@no_extends
class Date {
    
    @native("date_constructor_now")
    static func now(classId: Int = getClassId(Date)): Date

    @native("date_constructor_ts")
    static func fromTimestamp(timestamp: Int, classId: Int = getClassId(Date)): Date

    @native("date_get_year")
    func getYear(): Int

    @native("date_get_month")
    func getMonth(): Int

    @native("date_get_day")
    func getDay(): Int

    @native("date_get_hours")
    func getHours(): Int

    @native("date_get_minutes")
    func getMinutes(): Int

    @native("date_get_seconds")
    func getSeconds(): Int

    @native("date_get_time")
    func getTime(): Int

    // Format string C++ ("%Y-%m-%d %H:%M:%S")
    @native("date_format")
    func format(pattern: String): String

    @native("date_current_time_millis")
    static func currentTimeMillis(): Int

    @native("date_add_days")
    func addDays(days: Int): Date

    @native("date_add_hours")
    func addHours(hours: Int): Date

    @native("date_add_minutes")
    func addMinutes(minutes: Int): Date

    @native("date_add_seconds")
    func addSeconds(seconds: Int): Date

    @native("date_is_leap_year")
    func isLeapYear(): Bool
}
    )###",
	    false,
	    ANativeMap({
	        {"date_constructor_now", &date::constructor_now},
	        {"date_constructor_ts", &date::constructor_ts},
	        {"date_get_year", &date::get_year},
	        {"date_get_month", &date::get_month},
	        {"date_get_day", &date::get_day},
	        {"date_get_hours", &date::get_hours},
	        {"date_get_minutes", &date::get_minutes},
	        {"date_get_seconds", &date::get_seconds},
	        {"date_get_time", &date::get_time},
	        {"date_format", &date::format},
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
} // namespace AutoLang
#endif
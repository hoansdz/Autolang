#ifndef LIB_REGEX_CPP
#define LIB_REGEX_CPP

#include "regex.hpp"
#include "frontend/ACompiler.hpp"
#include "shared/DefaultOperator.hpp"
#include <regex>
#include <string>

namespace AutoLang {
class ACompiler;

namespace Libs {
namespace regex {

struct ARegexHandle {
	std::regex re;
};

static void destroyRegex(ANotifier &notifier, void *regexData) {
	auto handle = static_cast<ARegexHandle *>(regexData);
	if (handle) {
		delete handle;
	}
}

inline AObject *constructor(NativeFuncInData) {
	ClassId classId = args[0]->i;
	const std::string &pattern = args[1]->str->data;

	try {

		auto handle = new ARegexHandle{std::regex(pattern)};
		return notifier.createNativeData(classId, handle, destroyRegex);
	} catch (const std::regex_error &e) {

		notifier.throwException(std::string("Invalid Regex Pattern: ") +
		                        e.what());
		return nullptr;
	}
}

#define GET_VALID_REGEX_OR_RETURN_NULL(handle_ptr, re_var)                     \
	auto handle = static_cast<ARegexHandle *>(handle_ptr);                     \
	if (!handle) {                                                             \
		notifier.throwException("Regex instance is null or uninitialized");    \
		return nullptr;                                                        \
	}                                                                          \
	std::regex &re_var = handle->re;

inline AObject *is_match(NativeFuncInData) {
	GET_VALID_REGEX_OR_RETURN_NULL(args[0]->data->data, re);
	const std::string &text = args[1]->str->data;

	bool result = std::regex_search(text, re);
	return notifier.createBool(result);
}

inline AObject *find_all(NativeFuncInData) {
	GET_VALID_REGEX_OR_RETURN_NULL(args[0]->data->data, re);
	const std::string &text = args[1]->str->data;
	ClassId arrayClassId = args[2]->i;

	auto newArr = notifier.createArray(arrayClassId);

	auto words_begin = std::sregex_iterator(text.begin(), text.end(), re);
	auto words_end = std::sregex_iterator();

	for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
		std::smatch match = *i;
		notifier.arrayAdd(newArr, notifier.createString(match.str()));
	}

	return newArr;
}

inline AObject *replace(NativeFuncInData) {
	GET_VALID_REGEX_OR_RETURN_NULL(args[0]->data->data, re);
	const std::string &text = args[1]->str->data;
	const std::string &replacement = args[2]->str->data;

	std::string result = std::regex_replace(text, re, replacement);
	return notifier.createString(result);
}

void init(ACompiler &compiler) {
	auto nativeMap = ANativeMap();
	nativeMap.reserve(5);

	nativeMap.emplace("regex_constructor", &regex::constructor);
	nativeMap.emplace("regex_is_match", &regex::is_match);
	nativeMap.emplace("regex_find_all", &regex::find_all);
	nativeMap.emplace("regex_replace", &regex::replace);

	compiler.registerBuiltInLibrary("std/regex", R"###(
@no_constructor
@no_extends
class Regex {
    
    @native("regex_constructor")
    private static func _create(classId: Int, pattern: String): Regex
    
    static func compile(pattern: String): Regex = _create(getClassId(Regex), pattern)

    @native("regex_is_match")
    func isMatch(text: String): Bool

    @native("regex_find_all")
    func findAll(text: String, arrayClassId: Int = getClassId(Array<String>)): Array<String>

    @native("regex_replace")
    func replace(text: String, replacement: String): String
}
    )###",
	                                LibraryConfig(), std::move(nativeMap));
}

} // namespace regex
} // namespace Libs
} // namespace AutoLang
#endif
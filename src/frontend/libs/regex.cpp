#ifndef LIB_REGEX_CPP
#define LIB_REGEX_CPP

#include "regex.hpp"
#include "frontend/ACompiler.hpp"
#include "shared/DefaultOperator.hpp"
#include <regex>
#include <string>

namespace Autolang {
class ACompiler;

namespace Libs {
namespace regex {

struct ARegexHandle {
	std::regex re;
	std::string pattern;
};

static void destroyRegex(ANotifier &notifier, void *regexData) {
	auto handle = static_cast<ARegexHandle *>(regexData);
	if (handle) {
		notifier.addManagedMemory(-128);
		delete handle;
	}
}

AObject *constructor(NativeFuncInData) {
	ClassId classId = args[0]->i;
	const std::string &pattern = args[1]->str->data;

	try {
		auto handle = new ARegexHandle{std::regex(pattern), pattern};
		notifier.addManagedMemory(128);
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

AObject *get_pattern(NativeFuncInData) {
	auto handle = static_cast<ARegexHandle *>(args[0]->data->data);
	if (!handle) {
		notifier.throwException("Regex instance is null or uninitialized");
		return nullptr;
	}
	return notifier.createString(handle->pattern);
}

AObject *is_match(NativeFuncInData) {
	GET_VALID_REGEX_OR_RETURN_NULL(args[0]->data->data, re);
	const std::string &text = args[1]->str->data;

	bool result = std::regex_search(text, re);
	return notifier.createBool(result);
}

AObject *find(NativeFuncInData) {
	GET_VALID_REGEX_OR_RETURN_NULL(args[0]->data->data, re);
	const std::string &text = args[1]->str->data;
	std::smatch m;
	if (std::regex_search(text, m, re)) {
		return notifier.createString(m.str());
	}
	return notifier.createString("");
}

AObject *find_all(NativeFuncInData) {
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

AObject *replace(NativeFuncInData) {
	GET_VALID_REGEX_OR_RETURN_NULL(args[0]->data->data, re);
	const std::string &text = args[1]->str->data;
	const std::string &replacement = args[2]->str->data;

	std::string result = std::regex_replace(text, re, replacement);
	return notifier.createString(result);
}

AObject *split(NativeFuncInData) {
	GET_VALID_REGEX_OR_RETURN_NULL(args[0]->data->data, re);
	const std::string &text = args[1]->str->data;
	ClassId arrayClassId = args[2]->i;

	auto newArr = notifier.createArray(arrayClassId);
	std::sregex_token_iterator iter(text.begin(), text.end(), re, -1);
	std::sregex_token_iterator end;

	for (; iter != end; ++iter) {
		notifier.arrayAdd(newArr, notifier.createString(iter->str()));
	}
	return newArr;
}

void init(ACompiler &compiler) {
	auto nativeMap = ANativeMap();
	nativeMap.reserve(8);

	nativeMap.emplace("regex_constructor", &regex::constructor);
	nativeMap.emplace("regex_get_pattern", &regex::get_pattern);
	nativeMap.emplace("regex_is_match", &regex::is_match);
	nativeMap.emplace("regex_find", &regex::find);
	nativeMap.emplace("regex_find_all", &regex::find_all);
	nativeMap.emplace("regex_replace", &regex::replace);
	nativeMap.emplace("regex_split", &regex::split);

	compiler.registerBuiltInLibrary("std/regex", R"###(
@no_constructor
@no_extends
class Regex {
    
    @native("regex_constructor")
    private static fun _create(classId: Int, pattern: String): Regex
    
    static fun Regex(pattern: String): Regex = _create(getClassId(Regex), pattern)
    static fun compile(pattern: String): Regex = _create(getClassId(Regex), pattern)
    static fun from(pattern: String): Regex = _create(getClassId(Regex), pattern)

    @native("regex_get_pattern")
    fun pattern(): String

    @native("regex_get_pattern")
    fun getPattern(): String

    @native("regex_is_match")
    fun isMatch(text: String): Bool

    @native("regex_is_match")
    fun test(text: String): Bool

    @native("regex_is_match")
    fun matches(text: String): Bool

    @native("regex_is_match")
    fun match(text: String): Bool

    fun containsMatchIn(text: String): Bool = this.isMatch(text)

    @native("regex_find")
    fun find(text: String): String

    @native("regex_find_all")
    fun findAll(text: String, arrayClassId: Int = getClassId(Array<String>)): Array<String>

    @native("regex_find_all")
    fun matchAll(text: String, arrayClassId: Int = getClassId(Array<String>)): Array<String>

    @native("regex_find_all")
    fun search(text: String, arrayClassId: Int = getClassId(Array<String>)): Array<String>

    @native("regex_find_all")
    fun findAllMatches(text: String, arrayClassId: Int = getClassId(Array<String>)): Array<String>

    @native("regex_replace")
    fun replace(text: String, replacement: String): String

    @native("regex_replace")
    fun replaceAll(text: String, replacement: String): String

    @native("regex_replace")
    fun sub(text: String, replacement: String): String

    @native("regex_split")
    fun split(text: String, arrayClassId: Int = getClassId(Array<String>)): Array<String>

    static fun isMatch(pattern: String, text: String): Bool = Regex.compile(pattern).isMatch(text)
    static fun replace(pattern: String, text: String, replacement: String): String = Regex.compile(pattern).replace(text, replacement)
    static fun split(pattern: String, text: String): Array<String> = Regex.compile(pattern).split(text)
}
fun String.toRegex(): Regex = Regex.compile(this)
fun String.matches(regex: Regex): Bool = regex.matches(this)
fun String.replace(regex: Regex, replacement: String): String = regex.replace(this, replacement)
fun String.split(regex: Regex): Array<String> = regex.split(this)
fun String.contains(regex: Regex): Bool = regex.isMatch(this)
    )###",
	                                LibraryConfig(), std::move(nativeMap));
}

} // namespace regex
} // namespace Libs
} // namespace Autolang
#endif

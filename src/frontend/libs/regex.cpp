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
	int64_t options = (argSize >= 3 && args[2]->type == DefaultClass::intClassId) ? args[2]->i : 0;

	std::regex_constants::syntax_option_type flags = std::regex_constants::ECMAScript;
	if (options & 1) {
		flags |= std::regex_constants::icase;
	}

	try {
		auto handle = new ARegexHandle{std::regex(pattern, flags), pattern};
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

AObject *match_entire(NativeFuncInData) {
	GET_VALID_REGEX_OR_RETURN_NULL(args[0]->data->data, re);
	const std::string &text = args[1]->str->data;
	return notifier.createBool(std::regex_match(text, re));
}

AObject *find(NativeFuncInData) {
	GET_VALID_REGEX_OR_RETURN_NULL(args[0]->data->data, re);
	const std::string &text = args[1]->str->data;
	int64_t startIndex = (argSize >= 3 && args[2]->type == DefaultClass::intClassId) ? args[2]->i : 0;
	if (startIndex < 0) startIndex = 0;
	if (static_cast<size_t>(startIndex) > text.size()) return notifier.createString("");

	std::smatch m;
	auto startIt = text.cbegin() + startIndex;
	if (std::regex_search(startIt, text.cend(), m, re)) {
		return notifier.createString(m.str());
	}
	return notifier.createString("");
}

AObject *find_all(NativeFuncInData) {
	GET_VALID_REGEX_OR_RETURN_NULL(args[0]->data->data, re);
	const std::string &text = args[1]->str->data;
	int64_t startIndex = 0;
	ClassId arrayClassId = DefaultClass::anyClassId;
	if (argSize >= 3 && args[2]->type == DefaultClass::intClassId) {
		startIndex = args[2]->i;
	}
	if (argSize >= 4 && args[3]->type == DefaultClass::intClassId) {
		arrayClassId = args[3]->i;
	} else if (argSize >= 3 && args[2]->type != DefaultClass::intClassId) {
		arrayClassId = args[2]->i;
	}
	if (startIndex < 0) startIndex = 0;
	if (static_cast<size_t>(startIndex) > text.size()) startIndex = text.size();

	auto newArr = notifier.createArray(arrayClassId);
	auto startIt = text.cbegin() + startIndex;
	auto words_begin = std::sregex_iterator(startIt, text.cend(), re);
	auto words_end = std::sregex_iterator();

	for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
		std::smatch match = *i;
		notifier.arrayAdd(newArr, notifier.createString(match.str()));
	}

	return newArr;
}

AObject *replace_eval(NativeFuncInData) {
	GET_VALID_REGEX_OR_RETURN_NULL(args[0]->data->data, re);
	const std::string &text = args[1]->str->data;
	auto transform = args[2];

	std::string result;
	size_t lastPos = 0;
	auto words_begin = std::sregex_iterator(text.begin(), text.end(), re);
	auto words_end = std::sregex_iterator();

	for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
		std::smatch match = *i;
		result.append(text, lastPos, match.position() - lastPos);
		auto matchStr = notifier.createString(match.str());
		matchStr->retain();
		auto rep = notifier.callFunctionObject(transform, matchStr);
		notifier.release(matchStr);
		if (notifier.hasException()) return nullptr;
		if (rep && rep->type == DefaultClass::stringClassId) {
			result.append(rep->str->data);
		}
		if (rep) notifier.release(rep);
		lastPos = match.position() + match.length();
	}
	result.append(text, lastPos, text.size() - lastPos);
	return notifier.createString(result);
}

AObject *replace(NativeFuncInData) {
	if (args[2]->type == DefaultClass::functionClassId) {
		return replace_eval(notifier, args, argSize);
	}
	GET_VALID_REGEX_OR_RETURN_NULL(args[0]->data->data, re);
	const std::string &text = args[1]->str->data;
	const std::string &replacement = args[2]->str->data;

	std::string result = std::regex_replace(text, re, replacement);
	return notifier.createString(result);
}

AObject *split(NativeFuncInData) {
	GET_VALID_REGEX_OR_RETURN_NULL(args[0]->data->data, re);
	const std::string &text = args[1]->str->data;
	int64_t limit = 0;
	ClassId arrayClassId = DefaultClass::anyClassId;
	if (argSize >= 3 && args[2]->type == DefaultClass::intClassId) {
		limit = args[2]->i;
	}
	if (argSize >= 4 && args[3]->type == DefaultClass::intClassId) {
		arrayClassId = args[3]->i;
	} else if (argSize >= 3 && args[2]->type != DefaultClass::intClassId) {
		arrayClassId = args[2]->i;
	}

	auto newArr = notifier.createArray(arrayClassId);
	if (limit == 1) {
		notifier.arrayAdd(newArr, notifier.createString(text));
		return newArr;
	}

	size_t lastPos = 0;
	auto words_begin = std::sregex_iterator(text.begin(), text.end(), re);
	auto words_end = std::sregex_iterator();
	int64_t count = 0;

	for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
		std::smatch match = *i;
		if (limit > 0 && count + 1 >= limit) {
			break;
		}
		notifier.arrayAdd(newArr, notifier.createString(text.substr(lastPos, match.position() - lastPos)));
		lastPos = match.position() + match.length();
		count++;
	}
	notifier.arrayAdd(newArr, notifier.createString(text.substr(lastPos)));
	return newArr;
}

void init(ACompiler &compiler) {
	auto nativeMap = ANativeMap();
	nativeMap.reserve(10);

	nativeMap.emplace("regex_constructor", &regex::constructor);
	nativeMap.emplace("regex_get_pattern", &regex::get_pattern);
	nativeMap.emplace("regex_is_match", &regex::is_match);
	nativeMap.emplace("regex_match_entire", &regex::match_entire);
	nativeMap.emplace("regex_find", &regex::find);
	nativeMap.emplace("regex_find_all", &regex::find_all);
	nativeMap.emplace("regex_replace", &regex::replace);
	nativeMap.emplace("regex_replace_eval", &regex::replace_eval);
	nativeMap.emplace("regex_split", &regex::split);

	compiler.registerBuiltInLibrary("std/regex", R"###(
@no_constructor
@no_extends
class Regex {
    
    @native("regex_constructor")
    private static fun _create(classId: Int, pattern: String, options: Int = 0): Regex
    
    static fun Regex(pattern: String, options: Int = 0): Regex = _create(getClassId(Regex), pattern, options)
    static fun compile(pattern: String, options: Int = 0): Regex = _create(getClassId(Regex), pattern, options)
    static fun from(pattern: String, options: Int = 0): Regex = _create(getClassId(Regex), pattern, options)

    @native("regex_get_pattern")
    fun pattern(): String

    @native("regex_get_pattern")
    fun getPattern(): String

    @native("regex_is_match")
    fun isMatch(text: String): Bool

    @native("regex_match_entire")
    fun matchEntire(text: String): Bool

    @native("regex_is_match")
    fun test(text: String): Bool

    @native("regex_is_match")
    fun matches(text: String): Bool

    @native("regex_is_match")
    fun match(text: String): Bool

    fun containsMatchIn(text: String): Bool = this.isMatch(text)

    @native("regex_find")
    fun find(text: String, startIndex: Int = 0): String

    @native("regex_find_all")
    fun findAll(text: String, startIndex: Int = 0, arrayClassId: Int = getClassId(Array<String>)): Array<String>

    @native("regex_find_all")
    fun matchAll(text: String, startIndex: Int = 0, arrayClassId: Int = getClassId(Array<String>)): Array<String>

    @native("regex_find_all")
    fun search(text: String, startIndex: Int = 0, arrayClassId: Int = getClassId(Array<String>)): Array<String>

    @native("regex_find_all")
    fun findAllMatches(text: String, startIndex: Int = 0, arrayClassId: Int = getClassId(Array<String>)): Array<String>

    @native("regex_replace")
    fun replace(text: String, replacement: String): String

    @native("regex_replace_eval")
    fun replace(text: String, transform: (String) -> String): String

    @native("regex_replace")
    fun replaceAll(text: String, replacement: String): String

    @native("regex_replace")
    fun sub(text: String, replacement: String): String

    @native("regex_split")
    fun split(text: String, limit: Int = 0, arrayClassId: Int = getClassId(Array<String>)): Array<String>

    static fun isMatch(pattern: String, text: String): Bool = Regex.compile(pattern).isMatch(text)
    static fun replace(pattern: String, text: String, replacement: String): String = Regex.compile(pattern).replace(text, replacement)
    static fun split(pattern: String, text: String, limit: Int = 0): Array<String> = Regex.compile(pattern).split(text, limit)
}
fun String.toRegex(options: Int = 0): Regex = Regex.compile(this, options)
fun String.matches(regex: Regex): Bool = regex.matches(this)
fun String.replace(regex: Regex, replacement: String): String = regex.replace(this, replacement)
fun String.split(regex: Regex, limit: Int = 0): Array<String> = regex.split(this, limit)
fun String.contains(regex: Regex): Bool = regex.isMatch(this)
    )###",
	                                LibraryConfig(), std::move(nativeMap));
}

} // namespace regex
} // namespace Libs
} // namespace Autolang
#endif

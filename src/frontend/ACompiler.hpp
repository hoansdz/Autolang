#ifndef ACOMPILER_HPP
#define ACOMPILER_HPP

#include "frontend/lexer/Lexer.hpp"
#include "frontend/parser/Debugger.hpp"
#include "frontend/parser/FunctionEvent.hpp"
#include "frontend/parser/ParserContext.hpp"
#include "shared/ANativeFunctionData.hpp"
#include <iostream>

namespace AutoLang {

enum class CompilerState { CT_READY, CT_ERROR, CT_ANALYZED, CT_BYTECODE_READY };

enum class AllowRuleType {
	PLAIN_PREFIX = 0, // Plain string prefix match (special chars escaped automatically)
	REGEX        = 1  // Full ECMAScript regex pattern
};

struct AllowRule {
	AllowRuleType type;
	std::string   value;
	AllowRule(AllowRuleType type, std::string value)
	    : type(type), value(std::move(value)) {}
};

enum LibraryFlags : uint32_t {
	IS_BUILT_IN = 1u << 0,
	AUTO_IMPORT = 1u << 1,
	IS_MAIN_LIB = 1u << 2,
	IS_FILE = 1u << 3,
	IS_JS_BRIDGE = 1u << 4,
	ALLOW_LATEINIT_KEYWORD = 1u << 5,
	ALLOW_NON_NULL_ASSERTION = 1u << 6
};

static const ANativeMap EMPTY_NATIVE_MAP;

struct LibraryConfig {
	bool autoImport;
	bool allowLateinitKeyword;
	bool allowNonNullAssertion;
	LibraryConfig(bool autoImport = false, bool allowLateinitKeyword = true,
	              bool allowNonNullAssertion = true)
	    : autoImport(autoImport), allowLateinitKeyword(allowLateinitKeyword),
	      allowNonNullAssertion(allowNonNullAssertion) {}
};

struct LibraryData {
	std::string path;
	Lexer::Context lexerContext;
	HashMap<std::string, LibraryData *> dependencies;
	ANativeMap nativeFuncMap;
	std::string rawData;
	uint32_t flags;
	LibraryData(std::string path, uint32_t flags,
	            ANativeMap nativeFuncMap = EMPTY_NATIVE_MAP)
	    : path(std::move(path)), nativeFuncMap(std::move(nativeFuncMap)),
	      flags(flags) {}
	~LibraryData() {
		for (auto &[k, v] : nativeFuncMap) {
			switch (v.type) {
				case ANativeFunctionType::FUNC: {
					break;
				}
				case ANativeFunctionType::LAMBDA: {
					delete v.nativeLambda;
					break;
				}
				default: {
#ifdef __EMSCRIPTEN__
					if (flags & IS_JS_BRIDGE) {
						delete v.jsFunction;
					}
#endif
					break;
				}
			}
		}
	}
};

struct ACompilerConfig {
#ifndef NO_INCLUDE_LIBS_FILE
	bool addStdFile = true;
#endif
#ifndef NO_INCLUDE_LIBS_REGEX
	bool addStdRegex = true;
#endif
#ifndef NO_INCLUDE_LIBS_JSON
	bool addStdJson = true;
#endif
#ifndef NO_INCLUDE_LIBS_HTTP
	bool addHttpJson = true;
#endif
	bool addStdMath = true;
};

class ACompiler {
  public:
	LibraryData *mainSource;
	ParserContext parserContext;
	CompilerState state;
	bool loadedMainSource = false;
	bool loadedBuiltIn = false;

	const char* exceptionMessage = nullptr;

	std::vector<LibraryData *> generatedLibraries;
	std::vector<LibraryData *> builtInLibraries;
	HashMap<std::string, LibraryData *> autoImportMap;
	HashMap<std::string, Offset> generatedLibraryMap;
	HashMap<std::string, Offset> builtInLibrariesMap;
	// Add built in library
	void loadSource(LibraryData *library);
	void lexerTextToToken(LibraryData *library);
	void loadMainSource(LibraryData *library);
	void loadMainSource(
	    const char *path, LibraryConfig config = LibraryConfig(),
	    const ANativeMap &nativeFuncMap = AutoLang::EMPTY_NATIVE_MAP);
	void loadMainSource(
	    const char *path, const char *data,
	    LibraryConfig config = LibraryConfig(),
	    const ANativeMap &nativeFuncMap = AutoLang::EMPTY_NATIVE_MAP);
	LibraryData *requestImport(LibraryData *currentLibrary, const char *path);

	AVM vm = AVM(false);
	ACompiler(ACompilerConfig config = ACompilerConfig());
	~ACompiler();
	inline AutoLang::CompilerState getState() { return state; }
	void refresh();
	LibraryData *
	registerBuiltInLibrary(const char *path,
	                       LibraryConfig config = LibraryConfig(),
	                       const ANativeMap &nativeFuncMap = EMPTY_NATIVE_MAP);
	LibraryData *
	registerBuiltInLibrary(const char *path, const char *data,
	                       LibraryConfig config = LibraryConfig(),
	                       const ANativeMap &nativeFuncMap = EMPTY_NATIVE_MAP);
	void loadBuiltInFunctions();
	void generateBytecodes();
	void run();
	bool compile(const char *path, LibraryConfig config = LibraryConfig(),
	             const ANativeMap &nativeFuncMap = EMPTY_NATIVE_MAP);
	bool compile(const char *path, const char *data,
	             LibraryConfig config = LibraryConfig(),
	             const ANativeMap &nativeFuncMap = EMPTY_NATIVE_MAP);

#ifdef AUTOLANG_LIMIT_OPCODE
	void setLimitOpcodeCount(uint32_t limitOpcodeCount);
	uint32_t getLimitOpcodeCount();
#endif

#ifndef NO_INCLUDE_LIBS_HTTP
	void setAllowedDomainsRules(const std::vector<AllowRule> &rules);
#endif

#ifndef NO_INCLUDE_LIBS_FILE
	void setAllowFileRead(bool allow);
	void setAllowFileWrite(bool allow);
	void setAllowFileDelete(bool allow);
	void setAllowedFilePathsRules(const std::vector<AllowRule> &rules);
	void setFileBasePath(const std::string &path);
#endif

	inline void setOnError(FunctionEvent *onError) {
		if (parserContext.onError) {
			delete parserContext.onError;
		}
		parserContext.onError = onError;
	}
	inline void setOnWarning(FunctionEvent *onWarning) {
		if (parserContext.onWarning) {
			delete parserContext.onWarning;
		}
		parserContext.onWarning = onWarning;
	}
	inline bool hasError() {
		return state == AutoLang::CompilerState::CT_ERROR;
	}
};

} // namespace AutoLang

#endif
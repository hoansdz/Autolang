#ifndef ACOMPILER_HPP
#define ACOMPILER_HPP

#include "frontend/lexer/Lexer.hpp"
#include "frontend/parser/Debugger.hpp"
#include "frontend/parser/FunctionEvent.hpp"
#include "frontend/parser/ParserContext.hpp"
#include "shared/ANativeFunctionData.hpp"
#include <iostream>
#include <memory>

namespace Autolang {

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
#ifdef __PYBIND11__
	IS_PY_BRIDGE = 1u << 4,
#else
	IS_JS_BRIDGE = 1u << 4,
#endif
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
#if defined(__EMSCRIPTEN__) || defined(__PYBIND11__)
		for (auto &[k, v] : nativeFuncMap) {
#ifdef __EMSCRIPTEN__
			if ((flags & IS_JS_BRIDGE) && v.type == ANativeFunctionType::JS_FUNCTION && v.jsFunction) {
				delete v.jsFunction;
				v.jsFunction = nullptr;
			}
#elif __PYBIND11__
			if ((flags & IS_PY_BRIDGE) && v.type == ANativeFunctionType::PY_FUNCTION && v.pyFunction) {
				delete v.pyFunction;
				v.pyFunction = nullptr;
			}
#endif
		}
#endif
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
	bool addStdHttp = true;
#endif
	bool addStdMath = true;
	bool addStdBytes = true;
	bool addStdDate = true;
	bool enableKotlinCompat = true;
	bool strictMode = false;
	bool showWarnings = false;
	bool autoCloseBracketsOnEof = true;
	bool allowImplicitVarDeclaration = true;
};

class ACompiler {
  public:
	LibraryData *mainSource = nullptr;
	ParserContext parserContext;
	CompilerState state = CompilerState::CT_READY;
	bool loadedMainSource = false;
	bool loadedBuiltIn = false;
	bool shouldRefresh = false;

	const char* exceptionMessage = nullptr;

	std::vector<std::unique_ptr<LibraryData>> generatedLibraries;
	std::vector<std::unique_ptr<LibraryData>> builtInLibraries;
	HashMap<std::string, LibraryData *> autoImportMap;
	HashMap<std::string, Offset> generatedLibraryMap;
	HashMap<std::string, Offset> builtInLibrariesMap;
	ANativeMap globalNativeMap;
	// Add built in library
	void loadSource(LibraryData *library);
	void lexerTextToToken(LibraryData *library);
	void loadMainSource(LibraryData *library);
	void loadMainSource(
	    const char *path, LibraryConfig config = LibraryConfig(),
	    const ANativeMap &nativeFuncMap = Autolang::EMPTY_NATIVE_MAP);
	void loadMainSource(
	    const char *path, const char *data,
	    LibraryConfig config = LibraryConfig(),
	    const ANativeMap &nativeFuncMap = Autolang::EMPTY_NATIVE_MAP);
	LibraryData *requestImport(LibraryData *currentLibrary, const char *path);

	AVM vm = AVM(false);
	ACompiler(ACompilerConfig config = ACompilerConfig());
	~ACompiler();
	inline Autolang::CompilerState getState() { return state; }
	void refresh();
	inline void reset() { refresh(); }

	LibraryData *
	registerBuiltInLibrary(const char *path,
	                       LibraryConfig config = LibraryConfig(),
	                       const ANativeMap &nativeFuncMap = EMPTY_NATIVE_MAP);
	LibraryData *
	registerBuiltInLibrary(const char *path, const char *data,
	                       LibraryConfig config = LibraryConfig(),
	                       const ANativeMap &nativeFuncMap = EMPTY_NATIVE_MAP);

	inline LibraryData *
	registerBuiltInLibrary(const std::string &path,
	                       LibraryConfig config = LibraryConfig(),
	                       const ANativeMap &nativeFuncMap = EMPTY_NATIVE_MAP) {
		return registerBuiltInLibrary(path.c_str(), config, nativeFuncMap);
	}
	inline LibraryData *
	registerBuiltInLibrary(const std::string &path, const std::string &data,
	                       LibraryConfig config = LibraryConfig(),
	                       const ANativeMap &nativeFuncMap = EMPTY_NATIVE_MAP) {
		return registerBuiltInLibrary(path.c_str(), data.c_str(), config, nativeFuncMap);
	}

	inline void registerFunction(const std::string &name, ANativeFunction func) {
		globalNativeMap[name] = ANativeFunctionData(func);
	}
	inline void registerFunction(const std::string &name, ANativeLambdaFunction func) {
		globalNativeMap[name] = ANativeFunctionData(std::move(func));
	}
	inline void clearRegisteredFunctions() {
		globalNativeMap.clear();
	}
	inline bool registerLibraryFunction(const std::string &libraryPath,
	                                    const std::string &functionName,
	                                    ANativeLambdaFunction func) {
		auto it = builtInLibrariesMap.find(libraryPath);
		if (it != builtInLibrariesMap.end()) {
			builtInLibraries[it->second]->nativeFuncMap[functionName] =
			    ANativeFunctionData(std::move(func));
			return true;
		}
		return false;
	}

	void loadBuiltInFunctions();
	void generateBytecodes();
	void run();
	bool compileAndRun(const char *path, LibraryConfig config = LibraryConfig(),
	             const ANativeMap &nativeFuncMap = EMPTY_NATIVE_MAP);
	bool compileAndRun(const char *path, const char *data,
	             LibraryConfig config = LibraryConfig(),
	             const ANativeMap &nativeFuncMap = EMPTY_NATIVE_MAP);
	bool compile(const char *path, LibraryConfig config = LibraryConfig(),
	             const ANativeMap &nativeFuncMap = EMPTY_NATIVE_MAP);
	bool compile(const char *path, const char *data,
	             LibraryConfig config = LibraryConfig(),
	             const ANativeMap &nativeFuncMap = EMPTY_NATIVE_MAP);

	inline bool runSource(const std::string &source, const std::string &path = "main.atl",
	                      LibraryConfig config = LibraryConfig(),
	                      const ANativeMap &nativeFuncMap = EMPTY_NATIVE_MAP) {
		return compileAndRun(path.c_str(), source.c_str(), config, nativeFuncMap);
	}
	inline bool runFile(const std::string &path,
	                    LibraryConfig config = LibraryConfig(),
	                    const ANativeMap &nativeFuncMap = EMPTY_NATIVE_MAP) {
		return compileAndRun(path.c_str(), config, nativeFuncMap);
	}
	inline bool compileSource(const std::string &source, const std::string &path = "main.atl",
	                         LibraryConfig config = LibraryConfig(),
	                         const ANativeMap &nativeFuncMap = EMPTY_NATIVE_MAP) {
		return compile(path.c_str(), source.c_str(), config, nativeFuncMap);
	}
	inline bool compileFile(const std::string &path,
	                       LibraryConfig config = LibraryConfig(),
	                       const ANativeMap &nativeFuncMap = EMPTY_NATIVE_MAP) {
		return compile(path.c_str(), config, nativeFuncMap);
	}

#ifdef AUTOLANG_LIMIT_OPCODE
	void setLimitOpcodeCount(uint32_t limitOpcodeCount);
	uint32_t getLimitOpcodeCount();
#endif

	void setMaxManagedMemory(size_t limit);
	size_t getMaxManagedMemory();
	size_t getCurrentManagedMemory();

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

	inline void setOnError(std::function<void(std::string_view)> onErrorCallback) {
		if (parserContext.onError) {
			delete parserContext.onError;
		}
		parserContext.onError = new FunctionEvent(std::move(onErrorCallback));
	}
	inline void setOnError(FunctionEvent *onError) {
		if (parserContext.onError) {
			delete parserContext.onError;
		}
		parserContext.onError = onError;
	}
	inline void setOnWarning(std::function<void(std::string_view)> onWarningCallback) {
		if (parserContext.onWarning) {
			delete parserContext.onWarning;
		}
		parserContext.onWarning = new FunctionEvent(std::move(onWarningCallback));
	}
	inline void setOnWarning(FunctionEvent *onWarning) {
		if (parserContext.onWarning) {
			delete parserContext.onWarning;
		}
		parserContext.onWarning = onWarning;
	}
	inline const std::string &getLastError() const {
		return parserContext.lastErrorMessage;
	}
	inline void clearLastError() {
		parserContext.lastErrorMessage.clear();
	}
	inline const char *getExceptionMessage() const {
		return exceptionMessage;
	}
	inline std::string getException() const {
		return exceptionMessage ? exceptionMessage : "";
	}
	inline void setIgnoreForeignImports(bool ignore) {
		parserContext.ignoreForeignImports = ignore;
	}
	inline void setKotlinCompat(bool enable) {
		parserContext.kotlinCompatEnabled = enable;
		if (!enable) {
			parserContext.classAliasMap.erase("kotlin.math");
		} else {
			auto it = parserContext.defaultClassMap.find(
			    parserContext.createLexerStringIfNotExists("Math"));
			if (it != parserContext.defaultClassMap.end()) {
				parserContext.classAliasMap[parserContext.stringArena.allocateView("kotlin.math")] = it->second;
			}
		}
	}
	inline void setStrictMode(bool enable) {
		parserContext.strictMode = enable;
		if (enable) {
			parserContext.autoCloseBracketsOnEof = false;
			parserContext.allowImplicitVarDeclaration = false;
		} else {
			parserContext.autoCloseBracketsOnEof = true;
			parserContext.allowImplicitVarDeclaration = true;
		}
	}
	inline bool getStrictMode() const {
		return parserContext.strictMode;
	}
	inline void setShowWarnings(bool enable) {
		parserContext.showWarnings = enable;
	}
	inline bool getShowWarnings() const {
		return parserContext.showWarnings;
	}
	inline void setAutoCloseBracketsOnEof(bool enable) {
		parserContext.autoCloseBracketsOnEof = enable;
	}
	inline bool getAutoCloseBracketsOnEof() const {
		return parserContext.autoCloseBracketsOnEof;
	}
	inline void setAllowImplicitVarDeclaration(bool enable) {
		parserContext.allowImplicitVarDeclaration = enable;
	}
	inline bool getAllowImplicitVarDeclaration() const {
		return parserContext.allowImplicitVarDeclaration;
	}
	inline bool hasError() {
		return state == Autolang::CompilerState::CT_ERROR;
	}
	inline bool hasException() {
		return exceptionMessage != nullptr;
	}
};

} // namespace Autolang

#endif
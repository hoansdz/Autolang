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

enum LibraryFlags : uint32_t {
	IS_BUILT_IN = 1u << 0,
	AUTO_IMPORT = 1u << 1,
	IS_MAIN_LIB = 1u << 2,
	IS_FILE = 1u << 3,
	IS_JS_BRIDGE = 1u << 4
};

static const ANativeMap EMPTY_NATIVE_MAP;

struct LibraryData {
	std::string path;
	Lexer::Context lexerContext;
	HashMap<std::string, LibraryData *> dependencies;
	uint32_t flags;
	ANativeMap nativeFuncMap;
	std::string rawData;
	LibraryData(std::string path, uint32_t flags,
	            ANativeMap nativeFuncMap = EMPTY_NATIVE_MAP)
	    : path(std::move(path)), flags(flags),
	      nativeFuncMap(std::move(nativeFuncMap)) {}
#ifdef __EMSCRIPTEN__
	~LibraryData() {
		if (flags & IS_JS_BRIDGE) {
			for (auto &[k, v] : nativeFuncMap) {
				delete v.jsFunction;
			}
		}
	}
#endif
};

class ACompiler {
  public:
	LibraryData *mainSource;
	ParserContext parserContext;
	CompilerState state;
	bool loadedMainSource = false;
	bool loadedBuiltIn = false;

	std::vector<LibraryData *> generatedLibraries;
	std::vector<LibraryData *> builtInLibraries;
	HashMap<std::string, LibraryData *> autoImportMap;
	HashMap<std::string, Offset> generatedLibraryMap;
	HashMap<std::string, Offset> builtInLibrariesMap;
	// Add built in library
	void loadSource(LibraryData *library);
	void lexerTextToToken(LibraryData *library);
	void loadMainSource(LibraryData *library);
	void loadMainSource(const char *path, const ANativeMap &nativeFuncMap =
	                                          AutoLang::EMPTY_NATIVE_MAP);
	void loadMainSource(
	    const char *path, const char *data,
	    const ANativeMap &nativeFuncMap = AutoLang::EMPTY_NATIVE_MAP);
	LibraryData *requestImport(LibraryData *currentLibrary, const char *path);

	AVM vm = AVM(false);
	ACompiler();
	~ACompiler();
	inline AutoLang::CompilerState getState() { return state; }
	void refresh();
	LibraryData *
	registerBuiltInLibrary(const char *path, bool autoImport = false,
	                       const ANativeMap &nativeFuncMap = EMPTY_NATIVE_MAP);
	LibraryData *
	registerBuiltInLibrary(const char *path, const char *data,
	                       bool autoImport = false,
	                       const ANativeMap &nativeFuncMap = EMPTY_NATIVE_MAP);
	void loadBuiltInFunctions();
	void generateBytecodes();
	void run();
	bool compile(const char *path,
	             const ANativeMap &nativeFuncMap = EMPTY_NATIVE_MAP);
	bool compile(const char *path, const char *data,
	             const ANativeMap &nativeFuncMap = EMPTY_NATIVE_MAP);
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
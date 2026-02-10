#ifndef ACOMPILER_HPP
#define ACOMPILER_HPP

#include "frontend/lexer/Lexer.hpp"
#include "frontend/parser/Debugger.hpp"
#include <iostream>

namespace AutoLang {

enum class CompilerState { READY, ERROR, ANALYZED, BYTECODE_READY };

enum LibraryFlags : uint32_t {
	IS_BUILT_IN = 1u << 0,
	AUTO_IMPORT = 1u << 1,
};

static const ANativeMap EMPTY_NATIVE_MAP;

struct LibraryData {
	std::string path;
	Lexer::Context lexerContext;
	HashMap<std::string, LibraryData*> dependencies;
	uint32_t flags;
	ANativeMap nativeFuncMap;
	std::string rawData;
	LibraryData(std::string path, uint32_t flags, ANativeMap nativeFuncMap = EMPTY_NATIVE_MAP)
	    : path(std::move(path)), flags(flags),
	      nativeFuncMap(std::move(nativeFuncMap)) {}
};

class ACompiler {
  public:
  	LibraryData* mainSource;
	ParserContext parserContext;
	CompilerState state;
	bool loadedMainSource = false;
	bool loadedBuiltIn = false;

	std::vector<LibraryData *> generatedLibraries;
	std::vector<LibraryData *> builtInLibraries;
	HashMap<std::string, LibraryData*> autoImportMap;
	HashMap<std::string, Offset> builtInLibrariesMap;
	// Add built in library
	void loadSource(LibraryData *library);
	void lexerTextToToken(LibraryData *library);
	void loadMainSource(LibraryData *library);
	void loadMainSource(const char *path, const ANativeMap &nativeFuncMap);
	void loadMainSource(const char *path, const char *data,
	                    const ANativeMap &nativeFuncMap);
	LibraryData *requestImport(const char *path);

	AVM vm = AVM(false);
	ACompiler();
	~ACompiler();
	inline CompilerState getState() { return state; }
	void refresh();
	void registerFromSource(const char *path, bool autoImport = false,
	                        const ANativeMap &nativeFuncMap = EMPTY_NATIVE_MAP);
	void registerFromSource(const char *path, const char *data,
	                        bool autoImport = false,
	                        const ANativeMap &nativeFuncMap = EMPTY_NATIVE_MAP);
	void loadBuiltInFunctions();
	void generateBytecodes();
	void run();
};

} // namespace AutoLang

#endif
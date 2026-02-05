#ifndef ACOMPILER_HPP
#define ACOMPILER_HPP

#include "frontend/lexer/Lexer.hpp"
#include "frontend/parser/Debugger.hpp"
#include <iostream>

namespace AutoLang {

enum class CompilerState { READY, ERROR, ANALYZED, BYTECODE_READY };

class ACompiler {
  private:
	Lexer::Context lexerContext;
	ParserContext parserContext;
	CompilerState state;
	void registerFromSource(SourceChunk &source);

  public:
	AVM vm = AVM(false);
	ACompiler();
	~ACompiler();
	inline CompilerState getState() { return state; }
	void refresh();
	void registerFromSource(
	    const char *path, bool allowImportOtherFile,
	    const ANativeMap
	        &nativeFuncMap =
	            ANativeMap());
	void registerFromSource(
	    const char *path, bool allowImportOtherFile, const char *data,
	    const ANativeMap
	        &nativeFuncMap =
	            ANativeMap());
	void generateBytecodes();
	void run();
};

} // namespace AutoLang

#endif
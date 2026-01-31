#ifndef ACOMPILER_HPP
#define ACOMPILER_HPP

#include "frontend/parser/Debugger.hpp"
#include "frontend/lexer/Lexer.hpp"
#include <iostream>

namespace AutoLang {

class ACompiler {
  private:
	Lexer::Context lexerContext;
	ParserContext parserContext;
  public:
	ACompiler(AVM *vm);
	~ACompiler();
	AVM* vm;
	void registerSourceFromFile(AVMReadFileMode &mode);
};

} // namespace AutoLang

#endif
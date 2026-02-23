#ifndef DEBUGGER_IMPORT_CPP
#define DEBUGGER_IMPORT_CPP

#include "Debugger.hpp"
#include "frontend/ACompiler.hpp"

namespace AutoLang {

LibraryData *loadImport(in_func, LibraryData* currentLibrary, std::vector<Lexer::Token> &tokens,
                        ACompiler &compiler, size_t i) {
	Lexer::Token *token = &tokens[i];
	// std::cerr<<i<<" & "<<tokens.size() << "\n";
	Lexer::Token *importToken = token;
	uint32_t firstLine = token->line;
	if (token->type != Lexer::TokenType::IMPORT) {
		std::cerr<<token->toString(context)<<" "<<tokens[i+1].toString(context)<<" "<<tokens[i+2].toString(context)<<"\n";
		int* x = nullptr;
		*x = 5;
	}
	assert(token->type == Lexer::TokenType::IMPORT);
	if (!nextTokenSameLine(&token, tokens, i, firstLine) ||
	    !expect(token, Lexer::TokenType::LPAREN)) {
		--i;
		throw ParserError(firstLine, "@import expected string value (");
	}
	if (!nextTokenSameLine(&token, tokens, i, firstLine) ||
	    !expect(token, Lexer::TokenType::STRING)) {
		throw ParserError(firstLine, "@import expected string value (String");
	}
	std::string &path = context.lexerString[token->indexData];
	if (!nextTokenSameLine(&token, tokens, i, firstLine) ||
	    !expect(token, Lexer::TokenType::RPAREN)) {
		--i;
		throw ParserError(firstLine, "@import expects a constant "
		                             "string value, not an expression");
	}
	if (path.empty()) {
		throw ParserError(firstLine, "import path is empty");
	}
	// {
	// 	auto it = context.importMap.find(path);
	// 	if (it != context.importMap.end()) {
	// 		return it->second;
	// 	}
	// }
	LibraryData *library = compiler.requestImport(currentLibrary, path.c_str());
	if (!library) {
		throw ParserError(firstLine, "Cannot find library '" + path + "'");
	}
	// context.importMap[path] = library;
	if (!library->lexerContext.tokens.empty()) {
		return library;
	}
	compiler.loadSource(library);
	library->rawData.clear();
	return library;
}

} // namespace AutoLang

#endif
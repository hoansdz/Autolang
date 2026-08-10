#ifndef DEBUGGER_IMPORT_CPP
#define DEBUGGER_IMPORT_CPP

#include "Debugger.hpp"
#include "frontend/parser/ParserContext.hpp"
#include "frontend/ACompiler.hpp"

namespace Autolang {

LibraryData *loadImport(in_func, LibraryData* currentLibrary, std::vector<Lexer::Token> &tokens,
                        ACompiler &compiler, size_t i) {
	Lexer::Token *token = &tokens[i];
	// std::cerr<<i<<" & "<<tokens.size() << "\n";
	Lexer::Token *importToken = token;
	uint32_t firstLine = token->line;
	if (token->type != Lexer::TokenType::IMPORT) {
		std::cerr<<token->toString(context)<<" "<<tokens[i+1].toString(context)<<" "<<tokens[i+2].toString(context)<<"\n";
		int* x = nullptr;
		*x = 5; // gdb log stack trace
	}
	assert(token->type == Lexer::TokenType::IMPORT);
	if (!nextTokenSameLine(&token, tokens, i, firstLine) ||
	    !expect(token, Lexer::TokenType::LPAREN)) {
		--i;
		throw ParserError(
		    firstLine,
		    "@import expects an opening '(' bracket\nHint: Use "
		    "'@import(\"file_path\")' format with '(' after @import");
	}
	if (!nextTokenSameLine(&token, tokens, i, firstLine) ||
	    !expect(token, Lexer::TokenType::STRING)) {
		throw ParserError(
		    firstLine,
		    "@import expects a string value\nHint: Provide a string literal inside "
		    "parentheses, e.g. @import(\"path/to/file\")");
	}
	const std::string &path = context.lexerString[token->indexData];
	if (!nextTokenSameLine(&token, tokens, i, firstLine) ||
	    !expect(token, Lexer::TokenType::RPAREN)) {
		--i;
		throw ParserError(
		    firstLine,
		    "@import expects a constant string value, not an expression\nHint: "
		    "Close the import statement with ')' right after the string literal");
	}
	if (path.empty()) {
		throw ParserError(
		    firstLine,
		    "Import path is empty\nHint: Provide a non-empty file path string inside "
		    "@import(...)");
	}
	// {
	// 	auto it = context.importMap.find(path);
	// 	if (it != context.importMap.end()) {
	// 		return it->second;
	// 	}
	// }
	LibraryData *library = compiler.requestImport(currentLibrary, path.c_str());
	if (!library) {
		throw ParserError(
		    firstLine,
		    "Cannot find library '" + path +
		        "'\nHint: Verify that the imported file path exists and is "
		        "accessible");
	}
	// context.importMap[path] = library;
	if (!library->lexerContext.tokens.empty()) {
		return library;
	}
	compiler.loadSource(library);
	library->rawData.clear();
	return library;
}

} // namespace Autolang

#endif
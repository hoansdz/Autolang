#ifndef DEBUGGER_TYPEALIAS_CPP
#define DEBUGGER_TYPEALIAS_CPP

#include "frontend/ACompiler.hpp"
#include "frontend/parser/Debugger.hpp"
#include "frontend/parser/ParserContext.hpp"

namespace Autolang {

void loadTypealias(in_func, size_t &i) {
	Lexer::Token *token = &context.tokens[i];
	uint32_t firstLine = token->line;
	if (context.currentClassId ||
	    context.currentFunctionId != context.mainFunctionId ||
	    context.currentClosureNode) {
		throw ParserError(firstLine,
		                  "Typealias is only be declared at top level");
	}
	ensureNoKeyword(in_data, i);
	ensureNoAnnotations(in_data, i);
	// Condition
	if (!nextToken(&token, context.tokens, i) ||
	    !expect(token, Lexer::TokenType::IDENTIFIER)) {
		--i;
		throw ParserError(firstLine,
		                  "Expected class name but not found\nHint: Use "
		                  "identfier after typealias");
	}
	LexerStringId className = token->indexData;
	if (!nextToken(&token, context.tokens, i) ||
	    !expect(token, Lexer::TokenType::EQUAL)) {
		--i;
		throw ParserError(firstLine,
		                  "Expected '=' after identifier but not found\nHint: "
		                  "Add '=' after the identifier");
	}
	auto classDeclaration =
	    loadClassDeclaration(in_data, i, token->line, false);
	auto it = context.typealiasMap.find(className);
	if (it != context.typealiasMap.end()) {
		if (it->second) {
			throw ParserError(
			    firstLine,
			    "Typealias has been declared at " +
			        it->second->classDeclaration->mode->path + ":" +
			        std::to_string(it->second->classDeclaration->line));
		}
		throw ParserError(firstLine, "Typealias cannot have a duplicate name");
	}
	context.typealiasMap[className] = context.typealiasPool.push(
	    classDeclaration, TypealiasState::TAS_UNVISITED);
	context.allClassDeclarations.push_back(classDeclaration);
}

} // namespace Autolang

#endif
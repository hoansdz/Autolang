#ifndef DEBUGGER_TRY_CATCH_CPP
#define DEBUGGER_TRY_CATCH_CPP

#include "frontend/parser/Debugger.hpp"

namespace AutoLang {

TryCatchNode *loadTryCatch(in_func, size_t &i) {
	Lexer::Token *token = &context.tokens[i];
	uint32_t firstLine = token->line;
	TryCatchNode *node =
	    context.tryCatchPool.push(firstLine); // WhilePool managed
	// Condition
	if (!nextToken(&token, context.tokens, i) ||
	    !expect(token, Lexer::TokenType::LBRACE)) {
		--i;
		throw ParserError(firstLine,
		                  "Expected body open with { after try but not found");
	}
	loadBody(in_data, node->body.nodes, i, true);
	if (!nextToken(&token, context.tokens, i) ||
	    !expect(token, Lexer::TokenType::CATCH)) {
		--i;
		throw ParserError(firstLine, "Expected catch after try but not found");
	}
	if (!nextToken(&token, context.tokens, i) ||
	    !expect(token, Lexer::TokenType::LPAREN)) {
		--i;
		throw ParserError(firstLine,
		                  "Expected left paren after catch but not found");
	}
	if (!nextToken(&token, context.tokens, i) ||
	    !expect(token, Lexer::TokenType::IDENTIFIER)) {
		--i;
		throw ParserError(firstLine,
		                  "Expected variable name after catch but not found");
	}
	std::string &name = context.lexerString[token->indexData];
	if (!nextToken(&token, context.tokens, i) ||
	    !expect(token, Lexer::TokenType::RPAREN)) {
		--i;
		throw ParserError(firstLine,
		                  "Expected variable name after try but not found");
	}
	if (!nextToken(&token, context.tokens, i) ||
	    !expect(token, Lexer::TokenType::LBRACE)) {
		--i;
		throw ParserError(
		    firstLine, "Expected body open with { after catch but not found");
	}
	auto funcInfo = context.getCurrentFunctionInfo(in_data);
	funcInfo->scopes.emplace_back();
	auto declarationNode = context.makeDeclarationNode(
	    in_data, token->line, true, name, nullptr, true,
	    context.currentFunctionId == context.mainFunctionId, false);
	declarationNode->classId = AutoLang::DefaultClass::exceptionClassId;
	funcInfo->scopes.back()[name] = declarationNode;
	node->exceptionDeclaration = declarationNode;
	loadBody(in_data, node->catchBody.nodes, i, true);
	return node;
}

ThrowNode *loadThrow(in_func, size_t &i) {
	Lexer::Token *token = &context.tokens[i];
	uint32_t firstLine = token->line;
	if (!nextTokenSameLine(&token, context.tokens, i, token->line)) {
		throw ParserError(firstLine,
		                  "Expected value after throw but not found");
	}
	return context.throwPool.push(firstLine, loadExpression(in_data, 0, i));
}

} // namespace AutoLang

#endif
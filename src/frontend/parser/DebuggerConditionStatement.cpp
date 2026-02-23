#ifndef DEBUGGER_CONDITION_STATEMENT_CPP
#define DEBUGGER_CONDITION_STATEMENT_CPP

#include "frontend/parser/Debugger.hpp"

namespace AutoLang {

IfNode *loadIf(in_func, size_t &i, bool mustReturnValue) {
	Lexer::Token *token = &context.tokens[i];
	uint32_t firstLine = token->line;
	IfNode *node =
	    context.ifPool.push(firstLine, mustReturnValue); // IfPool managed
	if (!nextTokenSameLine(&token, context.tokens, i, firstLine) ||
	    !expect(token, Lexer::TokenType::LPAREN)) {
		--i;
		throw ParserError(firstLine, "Expected ( after if but not found");
	}
	if (!nextTokenSameLine(&token, context.tokens, i, firstLine)) {
		--i;
		throw ParserError(firstLine,
		                  "Expected expression after if but not found");
	}
	node->condition = loadExpression(in_data, 0, i);
	if (!nextToken(&token, context.tokens, i) ||
	    !expect(token, Lexer::TokenType::RPAREN)) {
		--i;
		throw ParserError(context.tokens[i].line, "Expected ) but not found");
	}
	if (!nextToken(&token, context.tokens, i)) {
		--i;
		throw ParserError(context.tokens[i].line,
		                  "Expected command after if but not found");
	}
	loadBody(in_data, node->ifTrue.nodes, i);
	if (!nextToken(&token, context.tokens, i) ||
	    !expect(token, Lexer::TokenType::ELSE)) {
		--i;
		if (mustReturnValue) {
			throw ParserError(context.tokens[i].line,
			                  "If expression must have an else branch");
		}
		return node;
	}
	if (!nextToken(&token, context.tokens, i)) {
		--i;
		throw ParserError(token->line,
		                  "Expected command after else but not found");
	}
	node->ifFalse = context.blockNodePool.push(token->line);
	loadBody(in_data, node->ifFalse->nodes, i);
	return node;
}

} // namespace AutoLang

#endif
#ifndef DEBUGGER_CONDITION_STATEMENT_CPP
#define DEBUGGER_CONDITION_STATEMENT_CPP

#include "frontend/parser/Debugger.hpp"
#include "frontend/parser/ParserContext.hpp"

namespace Autolang {

IfNode *loadIf(in_func, size_t &i, bool mustReturnValue) {
	Lexer::Token *token = &context.tokens[i];
	uint32_t firstLine = token->line;
	IfNode *node =
	    context.ifPool.push(firstLine, mustReturnValue); // IfPool managed
	if (!nextTokenSameLine(&token, context.tokens, i, firstLine)) {
		--i;
		throw ParserError(firstLine,
		                  "Expected condition after 'if' but not found\nHint: Provide a boolean condition after 'if'");
	}
	bool hasOuterParen = false;
	if (expect(token, Lexer::TokenType::LPAREN)) {
		int depth = 0;
		for (size_t s = i; s < context.tokens.size(); ++s) {
			if (context.tokens[s].type == Lexer::TokenType::LPAREN) {
				depth++;
			} else if (context.tokens[s].type == Lexer::TokenType::RPAREN) {
				depth--;
				if (depth == 0) {
					size_t nextIdx = s + 1;
					if (nextIdx < context.tokens.size()) {
						auto nextType = context.tokens[nextIdx].type;
						if (nextType == Lexer::TokenType::LBRACE ||
						    nextType == Lexer::TokenType::RETURN ||
						    nextType == Lexer::TokenType::THROW ||
						    nextType == Lexer::TokenType::VAR ||
						    nextType == Lexer::TokenType::VAL ||
						    getPrecedence(nextType) == -1) {
							hasOuterParen = true;
						}
					} else {
						hasOuterParen = true;
					}
					break;
				}
			}
		}
		if (hasOuterParen) {
			if (!nextTokenSameLine(&token, context.tokens, i, firstLine)) {
				--i;
				throw ParserError(firstLine,
				                  "Expected an expression after 'if (' but not found\nHint: Provide a boolean condition inside 'if (...)'");
			}
		}
	}
	bool prevAllowTrailingClosure = context.allowTrailingClosure;
	if (!hasOuterParen) context.allowTrailingClosure = false;
	node->condition = loadExpression(in_data, 0, i);
	context.allowTrailingClosure = prevAllowTrailingClosure;
	if (hasOuterParen) {
		if (!nextToken(&token, context.tokens, i) ||
		    !expect(token, Lexer::TokenType::RPAREN)) {
			--i;
			throw ParserError(context.tokens[i].line, "Expected ')' but not found\nHint: Close 'if' condition with ')'");
		}
	}
	if (!nextToken(&token, context.tokens, i)) {
		--i;
		throw ParserError(context.tokens[i].line,
		                  "Expected a command after 'if' but not found\nHint: Provide a statement block '{ ... }' after 'if (...)'");
	}
	loadBody<false>(in_data, node->ifTrue.nodes, i);
	if (!nextToken(&token, context.tokens, i) ||
	    !expect(token, Lexer::TokenType::ELSE)) {
		--i;
		if (mustReturnValue) {
			throw ParserError(context.tokens[i].line,
			                  "'if' expression must return a value, so it must "
			                  "have an 'else' branch\nHint: Add an 'else' branch to single-expression 'if'");
		}
		return node;
	}
	if (!nextToken(&token, context.tokens, i)) {
		--i;
		throw ParserError(token->line,
		                  "Expected a command after 'else' but not found\nHint: Provide a statement block '{ ... }' or 'if' statement after 'else'");
	}
	node->ifFalse = context.blockNodePool.push(token->line);
	loadBody<false>(in_data, node->ifFalse->nodes, i);
	return node;
}

} // namespace Autolang

#endif
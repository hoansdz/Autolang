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
						} else if (nextType == Lexer::TokenType::MINUS ||
						           nextType == Lexer::TokenType::PLUS ||
						           nextType == Lexer::TokenType::EXMARK) {
							int scanDepth = 0;
							for (size_t t = nextIdx; t < context.tokens.size(); ++t) {
								if (context.tokens[t].type == Lexer::TokenType::LPAREN ||
								    context.tokens[t].type == Lexer::TokenType::LBRACKET ||
								    context.tokens[t].type == Lexer::TokenType::LBRACE) {
									scanDepth++;
								} else if (context.tokens[t].type == Lexer::TokenType::RPAREN ||
								           context.tokens[t].type == Lexer::TokenType::RBRACKET ||
								           context.tokens[t].type == Lexer::TokenType::RBRACE) {
									if (scanDepth == 0) break;
									scanDepth--;
								} else if (scanDepth == 0 && context.tokens[t].type == Lexer::TokenType::ELSE) {
									hasOuterParen = true;
									break;
								}
							}
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

	extractSmartCasts(in_data, node->condition, node->trueCasts, node->falseCasts);

	for (auto &cast : node->trueCasts) cast.apply();
	loadBody<false>(in_data, node->ifTrue.nodes, i);
	for (auto &cast : node->trueCasts) cast.restore();

	if (!node->ifTrue.nodes.empty()) {
		auto lastKind = node->ifTrue.nodes.back()->kind;
		if (lastKind == NodeType::RET || lastKind == NodeType::THROW || lastKind == NodeType::SKIP) {
			node->trueBranchReturns = true;
		}
	}

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

	for (auto &cast : node->falseCasts) cast.apply();
	loadBody<false>(in_data, node->ifFalse->nodes, i);
	for (auto &cast : node->falseCasts) cast.restore();

	if (!node->ifFalse->nodes.empty()) {
		auto falseLastKind = node->ifFalse->nodes.back()->kind;
		if (falseLastKind == NodeType::RET || falseLastKind == NodeType::THROW || falseLastKind == NodeType::SKIP) {
			node->falseBranchReturns = true;
		}
	}

	return node;
}

} // namespace Autolang

#endif
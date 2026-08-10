#ifndef DEBUGGER_WHEN_CPP
#define DEBUGGER_WHEN_CPP

#include "frontend/parser/Debugger.hpp"
#include "frontend/parser/ParserContext.hpp"

namespace Autolang {

HasClassIdNode *loadWhenExpression(in_func, size_t &i, HasClassIdNode *value) {
	Lexer::Token *token = &context.tokens[i];
	uint32_t firstLine = token->line;
	uint32_t tokenIndex = i;
	switch (token->type) {
		case Lexer::TokenType::IN_:
		case Lexer::TokenType::IS: {
			if (!value) {
				throw ParserError(
				    firstLine,
				    "Cannot use 'is', 'in' because 'when' has no value\nHint: Provide "
				    "a target expression in when header: when (x) { ... }");
			}
			auto op = token->type;
			if (!nextTokenSameLine(&token, context.tokens, i, firstLine)) {
				--i;
				throw ParserError(
				    context.tokens[i].line,
				    "Bug: Lexer did not ensure a closing bracket\nHint: Ensure the "
				    "when block is properly closed with '}'");
			}
			auto right = loadExpression(in_data, 0, i);
			return context.binaryNodePool.push(token->line, tokenIndex,
			                                   context.currentClassId, op,
			                                   value, right);
		}
		default: {
			auto condition = loadExpression(in_data, 0, i);
			if (!value || condition->kind == NodeType::BINARY) {
				return condition;
			}
			return context.binaryNodePool.push(
			    token->line, tokenIndex, context.currentClassId,
			    Lexer::TokenType::EQEQ, value, condition);
		}
	}
}

HasClassIdNode *loadWhenCondition(in_func, size_t &i, HasClassIdNode *value) {
	Lexer::Token *token = &context.tokens[i];
	uint32_t tokenIndex = i;
	auto left = loadWhenExpression(in_data, i, value);
	while (nextToken(&token, context.tokens, i)) {
		switch (token->type) {
			case Lexer::TokenType::COMMA: {
				if (!nextToken(&token, context.tokens, i)) {
					--i;
					throw ParserError(
					    context.tokens[i].line,
					    "Bug: Lexer did not ensure a closing bracket\nHint: "
					    "Provide a condition after ',' in when branch");
				}
				auto right = loadWhenExpression(in_data, i, value);
				left = context.binaryNodePool.push(
				    token->line, tokenIndex, context.currentClassId,
				    Lexer::TokenType::OR_OR, left, right);
				break;
			}
			case Lexer::TokenType::MINUS_GT: {
				// --i;
				return left;
			}
			default: {
				throw ParserError(
				    token->line,
				    "Expected 'when' condition but '" + token->toString(context) +
				        "' found\nHint: Specify a valid branch condition followed by "
				        "'->'");
			}
		}
	}
	--i;
	throw ParserError(
	    context.tokens[i].line,
	    "Bug: Lexer did not ensure a closing bracket\nHint: Ensure the when "
	    "expression block is closed with '}'");
}

HasClassIdNode *loadWhen(in_func, size_t &i, bool mustReturnValue) {
	Lexer::Token *token = &context.tokens[i];
	uint32_t firstLine = token->line;
	if (!nextToken(&token, context.tokens, i)) {
		--i;
		throw ParserError(
		    firstLine,
		    "Expected '(' after when but not found\nHint: Open the when "
		    "expression or block with '(' or '{'");
	}
	HasClassIdNode *value = nullptr;
	if (expect(token, Lexer::TokenType::LPAREN)) {
		if (!nextToken(&token, context.tokens, i)) {
			--i;
			throw ParserError(
			    firstLine,
			    "Expected expression after 'when' but not found\nHint: Provide "
			    "a value expression inside when (...)");
		}
		value = loadExpression(in_data, 0, i);
		if (!nextToken(&token, context.tokens, i) ||
		    !expect(token, Lexer::TokenType::RPAREN)) {
			--i;
			throw ParserError(
			    firstLine,
			    "Expected ')' after 'when' but not found\nHint: Close the when "
			    "expression header with ')'");
		}
		if (!nextToken(&token, context.tokens, i)) {
			--i;
			throw ParserError(
			    firstLine,
			    "Expected body to open with '{' after 'when' but not found\nHint: "
			    "Open the when body with '{' after when (...)");
		}
	}
	if (!expect(token, Lexer::TokenType::LBRACE)) {
		--i;
		throw ParserError(
		    firstLine,
		    "Expected body to open with '{' after 'when' but not found\nHint: "
		    "Open the when body with '{'");
	}
	IfNode *mainIfNode = nullptr;
	IfNode *currentIfNode = nullptr;
	bool loadedElse = false;
	while (true) {
		if (!nextToken(&token, context.tokens, i)) {
			--i;
			throw ParserError(
			    firstLine,
			    "Bug: Lexer did not ensure a closing bracket\nHint: Ensure the "
			    "when body is properly closed with '}'");
		}
		switch (token->type) {
			case Lexer::TokenType::RBRACE: {
				if (!loadedElse && mustReturnValue) {
					throw ParserError(
					    firstLine,
					    "When expression requires an else branch to produce a "
					    "value\nHint: Add an 'else -> ...' branch to handle "
					    "unmatched cases when evaluated as an expression");
				}
				return context.whenNodePool.push(firstLine, value, mainIfNode);
			}
			case Lexer::TokenType::ELSE: {
				if (loadedElse) {
					throw ParserError(
					    token->line,
					    "Duplicate 'else' branch\nHint: 'when' statement can "
					    "only have one 'else ->' branch");
				}
				if (!nextToken(&token, context.tokens, i)) {
					--i;
					throw ParserError(
					    context.tokens[i].line,
					    "Expected '->' after condition but not found\nHint: "
					    "Separate branch condition and action with '->'");
				}
				if (!nextToken(&token, context.tokens, i)) {
					--i;
					throw ParserError(
					    context.tokens[i].line,
					    "Expected body but not found\nHint: Provide an action "
					    "expression or block after '->'");
				}
				loadedElse = true;
				if (currentIfNode == nullptr) {
					IfNode *ifNode = context.ifPool.push(token->line, false);
					currentIfNode = ifNode;
					mainIfNode = ifNode;
					ifNode->condition =
					    context.constValuePool.push(token->line, true);
					loadBody<false>(in_data, ifNode->ifTrue.nodes, i, true);
					break;
				}
				currentIfNode->ifFalse =
				    context.blockNodePool.push(token->line);
				loadBody<false>(in_data, currentIfNode->ifFalse->nodes, i,
				                true);
				break;
			}
			default: {
				if (loadedElse) {
					throw ParserError(
					    token->line,
					    "'else' must be the last branch in 'when'\nHint: Move "
					    "'else ->' branch to the very end of the when block");
				}
				auto condition = loadWhenCondition(in_data, i, value);
				if (!nextToken(&token, context.tokens, i)) {
					--i;
					throw ParserError(
					    context.tokens[i].line,
					    "Expected body but not found\nHint: Provide an action "
					    "expression or block after '->' in when branch");
				}
				token = &context.tokens[i];
				IfNode *ifNode =
				    context.ifPool.push(token->line, mustReturnValue);
				ifNode->condition = condition;
				if (currentIfNode == nullptr) {
					mainIfNode = ifNode;
					currentIfNode = ifNode;
				} else {
					currentIfNode->ifFalse =
					    context.blockNodePool.push(token->line);
					currentIfNode->ifFalse->nodes.push_back(ifNode);
					currentIfNode = ifNode;
				}
				loadBody<false>(in_data, ifNode->ifTrue.nodes, i, true);
				break;
			}
		}
	}
}

} // namespace Autolang

#endif
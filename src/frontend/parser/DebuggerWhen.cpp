#ifndef DEBUGGER_WHEN_CPP
#define DEBUGGER_WHEN_CPP

#include "frontend/parser/Debugger.hpp"
#include "frontend/parser/ParserContext.hpp"

namespace AutoLang {

HasClassIdNode *loadWhenExpression(in_func, size_t &i, HasClassIdNode *value) {
	Lexer::Token *token = &context.tokens[i];
	uint32_t firstLine = token->line;
	switch (token->type) {
		case Lexer::TokenType::IN:
		case Lexer::TokenType::IS: {
			if (!value) {
				throw ParserError(
				    firstLine,
				    "Cannot use 'is', 'in' because 'when' has no value");
			}
			auto op = token->type;
			if (!nextTokenSameLine(&token, context.tokens, i, firstLine)) {
				--i;
				throw ParserError(context.tokens[i].line,
				                  "Bug: Lexer not ensure close bracket");
			}
			auto right = loadExpression(in_data, 0, i);
			return context.binaryNodePool.push(
			    token->line, context.currentClassId, op, value, right);
		}
		default: {
			auto condition = loadExpression(in_data, 0, i);
			if (condition->kind == NodeType::BINARY) {
				return condition;
			}
			return context.binaryNodePool.push(
			    token->line, context.currentClassId, Lexer::TokenType::EQEQ,
			    value, condition);
		}
	}
}

HasClassIdNode *loadWhenCondition(in_func, size_t &i, HasClassIdNode *value) {
	Lexer::Token *token = &context.tokens[i];
	auto left = loadWhenExpression(in_data, i, value);
	while (nextToken(&token, context.tokens, i)) {
		switch (token->type) {
			case Lexer::TokenType::COMMA: {
				if (!nextToken(&token, context.tokens, i)) {
					--i;
					throw ParserError(context.tokens[i].line,
					                  "Bug: Lexer not ensure close bracket");
				}
				auto right = loadWhenExpression(in_data, i, value);
				left = context.binaryNodePool.push(
				    token->line, context.currentClassId,
				    Lexer::TokenType::OR_OR, left, right);
				break;
			}
			case Lexer::TokenType::MINUS_GT: {
				// --i;
				return left;
			}
			default: {
				throw ParserError(token->line, "Expected when condition but '" +
				                                   token->toString(context) +
				                                   "' found");
			}
		}
	}
	--i;
	throw ParserError(context.tokens[i].line,
	                  "Bug: Lexer not ensure close bracket");
}

WhenNode *loadWhen(in_func, size_t &i, bool mustReturnValue) {
	Lexer::Token *token = &context.tokens[i];
	uint32_t firstLine = token->line;
	if (!nextToken(&token, context.tokens, i)) {
		--i;
		throw ParserError(firstLine, "Expected ( after when but not found");
	}
	HasClassIdNode *value = nullptr;
	if (expect(token, Lexer::TokenType::LPAREN)) {
		if (!nextToken(&token, context.tokens, i)) {
			--i;
			throw ParserError(firstLine,
			                  "Expected expression after when but not found");
		}
		value = loadExpression(in_data, 0, i);
		if (!nextToken(&token, context.tokens, i) ||
		    !expect(token, Lexer::TokenType::RPAREN)) {
			--i;
			throw ParserError(firstLine, "Expected ) after when but not found");
		}
		if (!nextToken(&token, context.tokens, i)) {
			--i;
			throw ParserError(
			    firstLine,
			    "Expected body open with { after when but not found");
		}
	}
	if (!expect(token, Lexer::TokenType::LBRACE)) {
		--i;
		throw ParserError(firstLine,
		                  "Expected body open with { after when but not found");
	}
	IfNode *mainIfNode = nullptr;
	IfNode *currentIfNode = nullptr;
	bool loadedElse = false;
	while (true) {
		if (!nextToken(&token, context.tokens, i)) {
			--i;
			throw ParserError(firstLine, "Bug: Lexer not ensure close bracket");
		}
		switch (token->type) {
			case Lexer::TokenType::RBRACE: {
				return context.whenNodePool.push(firstLine, value, mainIfNode);
			}
			case Lexer::TokenType::ELSE: {
				if (loadedElse) {
					throw ParserError(token->line, "Duplicated else condition");
				}
				if (!nextToken(&token, context.tokens, i)) {
					--i;
					throw ParserError(
					    context.tokens[i].line,
					    "Expected -> after condition but not found");
				}
				if (!nextToken(&token, context.tokens, i)) {
					--i;
					throw ParserError(context.tokens[i].line,
					                  "Expected body but not found");
				}
				loadedElse = true;
				if (currentIfNode == nullptr) {
					IfNode *ifNode =
					    context.ifPool.push(token->line, mustReturnValue);
					currentIfNode = ifNode;
					mainIfNode = ifNode;
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
					throw ParserError(token->line,
					                  "Else must be the last branch in when");
				}
				auto condition = loadWhenCondition(in_data, i, value);
				if (!nextToken(&token, context.tokens, i)) {
					--i;
					throw ParserError(context.tokens[i].line,
					                  "Expected body but not found");
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

} // namespace AutoLang

#endif
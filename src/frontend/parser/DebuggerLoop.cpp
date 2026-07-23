#ifndef DEBUGGER_LOOP_CPP
#define DEBUGGER_LOOP_CPP

#include "frontend/parser/Debugger.hpp"
#include "frontend/parser/ParserContext.hpp"

namespace Autolang {

WhileNode *loadWhile(in_func, size_t &i) {
	Lexer::Token *token = &context.tokens[i];
	uint32_t firstLine = token->line;
	WhileNode *node = context.whilePool.push(firstLine); // WhilePool managed
	// Condition
	if (!nextTokenSameLine(&token, context.tokens, i, firstLine) ||
	    !expect(token, Lexer::TokenType::LPAREN)) {
		--i;
		throw ParserError(firstLine,
		                  "Expected '(' after 'while' in while statement");
	}
	if (!nextTokenSameLine(&token, context.tokens, i, firstLine)) {
		--i;
		throw ParserError(firstLine,
		                  "Expected condition expression in while statement");
	}
	node->condition = loadExpression(in_data, 0, i);
	if (!nextToken(&token, context.tokens, i) ||
	    !expect(token, Lexer::TokenType::RPAREN)) {
		--i;
		throw ParserError(context.tokens[i].line,
		                  "Expected closing ')' after while condition");
	}
	if (!nextToken(&token, context.tokens, i)) {
		--i;
		throw ParserError(context.tokens[i].line,
		                  "Expected body after while statement");
	}
	loadBody<false>(in_data, node->body.nodes, i);
	return node;
}

ExprNode *loadFor(in_func, size_t &i) {
	Lexer::Token *token = &context.tokens[i];
	uint32_t firstLine = token->line;

	if (!nextTokenSameLine(&token, context.tokens, i, firstLine) ||
	    !expect(token, Lexer::TokenType::LPAREN)) {
		--i;
		throw ParserError(firstLine,
		                  "Expected '(' after 'for' in for statement");
	}
	if (!nextTokenSameLine(&token, context.tokens, i, firstLine)) {
		--i;
		throw ParserError(firstLine,
		                  "Expected loop variable name in for statement");
	}
	if (!expect(token, Lexer::TokenType::IDENTIFIER)) {
		--i;
		throw ParserError(
		    context.tokens[i].line,
		    "Expected identifier as loop variable in for statement");
	}
	auto baseName = token->indexData;
	const std::string &name = context.lexerString[token->indexData];
	VarNode *declaration;
	context.getCurrentFunctionInfo(in_data)->scopes.emplace_back();
	// Create temp declaration
	auto declarationNode = context.makeDeclarationNode(
	    in_data, token->line, baseName, name, nullptr, true,
	    context.currentFunctionId == context.mainFunctionId, false, true, true);
	declarationNode->classId = Autolang::DefaultClass::nullClassId;
	declaration =
	    context.varPool.push(firstLine, declarationNode, false, false);
	if (!nextToken(&token, context.tokens, i) ||
	    !expect(token, Lexer::TokenType::IN_)) {
		--i;
		throw ParserError(context.tokens[i].line,
		                  "Expected 'in' after loop variable in for statement");
	}
	if (!nextToken(&token, context.tokens, i)) {
		--i;
		throw ParserError(
		    context.tokens[i].line,
		    "Expected iterable expression after 'in' in for statement");
	}
	HasClassIdNode *data = loadExpression(in_data, 0, i);
	VarNode *iteratorNode = nullptr;
	if (data->kind != NodeType::RANGE) {
		// Create temp declaration
		auto declarationNode = context.makeDeclarationNode(
		    in_data, token->line,
		    context.createLexerStringIfNotExists(".iterator"), ".iterator",
		    nullptr, true, context.currentFunctionId == context.mainFunctionId,
		    false, true, true);
		declarationNode->classId = Autolang::DefaultClass::intClassId;
		iteratorNode =
		    context.varPool.push(firstLine, declarationNode, false, false);
	}
	if (!nextToken(&token, context.tokens, i) ||
	    !expect(token, Lexer::TokenType::RPAREN)) {
		--i;
		throw ParserError(context.tokens[i].line,
		                  "Expected closing ')' after for condition");
	}
	if (!nextToken(&token, context.tokens, i)) {
		--i;
		throw ParserError(context.tokens[i].line,
		                  "Expected body after for statement");
	}
	auto node =
	    context.forPool.push(firstLine, declaration, data, iteratorNode);
	loadBody<false>(in_data, node->body.nodes, i, false);
	context.getCurrentFunctionInfo(in_data)->popBackScope();
	return node;
	//}
}

} // namespace Autolang

#endif
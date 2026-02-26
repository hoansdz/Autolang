#ifndef DEBUGGER_LOOP_CPP
#define DEBUGGER_LOOP_CPP

#include "frontend/parser/Debugger.hpp"

namespace AutoLang {

WhileNode *loadWhile(in_func, size_t &i) {
	Lexer::Token *token = &context.tokens[i];
	uint32_t firstLine = token->line;
	WhileNode *node = context.whilePool.push(firstLine); // WhilePool managed
	// Condition
	if (!nextTokenSameLine(&token, context.tokens, i, firstLine) ||
	    !expect(token, Lexer::TokenType::LPAREN)) {
		--i;
		throw ParserError(firstLine, "Expected ( after while but not found");
	}
	if (!nextTokenSameLine(&token, context.tokens, i, firstLine)) {
		--i;
		throw ParserError(firstLine,
		                  "Expected expression after while but not found");
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
		                  "Expected command after while but not found");
	}
	loadBody(in_data, node->body.nodes, i);
	return node;
}

ExprNode *loadFor(in_func, size_t &i) {
	Lexer::Token *token = &context.tokens[i];
	uint32_t firstLine = token->line;

	if (!nextTokenSameLine(&token, context.tokens, i, firstLine) ||
	    !expect(token, Lexer::TokenType::LPAREN)) {
		--i;
		throw ParserError(firstLine, "Expected ( but not found");
	}
	if (!nextTokenSameLine(&token, context.tokens, i, firstLine)) {
		--i;
		throw ParserError(firstLine, "Expected name but not found");
	}
	if (!expect(token, Lexer::TokenType::IDENTIFIER)) {
		--i;
		throw ParserError(context.tokens[i].line,
		                  "Expected name but not found");
	}
	std::string &name = context.lexerString[token->indexData];
	VarNode *declaration;
	context.getCurrentFunctionInfo(in_data)->scopes.emplace_back();
	// Create temp declaration
	auto declarationNode = context.makeDeclarationNode(
	    in_data, token->line, true, name, nullptr, true,
	    context.currentFunctionId == context.mainFunctionId, false, true);
	declarationNode->classId = AutoLang::DefaultClass::nullClassId;
	declaration =
	    context.varPool.push(firstLine, declarationNode, false, false);
	if (!nextToken(&token, context.tokens, i) ||
	    !expect(token, Lexer::TokenType::IN)) {
		--i;
		throw ParserError(context.tokens[i].line,
		                  "Expected 'in' but not found");
	}
	if (!nextToken(&token, context.tokens, i)) {
		--i;
		throw ParserError(context.tokens[i].line,
		                  "Expected expression after 'in' but not found");
	}
	HasClassIdNode *data = loadExpression(in_data, 0, i);
	VarNode *iteratorNode = nullptr;
	if (data->kind != NodeType::RANGE) {
		// Create temp declaration
		auto declarationNode = context.makeDeclarationNode(
		    in_data, token->line, true, ".iterator", nullptr, true,
		    context.currentFunctionId == context.mainFunctionId, false, true);
		declarationNode->classId = AutoLang::DefaultClass::intClassId;
		iteratorNode =
		    context.varPool.push(firstLine, declarationNode, false, false);
	}
	if (!nextToken(&token, context.tokens, i) ||
	    !expect(token, Lexer::TokenType::RPAREN)) {
		--i;
		throw ParserError(context.tokens[i].line, "Expected ')' but not found");
	}
	if (!nextToken(&token, context.tokens, i)) {
		--i;
		throw ParserError(context.tokens[i].line,
		                  "Expected function body but not found");
	}
	auto node =
	    context.forPool.push(firstLine, declaration, data, iteratorNode);
	loadBody(in_data, node->body.nodes, i, false);
	context.getCurrentFunctionInfo(in_data)->popBackScope();
	return node;
	//}
}

} // namespace AutoLang

#endif
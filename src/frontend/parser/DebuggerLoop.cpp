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
	uint8_t createNewDeclaration = 0; // None: 0, Var: 1, Val: 2
	if (expect(token, Lexer::TokenType::VAR) ||
	    expect(token, Lexer::TokenType::VAL)) {
		createNewDeclaration = expect(token, Lexer::TokenType::VAR) ? 1 : 2;
		if (!nextTokenSameLine(&token, context.tokens, i, firstLine)) {
			--i;
			throw ParserError(context.tokens[i].line,
			                  "Expected name but not found");
		}
	}
	if (!expect(token, Lexer::TokenType::IDENTIFIER)) {
		--i;
		throw ParserError(context.tokens[i].line,
		                  "Expected name but not found");
	}
	std::string &name = context.lexerString[token->indexData];
	AccessNode *declaration;
	if (createNewDeclaration) {
		context.getCurrentFunctionInfo(in_data)->scopes.emplace_back();
		// Create temp declaration
		auto declarationNode = context.makeDeclarationNode(
		    in_data, token->line, true, name, nullptr,
		    createNewDeclaration != 1,
		    context.currentFunctionId == context.mainFunctionId, false, true);
		declarationNode->classId = AutoLang::DefaultClass::intClassId;
		declaration =
		    context.varPool.push(firstLine, declarationNode, false, true);
	} else {
		declaration = static_cast<AccessNode *>(
		    context.getCurrentFunctionInfo(in_data)->findDeclaration(
		        in_data, firstLine, name));
		if (!declaration) {
			--i;
			throw ParserError(context.tokens[i].line,
			                  "Cannot find variable name: " + name);
		}
	}
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
	HasClassIdNode *first = loadExpression(in_data, 0, i);
	// if (first->kind == NodeType::CONST || first->kind == NodeType::VAR) {
	if (!nextToken(&token, context.tokens, i) ||
	    !expect(token, Lexer::TokenType::DOT_DOT)) {
		--i;
		throw ParserError(context.tokens[i].line, "Expected .. but not found");
	}
	if (!nextToken(&token, context.tokens, i)) {
		--i;
		throw ParserError(context.tokens[i].line,
		                  "Expected expression after '..' but not found");
	}
	bool isLessEqThan = true;
	if (expect(token, Lexer::TokenType::LT)) {
		isLessEqThan = false;
		if (!nextToken(&token, context.tokens, i)) {
			--i;
			throw ParserError(context.tokens[i].line,
			                  "Expected value after '..<' but not found");
		}
	}
	HasClassIdNode *second = loadExpression(in_data, 0, i);
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
	auto node = context.forRangePool.push(firstLine, declaration, first, second,
	                                      isLessEqThan);
	loadBody(in_data, node->body.nodes, i);
	if (createNewDeclaration) {
		context.getCurrentFunctionInfo(in_data)->popBackScope();
	}
	return node;
	//}
}

} // namespace AutoLang

#endif
#ifndef DEBUGGER_LOOP_CPP
#define DEBUGGER_LOOP_CPP

#include "DebuggerLoop.hpp"

namespace AutoLang
{

WhileNode* loadWhile(in_func, size_t& i) {
	if (!context.keywords.empty())
		throw std::runtime_error("Invalid keyword");
	WhileNode* node = context.whilePool.push(); //WhilePool managed
	Lexer::Token* token;
	//Condition
	if (!nextToken(&token, context.tokens, i) ||
		 !expect(token, Lexer::TokenType::LPAREN)) {
		throw std::runtime_error("Expected ( after while but not found");
	}
	if (!nextToken(&token, context.tokens, i)) 
		throw std::runtime_error("Expected expression after while but not found");
	node->condition = loadExpression(in_data, 0, i);
	if (!nextToken(&token, context.tokens, i) ||
		 !expect(token, Lexer::TokenType::RPAREN)) {
		throw std::runtime_error("Expected ) but not found");
	}
	if (!nextToken(&token, context.tokens, i))
		throw std::runtime_error("Expected command after while but not found");
	loadBody(in_data, node->body.nodes, i);
	return node;
}

ExprNode* loadFor(in_func, size_t& i) {
	if (!context.keywords.empty())
		throw std::runtime_error("Invalid keyword");
	Lexer::Token *token;
	if (!nextToken(&token, context.tokens, i) ||
		 !expect(token, Lexer::TokenType::LPAREN)) {
		throw std::runtime_error("Expected ( but not found");
	}
	if (!nextToken(&token, context.tokens, i)) {
		throw std::runtime_error("Expected name but not found");
	}
	uint8_t createNewDeclaration = 0; //None: 0, Var: 1, Val: 2
	if (expect(token, Lexer::TokenType::VAR) || expect(token, Lexer::TokenType::VAL)) {
		createNewDeclaration = expect(token, Lexer::TokenType::VAR) ? 1 : 2;
		if (!nextToken(&token, context.tokens, i)) {
			throw std::runtime_error("Expected name but not found");
		}
	}
	if (!expect(token, Lexer::TokenType::IDENTIFIER)) {
		throw std::runtime_error("Expected name but not found");
	}
	std::string& name = context.lexerString[token->indexData];
	std::unique_ptr<AccessNode> declaration;
	if (createNewDeclaration) {
		context.getCurrentFunctionInfo(in_data)->scopes.emplace_back();
		//Create temp declaration
		auto declarationNode = context.makeDeclarationNode(
			in_data, true, name, "", createNewDeclaration == 2, context.currentFunctionId == context.mainFunctionId, false
		);
		declarationNode->classId = AutoLang::DefaultClass::INTCLASSID;
		declaration.reset(new VarNode(
			declarationNode,
			false,
			true
		));
	} else {
		declaration.reset(static_cast<AccessNode*>(context.getCurrentFunctionInfo(in_data)->findDeclaration(in_data, name)));
		if (!declaration) {
			throw std::runtime_error("Cannot find variable name: "+name);
		}
	}
	if (!nextToken(&token, context.tokens, i) ||
		!expect(token, Lexer::TokenType::IN)) {
		throw std::runtime_error("Expected 'in' but not found");
	}
	if (!nextToken(&token, context.tokens, i)) {
		throw std::runtime_error("Expected expression after 'in' but not found");
	}
	auto first = std::unique_ptr<HasClassIdNode>(loadExpression(in_data, 0, i));
	//if (first->kind == NodeType::CONST || first->kind == NodeType::VAR) {
		if (!nextToken(&token, context.tokens, i) ||
			!expect(token, Lexer::TokenType::DOT_DOT)) {
			throw std::runtime_error("Expected .. but not found");
		}
		if (!nextToken(&token, context.tokens, i)) {
			throw std::runtime_error("Expected expression after '..' but not found");
		}
		bool isLessEqThan = true;
		if (expect(token, Lexer::TokenType::LT)) {
			isLessEqThan = false;
			if (!nextToken(&token, context.tokens, i)) {
				throw std::runtime_error("Expected value after '..<' but not found");
			}
		}
		auto second = std::unique_ptr<HasClassIdNode>(loadExpression(in_data, 0, i));
		if (!nextToken(&token, context.tokens, i) ||
			!expect(token, Lexer::TokenType::RPAREN)) {
			throw std::runtime_error("Expected ')' but not found");
		}
		if (!nextToken(&token, context.tokens, i)) {
			throw std::runtime_error("Expected function body but not found");
		}
		auto node = std::make_unique<ForRangeNode>(
			declaration.release(), first.release(), second.release(), isLessEqThan
		);
		loadBody(in_data, node->body.nodes, i);
		if (createNewDeclaration) {
			context.getCurrentFunctionInfo(in_data)->popBackScope();
		}
		return node.release();
	//}
}

}

#endif
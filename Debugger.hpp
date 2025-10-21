#ifndef DEBUGGER_HPP
#define DEBUGGER_HPP

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <exception>
#include <memory>
#include "ParserContext.hpp"
#include "DefaultFunction.hpp"
#include "DefaultOperator.hpp"
#include "DefaultClass.hpp"
#include "CreateNode.hpp"

namespace AutoLang
{

void build(CompiledProgram& compile, std::string path);
void resolve(in_func);
ExprNode* loadLine(in_func, size_t& i);
std::vector<HasClassIdNode*> loadListArgument(in_func, size_t& i);
std::vector<DeclarationNode*> loadListDeclaration(in_func, size_t& i, bool allowVar = false);
HasClassIdNode* loadExpression(in_func, int minPrecedence, size_t& i);
HasClassIdNode* loadDeclaration(in_func, size_t& i);
HasClassIdNode* parsePrimary(in_func, size_t& i);
HasClassIdNode* loadIdentifier(in_func, size_t& i, bool allowAddThis = true);
void loadBody(in_func, std::vector<ExprNode*>& nodes, size_t& i, bool createScope = true);
IfNode* loadIf(in_func, size_t& i);
ExprNode* loadFor(in_func, size_t& i);
WhileNode* loadWhile(in_func, size_t& i);
CreateFuncNode* loadFunc(in_func, size_t& i);
void loadConstructor(in_func, size_t& i);
CreateClassNode* loadClass(in_func, size_t& i);
// void loadClassInit(in_func, size_t& i);
ReturnNode* loadReturn(in_func, size_t& i);
ConstValueNode* loadNumber(in_func, size_t& i);
HasClassIdNode* findIdentifierNode(in_func, std::string& name);
HasClassIdNode* findVarNode(in_func, std::string& name);
ConstValueNode* findConstValueNode(in_func, std::string& name);
char getCloseBracket(char chr);
char getOpenBracket(Lexer::TokenType type);
bool isCloseBracket(char openBracket, Lexer::TokenType closeBracket);
int getPrecedence(Lexer::TokenType type);

inline void putBlock(in_func, BlockNode* node, std::vector<uint8_t>& bytecodes) {
	node->optimize(in_data);
	node->putBytecodes(in_data, bytecodes);
	node->rewrite(in_data, bytecodes);
}

inline bool expect(Lexer::Token* token, Lexer::TokenType type) {
	return token->type == type;
}

inline bool expectSameLine(Lexer::Token* token, Lexer::TokenType type, int line) {
	return token->type == type || token->line == line;
}

inline bool nextToken(Lexer::Token** token, std::vector<Lexer::Token>& tokens, size_t& i) {
	++i;
	if (i >= tokens.size()) return false;
	*token = &tokens[i];
	return true;
}

inline bool nextTokenSameLine(Lexer::Token** token, std::vector<Lexer::Token>& tokens, size_t& i, int line) {
	++i;
	if (i >= tokens.size()) return false;
	*token = &tokens[i];
	return (*token)->line == line;
}

std::string logLine(Lexer::Token *token);

}

#endif
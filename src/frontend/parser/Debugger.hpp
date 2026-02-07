#ifndef DEBUGGER_HPP
#define DEBUGGER_HPP

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <exception>
#include <memory>
#include "frontend/parser/ParserContext.hpp"
#include "shared/DefaultFunction.hpp"
#include "shared/DefaultOperator.hpp"
#include "shared/DefaultClass.hpp"
#include "frontend/parser/node/CreateNode.hpp"

namespace AutoLang
{

struct ParserError : AutoLang::Lexer::LexerError {
	ParserError(uint32_t line, std::string msg): AutoLang::Lexer::LexerError(line, msg){}
};

void lexerData(in_func, AVMReadFileMode& mode, Lexer::Context& lexerContext);
void estimate(in_func, Lexer::Context& lexerContext);
void freeData(in_func);
inline void ensureNoKeyword(in_func, size_t& i);
inline void ensureNoAnnotations(in_func, size_t& i);
Lexer::TokenType getAndEnsureOneAccessModifier(in_func, size_t& i);
void ensureEndline(in_func, size_t& i);
ExprNode* loadLine(in_func, size_t& i);
std::vector<HasClassIdNode*> loadListArgument(in_func, size_t& i);
std::vector<DeclarationNode*> loadListDeclaration(in_func, size_t& i, bool allowVar = false);
ClassDeclaration loadClassDeclaration(in_func, size_t& i, uint32_t line);
HasClassIdNode* loadExpression(in_func, int minPrecedence, size_t& i);
HasClassIdNode* loadDeclaration(in_func, size_t& i);
HasClassIdNode* parsePrimary(in_func, size_t& i);
HasClassIdNode* loadIdentifier(in_func, size_t& i, bool allowAddThis = true);
bool nextTokenIfMarkNonNull(in_func, size_t& i);
void loadAnnotations(in_func, size_t& i);
void loadBody(in_func, std::vector<ExprNode*>& nodes, size_t& i, bool createScope = true);
IfNode* loadIf(in_func, size_t& i);
ExprNode* loadFor(in_func, size_t& i);
WhileNode* loadWhile(in_func, size_t& i);
TryCatchNode* loadTryCatch(in_func, size_t& i);
ThrowNode* loadThrow(in_func, size_t& i);
CreateFuncNode* loadFunc(in_func, size_t& i);
void loadConstructor(in_func, size_t& i);
CreateClassNode* loadClass(in_func, size_t& i);
// void loadClassInit(in_func, size_t& i);
ReturnNode* loadReturn(in_func, size_t& i);
ConstValueNode* loadNumber(in_func, size_t& i);
HasClassIdNode* findIdentifierNode(in_func, size_t& i, std::string& name, bool nullable);
HasClassIdNode* findVarNode(in_func, size_t& i, std::string& name, bool nullable);
ConstValueNode* findConstValueNode(in_func, size_t& i, std::string& name);
char getOpenBracket(Lexer::TokenType type);
bool isCloseBracket(char openBracket, Lexer::TokenType closeBracket);
int getPrecedence(Lexer::TokenType type);

inline bool expect(Lexer::Token* token, Lexer::TokenType type) {
	return token->type == type;
}

inline bool expectSameLine(Lexer::Token* token, Lexer::TokenType type, uint32_t line) {
	return token->type == type || token->line == line;
}

inline bool nextToken(Lexer::Token** token, std::vector<Lexer::Token>& tokens, size_t& i) {
	++i;
	if (i >= tokens.size()) return false;
	*token = &tokens[i];
	return true;
}

inline bool nextTokenSameLine(Lexer::Token** token, std::vector<Lexer::Token>& tokens, size_t& i, uint32_t line) {
	++i;
	if (i >= tokens.size()) return false;
	*token = &tokens[i];
	return (*token)->line == line;
}

}

#endif
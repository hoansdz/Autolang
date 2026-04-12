#ifndef DEBUGGER_HPP
#define DEBUGGER_HPP

#include "frontend/parser/node/CreateFuncNode.hpp"
#include "frontend/parser/node/CreateNode.hpp"
#include "frontend/parser/node/Node.hpp"
#include "shared/DefaultClass.hpp"
#include <exception>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace AutoLang {

struct ParserContext;

struct ParserError : AutoLang::Lexer::LexerError {
	ParserError(uint32_t line, std::string msg)
	    : AutoLang::Lexer::LexerError(line, msg) {}
};

inline void lexerData(in_func, ACompiler &compiler, LibraryData *library,
                      std::vector<Offset> *importOffset) {
	// auto startLexer = std::chrono::high_resolution_clock::now();
	Lexer::load(&context, library, importOffset);
	// auto lexerTime = std::chrono::high_resolution_clock::now();
	// auto total = std::chrono::duration_cast<std::chrono::milliseconds>(
	//                  lexerTime - startLexer)
	//                  .count();
	// std::cerr << "Lexer file " << library->path << " in  " << total << "
	// ms\n";
}
LibraryData *loadImport(in_func, LibraryData *currentLibrary,
                        std::vector<Lexer::Token> &tokens, ACompiler &compiler,
                        size_t i);
void estimate(in_func, Lexer::Context &lexerContext);
void freeData(in_func);
ClassId loadClassGenerics(in_func, std::string &name,
                          ClassDeclaration *classDeclaration);
FunctionId loadFunctionGenerics(in_func, std::string &name,
                                ClassDeclaration *classDeclaration);
inline void ensureNoKeyword(in_func, size_t &i);
inline void ensureNoAnnotations(in_func, size_t &i);
Lexer::TokenType getAndEnsureOneAccessModifier(in_func, size_t &i);
void ensureEndline(in_func, size_t &i);
ExprNode *loadLine(in_func, size_t &i);
template <bool trailingComma = false>
std::vector<HasClassIdNode *> loadListArgument(in_func, size_t &i);
Parameter *loadListDeclaration(in_func, size_t &i, bool allowVar = false);
std::vector<ClassDeclaration *> loadListClassDeclaration(in_func, size_t &i,
                                                         uint32_t line,
                                                         bool allowReturnVoid,
                                                         bool &isGeneric);
ClassDeclaration *loadClassDeclaration(in_func, size_t &i, uint32_t line,
                                       bool allowReturnVoid);
void loadListGenericDeclarationType(in_func, size_t &i, uint32_t line,
                                    bool allowReturnVoid,
                                    std::vector<ClassDeclaration *> &inputVecs,
                                    bool &isGeneric);
HasClassIdNode *loadSetOrMap(in_func, size_t &i, NodeType canBeNodeType);
HasClassIdNode *loadSet(in_func, size_t &i, HasClassIdNode *firstExpression);
HasClassIdNode *loadMap(in_func, size_t &i, HasClassIdNode *firstExpression);
HasClassIdNode *parsePrimary(in_func, size_t &i);
HasClassIdNode *loadExpression(in_func, int minPrecedence, size_t &i);
void loadEnum(in_func, size_t &i);
HasClassIdNode *loadDeclaration(in_func, size_t &i);
HasClassIdNode *parsePrimary(in_func, size_t &i);
HasClassIdNode *loadIdentifier(in_func, size_t &i, bool allowAddThis = true);
bool nextTokenIfMarkNonNull(in_func, size_t &i);
void loadAnnotations(in_func, size_t &i);
template <bool loadedLBrace>
void loadBody(in_func, std::vector<ExprNode *> &nodes, size_t &i,
              bool createScope = true);
IfNode *loadIf(in_func, size_t &i, bool mustReturnValue);
WhenNode *loadWhen(in_func, size_t &i, bool mustReturnValue);
ExprNode *loadFor(in_func, size_t &i);
WhileNode *loadWhile(in_func, size_t &i);
TryCatchNode *loadTryCatch(in_func, size_t &i);
ThrowNode *loadThrow(in_func, size_t &i);
CreateFuncNode *loadFunc(in_func, size_t &i);
void loadConstructor(in_func, size_t &i);
CreateClassNode *loadClass(in_func, size_t &i);
// void loadClassInit(in_func, size_t& i);
ReturnNode *loadReturn(in_func, size_t &i);
ConstValueNode *loadNumber(in_func, size_t &i);
HasClassIdNode *findIdentifierNode(in_func, size_t &i, LexerStringId nameId,
                                   bool nullable);
HasClassIdNode *findVarNode(in_func, size_t &i, LexerStringId nameId,
                            bool nullable);
ConstValueNode *findConstValueNode(in_func, size_t &i, LexerStringId nameId);
char getOpenBracket(Lexer::TokenType type);
bool isCloseBracket(char openBracket, Lexer::TokenType closeBracket);
int getPrecedence(Lexer::TokenType type);

inline bool expect(Lexer::Token *token, Lexer::TokenType type) {
	return token->type == type;
}

inline bool expectSameLine(Lexer::Token *token, Lexer::TokenType type,
                           uint32_t line) {
	return token->type == type || token->line == line;
}

inline bool nextToken(Lexer::Token **token, std::vector<Lexer::Token> &tokens,
                      size_t &i) {
	++i;
	if (i >= tokens.size())
		return false;
	*token = &tokens[i];
	return true;
}

inline bool nextTokenSameLine(Lexer::Token **token,
                              std::vector<Lexer::Token> &tokens, size_t &i,
                              uint32_t line) {
	++i;
	if (i >= tokens.size())
		return false;
	*token = &tokens[i];
	return (*token)->line == line;
}

} // namespace AutoLang

#endif
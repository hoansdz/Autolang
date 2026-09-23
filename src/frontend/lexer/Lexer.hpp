#ifndef LEXER_HPP
#define LEXER_HPP

#include "shared/Type.hpp"
#include "third_party/ankerl/unordered_dense.h"
#include <exception>
#include <fstream>
#include <iostream>
#include <vector>

#define AUTOLANG_DEBUG_

inline void printDebug(std::string msg) {
#ifdef AUTOLANG_DEBUG
	std::cerr << msg << '\n';
#endif
}

inline void printDebug(long msg) {
#ifdef AUTOLANG_DEBUG
	std::cerr << msg << '\n';
#endif
}

namespace Autolang {

struct LibraryData;
struct ParserContext;

namespace Lexer {

enum TokenType : uint8_t {
	COMMENT_SINGLE_LINE,
	// ===== Literals =====
	NUMBER,
	STRING,
	IDENTIFIER,

	START_COMMENT,

	// ===== Operators =====
	PLUS,
	PLUS_PLUS,
	MINUS,
	MINUS_MINUS,
	STAR,
	SLASH,
	PERCENT,
	EQUAL,
	PLUS_EQUAL,
	MINUS_EQUAL,
	STAR_EQUAL,
	SLASH_EQUAL,
	PERCENT_EQUAL,

	// ===== Comparisons =====
	EQEQ,
	NOTEQ,
	EQEQEQ,
	NOTEQEQ,
	LT,
	GT,
	LTE,
	GTE,

	// ===== Logical operators =====
	AND_AND,
	OR_OR,

	// ===== Delimiters =====
	LPAREN,
	RPAREN,
	LBRACE,
	RBRACE,
	LBRACKET,
	RBRACKET,
	COMMA,
	DOT,
	QMARK_DOT, //    ?. operator
	DOT_DOT,
	DOT_DOT_LT,
	SEMICOLON,
	COLON,
	COLON_COLON,
	QMARK, //   Question mark ?
	QMARK_QMARK,
	EXMARK,  //   Exclamation mark !
	AT_SIGN, // @

	// ===== Keywords =====
	IF,
	ELSE,
	WHILE,
	FOR,
	FUNC,
	RETURN,
	VAL,
	VAR,
	BREAK,
	CONTINUE,
	NOT,
	AND,
	OR,
	IN_,
	PUBLIC,
	PRIVATE,
	PROTECTED,
	CLASS,
	CONSTRUCTOR,
	STATIC,
	TRY,
	CATCH,
	FINALLY,
	THROW,
	EXTENDS,
	NATIVE,
#ifdef __EMSCRIPTEN__
	JS_OBJECT,
#elif __PYBIND11__
	PY_OBJECT,
#endif
	OVERRIDE,
	NO_OVERRIDE,
	NO_CONSTRUCTOR,
	NO_EXTENDS,
	NATIVE_DATA,
	IMPORT,
	END_IMPORT,
	WAIT_INPUT,
	IS,
	NOT_IS,
	NOT_IN,
	SAFE_CAST,
	UNSAFE_CAST,
	LATEINIT,
	ENUM,
	SEMI_COLON,
	CONST,
	WHEN,
	MINUS_GT,
	TYPEALIAS,
	OPERATOR,
	IMPLICIT,
	TO,

	// ===== Special =====
	NON_NULL,
	END_OF_FILE,
	INVALID
};

struct Token {
	uint32_t line;
	LexerStringId indexData;
	TokenType type;
	Token() {}
	Token(uint32_t line, TokenType type, LexerStringId indexData)
	    : line(line), indexData(indexData), type(type) {}
	Token(uint32_t line, TokenType type)
	    : line(line), indexData(0), type(type) {}
	std::string toString(ParserContext &context);
};

class LexerError : public std::exception {
  public:
	uint32_t line;
	std::string message;
	LexerError(uint32_t line, std::string msg)
	    : line(line), message(std::move(msg)) {}
	const char *what() const noexcept override { return message.c_str(); }
};

struct Estimate {
	uint32_t declaration = 0;
	uint32_t classes = 0;
	uint32_t functions = 0;
	uint32_t constructorNode = 0;
	// uint32_t ifNode = 0;
	// uint32_t whileNode = 0;
	// uint32_t returnNode = 0;
	// uint32_t setNode = 0;
	// uint32_t binaryNode = 0;
	// uint32_t tryCatchNode = 0;
	// uint32_t throwNode = 0;
};

struct Context {
	ParserContext *mainContext;
	const char *line;
	size_t lineSize;
	size_t nextLinePosition = 0;
	size_t totalSize;
	uint32_t linePos;
	uint32_t pos;
	uint32_t absolutePos;
	std::vector<char> bracketStack;
	std::vector<Token> tokens;
	LibraryData *library;
	std::vector<Offset> *importOffset;

	bool hasError = false;

	Estimate estimate;
	Context() {
		tokens.reserve(256);
		bracketStack.reserve(16);
	}
	inline void refresh() {
		bracketStack.clear();
		tokens.clear();
		hasError = false;
	}
};

inline bool isEndOfLine(Context &context, uint32_t i) { return i >= context.lineSize; }
bool nextLine(Context &context, const char *lines, uint32_t &i);
bool isOperator(char chr);
void loadFile(ParserContext *mainContext, LibraryData *library);
void load(ParserContext *mainContext, LibraryData *library,
          std::vector<Offset> *importOffset);
template <bool addLParen, bool isChar, bool isRawString>
void loadQuote(Context &context, uint32_t &i);
std::string loadIdentifier(Context &context, uint32_t &i);
std::string_view loadIdentifierView(Context &context, uint32_t &i);
std::string loadNumber(Context &context, uint32_t &i);
TokenType loadOp(Context &context, uint32_t &i);
bool loadNextTokenNoCloseBracket(Context &context, uint32_t &i);
void pushAndEnsureBracket(Context &context, uint32_t &i);
void pushIdentifier(Context &context, uint32_t &i);
uint32_t pushLexerString(Context &context, std::string &&str);
uint32_t pushLexerString(Context &context, std::string_view str);
uint32_t pushLexerString(Context &context, const char *str);
char getCloseBracket(char chr);

} // namespace Lexer
} // namespace Autolang

#endif
#ifndef DEBUGGER_UTIL_CPP
#define DEBUGGER_UTIL_CPP

#include "frontend/ACompiler.hpp"
#include "frontend/parser/Debugger.hpp"
#include "frontend/parser/ParserContext.hpp"

namespace Autolang {

void ensureNoKeyword(in_func, size_t &i) {
	if (!context.modifierflags)
		return;
	throw ParserError(
	    context.tokens[i].line,
	    "Command doesn't support any keyword\nHint: Remove modifiers (public, "
	    "private, static, etc.) from this command");
}

void ensureNoAnnotations(in_func, size_t &i) {
	if (!context.annotationFlags)
		return;
	throw ParserError(context.tokens[i].line,
	                  "Command doesn't support any annotations\nHint: Remove "
	                  "'@' annotations from this command");
}

Lexer::TokenType getAndEnsureOneAccessModifier(in_func, size_t &i) {
	// No keywords
	if (!context.modifierflags)
		return Lexer::TokenType::PUBLIC;
	if (context.modifierflags & ModifierFlags::MF_STATIC)
		throw ParserError(
		    context.tokens[i].line,
		    "Command doesn't support 'static' keyword\nHint: Remove 'static' "
		    "keyword");
	if (context.modifierflags & ModifierFlags::MF_LATEINIT)
		throw ParserError(
		    context.tokens[i].line,
		    "Command doesn't support 'lateinit' keyword\nHint: Remove "
		    "'lateinit' keyword");
	switch (context.modifierflags) {
		case ModifierFlags::MF_PUBLIC:
			return Lexer::TokenType::PUBLIC;
		case ModifierFlags::MF_PRIVATE:
			return Lexer::TokenType::PRIVATE;
		case ModifierFlags::MF_PROTECTED:
			return Lexer::TokenType::PROTECTED;
		default:
			throw ParserError(
			    0, "Bug: Parser did not ensure exactly one modifier\nHint: Use "
			       "only "
			       "one access modifier (public, private, or protected)");
	}
}

char getOpenBracket(Lexer::TokenType type) {
	switch (type) {
		case Lexer::TokenType::LPAREN:
			return '(';
		case Lexer::TokenType::LBRACKET:
			return '[';
		case Lexer::TokenType::LBRACE:
			return '{';
		default:
			return '\0';
	}
}

bool isCloseBracket(char openBracket, Lexer::TokenType closeBracket) {
	switch (openBracket) {
		case '(':
			return closeBracket == Lexer::TokenType::RPAREN;
		case '[':
			return closeBracket == Lexer::TokenType::RBRACKET;
		case '{':
			return closeBracket == Lexer::TokenType::RBRACE;
		default:
			return false;
	}
}

int getPrecedence(Lexer::TokenType type) {
	switch (type) {
		case Lexer::TokenType::DOT_DOT_LT:
		case Lexer::TokenType::DOT_DOT: {
			return 9;
		}
		case Lexer::TokenType::PLUS:
		case Lexer::TokenType::MINUS: {
			return 10;
		}
		case Lexer::TokenType::STAR:
		case Lexer::TokenType::PERCENT:
		case Lexer::TokenType::SLASH:
		case Lexer::TokenType::AND:
		case Lexer::TokenType::OR: {
			return 20;
		}
		case Lexer::TokenType::QMARK_QMARK: {
			return 10;
		}
		case Lexer::TokenType::SAFE_CAST:
		case Lexer::TokenType::UNSAFE_CAST:
		case Lexer::TokenType::IS:
		case Lexer::TokenType::NOT_IS:
		case Lexer::TokenType::EQEQ:
		case Lexer::TokenType::NOTEQ:
		case Lexer::TokenType::EQEQEQ:
		case Lexer::TokenType::NOTEQEQ:
		case Lexer::TokenType::LTE:
		case Lexer::TokenType::GTE:
		case Lexer::TokenType::LT:
		case Lexer::TokenType::GT: {
			return 7;
		}
		case Lexer::TokenType::IN_:
		case Lexer::TokenType::NOT_IN: {
			return 5;
		}
		case Lexer::TokenType::OR_OR:
		case Lexer::TokenType::AND_AND: {
			return 3;
		}
		default:
			return -1;
	}
}

} // namespace Autolang

#endif

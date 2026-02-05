#ifndef DEBUGGER_ANNOTATIONS_CPP
#define DEBUGGER_ANNOTATIONS_CPP

#include "Debugger.hpp"

namespace AutoLang {

void loadAnnotations(in_func, size_t &i) {
	Lexer::Token *token = &context.tokens[i];
	uint32_t firstLine = token->line;
	if (!nextTokenSameLine(&token, context.tokens, i, firstLine)) {
		std::cerr << ((int)(token->type == Lexer::TokenType::AT_SIGN)) << " "
		          << token->line << "\n";
		--i;
		throw ParserError(firstLine, "Unexpected @");
	}
	switch (token->type) {
		case Lexer::TokenType::OVERRIDE: {
			if (context.annotationFlags & AnnotationFlags::AN_OVERRIDE) {
				throw ParserError(firstLine, "Duplicate annotation @override");
			}
			context.annotationFlags |= AnnotationFlags::AN_OVERRIDE;
			break;
		}
		case Lexer::TokenType::NO_OVERRIDE: {
			if (context.annotationFlags & AnnotationFlags::AN_NO_OVERRIDE) {
				throw ParserError(firstLine,
				                  "Duplicate annotation @no_override");
			}
			context.annotationFlags |= AnnotationFlags::AN_NO_OVERRIDE;
			break;
		}
		case Lexer::TokenType::NATIVE: {
			if (context.annotationFlags & AnnotationFlags::AN_NATIVE) {
				throw ParserError(firstLine, "Duplicate annotation @native");
			}
			context.annotationFlags |= AnnotationFlags::AN_NATIVE;
			if (!nextTokenSameLine(&token, context.tokens, i, firstLine) ||
			    !expect(token, Lexer::TokenType::LPAREN)) {
				--i;
				throw ParserError(firstLine, "@native expected string value");
			}
			if (!nextTokenSameLine(&token, context.tokens, i, firstLine) ||
			    !expect(token, Lexer::TokenType::STRING)) {
				throw ParserError(firstLine, "@native expected string value");
			}
			context.annotationMetadata[AnnotationFlags::AN_NATIVE] = *token;
			if (!nextTokenSameLine(&token, context.tokens, i, firstLine) ||
			    !expect(token, Lexer::TokenType::RPAREN)) {
				--i;
				throw ParserError(firstLine, "@native expects a constant "
				                             "string value, not an expression");
			}
			break;
		}
		case Lexer::TokenType::NO_CONSTRUCTOR: {
			if (context.annotationFlags & AnnotationFlags::AN_NO_CONSTRUCTOR) {
				throw ParserError(firstLine,
				                  "Duplicate annotation @no_constructor");
			}
			context.annotationFlags |= AnnotationFlags::AN_NO_CONSTRUCTOR;
			break;
		}
		default: {
			throw ParserError(firstLine, "Unknown annotation '@" +
			                                 token->toString(context) + "'");
		}
	}
}

} // namespace AutoLang

#endif
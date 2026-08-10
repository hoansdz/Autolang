#ifndef DEBUGGER_ANNOTATIONS_CPP
#define DEBUGGER_ANNOTATIONS_CPP

#include "Debugger.hpp"
#include "frontend/ACompiler.hpp"
#include "frontend/parser/ParserContext.hpp"
#include <filesystem>

namespace Autolang {

void loadAnnotations(in_func, size_t &i) {
	Lexer::Token *token = &context.tokens[i];
	uint32_t firstLine = token->line;
	if (!nextTokenSameLine(&token, context.tokens, i, firstLine)) {
		std::cerr << ((int)(token->type == Lexer::TokenType::AT_SIGN)) << " "
		          << token->line << "\n";
		--i;
		throw ParserError(
		    firstLine, "Expected annotation name after '@'\nHint: Write a "
		               "valid annotation such as @override, @native(\"name\"), "
		               "@no_constructor, @no_extends, or @native_data.");
	}
	switch (token->type) {
		case Lexer::TokenType::OVERRIDE: {
			if (context.annotationFlags & AnnotationFlags::AN_OVERRIDE) {
				throw ParserError(firstLine,
				                  "Duplicate annotation @override\nHint: Use "
				                  "@override only once before a function "
				                  "declaration, e.g. @override fun foo() { }");
			}
			context.annotationFlags |= AnnotationFlags::AN_OVERRIDE;
			if (!nextToken(&token, context.tokens, i)) {
				--i;
				throw ParserError(
				    context.tokens[i].line,
				    "@override must be followed by a function\nHint: Correct "
				    "syntax is '@override fun foo() { }'");
			}
			--i;
			break;
		}
		case Lexer::TokenType::NO_OVERRIDE: {
			if (context.annotationFlags & AnnotationFlags::AN_NO_OVERRIDE) {
				throw ParserError(
				    firstLine, "Duplicate annotation @no_override\nHint: Use "
				               "@no_override only once before a function "
				               "declaration, e.g. @no_override fun foo() { }");
			}
			context.annotationFlags |= AnnotationFlags::AN_NO_OVERRIDE;
			if (!nextToken(&token, context.tokens, i)) {
				--i;
				throw ParserError(
				    context.tokens[i].line,
				    "@no_override must be followed by a function\nHint: "
				    "Correct syntax is '@no_override fun foo() { }'");
			}
			--i;
			break;
		}
		case Lexer::TokenType::WAIT_INPUT: {
			throw ParserError(firstLine,
			                  "@wait_input is currently not supported\nHint: "
			                  "Remove @wait_input annotation");
		}
		case Lexer::TokenType::NATIVE: {
			if (context.annotationFlags & AnnotationFlags::AN_NATIVE) {
				throw ParserError(
				    firstLine,
				    "Duplicate annotation @native\nHint: Use @native only once "
				    "before a function declaration, e.g. @native(\"name\") fun "
				    "foo() { }");
			}
			context.annotationFlags |= AnnotationFlags::AN_NATIVE;
			if (!nextTokenSameLine(&token, context.tokens, i, firstLine) ||
			    !expect(token, Lexer::TokenType::LPAREN)) {
				--i;
				throw ParserError(
				    firstLine,
				    "@native expects a string value in parentheses\nHint: "
				    "Write @native(\"native_name\") before the function");
			}
			if (!nextTokenSameLine(&token, context.tokens, i, firstLine) ||
			    !expect(token, Lexer::TokenType::STRING)) {
				throw ParserError(
				    firstLine,
				    "@native expects a string value\nHint: Provide a string "
				    "literal inside parentheses, e.g. @native(\"name\")");
			}
			context.annotationMetadata[AnnotationFlags::AN_NATIVE] = *token;
			if (!nextTokenSameLine(&token, context.tokens, i, firstLine) ||
			    !expect(token, Lexer::TokenType::RPAREN)) {
				--i;
				throw ParserError(
				    firstLine, "@native expects a constant string value, not "
				               "an expression\nHint: Write @native(\"name\") "
				               "instead of @native(variable)");
			}
			if (!nextToken(&token, context.tokens, i)) {
				--i;
				throw ParserError(
				    context.tokens[i].line,
				    "@native must be followed by a function\nHint: Correct "
				    "syntax is '@native(\"name\") fun foo() { }'");
			}
			--i;
			break;
		}
		case Lexer::TokenType::NO_CONSTRUCTOR: {
			if (context.annotationFlags & AnnotationFlags::AN_NO_CONSTRUCTOR) {
				throw ParserError(
				    firstLine, "Duplicate annotation @no_constructor\nHint: "
				               "Use @no_constructor only once before a class "
				               "declaration, e.g. @no_constructor class A { }");
			}
			context.annotationFlags |= AnnotationFlags::AN_NO_CONSTRUCTOR;
			if (!nextToken(&token, context.tokens, i)) {
				--i;
				throw ParserError(
				    context.tokens[i].line,
				    "@no_constructor must be followed by a class\nHint: "
				    "Correct syntax is '@no_constructor class A { }'");
			}
			--i;
			break;
		}
		case Lexer::TokenType::NO_EXTENDS: {
			if (context.annotationFlags & AnnotationFlags::AN_NO_EXTENDS) {
				throw ParserError(firstLine,
				                  "Duplicate annotation @no_extends\nHint: Use "
				                  "@no_extends only once before a class "
				                  "declaration, e.g. @no_extends class A { }");
			}
			context.annotationFlags |= AnnotationFlags::AN_NO_EXTENDS;
			if (!nextToken(&token, context.tokens, i)) {
				--i;
				throw ParserError(
				    context.tokens[i].line,
				    "@no_extends must be followed by a class\nHint: Correct "
				    "syntax is '@no_extends class A { }'");
			}
			--i;
			break;
		}
		case Lexer::TokenType::NATIVE_DATA: {
			if (context.annotationFlags & AnnotationFlags::AN_NATIVE_DATA) {
				throw ParserError(firstLine,
				                  "Duplicate annotation @native_data\nHint: "
				                  "Use @native_data only once before a native "
				                  "class, e.g. @native_data class A { }");
			}
			context.annotationFlags |= AnnotationFlags::AN_NATIVE_DATA;
			if (!nextToken(&token, context.tokens, i)) {
				--i;
				throw ParserError(
				    context.tokens[i].line,
				    "@native_data must be followed by a class\nHint: Correct "
				    "syntax is '@native_data class A { }'");
			}
			--i;
			break;
		}
#ifdef __EMSCRIPTEN__
		case Lexer::TokenType::JS_OBJECT: {
			if (context.annotationFlags & AnnotationFlags::AN_JS_OBJECT) {
				throw ParserError(firstLine,
				                  "Duplicate annotation @js_object\nHint: Use "
				                  "@js_object only once before a class "
				                  "declaration, e.g. @js_object class A { }");
			}
			context.annotationFlags |= AnnotationFlags::AN_JS_OBJECT;
			if (!nextToken(&token, context.tokens, i)) {
				--i;
				throw ParserError(
				    context.tokens[i].line,
				    "@js_object must be followed by a class\nHint: Correct "
				    "syntax is '@js_object class A { }'");
			}
			--i;
			break;
		}
#elif __PYBIND11__
		case Lexer::TokenType::PY_OBJECT: {
			if (context.annotationFlags & AnnotationFlags::AN_PY_OBJECT) {
				throw ParserError(firstLine,
				                  "Duplicate annotation @py_object\nHint: Use "
				                  "@py_object only once before a class "
				                  "declaration, e.g. @py_object class A { }");
			}
			context.annotationFlags |= AnnotationFlags::AN_PY_OBJECT;
			if (!nextToken(&token, context.tokens, i)) {
				--i;
				throw ParserError(
				    context.tokens[i].line,
				    "@py_object must be followed by a class\nHint: Correct "
				    "syntax is '@py_object class A { }'");
			}
			--i;
			break;
		}
#endif
		case Lexer::TokenType::IMPORT: {
			if (!nextTokenSameLine(&token, context.tokens, i, firstLine) ||
			    !expect(token, Lexer::TokenType::LPAREN)) {
				--i;
				throw ParserError(
				    firstLine,
				    "@import is missing opening '(' bracket\nHint: Correct "
				    "syntax is '@import(\"path/to/file.atl\")'");
			}
			if (!nextTokenSameLine(&token, context.tokens, i, firstLine) ||
			    !expect(token, Lexer::TokenType::STRING)) {
				throw ParserError(
				    firstLine, "@import is missing a string argument\nHint: "
				               "Provide file path string inside parentheses, "
				               "e.g. @import(\"path/to/file.atl\")");
			}
			std::string path = context.lexerString[token->indexData];
			if (path[0] == '.') {
				std::filesystem::path input = path;
				std::filesystem::path currentPath;
				if (context.mode->flags & LibraryFlags::IS_FILE) {
					currentPath =
					    std::filesystem::path(context.mode->path).parent_path();
				} else {
					currentPath = std::filesystem::current_path();
				}
				std::filesystem::path resolved =
				    (currentPath / input).lexically_normal();
				path = resolved.string();
			}
			bool mustAppend =
			    context.importMap.find(path) == context.importMap.end();
			if (!nextTokenSameLine(&token, context.tokens, i, firstLine) ||
			    !expect(token, Lexer::TokenType::RPAREN)) {
				--i;
				throw ParserError(
				    firstLine, "@import is missing closing ')' bracket\nHint: "
				               "Close @import argument with ')'");
			}
			if (!mustAppend)
				break;
			auto it =
			    context.mainLexerContext->library->dependencies.find(path);
			if (it == context.mainLexerContext->library->dependencies.end()) {
				throw ParserError(
				    firstLine,
				    "Library '" + context.mainLexerContext->library->path +
				        "' is missing dependency '" + path +
				        "'\nHint: Verify specified import file path exists");
			}
			auto library = it->second;
			context.mode = library;
			context.importMap[path] = library;
			context.loadingLibs.push_back(library);
			context.tokens.insert(context.tokens.begin() + i + 1,
			                      library->lexerContext.tokens.begin(),
			                      library->lexerContext.tokens.end());
			// for (auto &token : context.tokens) {
			// 	std::cerr << token.toString(context) << " ";
			// }
			// std::cerr << "\n";
			break;
		}
		default: {
			throw ParserError(
			    firstLine,
			    "Unknown annotation '@" + token->toString(context) +
			        "'\nHint: Check annotation name or remove unsupported '@" +
			        token->toString(context) + "'");
		}
	}
}

} // namespace Autolang

#endif

#ifndef LEXER_CPP
#define LEXER_CPP

#include "frontend/lexer/Lexer.hpp"
#include "frontend/ACompiler.hpp"
#include "frontend/parser/ParserContext.hpp"
#include <fstream>

#define ESTIMATE_CASE_ADD(op, val)                                             \
	case TokenType::op:                                                        \
		++context.estimate.val;                                                \
		break;

namespace Autolang {
namespace Lexer {

struct TransparentStringHash {
	using is_transparent = void;
	using is_avalanching = void;
	uint64_t operator()(std::string_view sv) const noexcept {
		return ankerl::unordered_dense::detail::wyhash::hash(sv.data(), sv.size());
	}
	uint64_t operator()(const std::string &s) const noexcept {
		return ankerl::unordered_dense::detail::wyhash::hash(s.data(), s.size());
	}
	uint64_t operator()(const char *s) const noexcept {
		return ankerl::unordered_dense::detail::wyhash::hash(s, std::strlen(s));
	}
};

static const ankerl::unordered_dense::map<std::string, TokenType, TransparentStringHash, std::equal_to<>> KEYWORDS = {
    {"to", TokenType::TO},
    {"var", TokenType::VAR},
    {"val", TokenType::VAL},
    {"const", TokenType::CONST},
    {"not", TokenType::NOT},
    {"while", TokenType::WHILE},
    {"if", TokenType::IF},
    {"else", TokenType::ELSE},
    {"and", TokenType::AND_AND},
    {"for", TokenType::FOR},
    {"in", TokenType::IN_},
    {"or", TokenType::OR_OR},
    {"fun", TokenType::FUNC},
    {"return", TokenType::RETURN},
    {"continue", TokenType::CONTINUE},
    {"break", TokenType::BREAK},
    {"try", TokenType::TRY},
    {"catch", TokenType::CATCH},
    {"finally", TokenType::FINALLY},
    {"throw", TokenType::THROW},
    {"class", TokenType::CLASS},
    {"static", TokenType::STATIC},
    {"private", TokenType::PRIVATE},
    {"public", TokenType::PUBLIC},
    {"protected", TokenType::PROTECTED},
    {"constructor", TokenType::CONSTRUCTOR},
    {"extends", TokenType::EXTENDS},
    {"native", TokenType::NATIVE},
#ifdef __EMSCRIPTEN__
    {"js_object", TokenType::JS_OBJECT},
#elif __PYBIND11__
    {"py_object", TokenType::PY_OBJECT},
#endif
    {"override", TokenType::OVERRIDE},
    {"no_override", TokenType::NO_OVERRIDE},
    {"no_constructor", TokenType::NO_CONSTRUCTOR},
    {"no_extends", TokenType::NO_EXTENDS},
    {"native_data", TokenType::NATIVE_DATA},
    {"import", TokenType::IMPORT},
    {"is", TokenType::IS},
    {"as", TokenType::UNSAFE_CAST},
    {"wait_input", TokenType::WAIT_INPUT},
    {"lateinit", TokenType::LATEINIT},
    {"enum", TokenType::ENUM},
    {"when", TokenType::WHEN},
    {"typealias", TokenType::TYPEALIAS},
    {"operator", TokenType::OPERATOR},
    {"implicit", TokenType::IMPLICIT},
    {"until", TokenType::DOT_DOT_LT},
};

void loadFile(ParserContext *mainContext, LibraryData *library) {
	std::ifstream file(library->path, std::ios::binary | std::ios::ate);
	if (!file.is_open()) {
		throw LexerError(0, "File " + library->path + " doesn't exists\nHint: Verify file path and ensure source file exists");
	}
	library->flags |= LibraryFlags::IS_FILE;
	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);
	library->rawData.resize(size);
	file.read(library->rawData.data(), size);
	if (!file) {
		throw LexerError(0, "An error occurred while reading the file\nHint: Check file permissions and read access");
	}
	file.close();
}

void load(ParserContext *mainContext, LibraryData *library,
          std::vector<Offset> *importOffset) {
	auto &context = library->lexerContext;
	context.library = library;
	context.mainContext = mainContext;
	context.totalSize = library->rawData.size();
	context.absolutePos = 0;
	context.linePos = 0;
	context.lineSize = 0;
	context.nextLinePosition = 0;
	context.importOffset = importOffset;
	ParserContext::mode = library;
	uint32_t i = 0;
	while (nextLine(context, library->rawData.data(), i)) {
		try {
			while (!isEndOfLine(context, i)) {
				loadNextTokenNoCloseBracket(context, i);
			}
		} catch (const LexerError &err) {
			context.mainContext->logError(err.line, err.message);
			while (!isEndOfLine(context, i)) {
				++i;
			}
			context.hasError = true;
		}
	}
	context.tokens.emplace_back(context.linePos + 1, TokenType::END_IMPORT);
}

bool loadNextTokenNoCloseBracket(Context &context, uint32_t &i) {
	char chr = context.line[i];
	if (std::isblank(chr) || (unsigned char)chr < 32 ||
	    (unsigned char)chr >= 127) {
		++i;
		return true;
	}
	if (std::isalpha((unsigned char)chr) || chr == '_') {
		context.pos = i;
		++i;
		pushIdentifier(context, i);
		return true;
	}
	if (std::isdigit(chr)) {
		context.pos = i;
		++i;
		context.tokens.emplace_back(
		    context.linePos, TokenType::NUMBER,
		    pushLexerString(context, loadNumber(context, i)));
		return true;
	}
	switch (chr) {
		case '@': {
			context.tokens.emplace_back(context.linePos, TokenType::AT_SIGN);
			++i;
			return true;
		}
		case '\"': {
			context.pos = i + 1;
			++i;
			if (!isEndOfLine(context, i + 1) && context.line[i] == '\"' &&
			    context.line[i + 1] == '\"') {
				i += 2;
				loadQuote<true, false, true>(context, i);
			} else {
				loadQuote<true, false, false>(context, i);
			}

			return true;
		}
		case '\'': {
			context.pos = i + 1;
			++i;
			loadQuote<true, true, false>(context, i);
			return true;
		}
		case '(':
		case '{':
		case '[': {
			pushAndEnsureBracket(context, i);
			return true;
		}
		case ')': {
			if (context.bracketStack.empty() ||
			    context.bracketStack.back() != '(')
				throw LexerError(context.linePos,
				                 "Unexpected ')', you must use '(' before\nHint: Remove extra ')' or add matching '(' before");
			context.bracketStack.pop_back();
			context.tokens.emplace_back(context.linePos, TokenType::RPAREN);
			// ++i;
			return false;
		}
		case ']': {
			if (context.bracketStack.empty() ||
			    context.bracketStack.back() != '[')
				throw LexerError(context.linePos,
				                 "Unexpected ']', you must use '[' before\nHint: Remove extra ']' or add matching '[' before");
			context.bracketStack.pop_back();
			context.tokens.emplace_back(context.linePos, TokenType::RBRACKET);
			// ++i;
			return false;
		}
		case '}': {
			if (context.bracketStack.empty() ||
			    context.bracketStack.back() != '{')
				throw LexerError(context.linePos,
				                 "Unexpected '}', you must use '{' before\nHint: Remove extra '}' or add matching '{' before");
			context.bracketStack.pop_back();
			context.tokens.emplace_back(context.linePos, TokenType::RBRACE);
			// ++i;
			return false;
		}
		case ',': {
			context.tokens.emplace_back(context.linePos, TokenType::COMMA);
			++i;
			return true;
		}
		case ':': {
			if (!isEndOfLine(context, i + 1) && context.line[i + 1] == ':') {
				context.tokens.emplace_back(context.linePos, TokenType::COLON_COLON);
				i += 2;
				return true;
			}
			context.tokens.emplace_back(context.linePos, TokenType::COLON);
			++i;
			return true;
		}
		case ';': {
			context.tokens.emplace_back(context.linePos, TokenType::SEMI_COLON);
			++i;
			return true;
		}
	}

	if (isOperator(chr)) {
		auto op = loadOp(context, i);
		switch (op) {
			// ESTIMATE_CASE_ADD(EQUAL, setNode)
			// ESTIMATE_CASE_ADD(PLUS_EQUAL, setNode)
			// ESTIMATE_CASE_ADD(MINUS_EQUAL, setNode)
			// ESTIMATE_CASE_ADD(STAR_EQUAL, setNode)
			// ESTIMATE_CASE_ADD(SLASH_EQUAL, setNode)
			case TokenType::COMMENT_SINGLE_LINE: {
				while (!isEndOfLine(context, i)) {
					++i;
				}
				nextLine(context, context.library->rawData.data(), i);
				return true;
			}
			case TokenType::START_COMMENT: {
			start:;
				uint32_t firstLine = context.linePos;
				while (!isEndOfLine(context, i)) {
					if (context.line[i++] != '*')
						continue;
					if (isEndOfLine(context, i))
						break;
					if (context.line[i++] != '/')
						continue;
					goto end;
				}
				if (!nextLine(context, context.library->rawData.data(), i)) {
					throw LexerError(firstLine, std::string("Cannot find */\nHint: Close multi-line comment with '*/'"));
				}
				goto start;
			end:;
				return true;
			}
			default:
				break;
		}
		context.tokens.emplace_back(context.linePos, op);
		return true;
	}
	throw LexerError(
	    context.linePos,
	    std::string("Unknown character: '") + chr +
	        "' - ASCII value: " + std::to_string(static_cast<int>(chr)) +
	        "\nHint: Remove or replace invalid character");
}

void pushAndEnsureBracket(Context &context, uint32_t &i) {
	char chr = context.line[i];
	switch (chr) {
		case '(':
			context.tokens.emplace_back(context.linePos, TokenType::LPAREN);
			break;
		case '{':
			context.tokens.emplace_back(context.linePos, TokenType::LBRACE);
			break;
		case '[': {
			context.tokens.emplace_back(context.linePos, TokenType::LBRACKET);
			break;
		}
	}
	context.bracketStack.push_back(chr);
	++i;
start:;
	uint32_t firstLine = context.linePos;
	while (!isEndOfLine(context, i)) {
		if (loadNextTokenNoCloseBracket(context, i))
			continue;
		++i;
		return;
	}
	if (!nextLine(context, context.library->rawData.data(), i)) {
		if (context.mainContext && context.mainContext->autoCloseBracketsOnEof) {
			if (!context.bracketStack.empty() && context.bracketStack.back() == chr) {
				context.bracketStack.pop_back();
			}
			return;
		}
		throw LexerError(firstLine, std::string("Expected '") +
		                                getCloseBracket(chr) +
		                                "' but not found\nHint: Close bracket with '" +
		                                getCloseBracket(chr) + "'");
	}
	goto start;
}

TokenType loadOp(Context &context, uint32_t &i) {
	char first = context.line[i++];
	switch (first) {
		case '+':
			if (!isEndOfLine(context, i)) {
				if (context.line[i] == '+') { ++i; return TokenType::PLUS_PLUS; }
				if (context.line[i] == '=') { ++i; return TokenType::PLUS_EQUAL; }
			}
			return TokenType::PLUS;
		case '-':
			if (!isEndOfLine(context, i)) {
				if (context.line[i] == '-') { ++i; return TokenType::MINUS_MINUS; }
				if (context.line[i] == '=') { ++i; return TokenType::MINUS_EQUAL; }
				if (context.line[i] == '>') { ++i; return TokenType::MINUS_GT; }
			}
			return TokenType::MINUS;
		case '*':
			if (!isEndOfLine(context, i) && context.line[i] == '=') {
				++i; return TokenType::STAR_EQUAL;
			}
			return TokenType::STAR;
		case '/':
			if (!isEndOfLine(context, i)) {
				if (context.line[i] == '/') { ++i; return TokenType::COMMENT_SINGLE_LINE; }
				if (context.line[i] == '*') { ++i; return TokenType::START_COMMENT; }
				if (context.line[i] == '=') { ++i; return TokenType::SLASH_EQUAL; }
			}
			return TokenType::SLASH;
		case '%':
			if (!isEndOfLine(context, i) && context.line[i] == '=') {
				++i; return TokenType::PERCENT_EQUAL;
			}
			return TokenType::PERCENT;
		case '=':
			if (!isEndOfLine(context, i)) {
				if (context.line[i] == '=') {
					++i;
					if (!isEndOfLine(context, i) && context.line[i] == '=') {
						++i; return TokenType::EQEQEQ;
					}
					return TokenType::EQEQ;
				}
				if (context.line[i] == '>') { ++i; return TokenType::MINUS_GT; }
			}
			return TokenType::EQUAL;
		case '!':
			if (!isEndOfLine(context, i)) {
				if (context.line[i] == 'i' && !isEndOfLine(context, i + 1)) {
					if (context.line[i + 1] == 's' && (isEndOfLine(context, i + 2) || (!std::isalnum((unsigned char)context.line[i + 2]) && context.line[i + 2] != '_'))) {
						i += 2;
						return TokenType::NOT_IS;
					}
					if (context.line[i + 1] == 'n' && (isEndOfLine(context, i + 2) || (!std::isalnum((unsigned char)context.line[i + 2]) && context.line[i + 2] != '_'))) {
						i += 2;
						return TokenType::NOT_IN;
					}
				}
				if (context.line[i] == '=') {
					++i;
					if (!isEndOfLine(context, i) && context.line[i] == '=') {
						++i; return TokenType::NOTEQEQ;
					}
					return TokenType::NOTEQ;
				}
			}
			return TokenType::EXMARK;
		case '<':
			if (!isEndOfLine(context, i) && context.line[i] == '=') {
				++i; return TokenType::LTE;
			}
			return TokenType::LT;
		case '>':
			if (!isEndOfLine(context, i) && context.line[i] == '=') {
				++i; return TokenType::GTE;
			}
			return TokenType::GT;
		case '&':
			if (!isEndOfLine(context, i) && context.line[i] == '&') {
				++i; return TokenType::AND_AND;
			}
			return TokenType::AND;
		case '|':
			if (!isEndOfLine(context, i) && context.line[i] == '|') {
				++i; return TokenType::OR_OR;
			}
			return TokenType::OR;
		case '?':
			if (!isEndOfLine(context, i)) {
				if (context.line[i] == '.') { ++i; return TokenType::QMARK_DOT; }
				if (context.line[i] == '?') { ++i; return TokenType::QMARK_QMARK; }
				if (context.line[i] == ':') { ++i; return TokenType::QMARK_QMARK; }
			}
			return TokenType::QMARK;
		case '.':
			if (!isEndOfLine(context, i) && context.line[i] == '.') {
				++i;
				if (!isEndOfLine(context, i) && context.line[i] == '<') {
					++i; return TokenType::DOT_DOT_LT;
				}
				return TokenType::DOT_DOT;
			}
			return TokenType::DOT;
		case ':':
			if (!isEndOfLine(context, i) && context.line[i] == ':') {
				++i; return TokenType::COLON_COLON;
			}
			return TokenType::COLON;
		default:
			throw LexerError(context.linePos, std::string("Cannot find operator: ") + first + "\nHint: Check operator syntax");
	}
}

void pushIdentifier(Context &context, uint32_t &i) {
	std::string_view identifier = loadIdentifierView(context, i);
	auto it = KEYWORDS.find(identifier);
	if (it == KEYWORDS.end()) {
		context.tokens.emplace_back(
		    context.linePos, TokenType::IDENTIFIER,
		    pushLexerString(context, identifier));
		return;
	}
	switch (it->second) {
		case TokenType::NOT:
		case TokenType::TO: {
			context.tokens.emplace_back(
			    context.linePos, it->second,
			    pushLexerString(context, identifier));
			return;
		}
		ESTIMATE_CASE_ADD(CLASS, classes)
		// ESTIMATE_CASE_ADD(FUNC, functions)
		// ESTIMATE_CASE_ADD(CONSTRUCTOR, constructorNode)
		ESTIMATE_CASE_ADD(VAL, declaration)
		ESTIMATE_CASE_ADD(VAR, declaration)
		// ESTIMATE_CASE_ADD(IF, ifNode)
		// ESTIMATE_CASE_ADD(WHILE, whileNode)
		// ESTIMATE_CASE_ADD(RETURN, returnNode)
		// ESTIMATE_CASE_ADD(TRY, tryCatchNode)
		// ESTIMATE_CASE_ADD(THROW, throwNode)
		case TokenType::UNSAFE_CAST: {
			if (!isEndOfLine(context, i)) {
				if (context.line[i] != '?') {
					break;
				}
				++i;
				context.tokens.emplace_back(context.linePos, SAFE_CAST);
				return;
			}
			break;
		}
		case TokenType::IMPORT: {
			if (context.tokens.empty() ||
			    context.tokens.back().type != TokenType::AT_SIGN) {
				if (!context.mainContext || !context.mainContext->ignoreForeignImports) {
					throw LexerError(context.linePos,
					                 "import must have @ in prefix\nHint: Write '@import' instead of 'import'");
				}
				size_t start = i;
				while (start < context.lineSize && (context.line[start] == ' ' || context.line[start] == '\t')) {
					start++;
				}
				size_t end = start;
				while (end < context.lineSize && context.line[end] != ';' && context.line[end] != '\r' && context.line[end] != '\n') {
					end++;
				}
				std::string restOfLine = std::string(context.line + start, end - start);
				while (!restOfLine.empty() && (restOfLine.back() == ' ' || restOfLine.back() == '\t')) {
					restOfLine.pop_back();
				}
				std::string fullImport = "import" + (restOfLine.empty() ? "" : " " + restOfLine);
				if (context.mainContext) {
					context.mainContext->warning(context.linePos,
					    "Foreign import statement '" + fullImport + "' is ignored. AutoLang uses '@import(\"path\")' to import modules.");
				}
				i = (end < context.lineSize && context.line[end] == ';') ? (uint32_t)(end + 1) : (uint32_t)context.lineSize;
				return;
			}
			// if (!context.mode->allowImportOtherFile) {
			// 	throw LexerError(context.linePos, "@import isn't allowed here");
			// }
			if (!context.bracketStack.empty()) {
				throw LexerError(context.linePos,
				                 "@import is only allowed at file scope\nHint: Move @import statement to the top of the file outside any function or class");
			}
			context.importOffset->push_back(context.tokens.size());
			break;
		}

		// ESTIMATE_CASE_ADD(PLUS, binaryNode)
		// ESTIMATE_CASE_ADD(MINUS, binaryNode)
		// ESTIMATE_CASE_ADD(STAR, binaryNode)
		// ESTIMATE_CASE_ADD(SLASH, binaryNode)
		// ESTIMATE_CASE_ADD(PERCENT, binaryNode)
		// ESTIMATE_CASE_ADD(LT, binaryNode)
		// ESTIMATE_CASE_ADD(GT, binaryNode)
		// ESTIMATE_CASE_ADD(LTE, binaryNode)
		// ESTIMATE_CASE_ADD(GTE, binaryNode)
		// ESTIMATE_CASE_ADD(EQEQ, binaryNode)
		// ESTIMATE_CASE_ADD(NOTEQ, binaryNode)
		// ESTIMATE_CASE_ADD(EQEQEQ, binaryNode)
		// ESTIMATE_CASE_ADD(NOTEQEQ, binaryNode)
		// ESTIMATE_CASE_ADD(AND_AND, binaryNode)
		// ESTIMATE_CASE_ADD(OR_OR, binaryNode)
		// ESTIMATE_CASE_ADD(AND, binaryNode)
		// ESTIMATE_CASE_ADD(OR, binaryNode)
		default:
			break;
	}
	context.tokens.emplace_back(context.linePos, it->second);
}

std::string_view loadIdentifierView(Context &context, uint32_t &i) {
	for (; !isEndOfLine(context, i); ++i) {
		char chr = context.line[i];
		if (std::isblank(static_cast<unsigned char>(chr)))
			break;
		if (std::isalnum(static_cast<unsigned char>(chr)) || chr == '_') {
			continue;
		}
		break;
	}
	return std::string_view(context.line + context.pos, i - context.pos);
}

std::string loadIdentifier(Context &context, uint32_t &i) {
	return std::string(loadIdentifierView(context, i));
}

bool isOperator(char chr) {
	switch (chr) {
		case '?':
		case '&':
		case '|':
		case '.':
		case '+':
		case '-':
		case '*':
		case '/':
		case '%':
		case '!':
		case '=':
		case '<':
		case '>':
		case ':': {
			return true;
		}
		default:
			return false;
	}
}

uint32_t pushLexerString(Context &context, std::string_view str) {
	auto it = context.mainContext->lexerStringMap.find(str);
	if (it == context.mainContext->lexerStringMap.end()) {
		uint32_t id = static_cast<uint32_t>(context.mainContext->lexerString.size());
		std::string_view arenaStr = context.mainContext->stringArena.allocateView(str);
		context.mainContext->lexerStringMap[arenaStr] = id;
		context.mainContext->lexerString.push_back(arenaStr);
		return id;
	}
	return it->second;
}

uint32_t pushLexerString(Context &context, std::string &&str) {
	return pushLexerString(context, std::string_view(str));
}

uint32_t pushLexerString(Context &context, const char *str) {
	return pushLexerString(context, std::string_view(str));
}

bool nextLine(Context &context, const char *lines, uint32_t &i) {
	context.absolutePos = context.nextLinePosition;
	if (context.absolutePos >= context.totalSize)
		return false;
	context.line = lines + context.absolutePos;
	context.lineSize = 0;
	i = 0;
	++context.linePos;
	while (context.absolutePos + context.lineSize < context.totalSize) {
		switch (context.line[context.lineSize]) {
			case '\n':
				context.nextLinePosition =
				    context.absolutePos + context.lineSize + 1;
				goto out;
			case '\r':
				if (context.absolutePos + context.lineSize + 1 <
				        context.totalSize &&
				    context.line[context.lineSize + 1] == '\n') {
					context.nextLinePosition =
					    context.absolutePos + context.lineSize + 2;
				} else {
					context.nextLinePosition =
					    context.absolutePos + context.lineSize + 1;
				}
				goto out;
		}
		++context.lineSize;
	}
	if (context.absolutePos + context.lineSize >= context.totalSize) {
		context.nextLinePosition = context.totalSize;
	}
out:;
	// printDebug(context.nextLinePosition);
	// printDebug(std::to_string(context.linePos)+ ", " +
	// std::to_string(context.absolutePos) + "] " +
	// std::to_string(context.lineSize) + "} " + std::string(context.line,
	// context.lineSize));
	return true;
}

char getCloseBracket(char chr) {
	switch (chr) {
		case '(':
			return ')';
		case '[':
			return ']';
		case '{':
			return '}';
		case '<':
			return '>';
	}
	return '\0';
}

} // namespace Lexer
} // namespace Autolang
#endif
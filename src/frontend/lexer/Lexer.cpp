#ifndef LEXER_CPP
#define LEXER_CPP

#include "frontend/lexer/Lexer.hpp"
#include "frontend/ACompiler.hpp"
#include "frontend/parser/ParserContext.hpp"
#include <fstream>

#define TOKEN_CASE(ch, type)                                                   \
	case ch:                                                                   \
		context.tokens.emplace_back(context.linePos, TokenType::type);         \
		break;

#define ESTIMATE_CASE_ADD(op, val)                                             \
	case TokenType::op:                                                        \
		++context.estimate.val;                                                \
		break;

namespace AutoLang {
namespace Lexer {

void loadFile(ParserContext *mainContext, LibraryData *library) {
	std::ifstream file(library->path, std::ios::binary | std::ios::ate);
	if (!file.is_open()) {
		throw LexerError(0, "File " + library->path + " doesn't exists");
	}
	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);
	library->rawData.resize(size);
	file.read(library->rawData.data(), size);
	if (!file) {
		throw LexerError(0, "An error occurred while reading the file");
	}
	file.close();
}

void load(ParserContext *mainContext, LibraryData *library) {
	auto &context = library->lexerContext;
	context.library = library;
	context.mainContext = mainContext;
	context.totalSize = library->rawData.size();
	context.absolutePos = 0;
	context.linePos = 0;
	context.lineSize = 0;
	context.nextLinePosition = 0;
	uint32_t i = 0;
	while (nextLine(context, library->rawData.data(), i)) {
		try {
			while (!isEndOfLine(context, i)) {
				loadNextTokenNoCloseBracket(context, i);
			}
		} catch (const LexerError &err) {
			context.mainContext->logMessage(err.line, err.message);
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
	if (std::isblank(chr)) {
		++i;
		return true;
	}
	if (std::isalpha((unsigned char)chr)) {
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
		case '\"':
		case '\'': {
			context.pos = i + 1;
			++i;
			loadQuote(context, chr, i);
			return true;
		}
	}
	if (isOperator(chr)) {
		auto op = loadOp(context, i);
		switch (op) {
			ESTIMATE_CASE_ADD(EQUAL, setNode)
			ESTIMATE_CASE_ADD(PLUS_EQUAL, setNode)
			ESTIMATE_CASE_ADD(MINUS_EQUAL, setNode)
			ESTIMATE_CASE_ADD(STAR_EQUAL, setNode)
			ESTIMATE_CASE_ADD(SLASH_EQUAL, setNode)
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
					throw LexerError(firstLine, std::string("Cannot found */"));
				}
				goto start;
			end:;
				return true;
			}
		}
		context.tokens.emplace_back(context.linePos, op);
		return true;
	}
	switch (chr) {
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
				                 "Unexpected ')', you must use '(' before");
			context.bracketStack.pop_back();
			context.tokens.emplace_back(context.linePos, TokenType::RPAREN);
			// ++i;
			return false;
		}
		case ']': {
			if (context.bracketStack.empty() ||
			    context.bracketStack.back() != '[')
				throw LexerError(context.linePos,
				                 "Unexpected ']', you must use '[' before");
			context.bracketStack.pop_back();
			context.tokens.emplace_back(context.linePos, TokenType::RBRACKET);
			// ++i;
			return false;
		}
		case '}': {
			if (context.bracketStack.empty() ||
			    context.bracketStack.back() != '{')
				throw LexerError(context.linePos,
				                 "Unexpected '{', you must use '}' before");
			context.bracketStack.pop_back();
			context.tokens.emplace_back(context.linePos, TokenType::RBRACE);
			// ++i;
			return false;
		}
			TOKEN_CASE(',', COMMA)
			TOKEN_CASE(':', COLON)
		default:
			throw LexerError(context.linePos,
			                 std::string("Unknow character: '") + chr + "'");
	}
	++i;
	return true;
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
		throw LexerError(firstLine, std::string("Expected '") +
		                                getCloseBracket(chr) +
		                                "' but not found");
	}
	goto start;
}

TokenType loadOp(Context &context, uint32_t &i) {
	char first = context.line[i++];
	if (isEndOfLine(context, i) || !isOperator(context.line[i])) {
	loadFirst:;
		std::string str = {first};
		auto it = CAST.find(str);
		if (it == CAST.end())
			throw LexerError(context.linePos, "Cannot find operator: " + str);
		return it->second;
	}
	char second = context.line[i++];
	if (isEndOfLine(context, i) || second == '.' || second == '/' ||
	    !isOperator(context.line[i])) {
	loadSecond:;
		std::string str = {first, second};
		auto it = CAST.find(str);
		if (it == CAST.end()) {
			--i;
			goto loadFirst;
		}
		return it->second;
	}
	char third = context.line[i++];
	std::string str = {first, second, third};
	auto it = CAST.find(str);
	if (it == CAST.end()) {
		--i;
		goto loadSecond;
	}
	return it->second;
}

void pushIdentifier(Context &context, uint32_t &i) {
	std::string identifier = loadIdentifier(context, i);
	auto it = CAST.find(identifier);
	if (it == CAST.end()) {
		context.tokens.emplace_back(
		    context.linePos, TokenType::IDENTIFIER,
		    pushLexerString(context, std::move(identifier)));
		return;
	}
	switch (it->second) {
		ESTIMATE_CASE_ADD(CLASS, classes)
		ESTIMATE_CASE_ADD(FUNC, functions)
		ESTIMATE_CASE_ADD(CONSTRUCTOR, constructorNode)
		ESTIMATE_CASE_ADD(VAL, declaration)
		ESTIMATE_CASE_ADD(VAR, declaration)
		ESTIMATE_CASE_ADD(IF, ifNode)
		ESTIMATE_CASE_ADD(WHILE, whileNode)
		ESTIMATE_CASE_ADD(RETURN, returnNode)
		ESTIMATE_CASE_ADD(TRY, tryCatchNode)
		ESTIMATE_CASE_ADD(THROW, throwNode)
		case TokenType::IMPORT: {
			if (context.tokens.empty() ||
			    context.tokens.back().type != TokenType::AT_SIGN) {
				throw LexerError(context.linePos,
				                 "import must have @ in prefix");
			}
			// if (!context.mode->allowImportOtherFile) {
			// 	throw LexerError(context.linePos, "@import isn't allowed here");
			// }
			if (!context.bracketStack.empty()) {
				throw LexerError(context.linePos,
				                 "@import is only allowed at file scope");
			}
			context.mainContext->importOffset.insert(context.tokens.size());
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

std::string loadIdentifier(Context &context, uint32_t &i) {
	for (; !isEndOfLine(context, i); ++i) {
		char chr = context.line[i];
		if (std::isblank(chr))
			break;
		if (std::isalnum(chr) || chr == '_' || chr == '$') {
			continue;
		}
		break;
	}
	return std::string(context.line + context.pos, i - context.pos);
}

std::string loadNumber(Context &context, uint32_t &i) {
	bool hasDot = false;
	bool hasUnderscore = false;
	bool scientific = false;
	char chr;
	for (; !isEndOfLine(context, i); ++i) {
		chr = context.line[i];
		switch (chr) {
			case 'e':
			case 'E': {
				if (scientific)
					throw LexerError(context.linePos,
					                 "Unknow value: " +
					                     std::string(context.line + context.pos,
					                                 i - context.pos));
				scientific = true;
				if (isEndOfLine(context, ++i)) {
					--i;
					goto ended;
				}
				chr = context.line[i];
				if (std::isdigit(chr)) {
					continue;
				}
				if (chr == '+' || chr == '-') {
					if (isEndOfLine(context, ++i)) {
						--i;
						goto ended;
					}
					chr = context.line[i];
					if (!std::isdigit(chr)) {
						std::string num = std::string(
						    context.line + context.pos, i - context.pos);
						throw LexerError(context.linePos,
						                 std::string("Expected digit after ") +
						                     num + " but '" + chr +
						                     "' was found");
					}
					continue;
				}
				std::string num =
				    std::string(context.line + context.pos, i - context.pos);
				throw LexerError(
				    context.linePos,
				    std::string("Expected digit after exponent '") + num +
				        "' but '" + chr + "' was found, did you mean " + num +
				        "0 ?");
			}
			case '_': {
				hasUnderscore = true;
				if (isEndOfLine(context, ++i)) {
					--i;
					goto ended;
				}
				chr = context.line[i];
				if (!std::isdigit(chr)) {
					std::string num = std::string(context.line + context.pos,
					                              i - context.pos - 1);
					throw LexerError(
					    context.linePos,
					    std::string("Expected digit after ") + num + "_ but '" +
					        chr + "' was found, did you mean " + num + " ?");
				}
				continue;
			}
			case '.': {
				if (scientific) {
					std::string num = std::string(context.line + context.pos,
					                              i - context.pos);
					throw LexerError(context.linePos,
					                 "Expected integer exponent after " + num +
					                     ", but '.' was found");
				}
				if (hasDot) {
					std::string num = std::string(context.line + context.pos,
					                              i - context.pos);
					throw LexerError(context.linePos,
					                 "Expected digit after " + num +
					                     " but '.' was found, did you mean " +
					                     num + "0 ?");
				}
				if (isEndOfLine(context, ++i) ||
				    !std::isdigit(context.line[i])) {
					--i;
					goto ended;
				}
				hasDot = true;
				continue;
			}
			default:
				break;
		}
		if (std::isdigit(chr)) {
			continue;
		}
		if (std::isalpha(chr)) {
			throw LexerError(
			    context.linePos,
			    std::string("Unexpected character '") + chr +
			        "' after numeric literal: " +
			        std::string(context.line + context.pos, i - context.pos) +
			        chr);
		}
		break;
	}
ended:;
	if (hasUnderscore) {
		std::string newStr;
		size_t size = i - context.pos;
		newStr.reserve(size);
		auto pos = context.line + context.pos;
		for (int j = 0; j < size; ++j) {
			char chr = pos[j];
			if (chr == '_')
				continue;
			newStr += chr;
		}
		return newStr;
	}
	return std::string(context.line + context.pos, i - context.pos);
}

void loadQuote(Context &context, char quote, uint32_t &i) {
	bool isSpecialCase = false;
	std::string newStr;
	char chr;
	for (; !isEndOfLine(context, i); ++i) {
		chr = context.line[i];
		if (!isSpecialCase) {
			switch (chr) {
				case '\\': {
					isSpecialCase = true;
					continue;
				}
				//"Hello ${value + value}"
				case '$': {
					if (isEndOfLine(context, ++i))
						throw LexerError(context.linePos,
						                 std::string("Expected ") + quote +
						                     " but not found");
					if (!context.line[i] == '{') {
						newStr += '$';
						break;
					}
					context.tokens.emplace_back(context.linePos,
					                            TokenType::LPAREN);
					context.tokens.emplace_back(
					    context.linePos, TokenType::STRING,
					    pushLexerString(context, std::move(newStr)));
					context.tokens.emplace_back(context.linePos,
					                            TokenType::PLUS);
					// '{' => '(' to support "Hello ${a}"  => ("Hello" + (a))
					// instead of ("Hello" + {a})
					uint32_t bracketReplacePos = context.tokens.size();
					pushAndEnsureBracket(context, i);
					context.tokens[bracketReplacePos].type = TokenType::LPAREN;
					context.tokens.back().type = TokenType::RPAREN;
					if (isEndOfLine(context, i))
						throw LexerError(context.linePos,
						                 std::string("Expected ") + quote +
						                     " but not found");
					if (context.line[i] == quote) {
						++i;
						context.tokens.emplace_back(context.linePos,
						                            TokenType::RPAREN);
						return;
					}
					context.tokens.emplace_back(context.linePos,
					                            TokenType::PLUS);
					loadQuote(context, quote, i);
					context.tokens.emplace_back(context.linePos,
					                            TokenType::RPAREN);
					return;
				}
			}
			if (chr == quote) {
				++i;
				context.tokens.emplace_back(
				    context.linePos, TokenType::STRING,
				    pushLexerString(context, std::move(newStr)));
				return;
			}
			newStr += chr;
			continue;
		}
		switch (chr) {
			case 'n':
				newStr += '\n';
				break;
			case 't':
				newStr += '\t';
				break;
			case '\\':
				newStr += '\\';
				break;
			case '\'':
				newStr += '\'';
				break;
			case '\"':
				newStr += '\"';
				break;
			case 'r':
				newStr += '\r';
				break;
			case '0':
				newStr += '\0';
				break;
			default:
				throw LexerError(context.linePos,
				                 std::string("Unknown escape sequence '\\") +
				                     chr + '\'');
		}
		isSpecialCase = false;
	}
	throw LexerError(context.linePos,
	                 std::string("Expected ") + quote + " but not found");
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
		case '>': {
			return true;
		}
		default:
			return false;
	}
}

std::string Token::toString(ParserContext &context) {
	switch (type) {
		case TokenType::VAR:
			return "var";
		case TokenType::VAL:
			return "val";
		case TokenType::FUNC:
			return "func";
		case TokenType::IF:
			return "if";
		case TokenType::FOR:
			return "for";
		case TokenType::WHILE:
			return "while";
		case TokenType::CONTINUE:
			return "continue";
		case TokenType::IN:
			return "in";
		case TokenType::QMARK:
			return "?";
		case TokenType::QMARK_DOT:
			return "?.";
		case TokenType::EXMARK:
			return "!";
		case TokenType::AT_SIGN:
			return "@";
		case TokenType::RETURN:
			return "return";
		case TokenType::AND_AND:
			return "and";
		case TokenType::OR_OR:
			return "or";
		case TokenType::NOT:
			return "not";
		case TokenType::DOT:
			return ".";
		case TokenType::DOT_DOT:
			return "..";
		case TokenType::COMMA:
			return ",";
		case TokenType::COLON:
			return ":";
		case TokenType::EQUAL:
			return "=";
		case TokenType::LPAREN:
			return "(";
		case TokenType::RPAREN:
			return ")";
		case TokenType::LBRACKET:
			return "[";
		case TokenType::RBRACKET:
			return "]";
		case TokenType::LBRACE:
			return "{";
		case TokenType::RBRACE:
			return "}";
		case TokenType::PLUS:
			return "+";
		case TokenType::PLUS_PLUS:
			return "++";
		case TokenType::PLUS_EQUAL:
			return "+=";
		case TokenType::MINUS:
			return "-";
		case TokenType::MINUS_MINUS:
			return "--";
		case TokenType::MINUS_EQUAL:
			return "-=";
		case TokenType::STAR:
			return "*";
		case TokenType::STAR_EQUAL:
			return "*=";
		case TokenType::SLASH:
			return "/";
		case TokenType::SLASH_EQUAL:
			return "/=";
		case TokenType::PERCENT:
			return "%";
		case TokenType::PERCENT_EQUAL:
			return "%=";
		case TokenType::STRING:
			return "\"" + context.lexerString[indexData] + "\"";
		case TokenType::EQEQ:
			return "==";
		case TokenType::NOTEQ:
			return "!=";
		case TokenType::EQEQEQ:
			return "===";
		case TokenType::NOTEQEQ:
			return "!==";
		case TokenType::LT:
			return "<";
		case TokenType::GT:
			return ">";
		case TokenType::LTE:
			return "<=";
		case TokenType::GTE:
			return ">=";
		case TokenType::IDENTIFIER:
			return context.lexerString[indexData];
		default:
			for (auto &pair : CAST) {
				if (pair.second == type)
					return pair.first;
			}
			return context.lexerString[indexData];
	}
}

uint32_t pushLexerString(Context &context, std::string &&str) {
	auto it = context.mainContext->lexerStringMap.find(str);
	if (it == context.mainContext->lexerStringMap.end()) {
		uint32_t id = context.mainContext->lexerString.size();
		context.mainContext->lexerStringMap[str] = id;
		context.mainContext->lexerString.push_back(std::move(str));
		return id;
	}
	return it->second;
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

bool isEndOfLine(Context &context, uint32_t &i) {
	return i >= context.lineSize;
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
} // namespace AutoLang
#endif
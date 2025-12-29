#ifndef LEXER_CPP
#define LEXER_CPP

#include <fstream>
#include "Lexer.hpp"
#include "ParserContext.hpp"

#define TOKEN_CASE(ch, type) \
    case ch: tokens.emplace_back(context.linePos, TokenType::type); break;

namespace AutoLang {
namespace Lexer {

std::vector<Token> load(ParserContext* mainContext, const char* path, Context& context) {
	std::vector<char> lines;
	std::ifstream file(path, std::ios::binary | std::ios::ate);
	if (!file.is_open()) {
		throw std::runtime_error(std::string("File ") + path + " doesn't exists");
	}
	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);
	lines.resize(size);
	file.read(lines.data(), size);
	if (!file) {
		throw std::runtime_error(std::string(path) + ": An error occurred while reading the file");
	}
	file.close();
	auto pair = std::pair<const char*, size_t>(reinterpret_cast<const char*>(lines.data()), lines.size());
	return load(mainContext, pair, context);
}

std::vector<Token> load(ParserContext* mainContext, std::pair<const char*, size_t>& lineData, Context& context) {
	std::vector<Token> tokens;
	tokens.reserve(256);
	context.mainContext = mainContext;
	context.totalSize = lineData.second;
	context.absolutePos = 0;
	context.linePos = 0;
	context.lineSize = 0;
	context.nextLinePosition = 0;
	uint32_t i = 0;
	while (nextLine(context, lineData.first, i)) {
		while (!isEndOfLine(context, i)) {
			char chr = context.line[i];
			if (std::isblank(chr)) {
				++i;
				continue;
			}
			if (std::isalpha(chr)) {
				context.pos = i;
				++i;
				pushIdentifier(context, tokens, i);
				continue;
			}
			if (std::isdigit(chr)) {
				context.pos = i;
				++i;
				tokens.emplace_back(
					context.linePos,
					TokenType::NUMBER,
					pushLexerString(context, loadNumber(context, i))
				);
				continue;
			}
			if (chr == '\"' || chr =='\'') {
				context.pos = i + 1;
				++i;
				tokens.emplace_back(
					context.linePos,
					TokenType::STRING,
					pushLexerString(context, loadQuote(context, chr, i))
				);
				continue;
			}
			if (isOperator(chr)) {
				auto op = loadOp(context, i);
				if (op == TokenType::COMMENT_SINGLE_LINE)
					break;
				tokens.emplace_back(context.linePos, op);
				continue;
			}
			switch (chr) {
				TOKEN_CASE('(', LPAREN)
				TOKEN_CASE(')', RPAREN)
				TOKEN_CASE('[', LBRACKET)
				TOKEN_CASE(']', RBRACKET)
				TOKEN_CASE('{', LBRACE)
				TOKEN_CASE('}', RBRACE)
				TOKEN_CASE(',', COMMA)
				TOKEN_CASE(':', COLON)
				default:
					throw std::runtime_error(std::to_string(i) + std::string("Cannot find expression '")+chr+"'"+std::to_string((int)chr));
			}
			++i;
		}
	}
	return tokens;
}

TokenType loadOp(Context& context, uint32_t& i) {
	char first = context.line[i++];
	if (isEndOfLine(context, i) || !isOperator(context.line[i])) {
		std::string str = {first};
		auto it = CAST.find(str);
		if (it == CAST.end())
			throw std::runtime_error("Cannot find " + str + " operator ");
		return it->second;
	}
	char second = context.line[i++];
	if (isEndOfLine(context, i) || second == '.' || second == '/' || !isOperator(context.line[i])) {
		std::string str = {first, second};
		auto it = CAST.find(str);
		if (it == CAST.end()) 
			throw std::runtime_error("Cannot find " + str + " operator ");
		return it->second;
	}
	char third= context.line[i];
	++i;
	std::string str = {first, second, third};
	auto it = CAST.find(str);
	if (it == CAST.end()) 
		throw std::runtime_error("Cannot find " + str + " operator ");
	return it->second;
}

#define ESTIMATE_CASE_ADD(op, val) case TokenType::op: ++context.estimate.val; break;

void pushIdentifier(Context& context, std::vector<Token>& tokens, uint32_t& i) {
	std::string identifier = loadIdentifier(context, i);
	auto it = CAST.find(identifier);
	if (it == CAST.end()) {
		tokens.emplace_back(
			context.linePos,
			TokenType::IDENTIFIER,
			pushLexerString(context, std::move(identifier))
		);
		return;
	}
	switch (it->second) {
		ESTIMATE_CASE_ADD(CLASS, classes)
		ESTIMATE_CASE_ADD(FUNC, functions)
		ESTIMATE_CASE_ADD(CONSTRUCTOR, functions)
		ESTIMATE_CASE_ADD(VAL, declaration)
		ESTIMATE_CASE_ADD(VAR, declaration)
		ESTIMATE_CASE_ADD(IF, ifNode)
		ESTIMATE_CASE_ADD(WHILE, whileNode)
		ESTIMATE_CASE_ADD(FOR, forNode)
		default: break;
	}
	tokens.emplace_back(
		context.linePos,
		it->second
	);
}

std::string loadIdentifier(Context& context, uint32_t& i) {
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

std::string loadNumber(Context& context, uint32_t& i) {
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
					throw std::runtime_error(std::string("Expected number but ")+chr+" was found ");
				scientific = true;
				if (isEndOfLine(context, ++i)) {
					--i;
					goto ended;
				}
				chr = context.line[i];
				if (std::isdigit(chr) || chr == '+' || chr == '-') {
					continue;
				}
				throw std::runtime_error(std::string("Expected number after e but ")+chr+" was found ");
			}
			case '_': {
				hasUnderscore = true;
				if (isEndOfLine(context, ++i)) {
					--i;
					goto ended;
				}
				chr = context.line[i];
				if (!std::isdigit(chr)) {
					throw std::runtime_error(std::string("Expected number after _ but ")+chr+" was found ");
				}
				continue;
			}
			case '.': {
				if (hasDot) {
					throw std::runtime_error(std::string("Expected number but . was found "));
				}
				if (isEndOfLine(context, ++i) || !std::isdigit(context.line[i])) {
					--i;
					goto ended;
				}
				hasDot = true;
				continue;
			}
			default: break;
		}
		if (std::isdigit(chr)) {
			continue;
		}
		if (std::isalpha(chr)) {
			throw std::runtime_error("Unexpected character after number here " + std::string(context.line + context.pos, i - context.pos) + chr);
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
			if (chr == '_') continue;
			newStr += chr;
		}
		return newStr;
	}
	return std::string(context.line + context.pos, i - context.pos);
}

std::string loadQuote(Context& context, char quote, uint32_t& i) {
	bool isSpecialCase = false;
	std::string newStr;
	char chr;
	for (; !isEndOfLine(context, i); ++i) {
		chr = context.line[i];
		if (!isSpecialCase) {
			if (chr == '\\'){
				isSpecialCase = true;
				continue;
			}
			if (chr == quote) {
				++i;
				return newStr;
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
				throw std::runtime_error(std::string("Unknown escape sequence '\\") + chr + '\'');
		}
		isSpecialCase = false;
	}
	throw std::runtime_error(std::string("Expected ") + quote + " but not found");
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

std::string Token::toString(ParserContext& context) {
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
			for (auto& pair : CAST) {
				if (pair.second == type)
					return pair.first;
			}
			return context.lexerString[indexData];
	}
}

uint32_t pushLexerString(Context& context, std::string&& str) {
	auto it = context.mainContext->lexerStringMap.find(str);
	if (it == context.mainContext->lexerStringMap.end()) {
		uint32_t id = context.mainContext->lexerString.size();
		context.mainContext->lexerStringMap[str] = id;
		context.mainContext->lexerString.push_back(std::move(str));
		return id;
	}
	return it->second;
}

bool nextLine(Context& context, const char* lines, uint32_t& i) {
	context.absolutePos = context.nextLinePosition;
	if (context.absolutePos >= context.totalSize)
		return false;
	context.line = lines + context.absolutePos;
	context.lineSize = 0;
	i = 0;
	++context.linePos;
	while (context.absolutePos + context.lineSize < context.totalSize) {
		switch (context.line[context.lineSize]){
			case '\n':
				context.nextLinePosition = context.absolutePos + context.lineSize + 1;
				goto out;
			case '\r':
				context.nextLinePosition = context.absolutePos + context.lineSize + 2;
				goto out;
		}
		++context.lineSize;
	}
	if (context.absolutePos + context.lineSize >= context.totalSize) {
		context.nextLinePosition = context.totalSize;
	}
	out:;
	// printDebug(context.nextLinePosition);
	// printDebug(std::to_string(context.linePos)+ ", " + std::to_string(context.absolutePos) + "] " + std::to_string(context.lineSize) + "} " + std::string(context.line, context.lineSize));
	return true;
}

bool isEndOfLine(Context& context, uint32_t& i) {
	return i >= context.lineSize;
}

}
}
#endif
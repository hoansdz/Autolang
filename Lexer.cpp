#ifndef LEXER_CPP
#define LEXER_CPP

//#include "Lexer.hpp"
#include "ParserContext.hpp"

#define TOKEN_CASE(ch, type) \
    case ch: tokens.emplace_back(context.linePos, TokenType::type); break;

namespace AutoLang {
namespace Lexer {

std::vector<Token> load(ParserContext* mainContext, std::string path) {
	std::vector<Token> tokens;
	Context context;
	context.mainContext = mainContext;
	context.reader.open(path);
	if (!context.reader) {
		throw std::runtime_error(path + " doesn't exists");
	}
	context.linePos = 0;
	while (nextLine(context)) {
		int i=0;
		while (i<context.line.length()) {
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
					throw std::runtime_error(std::string("Cannot find expression '")+chr+"'");
			}
			++i;
		}
	}
	return tokens;
}

TokenType loadOp(Context& context, int &i) {
	char first = context.line[i++];
	if (i == context.line.length() || !isOperator(context.line[i])) {
		std::string str = {first};
		auto it = CAST.find(str);
		if (it == CAST.end()) 
			throw std::runtime_error("Cannot find " + str + " operator ");
		return it->second;
	}
	char second = context.line[i++];
	if (i == context.line.length() || second == '.' || second == '/' || !isOperator(context.line[i])) {
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

void pushIdentifier(Context& context, std::vector<Token>& tokens, int &i) {
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
	tokens.emplace_back(
		context.linePos,
		it->second
	);
}

std::string loadIdentifier(Context& context, int &i) {
	for (; i<context.line.length(); ++i) {
		char chr = context.line[i];
		if (std::isblank(chr))
			break;
		if (std::isalnum(chr)) {
			continue;
		}
		break;
	}
	return context.line.substr(context.pos, i - context.pos);
}

std::string loadNumber(Context& context, int &i) {
	bool hasDot = false;
	bool scientific = false;
	char chr;
	for (; i<context.line.length(); ++i) {
		chr = context.line[i];
		if (std::isdigit(chr)) {
			continue;
		}
		if (std::isalpha(chr)) {
			if (chr == 'e' || chr == 'E') {
				if (scientific)
					throw std::runtime_error(std::string("Expected number but e was found "));
				scientific = true;
				chr = context.line[++i];
				if (std::isdigit(chr)) {
					--i;
					continue;
				}
				if (chr == '+' || chr == '-') continue;
			}
			throw std::runtime_error(std::string("Unexpected character after number here ")+context.line.substr(context.pos, i - context.pos));
		}
		if (chr == '.') {
			if (hasDot) break;
			++i;
			if (i==context.line.length() || !std::isdigit(context.line[i])) {
				--i;
				break;
			}
			hasDot = true;
			continue;
		}
		break;
	}
	return context.line.substr(context.pos, i - context.pos);
}

std::string loadQuote(Context& context, char quote, int &i) {
	bool isSpecialCase = false;
	std::string newStr;
	char chr;
	for (; i<context.line.length(); ++i) {
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

bool nextLine(Context& context) {
	if (std::getline(context.reader, context.line)) {
		++context.linePos;
		return true;
	}
	return false;
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

}
}
#endif
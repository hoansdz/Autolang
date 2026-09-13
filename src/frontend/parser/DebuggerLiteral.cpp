#ifndef DEBUGGER_LITERAL_CPP
#define DEBUGGER_LITERAL_CPP

#include "frontend/ACompiler.hpp"
#include "frontend/parser/Debugger.hpp"
#include "frontend/parser/ParserContext.hpp"

namespace Autolang {

template <bool trailingComma>
std::vector<HasClassIdNode *>
loadListArgument(in_func, size_t &i,
                 std::vector<LexerStringId> *argumentNames) {
	Lexer::Token *token = &context.tokens[i];
	char openBracket = getOpenBracket(token->type);
	if (openBracket == '\0')
		throw ParserError(
		    token->line,
		    "Unexpected token " + token->toString(context) +
		        "\nHint: Ensure arguments list opens with a valid bracket '('");
	if (!nextToken(&token, context.tokens, i)) {
		--i;
		throw ParserError(0, "Bug: Lexer did not ensure a closing bracket");
	}
	std::vector<HasClassIdNode *> nodes;
	bool hasSeenNamed = false;
	auto parseOneArgument = [&](Lexer::Token *&tok) -> HasClassIdNode * {
		LexerStringId argName = 0;
		if (argumentNames && tok->type == Lexer::TokenType::IDENTIFIER &&
		    i + 1 < context.tokens.size() &&
		    context.tokens[i + 1].type == Lexer::TokenType::EQUAL &&
		    context.tokens[i + 1].line == tok->line) {
			argName = tok->indexData;
			i += 2;
			if (i >= context.tokens.size()) {
				throw ParserError(tok->line,
				                  "Expected expression after '=' in named argument\n"
				                  "Hint: Provide a valid value expression.");
			}
			tok = &context.tokens[i];
			hasSeenNamed = true;
		} else if (hasSeenNamed) {
			throw ParserError(tok->line,
			                  "Positional arguments cannot follow named arguments\n"
			                  "Hint: Use named arguments for all arguments following the first named argument.");
		}
		if (argumentNames) {
			if (argName != 0) {
				for (auto prevName : *argumentNames) {
					if (prevName != 0 && prevName == argName) {
						throw ParserError(tok->line,
						                  "Duplicate argument '" +
						                      context.lexerString[argName] +
						                      "' in function call\n"
						                      "Hint: Remove or rename the duplicate argument.");
					}
				}
			}
			argumentNames->push_back(argName);
		}
		return loadExpression(in_data, 0, i);
	};

	switch (token->type) {
		case Lexer::TokenType::RPAREN:
		case Lexer::TokenType::RBRACKET:
		case Lexer::TokenType::RBRACE: {
			if (!isCloseBracket(openBracket, token->type)) {
				for (auto *node : nodes)
					ExprNode::deleteNode(node);
				throw ParserError(
				    token->line, "Bug: Lexer did not ensure a closing bracket");
			}
			return nodes;
		}
		default: {
			nodes.push_back(parseOneArgument(token));
			break;
		}
	}
	while (nextToken(&token, context.tokens, i)) {
		switch (token->type) {
			using namespace Lexer;
			case Lexer::TokenType::LPAREN:
			case Lexer::TokenType::LBRACKET:
			case Lexer::TokenType::LBRACE: {
				nodes.push_back(parseOneArgument(token));
				break;
			}
			case Lexer::TokenType::RPAREN:
			case Lexer::TokenType::RBRACKET:
			case Lexer::TokenType::RBRACE: {
				if (!isCloseBracket(openBracket, token->type)) {
					// for (auto *node : nodes)
					// 	ExprNode::deleteNode(node);
					throw ParserError(
					    token->line,
					    "Bug: Lexer did not ensure a closing bracket");
				}
				return nodes;
			}
			case TokenType::COMMA: {
				if (!nextToken(&token, context.tokens, i))
					goto expectedCloseBracket;
				if constexpr (trailingComma) {
					switch (token->type) {
						case Lexer::TokenType::RPAREN:
						case Lexer::TokenType::RBRACKET:
						case Lexer::TokenType::RBRACE: {
							if (!isCloseBracket(openBracket, token->type)) {
								for (auto *node : nodes)
									ExprNode::deleteNode(node);
								throw ParserError(token->line,
								                  "Bug: Lexer did not ensure a "
								                  "closing bracket");
							}
							return nodes;
						}
						default:
							break;
					}
				}
				nodes.push_back(parseOneArgument(token));
				break;
			}
			default: {
				--i;
				throw ParserError(token->line,
				                  "Unknown token " + token->toString(context) +
				                      "\nHint: Provide a valid argument "
				                      "expression or closing "
				                      "bracket");
			}
		}
	}
expectedCloseBracket:
	for (auto *node : nodes)
		ExprNode::deleteNode(node);
	throw ParserError(token->line,
	                  "Bug: Lexer did not ensure a closing bracket");
}
template std::vector<HasClassIdNode *>
loadListArgument<false>(in_func, size_t &i,
                        std::vector<LexerStringId> *argumentNames);
template std::vector<HasClassIdNode *>
loadListArgument<true>(in_func, size_t &i,
                       std::vector<LexerStringId> *argumentNames);

} // namespace Autolang

#endif

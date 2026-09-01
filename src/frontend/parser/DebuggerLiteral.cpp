#ifndef DEBUGGER_LITERAL_CPP
#define DEBUGGER_LITERAL_CPP

#include "frontend/ACompiler.hpp"
#include "frontend/parser/Debugger.hpp"
#include "frontend/parser/ParserContext.hpp"

namespace Autolang {

template <bool trailingComma>
std::vector<HasClassIdNode *> loadListArgument(in_func, size_t &i) {
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
			nodes.push_back(loadExpression(in_data, 0, i));
			break;
		}
	}
	while (nextToken(&token, context.tokens, i)) {
		switch (token->type) {
			using namespace Lexer;
			case Lexer::TokenType::LPAREN:
			case Lexer::TokenType::LBRACKET:
			case Lexer::TokenType::LBRACE: {
				nodes.push_back(loadExpression(in_data, 0, i));
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
				nodes.push_back(loadExpression(in_data, 0, i));
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

HasClassIdNode *inferenceNodeFromLBrace(in_func, size_t &i,
                                        NodeType canBeNodeType) {
	Lexer::Token *token;
	if (!nextToken(&token, context.tokens, i)) {
		--i;
		throw ParserError(0, "Bug: Lexer did not ensure a closing bracket");
	}
	switch (token->type) {
		case Lexer::TokenType::RPAREN:
		case Lexer::TokenType::RBRACKET: {
			throw ParserError(token->line,
			                  "Bug: Lexer did not ensure a closing bracket");
		}
		case Lexer::TokenType::RBRACE: {
			switch (canBeNodeType) {
				case NodeType::CREATE_SET: {
					return context.createSetPool.push(
					    token->line, nullptr, std::vector<HasClassIdNode *>());
				}
				case NodeType::CREATE_MAP: {
					return context.createMapPool.push(
					    token->line, nullptr,
					    std::vector<
					        std::pair<HasClassIdNode *, HasClassIdNode *>>());
				}
				default: {
					return context.createSetPool.push(
					    token->line, nullptr, std::vector<HasClassIdNode *>());
				}
			}
			break;
		}
		case Lexer::TokenType::OR_OR: {
			switch (canBeNodeType) {
				case NodeType::CREATE_SET: {
					throw ParserError(
					    token->line,
					    "Expected Set<> but closure found\nHint: Do not pass "
					    "closure parameters inside Set literal");
				}
				case NodeType::CREATE_MAP: {
					throw ParserError(
					    token->line,
					    "Expected Map<> but closure found\nHint: Do not pass "
					    "closure parameters inside Map literal");
				}
				default:
					break;
			}
			--i;
			return loadClosure<false>(in_data, i);
		}
		case Lexer::TokenType::OR: {
			switch (canBeNodeType) {
				case NodeType::CREATE_SET: {
					throw ParserError(
					    token->line,
					    "Expected Set<> but closure found\nHint: Do not pass "
					    "closure parameters inside Set literal");
				}
				case NodeType::CREATE_MAP: {
					throw ParserError(
					    token->line,
					    "Expected Map<> but closure found\nHint: Do not pass "
					    "closure parameters inside Map literal");
				}
			}
			--i;
			return loadClosure(in_data, i);
		}
		default: {
			auto firstExpression = loadExpression(in_data, 0, i);
			if (!nextToken(&token, context.tokens, i)) {
				--i;
				throw ParserError(
				    0, "Bug: Lexer did not ensure a closing bracket");
			}
			switch (token->type) {
				case Lexer::TokenType::COMMA: {
					if (canBeNodeType == NodeType::CREATE_MAP) {
						throw ParserError(
						    token->line,
						    "Expected Map<> but Set<> found\nHint: Map "
						    "elements must use 'key: value' pairs");
					}
					return loadSet(in_data, i, firstExpression);
				}
				case Lexer::TokenType::COLON: {
					if (canBeNodeType == NodeType::CREATE_SET) {
						throw ParserError(
						    token->line,
						    "Expected Set<> but Map<> found\nHint: Set "
						    "elements must be single values without ':'");
					}
					return loadMap(in_data, i, firstExpression);
				}
				case Lexer::TokenType::RPAREN:
				case Lexer::TokenType::RBRACKET: {
					throw ParserError(
					    token->line,
					    "Bug: Lexer did not ensure a closing bracket");
				}
				case Lexer::TokenType::RBRACE: {
					return context.createSetPool.push(
					    token->line, nullptr,
					    std::vector<HasClassIdNode *>{firstExpression});
				}
			}
			throw ParserError(
			    token->line,
			    "Unexpected token " + token->toString(context) +
			        "\nHint: Expected ',' or ':' inside literal structure");
		}
	}
}

HasClassIdNode *loadSet(in_func, size_t &i, HasClassIdNode *firstExpression) {
	Lexer::Token *token;
	std::vector<HasClassIdNode *> values = {firstExpression};
	--i;
	while (nextToken(&token, context.tokens, i)) {
		switch (token->type) {
			using namespace Lexer;
			case Lexer::TokenType::RPAREN:
			case Lexer::TokenType::RBRACKET: {
				throw ParserError(
				    token->line, "Bug: Lexer did not ensure a closing bracket");
			}
			case Lexer::TokenType::RBRACE: {
				return context.createSetPool.push(token->line, nullptr,
				                                  std::move(values));
			}
			case TokenType::COMMA: {
				if (!nextToken(&token, context.tokens, i)) {
					--i;
					throw ParserError(
					    context.tokens[i].line,
					    "Bug: Lexer did not ensure a closing bracket");
				}
				if (expect(token, Lexer::TokenType::RBRACE)) {
					return context.createSetPool.push(token->line, nullptr,
					                                  std::move(values));
				}
				values.push_back(loadExpression(in_data, 0, i));
				break;
			}
			default: {
				--i;
				throw ParserError(
				    token->line,
				    "Unknown token " + token->toString(context) +
				        "\nHint: Expected element expression or ',' inside set "
				        "literal");
			}
		}
	}
	--i;
	throw ParserError(context.tokens[i].line,
	                  "Bug: Lexer did not ensure a closing bracket");
}

HasClassIdNode *loadMap(in_func, size_t &i, HasClassIdNode *firstExpression) {
	std::vector<std::pair<HasClassIdNode *, HasClassIdNode *>> values;
	Lexer::Token *token;
	if (!nextToken(&token, context.tokens, i)) {
		--i;
		throw ParserError(context.tokens[i].line,
		                  "Bug: Lexer did not ensure a closing bracket");
	}
	values.push_back(
	    std::make_pair(firstExpression, loadExpression(in_data, 0, i)));
	while (nextToken(&token, context.tokens, i)) {
		switch (token->type) {
			using namespace Lexer;
			case Lexer::TokenType::RPAREN:
			case Lexer::TokenType::RBRACKET: {
				throw ParserError(
				    token->line, "Bug: Lexer did not ensure a closing bracket");
			}
			case Lexer::TokenType::RBRACE: {
				return context.createMapPool.push(token->line, nullptr,
				                                  std::move(values));
			}
			case TokenType::COMMA: {
				if (!nextToken(&token, context.tokens, i)) {
					--i;
					throw ParserError(
					    context.tokens[i].line,
					    "Bug: Lexer did not ensure a closing bracket");
				}
				if (expect(token, Lexer::TokenType::RBRACE)) {
					return context.createMapPool.push(token->line, nullptr,
					                                  std::move(values));
				}
				auto key = loadExpression(in_data, 0, i);
				if (!nextToken(&token, context.tokens, i) ||
				    !expect(token, Lexer::TokenType::COLON)) {
					--i;
					throw ParserError(context.tokens[i].line,
					                  "Expected :\nHint: Use ':' to separate "
					                  "key and value in "
					                  "map entry");
				}
				if (!nextToken(&token, context.tokens, i)) {
					--i;
					throw ParserError(
					    context.tokens[i].line,
					    "Bug: Lexer did not ensure a closing bracket");
				}
				values.push_back(
				    std::make_pair(key, loadExpression(in_data, 0, i)));
				break;
			}
			default: {
				--i;
				throw ParserError(
				    token->line,
				    "Unknown token " + token->toString(context) +
				        "\nHint: Expected map entry 'key: value' or closing "
				        "bracket '}'");
			}
		}
	}
	--i;
	throw ParserError(context.tokens[i].line,
	                  "Bug: Lexer did not ensure a closing bracket");
}

template std::vector<HasClassIdNode *> loadListArgument<false>(in_func,
                                                               size_t &i);
template std::vector<HasClassIdNode *> loadListArgument<true>(in_func,
                                                              size_t &i);

} // namespace Autolang

#endif

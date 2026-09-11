#ifndef DEBUGGER_LITERAL_CPP
#define DEBUGGER_LITERAL_CPP

#include "frontend/ACompiler.hpp"
#include "frontend/parser/Debugger.hpp"
#include "frontend/parser/ParserContext.hpp"

namespace Autolang {

HasClassIdNode *loadMapEntries(
    in_func, size_t &i,
    std::vector<std::pair<HasClassIdNode *, HasClassIdNode *>> values);

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
		case Lexer::TokenType::MINUS_GT: {
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
			return loadClosure(in_data, i);
		}
		default: {
			if (hasArrowAtCurrentBraceLevel(context.tokens, i)) {
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
				return loadClosure(in_data, i);
			}
			auto firstExpression = loadExpression(in_data, 0, i);
			if (!nextToken(&token, context.tokens, i)) {
				--i;
				throw ParserError(
				    0, "Bug: Lexer did not ensure a closing bracket");
			}
			if (firstExpression->kind == NodeType::PAIR) {
				auto pair = static_cast<PairNode *>(firstExpression);
				std::vector<std::pair<HasClassIdNode *, HasClassIdNode *>> values;
				values.emplace_back(pair->first, pair->second);
				if (token->type == Lexer::TokenType::RBRACE) {
					return context.createMapPool.push(token->line, nullptr,
					                                  std::move(values));
				}
				if (token->type == Lexer::TokenType::COMMA) {
					--i;
					return loadMapEntries(in_data, i, std::move(values));
				}
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

HasClassIdNode *loadMapEntries(
    in_func, size_t &i,
    std::vector<std::pair<HasClassIdNode *, HasClassIdNode *>> values) {
	Lexer::Token *token;
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
				if (key->kind == NodeType::PAIR) {
					auto pair = static_cast<PairNode *>(key);
					values.emplace_back(pair->first, pair->second);
					break;
				}
				if (!nextToken(&token, context.tokens, i) ||
				    !expect(token, Lexer::TokenType::COLON)) {
					--i;
					throw ParserError(context.tokens[i].line,
					                  "Expected ':' or 'to'\nHint: Use ':' or 'to' to separate "
					                  "key and value in map entry");
				}
				if (!nextToken(&token, context.tokens, i)) {
					--i;
					throw ParserError(
					    context.tokens[i].line,
					    "Bug: Lexer did not ensure a closing bracket");
				}
				values.emplace_back(key, loadExpression(in_data, 0, i));
				break;
			}
			default: {
				--i;
				throw ParserError(
				    token->line,
				    "Unknown token " + token->toString(context) +
				        "\nHint: Expected map entry 'key: value', 'key to value', or closing "
				        "bracket '}'");
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
	values.emplace_back(firstExpression, loadExpression(in_data, 0, i));
	return loadMapEntries(in_data, i, std::move(values));
}

template std::vector<HasClassIdNode *>
loadListArgument<false>(in_func, size_t &i,
                        std::vector<LexerStringId> *argumentNames);
template std::vector<HasClassIdNode *>
loadListArgument<true>(in_func, size_t &i,
                       std::vector<LexerStringId> *argumentNames);

} // namespace Autolang

#endif

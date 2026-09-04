#ifndef DEBUGGER_EXPRESSION_CPP
#define DEBUGGER_EXPRESSION_CPP

#include "frontend/ACompiler.hpp"
#include "frontend/parser/Debugger.hpp"
#include "frontend/parser/ParserContext.hpp"
#include "shared/DefaultClass.hpp"
#include "shared/DefaultFunction.hpp"
#include "shared/DefaultOperator.hpp"
#include <cassert>
#include <cctype>
#include <cstdlib>
#include <string>

namespace Autolang {

HasClassIdNode *loadExpression(in_func, int minPrecedence, size_t &i) {
	HasClassIdNode *left = parsePrimary(in_data, i);
	Lexer::Token *token = &context.tokens[i];
	uint32_t firstLine = token->line;
	uint32_t tokenIndex = i;
	while (nextToken(&token, context.tokens, i)) {
		switch (token->type) {
			case Lexer::TokenType::COMMA:
			case Lexer::TokenType::RPAREN:
			case Lexer::TokenType::RBRACKET:
			case Lexer::TokenType::RBRACE: {
				--i;
				return left;
			}
			default:
				break;
		};
		int precedence = getPrecedence(token->type);
		if (precedence == -1 || precedence < minPrecedence)
			break;
		Lexer::TokenType op = token->type;
		if (firstLine != token->line &&
		    (op == Lexer::TokenType::IS || op == Lexer::TokenType::NOT_IS ||
		     op == Lexer::TokenType::IN_ || op == Lexer::TokenType::NOT_IN)) {
			break;
		}
		if (!nextToken(&token, context.tokens, i)) {
			--i;
			throw ParserError(
			    context.tokens[i].line,
			    "Expected expression after operator but not found\nHint: "
			    "Provide a valid right operand expression after operator");
		}
		HasClassIdNode *right = loadExpression(in_data, precedence + 1, i);
		switch (op) {
			case Lexer::TokenType::DOT_DOT_LT: {
				left = context.rangeNode.push(firstLine, left, right, true);
				continue;
			}
			case Lexer::TokenType::DOT_DOT: {
				left = context.rangeNode.push(firstLine, left, right, false);
				continue;
			}
			case Lexer::TokenType::QMARK_QMARK: {
				left = context.nullCoalescingPool.push(firstLine, left, right);
				continue;
			}
			default:
				break;
		}
		// auto binaryNode = context.binaryNodePool.push(op, left, right);
		left = context.binaryNodePool.push(
		    firstLine, tokenIndex, context.currentClassId, op, left, right);
		// auto binaryNode =
		//     std::make_unique<BinaryNode>(firstLine, op, left.release(),
		//     right);
		// if (minPrecedence == 0) {
		// 	left.reset(binaryNode);
		// 	auto node = binaryNode->calculate(in_data);
		// 	if (node != nullptr) {
		// 		// binaryNode->left = nullptr;
		// 		// binaryNode->right = nullptr;
		// 		left.reset(node);
		// 		continue;
		// 	}
		// }
		// left.reset(binaryNode.release());
		// std::cerr<<"op "<<binaryNode<<":"<<Lexer::Token(0,
		// binaryNode->op,
		// "").toString()<<'\n';
	}
	--i;
	return left;
}

HasClassIdNode *parsePrimary(in_func, size_t &i) {
	Lexer::Token *token = &context.tokens[i];
	uint32_t firstLine = token->line;
	HasClassIdNode *node;
	switch (token->type) {
		case Lexer::TokenType::IDENTIFIER: {
			node = loadIdentifier(in_data, i);
			if (node->kind != NodeType::CALL ||
			    (context.currentClassId &&
			     context.currentFunctionId == context.mainFunctionId)) {
				break;
			}
			auto n = static_cast<CallNode *>(node);
			if (!n->caller) {
				auto declarationNode = context.findDeclaration(
				    in_data, token->line, n->nameId, true);
				n->funcObject = declarationNode;
			}
			break;
		}
		case Lexer::TokenType::PLUS_PLUS:
		case Lexer::TokenType::MINUS_MINUS:
		case Lexer::TokenType::PLUS:
		case Lexer::TokenType::EXMARK:
		case Lexer::TokenType::MINUS: {
			auto op = token->type;
			if (!nextTokenSameLine(&token, context.tokens, i, token->line)) {
				--i;
				throw ParserError(
				    firstLine, "Expected value after '" +
				                   Lexer::Token(0, op).toString(context) + "'");
			}
			return context.unaryNodePool.push(
			    token->line,
			    op == Lexer::TokenType::EXMARK ? Lexer::TokenType::NOT : op,
			    parsePrimary(in_data, i));
		}
		case Lexer::TokenType::NUMBER: {
			node = loadNumber(in_data, i);
			break;
		}
		case Lexer::TokenType::STRING: {
			node = context.constValuePool.push(
			    firstLine, context.lexerString[token->indexData]);
			break;
		}
		case Lexer::TokenType::LT: {
			bool isGeneric = false;
			std::vector<ClassDeclaration *> inputVecs;
			loadListGenericDeclarationType(in_data, i, firstLine, false,
			                               inputVecs, isGeneric);
			if (!nextTokenSameLine(&token, context.tokens, i, firstLine)) {
				--i;
				throw ParserError(firstLine, "Expected array after <Type>");
			}
			switch (token->type) {
				case Lexer::TokenType::LBRACKET: {
					if (inputVecs.size() != 1) {
						throw ParserError(
						    token->line,
						    "'Array' expects 1 type argument but " +
						        std::to_string(inputVecs.size()) +
						        " were given");
					}
					auto classDeclaration =
					    context.classDeclarationAllocator.push();
					classDeclaration->baseClassLexerStringId = lexerIdArray;
					classDeclaration->inputClassId = std::move(inputVecs);
					classDeclaration->line = firstLine;
					classDeclaration->isGeneric = isGeneric;
					if (!isGeneric) {
						context.allClassDeclarations.push_back(
						    classDeclaration);
					}
					auto list = loadListArgument<true>(in_data, i);
					node = context.createArrayPool.push(
					    firstLine, classDeclaration, std::move(list));
					break;
				}
				case Lexer::TokenType::LBRACE: {
					if (inputVecs.size() == 1) {
						auto classDeclaration =
						    context.classDeclarationAllocator.push();
						classDeclaration->baseClassLexerStringId = lexerIdSet;
						classDeclaration->inputClassId = std::move(inputVecs);
						classDeclaration->line = firstLine;
						classDeclaration->isGeneric = isGeneric;
						if (!isGeneric) {
							context.allClassDeclarations.push_back(
							    classDeclaration);
						}
						node = inferenceNodeFromLBrace(in_data, i,
						                               NodeType::CREATE_SET);
						static_cast<CreateSetNode *>(node)->classDeclaration =
						    classDeclaration;
					} else {
						if (inputVecs.size() != 2) {
							throw ParserError(
							    token->line,
							    "'Map' expects 2 type argument but " +
							        std::to_string(inputVecs.size()) +
							        " were given");
						}
						auto classDeclaration =
						    context.classDeclarationAllocator.push();
						classDeclaration->baseClassLexerStringId = lexerIdMap;
						classDeclaration->inputClassId = std::move(inputVecs);
						classDeclaration->line = firstLine;
						classDeclaration->isGeneric = isGeneric;
						if (!isGeneric) {
							context.allClassDeclarations.push_back(
							    classDeclaration);
						}
						node = inferenceNodeFromLBrace(in_data, i,
						                               NodeType::CREATE_MAP);
						static_cast<CreateMapNode *>(node)->classDeclaration =
						    classDeclaration;
					}
					break;
				}
				default: {
					throw ParserError(firstLine, "Expected array after <Type>");
				}
			}
			break;
		}
		case Lexer::TokenType::LBRACKET: {
			auto list = loadListArgument<true>(in_data, i);
			node = context.createArrayPool.push(firstLine, nullptr,
			                                    std::move(list));
			break;
		}
		case Lexer::TokenType::LBRACE: {
			node = inferenceNodeFromLBrace(in_data, i, NodeType::UNKNOW);
			break;
		}
		case Lexer::TokenType::LPAREN: {
			auto list = loadListArgument(in_data, i);
			if (list.size() != 1) {
				if (list.empty()) {
					throw ParserError(firstLine,
					                  "Expected value but empty bracket found");
				}
				throw ParserError(firstLine,
				                  "Expected value but arguments found");
			}
			node = list[0];
			break;
		}
		case Lexer::TokenType::IF: {
			node = loadIf(in_data, i, true);
			break;
		}
		case Lexer::TokenType::WHEN: {
			node = loadWhen(in_data, i, true);
			break;
		}
		default:
			throw ParserError(firstLine, "Expected value but token '" +
			                                 token->toString(context) +
			                                 "' found");
	}
	bool addOptionalNode = false;
	while (true) {
		uint32_t endLine = context.tokens[i].line;
		if (!nextToken(&token, context.tokens, i))
			goto ret;
		switch (token->type) {
			case Lexer::TokenType::LBRACKET: {
				if (token->line != endLine)
					goto ret;
				uint32_t firstLine = token->line;
				size_t tokenIndex = i;
				auto arguments = loadListArgument(in_data, i);
				bool isForceNonNull = nextTokenIfMarkNonNull(in_data, i);
				node = context.callNodePool.push(
				    firstLine, tokenIndex, context.currentClassId, node,
				    lexerIdLRBRACKET, std::move(arguments),
				    context.justFindStatic, !isForceNonNull, false);
				if (isForceNonNull) {
					static_cast<CallNode *>(node)->isForceNonNull = true;
				}
				break;
			}
			case Lexer::TokenType::LPAREN: {
				if (token->line != endLine)
					goto ret;
				size_t tokenIndex = i;
				auto arguments = loadListArgument(in_data, i);
				bool isForceNonNull = nextTokenIfMarkNonNull(in_data, i);
				auto callNode = context.callNodePool.push(
				    firstLine, tokenIndex, context.currentClassId, nullptr, 0,
				    std::move(arguments), context.justFindStatic,
				    !isForceNonNull, false);
				if (isForceNonNull) {
					callNode->isForceNonNull = true;
				}
				callNode->funcObject = node;
				node = callNode;
				break;
			}
			case Lexer::TokenType::PLUS_PLUS: {
				node = context.unaryNodePool.push(token->line, token->type,
				                                  node, false);
				break;
			}
			case Lexer::TokenType::MINUS_MINUS: {
				node = context.unaryNodePool.push(token->line, token->type,
				                                  node, false);
				break;
			}
			case Lexer::TokenType::EXMARK: {
				node->setNullable(false);
				if (node->isNullableNode()) {
					static_cast<NullableNode *>(node)->isForceNonNull = true;
				}
				while (
				    nextTokenSameLine(&token, context.tokens, i, token->line) &&
				    expect(token, Lexer::TokenType::EXMARK)) {
				}
				--i;
				if (!nextToken(&token, context.tokens, i) ||
				    !expect(token, Lexer::TokenType::DOT)) {
					goto ret;
				}
			}
			case Lexer::TokenType::QMARK_DOT:
			case Lexer::TokenType::DOT: {
				bool accessNullable =
				    token->type == Lexer::TokenType::QMARK_DOT;
				if (!addOptionalNode && accessNullable)
					addOptionalNode = true;
				if (!nextToken(&token, context.tokens, i) ||
				    !expect(token, Lexer::TokenType::IDENTIFIER)) {
					--i;
					throw ParserError(
					    context.tokens[i].line,
					    "Expected identifier after '.' but not found");
				}
				auto temp = loadIdentifier(in_data, i, false);
				switch (temp->kind) {
					case NodeType::VAR: {
						node = context.getPropPool.push(
						    token->line, nullptr, context.currentClassId, node,
						    token->indexData, false, temp->isNullable(),
						    accessNullable);
						ExprNode::deleteNode(temp);
						break;
					}
					case NodeType::UNKNOW: {
						node = context.getPropPool.push(
						    token->line, nullptr, context.currentClassId, node,
						    token->indexData, false,
						    static_cast<UnknowNode *>(temp)->nullable,
						    accessNullable);
						ExprNode::deleteNode(temp);
						break;
					}
					case NodeType::CONST_VAL: {
						throw ParserError(firstLine,
						                  "Cannot call a constant value");
					}
					default: {
						assert(temp->kind == NodeType::CALL);
						static_cast<CallNode *>(temp)->caller = node;
						static_cast<CallNode *>(temp)->accessNullable =
						    accessNullable;
						node = temp;
						break;
					}
				}
				break;
			}
			case Lexer::TokenType::PLUS_EQUAL:
			case Lexer::TokenType::MINUS_EQUAL:
			case Lexer::TokenType::STAR_EQUAL:
			case Lexer::TokenType::SLASH_EQUAL:
			case Lexer::TokenType::PERCENT_EQUAL:
			case Lexer::TokenType::EQUAL: {
				Lexer::TokenType op = token->type;
				if (!nextToken(&token, context.tokens, i)) {
					--i;
					throw ParserError(
					    context.tokens[i].line,
					    "Expected expression after '=' but not found");
				}
				if (addOptionalNode) {
					--i;
					throw ParserError(
					    firstLine,
					    "Invalid assignment target, you must use non "
					    "null variables to assignment");
				}
				auto value = loadExpression(in_data, 0, i);
				switch (node->kind) {
					case NodeType::GET_PROP:
					case NodeType::VAR: {
						auto varNode = static_cast<AccessNode *>(node);
						if (varNode->declaration) {
							if (varNode->declaration->isVal) {
								ExprNode::deleteNode(value);
								throw ParserError(
								    token->line,
								    varNode->declaration->name +
								        " cannot be changed because it's val");
							}
						}
						return context.setValuePool.push(
						    token->line, varNode, value, context.justFindStatic,
						    op);
					}
					case NodeType::CALL:
					case NodeType::UNKNOW: {
						return context.setValuePool.push(
						    token->line, node, value, context.justFindStatic,
						    op);
					}
					default:
						break;
				}
				ExprNode::deleteNode(value);
				throw ParserError(firstLine, "Invalid assignment target");
			}
			default:
				goto ret;
		}
	}
ret:
	--i;
	if (addOptionalNode) {
		return context.optionalAccessNodePool.push(firstLine, node);
	}
	return node;
}

bool nextTokenIfMarkNonNull(in_func, size_t &i) {
	Lexer::Token *token = &context.tokens[i];
	if (nextTokenSameLine(&token, context.tokens, i, token->line) &&
	    expect(token, Lexer::TokenType::EXMARK)) {
		if (!(context.mode->flags & LibraryFlags::ALLOW_NON_NULL_ASSERTION)) {
			throw ParserError(
			    token->line,
			    "Non-null assertion operator '!' is disabled (enable "
			    "'allowNonNullAssertion' option to use it)");
		}
		while (nextTokenSameLine(&token, context.tokens, i, token->line) &&
		       expect(token, Lexer::TokenType::EXMARK)) {
		}
		--i;
		return true;
	}
	--i;
	return false;
}

HasClassIdNode *loadIdentifier(in_func, size_t &i, bool allowAddThis) {
	Lexer::Token *identifier = &context.tokens[i];
	Lexer::Token *token;
	if (!nextToken(&token, context.tokens, i)) {
		--i;
		if (!allowAddThis) {
			// std::cerr << "B " << context.lexerString[identifier->indexData]
			// << "\n";
			// Never happen because closure end with '}'
			// addThisToClosure(in_data, i);
			return context.unknowNodePool.push(
			    context.tokens[i].line, context.currentClassId,
			    context.currentFunctionId, identifier->indexData, true,
			    context.justFindStaticMember);
		}
		return findIdentifierNode(in_data, i, identifier->indexData, true);
	}
	bool nullable = true;
	switch (token->type) {
		case Lexer::TokenType::LT: {
			if (!nextToken(&token, context.tokens, i) ||
			    !expect(token, Lexer::TokenType::IDENTIFIER)) {
				--i;
				break;
			}
			if (!nextToken(&token, context.tokens, i)) {
				i -= 2;
				break;
			}
			switch (token->type) {
				case Lexer::TokenType::LT:
				case Lexer::TokenType::GT:
				case Lexer::TokenType::COMMA:
				case Lexer::TokenType::QMARK:
				case Lexer::TokenType::LPAREN:
				case Lexer::TokenType::MINUS_GT:
				case Lexer::TokenType::AT_SIGN: {
					break;
				}
				default: {
					i -= 2;
					goto doneLT;
				}
			}
			auto firstLine = token->line;
			i -= 4;
			// std::cerr << "Creating " << context.tokens[i].toString(context)
			//           << "\n";
			auto classDeclaration =
			    loadClassDeclaration(in_data, i, token->line, true);
			auto funcInfo = context.getCurrentFunctionInfo(in_data);
			if (!nextToken(&token, context.tokens, i) ||
			    !expect(token, Lexer::TokenType::LPAREN)) {
				bool isGeneric = classDeclaration->isGenerics(in_data);
				if (!isGeneric) {
					context.allClassDeclarations.push_back(classDeclaration);
					// std::cerr << "Created unknownode: "
					//           << classDeclaration->getName(in_data) << " "
					//           << classDeclaration->isGenerics(in_data) <<
					//           "\n";
					// std::cerr << ParserContext::mode->path << ":" <<
					// token->line
					//           << "\n";
				}
				--i;
				auto name = classDeclaration->getName(in_data);

				LexerStringId newNameId =
				    context.createLexerStringIfNotExists(name);

				auto node = context.unknowNodePool.push(
				    context.tokens[i].line, context.currentClassId,
				    context.currentFunctionId, newNameId, true,
				    context.justFindStaticMember);
				if (isGeneric) {
					// std::cerr << "Created unknownode: "
					//           << classDeclaration->getName(in_data) << "\n";
					// std::cerr << ParserContext::mode->path << ":" <<
					// token->line
					//           << "\n";
					if (context.currentClassId) {
						auto classInfo = context.getCurrentClassInfo(in_data);
						classInfo->genericData
						    ->mustRenameNodes[classDeclaration] = node;
					} else if (context.preloadGenericData) {
						context.preloadGenericData
						    ->mustRenameNodes[classDeclaration] = node;
					}
				}
				return node;
			}
			// std::cerr << "Created callnode "
			//           << classDeclaration->getName(in_data) << "\n";
			size_t tokenIndex = i;
			auto arguments = loadListArgument(in_data, i);
			bool isForceNonNull = nextTokenIfMarkNonNull(in_data, i);
			auto callNode = context.callNodePool.push(
			    firstLine, tokenIndex, context.currentClassId, nullptr,
			    context.createLexerStringIfNotExists(
			        classDeclaration->getName(in_data)),
			    std::move(arguments), context.justFindStatic, !isForceNonNull,
			    false);
			if (isForceNonNull) {
				callNode->isForceNonNull = true;
			}
			// if (classDeclaration->isGenerics(in_data)) {

			// Must rename in both if T in class, R in function
			bool isGeneric = classDeclaration->isGenerics(in_data);
			if (!isGeneric) {
				context.genericCallers.push_back(classDeclaration);
			} else {
				if (context.currentClassId) {
					auto classInfo = context.getCurrentClassInfo(in_data);
					if (classInfo->genericData) {
						classInfo->genericData
						    ->mustRenameNodes[classDeclaration] = callNode;
					}
				} else if (context.preloadGenericData) {
					context.preloadGenericData
					    ->mustRenameNodes[classDeclaration] = callNode;
				}
			}
			// }
			return callNode;
		}
		case Lexer::TokenType::LPAREN: {
			uint32_t firstLine = token->line;
			auto arguments = loadListArgument(in_data, i);
			if (!nextToken(&token, context.tokens, i) ||
			    !expect(token, Lexer::TokenType::LBRACE)) {
				--i;
				token = &context.tokens[i];
			} else {
				auto closureNode = loadClosure(in_data, i);
				arguments.push_back(closureNode);
			}
			switch (identifier->indexData) {
				case lexerIdInt: {
					if (arguments.size() != 1) {
						throw ParserError(firstLine,
						                  "Invalid call: Int expects 1 "
						                  "argument, but " +
						                      std::to_string(arguments.size()) +
						                      " were provided");
					}
					return context.castPool.push(arguments[0],
					                             DefaultClass::intClassId);
				}
				case lexerIdFloat: {
					if (arguments.size() != 1) {
						throw ParserError(firstLine,
						                  "Invalid call: Float expects 1 "
						                  "argument, but " +
						                      std::to_string(arguments.size()) +
						                      " were provided");
					}
					return context.castPool.push(arguments[0],
					                             DefaultClass::floatClassId);
				}
				case lexerIdBool: {
					if (arguments.size() != 1) {
						throw ParserError(firstLine,
						                  "Invalid call: Bool expects 1 "
						                  "argument, but " +
						                      std::to_string(arguments.size()) +
						                      " were provided");
					}
					return context.castPool.push(arguments[0],
					                             DefaultClass::boolClassId);
				}
				case lexerIdgetClassId: {
					if (arguments.size() != 1) {
						throw ParserError(firstLine,
						                  "Invalid call: Bool expects 1 "
						                  "argument, but " +
						                      std::to_string(arguments.size()) +
						                      " were provided");
					}
					break;
				}
			}

			{
				size_t tokenIndex = i;
				auto funcObject = context.findDeclaration(
				    in_data, token->line, token->indexData, false);
				if (funcObject) {
					auto isForceNonNull = nextTokenIfMarkNonNull(in_data, i);
					auto callNode = context.callNodePool.push(
					    firstLine, tokenIndex, context.currentClassId, nullptr,
					    identifier->indexData, context.justFindStatic,
					    std::move(arguments), !isForceNonNull, false);
					if (isForceNonNull) {
						callNode->isForceNonNull = true;
					}
					return callNode;
				}
			}

			addThisToClosure(in_data, i);
			size_t tokenIndex = i;
			auto isForceNonNull = nextTokenIfMarkNonNull(in_data, i);
			auto callNode = context.callNodePool.push(
			    firstLine, tokenIndex, context.currentClassId, nullptr,
			    identifier->indexData, std::move(arguments),
			    context.justFindStatic, !isForceNonNull, false);
			if (isForceNonNull) {
				callNode->isForceNonNull = isForceNonNull;
			}
			auto funcInfo = context.getCurrentFunctionInfo(in_data);
			if (context.currentClassId) {
				if (context.currentFunctionId != context.mainFunctionId &&
				    funcInfo->genericData) {
					auto genericDeclaration =
					    funcInfo->findGenericDeclaration(identifier->indexData);
					if (genericDeclaration) {
						genericDeclaration->allCallNodes.push_back(callNode);
					}
				} else {
					auto classInfo = context.getCurrentClassInfo(in_data);
					auto genericDeclaration = classInfo->findGenericDeclaration(
					    identifier->indexData);
					if (genericDeclaration) {
						genericDeclaration->allCallNodes.push_back(callNode);
					}
				}
			} else if (context.currentFunctionId == context.mainFunctionId &&
			           funcInfo->genericData) {
				auto genericDeclaration =
				    funcInfo->findGenericDeclaration(identifier->indexData);
				if (genericDeclaration) {
					genericDeclaration->allCallNodes.push_back(callNode);
				}
			}
			return callNode;
		}
		case Lexer::TokenType::LBRACKET: {
			auto varNode =
			    findIdentifierNode(in_data, i, identifier->indexData, true);
			uint32_t firstLine = token->line;
			size_t tokenIndex = i;
			auto arguments = loadListArgument(in_data, i);
			auto isForceNonNull = nextTokenIfMarkNonNull(in_data, i);
			auto callNode = context.callNodePool.push(
			    firstLine, tokenIndex, context.currentClassId, varNode,
			    lexerIdLRBRACKET, std::move(arguments), context.justFindStatic,
			    !isForceNonNull, false);
			if (isForceNonNull) {
				callNode->isForceNonNull = isForceNonNull;
			}
			return callNode;
		}
		case Lexer::TokenType::LBRACE: {
			addThisToClosure(in_data, i);
			uint32_t firstLine = token->line;
			size_t tokenIndex = i;
			auto closureNode = loadClosure(in_data, i);
			return context.callNodePool.push(
			    firstLine, tokenIndex, context.currentClassId, nullptr,
			    identifier->indexData,
			    std::vector<HasClassIdNode *>{closureNode},
			    context.justFindStatic, false, false);
		}
		case Lexer::TokenType::EXMARK: {
			if (!(context.mode->flags &
			      LibraryFlags::ALLOW_NON_NULL_ASSERTION)) {
				throw ParserError(token->line,
				                  "Non-null assertion operator '!' is "
				                  "disabled (enable 'allowNonNullAssertion' "
				                  "option to use it)");
			}
			while (nextTokenSameLine(&token, context.tokens, i, token->line) &&
			       expect(token, Lexer::TokenType::EXMARK)) {
			}
			nullable = false;
			break;
		}
		default:
			break;
	}
doneLT:;
	--i;
	token = &context.tokens[i];
	if (!allowAddThis) {
		addThisToClosure(in_data, i);
		auto node = context.unknowNodePool.push(
		    token->line, context.currentClassId, context.currentFunctionId,
		    identifier->indexData, nullable, context.justFindStaticMember);
		if (!nullable) {
			node->isForceNonNull = true;
		}
		return node;
	}
	auto node = findIdentifierNode(in_data, i, identifier->indexData, nullable);
	if (!nullable && node->isNullableNode()) {
		static_cast<NullableNode *>(node)->isForceNonNull = true;
	}
	return node;
}

bool addThisToClosure(in_func, size_t &i) {
	if (context.currentClosureNode && context.currentClassId &&
	    !context.currentClosureNode->declarationThis) {
		auto declarationThis =
		    context.classInfo[*context.currentClassId]->declarationThis;
		for (int i = context.closureScopes.size(); i-- > 0;) {
			auto closure = context.closureScopes[i];
			if (closure->declarationThis) {
				break;
			}
			closure->declarationThis = declarationThis;
			closure->scopes[0][lexerIdthis] = declarationThis;
		}
		return true;
	}
	return false;
}

HasClassIdNode *findIdentifierNode(in_func, size_t &i, LexerStringId nameId,
                                   bool nullable) {
	if (nameId == lexerIdthis) {
		if (!context.currentClassId) {
			return context.unknowNodePool.push(
			    context.tokens[i].line, std::nullopt, context.currentFunctionId,
			    nameId, nullable, context.justFindStaticMember);
		}
		addThisToClosure(in_data, i);
		if (context.currentFunctionId == context.mainFunctionId ||
		    !(context.getCurrentFunction(in_data)->functionFlags &
		      FunctionFlags::FUNC_IS_STATIC)) {
			auto declarationThis =
			    context.classInfo[*context.currentClassId]->declarationThis;
			return context.varPool.push(declarationThis->line, declarationThis,
			                            false, false);
		}
		return context.unknowNodePool.push(
		    context.tokens[i].line, context.currentClassId,
		    context.currentFunctionId, nameId, nullable,
		    context.justFindStaticMember);
	}
	auto varNode = findVarNode(in_data, i, nameId, nullable);
	if (varNode) {
		return varNode;
	}
	// std::cerr << "C " << context.lexerString[nameId] << "\n";
	addThisToClosure(in_data, i);
	auto unknowNode = context.unknowNodePool.push(
	    context.tokens[i].line, context.currentClassId,
	    context.currentFunctionId, nameId, nullable,
	    context.justFindStaticMember);
	if (context.preloadGenericData) {
		auto declaration = context.preloadGenericData->findDeclaration(nameId);
		if (declaration) {
			auto classDeclaration = context.classDeclarationAllocator.push();
			classDeclaration->line = context.tokens[i].line;
			classDeclaration->baseClassLexerStringId = nameId;
			classDeclaration->isGeneric = true;
			declaration->allClassDeclarations.push_back(classDeclaration);
			context.preloadGenericData->mustRenameNodes[classDeclaration] =
			    unknowNode;
		}
	}
	return unknowNode;
}

HasClassIdNode *findVarNode(in_func, size_t &i, LexerStringId nameId,
                            bool nullable) {
	auto constValueNode = findConstValueNode(in_data, i, nameId);
	if (constValueNode)
		return constValueNode;
	auto node =
	    context.findDeclaration(in_data, context.tokens[i].line, nameId, true);
	if (!node)
		return nullptr;
	if (static_cast<AccessNode *>(node)->nullable) // #
		static_cast<AccessNode *>(node)->nullable = nullable;
	return node;
}

ConstValueNode *findConstValueNode(in_func, size_t &i, LexerStringId nameId) {
	switch (nameId) {
		case lexerId__FILE__: {
			return context.constValuePool.push(0, context.mode->path);
		}
		case lexerId__LINE__: {
			return context.constValuePool.push(
			    0, static_cast<int64_t>(context.tokens[i].line));
		}
		case lexerId__FUNC__: {
			return context.constValuePool.push(
			    0, context.getCurrentFunction(in_data)->getName(compile));
		}
		case lexerId__CLASS__: {
			if (context.currentClassId) {
				auto classInfo = context.getCurrentClassInfo(in_data);
				if (classInfo->genericData) {
					return nullptr;
				}
			}
			return context.constValuePool.push(
			    0, context.currentClassId
			           ? context.getCurrentClass(in_data)->getName(compile)
			           : "");
		}
		case lexerIdtrue:
		case lexerIdfalse:
		case lexerIdnull: {
			return context.constValue[nameId];
		}
		default:
			break;
	}
	return nullptr;
}

ConstValueNode *loadNumber(in_func, size_t &i) {
	Lexer::Token *token = &context.tokens[i];
	uint32_t type = Autolang::DefaultClass::intClassId;
	const std::string &data = context.lexerString[token->indexData];
	const char *s = data.c_str();
	while (*s) {
		switch (*s) {
			case '.':
			case 'e':
			case 'E': {
				type = Autolang::DefaultClass::floatClassId;
				goto foundFlag;
			}
		}
		++s;
	}
foundFlag:
	return type == Autolang::DefaultClass::intClassId
	           ? context.constValuePool.push(
	                 token->line, static_cast<int64_t>(std::stoll(data)))
	           : context.constValuePool.push(
	                 token->line, static_cast<double>(std::stod(data)));
}

} // namespace Autolang

#endif

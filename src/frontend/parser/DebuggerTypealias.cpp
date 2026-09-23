#ifndef DEBUGGER_TYPEALIAS_CPP
#define DEBUGGER_TYPEALIAS_CPP

#include "GenericData.hpp"
#include "frontend/ACompiler.hpp"
#include "frontend/parser/Debugger.hpp"
#include "frontend/parser/ParserContext.hpp"

namespace Autolang {

void loadTypealias(in_func, size_t &i) {
	Lexer::Token *token = &context.tokens[i];
	uint32_t firstLine = token->line;
	if (context.currentClassId ||
	    context.currentFunctionId != context.mainFunctionId ||
	    context.currentClosureNode) {
		throw ParserError(firstLine,
		                  "Typealias is only be declared at top level");
	}
	ensureNoKeyword(in_data, i);
	ensureNoAnnotations(in_data, i);
	// Condition
	if (!nextToken(&token, context.tokens, i) ||
	    !expect(token, Lexer::TokenType::IDENTIFIER)) {
		--i;
		throw ParserError(firstLine,
		                  "Expected class name but not found\nHint: Use "
		                  "identfier after typealias");
	}
	LexerStringId className = token->indexData;
	std::string leftName = std::string(context.lexerString[className]);
	bool hasDotLeft = false;

	while (nextToken(&token, context.tokens, i)) {
		if (expect(token, Lexer::TokenType::DOT)) {
			hasDotLeft = true;
			if (!nextToken(&token, context.tokens, i) ||
			    !expect(token, Lexer::TokenType::IDENTIFIER)) {
				--i;
				throw ParserError(firstLine,
				                  "Expected identifier after '.' in typealias name");
			}
			leftName += "." + std::string(context.lexerString[token->indexData]);
		} else {
			--i;
			break;
		}
	}

	if (!nextToken(&token, context.tokens, i)) {
		--i;
		throw ParserError(firstLine,
		                  "Expected '=' after identifier but not found\nHint: "
		                  "Add '=' after the identifier");
	}
	try {
		GenericData *genericData = nullptr;
		if (!hasDotLeft && expect(token, Lexer::TokenType::LT)) {
			genericData = loadGenericParameters(in_data, i);
			context.preloadGenericData = genericData;
			if (!nextToken(&token, context.tokens, i)) {
				--i;
				throw ParserError(
				    firstLine, "Expected '=' after generic parameters but not "
				               "found\nHint: Add '=' after the generic "
				               "parameters");
			}
		}
		if (!expect(token, Lexer::TokenType::EQUAL)) {
			--i;
			throw ParserError(
			    firstLine, "Expected '=' after identifier but not found\nHint: "
			               "Add '=' after the identifier");
		}

		// Check if right-hand side is a dotted identifier path or class alias
		size_t rightStart = i;
		Lexer::Token *rightToken = nullptr;
		if (nextToken(&rightToken, context.tokens, rightStart) &&
		    expect(rightToken, Lexer::TokenType::IDENTIFIER)) {
			Lexer::Token *dotToken = nullptr;
			bool hasDotRight = false;
			std::string rightName = std::string(context.lexerString[rightToken->indexData]);
			size_t lastConsumedIndex = rightStart;
			size_t peekPos = rightStart;
			while (nextToken(&dotToken, context.tokens, peekPos)) {
				if (expect(dotToken, Lexer::TokenType::DOT)) {
					hasDotRight = true;
					Lexer::Token *partToken = nullptr;
					if (!nextToken(&partToken, context.tokens, peekPos) ||
					    !expect(partToken, Lexer::TokenType::IDENTIFIER)) {
						break;
					}
					rightName += "." + std::string(context.lexerString[partToken->indexData]);
					lastConsumedIndex = peekPos;
				} else {
					break;
				}
			}

			if (hasDotLeft || hasDotRight) {
				i = lastConsumedIndex;

				std::optional<ClassId> resolvedClassId = std::nullopt;

				auto aliasIt = context.classAliasMap.find(rightName);
				if (aliasIt != context.classAliasMap.end()) {
					resolvedClassId = aliasIt->second;
				}

				if (!resolvedClassId.has_value()) {
					auto defIt = context.defaultClassMap.find(
					    context.createLexerStringIfNotExists(rightName));
					if (defIt != context.defaultClassMap.end()) {
						resolvedClassId = defIt->second;
					}
				}

				if (!resolvedClassId.has_value()) {
					size_t lastDot = rightName.rfind('.');
					if (lastDot != std::string::npos && lastDot + 1 < rightName.size()) {
						std::string suffix = rightName.substr(lastDot + 1);
						auto itSuffix = context.defaultClassMap.find(
						    context.createLexerStringIfNotExists(suffix));
						if (itSuffix != context.defaultClassMap.end()) {
							resolvedClassId = itSuffix->second;
						} else {
							suffix[0] = std::toupper(suffix[0]);
							auto itCap = context.defaultClassMap.find(
							    context.createLexerStringIfNotExists(suffix));
							if (itCap != context.defaultClassMap.end()) {
								resolvedClassId = itCap->second;
							}
						}
					}
				}

				if (!resolvedClassId.has_value()) {
					auto leftDefIt = context.defaultClassMap.find(
					    context.createLexerStringIfNotExists(leftName));
					if (leftDefIt != context.defaultClassMap.end()) {
						resolvedClassId = leftDefIt->second;
						context.classAliasMap[context.stringArena.allocateView(rightName)] = *resolvedClassId;
						context.classAliasMap[context.stringArena.allocateView(leftName)] = *resolvedClassId;
						return;
					}
				}

				if (!resolvedClassId.has_value()) {
					throw ParserError(firstLine,
					                  "Cannot resolve '" + rightName + "' as a built-in class");
				}

				context.classAliasMap[context.stringArena.allocateView(leftName)] = *resolvedClassId;
				if (!hasDotLeft) {
					context.defaultClassMap[className] = *resolvedClassId;
				}
				return;
			}
		}

		auto classDeclaration =
		    loadClassDeclaration(in_data, i, token->line, true);
		if (genericData) {
			context.preloadGenericData = nullptr;
		}
		if (!classDeclaration->isGeneric) {
			context.allClassDeclarations.push_back(classDeclaration);
		}
		{
			auto it = context.defaultClassMap.find(className);
			if (it != context.defaultClassMap.end()) {
				std::string hint =
				    "Choose a unique typealias name or remove duplicate declaration";
				if (it->second < context.classInfo.size()) {
					auto prevClassInfo = context.classInfo[it->second];
					if (prevClassInfo && prevClassInfo->mode) {
						hint = "Previously defined at " +
						       prevClassInfo->mode->path + ":" +
						       std::to_string(prevClassInfo->line) + ". " + hint;
					}
				}
				throw ParserError(firstLine,
				                  "Class " + std::string(context.lexerString[className]) +
				                      " already exists\nHint: " + hint);
			}
		}
		{
			auto it = context.typealiasMap.find(className);
			if (it != context.typealiasMap.end()) {
				std::string hint = "Typealias cannot have a duplicate name";
				if (it->second && it->second->classDeclaration &&
				    it->second->classDeclaration->mode) {
					hint = "Previously defined at " +
					       it->second->classDeclaration->mode->path + ":" +
					       std::to_string(it->second->classDeclaration->line) +
					       ". " + hint;
				}
				throw ParserError(
				    firstLine,
				    "Typealias '" + std::string(context.lexerString[className]) +
				        "' already exists\nHint: " + hint);
			}
		}
		context.typealiasMap[className] = context.typealiasPool.push(
		    classDeclaration, TypealiasState::TAS_UNVISITED, genericData);

		if (!classDeclaration->isGeneric && !classDeclaration->nullable) {
			auto it = context.defaultClassMap.find(classDeclaration->baseClassLexerStringId);
			if (it != context.defaultClassMap.end()) {
				context.classAliasMap[context.stringArena.allocateView(leftName)] = it->second;
			}
		}
	} catch (ParserError &error) {
		if (context.preloadGenericData) {
			context.preloadGenericData = nullptr;
		}
		throw error;
	}
}

} // namespace Autolang

#endif
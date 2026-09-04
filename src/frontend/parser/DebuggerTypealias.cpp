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
	if (!nextToken(&token, context.tokens, i)) {
		--i;
		throw ParserError(firstLine,
		                  "Expected '=' after identifier but not found\nHint: "
		                  "Add '=' after the identifier");
	}
	try {
		GenericData *genericData = nullptr;
		if (expect(token, Lexer::TokenType::LT)) {
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
				                  "Class " + context.lexerString[className] +
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
				    "Typealias '" + context.lexerString[className] +
				        "' already exists\nHint: " + hint);
			}
		}
		context.typealiasMap[className] = context.typealiasPool.push(
		    classDeclaration, TypealiasState::TAS_UNVISITED, genericData);
	} catch (ParserError &error) {
		if (context.preloadGenericData) {
			context.preloadGenericData = nullptr;
		}
		throw error;
	}
}

} // namespace Autolang

#endif
#ifndef DEBUGGER_CLASS_CPP
#define DEBUGGER_CLASS_CPP

#include "frontend/parser/Debugger.hpp"
#include <memory>

namespace AutoLang {

CreateClassNode *loadClass(in_func, size_t &i) {
	ensureNoKeyword(in_data, i);
	Lexer::Token *token = &context.tokens[i];
	uint32_t firstLine = token->line;
	// Name
	if (!nextTokenSameLine(&token, context.tokens, i, firstLine) ||
	    !expect(token, Lexer::TokenType::IDENTIFIER)) {
		--i;
		throw ParserError(firstLine, "Expected name but not found");
	}
	LexerStringId nameId = token->indexData;
	std::string &name = context.lexerString[nameId];
	if (!nextToken(&token, context.tokens, i)) {
		--i;
		auto n = context.newClasses.push(firstLine, nameId,
		                                 std::nullopt); // NewClasses managed
		n->pushClass(in_data);
		context.newClassesMap[n->classId] = n;
		auto lastClass = context.getCurrentClass(in_data);
		auto clazz = compile.classes[n->classId];
		context.gotoClass(clazz);
		auto declarationThis = context.declarationNodePool.push(
		    firstLine, "this", "", true, false, false);
		declarationThis->classId = n->classId;
		//'this' is always input at first position
		declarationThis->id = 0;
		auto *constructor = context.createConstructorPool.push(
		    firstLine, *context.currentClassId, name + "()",
		    std::vector<DeclarationNode *>{}, false, Lexer::TokenType::PUBLIC);
		context.getCurrentClassInfo(in_data)->secondaryConstructor.push_back(
		    constructor);
		constructor->pushFunction(in_data);
		context.gotoClass(lastClass);
		return n;
	}
	std::optional<LexerStringId> superStringId;
	if (expect(token, Lexer::TokenType::EXTENDS)) {
		if (!nextToken(&token, context.tokens, i) ||
		    !expect(token, Lexer::TokenType::IDENTIFIER)) {
			--i;
			throw ParserError(context.tokens[i].line,
			                  "Expected class name but not found");
		}
		superStringId = token->indexData;
		if (!nextToken(&token, context.tokens, i)) {
			--i;
			throw ParserError(context.tokens[i].line,
			                  "Extended class must have constructor");
		}
	}
	// Body class
	CreateClassNode *node = context.newClasses.push(
	    firstLine, nameId, superStringId); // NewClasses managed
	node->pushClass(in_data);
	context.newClassesMap[node->classId] = node;
	auto lastClass = context.getCurrentClass(in_data);
	auto clazz = compile.classes[node->classId];
	context.gotoClass(clazz);

	auto declarationThis = context.declarationNodePool.push(
	    firstLine, "this", "", true, false, false);
	declarationThis->classId = node->classId;
	//'this' is always input at first position
	declarationThis->id = 0;
	// Add declaration this
	context.getCurrentClassInfo(in_data)->declarationThis = declarationThis;
	// Has PrimaryConstructor
	//  bool hasPrimaryConstructor = false;
	if (expect(token, Lexer::TokenType::LPAREN)) {
		if (superStringId) {
			throw ParserError(context.tokens[i].line,
			                  "Extended class doesn't support data class "
			                  "constructor, you must "
			                  "declare constructor and call super");
		}
		context.getCurrentClassInfo(in_data)->primaryConstructor =
		    context.createConstructorPool.push(
		        firstLine, *context.currentClassId, name + "()",
		        loadListDeclaration(in_data, i, true), true,
		        Lexer::TokenType::PUBLIC);
		context.getCurrentClassInfo(in_data)->primaryConstructor->pushFunction(
		    in_data);
		compile
		    .functions[context.getCurrentClassInfo(in_data)
		                   ->primaryConstructor->funcId]
		    ->maxDeclaration += context.getCurrentClassInfo(in_data)
		                            ->primaryConstructor->arguments.size();
		if (!nextToken(&token, context.tokens, i)) {
			context.gotoClass(lastClass);
			--i;
		}
	}

	if (expect(token, Lexer::TokenType::LBRACE)) {
		loadBody(in_data, node->body.nodes, i, false);
		// Create constructor if it hasn't constructor
		if (!context.getCurrentClassInfo(in_data)->primaryConstructor &&
		    context.getCurrentClassInfo(in_data)
		        ->secondaryConstructor.empty()) {
			if (superStringId) {
				throw ParserError(
				    firstLine, "Extended class must be declarated constructor");
			}
			auto *constructor = context.createConstructorPool.push(
			    firstLine, *context.currentClassId, name + "()",
			    std::vector<DeclarationNode *>{}, false,
			    Lexer::TokenType::PUBLIC);
			context.getCurrentClassInfo(in_data)
			    ->secondaryConstructor.push_back(constructor);
			constructor->pushFunction(in_data);
		}

		context.gotoClass(lastClass);
	} else {
		// Create constructor if it hasn't constructor
		if (!context.getCurrentClassInfo(in_data)->primaryConstructor &&
		    context.getCurrentClassInfo(in_data)
		        ->secondaryConstructor.empty()) {
			if (superStringId) {
				throw ParserError(
				    firstLine, "Extended class must be declarated constructor");
			}
			context.getCurrentClassInfo(in_data)->primaryConstructor =
			    context.createConstructorPool.push(
			        firstLine, *context.currentClassId, name + "()",
			        std::vector<DeclarationNode *>{}, true,
			        Lexer::TokenType::PUBLIC);
			context.getCurrentClassInfo(in_data)
			    ->primaryConstructor->pushFunction(in_data);
		}
		context.gotoClass(lastClass);
		--i;
	}
	return node;
}

void loadConstructor(in_func, size_t &i) {
	if (context.getCurrentClassInfo(in_data)->primaryConstructor)
		throw ParserError(context.tokens[i].line,
		                  "Cannot declare constructor in data class");
	Lexer::Token *token = &context.tokens[i];
	uint32_t firstLine = token->line;
	// Arguments
	if (!nextToken(&token, context.tokens, i)) {
		--i;
		throw ParserError(context.tokens[i].line, "Expected ( but not found");
	}
	std::vector<DeclarationNode *> listDeclarationNode;
	if (expect(token, Lexer::TokenType::LPAREN)) {
		if (firstLine != token->line) {
			throw ParserError(firstLine, "Expected ( but not found");
		}
		listDeclarationNode = loadListDeclaration(in_data, i);
		if (!nextToken(&token, context.tokens, i)) {
			--i;
			throw ParserError(context.tokens[i].line,
			                  "Expected body but not found");
		}
	}
	// listDeclarationNode.insert(listDeclarationNode.begin(),
	// context.getCurrentClassInfo(in_data)->declarationThis);
	// Body
	if (!expect(token, Lexer::TokenType::LBRACE)) {
		--i;
		throw ParserError(context.tokens[i].line,
		                  "Expected body but not found");
	}
	// Create constructor
	auto constructor = context.createConstructorPool.push(
	    firstLine, *context.currentClassId,
	    context.getCurrentClass(in_data)->name + "()",
	    std::move(listDeclarationNode), false,
	    getAndEnsureOneAccessModifier(in_data, i));
	context.getCurrentClassInfo(in_data)->secondaryConstructor.push_back(
	    constructor);
	constructor->pushFunction(in_data);
	compile.functions[constructor->funcId]->maxDeclaration +=
	    constructor->arguments.size();
	context.gotoFunction(constructor->funcId);
	// compile
	//     .funcMap[compile.classes[*context.currentClassId]->name + "." +
	//              compile.classes[*context.currentClassId]->name]
	//     .push_back(constructor->funcId);

	// Add to scope
	auto &scope = context.getCurrentFunctionInfo(in_data)->scopes.back();
	scope["this"] = context.getCurrentClassInfo(in_data)->declarationThis;

	// context.getCurrentFunctionInfo(in_data)->declaration = 1;
	for (size_t j = 0; j < constructor->arguments.size(); ++j) {
		auto *argument = constructor->arguments[j];
		scope[argument->name] = argument;
		argument->id = j + 1;
	}
	loadBody(in_data, constructor->body.nodes, i, false);
	context.gotoFunction(context.mainFunctionId);
}

} // namespace AutoLang

#endif
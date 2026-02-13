#ifndef DEBUGGER_CLASS_CPP
#define DEBUGGER_CLASS_CPP

#include "frontend/ACompiler.hpp"
#include "frontend/parser/Debugger.hpp"
#include "shared/ClassFlags.hpp"
#include <memory>

namespace AutoLang {

CreateClassNode *loadClass(in_func, size_t &i) {
	ensureNoKeyword(in_data, i);
	Lexer::Token *token = &context.tokens[i];
	uint32_t firstLine = token->line;

	uint32_t classFlags = 0;
	if (context.annotationFlags & AnnotationFlags::AN_NATIVE) {
		throw ParserError(firstLine, "@native is only supported functions");
	}
	if (context.annotationFlags & AnnotationFlags::AN_OVERRIDE) {
		throw ParserError(firstLine, "@override is only supported functions");
	}
	if (context.annotationFlags & AnnotationFlags::AN_NO_OVERRIDE) {
		throw ParserError(firstLine,
		                  "@no_override is only supported functions");
	}
	if (context.annotationFlags & AnnotationFlags::AN_NO_CONSTRUCTOR) {
		classFlags |= ClassFlags::CLASS_NO_CONSTRUCTOR;
	}

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
		auto n = context.newClasses.push(firstLine, nameId, 0,
		                                 classFlags); // NewClasses managed
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
		context.getCurrentClassInfo(in_data)->declarationThis = declarationThis;
		if (!(classFlags & ClassFlags::CLASS_NO_CONSTRUCTOR)) {
			auto *constructor = context.createConstructorPool.push(
			    firstLine, *context.currentClassId, name + "()",
			    std::vector<DeclarationNode *>{}, false,
			    FunctionFlags::FUNC_PUBLIC);
			context.getCurrentClassInfo(in_data)
			    ->secondaryConstructor.push_back(constructor);
			constructor->pushFunction(in_data);
		}
		context.gotoClass(lastClass);
		return n;
	}
	if (expect(token, Lexer::TokenType::LT)) {
		while (true) {
			if (!nextToken(&token, context.tokens, i) ||
			    !expect(token, Lexer::TokenType::IDENTIFIER)) {
				--i;
				throw ParserError(context.tokens[i].line,
				                  "Expected class name but not found");
			}
			if (!nextToken(&token, context.tokens, i)) {
				--i;
				throw ParserError(
				    context.tokens[i].line,
				    "Expected '>' after class name but not found");
			}
			switch (token->type) {
				case Lexer::TokenType::COMMA: {
					break;
				}
				case Lexer::TokenType::GT: {
					goto finishedGenerics;
				}
				default: {
					throw ParserError(
					    firstLine,
					    "Expected '>' after class name but not found");
				}
			}
		}
	finishedGenerics:;
		if (!nextToken(&token, context.tokens, i)) {
			--i;
			throw ParserError(context.tokens[i].line,
			                  "Generics class must have body");
		}
	}
	LexerStringId superStringId;
	if (expect(token, Lexer::TokenType::EXTENDS)) {
		if (!nextToken(&token, context.tokens, i) ||
		    !expect(token, Lexer::TokenType::IDENTIFIER)) {
			--i;
			throw ParserError(context.tokens[i].line,
			                  "Expected class name but not found");
		}
		classFlags |= ClassFlags::CLASS_HAS_PARENT;
		superStringId = token->indexData;
		if (!nextToken(&token, context.tokens, i)) {
			--i;
			throw ParserError(context.tokens[i].line,
			                  "Extended class must have constructor");
		}
	}
	// Body class
	CreateClassNode *node = context.newClasses.push(
	    firstLine, nameId, superStringId, classFlags); // NewClasses managed
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
	// std::cerr<<"Clazz: "<<clazz->name<<" "<<declarationThis->id<<"\n";
	// Has PrimaryConstructor
	//  bool hasPrimaryConstructor = false;
	if (expect(token, Lexer::TokenType::LPAREN)) {
		if (classFlags & ClassFlags::CLASS_HAS_PARENT) {
			throw ParserError(context.tokens[i].line,
			                  "Extended class doesn't support data class "
			                  "constructor, you must "
			                  "declare constructor and call super");
		}
		if (classFlags & ClassFlags::CLASS_NO_CONSTRUCTOR) {
			throw ParserError(context.tokens[i].line,
			                  "@no_constructor is already applied");
		}
		context.getCurrentClassInfo(in_data)->primaryConstructor =
		    context.createConstructorPool.push(
		        firstLine, *context.currentClassId, name + "()",
		        loadListDeclaration(in_data, i, true), true,
		        FunctionFlags::FUNC_PUBLIC);
		context.getCurrentClassInfo(in_data)->primaryConstructor->pushFunction(
		    in_data);
		if (!nextToken(&token, context.tokens, i)) {
			context.gotoClass(lastClass);
			--i;
		}
	}

	if (expect(token, Lexer::TokenType::LBRACE)) {
		loadBody(in_data, node->body.nodes, i, false);
		// Create constructor if it hasn't constructor
		if (!(classFlags & ClassFlags::CLASS_NO_CONSTRUCTOR) &&
		    !context.getCurrentClassInfo(in_data)->primaryConstructor &&
		    context.getCurrentClassInfo(in_data)
		        ->secondaryConstructor.empty()) {
			if (classFlags & ClassFlags::CLASS_HAS_PARENT) {
				throw ParserError(
				    firstLine, "Extended class must be declarated constructor");
			}
			auto *constructor = context.createConstructorPool.push(
			    firstLine, *context.currentClassId, name + "()",
			    std::vector<DeclarationNode *>{}, false,
			    FunctionFlags::FUNC_PUBLIC);
			context.getCurrentClassInfo(in_data)
			    ->secondaryConstructor.push_back(constructor);
			constructor->pushFunction(in_data);
		}

		context.gotoClass(lastClass);
	} else {
		// Create constructor if it hasn't constructor
		if (!(classFlags & ClassFlags::CLASS_NO_CONSTRUCTOR) &&
		    !context.getCurrentClassInfo(in_data)->primaryConstructor &&
		    context.getCurrentClassInfo(in_data)
		        ->secondaryConstructor.empty()) {
			if (classFlags & ClassFlags::CLASS_HAS_PARENT) {
				throw ParserError(
				    firstLine, "Extended class must be declarated constructor");
			}
			context.getCurrentClassInfo(in_data)->primaryConstructor =
			    context.createConstructorPool.push(
			        firstLine, *context.currentClassId, name + "()",
			        std::vector<DeclarationNode *>{}, true,
			        FunctionFlags::FUNC_PUBLIC);
			context.getCurrentClassInfo(in_data)
			    ->primaryConstructor->pushFunction(in_data);
		}
		context.gotoClass(lastClass);
		--i;
	}
	return node;
}

void loadConstructor(in_func, size_t &i) {
	Lexer::Token *token = &context.tokens[i];
	uint32_t firstLine = token->line;
	auto clazz = context.getCurrentClass(in_data);
	auto classInfo = context.getCurrentClassInfo(in_data);

	if (clazz->classFlags & ClassFlags::CLASS_NO_CONSTRUCTOR) {
		throw ParserError(firstLine, "@no_constructor is already applied");
	}
	if (classInfo->primaryConstructor)
		throw ParserError(firstLine,
		                  "Cannot declare constructor in data class");
	uint32_t functionFlags = 0;
	Lexer::TokenType accessModifier = getAndEnsureOneAccessModifier(in_data, i);
	switch (accessModifier) {
		case Lexer::TokenType::PUBLIC: {
			functionFlags |= FunctionFlags::FUNC_PUBLIC;
			break;
		}
		case Lexer::TokenType::PRIVATE: {
			functionFlags |= FunctionFlags::FUNC_PRIVATE;
			break;
		}
		case Lexer::TokenType::PROTECTED: {
			functionFlags |= FunctionFlags::FUNC_PROTECTED;
			break;
		}
		default:
			break;
	}
	if (context.annotationFlags & AnnotationFlags::AN_NO_CONSTRUCTOR) {
		throw ParserError(firstLine,
		                  "@no_constructor is only supported classes");
	}
	if (context.annotationFlags & AnnotationFlags::AN_NATIVE) {
		throw ParserError(firstLine, "@native is only supported functions");
	}
	if (context.annotationFlags & AnnotationFlags::AN_OVERRIDE) {
		throw ParserError(firstLine, "@override is only supported functions");
	}
	if (context.annotationFlags & AnnotationFlags::AN_NO_OVERRIDE) {
		throw ParserError(firstLine,
		                  "@no_override is only supported functions");
	}

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
	if (expect(token, Lexer::TokenType::LBRACE)) {
		if (functionFlags & FunctionFlags::FUNC_IS_NATIVE) {
			--i;
			throw ParserError(firstLine,
			                  "@native function must not have a body");
		}
	} else {
		--i;
		if (!(functionFlags & FunctionFlags::FUNC_IS_NATIVE)) {
			throw ParserError(context.tokens[i].line,
			                  "Expected body but not found");
		}
	}
	// Create constructor
	auto constructor = context.createConstructorPool.push(
	    firstLine, *context.currentClassId, clazz->name + "()",
	    std::move(listDeclarationNode), false, functionFlags);
	classInfo->secondaryConstructor.push_back(constructor);
	constructor->pushFunction(in_data);
	context.gotoFunction(constructor->funcId);
	auto func = compile.functions[constructor->funcId];
	// compile
	//     .funcMap[compile.classes[*context.currentClassId]->name + "." +
	//              compile.classes[*context.currentClassId]->name]
	//     .push_back(constructor->funcId);

	if (functionFlags & FunctionFlags::FUNC_IS_NATIVE) {
		auto &token = context.annotationMetadata[AnnotationFlags::AN_NATIVE];
		auto &name = context.lexerString[token.indexData];

		auto it = context.mode->nativeFuncMap.find(name);
		if (it == context.mode->nativeFuncMap.end()) {
			throw ParserError(firstLine, "Native function name '" + name +
			                                 "' could not be found");
		}
		for (size_t j = 0; j < constructor->arguments.size(); ++j) {
			auto *argument = constructor->arguments[j];
			argument->id = j + 1;
		}
		func->native = it->second;
	} else {
		// Add to scope
		auto &scope = context.getCurrentFunctionInfo(in_data)->scopes.back();
		scope["this"] = classInfo->declarationThis;

		// context.getCurrentFunctionInfo(in_data)->declaration = 1;
		for (size_t j = 0; j < constructor->arguments.size(); ++j) {
			auto *argument = constructor->arguments[j];
			scope[argument->name] = argument;
			argument->id = j + 1;
		}
		loadBody(in_data, constructor->body.nodes, i, false);
	}
	context.gotoFunction(context.mainFunctionId);
}

} // namespace AutoLang

#endif
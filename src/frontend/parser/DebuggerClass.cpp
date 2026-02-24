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
	if (context.annotationFlags & AnnotationFlags::AN_NO_EXTENDS) {
		classFlags |= ClassFlags::CLASS_NO_EXTENDS;
	}

	// Name
	if (!nextTokenSameLine(&token, context.tokens, i, firstLine) ||
	    !expect(token, Lexer::TokenType::IDENTIFIER)) {
		--i;
		throw ParserError(firstLine, "Expected name but not found");
	}
	LexerStringId nameId = token->indexData;
	std::string &name = context.lexerString[nameId];

	auto node = context.newClasses.push(firstLine, nameId,
	                                    classFlags); // NewClasses managed
	node->pushClass(in_data);
	auto lastClass = context.getCurrentClass(in_data);
	auto clazz = compile.classes[node->classId];
	context.gotoClass(clazz);
	auto declarationThis = context.declarationNodePool.push(
	    firstLine, context.currentClassId, "this", nullptr, true, false, false);
	declarationThis->classId = node->classId;
	//'this' is always input at first position
	declarationThis->id = 0;
	auto classInfo = context.getCurrentClassInfo(in_data);
	classInfo->declarationThis = declarationThis;

	if (!nextToken(&token, context.tokens, i)) {
		--i;
		context.newDefaultClassesMap[node->classId] = node;
		if (!(classFlags & ClassFlags::CLASS_NO_CONSTRUCTOR)) {
			auto *constructor = context.createConstructorPool.push(
			    firstLine, *context.currentClassId, name + "()",
			    std::vector<DeclarationNode *>{}, false,
			    FunctionFlags::FUNC_PUBLIC);
			classInfo->secondaryConstructor.push_back(constructor);
			constructor->pushFunction(in_data);
		}
		context.gotoClass(lastClass);
		return node;
	}
	if (expect(token, Lexer::TokenType::LT)) {
		classInfo->genericData = new GenericData();
		context.newGenericClassesMap[node->classId] = node;
		while (true) {
			if (!nextToken(&token, context.tokens, i) ||
			    !expect(token, Lexer::TokenType::IDENTIFIER)) {
				--i;
				throw ParserError(context.tokens[i].line,
				                  "Expected class name but not found");
			}
			auto &genericDeclarationName =
			    context.lexerString[token->indexData];
			if (classInfo->findGenericDeclaration(token->indexData)) {
				throw ParserError(firstLine,
				                  "Redefined " + genericDeclarationName);
			}
			Offset id = classInfo->genericData->genericDeclarations.size();
			auto declarationData =
			    new GenericDeclarationNode(firstLine, token->indexData);
			// declarationData->classDeclaration.baseClassLexerStringId =
			// nameId; declarationData->classDeclaration.isGenericDeclaration =
			// true; declarationData->classDeclaration.line = token->line;
			classInfo->genericData->genericDeclarations.push_back(
			    declarationData);
			classInfo->genericData->genericDeclarationMap[token->indexData] =
			    id;
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
	} else {
		context.newDefaultClassesMap[node->classId] = node;
	}

	if (expect(token, Lexer::TokenType::EXTENDS)) {
		node->superDeclaration =
		    loadClassDeclaration(in_data, i, token->line, false);
		context.allClassDeclarations.push_back(node->superDeclaration);
		// auto name = node->superDeclaration->getName(in_data);
		// std::cerr << "Created extends: " << name << "\n";
		// LexerStringId newNameId;
		// {
		// 	auto it = context.lexerStringMap.find(name);
		// 	if (it == context.lexerStringMap.end()) {
		// 		newNameId = context.lexerString.size();
		// 		context.lexerStringMap[name] = newNameId;
		// 		context.lexerString.push_back(name);
		// 	} else {
		// 		newNameId = it->second;
		// 	}
		// }

		classFlags |= ClassFlags::CLASS_HAS_PARENT;
		clazz->classFlags |= ClassFlags::CLASS_HAS_PARENT;
		node->classFlags |= ClassFlags::CLASS_HAS_PARENT;
		if (!nextToken(&token, context.tokens, i)) {
			--i;
			throw ParserError(context.tokens[i].line,
			                  "Extended class must have constructor");
		}
	}
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
		classInfo->primaryConstructor = context.createConstructorPool.push(
		    firstLine, *context.currentClassId, name + "()",
		    loadListDeclaration(in_data, i, true), true,
		    FunctionFlags::FUNC_PUBLIC);
		classInfo->primaryConstructor->pushFunction(in_data);
		if (!nextToken(&token, context.tokens, i)) {
			context.gotoClass(lastClass);
			--i;
			return node;
		}
	}

	if (expect(token, Lexer::TokenType::LBRACE)) {
		loadBody(in_data, node->body.nodes, i, false);
		// Create constructor if it hasn't constructor
		if (!(classFlags & ClassFlags::CLASS_NO_CONSTRUCTOR) &&
		    !classInfo->primaryConstructor &&
		    classInfo->secondaryConstructor.empty()) {
			if (classFlags & ClassFlags::CLASS_HAS_PARENT) {
				throw ParserError(
				    firstLine, "Extended class must be declarated constructor");
			}
			auto *constructor = context.createConstructorPool.push(
			    firstLine, *context.currentClassId, name + "()",
			    std::vector<DeclarationNode *>{}, false,
			    FunctionFlags::FUNC_PUBLIC);
			classInfo->secondaryConstructor.push_back(constructor);
			constructor->pushFunction(in_data);
		}

		context.gotoClass(lastClass);
	} else {
		// Create constructor if it hasn't constructor
		if (!(classFlags & ClassFlags::CLASS_NO_CONSTRUCTOR) &&
		    !classInfo->primaryConstructor &&
		    classInfo->secondaryConstructor.empty()) {
			if (classFlags & ClassFlags::CLASS_HAS_PARENT) {
				throw ParserError(
				    firstLine, "Extended class must be declarated constructor");
			}
			classInfo->primaryConstructor = context.createConstructorPool.push(
			    firstLine, *context.currentClassId, name + "()",
			    std::vector<DeclarationNode *>{}, true,
			    FunctionFlags::FUNC_PUBLIC);
			classInfo->primaryConstructor->pushFunction(in_data);
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
	if (context.annotationFlags & AnnotationFlags::AN_NO_EXTENDS) {
		throw ParserError(firstLine, "@no_extends is only supported classes");
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
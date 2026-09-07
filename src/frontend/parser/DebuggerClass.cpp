#ifndef DEBUGGER_CLASS_CPP
#define DEBUGGER_CLASS_CPP

#include "frontend/ACompiler.hpp"
#include "frontend/parser/Debugger.hpp"
#include "frontend/parser/ParserContext.hpp"
#include "shared/ClassFlags.hpp"
#include <memory>

namespace Autolang {

CreateClassNode *loadClass(in_func, size_t &i) {
	ensureNoKeyword(in_data, i);
	Lexer::Token *token = &context.tokens[i];
	uint32_t firstLine = token->line;

	uint32_t classFlags = 0;
	if (context.annotationFlags & AnnotationFlags::AN_NATIVE) {
		throw ParserError(firstLine,
		                  "@native is only supported on functions\nHint: "
		                  "Remove @native from class declaration");
	}
	if (context.annotationFlags & AnnotationFlags::AN_OVERRIDE) {
		throw ParserError(firstLine,
		                  "@override is only supported on functions\nHint: "
		                  "Remove @override from class declaration");
	}
	if (context.annotationFlags & AnnotationFlags::AN_NO_OVERRIDE) {
		throw ParserError(firstLine,
		                  "@no_override is only supported on functions\nHint: "
		                  "Remove @no_override from class declaration");
	}
	if (context.annotationFlags & AnnotationFlags::AN_OPERATOR) {
		throw ParserError(firstLine,
		                  "@operator is only supported on functions\nHint: "
		                  "Remove @operator from class declaration");
	}
	if (context.annotationFlags & AnnotationFlags::AN_NO_CONSTRUCTOR) {
		classFlags |= ClassFlags::CLASS_NO_CONSTRUCTOR;
	}
	if (context.annotationFlags & AnnotationFlags::AN_NO_EXTENDS) {
		classFlags |= ClassFlags::CLASS_NO_EXTENDS;
	}
	if (context.annotationFlags & AnnotationFlags::AN_NATIVE_DATA) {
		// classFlags |= ClassFlags::CLASS_NATIVE_DATA;
	}
#ifdef __EMSCRIPTEN__
	if (context.annotationFlags & AnnotationFlags::AN_JS_OBJECT) {
		classFlags |= ClassFlags::CLASS_JS_OBJECT;
		classFlags |= ClassFlags::CLASS_NO_CONSTRUCTOR;
	}
#elif __PYBIND11__
	if (context.annotationFlags & AnnotationFlags::AN_PY_OBJECT) {
		classFlags |= ClassFlags::CLASS_PY_OBJECT;
		classFlags |= ClassFlags::CLASS_NO_CONSTRUCTOR;
	}
#endif

	// Name
	if (!nextTokenSameLine(&token, context.tokens, i, firstLine) ||
	    !expect(token, Lexer::TokenType::IDENTIFIER)) {
		--i;
		throw ParserError(firstLine,
		                  "Expected name but not found\nHint: Provide a valid "
		                  "class name, e.g. 'class MyClass'");
	}
	LexerStringId nameId = token->indexData;
	const std::string &name = context.lexerString[nameId];
	{
		auto it = context.defaultClassMap.find(nameId);
		if (it != context.defaultClassMap.end()) {
			std::string hint =
			    "Choose a unique class name or remove duplicate declaration";
			if (it->second < context.classInfo.size()) {
				auto prevClassInfo = context.classInfo[it->second];
				if (prevClassInfo && prevClassInfo->mode) {
					hint = "Previously defined at " +
					       prevClassInfo->mode->path + ":" +
					       std::to_string(prevClassInfo->line) + ". " + hint;
				}
			}
			throw ParserError(firstLine, "Class " + name +
			                                 " already exists\nHint: " + hint);
		}
	}
	{
		auto it = context.typealiasMap.find(nameId);
		if (it != context.typealiasMap.end()) {
			std::string hint =
			    "Class names cannot collide with typealias names.";
			if (it->second && it->second->classDeclaration &&
			    it->second->classDeclaration->mode) {
				hint = "Previously defined at " +
				       it->second->classDeclaration->mode->path + ":" +
				       std::to_string(it->second->classDeclaration->line) +
				       ". " + hint;
			}
			throw ParserError(
			    firstLine,
			    "Cannot declare class with the same name as typealias: '" +
			        name + "'\nHint: " + hint);
		}
	}

	auto node = context.newClasses.push(firstLine, nameId,
	                                    classFlags); // NewClasses managed
	node->pushClass(in_data);
	auto lastClass = context.getCurrentClass(in_data);
	auto clazz = compile.classes[node->classId];
	context.gotoClass(clazz);
	auto declarationThis = context.declarationNodePool.push(
	    firstLine, context.currentClassId, lexerIdthis, "this", nullptr, true,
	    false, false);
	declarationThis->classId = node->classId;
	//'this' is always input at first position
	declarationThis->id = 0;
	auto classInfo = context.getCurrentClassInfo(in_data);
	classInfo->declarationThis = declarationThis;

	try {

		if (!nextToken(&token, context.tokens, i)) {
			--i;
			context.newDefaultClassesMap[node->classId] = node;
			if (classFlags & ClassFlags::CLASS_NO_CONSTRUCTOR) {
				context.gotoClass(lastClass);
				return node;
			}
			auto parameter = context.parameterPool.push();
			parameter->defaultValuePos = 1;
			parameter->parameters =
			    std::vector<DeclarationNode *>{classInfo->declarationThis};
			auto *constructor = context.createConstructorPool.push(
			    firstLine, *context.currentClassId, nameId, parameter, false,
			    FunctionFlags::FUNC_PUBLIC);
			classInfo->secondaryConstructor.push_back(constructor);
			constructor->pushFunction(in_data);
			context.gotoClass(lastClass);
			return node;
		}
		if (expect(token, Lexer::TokenType::LT)) {
			classInfo->genericData = loadGenericParameters(in_data, i);
			context.newGenericClassesMap[node->classId] = node;
			if (!nextToken(&token, context.tokens, i)) {
				--i;
				throw ParserError(context.tokens[i].line,
				                  "Generics class must have body\nHint: "
				                  "Provide class body '{ ... }'");
			}
		} else {
			context.newDefaultClassesMap[node->classId] = node;
		}

		if (expect(token, Lexer::TokenType::EXTENDS)) {
			node->superDeclaration =
			    loadClassDeclaration(in_data, i, token->line, false);
			if (!node->superDeclaration->isGenerics(in_data)) {
				context.allClassDeclarations.push_back(node->superDeclaration);
			}
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
				throw ParserError(
				    context.tokens[i].line,
				    "Extended class must have constructor\nHint: Add "
				    "constructor or primary constructor parameter list");
			}
		}
		// std::cerr<<"Clazz: "<<clazz->getName(compile)<<"
		// "<<declarationThis->id<<"\n"; Has PrimaryConstructor
		//  bool hasPrimaryConstructor = false;
		if (expect(token, Lexer::TokenType::LPAREN)) {
			if (classFlags & ClassFlags::CLASS_HAS_PARENT) {
				throw ParserError(
				    context.tokens[i].line,
				    "Extended classes don't support a primary (data class) "
				    "constructor; you must declare a constructor and call "
				    "super\nHint: Remove primary constructor parameters from "
				    "class header and declare 'constructor(...)' inside body");
			}
			if (classFlags & ClassFlags::CLASS_NO_CONSTRUCTOR) {
				throw ParserError(
				    context.tokens[i].line,
				    "@no_constructor is already applied, cannot use "
				    "primary constructor\nHint: Remove @no_constructor "
				    "annotation if class needs constructor");
			}
			if (classFlags & ClassFlags::CLASS_NATIVE_DATA) {
				throw ParserError(
				    context.tokens[i].line,
				    "@native_data is already applied\nHint: Remove primary "
				    "constructor or @native_data annotation");
			}
			auto parameters = loadListDeclaration(in_data, i, true);
			if (!parameters->parameterDefaultValues.empty() &&
			    !classInfo->genericData) {
				context.defaultValueParameter.push_back(parameters);
			}

			for (int i = 0; i < parameters->parameters.size(); ++i) {
				parameters->parameters[i]->id = i;
			}

			parameters->parameters.insert(parameters->parameters.begin(),
			                              classInfo->declarationThis);
			parameters->defaultValuePos += 1;
			uint32_t ctorFlags = FunctionFlags::FUNC_PUBLIC;
			if (context.annotationFlags & AnnotationFlags::AN_IMPLICIT) {
				ctorFlags |= FunctionFlags::FUNC_IS_IMPLICIT;
				context.annotationFlags &= ~AnnotationFlags::AN_IMPLICIT;
			}
			classInfo->primaryConstructor = context.createConstructorPool.push(
			    firstLine, *context.currentClassId, nameId, parameters, true,
			    ctorFlags);
			classInfo->primaryConstructor->pushFunction(in_data);
			if (!nextToken(&token, context.tokens, i)) {
				context.isInGeneric = false;
				context.gotoClass(lastClass);
				--i;
				return node;
			}
		}

		if (expect(token, Lexer::TokenType::LBRACE)) {
			loadBody<false>(in_data, node->body.nodes, i, false);
			// Create constructor if it hasn't constructor
			if (!(classFlags & ClassFlags::CLASS_NO_CONSTRUCTOR) &&
			    !classInfo->primaryConstructor &&
			    classInfo->secondaryConstructor.empty()) {
				if (classFlags & ClassFlags::CLASS_HAS_PARENT) {
					throw ParserError(firstLine,
					                  "Extended class must declare a "
					                  "constructor\nHint: Declare an explicit "
					                  "'constructor(...)' inside class body");
				}
				auto parameter = context.parameterPool.push();
				parameter->defaultValuePos = 1;
				parameter->parameters =
				    std::vector<DeclarationNode *>{classInfo->declarationThis};
				auto *constructor = context.createConstructorPool.push(
				    firstLine, *context.currentClassId, nameId, parameter,
				    false, FunctionFlags::FUNC_PUBLIC);
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
					throw ParserError(firstLine,
					                  "Extended class must declare a "
					                  "constructor\nHint: Declare an explicit "
					                  "'constructor(...)' inside class body");
				}

				auto parameter = context.parameterPool.push();
				parameter->defaultValuePos = 1;
				parameter->parameters =
				    std::vector<DeclarationNode *>{classInfo->declarationThis};
				classInfo->primaryConstructor =
				    context.createConstructorPool.push(
				        firstLine, *context.currentClassId, nameId, parameter,
				        true, FunctionFlags::FUNC_PUBLIC);
				classInfo->primaryConstructor->pushFunction(in_data);
			}

			context.gotoClass(lastClass);
			--i;
		}
		if (context.isInGeneric)
			context.isInGeneric = false;
		return node;
	} catch (const ParserError &err) {
		context.isInGeneric = false;
		context.gotoClass(lastClass);
		throw err;
	}
}

void loadConstructor(in_func, size_t &i) {
	Lexer::Token *token = &context.tokens[i];
	uint32_t firstLine = token->line;
	auto clazz = context.getCurrentClass(in_data);
	auto classInfo = context.getCurrentClassInfo(in_data);

	if (clazz->classFlags & ClassFlags::CLASS_NO_CONSTRUCTOR) {
		throw ParserError(
		    firstLine,
		    "@no_constructor is already applied\nHint: Remove @no_constructor "
		    "annotation to allow constructor declaration");
	}
	if (classInfo->primaryConstructor)
		throw ParserError(
		    firstLine,
		    "Cannot declare constructor in a data class\nHint: Data classes "
		    "use primary constructor header 'class Name(...)'");
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
		throw ParserError(
		    firstLine, "@no_constructor is only supported on classes\nHint: "
		               "Remove @no_constructor from constructor declaration");
	}
	if (context.annotationFlags & AnnotationFlags::AN_NATIVE_DATA) {
		throw ParserError(firstLine,
		                  "@native_data is only supported on classes\nHint: "
		                  "Remove @native_data from constructor declaration");
	}
	if (context.annotationFlags & AnnotationFlags::AN_NO_EXTENDS) {
		throw ParserError(firstLine,
		                  "@no_extends is only supported on classes\nHint: "
		                  "Remove @no_extends from constructor declaration");
	}
	if (context.annotationFlags & AnnotationFlags::AN_NATIVE) {
		throw ParserError(firstLine,
		                  "@native is only supported on functions\nHint: "
		                  "Remove @native from constructor declaration");
	} else if (clazz->classFlags & ClassFlags::CLASS_NATIVE_DATA) {
		throw ParserError(
		    firstLine,
		    "Class " + clazz->getName(compile) +
		        " is marked @native_data, so the constructor must be "
		        "native\nHint: Add @native(\"name\") to constructor");
	}
	if (context.annotationFlags & AnnotationFlags::AN_OVERRIDE) {
		throw ParserError(firstLine,
		                  "@override is only supported on functions\nHint: "
		                  "Remove @override from constructor declaration");
	}
	if (context.annotationFlags & AnnotationFlags::AN_NO_OVERRIDE) {
		throw ParserError(firstLine,
		                  "@no_override is only supported on functions\nHint: "
		                  "Remove @no_override from constructor declaration");
	}
	if (context.annotationFlags & AnnotationFlags::AN_IMPLICIT) {
		functionFlags |= FunctionFlags::FUNC_IS_IMPLICIT;
		context.annotationFlags &= ~AnnotationFlags::AN_IMPLICIT;
	}

	// Arguments
	if (!nextToken(&token, context.tokens, i)) {
		--i;
		throw ParserError(context.tokens[i].line,
		                  "Expected '(' but not found\nHint: Add '(' to start "
		                  "constructor parameter list");
	}
	loadConstructorBody(in_data, i, firstLine, functionFlags, clazz, classInfo);
}

void loadConstructorBody(in_func, size_t &i, uint32_t firstLine,
                         uint32_t functionFlags, AClass *clazz,
                         ClassInfo *classInfo) {
	Lexer::Token *token = &context.tokens[i];
	Parameter *parameter = nullptr;
	if (expect(token, Lexer::TokenType::LPAREN)) {
		if (firstLine != token->line) {
			throw ParserError(firstLine,
			                  "Expected '(' but not found\nHint: Add '(' to "
			                  "start constructor parameter list");
		}
		parameter = loadListDeclaration(in_data, i);
		if (!parameter->parameterDefaultValues.empty() &&
		    !classInfo->genericData) {
			context.defaultValueParameter.push_back(parameter);
		}
		if (!nextToken(&token, context.tokens, i)) {
			--i;
			throw ParserError(context.tokens[i].line,
			                  "Expected body but not found\nHint: Provide "
			                  "constructor body block '{ ... }'");
		}
	} else {
		parameter = context.parameterPool.push();
	}

	// Optional return type: e.g. fun ImplicitJson(...): ImplicitJson
	if (token->type == Lexer::TokenType::COLON) {
		auto classDeclaration =
		    loadClassDeclaration(in_data, i, token->line, true);
		if (!classDeclaration->isGenerics(in_data)) {
			context.allClassDeclarations.push_back(classDeclaration);
		}
		if (classDeclaration->nullable) {
			throw ParserError(firstLine,
			                  "Constructor return type cannot be nullable\nHint: "
			                  "Remove '?' from constructor return type");
		}
		if (context.lexerString[classDeclaration->baseClassLexerStringId] !=
		    clazz->getName(compile)) {
			throw ParserError(firstLine,
			                  "Constructor return type must be '" +
			                      clazz->getName(compile) +
			                      "' or omitted\nHint: Remove return type or "
			                      "change to '" +
			                      clazz->getName(compile) + "'");
		}
		if (!nextToken(&token, context.tokens, i)) {
			--i;
			throw ParserError(firstLine,
			                  "Expected body but not found\nHint: Provide "
			                  "constructor body block '{ ... }'");
		}
	}

	// listDeclarationNode.insert(listDeclarationNode.begin(),
	// context.getCurrentClassInfo(in_data)->declarationThis);
	// Body
	if (expect(token, Lexer::TokenType::LBRACE)) {
		if (functionFlags & FunctionFlags::FUNC_IS_NATIVE) {
			--i;
			throw ParserError(firstLine,
			                  "@native function must not have a body\nHint: "
			                  "Remove '{ ... }' body from native constructor");
		}
	} else {
		--i;
		if (!(functionFlags & FunctionFlags::FUNC_IS_NATIVE)) {
			throw ParserError(context.tokens[i].line,
			                  "Expected body but not found\nHint: Provide "
			                  "constructor body block '{ ... }'");
		}
	}
	if (functionFlags & FunctionFlags::FUNC_IS_IMPLICIT) {
		if (parameter->defaultValuePos > 1) {
			throw ParserError(
			    firstLine,
			    "Implicit constructor must have at most 1 required parameter\nHint: "
			    "Provide default values for additional parameters or declare constructor with at most 1 parameter");
		}
	}
	// Create constructor
	parameter->parameters.insert(parameter->parameters.begin(),
	                             classInfo->declarationThis);
	parameter->defaultValuePos += 1;
	auto constructor = context.createConstructorPool.push(
	    firstLine, *context.currentClassId,
	    context.lexerStringMap[clazz->getName(compile)], parameter, false,
	    functionFlags);
	classInfo->secondaryConstructor.push_back(constructor);
	constructor->pushFunction(in_data);
	context.gotoFunction(constructor->funcId);
	auto func = compile.functions[constructor->funcId];
	auto funcInfo = context.functionInfo[constructor->funcId];
	// compile
	//     .funcMap[compile.classes[*context.currentClassId]->getName(compile) +
	//     "." +
	//              compile.classes[*context.currentClassId]->getName(compile)]
	//     .push_back(constructor->funcId);

	if (functionFlags & FunctionFlags::FUNC_IS_NATIVE) {
		auto &token =
		    context.annotationMetadata[AnnotationMetadataIndex::AMI_NATIVE];
		const auto &name = context.lexerString[token.indexData];

		auto it = context.mode->nativeFuncMap.find(name);
		if (it == context.mode->nativeFuncMap.end()) {
			throw ParserError(firstLine,
			                  "Native function name '" + name +
			                      "' could not be found\nHint: Register native "
			                      "function binding in host environment");
		}
		// for (size_t j = 1; j < constructor->parameter->parameters.size();
		// ++j) { 	auto *param = constructor->parameter->parameters[j];
		// param->id = j;
		// }
		func->native = &it->second;
	} else {
		// Add to scope
		auto &scope = funcInfo->scopes.back();
		scope[lexerIdthis] = classInfo->declarationThis;

		// context.getCurrentFunctionInfo(in_data)->declaration = 1;
		for (size_t j = 0; j < constructor->parameter->parameters.size(); ++j) {
			auto *param = constructor->parameter->parameters[j];
			scope[param->baseName] = param;
			param->id = j;
		}
		loadBody<false>(in_data, constructor->body.nodes, i, false);
	}
	context.gotoFunction(context.mainFunctionId);
}

} // namespace Autolang

#endif
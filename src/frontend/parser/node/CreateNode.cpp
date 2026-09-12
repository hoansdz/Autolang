#ifndef CREATE_NODE_CPP
#define CREATE_NODE_CPP

#include "frontend/parser/node/CreateNode.hpp"
#include "frontend/ACompiler.hpp"
#include "frontend/parser/ParserContext.hpp"
#include "shared/ClassFlags.hpp"
#include "shared/DefaultFunction.hpp"
#include "shared/Type.hpp"
#include <functional>
#include <rapidfuzz/fuzz.hpp>

namespace Autolang {

ExprNode *DeclarationNode::optimize(in_func) {
	if (loaded) {
		return this;
	}
	{
		auto it = context.globalFunction.find(baseName);
		if (it != context.globalFunction.end()) {
			std::string hint =
			    "Rename the variable or function to avoid name collision.";
			if (!it->second.empty()) {
				FunctionId prevFuncId = it->second.front();
				auto prevFunc = compile.functions[prevFuncId];
				auto prevFuncInfo = context.functionInfo[prevFuncId];
				std::string prevPath = (prevFunc && prevFunc->path)
				                           ? prevFunc->path
				                           : "unknown";
				hint =
				    "Previously defined at " + prevPath + ":" +
				    std::to_string(prevFuncInfo ? prevFuncInfo->line : 0) +
				    ". " + hint;
			}
			throwError(
			    "Cannot declare variable with the same name as function: '" +
			    name + "'\nHint: " + hint);
		}
	}
	// if (contextCallClassId) {
	// 	auto classInfo = context.classInfo[*contextCallClassId];
	// 	auto it = classInfo->allFunction.find(baseName);
	// 	if (it != classInfo->allFunction.end()) {
	// 		std::string hint =
	// 		    "Variable names in a class cannot shadow member functions.";
	// 		if (!it->second.empty()) {
	// 			FunctionId prevFuncId = it->second.front();
	// 			auto prevFunc = compile.functions[prevFuncId];
	// 			auto prevFuncInfo = context.functionInfo[prevFuncId];
	// 			std::string prevPath = (prevFunc && prevFunc->path)
	// 			                           ? prevFunc->path
	// 			                           : "unknown";
	// 			hint =
	// 			    "Previously defined at " + prevPath + ":" +
	// 			    std::to_string(prevFuncInfo ? prevFuncInfo->line : 0) +
	// 			    ". " + hint;
	// 		}
	// 		throwError(
	// 		    "Cannot declare variable with the same name as function: '" +
	// 		    name + "'\nHint: " + hint);
	// 	}
	// }
	{
		auto it = context.defaultClassMap.find(baseName);
		if (it != context.defaultClassMap.end()) {
			std::string hint =
			    "Choose a different variable name that does not conflict with existing class names.";
			if (it->second < context.classInfo.size()) {
				auto prevClassInfo = context.classInfo[it->second];
				if (prevClassInfo && prevClassInfo->mode) {
					hint = "Previously defined at " +
					       prevClassInfo->mode->path + ":" +
					       std::to_string(prevClassInfo->line) + ". " + hint;
				}
			}
			throwError(
			    "Cannot declare variable with the same name as class: '" +
			    name + "'\nHint: " + hint);
		}
	}
	{
		auto it = context.typealiasMap.find(baseName);
		if (it != context.typealiasMap.end()) {
			std::string hint =
			    "Choose a different variable name that does not conflict with existing typealias names.";
			if (it->second && it->second->classDeclaration &&
			    it->second->classDeclaration->mode) {
				hint = "Previously defined at " +
				       it->second->classDeclaration->mode->path + ":" +
				       std::to_string(it->second->classDeclaration->line) +
				       ". " + hint;
			}
			throwError(
			    "Cannot declare variable with the same name as typealias: '" +
			    name + "'\nHint: " + hint);
		}
	}
	if (classDeclaration) {
		// Doesn't changed
		if (classDeclaration->isGenerics(in_data)) {
			return this;
		}
		loaded = true;
		auto &baseClassName =
		    context.lexerString[classDeclaration->baseClassLexerStringId];
		auto it = context.defaultClassMap.find(
		    classDeclaration->baseClassLexerStringId);
		if (it == context.defaultClassMap.end()) {
			std::string bestSuggestion;
			double bestScore = 0.0;
			auto checkSuggestion = [&](const std::string &candidate) {
				double score = rapidfuzz::fuzz::ratio(baseClassName, candidate);
				if (score > bestScore && score >= 60.0) {
					bestScore = score;
					bestSuggestion = candidate;
				}
			};
			for (const auto &pair : context.defaultClassMap) {
				checkSuggestion(context.lexerString[pair.first]);
			}
			for (const auto &clazz : compile.classes) {
				if (clazz) {
					checkSuggestion(clazz->getName(compile));
				}
			}

			std::string errorMsg =
			    "Cannot find class name: '" + baseClassName + "'";
			if (!bestSuggestion.empty() && bestSuggestion != baseClassName) {
				errorMsg += "\nDid you mean: '" + bestSuggestion + "'?";
			}
			errorMsg += "\nHint: Ensure the class name is defined and "
			            "correctly spelled in the current scope.";
			throwError(errorMsg);
		}
		auto classInfo = context.classInfo[it->second];
		if (!classInfo->genericData) {
			classId = it->second;
			nullable = classDeclaration->nullable;
			return this;
		}
		if (classInfo->genericData->genericDeclarations.size() !=
		        classDeclaration->inputClassId.size() &&
		    !classDeclaration->isGenericDeclaration) {
			throwError("'" + baseClassName + "' expects " +
			           std::to_string(
			               classInfo->genericData->genericDeclarations.size()) +
			           " type argument but " +
			           std::to_string(classDeclaration->inputClassId.size()) +
			           " were given\nHint: Pass the correct number of generic "
			           "type arguments.");
		}
		// Generics
		if (!classDeclaration->classId) {
			throwError("Unresolved class ID for declaration '" + name +
			           "'\nHint: Internal compiler error - class declaration "
			           "was not resolved.");
		}
		classId = *classDeclaration->classId;
		// if (classId == DefaultClass::functionClassId) {
		nullable = classDeclaration->nullable;
		// }
		return this;
	}
	// printDebug("DeclarationNode: " + name + " is " +
	//            compile.classes[classId]->getName(compile));
	return this;
}

ExprNode *DeclarationNode::copy(in_func) {
	if (context.currentClassId) {
		if (baseName == lexerIdthis) {
			return context.classInfo[*context.currentClassId]->declarationThis;
		}
	}
	if (classDeclaration && !classDeclaration->isGenerics(in_data)) {
		return this;
	}
	ClassDeclaration *newClassDecl = classDeclaration;
	if (classDeclaration && classDeclaration->isGeneric) {
		newClassDecl = classDeclaration->copy(in_data);
	}
	auto newNode = context.declarationNodePool.push(
	    line, context.currentClassId, baseName, name, newClassDecl, isVal,
	    isGlobal, nullable);
	newNode->mustInferenceNullable = mustInferenceNullable;
	if (isGlobal && context.newPositionOfStaticDeclaration) {
		auto it = context.newPositionOfStaticDeclaration->find(id);
		if (it != context.newPositionOfStaticDeclaration->end()) {
			newNode->id = it->second;
		} else {
			newNode->id = id;
		}
	} else {
		newNode->id = id;
	}
	newNode->tokenIndex = tokenIndex;

	if (newClassDecl) {
		if (!newClassDecl->classId) {
			newClassDecl->template load<false>(in_data);
			if (!newClassDecl->classId) {
				newClassDecl->template load<true>(in_data);
			}
			if (!newClassDecl->classId) {
				throwError("Bug: DeclarationNode copy: Unresolved class " +
				           newClassDecl->getName(in_data) +
				           "\nHint: Internal compiler error - class "
				           "declaration was not resolved before copy.");
			}
			newNode->classId = *newClassDecl->classId;
			if (newClassDecl->isGeneric &&
			    !newClassDecl->isGenericDeclaration) {
				newClassDecl->classId = std::nullopt;
			}
		} else if (newClassDecl->classId == DefaultClass::functionClassId) {
			newClassDecl->template load<false>(in_data);
			if (!newClassDecl->classId) {
				newClassDecl->template load<true>(in_data);
			}
			newNode->classId = *newClassDecl->classId;
		} else {
			newNode->classId = *newClassDecl->classId;
		}
		// newNode->mustInferenceNullable = classDeclaration->mustInference;
		newNode->nullable = newClassDecl->nullable;

	} else {
		newNode->classId = classId;
	}
	return newNode;
}

std::string DeclarationNode::toString(in_func, bool isStaticMember) {
	std::string result = "  ";
	if (accessModifier == Lexer::TokenType::PRIVATE) {
		result += "private ";
	} else if (accessModifier == Lexer::TokenType::PROTECTED) {
		result += "protected ";
	} else {
		result += "public ";
	}
	if (isStaticMember || (isGlobal && contextCallClassId.has_value())) {
		result += "static ";
	}
	if (isLateInit) {
		result += "lateinit ";
	}
	result += (isVal ? "val " : "var ");
	result += name + ": ";
	if (classDeclaration) {
		result += classDeclaration->getName<true>(in_data);
	} else if (classId && classId < compile.classes.size()) {
		result +=
		    compile.classes[classId]->getName(compile) + (nullable ? "?" : "");
	} else {
		result += "Unknown";
	}
	return result;
}

void CreateConstructorNode::pushFunction(in_func) {
	auto *clazz = compile.classes[classId];
	auto *classInfo = context.classInfo[classId];
	funcId = compile.registerFunction<true>(
	    mode->path.c_str(), clazz, context.lexerString[nameId],
	    new ClassId[parameter->parameters.size()]{},
	    parameter->parameters.size(), classId,
	    functionFlags | FunctionFlags::FUNC_IS_CONSTRUCTOR);
	classInfo->allFunction[nameId].push_back(funcId);
	context.functionInfo.push_back(context.functionInfoAllocator.push());
	auto func = compile.functions[funcId];
	auto funcInfo = context.functionInfo[funcId];
	funcInfo->clazz = clazz;
	funcInfo->line = line;
	func->maxDeclaration = parameter->parameters.size();
	funcInfo->declaration = parameter->parameters.size();
	funcInfo->parameter = parameter;
	funcInfo->tokenIndex = 0;
	funcInfo->id = funcId;

	if (isPrimary) {
		func->functionFlags |= FunctionFlags::FUNC_IS_DATA_CONSTRUCTOR;
		// printDebug(clazz->getName(compile));
		// printDebug(arguments.size());
		for (size_t i = 1; i < parameter->parameters.size(); ++i) {
			auto param = parameter->parameters[i];
			classInfo->memberMap[param->baseName] = i - 1;
			clazz->memberMap[compile.stringArena.allocateView(param->name)] = i - 1;
			classInfo->member.push_back(param);
		}
	}
}

ExprNode *CreateConstructorNode::copy(in_func) {
	auto constructor = context.createConstructorPool.push(
	    line, *context.currentClassId,
	    context.lexerStringMap[compile.classes[*context.currentClassId]
	                               ->getName(compile)],
	    parameter->copy(in_data), true, functionFlags);
	constructor->pushFunction(in_data);
	return constructor;
}

ExprNode *CreateConstructorNode::optimize(in_func) {
	if (optimized) {
		return this;
	}
	optimized = true;
	const auto &name = context.lexerString[nameId];
	auto func = compile.functions[funcId];
	auto funcInfo = context.functionInfo[funcId];
	auto clazz = compile.classes[classId];
	// Add argument class id
	auto classInfo = context.classInfo[classId];
	ClassId *memberId = compile.getMemberRef(clazz->memberIdOffset);
	for (size_t i = 0; i < parameter->parameters.size(); ++i) {
		auto &param = parameter->parameters[i];
		if (param->classDeclaration) {
			if (!param->classDeclaration->classId) {
				param->classDeclaration->template load<false>(in_data);
			}
			if (param->classDeclaration->classId) {
				param->classId = *param->classDeclaration->classId;
				param->nullable = param->classDeclaration->nullable;
			}
		}
		func->args[i] = param->classId;
		if (isPrimary && i != 0) {
			if (clazz->memberIdOffset + i - 1 < compile.allMemberId.size()) {
				memberId[i - 1] = param->classId;
			}
		}
	}

	// Check redefine
	funcInfo->hash = funcInfo->loadHash(func);
	auto &hash = classInfo->func[nameId];
	auto it = hash.find(funcInfo->hash);
	if (it != hash.end() && compile.functions[it->second]->getName(compile) ==
	                            func->getName(compile)) {
		auto previousFunc = compile.functions[it->second];
		auto previousFuncInfo = context.functionInfo[it->second];
		std::string prevPath =
		    (previousFunc && previousFunc->path) ? previousFunc->path : "unknown";
		throwError("Redefined function: " + funcInfo->toString(in_data) +
		           "\nHint: Previously defined at " + prevPath + ":" +
		           std::to_string(previousFuncInfo ? previousFuncInfo->line : 0) +
		           ". A constructor or function with the same signature "
		           "already exists in this class.");
	}
	hash[funcInfo->hash] = func->id;

	// Check super
	if (clazz->classFlags & ClassFlags::CLASS_HAS_PARENT) {
		if (body.nodes.empty()) {
			throwError("super() must be called first in a derived class "
			           "constructor.\nHint: Call super(...) at the beginning "
			           "of the constructor body.");
		}
		auto *n = body.nodes[0];
		switch (n->kind) {
			case NodeType::CALL: {
				auto node = static_cast<CallNode *>(n);
				if (node->caller || node->nameId != lexerIdsuper) {
					throwError(
					    "super() must be called first in a derived class "
					    "constructor.\nHint: Ensure super(...) call is the "
					    "very first statement.");
				}
				node->isSuper = true;
				node->nameId = context.createLexerStringIfNotExists(
				    compile.classes[clazz->parentId]->getName(compile));
				break;
			}
			default:
				throwError("super() must be called first in a derived class "
				           "constructor.\nHint: First statement must be a "
				           "super(...) call.");
		}
	}

	// Add return bytecodes
	auto thisNode =
	    context.varPool.push(line, classInfo->declarationThis, false, false);
	body.nodes.push_back(context.returnPool.push(line, funcId, thisNode));
	return this;
}

void CreateClassNode::pushClass(in_func) {
	classId = compile.registerClass(context.lexerString[nameId], classFlags);
	context.defaultClassMap[nameId] = classId;
	auto clazz = compile.classes[classId];
	auto classInfo = context.classInfoAllocator.push();
	context.classInfo.push_back(classInfo);
	classInfo->mode = mode;
	classInfo->line = line;
	clazz->memberIdOffset = compile.allMemberId.size();
	clazz->parentId = 0;
	// if (classFlags & ClassFlags::CLASS_HAS_PARENT) {
	// 	if (!classInfo->genericData) {
	// 		return;
	// 	}
	// 	context.allClassDeclarations.push_back();
	// 	if (!superDeclaration->classId) {
	// 		throwError("Unresolved class name " +
	// 		           superDeclaration->getName(in_data));
	// 	}
	// 	auto classInfo = context.classInfo[classId];
	// 	classInfo->parent = *superDeclaration->classId;
	// }
	// auto clazz = compile.classes[classId];
	// std::cerr << "Created class name: " << clazz->getName(compile) << " id "
	// << classId
	//           << "\n";
}

ExprNode *CreateClassNode::optimize(in_func) {
	if (optimized) {
		return this;
	}
	optimized = true;
	const auto &name = context.lexerString[nameId];
	auto classInfo = context.classInfo[classId];
	if (classInfo->genericData) {
		for (auto genericDeclaration :
		     classInfo->genericData->genericDeclarations) {
			if (context.defaultClassMap.find(genericDeclaration->nameId) !=
			    context.defaultClassMap.end()) {
				throwError("Cannot declare generic type parameter with the "
				           "same name as class: '" +
				           context.lexerString[genericDeclaration->nameId] +
				           "'\nHint: Rename the generic type parameter so it "
				           "does not conflict with a class name.");
			}
		}
	} else {
		{ // Function cannot have generics name as Func<T>
			auto it = context.globalFunction.find(nameId);
			if (it != context.globalFunction.end()) {
				std::string hint =
				    "Class names cannot collide with global function names.";
				if (!it->second.empty()) {
					FunctionId prevFuncId = it->second.front();
					auto prevFunc = compile.functions[prevFuncId];
					auto prevFuncInfo = context.functionInfo[prevFuncId];
					std::string prevPath = (prevFunc && prevFunc->path)
					                           ? prevFunc->path
					                           : "unknown";
					hint =
					    "Previously defined at " + prevPath + ":" +
					    std::to_string(prevFuncInfo ? prevFuncInfo->line : 0) +
					    ". " + hint;
				}
				throwError(
				    "Cannot declare class with the same name as function: '" +
				    name + "'\nHint: " + hint);
			}
		}
	}
	if (classFlags & ClassFlags::CLASS_HAS_PARENT) {
		if (classInfo->genericData) {
			// std::cerr<<"A "<<name<<"\n";
			return this;
		}
		// std::cerr<<"B "<<name<<"\n";
		if (!superDeclaration->classId) {
			throwError("Unresolved superclass name: '" +
			           superDeclaration->getName(in_data) +
			           "'\nHint: Check if the base class exists and is "
			           "imported properly.");
		}
		auto clazz = compile.classes[classId];
		clazz->parentId = *superDeclaration->classId;
	}
	return this;
}

void CreateClassNode::loadSuper(in_func) {
	if ((classFlags & ClassFlags::CLASS_HAS_PARENT) && !loadedSuper) {
		auto clazz = compile.classes[classId];
		auto classInfo = context.classInfo[classId];
		auto superClassId = clazz->parentId;
		auto superClass = compile.classes[superClassId];
		auto superClassInfo = context.classInfo[superClassId];

		if (classInfo->genericData) {
			return;
		}

		if (superClass->classFlags & ClassFlags::CLASS_NO_EXTENDS) {
			throwError(
			    "Cannot inherit from class '" + superClass->getName(compile) +
			    "' because it is marked @no_extends\nHint: Remove @no_extends "
			    "from the base class or avoid extending it.");
		}

		loadedSuper = true;

		{
			auto node = context.findCreateClassNode(superClassId);
			if (!node) {
				throwError("Bug: Cannot find create class node " +
				           compile.classes[superClassId]->getName(compile) +
				           " " + std::to_string(superClassId) +
				           "\nHint: Internal compiler error - missing class "
				           "AST node.");
			}
			node->loadSuper(in_data);
		}

		if (superClass->inheritance.get(classId)) {
			throwError(
			    "Cyclic inheritance detected for class '" +
			    compile.classes[classId]->getName(compile) +
			    "'\nHint: Break circular dependency in class hierarchy.");
		}

		auto memberToFind = HashMap<std::string_view, DeclarationNode *>();
		memberToFind.reserve(classInfo->member.size());
		for (auto *declaration : classInfo->member) {
			memberToFind[declaration->name] = declaration;
			declaration->id += superClassInfo->member.size();
			classInfo->memberMap[declaration->baseName] = declaration->id;
			clazz->memberMap[compile.stringArena.allocateView(declaration->name)] = declaration->id;
		}
		uint32_t newPosition = superClassInfo->member.size();
		for (auto &declaration : superClassInfo->member) {
			if (declaration->accessModifier == Lexer::TokenType::PRIVATE) {
				continue;
			}
			auto it = memberToFind.find(declaration->name);
			if (it != memberToFind.end()) {
				throwError("Member '" + declaration->name +
				           "' is already declared in superclass '" +
				           superClass->getName(compile) +
				           "'. Overriding is not supported yet.\nHint: Rename "
				           "member variable in child class.");
			}
			classInfo->memberMap[declaration->baseName] = declaration->id;
			clazz->memberMap[compile.stringArena.allocateView(declaration->name)] = declaration->id;
		}
		classInfo->member.reserve(classInfo->member.size() +
		                          superClassInfo->member.size());
		classInfo->member.insert(classInfo->member.begin(),
		                         superClassInfo->member.begin(),
		                         superClassInfo->member.end());
		auto newStaticMember = superClassInfo->staticMember;
		for (auto &[key, declaration] : classInfo->staticMember) {
			if (declaration->accessModifier == Lexer::TokenType::PRIVATE) {
				continue;
			}
			newStaticMember[key] = declaration;
		}
		classInfo->staticMember = std::move(newStaticMember);
		uint32_t maxSize = std::max<ClassId>(
		    superClassId, superClass->inheritance.getSize() * 64);
		clazz->inheritance.from(superClass->inheritance, maxSize);
		clazz->parentId = superClassId;
		clazz->inheritance.set(superClassId);

		// Static can be override without error
		for (auto &[key, superStaticFuncHash] : superClassInfo->staticFunc) {
			auto &hash = classInfo->staticFunc[key];
			for (auto &[hashValue, offset] : superStaticFuncHash) {
				hash[hashValue] = offset;
			}
		}

		clazz->vtable = superClass->vtable;
		InheritanceBitset funcOverride;

		for (auto &[funcNameId, superFuncHash] : superClassInfo->func) {
			auto &hash = classInfo->func[funcNameId];
			for (auto &[hashValue, offset] : superFuncHash) {
				auto it = hash.find(hashValue);
				if (it != hash.end()) {
					auto func = compile.functions[it->second];
					auto funcInfo = context.functionInfo[it->second];
					auto superFunc = compile.functions[offset];
					auto superFuncInfo = context.functionInfo[offset];

					// Index virtual position : Three times override -> Twice
					// override -> First override  -> parent
					if (!(superFunc->functionFlags &
					      FunctionFlags::FUNC_IS_VIRTUAL)) {
						if (superFunc->functionFlags &
						    FunctionFlags::FUNC_NO_OVERRIDE) {
							throwError(
							    "Cannot override function " +
							    superFuncInfo->toString(in_data) +
							    " because it is marked @no_override\nHint: "
							    "Method in base class is marked @no_override "
							    "and cannot be redefined.");
						}
						ClassId parentId = clazz->parentId;
						while (true) {
							auto parentClassInfo = context.classInfo[parentId];
							auto it1 = parentClassInfo->func.find(funcNameId);
							if (it1 == parentClassInfo->func.end())
								break;
							auto &hashMap = it1->second;
							auto it2 = hashMap.find(hashValue);
							if (it2 == hashMap.end())
								break;
							auto parentClass = compile.classes[parentId];
							parentClass->vtable.push_back(offset);
							if (!parentClass->parentId)
								break;
							parentId = parentClass->parentId;
						}
						superFuncInfo->virtualPosition = clazz->vtable.size();
						superFunc->functionFlags |=
						    FunctionFlags::FUNC_IS_VIRTUAL;

						funcInfo->virtualPosition =
						    superFuncInfo->virtualPosition;
						clazz->vtable.push_back(it->second);
						func->functionFlags |= FunctionFlags::FUNC_IS_VIRTUAL;
					} else {
						funcInfo->virtualPosition =
						    superFuncInfo->virtualPosition;
						clazz->vtable[funcInfo->virtualPosition] = it->second;
						func->functionFlags |= FunctionFlags::FUNC_IS_VIRTUAL;
					}
					funcOverride.resize(superFunc->id + 1);
					// std::cerr << superFunc->getName(compile) << " & " <<
					// func->getName(compile) <<
					// "\n"; std::cerr << superFuncInfo->virtualPosition << " &
					// "
					//           << funcInfo->virtualPosition << "\n"
					//           << func->id << " & " << superFunc->id << "\n"
					//           << funcOverride.getSize() << "\n";
					funcOverride.set(superFunc->id);
				}
				hash[hashValue] = offset;
			}
		}

		for (auto &[nameId, vecs] : classInfo->func) {
			auto &classVecs = classInfo->allFunction[nameId];
			for (auto &[_, funcId] : vecs) {
				classVecs.push_back(funcId);
			}
		}

		for (auto &[nameId, vecs] : classInfo->staticFunc) {
			auto &classVecs = classInfo->allFunction[nameId];
			for (auto &[_, funcId] : vecs) {
				classVecs.push_back(funcId);
			}
		}

		for (auto &[name, allFuncVecs] : superClass->funcMap) {
			auto &vecs = clazz->funcMap[name];
			vecs.reserve(vecs.size() + allFuncVecs.size());
			for (auto val : allFuncVecs) {
				auto func = compile.functions[val];
				if (func->functionFlags & FunctionFlags::FUNC_IS_CONSTRUCTOR) {
					continue;
				}
				if (funcOverride.get(val)) {
					continue;
				}
				vecs.push_back(val);
			}
		}
	}
}

} // namespace Autolang

#endif
#ifndef CREATE_FUNC_NODE_CPP
#define CREATE_FUNC_NODE_CPP

#include "frontend/ACompiler.hpp"
#include "frontend/parser/node/CreateFuncNode.hpp"
#include "frontend/parser/ParserContext.hpp"

namespace Autolang {

template <bool addToGlobalScope> void CreateFuncNode::pushFunction(in_func) {
	AClass *clazz =
	    contextCallClassId ? compile.classes[*contextCallClassId] : nullptr;
	id = compile.registerFunction(
	    mode->path.c_str(), clazz, context.lexerString[nameId],
	    new ClassId[parameter->parameters.size()]{},
	    parameter->parameters.size(), Autolang::DefaultClass::voidClassId,
	    functionFlags);
	// Function can be overrided, it will be recreated in override phase
	if (clazz) {
		if (!(clazz->classFlags & ClassFlags::CLASS_HAS_PARENT)) {
			auto classInfo = context.classInfo[*contextCallClassId];
			classInfo->allFunction[nameId].push_back(id);
		}
	} else {
		if constexpr (addToGlobalScope) {
			context.globalFunction[nameId].push_back(id);
		}
	}
	context.functionInfo.push_back(context.functionInfoAllocator.push());
	auto func = compile.functions[id];
	auto funcInfo = context.functionInfo[id];
	funcInfo->clazz = clazz;
	funcInfo->id = id;
	func->maxDeclaration = parameter->parameters.size();
	funcInfo->declaration = parameter->parameters.size();
	funcInfo->parameter = parameter;
	funcInfo->tokenIndex = tokenIndex;
	if (classDeclaration &&
	    classDeclaration->classId == DefaultClass::functionClassId) {
		funcInfo->returnClass = classDeclaration;
	}
}

template <bool addToGlobalScope>
void CreateFuncNode::pushNativeFunction(in_func, ANativeFunctionData *native) {
	AClass *clazz =
	    contextCallClassId ? compile.classes[*contextCallClassId] : nullptr;
	id = compile.registerFunction(
	    mode->path.c_str(), clazz, context.lexerString[nameId],
	    new ClassId[parameter->parameters.size()]{},
	    parameter->parameters.size(), Autolang::DefaultClass::voidClassId,
	    functionFlags | FunctionFlags::FUNC_IS_NATIVE);
	// Function can be overrided, it will be recreated in override phase
	if (clazz) {
		if (!(clazz->classFlags & ClassFlags::CLASS_HAS_PARENT)) {
			auto classInfo = context.classInfo[*contextCallClassId];
			classInfo->allFunction[nameId].push_back(id);
		}
	} else {
		if constexpr (addToGlobalScope) {
			context.globalFunction[nameId].push_back(id);
		}
	}
	context.functionInfo.push_back(context.functionInfoAllocator.push());
	auto func = compile.functions[id];
	auto funcInfo = context.functionInfo[id];
	func->native = native;
	funcInfo->clazz = clazz;
	funcInfo->id = id;
	func->maxDeclaration = parameter->parameters.size();
	funcInfo->declaration = parameter->parameters.size();
	funcInfo->parameter = parameter;
	funcInfo->tokenIndex = tokenIndex;
	if (classDeclaration &&
	    classDeclaration->classId == DefaultClass::functionClassId) {
		funcInfo->returnClass = classDeclaration;
	}
}

ExprNode *CreateFuncNode::copy(in_func) {
	auto funcInfo = context.functionInfo[id];
	LexerStringId newNameId = nameId;
	if (nameId == lexerId__CLASS__) {
		if (!context.currentClassId) {
			throwError("Cannot resolve '__CLASS__' outside of class "
			           "declaration context");
		}
		newNameId =
		    context.lexerStringMap[compile.classes[*context.currentClassId]
		                               ->getName(compile)];
	}
	auto newCreateFuncNode = context.newFunctions.push(
	    line, tokenIndex, context.currentClassId, newNameId, nullptr,
	    parameter->copy(in_data), functionFlags);
	if (functionFlags & FunctionFlags::FUNC_IS_NATIVE) {
		newCreateFuncNode->pushNativeFunction(in_data,
		                                      compile.functions[id]->native);
	} else {
		newCreateFuncNode->pushFunction(in_data);
	}
	return newCreateFuncNode;
}

void CreateFuncNode::optimize(in_func) {
	const auto &name = context.lexerString[nameId];
	auto func = compile.functions[id];
	auto funcInfo = context.functionInfo[id];
	if (classDeclaration) {
		func->returnId = *classDeclaration->classId;
	}
	for (size_t i = 0; i < parameter->parameters.size(); ++i) {
		auto &param = parameter->parameters[i];
		func->args[i] = param->classId;
	}

	if (contextCallClassId) {
		funcInfo->hash = funcInfo->loadHash(func);
		auto classInfo = context.classInfo[*contextCallClassId];
		if (func->functionFlags & FunctionFlags::FUNC_IS_STATIC) {
			auto &hash = classInfo->staticFunc[nameId];
			auto it = hash.find(funcInfo->hash);
			if (it != hash.end() && compile.functions[it->second]->getName(
			                            compile) == func->getName(compile)) {
				throwError("Redefined function: " +
				           funcInfo->toString(in_data));
			}
			hash[funcInfo->hash] = func->id;
		} else {
			auto &hash = classInfo->func[nameId];
			auto it = hash.find(funcInfo->hash);
			if (it != hash.end() && compile.functions[it->second]->getName(
			                            compile) == func->getName(compile)) {
				throwError("Redefined function: " +
				           funcInfo->toString(in_data));
			}
			// std::cerr<<"Created "<<name<<" hash "<<funcInfo->hash<<"\n";
			hash[funcInfo->hash] = func->id;
		}
	}
}

} // namespace Autolang

#endif
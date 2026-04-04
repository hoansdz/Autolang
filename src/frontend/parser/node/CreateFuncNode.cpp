#ifndef CREATE_FUNC_NODE_CPP
#define CREATE_FUNC_NODE_CPP

#include "frontend/parser/node/CreateFuncNode.hpp"
#include "frontend/parser/ParserContext.hpp"

namespace AutoLang {

void CreateFuncNode::pushFunction(in_func) {
	AClass *clazz =
	    contextCallClassId ? compile.classes[*contextCallClassId] : nullptr;
	id = compile.registerFunction(clazz, context.lexerString[nameId],
	                              new ClassId[parameter->parameters.size()]{},
	                              parameter->parameters.size(),
	                              AutoLang::DefaultClass::voidClassId,
	                              functionFlags);
	// Function can be overrided, it will be recreated in override phase
	if (clazz) {
		if (!(clazz->classFlags & ClassFlags::CLASS_HAS_PARENT)) {
			auto classInfo = context.classInfo[*contextCallClassId];
			classInfo->allFunction[nameId].push_back(id);
		}
	} else {
		context.globalFunction[nameId].push_back(id);
	}
	context.functionInfo.push_back(context.functionInfoAllocator.push());
	auto func = compile.functions[id];
	auto funcInfo = context.functionInfo[id];
	new (&func->bytecodes) std::vector<uint8_t>();
	funcInfo->clazz = clazz;
	func->maxDeclaration = parameter->parameters.size();
	funcInfo->declaration = parameter->parameters.size();
	funcInfo->parameter = parameter;
}

void CreateFuncNode::pushNativeFunction(in_func, ANativeFunction native) {
	AClass *clazz =
	    contextCallClassId ? compile.classes[*contextCallClassId] : nullptr;
	id = compile.registerFunction(
	    clazz, context.lexerString[nameId],
	    new ClassId[parameter->parameters.size()]{},
	    parameter->parameters.size(), AutoLang::DefaultClass::voidClassId,
	    functionFlags | FunctionFlags::FUNC_IS_NATIVE);
	// Function can be overrided, it will be recreated in override phase
	if (clazz) {
		if (!(clazz->classFlags & ClassFlags::CLASS_HAS_PARENT)) {
			auto classInfo = context.classInfo[*contextCallClassId];
			classInfo->allFunction[nameId].push_back(id);
		}
	} else {
		context.globalFunction[nameId].push_back(id);
	}
	context.functionInfo.push_back(context.functionInfoAllocator.push());
	auto func = compile.functions[id];
	auto funcInfo = context.functionInfo[id];
	func->native = native;
	funcInfo->clazz = clazz;
	func->maxDeclaration = parameter->parameters.size();
	funcInfo->declaration = parameter->parameters.size();
	funcInfo->parameter = parameter;
}

ExprNode *CreateFuncNode::copy(in_func) {
	auto funcInfo = context.functionInfo[id];
	LexerStringId newNameId = nameId;
	if (nameId == lexerId__CLASS__) {
		if (!context.currentClassId) {
			throwError("Cannot find __CLASS__");
		}
		newNameId =
		    context
		        .lexerStringMap[compile.classes[*context.currentClassId]->name];
	}
	auto newCreateFuncNode = context.newFunctions.push(
	    line, context.currentClassId, newNameId, nullptr,
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
		funcInfo->hash = func->loadHash();
		auto classInfo = context.classInfo[*contextCallClassId];
		if (func->functionFlags & FunctionFlags::FUNC_IS_STATIC) {
			auto &hash = classInfo->staticFunc[nameId];
			auto it = hash.find(funcInfo->hash);
			if (it != hash.end() &&
			    compile.functions[it->second]->name == func->name) {
				throwError("Redefined function : " + func->toString(compile));
			}
			hash[funcInfo->hash] = func->id;
		} else {
			auto &hash = classInfo->func[nameId];
			auto it = hash.find(funcInfo->hash);
			if (it != hash.end() &&
			    compile.functions[it->second]->name == func->name) {
				throwError("Redefined function : " + func->toString(compile));
			}
			// std::cerr<<"Created "<<name<<" hash "<<funcInfo->hash<<"\n";
			hash[funcInfo->hash] = func->id;
		}
	}
}

} // namespace AutoLang

#endif
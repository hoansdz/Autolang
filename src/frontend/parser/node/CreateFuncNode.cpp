#ifndef CREATE_FUNC_NODE_CPP
#define CREATE_FUNC_NODE_CPP

#include "frontend/parser/node/CreateFuncNode.hpp"
#include "frontend/parser/ParserContext.hpp"

namespace AutoLang {

void CreateFuncNode::pushFunction(in_func) {
	AClass *clazz =
	    contextCallClassId ? compile.classes[*contextCallClassId] : nullptr;
	id = compile.registerFunction(
	    clazz, name, new ClassId[arguments.size()]{}, arguments.size(),
	    AutoLang::DefaultClass::voidClassId, functionFlags);
	context.functionInfo.push_back(context.functionInfoAllocator.push());
	auto func = compile.functions[id];
	auto funcInfo = context.functionInfo[id];
	new (&func->bytecodes) std::vector<uint8_t>();
	funcInfo->clazz = clazz;
	funcInfo->nullableArgs = new bool[arguments.size()]{};
	func->maxDeclaration = arguments.size();
	funcInfo->declaration = arguments.size();
}

void CreateFuncNode::pushNativeFunction(in_func, ANativeFunction native) {
	AClass *clazz =
	    contextCallClassId ? compile.classes[*contextCallClassId] : nullptr;
	id = compile.registerFunction(
	    clazz, name, new ClassId[arguments.size()]{}, arguments.size(),
	    AutoLang::DefaultClass::voidClassId,
	    functionFlags | FunctionFlags::FUNC_IS_NATIVE);
	context.functionInfo.push_back(context.functionInfoAllocator.push());
	auto func = compile.functions[id];
	auto funcInfo = context.functionInfo[id];
	func->native = native;
	funcInfo->clazz = clazz;
	funcInfo->nullableArgs = new bool[arguments.size()]{};
	func->maxDeclaration = arguments.size();
	funcInfo->declaration = arguments.size();
}

void CreateFuncNode::optimize(in_func) {
	if (!contextCallClassId && isDeclarationExist(in_data, name))
		throwError(
		    "Cannot declare function with the same name as declaration name " +
		    name);
	if (isClassExist(in_data, name))
		throwError("Cannot declare function with the same name as class name " +
		           name);
	auto func = compile.functions[id];
	auto funcInfo = context.functionInfo[id];
	if (classDeclaration) {
		func->returnId = *classDeclaration->classId;
	}
	for (size_t i = 0; i < arguments.size(); ++i) {
		auto &argument = arguments[i];
		func->args[i] = argument->classId;
		funcInfo->nullableArgs[i] = argument->nullable;
	}

	if (contextCallClassId) {
		funcInfo->hash = func->loadHash();
		auto classInfo = context.classInfo[*contextCallClassId];
		if (func->functionFlags & FunctionFlags::FUNC_IS_STATIC) {
			auto &hash = classInfo->staticFunc[name];
			auto it = hash.find(funcInfo->hash);
			if (it != hash.end() &&
			    compile.functions[it->second]->name == func->name) {
				throwError("Redefined function : " + func->toString(compile));
			}
			hash[funcInfo->hash] = func->id;
		} else {
			auto &hash = classInfo->func[name];
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
#ifndef CREATE_FUNC_NODE_CPP
#define CREATE_FUNC_NODE_CPP

#include "frontend/parser/ParserContext.hpp"
#include "frontend/parser/node/CreateFuncNode.hpp"

namespace AutoLang {

void CreateFuncNode::pushFunction(in_func) {
	AClass *clazz =
	    contextCallClassId ? compile.classes[*contextCallClassId] : nullptr;
	id = compile.registerFunction(clazz, isStatic, name,
	                              new ClassId[arguments.size()]{},
	                              std::vector<bool>(arguments.size(), false),
	                              clazz ? clazz->id
	                                    : AutoLang::DefaultClass::nullClassId,
	                              returnNullable, nullptr);
	auto func = compile.functions[id];
	auto funcInfo = &context.functionInfo[id];
	printDebug("Created function name: " + func->name);
	funcInfo->clazz = clazz;
	funcInfo->accessModifier = accessModifier;
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
	auto funcInfo = &context.functionInfo[id];
	if (!returnClass.empty()) {
		auto it = compile.classMap.find(returnClass);
		if (it == compile.classMap.end())
			throwError("CreateFuncNode: Cannot find class name: " +
			           returnClass);
		func->returnId = it->second;
	} else
		func->returnId = AutoLang::DefaultClass::nullClassId;
	for (size_t i = 0; i < arguments.size(); ++i) {
		auto &argument = arguments[i];
		func->args[i] = argument->classId;
		func->nullableArgs[i] = argument->nullable;
	}

	if (contextCallClassId) {
		funcInfo->hash = func->loadHash();
		auto classInfo = &context.classInfo[*contextCallClassId];
		if (isStatic) {
			auto& hash = classInfo->staticFunc[name];
			auto it = hash.find(funcInfo->hash);
			if (it != hash.end() && compile.functions[it->second]->name == func->name) {
				throwError("Redefined function : " + func->toString(compile));
			}
			hash[funcInfo->hash] = func->id;
		} else {
			auto& hash = classInfo->func[name];
			auto it = hash.find(funcInfo->hash);
			if (it != hash.end() && compile.functions[it->second]->name == func->name) {
				throwError("Redefined function : " + func->toString(compile));
			}
			std::cerr<<"Created "<<name<<" hash "<<funcInfo->hash<<"\n";
			hash[funcInfo->hash] = func->id;
		}
	}
}

} // namespace AutoLang

#endif
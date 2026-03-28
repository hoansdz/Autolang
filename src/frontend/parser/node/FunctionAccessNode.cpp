#ifndef FUNCTION_ACCESS_NODE_CPP
#define FUNCTION_ACCESS_NODE_CPP

#include "Node.hpp"
#include "frontend/parser/ParserContext.hpp"
#include "shared/ClassFlags.hpp"

namespace AutoLang {

void FunctionAccessNode::optimize(in_func) {

	if (!classDeclaration) {
		throwError("Did you forget '()' after " + context.lexerString[nameId] +
		           " ?");
	}

	if (*classDeclaration->classId != DefaultClass::functionClassId) {
		throwError("Expected Function but " +
		           compile.classes[*classDeclaration->classId]->name +
		           " found");
	}

	std::optional<FunctionId> matchFuncId;

	for (int i = 0; i < count; ++i) {
		auto vecs = funcs[i];
		for (auto funcId : *vecs) {
			auto func = compile.functions[funcId];
			if (func->returnId != *classDeclaration->inputClassId[0]->classId) {
				continue;
			}
			int j = !(func->functionFlags & FunctionFlags::FUNC_IS_STATIC);
			if (classDeclaration->inputClassId.size() - 1 !=
			    func->argSize - j) {
				continue;
			}
			int i = 1;
			for (; j < func->argSize; ++j) {
				if (func->args[j] !=
				    *classDeclaration->inputClassId[i++]->classId) {
					goto nextFunc;
				}
			}
			if (matchFuncId) {
				throwError("Ambiguous call " + func->name);
			}
			matchFuncId = funcId;
		nextFunc:;
		}
	}

	if (!matchFuncId) {
		throwError("Cannot find function has arguments " +
		           classDeclaration->getName(in_data));
	}

	funcId = *matchFuncId;
	auto func = compile.functions[funcId];

	if (!(func->functionFlags & FunctionFlags::FUNC_IS_STATIC)) {
		if (!caller) {
			throwError(
			    "Expected static function but found non static function: " +
			    compile.functions[funcId]->name);
		}
		objects.push_back(caller);
		return;
	}
}

void FunctionAccessNode::putBytecodes(in_func,
                                      std::vector<uint8_t> &bytecodes) {
	auto func = compile.functions[funcId];
	if (func->functionFlags & FunctionFlags::FUNC_IS_STATIC) {
		bytecodes.push_back(Opcode::CREATE_FUNCTION_OBJECT);
		put_opcode_u32(bytecodes, funcId);
		put_opcode_u32(bytecodes, 0);
	} else {
		for (auto obj : objects) {
			obj->putBytecodes(in_data, bytecodes);
		}
		bytecodes.push_back(Opcode::CREATE_FUNCTION_OBJECT);
		put_opcode_u32(bytecodes, funcId);
		put_opcode_u32(bytecodes, objects.size());
	}
}

} // namespace AutoLang

#endif
#ifndef FUNCTION_ACCESS_NODE_CPP
#define FUNCTION_ACCESS_NODE_CPP

#include "Node.hpp"
#include "frontend/parser/ParserContext.hpp"
#include "shared/ClassFlags.hpp"

namespace AutoLang {

void FunctionAccessNode::optimize(in_func) {
	if (!classDeclaration) {
		if (count == 1 && funcs[0]->size() == 1) {
			funcId = (*funcs[0])[0];
			auto func = compile.functions[funcId];
			auto funcInfo = context.functionInfo[funcId];
			classDeclaration = context.classDeclarationAllocator.push();
			classDeclaration->baseClassLexerStringId = nameId;
			classDeclaration->isGeneric = true;
			classDeclaration->classId = DefaultClass::functionClassId;
			classDeclaration->mode = mode;
			classDeclaration->line = line;
			classDeclaration->nullable = false;
			classDeclaration->isGenericDeclaration = false;
			classDeclaration->mustInference = false;
			classDeclaration->inputClassId.reserve(
			    funcInfo->parameter->parameters.size() + 1);
			if (func->returnId == DefaultClass::functionClassId) {
				classDeclaration->inputClassId.push_back(funcInfo->returnClass);
			} else {
				auto returnClassDeclaration =
				    context.classDeclarationAllocator.push();
				returnClassDeclaration->baseClassLexerStringId = nameId;
				returnClassDeclaration->isGeneric = true;
				returnClassDeclaration->classId = func->returnId;
				returnClassDeclaration->mode = mode;
				returnClassDeclaration->line = line;
				returnClassDeclaration->nullable =
				    func->functionFlags & FunctionFlags::FUNC_RETURN_NULLABLE;
				returnClassDeclaration->isGenericDeclaration = false;
				returnClassDeclaration->mustInference = false;
				classDeclaration->inputClassId.push_back(
				    returnClassDeclaration);
			}
			for (auto declaration : funcInfo->parameter->parameters) {
				if (!declaration->classDeclaration) {
					auto newClassDeclaration =
					    context.classDeclarationAllocator.push();
					newClassDeclaration->baseClassLexerStringId = nameId;
					newClassDeclaration->isGeneric = true;
					newClassDeclaration->classId = declaration->classId;
					newClassDeclaration->mode = mode;
					newClassDeclaration->line = line;
					newClassDeclaration->nullable =
					    func->functionFlags &
					    FunctionFlags::FUNC_RETURN_NULLABLE;
					newClassDeclaration->isGenericDeclaration = false;
					newClassDeclaration->mustInference = false;
				} else {
					classDeclaration->inputClassId.push_back(
					    declaration->classDeclaration);
				}
			}
			goto matched;
		} else {
			std::string found;
			bool isFirst1 = true;
			for (int j = 0; j < count; ++j) {
				auto &vecs = *funcs[j];
				if (vecs.empty()) {
					found = "EMPTY";
					printDebug("Empty");
				}
				for (auto v : vecs) {
					if (isFirst1) {
						isFirst1 = false;
					} else {
						found += "\n";
					}
					found += compile.functions[v]->toString(compile);
				}
			}
			throwError("Ambiguous reference to: '" +
			           context.lexerString[nameId] + "'\nFound: " + found);
		}
	}

	if (*classDeclaration->classId != DefaultClass::functionClassId) {
		throwError("Expected Function but " +
		           compile.classes[*classDeclaration->classId]->name +
		           " found");
	}

	{

		std::optional<FunctionId> matchFuncId;

		for (int i = 0; i < count; ++i) {
			auto vecs = funcs[i];
			for (auto funcId : *vecs) {
				auto func = compile.functions[funcId];
				if (func->returnId !=
				    *classDeclaration->inputClassId[0]->classId) {
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
			throwError(
			    "Cannot find function name: " + context.lexerString[nameId] +
			    " has arguments " + classDeclaration->getName(in_data));
		}

		funcId = *matchFuncId;
	}

matched:;
	auto func = compile.functions[funcId];

	if (!(func->functionFlags & FunctionFlags::FUNC_IS_STATIC)) {
		if (!caller) {
			throwError(
			    "Expected static function but found non static function: " +
			    compile.functions[funcId]->name);
		}
		object = caller;
		return;
	}
}

void FunctionAccessNode::putBytecodes(in_func,
                                      std::vector<uint8_t> &bytecodes) {
	auto func = compile.functions[funcId];
	if (caller) {
		caller->putBytecodesIfMustBeCalled(in_data, bytecodes);
	}
	auto funcInfo = context.functionInfo[funcId];
	if (object) {
		object->putBytecodes(in_data, bytecodes);
	}
	if (func->functionFlags & FunctionFlags::FUNC_IS_VIRTUAL) {
		bytecodes.push_back(Opcode::CREATE_FUNCTION_OBJECT_FROM_VTABLE);
		put_opcode_u32(bytecodes, funcInfo->virtualPosition);
	} else {
		bytecodes.push_back(Opcode::CREATE_FUNCTION_OBJECT);
		put_opcode_u32(bytecodes, funcId);
		put_opcode_u32(bytecodes, object ? 1 : 0);
	}
}

void FunctionAccessNode::rewrite(in_func, uint8_t *bytecodes) {
	if (caller) {
		caller->rewrite(in_data, bytecodes);
	}
	if (object) {
		object->rewrite(in_data, bytecodes);
	}
}

} // namespace AutoLang

#endif
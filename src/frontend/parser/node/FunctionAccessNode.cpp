#ifndef FUNCTION_ACCESS_NODE_CPP
#define FUNCTION_ACCESS_NODE_CPP

#include "Node.hpp"
#include "frontend/parser/ParserContext.hpp"
#include "shared/ClassFlags.hpp"

namespace Autolang {

ExprNode *FunctionAccessNode::resolve(in_func) {
	if (caller) {
		caller = static_cast<HasClassIdNode *>(caller->resolve(in_data));
	}
	return this;
}

ExprNode *FunctionAccessNode::copy(in_func) {
	auto newCaller =
	    caller ? static_cast<HasClassIdNode *>(caller->copy(in_data)) : nullptr;
	auto node = context.functionAccessPool.push(line, newCaller, nameId);
	node->funcId = funcId;
	node->classDeclaration = classDeclaration;
	node->classId = classId;
	node->object = object;
	node->count = count;
	node->funcs[0] = funcs[0];
	node->funcs[1] = funcs[1];
	return node;
}

ExprNode *FunctionAccessNode::optimize(in_func) {
	if (caller) {
		caller = static_cast<HasClassIdNode *>(caller->optimize(in_data));
	}
	if (funcId) {
		return this;
	}

	if (count == 0) {
		if (caller) {
			ClassId cId = caller->classId;
			if (cId != DefaultClass::nullClassId && cId < context.classInfo.size() &&
			    context.classInfo[cId]) {
				auto classInfo = context.classInfo[cId];
				auto it = classInfo->allFunction.find(nameId);
				if (it != classInfo->allFunction.end()) {
					funcs[count++] = &it->second;
				}
			}
			if (count == 0) {
				std::string className =
				    (caller->classId < compile.classes.size() &&
				     compile.classes[caller->classId])
				        ? compile.classes[caller->classId]->getName(compile)
				        : "Unknown";
				throwError("Cannot find member function '" +
				           context.lexerString[nameId] + "' in class '" +
				           className +
				           "'\nHint: Verify member function name spelling or accessibility.");
			}
		} else {
			if (context.currentClassId) {
				auto classInfo = context.getCurrentClassInfo(in_data);
				auto it = classInfo->allFunction.find(nameId);
				if (it != classInfo->allFunction.end()) {
					funcs[count++] = &it->second;
					if (context.currentFunctionId != context.mainFunctionId &&
					    !(context.getCurrentFunction(in_data)->functionFlags &
					      FunctionFlags::FUNC_IS_STATIC)) {
						caller = context.varPool.push(
						    line, classInfo->declarationThis, false, false);
					}
				}
			}
			auto it = context.globalFunction.find(nameId);
			if (it != context.globalFunction.end()) {
				funcs[count++] = &it->second;
			}
			if (count == 0) {
				throwError("Cannot find function '" +
				           context.lexerString[nameId] +
				           "'\nHint: Check function name spelling, ensure it is in scope or declared before usage.");
			}
		}
	}

	if (!classDeclaration) {
		if (count == 1 && funcs[0]->size() == 1) {
			funcId = (*funcs[0])[0];
			auto func = compile.functions[*funcId];
			auto funcInfo = context.functionInfo[*funcId];
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
			    funcInfo->parameter->parameters.size() + 2);
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
			bool isBoundMember = caller &&
			                     caller->kind != NodeType::CLASS_ACCESS &&
			                     !(func->functionFlags & FunctionFlags::FUNC_IS_STATIC);
			size_t startParamIdx = isBoundMember ? 1 : 0;
			for (size_t p = startParamIdx; p < funcInfo->parameter->parameters.size(); ++p) {
				auto declaration = funcInfo->parameter->parameters[p];
				if (!declaration->classDeclaration) {
					auto newClassDeclaration =
					    context.classDeclarationAllocator.push();
					newClassDeclaration->baseClassLexerStringId = declaration->baseName;
					newClassDeclaration->isGeneric = true;
					newClassDeclaration->classId = declaration->classId;
					newClassDeclaration->mode = mode;
					newClassDeclaration->line = line;
					newClassDeclaration->nullable = declaration->nullable;
					newClassDeclaration->isGenericDeclaration = false;
					newClassDeclaration->mustInference = false;
					classDeclaration->inputClassId.push_back(newClassDeclaration);
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
					found += context.functionInfo[v]->toString(in_data);
				}
			}
			throwError("Ambiguous reference to: '" +
			           context.lexerString[nameId] + "'\nFound: " + found +
			           "\nHint: Multiple functions match this identifier. Explicitly specify function parameter types or refine the type signature.");
		}
	}

	if (*classDeclaration->classId != DefaultClass::functionClassId) {
		throwError("Expected Function type, but '" +
		           compile.classes[*classDeclaration->classId]->getName(compile) +
		           "' found\nHint: Ensure the expression resolves to a Function type before referencing or invoking it.");
	}

	{
		std::optional<FunctionId> matchFuncId;

		for (int idx = 0; idx < count; ++idx) {
			auto vecs = funcs[idx];
			for (auto candidateId : *vecs) {
				auto func = compile.functions[candidateId];
				if (func->returnId !=
				    *classDeclaration->inputClassId[0]->classId) {
					continue;
				}
				bool isStatic = (func->functionFlags & FunctionFlags::FUNC_IS_STATIC) != 0;
				bool isUnbound = (caller && caller->kind == NodeType::CLASS_ACCESS && !isStatic);

				if (isUnbound) {
					if (classDeclaration->inputClassId.size() - 1 != func->argSize) {
						continue;
					}
					if (func->args[0] != *classDeclaration->inputClassId[1]->classId) {
						continue;
					}
					bool argsMatch = true;
					for (int p = 1; p < func->argSize; ++p) {
						if (func->args[p] != *classDeclaration->inputClassId[p + 1]->classId) {
							argsMatch = false;
							break;
						}
					}
					if (!argsMatch) continue;
				} else if (!isStatic) {
					// Bound member reference
					if (classDeclaration->inputClassId.size() - 1 != func->argSize - 1) {
						continue;
					}
					bool argsMatch = true;
					for (int p = 1; p < func->argSize; ++p) {
						if (func->args[p] != *classDeclaration->inputClassId[p]->classId) {
							argsMatch = false;
							break;
						}
					}
					if (!argsMatch) continue;
				} else {
					// Static or global function
					if (classDeclaration->inputClassId.size() - 1 != func->argSize) {
						continue;
					}
					bool argsMatch = true;
					for (int p = 0; p < func->argSize; ++p) {
						if (func->args[p] != *classDeclaration->inputClassId[p + 1]->classId) {
							argsMatch = false;
							break;
						}
					}
					if (!argsMatch) continue;
				}

				if (matchFuncId) {
					throwError("Ambiguous function call: overload conflict for '" + func->getName(compile) +
					           "'\nHint: Multiple overloaded functions match the targeted signature. Explicitly specify types or cast arguments to resolve conflict.");
				}
				matchFuncId = candidateId;
			}
		}

		if (!matchFuncId) {
			throwError(
			    "Cannot find function '" + context.lexerString[nameId] +
			    "' matching signature: " + classDeclaration->getName(in_data) +
			    "\nHint: Verify the function name spelling, argument count, parameter types, and return type against available definitions.");
		}

		funcId = *matchFuncId;
	}

matched:;
	auto func = compile.functions[*funcId];

	if (!(func->functionFlags & FunctionFlags::FUNC_IS_STATIC)) {
		if (caller && caller->kind != NodeType::CLASS_ACCESS) {
			object = caller;
		} else {
			object = nullptr;
		}
		classId = DefaultClass::functionClassId;
		return this;
	}
	classId = DefaultClass::functionClassId;
	return this;
}

void FunctionAccessNode::putBytecodes(in_func,
                                      std::vector<uint8_t> &bytecodes) {
	auto func = compile.functions[*funcId];
	if (caller) {
		caller->putBytecodesIfMustBeCalled(in_data, bytecodes);
	}
	auto funcInfo = context.functionInfo[*funcId];
	if (object) {
		object->putBytecodes(in_data, bytecodes);
	}
	if (object && (func->functionFlags & FunctionFlags::FUNC_IS_VIRTUAL)) {
		bytecodes.push_back(Opcode::CREATE_FUNCTION_OBJECT_FROM_VTABLE);
		put_opcode_u32(bytecodes, funcInfo->virtualPosition);
	} else {
		bytecodes.push_back(Opcode::CREATE_FUNCTION_OBJECT);
		put_opcode_u32(bytecodes, *funcId);
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

} // namespace Autolang

#endif
#ifndef CREATE_CLOSURE_NODE_CPP
#define CREATE_CLOSURE_NODE_CPP

#include "Node.hpp"
#include "frontend/parser/ParserContext.hpp"

namespace Autolang {

ExprNode *CreateClosureNode::resolve(in_func) {
	for (auto *&object : objects) {
		object = static_cast<HasClassIdNode *>(object->resolve(in_data));
	}
	return this;
}

ExprNode *CreateClosureNode::optimize(in_func) {
	if (funcId) {
		return this;
	}
	std::string name = "Closure@" + std::to_string(context.closureCount++);
	LexerStringId nameId = context.createLexerStringIfNotExists(name);
	CreateFuncNode *node = context.newFunctions.push(
	    line, 0, context.currentClassId, nameId, nullptr, parameter,
	    FUNC_IS_STATIC | FUNC_SKIP_LOAD | FUNC_PRIVATE);
	node->pushFunction<false>(in_data);
	funcId = node->id;
	auto func = compile.functions[*funcId];
	auto funcInfo = context.functionInfo[*funcId];
	funcInfo->body.nodes = std::move(body.nodes);
	funcInfo->body.line = line;
	if (mustInfer) {
		for (int i = 1; i < classDeclaration->inputClassId.size(); ++i) {
			if (classDeclaration->inputClassId[i])
				continue;
			// std::string message = "Closure (";
			// int j = parameter->parameters.size() + 1 -
			//         classDeclaration->inputClassId.size();
			// for (; j < parameter->parameters.size(); ++j) {
			// 	auto declaration = parameter->parameters[j];
			// 	message += declaration->getName(compile);
			// 	if (declaration->classDeclaration) {
			// 		message +=
			// 		    ": " + declaration->classDeclaration->getName(in_data);
			// 	}
			// 	if (j < parameter->parameters.size() - 1) {
			// 		message += ", \n";
			// 	} else {
			// 		message += ")";
			// 	}
			// }
			auto currentParameter =
			    parameter->parameters[parameter->parameters.size() + i -
			                          classDeclaration->inputClassId.size()];
			throwError(std::string("Cannot infer type for parameter: '") +
			           std::string(currentParameter->name) +
			           "'\nNote: Inference failed because the closure lacks an "
			           "explicit "
			           "type and no expected context was found.\nHint: Annotate parameter types explicitly (e.g. (x: Int) => ...) or pass/assign the closure to a typed target.");
		}
	}
	if (declarationThis) {
		// std::cerr << this << " " << name << "\n";
		parameter->parameters.insert(parameter->parameters.begin(),
		                             declarationThis);
		objects.insert(objects.begin(),
		               context.varPool.push(declarationThis->line,
		                                    declarationThis, false, false));
		++declarationCount;
	}
	funcInfo->declaration = declarationCount;
	for (auto &obj : objects) {
		obj = static_cast<HasClassIdNode *>(obj->optimize(in_data));
	}
	auto lastMustReturnValueNode = context.mustReturnValueNode;
	context.mustReturnValueNode = this;
	if (!objects.empty()) {
		delete[] func->args;
		func->args = new ClassId[parameter->parameters.size()];
	}
	for (int i = 0; i < parameter->parameters.size(); ++i) {
		func->args[i] = parameter->parameters[i]->classId;
	}
	auto lastClosureNode = context.currentClosureNode;
	context.currentClosureNode = this;
	if (!classDeclaration->inputClassId.empty() && classDeclaration->inputClassId[0] &&
	    classDeclaration->inputClassId[0]->classId) {
		func->returnId = *classDeclaration->inputClassId[0]->classId;
	}
	funcInfo->body.resolve(in_data);
	funcInfo->body.optimize(in_data);
	context.currentClosureNode = lastClosureNode;
	context.mustReturnValueNode = lastMustReturnValueNode;
	if (!classDeclaration->inputClassId.empty() && classDeclaration->inputClassId[0] &&
	    classDeclaration->inputClassId[0]->classId) {
		func->returnId = *classDeclaration->inputClassId[0]->classId;
	} else if (!funcInfo->body.nodes.empty()) {
		auto *lastNode = static_cast<HasClassIdNode *>(funcInfo->body.nodes.back());
		if (lastNode && lastNode->classId != DefaultClass::voidClassId) {
			func->returnId = lastNode->classId;
			if (classDeclaration->inputClassId.empty()) {
				classDeclaration->inputClassId.push_back(nullptr);
			}
			if (classDeclaration->inputClassId[0] == nullptr) {
				if (lastNode->classDeclaration) {
					classDeclaration->inputClassId[0] = lastNode->classDeclaration;
				} else {
					auto retDecl = context.classDeclarationAllocator.push();
					retDecl->classId = lastNode->classId;
					retDecl->nullable = lastNode->isNullable();
					retDecl->baseClassLexerStringId =
					    context.createLexerStringIfNotExists(
					        compile.classes[lastNode->classId]->getName(compile));
					classDeclaration->inputClassId[0] = retDecl;
				}
			}
		}
	}
	if (func->returnId != DefaultClass::voidClassId) {
	}
	// std::cerr<<funcId<<"\n";
	// std::cerr << classDeclaration->getName(in_data) << "\n";
	return this;
}

void CreateClosureNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
	if (!funcId) {
		std::cerr << "Not found " << this << "\n";
		return;
	}
	auto func = compile.functions[*funcId];
	func->argSize = parameter->parameters.size();
	auto funcInfo = context.functionInfo[*funcId];
	for (auto obj : objects) {
		switch (obj->kind) {
			case NodeType::VAR: {
				auto vn = static_cast<VarNode *>(obj);
				vn->isStore = false;
				vn->isCaptureRawBox = true;
				break;
			}
			case NodeType::GET_PROP: {
				static_cast<GetPropNode *>(obj)->isStore = false;
				break;
			}
			default:
				break;
		}
		obj->putBytecodes(in_data, bytecodes);
	}
	bytecodes.push_back(Opcode::CREATE_FUNCTION_OBJECT);
	put_opcode_u32(bytecodes, *funcId);
	put_opcode_u32(bytecodes, objects.size());
	uint32_t delta = objects.size();
	// std::cerr << "COUNT: " << func->argSize << " " << delta << "\n";
	maxDeclaration += delta;
	declarationCount += delta;
	func->maxDeclaration = maxDeclaration;
	for (auto declaration : newDeclaration) {
		// std::cerr << declaration->getName(compile) << " " << declaration->id
		// << "\n";
		declaration->id += delta;
	}
	auto listOffset =
	    new DeclarationOffset[funcInfo->parameter->parameters.size()];
	for (int i = 0; i < funcInfo->parameter->parameters.size(); ++i) {
		auto declaration = funcInfo->parameter->parameters[i];
		listOffset[i] = declaration->id;
		declaration->id = i;
		func->args[i] = declaration->classId;
	}
	auto lastMustReturnValueNode = context.mustReturnValueNode;
	context.mustReturnValueNode = this;
	auto lastCurrentBytecodePos = context.currentBytecodePos;
	auto lastCurrentAllOpcodeLine = context.currentAllOpcodeLine;
	context.currentBytecodePos = 0;
	context.currentOpcodeIndex = 0;
	context.currentAllOpcodeLine = &currentOpcodeLine;
	context.currentFunctionId = func->id;
	funcInfo->body.putBytecodes(in_data, currentBytecodes);
	context.currentBytecodePos = lastCurrentBytecodePos;
	context.mustReturnValueNode = lastMustReturnValueNode;
	context.currentAllOpcodeLine = lastCurrentAllOpcodeLine;
	for (int i = 0; i < funcInfo->parameter->parameters.size(); ++i) {
		auto declaration = funcInfo->parameter->parameters[i];
		declaration->id = listOffset[i];
	}
	delete[] listOffset;
}

void CreateClosureNode::rewrite(in_func, uint8_t *bytecodes) {
	auto funcInfo = context.functionInfo[*funcId];
	for (auto obj : objects) {
		obj->rewrite(in_data, bytecodes);
	}
	funcInfo->body.rewrite(in_data, currentBytecodes.data());
}

void CreateClosureNode::inferFrom(in_func, ClassDeclaration *from) {
	if (from->classId != DefaultClass::functionClassId) {
		throwError("Cannot cast Function to '" + from->getName(in_data) +
		           "'\nHint: Closures can only be assigned or cast to Function or compatible signature types.");
	}
	// if (canCast(classDeclaration))
	if (classDeclaration->inputClassId.size() != from->inputClassId.size()) {
		throwError("Closure expects " +
		           std::to_string(classDeclaration->inputClassId.size() - 1) +
		           " parameter(s) but " +
		           std::to_string(from->inputClassId.size() - 1) +
		           " were given\nHint: Ensure the closure parameter count matches the target function signature.");
	}
	for (int i = 0; i < classDeclaration->inputClassId.size(); ++i) {
		auto fromClass = classDeclaration->inputClassId[i];
		auto toClass = from->inputClassId[i];
		if (toClass && !toClass->classId) {
			toClass->template load<true>(in_data);
		}
		if (fromClass && !fromClass->classId) {
			fromClass->template load<true>(in_data);
		}
		if (fromClass && toClass) {
			if (fromClass->classId && toClass->classId && *fromClass->classId == *toClass->classId) {
				continue;
			}
			if (!fromClass->isSame(toClass)) {
				if (i == 0) {
					if (canCast(in_data, fromClass, toClass)) {
						classDeclaration->inputClassId[0] = toClass;
						continue;
					}
					throwError("Cannot cast '" + fromClass->getName(in_data) +
					           "' to '" + toClass->getName(in_data) +
					           "'\nHint: The closure return type must be assignable or castable to the expected return type.");
				}
				auto currentParameter =
				    parameter
				        ->parameters[parameter->parameters.size() + i -
 				                     classDeclaration->inputClassId.size()];
				throwError(std::string("Parameter '") + std::string(currentParameter->name) +
				           "' expected type '" + toClass->getName(in_data) +
				           "' but '" + fromClass->getName(in_data) +
				           "' found\nHint: Align the parameter type in the closure with the expected parameter type of the target signature.");
			}
			continue;
		}
		if (i == 0) {
			classDeclaration->inputClassId[0] = toClass;
			continue;
		}
		auto currentParameter =
		    parameter->parameters[parameter->parameters.size() + i -
		                          classDeclaration->inputClassId.size()];
		classDeclaration->inputClassId[i] = toClass;
		currentParameter->classDeclaration = toClass;
		currentParameter->classId = *toClass->classId;
		currentParameter->nullable = toClass->nullable;
	}
	mustInfer = false;
}

ExprNode *CreateClosureNode::copy(in_func) {
	std::vector<HasClassIdNode *> newObjects;
	newObjects.reserve(objects.size());
	for (auto object : objects) {
		newObjects.push_back(
		    static_cast<HasClassIdNode *>(object->copy(in_data)));
	}
	auto newParam = parameter->copy(in_data);
	auto *newNode =
	    context.createClosurePool.push(line, newParam);
	if (!classDeclaration->classId) {
		classDeclaration->template load<true>(in_data);
		if (!classDeclaration->classId) {
			throwError("Bug: Unresolved class " +
			           classDeclaration->getName(in_data) +
			           "\nHint: Internal compiler error - class declaration was not resolved before node duplication.");
		}
	}
	newNode->objects = std::move(newObjects);
	newNode->classDeclaration = classDeclaration->copy(in_data);
	for (size_t p = 0; p < newParam->parameters.size(); ++p) {
		if (p + 1 < newNode->classDeclaration->inputClassId.size()) {
			newNode->classDeclaration->inputClassId[p + 1] =
			    newParam->parameters[p]->classDeclaration;
		}
	}
	newNode->scopes = scopes;
	if (!newNode->scopes.scopes.empty()) {
		for (auto *param : newParam->parameters) {
			newNode->scopes.back()[param->baseName] = param;
		}
	}
	newNode->declarationCount = declarationCount;
	newNode->maxDeclaration = maxDeclaration;
	newNode->newDeclaration = newDeclaration;
	newNode->parameterCountFirstTime = parameterCountFirstTime;
	if (declarationThis) {
		newNode->declarationThis =
		    static_cast<DeclarationNode *>(declarationThis->copy(in_data));
	}
	newNode->body.nodes.reserve(body.nodes.size());
	for (auto node : body.nodes) {
		newNode->body.nodes.push_back(
		    static_cast<HasClassIdNode *>(node->copy(in_data)));
	}
	context.allClosureNode.push_back(newNode);
	return newNode;
}

bool CreateClosureNode::tryInferReturnType(in_func, ClassDeclaration *expectedFuncType) {
	if (!classDeclaration || classDeclaration->inputClassId.empty()) return false;
	if (classDeclaration->inputClassId[0] && classDeclaration->inputClassId[0]->classId.has_value()) {
		return true;
	}
	if (expectedFuncType && expectedFuncType->inputClassId.size() == classDeclaration->inputClassId.size()) {
		for (size_t p = 0; p < parameter->parameters.size(); ++p) {
			auto *param = parameter->parameters[p];
			if (!param) continue;
			if (!param->classDeclaration || !param->classDeclaration->classId.has_value()) {
				if (p + 1 < expectedFuncType->inputClassId.size()) {
					auto *expectedParamType = expectedFuncType->inputClassId[p + 1];
					if (expectedParamType && expectedParamType->classId.has_value()) {
						param->classDeclaration = expectedParamType;
						param->classId = *expectedParamType->classId;
						param->nullable = expectedParamType->nullable;
						if (p + 1 < classDeclaration->inputClassId.size()) {
							classDeclaration->inputClassId[p + 1] = expectedParamType;
						}
					}
				}
			}
		}
	}
	for (auto *param : parameter->parameters) {
		if (!param) return false;
		if (!param->classDeclaration) return false;
		if (!param->classDeclaration->classId.has_value()) {
			param->classDeclaration->template load<false>(in_data);
			if (!param->classDeclaration->classId.has_value()) return false;
		}
		param->classId = *param->classDeclaration->classId;
	}
	mustInfer = false;
	this->optimize(in_data);
	return classDeclaration->inputClassId[0] && classDeclaration->inputClassId[0]->classId.has_value();
}

} // namespace Autolang

#endif
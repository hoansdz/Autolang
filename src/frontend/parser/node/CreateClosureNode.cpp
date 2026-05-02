#ifndef CREATE_CLOSURE_NODE_CPP
#define CREATE_CLOSURE_NODE_CPP

#include "Node.hpp"
#include "frontend/parser/ParserContext.hpp"

namespace AutoLang {

ExprNode *CreateClosureNode::resolve(in_func) {
	for (auto *&object : objects) {
		object = static_cast<HasClassIdNode *>(object->resolve(in_data));
	}
	return this;
}

void CreateClosureNode::optimize(in_func) {
	std::string name = "Closure@" + std::to_string(context.closureCount++);
	LexerStringId nameId = context.createLexerStringIfNotExists(name);
	CreateFuncNode *node = context.newFunctions.push(
	    line, context.currentClassId, nameId, nullptr, parameter,
	    FUNC_IS_STATIC | FUNC_SKIP_LOAD | FUNC_PRIVATE);
	node->pushFunction(in_data);
	funcId = node->id;
	auto func = compile.functions[funcId];
	auto funcInfo = context.functionInfo[funcId];
	funcInfo->body.nodes = std::move(body.nodes);
	if (mustInfer) {
		for (int i = 1; i < classDeclaration->inputClassId.size(); ++i) {
			if (classDeclaration->inputClassId[i])
				continue;
			// std::string message = "Closure (";
			// int j = parameter->parameters.size() + 1 -
			//         classDeclaration->inputClassId.size();
			// for (; j < parameter->parameters.size(); ++j) {
			// 	auto declaration = parameter->parameters[j];
			// 	message += declaration->name;
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
			throwError("Cannot infer type for parameter: '" +
			           currentParameter->name +
			           "'\nNote: Inference failed because the closure lacks an "
			           "explicit "
			           "type and no expected context was found.");
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
	for (auto obj : objects) {
		obj->optimize(in_data);
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
	funcInfo->body.resolve(in_data);
	funcInfo->body.optimize(in_data);
	context.mustReturnValueNode = lastMustReturnValueNode;
	func->returnId = *classDeclaration->inputClassId[0]->classId;
	// std::cerr<<funcId<<"\n";
	// std::cerr << classDeclaration->getName(in_data) << "\n";
}

void CreateClosureNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
	auto func = compile.functions[funcId];
	func->argSize = parameter->parameters.size();
	auto funcInfo = context.functionInfo[funcId];
	for (auto obj : objects) {
		switch (obj->kind) {
			case NodeType::VAR: {
				static_cast<VarNode *>(obj)->isStore = false;
				break;
			}
			case NodeType::GET_PROP: {
				static_cast<GetPropNode *>(obj)->isStore = false;
				break;
			}
		}
		obj->putBytecodes(in_data, bytecodes);
	}
	bytecodes.push_back(Opcode::CREATE_FUNCTION_OBJECT);
	put_opcode_u32(bytecodes, funcId);
	put_opcode_u32(bytecodes, objects.size());
	uint32_t delta = objects.size();
	// std::cerr << "COUNT: " << func->argSize << " " << delta << "\n";
	maxDeclaration += delta;
	declarationCount += delta;
	func->maxDeclaration = maxDeclaration;
	for (auto declaration : newDeclaration) {
		// std::cerr << declaration->name << " " << declaration->id << "\n";
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
	context.currentBytecodePos = 0;
	funcInfo->body.putBytecodes(in_data, currentBytecodes);
	context.currentBytecodePos = lastCurrentBytecodePos;
	context.mustReturnValueNode = lastMustReturnValueNode;
	for (int i = 0; i < funcInfo->parameter->parameters.size(); ++i) {
		auto declaration = funcInfo->parameter->parameters[i];
		declaration->id = listOffset[i];
	}
	delete[] listOffset;
}

void CreateClosureNode::rewrite(in_func, uint8_t *bytecodes) {
	auto funcInfo = context.functionInfo[funcId];
	for (auto obj : objects) {
		obj->rewrite(in_data, bytecodes);
	}
	funcInfo->body.rewrite(in_data, currentBytecodes.data());
}

void CreateClosureNode::inferFrom(in_func, ClassDeclaration *from) {
	if (from->classId != DefaultClass::functionClassId) {
		throwError("Cannot cast Function to " + from->getName(in_data) + "");
	}
	// if (canCast(classDeclaration))
	if (classDeclaration->inputClassId.size() != from->inputClassId.size()) {
		throwError("Closure expects " +
		           std::to_string(classDeclaration->inputClassId.size()) +
		           " type argument but " +
		           std::to_string(from->inputClassId.size()) + " were given");
	}
	for (int i = 0; i < classDeclaration->inputClassId.size(); ++i) {
		auto fromClass = classDeclaration->inputClassId[i];
		auto toClass = from->inputClassId[i];
		if (fromClass) {
			if (!fromClass->isSame(toClass)) {
				if (i == 0) {
					if (canCast(in_data, fromClass, toClass)) {
						classDeclaration->inputClassId[0] =
						    toClass->inputClassId[0];
						continue;
					}
					throwError("Cannot cast '" + fromClass->getName(in_data) +
					           "' to '" + toClass->getName(in_data) + "'");
				}
				auto currentParameter =
				    parameter
				        ->parameters[parameter->parameters.size() + i -
				                     classDeclaration->inputClassId.size()];
				throwError("Parameter '" + currentParameter->name +
				           "' expected type '" + fromClass->getName(in_data) +
				           "' but '" + toClass->getName(in_data) + "' found");
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
	auto *newNode =
	    context.createClosurePool.push(line, parameter->copy(in_data));
	if (!classDeclaration->classId) {
		classDeclaration->load<true>(in_data);
		if (!classDeclaration->classId) {
			throwError("Bug: Unresolved class " +
			           classDeclaration->getName(in_data));
		}
	}
	newNode->objects = std::move(newObjects);
	newNode->classDeclaration = classDeclaration;
	newNode->scopes = scopes;
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
	return newNode;
}

} // namespace AutoLang

#endif
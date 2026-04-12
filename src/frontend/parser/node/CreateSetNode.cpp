#ifndef CREATE_SET_NODE_CPP
#define CREATE_SET_NODE_CPP

#include "Node.hpp"
#include "frontend/parser/ParserContext.hpp"

namespace AutoLang {

ExprNode *CreateSetNode::resolve(in_func) {
	for (auto *&value : values) {
		value = static_cast<HasClassIdNode *>(value->resolve(in_data));
	}
	return this;
}

void CreateSetNode::optimize(in_func) {
	if (classDeclaration) {
		classId = *classDeclaration->classId;
	}
	if (classId == DefaultClass::nullClassId) {
		throwError("Type mismatch in set initialization");
	}
	auto classInfo = context.classInfo[classId];
	if (classInfo->baseClassId != DefaultClass::setClassId) {
		throwError("Type mismatch, expected Set<> but '" +
		           compile.classes[classId]->name + "' found");
	}
	auto genericType = classInfo->genericTypeId[0];
	auto valueMustBeClassId = *genericType->classId;
	for (auto *&value : values) {
		switch (value->kind) {
			case NodeType::CREATE_ARRAY: {
				static_cast<CreateArrayNode *>(value)->classId =
				    valueMustBeClassId;
				break;
			}
			case NodeType::CREATE_SET: {
				static_cast<CreateSetNode *>(value)->classId =
				    valueMustBeClassId;
				break;
			}
		}
		value->optimize(in_data);
		if (value->classId == valueMustBeClassId ||
		    compile.classes[value->classId]->inheritance.get(
		        valueMustBeClassId)) {
			continue;
		}
		switch (value->classId) {
			case DefaultClass::intClassId: {
				if (valueMustBeClassId == DefaultClass::floatClassId) {
					value = context.castPool.push(value,
					                              DefaultClass::floatClassId);
					continue;
				}
				break;
			}
			case DefaultClass::nullClassId: {
				if (genericType->nullable) {
					continue;
				}
				throwError("Value in Set must non null");
			}
		}
		throwError("Cannot cast " + compile.classes[value->classId]->name +
		           " to " + compile.classes[valueMustBeClassId]->name);
	}
}

void CreateSetNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
	for (auto *value : values) {
		value->putBytecodes(in_data, bytecodes);
	}
	auto classInfo = context.classInfo[classId];
	auto keyId = *classInfo->genericTypeId[0]->classId;
	bytecodes.emplace_back(Opcode::CREATE_SET_OBJECT);
	put_opcode_u32(bytecodes, classId);
	put_opcode_u32(bytecodes, keyId);
	put_opcode_u32(bytecodes, values.size());
}

void CreateSetNode::rewrite(in_func, uint8_t *bytecodes) {
	for (auto *value : values) {
		value->rewrite(in_data, bytecodes);
	}
}

ExprNode *CreateSetNode::copy(in_func) {
	auto *newNode = context.createSetPool.push(line, nullptr,
	                                           std::vector<HasClassIdNode *>());
	for (auto *value : values) {
		newNode->values.push_back(
		    static_cast<HasClassIdNode *>(value->copy(in_data)));
	}
	if (classDeclaration) {
		if (!classDeclaration->classId) {
			classDeclaration->load<true>(in_data);
			if (!classDeclaration->classId) {
				throwError("Bug: DeclarationNode copy: Unresolved class " +
				           classDeclaration->getName(in_data));
			}
		}
		newNode->classId = *classDeclaration->classId;
	}
	return newNode;
}

CreateSetNode::~CreateSetNode() {}

} // namespace AutoLang

#endif
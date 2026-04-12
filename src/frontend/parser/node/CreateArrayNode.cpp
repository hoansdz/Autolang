#ifndef CREATE_ARRAY_NODE_CPP
#define CREATE_ARRAY_NODE_CPP

#include "Node.hpp"
#include "frontend/parser/ParserContext.hpp"

namespace AutoLang {

ExprNode *CreateArrayNode::resolve(in_func) {
	for (auto *&value : values) {
		value = static_cast<HasClassIdNode *>(value->resolve(in_data));
	}
	return this;
}

void CreateArrayNode::optimize(in_func) {
	if (classDeclaration) {
		classId = *classDeclaration->classId;
	}
	if (classId == DefaultClass::nullClassId) {
		throwError("Type mismatch in array initialization");
	}
	auto classInfo = context.classInfo[classId];
	if (classInfo->baseClassId != DefaultClass::arrayClassId) {
		throwError("Type mismatch, expected Array<> but '" +
		           compile.classes[classId]->name + "' found");
	}
	auto genericType = classInfo->genericTypeId[0];
	auto valueMustBeClassId = *genericType->classId;
	for (auto *&value : values) {
		if (value->kind == NodeType::CREATE_ARRAY) {
			static_cast<CreateArrayNode *>(value)->classId = valueMustBeClassId;
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
				throwError("Value in array must non null");
			}
		}
		throwError("Cannot cast " + compile.classes[value->classId]->name +
		           " to " + compile.classes[valueMustBeClassId]->name);
	}
}

void CreateArrayNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
	for (auto *value : values) {
		value->putBytecodes(in_data, bytecodes);
	}
	bytecodes.emplace_back(Opcode::FAST_SAVE_MEMBER);
	put_opcode_u32(bytecodes, classId);
	put_opcode_u32(bytecodes, values.size());
}

void CreateArrayNode::rewrite(in_func, uint8_t *bytecodes) {
	for (auto *value : values) {
		value->rewrite(in_data, bytecodes);
	}
}

ExprNode *CreateArrayNode::copy(in_func) {
	auto *newNode = context.createArrayPool.push(
	    line, nullptr, std::vector<HasClassIdNode *>());
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

CreateArrayNode::~CreateArrayNode() {}

} // namespace AutoLang

#endif
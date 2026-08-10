#ifndef CREATE_ARRAY_NODE_CPP
#define CREATE_ARRAY_NODE_CPP

#include "Node.hpp"
#include "frontend/parser/ParserContext.hpp"

namespace Autolang {

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
		throwError("Cannot infer element type for Array initialization\nHint: Provide an explicit type annotation or add typed elements to the Array.");
	}
	auto clazz = compile.classes[classId];
	auto classInfo = context.classInfo[classId];
	if (clazz->genericBaseClassId != DefaultClass::arrayClassId) {
		throwError("Type mismatch, expected Array<> but '" +
		           compile.classes[classId]->getName(compile) + "' found\nHint: Ensure target variable type is an Array<T> instance.");
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
				throwError("Elements in Array must be non-null\nHint: The Array element type is non-nullable. Use Array<T?> to allow null elements.");
			}
		}
		if (valueMustBeClassId == DefaultClass::anyClassId) {
			continue;
		}
		throwError("Cannot cast " +
		           compile.classes[value->classId]->getName(compile) + " to " +
		           compile.classes[valueMustBeClassId]->getName(compile) +
		           "\nHint: Ensure all elements in the Array match the expected element type or provide an explicit conversion.");
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
			classDeclaration->load<false>(in_data);
			if (!classDeclaration->classId) {
				throwError("Bug: DeclarationNode copy: Unresolved class " +
				           classDeclaration->getName(in_data) +
				           "\nHint: Internal compiler error - class declaration was not resolved before copy.");
			}
			newNode->classId = *classDeclaration->classId;
			classDeclaration->classId = std::nullopt;
		} else {
			newNode->classId = *classDeclaration->classId;
		}
	}
	return newNode;
}

CreateArrayNode::~CreateArrayNode() {}

} // namespace Autolang

#endif
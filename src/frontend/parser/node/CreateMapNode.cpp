#ifndef CREATE_MAP_NODE_CPP
#define CREATE_MAP_NODE_CPP

#include "Node.hpp"
#include "frontend/parser/ParserContext.hpp"

namespace AutoLang {

ExprNode *CreateMapNode::resolve(in_func) {
	for (auto &[key, value] : values) {
		key = static_cast<HasClassIdNode *>(key->resolve(in_data));
		value = static_cast<HasClassIdNode *>(value->resolve(in_data));
	}
	return this;
}

void CreateMapNode::optimize(in_func) {
	if (classDeclaration) {
		classId = *classDeclaration->classId;
	}
	if (classId == DefaultClass::nullClassId) {
		throwError("Type mismatch in map initialization");
	}
	auto classInfo = context.classInfo[classId];
	if (classInfo->baseClassId != DefaultClass::mapClassId) {
		throwError("Type mismatch, expected Map<> but '" +
		           compile.classes[classId]->name + "' found");
	}

	auto keyMustBeClassId = *classInfo->genericTypeId[0]->classId;
	auto valueMustBeClassId = *classInfo->genericTypeId[1]->classId;
	for (auto &[key, value] : values) {
		switch (key->kind) {
			case NodeType::CREATE_ARRAY: {
				static_cast<CreateArrayNode *>(key)->classId =
				    valueMustBeClassId;
				break;
			}
			case NodeType::CREATE_SET: {
				static_cast<CreateSetNode *>(key)->classId = valueMustBeClassId;
				break;
			}
		}
		key->optimize(in_data);
		if (key->classId == keyMustBeClassId ||
		    compile.classes[key->classId]->inheritance.get(keyMustBeClassId)) {
			goto loadValue;
		}
		switch (key->classId) {
			case DefaultClass::intClassId: {
				if (keyMustBeClassId == DefaultClass::floatClassId) {
					key =
					    context.castPool.push(key, DefaultClass::floatClassId);
					goto loadValue;
				}
				break;
			}
			case DefaultClass::nullClassId: {
				if (classInfo->genericTypeId[0]->nullable) {
					goto loadValue;
				}
				throwError("Key in Map must non null");
			}
		}
		throwError("Cannot cast " + compile.classes[key->classId]->name +
		           " to " + compile.classes[keyMustBeClassId]->name);
	loadValue:;
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
				if (classInfo->genericTypeId[1]->nullable) {
					continue;
				}
				throwError("Value in array must non null");
			}
		}
		throwError("Cannot cast " + compile.classes[value->classId]->name +
		           " to " + compile.classes[valueMustBeClassId]->name);
	}
}

void CreateMapNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
	for (auto it = values.rbegin(); it != values.rend(); ++it) {
		it->first->putBytecodes(in_data, bytecodes);
		it->second->putBytecodes(in_data, bytecodes);
	}
	auto classInfo = context.classInfo[classId];
	auto keyId = *classInfo->genericTypeId[0]->classId;
	bytecodes.emplace_back(Opcode::CREATE_MAP_OBJECT);
	put_opcode_u32(bytecodes, classId);
	put_opcode_u32(bytecodes, keyId);
	put_opcode_u32(bytecodes, values.size());
}

void CreateMapNode::rewrite(in_func, uint8_t *bytecodes) {
	for (auto &[key, value] : values) {
		key->rewrite(in_data, bytecodes);
		value->rewrite(in_data, bytecodes);
	}
}

ExprNode *CreateMapNode::copy(in_func) {
	auto *newNode = context.createMapPool.push(
	    line, nullptr,
	    std::vector<std::pair<HasClassIdNode *, HasClassIdNode *>>());
	for (auto &[key, value] : values) {
		newNode->values.push_back(std::make_pair(
		    static_cast<HasClassIdNode *>(key->copy(in_data)),
		    static_cast<HasClassIdNode *>(value->copy(in_data))));
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

CreateMapNode::~CreateMapNode() {}

} // namespace AutoLang

#endif
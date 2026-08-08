#ifndef CREATE_MAP_NODE_CPP
#define CREATE_MAP_NODE_CPP

#include "Node.hpp"
#include "frontend/parser/ParserContext.hpp"

namespace Autolang {

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
		throwError("Cannot infer key/value type for Map initialization");
	}
	auto clazz = compile.classes[classId];
	auto classInfo = context.classInfo[classId];
	if (clazz->genericBaseClassId != DefaultClass::mapClassId) {
		throwError("Type mismatch, expected Map<> but '" +
		           compile.classes[classId]->getName(compile) + "' found");
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
			default:
				break;
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
				throwError("Keys in Map must be non-null");
			}
		}
		if (keyMustBeClassId == DefaultClass::anyClassId) {
			goto loadValue;
		}
		throwError("Cannot cast " + compile.classes[key->classId]->getName(compile) +
		           " to " + compile.classes[keyMustBeClassId]->getName(compile));
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
			default:
				break;
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
				throwError("Values in Map must be non-null");
			}
		}
		if (valueMustBeClassId == DefaultClass::anyClassId) {
			continue;
		}
		throwError("Cannot cast " + compile.classes[value->classId]->getName(compile) +
		           " to " + compile.classes[valueMustBeClassId]->getName(compile));
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
	if (classInfo->genericTypeId[0]->nullable) {
		put_opcode_u32(bytecodes, DefaultClass::anyClassId);
	} else {
		put_opcode_u32(bytecodes, keyId);
	}
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
			classDeclaration->load<false>(in_data);
			if (!classDeclaration->classId) {
				throwError("Bug: DeclarationNode copy: Unresolved class " +
				           classDeclaration->getName(in_data));
			}
			newNode->classId = *classDeclaration->classId;
			classDeclaration->classId = std::nullopt;
		} else {
			newNode->classId = *classDeclaration->classId;
		}
	}
	return newNode;
}

CreateMapNode::~CreateMapNode() {}

} // namespace Autolang

#endif
#ifndef CREATE_MAP_NODE_CPP
#define CREATE_MAP_NODE_CPP

#include "Node.hpp"
#include "frontend/parser/ClassDeclaration.hpp"
#include "frontend/parser/ParserContext.hpp"
#include "shared/DefaultClass.hpp"

namespace Autolang {

ExprNode *CreateMapNode::resolve(in_func) {
	for (auto &[key, value] : values) {
		key = static_cast<HasClassIdNode *>(key->resolve(in_data));
		value = static_cast<HasClassIdNode *>(value->resolve(in_data));
	}
	return this;
}

ExprNode *CreateMapNode::optimize(in_func) {
	if (classDeclaration) {
		classId = *classDeclaration->classId;
	}
	if (classId == DefaultClass::nullClassId) {
		optimizeAndInferenceType(in_data);
		return this;
	}
	auto clazz = compile.classes[classId];
	auto classInfo = context.classInfo[classId];
	if (clazz->genericBaseClassId != DefaultClass::mapClassId) {
		throwError("Type mismatch, expected Map<> but '" +
		           compile.classes[classId]->getName(compile) + "' found\nHint: Ensure target variable type is a Map<K, V> instance.");
	}

	auto keyMustBeClassId = *classInfo->genericTypeId[0]->classId;
	auto valueMustBeClassId = *classInfo->genericTypeId[1]->classId;
	for (auto &[key, value] : values) {
		switch (key->kind) {
			case NodeType::CREATE_ARRAY: {
				if (compile.classes[keyMustBeClassId]->genericBaseClassId == DefaultClass::arrayClassId) {
					static_cast<CreateArrayNode *>(key)->classId = keyMustBeClassId;
				}
				break;
			}
			case NodeType::CREATE_SET: {
				if (compile.classes[keyMustBeClassId]->genericBaseClassId == DefaultClass::setClassId) {
					static_cast<CreateSetNode *>(key)->classId = keyMustBeClassId;
				}
				break;
			}
			case NodeType::CREATE_MAP: {
				if (compile.classes[keyMustBeClassId]->genericBaseClassId == DefaultClass::mapClassId) {
					static_cast<CreateMapNode *>(key)->classId = keyMustBeClassId;
				}
				break;
			}
			default:
				break;
		}
		key = static_cast<HasClassIdNode *>(key->optimize(in_data));
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
				throwError("Keys in Map must be non-null\nHint: The Map key type is non-nullable. Use Map<K?, V> to allow null keys.");
			}
		}
		if (keyMustBeClassId == DefaultClass::anyClassId) {
			goto loadValue;
		}
		throwError("Cannot cast " + compile.classes[key->classId]->getName(compile) +
		           " to " + compile.classes[keyMustBeClassId]->getName(compile) +
		           "\nHint: Ensure Map key expression matches the expected key type or provide an explicit conversion.");
	loadValue:;
		switch (value->kind) {
			case NodeType::CREATE_ARRAY: {
				if (compile.classes[valueMustBeClassId]->genericBaseClassId == DefaultClass::arrayClassId) {
					static_cast<CreateArrayNode *>(value)->classId =
					    valueMustBeClassId;
				}
				break;
			}
			case NodeType::CREATE_SET: {
				if (compile.classes[valueMustBeClassId]->genericBaseClassId == DefaultClass::setClassId) {
					static_cast<CreateSetNode *>(value)->classId =
					    valueMustBeClassId;
				}
				break;
			}
			case NodeType::CREATE_MAP: {
				if (compile.classes[valueMustBeClassId]->genericBaseClassId == DefaultClass::mapClassId) {
					static_cast<CreateMapNode *>(value)->classId =
					    valueMustBeClassId;
				}
				break;
			}
			default:
				break;
		}
		value = static_cast<HasClassIdNode *>(value->optimize(in_data));
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
				throwError("Values in Map must be non-null\nHint: The Map value type is non-nullable. Use Map<K, V?> to allow null values.");
			}
		}
		if (valueMustBeClassId == DefaultClass::anyClassId) {
			continue;
		}
		throwError("Cannot cast " + compile.classes[value->classId]->getName(compile) +
		           " to " + compile.classes[valueMustBeClassId]->getName(compile) +
		           "\nHint: Ensure Map value expression matches the expected value type or provide an explicit conversion.");
	}
	return this;
}

void CreateMapNode::optimizeAndInferenceType(in_func) {
	if (values.empty()) {
		if (canSkipFindType)
			return;
		throwError(
		    "Cannot infer key/value type for Map initialization\nHint: Provide "
		    "an explicit type annotation or add typed entries to the Map.");
		return;
	}
	std::optional<ClassId> keyMustBeClassId;
	std::optional<ClassId> valueMustBeClassId;
	ClassDeclaration *keyClassDeclaration = nullptr;
	ClassDeclaration *valueClassDeclaration = nullptr;
	bool mustReloadKey = false;
	bool mustReloadValue = false;

	for (auto &[key, value] : values) {
		// Optimize Key
		if (key->kind == NodeType::CREATE_ARRAY ||
		    key->kind == NodeType::CREATE_SET ||
		    key->kind == NodeType::CREATE_MAP) {
			if (keyMustBeClassId) {
				key->classDeclaration = keyClassDeclaration;
			} else {
				switch (key->kind) {
					case NodeType::CREATE_ARRAY: {
						static_cast<CreateArrayNode *>(key)->canSkipFindType = true;
						break;
					}
					case NodeType::CREATE_SET: {
						static_cast<CreateSetNode *>(key)->canSkipFindType = true;
						break;
					}
					case NodeType::CREATE_MAP: {
						static_cast<CreateMapNode *>(key)->canSkipFindType = true;
						break;
					}
					default:
						break;
				}
				key = static_cast<HasClassIdNode *>(key->optimize(in_data));
				if (key->classId == DefaultClass::nullClassId) {
					mustReloadKey = true;
					goto handleValue;
				}
				goto doneOptimizeKey;
			}
		}
		key = static_cast<HasClassIdNode *>(key->optimize(in_data));
	doneOptimizeKey:;
		if (!keyMustBeClassId) {
			keyMustBeClassId = key->classId;
			if (key->classDeclaration) {
				keyClassDeclaration = key->classDeclaration;
			} else {
				keyClassDeclaration =
				    context.classDeclarationAllocator.push();
				keyClassDeclaration->inputClassId =
				    std::vector{keyClassDeclaration};
				keyClassDeclaration->line = line;
				keyClassDeclaration->isGeneric = false;
			}
		} else if (key->classId != *keyMustBeClassId &&
		           !compile.classes[key->classId]->inheritance.get(*keyMustBeClassId)) {
			switch (key->classId) {
				case DefaultClass::intClassId: {
					if (keyMustBeClassId == DefaultClass::floatClassId) {
						key = context.castPool.push(key, DefaultClass::floatClassId);
						break;
					}
					goto keyMismatch;
				}
				case DefaultClass::floatClassId: {
					if (*keyMustBeClassId == DefaultClass::intClassId) {
						keyMustBeClassId = DefaultClass::floatClassId;
						key = context.castPool.push(key, DefaultClass::floatClassId);
						mustReloadKey = true;
						break;
					}
					goto keyMismatch;
				}
				default:
				keyMismatch:
					if (keyMustBeClassId != DefaultClass::anyClassId) {
						throwError("Cannot cast " +
						           compile.classes[key->classId]->getName(compile) + " to " +
						           compile.classes[*keyMustBeClassId]->getName(compile) +
						           "\nHint: Ensure all keys in the Map match the expected key type or provide an explicit conversion.");
					}
					break;
			}
		}

	handleValue:;
		// Optimize Value
		if (value->kind == NodeType::CREATE_ARRAY ||
		    value->kind == NodeType::CREATE_SET ||
		    value->kind == NodeType::CREATE_MAP) {
			if (valueMustBeClassId) {
				if (value->kind == NodeType::CREATE_SET &&
				    static_cast<CreateSetNode *>(value)->values.empty() &&
				    compile.classes[*valueMustBeClassId]->genericBaseClassId ==
				        DefaultClass::mapClassId) {
					value = context.createMapPool.push(
					    value->line, nullptr,
					    std::vector<std::pair<HasClassIdNode *,
					                          HasClassIdNode *>>());
				}
				value->classDeclaration = valueClassDeclaration;
			} else {
				switch (value->kind) {
					case NodeType::CREATE_ARRAY: {
						static_cast<CreateArrayNode *>(value)->canSkipFindType = true;
						break;
					}
					case NodeType::CREATE_SET: {
						static_cast<CreateSetNode *>(value)->canSkipFindType = true;
						break;
					}
					case NodeType::CREATE_MAP: {
						static_cast<CreateMapNode *>(value)->canSkipFindType = true;
						break;
					}
					default:
						break;
				}
				value = static_cast<HasClassIdNode *>(value->optimize(in_data));
				if (value->classId == DefaultClass::nullClassId) {
					mustReloadValue = true;
					continue;
				}
				goto doneOptimizeValue;
			}
		}
		value = static_cast<HasClassIdNode *>(value->optimize(in_data));
	doneOptimizeValue:;
		if (!valueMustBeClassId) {
			valueMustBeClassId = value->classId;
			if (value->classDeclaration) {
				valueClassDeclaration = value->classDeclaration;
			} else {
				valueClassDeclaration =
				    context.classDeclarationAllocator.push();
				valueClassDeclaration->inputClassId =
				    std::vector{valueClassDeclaration};
				valueClassDeclaration->line = line;
				valueClassDeclaration->isGeneric = false;
			}
			continue;
		}
		if (value->classId == *valueMustBeClassId ||
		    compile.classes[value->classId]->inheritance.get(*valueMustBeClassId)) {
			continue;
		}
		switch (value->classId) {
			case DefaultClass::intClassId: {
				if (valueMustBeClassId == DefaultClass::floatClassId) {
					value = context.castPool.push(value, DefaultClass::floatClassId);
					continue;
				}
				break;
			}
			case DefaultClass::floatClassId: {
				if (*valueMustBeClassId == DefaultClass::intClassId) {
					valueMustBeClassId = DefaultClass::floatClassId;
					value = context.castPool.push(value, DefaultClass::floatClassId);
					mustReloadValue = true;
					continue;
				}
				break;
			}
			case DefaultClass::boolClassId: {
				switch (*valueMustBeClassId) {
					case DefaultClass::intClassId: {
						value = context.castPool.push(value, DefaultClass::intClassId);
						continue;
					}
					case DefaultClass::floatClassId: {
						value = context.castPool.push(value, DefaultClass::floatClassId);
						continue;
					}
				}
				break;
			}
			case DefaultClass::nullClassId: {
				continue;
			}
		}
		if (valueMustBeClassId == DefaultClass::anyClassId) {
			continue;
		}
		throwError("Cannot cast " +
		           compile.classes[value->classId]->getName(compile) + " to " +
		           compile.classes[*valueMustBeClassId]->getName(compile) +
		           "\nHint: Ensure all values in the Map match the expected value type or provide an explicit conversion.");
	}

	if (!keyMustBeClassId || !valueMustBeClassId) {
		throwError(
		    "Cannot infer key/value type for Map initialization\nHint: Provide "
		    "an explicit type annotation or add typed entries to the Map.");
		return;
	}

	if (mustReloadKey) {
		for (auto &[key, value] : values) {
			if (key->classId == *keyMustBeClassId) {
				continue;
			}
			if (key->classId == DefaultClass::nullClassId) {
				key->classId = *keyMustBeClassId;
				key->classDeclaration = keyClassDeclaration;
				key = static_cast<HasClassIdNode *>(key->optimize(in_data));
			}
		}
	}

	if (mustReloadValue) {
		for (auto &[key, value] : values) {
			if (value->classId == *valueMustBeClassId) {
				continue;
			}
			if (value->classId == DefaultClass::nullClassId) {
				if (value->kind == NodeType::CREATE_SET) {
					auto setNode = static_cast<CreateSetNode *>(value);
					if (setNode->values.empty() &&
					    compile.classes[*valueMustBeClassId]->genericBaseClassId ==
					        DefaultClass::mapClassId) {
						value = context.createMapPool.push(
						    value->line, nullptr,
						    std::vector<std::pair<HasClassIdNode *,
						                          HasClassIdNode *>>());
					}
				}
				value->classId = *valueMustBeClassId;
				value->classDeclaration = valueClassDeclaration;
				value = static_cast<HasClassIdNode *>(value->optimize(in_data));
			}
		}
	}

	keyClassDeclaration->classId = *keyMustBeClassId;
	valueClassDeclaration->classId = *valueMustBeClassId;
	classDeclaration = context.classDeclarationAllocator.push();
	classDeclaration->baseClassLexerStringId = lexerIdMap;
	classDeclaration->inputClassId = std::vector{keyClassDeclaration, valueClassDeclaration};
	classDeclaration->line = line;
	classDeclaration->isGeneric = false;
	classDeclaration->template load<true, false, true>(in_data);
	classId = *classDeclaration->classId;
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
	newNode->canSkipFindType = canSkipFindType;
	for (auto &[key, value] : values) {
		newNode->values.push_back(std::make_pair(
		    static_cast<HasClassIdNode *>(key->copy(in_data)),
		    static_cast<HasClassIdNode *>(value->copy(in_data))));
	}
	if (classDeclaration) {
		if (!classDeclaration->classId) {
			classDeclaration->template load<false>(in_data);
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

CreateMapNode::~CreateMapNode() {}

} // namespace Autolang

#endif
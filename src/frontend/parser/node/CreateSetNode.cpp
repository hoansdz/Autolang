#ifndef CREATE_SET_NODE_CPP
#define CREATE_SET_NODE_CPP

#include "Node.hpp"
#include "frontend/parser/ClassDeclaration.hpp"
#include "frontend/parser/ParserContext.hpp"
#include "shared/DefaultClass.hpp"

namespace Autolang {

ExprNode *CreateSetNode::resolve(in_func) {
	for (auto *&value : values) {
		value = static_cast<HasClassIdNode *>(value->resolve(in_data));
	}
	return this;
}

ExprNode *CreateSetNode::optimize(in_func) {
	if (classDeclaration) {
		classId = *classDeclaration->classId;
	}
	if (classId == DefaultClass::nullClassId) {
		optimizeAndInferenceType(in_data);
		return this;
	}
	auto clazz = compile.classes[classId];
	auto classInfo = context.classInfo[classId];
	if (clazz->genericBaseClassId != DefaultClass::setClassId) {
		throwError("Type mismatch, expected Set<> but '" +
		           compile.classes[classId]->getName(compile) + "' found\nHint: Ensure target variable type is a Set<T> instance.");
	}
	auto genericType = classInfo->genericTypeId[0];
	auto valueMustBeClassId = *genericType->classId;
	for (auto *&value : values) {
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
				if (genericType->nullable) {
					continue;
				}
				// std::cerr << compile.classes[classId]->getName(compile) << "\n";
				throwError("Elements in Set must be non-null\nHint: The Set element type is non-nullable. Use Set<T?> to allow null elements.");
			}
		}
		if (valueMustBeClassId == DefaultClass::anyClassId) {
			continue;
		}
		throwError("Cannot cast " + compile.classes[value->classId]->getName(compile) +
		           " to " + compile.classes[valueMustBeClassId]->getName(compile) +
		           "\nHint: Ensure all elements in the Set match the expected element type or provide an explicit conversion.");
	}
	return this;
}

void CreateSetNode::optimizeAndInferenceType(in_func) {
	if (values.empty()) {
		if (canSkipFindType)
			return;
		throwError(
		    "Cannot infer element type for Set initialization\nHint: Provide "
		    "an explicit type annotation or add typed elements to the Set.");
		return;
	}
	std::optional<ClassId> valueMustBeClassId;
	bool nullable = false;
	ClassDeclaration *valueClassDeclaration = nullptr;
	bool mustReload = false;
	for (auto *&value : values) {
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
						static_cast<CreateArrayNode *>(value)->canSkipFindType =
						    true;
						break;
					}
					case NodeType::CREATE_SET: {
						static_cast<CreateSetNode *>(value)->canSkipFindType =
						    true;
						break;
					}
					case NodeType::CREATE_MAP: {
						static_cast<CreateMapNode *>(value)->canSkipFindType =
						    true;
						break;
					}
					default:
						break;
				}
				value = static_cast<HasClassIdNode *>(value->optimize(in_data));
				if (value->classId == DefaultClass::nullClassId) {
					mustReload = true;
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
				auto classInfo = context.classInfo[value->classId];
				valueClassDeclaration->inputClassId =
				    std::vector{valueClassDeclaration};
				valueClassDeclaration->line = line;
				valueClassDeclaration->isGeneric = false;
			}
			continue;
		}
		if (value->classId == *valueMustBeClassId ||
		    compile.classes[value->classId]->inheritance.get(
		        *valueMustBeClassId)) {
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
			case DefaultClass::floatClassId: {
				if (*valueMustBeClassId == DefaultClass::intClassId) {
					valueMustBeClassId = DefaultClass::floatClassId;
					value = context.castPool.push(value,
					                              DefaultClass::floatClassId);
					mustReload = true;
					continue;
				}
				break;
			}
			case DefaultClass::boolClassId: {
				switch (*valueMustBeClassId) {
					case DefaultClass::intClassId: {
						value = context.castPool.push(value,
						                              DefaultClass::intClassId);
						continue;
					}
					case DefaultClass::floatClassId: {
						value = context.castPool.push(
						    value, DefaultClass::floatClassId);
						continue;
					}
				}
				break;
			}
			case DefaultClass::nullClassId: {
				nullable = true;
				continue;
			}
		}
		if (valueMustBeClassId == DefaultClass::anyClassId) {
			continue;
		}
		throwError("Cannot cast " +
		           compile.classes[value->classId]->getName(compile) + " to " +
		           compile.classes[*valueMustBeClassId]->getName(compile) +
		           "\nHint: Ensure all elements in the Set match the "
		           "expected element type or provide an explicit conversion.");
	}
	if (!valueMustBeClassId) {
		throwError(
		    "Cannot infer element type for Set initialization\nHint: Provide "
		    "an explicit type annotation or add typed elements to the Set.");
		return;
	}
	if (mustReload) {
		for (auto *&value : values) {
			if (value->classId == *valueMustBeClassId) {
				continue;
			}
			switch (value->classId) {
				case DefaultClass::nullClassId: {
					switch (value->kind) {
						case NodeType::CREATE_SET: {
							auto setNode = static_cast<CreateSetNode *>(value);
							if (setNode->values.empty() &&
							    compile.classes[*valueMustBeClassId]->genericBaseClassId ==
							        DefaultClass::mapClassId) {
								value = context.createMapPool.push(
								    value->line, nullptr,
								    std::vector<std::pair<HasClassIdNode *,
								                          HasClassIdNode *>>());
							}
							value->classId = *valueMustBeClassId;
							value->classDeclaration = valueClassDeclaration;
							value = static_cast<HasClassIdNode *>(value->optimize(in_data));
							continue;
						}
						case NodeType::CREATE_MAP:
						case NodeType::CREATE_ARRAY: {
							value->classId = *valueMustBeClassId;
							value->classDeclaration = valueClassDeclaration;
							value = static_cast<HasClassIdNode *>(value->optimize(in_data));
							continue;
						}
						default:
							break;
					}
					continue;
				}
			}
		}
	}
	valueClassDeclaration->classId = *valueMustBeClassId;
	classDeclaration = context.classDeclarationAllocator.push();
	classDeclaration->baseClassLexerStringId = lexerIdSet;
	classDeclaration->inputClassId = std::vector{valueClassDeclaration};
	classDeclaration->line = line;
	classDeclaration->isGeneric = false;
	classDeclaration->template load<true, false, true>(in_data);
	classId = *classDeclaration->classId;
}

void CreateSetNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
	for (auto *value : values) {
		value->putBytecodes(in_data, bytecodes);
	}
	auto classInfo = context.classInfo[classId];
	auto keyId = *classInfo->genericTypeId[0]->classId;
	bytecodes.emplace_back(Opcode::CREATE_SET_OBJECT);
	put_opcode_u32(bytecodes, classId);
	if (classInfo->genericTypeId[0]->nullable) {
		put_opcode_u32(bytecodes, DefaultClass::anyClassId);
	} else {
		put_opcode_u32(bytecodes, keyId);
	}
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
	newNode->canSkipFindType = canSkipFindType;
	for (auto *value : values) {
		newNode->values.push_back(
		    static_cast<HasClassIdNode *>(value->copy(in_data)));
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

CreateSetNode::~CreateSetNode() {}

} // namespace Autolang

#endif
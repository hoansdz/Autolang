#ifndef CREATE_ARRAY_NODE_CPP
#define CREATE_ARRAY_NODE_CPP

#include "Node.hpp"
#include "frontend/parser/ClassDeclaration.hpp"
#include "frontend/parser/Debugger.hpp"
#include "frontend/parser/ParserContext.hpp"
#include "shared/DefaultClass.hpp"

namespace Autolang {

ExprNode *CreateArrayNode::resolve(in_func) {
	for (auto *&value : values) {
		value = static_cast<HasClassIdNode *>(value->resolve(in_data));
	}
	return this;
}

ExprNode *CreateArrayNode::optimize(in_func) {
	if (classDeclaration) {
		classId = *classDeclaration->classId;
	}
	if (classId == DefaultClass::nullClassId) {
		// throwError("Cannot infer element type for Array initialization\nHint:
		// Provide an explicit type annotation or add typed elements to the
		// Array.");
		optimizeAndInferenceType(in_data);
		return this;
	}
	auto clazz = compile.classes[classId];
	auto classInfo = context.classInfo[classId];
	if (clazz->genericBaseClassId != DefaultClass::arrayClassId) {
		throwError("Type mismatch, expected Array<> but '" +
		           compile.classes[classId]->getName(compile) +
		           "' found\nHint: Ensure target variable type is an Array<T> "
		           "instance.");
	}
	auto genericType = classInfo->genericTypeId[0];
	auto valueMustBeClassId = *genericType->classId;
	for (auto *&value : values) {
		if (value->kind == NodeType::CREATE_ARRAY) {
			if (compile.classes[valueMustBeClassId]->genericBaseClassId == DefaultClass::arrayClassId) {
				static_cast<HasClassIdNode *>(value)->classId = valueMustBeClassId;
			}
		} else if (value->kind == NodeType::CREATE_SET) {
			if (compile.classes[valueMustBeClassId]->genericBaseClassId == DefaultClass::setClassId) {
				static_cast<HasClassIdNode *>(value)->classId = valueMustBeClassId;
			}
		} else if (value->kind == NodeType::CREATE_MAP) {
			if (compile.classes[valueMustBeClassId]->genericBaseClassId == DefaultClass::mapClassId) {
				static_cast<HasClassIdNode *>(value)->classId = valueMustBeClassId;
			}
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
			case DefaultClass::boolClassId: {
				switch (valueMustBeClassId) {
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
				if (genericType->nullable) {
					continue;
				}
				throwError("Elements in Array must be non-null\nHint: The "
				           "Array element type is non-nullable. Use Array<T?> "
				           "to allow null elements.");
			}
		}
		if (valueMustBeClassId == DefaultClass::anyClassId) {
			continue;
		}
		throwError("Cannot cast " +
		           compile.classes[value->classId]->getName(compile) + " to " +
		           compile.classes[valueMustBeClassId]->getName(compile) +
		           "\nHint: Ensure all elements in the Array match the "
		           "expected element type or provide an explicit conversion.");
	}
	return this;
}

void CreateArrayNode::optimizeAndInferenceType(in_func) {
	if (values.empty()) {
		if (canSkipFindType)
			return;
		throwError(
		    "Cannot infer element type for Array initialization\nHint: Provide "
		    "an explicit type annotation or add typed elements to the Array.");
		return;
	}
	std::optional<ClassId> valueMustBeClassId;
	bool nullable = false;
	ClassDeclaration *valueClassDeclaration;
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
				valueClassDeclaration->classId = value->classId;
				valueClassDeclaration->baseClassLexerStringId =
				    context.createLexerStringIfNotExists(
				        compile.classes[value->classId]->getName(compile));
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
				if (*valueMustBeClassId == DefaultClass::boolClassId) {
					valueMustBeClassId = DefaultClass::intClassId;
					mustReload = true;
					continue;
				}
				break;
			}
			case DefaultClass::floatClassId: {
				if (*valueMustBeClassId == DefaultClass::intClassId ||
				    *valueMustBeClassId == DefaultClass::boolClassId) {
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
		           "\nHint: Ensure all elements in the Array match the "
		           "expected element type or provide an explicit conversion.");
	}
	if (!valueMustBeClassId) {
		throwError(
		    "Cannot infer element type for Array initialization\nHint: Provide "
		    "an explicit type annotation or add typed elements to the Array.");
		return;
	}
	if (mustReload) {
		for (auto *&value : values) {
			if (value->classId == *valueMustBeClassId) {
				continue;
			}
			switch (value->classId) {
				case DefaultClass::intClassId: {
					if (*valueMustBeClassId == DefaultClass::floatClassId) {
						value = context.castPool.push(value, DefaultClass::floatClassId);
					}
					continue;
				}
				case DefaultClass::boolClassId: {
					if (*valueMustBeClassId == DefaultClass::intClassId) {
						value = context.castPool.push(value, DefaultClass::intClassId);
					} else if (*valueMustBeClassId == DefaultClass::floatClassId) {
						value = context.castPool.push(value, DefaultClass::floatClassId);
					}
					continue;
				}
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
	if (valueClassDeclaration->baseClassLexerStringId == 0) {
		valueClassDeclaration->baseClassLexerStringId =
		    context.createLexerStringIfNotExists(
		        compile.classes[*valueMustBeClassId]->getName(compile));
	}
	classDeclaration = context.classDeclarationAllocator.push();
	classDeclaration->baseClassLexerStringId = lexerIdArray;
	classDeclaration->inputClassId = std::vector{valueClassDeclaration};
	classDeclaration->line = line;
	classDeclaration->isGeneric = false;
	classDeclaration->template load<true, false, true>(in_data);
	classId = *classDeclaration->classId;
}

void CreateArrayNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
	for (auto *value : values) {
		value->putBytecodes(in_data, bytecodes);
	}
	auto classInfo = context.classInfo[classId];
	ClassId keyId = DefaultClass::anyClassId;
	if (!classInfo->genericTypeId.empty()) {
		if (classInfo->genericTypeId[0]->nullable) {
			keyId = DefaultClass::anyClassId;
		} else if (classInfo->genericTypeId[0]->classId) {
			keyId = *classInfo->genericTypeId[0]->classId;
		}
	}
	bytecodes.emplace_back(Opcode::CREATE_ARRAY_OBJECT);
	put_opcode_u32(bytecodes, classId);
	put_opcode_u32(bytecodes, keyId);
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
		if (classDeclaration->isGeneric) {
			resetClassDeclTree(classDeclaration);
		}
		if (!classDeclaration->classId) {
			classDeclaration->template load<false>(in_data);
			if (!classDeclaration->classId) {
				classDeclaration->template load<true>(in_data);
			}
			if (!classDeclaration->classId) {
				throwError("Bug: DeclarationNode copy: Unresolved class " +
				           classDeclaration->getName(in_data) +
				           "\nHint: Internal compiler error - class "
				           "declaration was not resolved before copy.");
			}
			newNode->classId = *classDeclaration->classId;
			if (classDeclaration->isGeneric) {
				resetClassDeclTree(classDeclaration);
			}
		} else {
			newNode->classId = *classDeclaration->classId;
		}
	}
	return newNode;
}

CreateArrayNode::~CreateArrayNode() {}

} // namespace Autolang

#endif
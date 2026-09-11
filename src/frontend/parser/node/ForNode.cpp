#ifndef FOR_NODE_CPP
#define FOR_NODE_CPP

#include "Node.hpp"
#include "frontend/parser/ParserContext.hpp"

namespace Autolang {

ExprNode *ForNode::resolve(in_func) {
	// detach = static_cast<VarNode *>(detach->resolve(in_data));
	// switch (detach->kind) {
	// 	case NodeType::VAR:
	// 	case NodeType::GET_PROP: {
	// 		break;
	// 	}
	// 	default: {
	// 		throwError("Invalid assign target");
	// 	}
	// }
	data = static_cast<HasClassIdNode *>(data->resolve(in_data));
	body.resolve(in_data);
	return this;
}

ExprNode *ForNode::optimize(in_func) {
	detach = static_cast<VarNode *>(detach->optimize(in_data));
	if (detachValue != nullptr) {
		detachValue = static_cast<VarNode *>(detachValue->optimize(in_data));
	}
	switch (data->kind) {
		case NodeType::RANGE: {
			if (detachValue != nullptr) {
				throwError("Multiple loop variables are not supported for Range\nHint: Use 'for (i in start..end)'.");
			}
			switch (detach->classId) {
				case Autolang::DefaultClass::nullClassId: {
					detach->declaration->classId =
					    Autolang::DefaultClass::intClassId;
					break;
				}
				case Autolang::DefaultClass::intClassId: {
					break;
				}
				default: {
					throwError("Loop control variable for range must be of type Int\nHint: Declare loop variable as Int (e.g., for (i: Int in 0..10)).");
				}
			}
			auto rangeNode = static_cast<RangeNode *>(data);
			data = static_cast<HasClassIdNode *>(rangeNode->optimize(in_data));
			rangeNode = static_cast<RangeNode *>(data);
			rangeNode->from = static_cast<HasClassIdNode *>(rangeNode->from->optimize(in_data));
			if (rangeNode->from->kind == NodeType::CONST_VAL) {
				static_cast<ConstValueNode *>(rangeNode->from)->isLoadPrimary =
				    false;
			}
			switch (rangeNode->from->classId) {
				case Autolang::DefaultClass::intClassId: {
					break;
				}
				default: {
					throwError("Range start value must be of type Int\nHint: Ensure the starting expression of range (start..end) evaluates to an Int.");
				}
			}
			rangeNode->to = static_cast<HasClassIdNode *>(rangeNode->to->optimize(in_data));
			switch (rangeNode->to->classId) {
				case Autolang::DefaultClass::intClassId: {
					break;
				}
				default: {
					throwError("Range end value must be of type Int\nHint: Ensure the ending expression of range (start..end) evaluates to an Int.");
				}
			}
			if (rangeNode->to->kind == NodeType::CONST_VAL) {
				static_cast<ConstValueNode *>(rangeNode->to)->isLoadPrimary =
				    true;
			}
			break;
		}
		case NodeType::CLASS_ACCESS: {
			throwError("Expected iterable value in 'for' loop\nHint: Use an iterable collection like Array, Set, Map, or Range in the for loop.");
		}
		default: {
			data = static_cast<HasClassIdNode *>(data->optimize(in_data));
			if (collectionNode != nullptr) {
				collectionNode->declaration->classId = data->classId;
				collectionNode->declaration->nullable = data->isNullable();
				collectionNode = static_cast<VarNode *>(collectionNode->optimize(in_data));
			}
			auto classInfo = context.classInfo[data->classId];
			if (classInfo->genericTypeId.empty()) {
				throwError("Cannot iterate over type '" +
				           compile.classes[data->classId]->getName(compile) + "'\nHint: Only Array, Set, Map, and Range types support iteration in for loops.");
			}
			auto clazz = compile.classes[data->classId];
			auto baseClassId = clazz->genericBaseClassId;
			switch (baseClassId) {
				case DefaultClass::setClassId:
				case DefaultClass::arrayClassId: {
					if (detachValue != nullptr) {
						throwError("Multiple loop variables are not supported for '" + clazz->getName(compile) +
						           "'\nHint: Use 'for (item in list)' or iterate over a Map: 'for (key, value in map)'.");
					}
					auto classType = classInfo->genericTypeId[0];
					ClassId target = *classType->classId;
					switch (detach->classId) {
						case Autolang::DefaultClass::nullClassId: {
							detach->declaration->classId = target;
							if (target == DefaultClass::functionClassId) {
								detach->declaration->classDeclaration =
								    classType;
							}
							detach->declaration->nullable = classType->nullable;
							break;
						}
						default: {
							if (target == detach->classId ||
							    compile.classes[detach->classId]
							        ->inheritance.get(target)) {
								if (!detach->isNullable() &&
								    classInfo->genericTypeId[0]->nullable) {
									throwError(
									    "Cannot assign nullable element to non-nullable variable\nHint: Declare loop variable as nullable (T?) or ensure collection element type is non-nullable.");
								}
								break;
							}
							throwError(
							    "Type mismatch: expected '" +
							    compile.classes[target]->getName(compile) +
							    "' but '" +
							    compile.classes[detach->classId]->getName(
							        compile) +
							    "' found\nHint: Ensure the loop control variable type matches the collection element type.");
							break;
						}
					}
					break;
				}
				case DefaultClass::mapClassId: {
					auto keyType = classInfo->genericTypeId[0];
					ClassId keyTarget = *keyType->classId;
					switch (detach->classId) {
						case Autolang::DefaultClass::nullClassId: {
							detach->declaration->classId = keyTarget;
							if (keyTarget == DefaultClass::functionClassId) {
								detach->declaration->classDeclaration = keyType;
							}
							detach->declaration->nullable = keyType->nullable;
							break;
						}
						default: {
							if (keyTarget == detach->classId ||
							    compile.classes[detach->classId]->inheritance.get(keyTarget)) {
								if (!detach->isNullable() && keyType->nullable) {
									throwError(
									    "Cannot assign nullable key to non-nullable variable\nHint: Declare loop variable as nullable (K?) or ensure map key type is non-nullable.");
								}
								break;
							}
							throwError(
							    "Type mismatch: expected '" +
							    compile.classes[keyTarget]->getName(compile) +
							    "' but '" +
							    compile.classes[detach->classId]->getName(compile) +
							    "' found\nHint: Ensure the loop key variable type matches the map key type.");
							break;
						}
					}
					if (detachValue != nullptr) {
						auto valType = classInfo->genericTypeId[1];
						ClassId valTarget = *valType->classId;
						switch (detachValue->classId) {
							case Autolang::DefaultClass::nullClassId: {
								detachValue->declaration->classId = valTarget;
								if (valTarget == DefaultClass::functionClassId) {
									detachValue->declaration->classDeclaration = valType;
								}
								detachValue->declaration->nullable = valType->nullable;
								break;
							}
							default: {
								if (valTarget == detachValue->classId ||
								    compile.classes[detachValue->classId]->inheritance.get(valTarget)) {
									if (!detachValue->isNullable() && valType->nullable) {
										throwError(
										    "Cannot assign nullable value to non-nullable variable\nHint: Declare loop variable as nullable (V?) or ensure map value type is non-nullable.");
									}
									break;
								}
								throwError(
								    "Type mismatch: expected '" +
								    compile.classes[valTarget]->getName(compile) +
								    "' but '" +
								    compile.classes[detachValue->classId]->getName(compile) +
								    "' found\nHint: Ensure the loop value variable type matches the map value type.");
								break;
							}
						}
					}
					break;
				}
				default: {
					throwError("Cannot iterate over type '" + clazz->getName(compile) + "'\nHint: Only Array, Set, Map, and Range types support iteration in for loops.");
				}
			}
			break;
		}
	}
	body.optimize(in_data);
	return this;
}

ExprNode *ForNode::copy(in_func) {
	auto funcInfo = context.getCurrentFunctionInfo(in_data);
	auto newDetach = static_cast<VarNode *>(detach->copy(in_data));
	if (funcInfo && detach && detach->declaration && newDetach) {
		funcInfo->reflectDeclarationMap[detach->declaration] =
		    newDetach->declaration;
	}
	VarNode *newDetachValue = nullptr;
	if (detachValue) {
		newDetachValue = static_cast<VarNode *>(detachValue->copy(in_data));
		if (funcInfo && detachValue->declaration && newDetachValue) {
			funcInfo->reflectDeclarationMap[detachValue->declaration] =
			    newDetachValue->declaration;
		}
	}
	VarNode *newIteratorNode = nullptr;
	if (iteratorNode) {
		newIteratorNode = static_cast<VarNode *>(iteratorNode->copy(in_data));
		if (funcInfo && iteratorNode->declaration && newIteratorNode) {
			funcInfo->reflectDeclarationMap[iteratorNode->declaration] =
			    newIteratorNode->declaration;
		}
	}
	VarNode *newCollectionNode = nullptr;
	if (collectionNode) {
		newCollectionNode =
		    static_cast<VarNode *>(collectionNode->copy(in_data));
		if (funcInfo && collectionNode->declaration && newCollectionNode) {
			funcInfo->reflectDeclarationMap[collectionNode->declaration] =
			    newCollectionNode->declaration;
		}
	}
	auto newNode = context.forPool.push(
	    line, newDetach,
	    static_cast<HasClassIdNode *>(data->copy(in_data)), newIteratorNode,
	    newDetachValue, newCollectionNode);
	newNode->body.nodes.reserve(body.nodes.size());
	for (auto *node : body.nodes) {
		newNode->body.nodes.push_back(node->copy(in_data));
	}
	return newNode;
}

bool ForNode::putOptimizedRangeBytecode(in_func,
                                        std::vector<uint8_t> &bytecodes,
                                        BytecodePos &jumpIfFalseByte,
                                        BytecodePos &firstSkipByte) {
	OperatorId operatorId = static_cast<RangeNode *>(data)->lessThan
	                            ? OperatorId::OP_GREATER_EQ
	                            : OperatorId::OP_GREATER;
	auto right = static_cast<RangeNode *>(data)->to;
	switch (right->kind) {
		case NodeType::VAR: {
			auto rightNode = static_cast<VarNode *>(right);
			if (detach->declaration->isGlobal) {
				if (rightNode->declaration->isGlobal) {
					bytecodes.emplace_back(detach->declaration->isGlobal
					                           ? Opcode::PLUS_PLUS_GLOBAL
					                           : Opcode::PLUS_PLUS_LOCAL);
					put_opcode_u32(bytecodes, detach->declaration->id);
					bytecodes.emplace_back(Opcode::GLOBAL_CAL_GLOBAL_JUMP);
					bytecodes.emplace_back(operatorId);
					put_opcode_u32(bytecodes, detach->declaration->id);
					put_opcode_u32(bytecodes, rightNode->declaration->id);
					jumpIfFalseByte =
					    bytecodes.size() - context.currentBytecodePos;
					put_opcode_u32(bytecodes, 0);
					return true;
				}
				bytecodes.emplace_back(detach->declaration->isGlobal
				                           ? Opcode::PLUS_PLUS_GLOBAL
				                           : Opcode::PLUS_PLUS_LOCAL);
				put_opcode_u32(bytecodes, detach->declaration->id);
				bytecodes.emplace_back(Opcode::GLOBAL_CAL_LOCAL_JUMP);
				bytecodes.emplace_back(operatorId);
				put_opcode_u32(bytecodes, detach->declaration->id);
				put_opcode_u32(bytecodes, rightNode->declaration->id);
				jumpIfFalseByte = bytecodes.size() - context.currentBytecodePos;
				put_opcode_u32(bytecodes, 0);
				return true;
			}
			if (rightNode->declaration->isGlobal) {
				bytecodes.emplace_back(detach->declaration->isGlobal
				                           ? Opcode::PLUS_PLUS_GLOBAL
				                           : Opcode::PLUS_PLUS_LOCAL);
				put_opcode_u32(bytecodes, detach->declaration->id);
				bytecodes.emplace_back(Opcode::LOCAL_CAL_GLOBAL_JUMP);
				bytecodes.emplace_back(operatorId);
				put_opcode_u32(bytecodes, detach->declaration->id);
				put_opcode_u32(bytecodes, rightNode->declaration->id);
				jumpIfFalseByte = bytecodes.size() - context.currentBytecodePos;
				put_opcode_u32(bytecodes, 0);
				return true;
			}
			bytecodes.emplace_back(detach->declaration->isGlobal
			                           ? Opcode::PLUS_PLUS_GLOBAL
			                           : Opcode::PLUS_PLUS_LOCAL);
			put_opcode_u32(bytecodes, detach->declaration->id);
			bytecodes.emplace_back(Opcode::LOCAL_CAL_LOCAL_JUMP);
			bytecodes.emplace_back(operatorId);
			put_opcode_u32(bytecodes, detach->declaration->id);
			put_opcode_u32(bytecodes, rightNode->declaration->id);
			jumpIfFalseByte = bytecodes.size() - context.currentBytecodePos;
			put_opcode_u32(bytecodes, 0);
			return true;
		}
		case NodeType::CONST_VAL: {
			auto rightNode = static_cast<ConstValueNode *>(right);
			if (right->classId == Autolang::DefaultClass::nullClassId) {
				// throwError("Null must be cleared by optimizer");
				return false;
			}
			if (right->classId == Autolang::DefaultClass::boolClassId) {
				return false;
			}
			bytecodes.emplace_back(detach->declaration->isGlobal
			                           ? Opcode::PLUS_PLUS_GLOBAL
			                           : Opcode::PLUS_PLUS_LOCAL);
			put_opcode_u32(bytecodes, detach->declaration->id);
			bytecodes.emplace_back(detach->declaration->isGlobal
			                           ? Opcode::GLOBAL_CAL_CONST_JUMP
			                           : Opcode::LOCAL_CAL_CONST_JUMP);
			bytecodes.emplace_back(operatorId);
			put_opcode_u32(bytecodes, detach->declaration->id);
			put_opcode_u32(bytecodes, rightNode->id);
			jumpIfFalseByte = bytecodes.size() - context.currentBytecodePos;
			put_opcode_u32(bytecodes, 0);
			return true;
		}
		default: {
			break;
		}
	}
	return false;
}

void ForNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
	loadOpcodeLine(in_data, bytecodes);
	BytecodePos jumpIfFalseByte;
	std::optional<BytecodePos> setupJumpIfFalse;
	switch (data->kind) {
		// for (detach in from..to) { body }
		case NodeType::RANGE: {
			auto rangeNode = static_cast<RangeNode *>(data);
			// detach = from
			rangeNode->from->putBytecodes(in_data, bytecodes);
			detach->isStore = true;
			detach->putBytecodes(in_data, bytecodes);

			// Skip
			detach->isStore = false;
			detach->putBytecodes(in_data, bytecodes);
			rangeNode->to->putBytecodes(in_data, bytecodes);
			bytecodes.emplace_back(rangeNode->lessThan ? Opcode::LESS_THAN
			                                           : Opcode::LESS_THAN_EQ);
			bytecodes.emplace_back(Opcode::JUMP_IF_FALSE);
			setupJumpIfFalse = bytecodes.size() - context.currentBytecodePos;
			put_opcode_u32(bytecodes, 0);
			bytecodes.emplace_back(Opcode::JUMP);
			BytecodePos firstSkipByte =
			    bytecodes.size() - context.currentBytecodePos;
			put_opcode_u32(bytecodes, 0);
			// detach++ => skip first
			continuePos = bytecodes.size() - context.currentBytecodePos;
			rewrite_opcode_u32(bytecodes.data() + context.currentBytecodePos,
			                   firstSkipByte,
			                   bytecodes.size() - context.currentBytecodePos);
			if (putOptimizedRangeBytecode(in_data, bytecodes, jumpIfFalseByte,
			                              firstSkipByte)) {

			} else {
				// compare
				detach->putBytecodes(in_data, bytecodes);
				bytecodes.emplace_back(Opcode::FAST_PLUS_PLUS);
				rangeNode->to->putBytecodes(in_data, bytecodes);
				bytecodes.emplace_back(rangeNode->lessThan
				                           ? Opcode::LESS_THAN
				                           : Opcode::LESS_THAN_EQ);
				bytecodes.emplace_back(Opcode::JUMP_IF_FALSE);
				jumpIfFalseByte = bytecodes.size() - context.currentBytecodePos;
				put_opcode_u32(bytecodes, 0);
			}
			rewrite_opcode_u32(bytecodes.data() + context.currentBytecodePos,
			                   firstSkipByte,
			                   bytecodes.size() - context.currentBytecodePos);
			break;
		}
		default: {
			data->optimize(in_data);
			auto clazz = compile.classes[data->classId];
			auto classInfo = context.classInfo[data->classId];
			if (classInfo->genericTypeId.empty()) {
				throwError("Cannot iterate over type '" + clazz->getName(compile) + "'\nHint: Only Array, Set, Map, and Range types support iteration in for loops.");
			}
			auto baseClassId = clazz->genericBaseClassId;
			switch (baseClassId) {
				case DefaultClass::setClassId:
				case DefaultClass::arrayClassId: {
					bytecodes.emplace_back(iteratorNode->declaration->isGlobal
					                           ? Opcode::GLOBAL_STORE_CONST
					                           : Opcode::LOCAL_STORE_CONST);
					put_opcode_u32(bytecodes, iteratorNode->declaration->id);
					put_opcode_u32(bytecodes, 0); // null value

					if (collectionNode != nullptr) {
						if (data->kind == NodeType::VAR) {
							static_cast<AccessNode *>(data)->isStore = false;
						}
						data->putBytecodes(in_data, bytecodes);
						bytecodes.emplace_back(collectionNode->declaration->isGlobal
						                           ? Opcode::STORE_GLOBAL
						                           : Opcode::STORE_LOCAL);
						put_opcode_u32(bytecodes, collectionNode->declaration->id);
					}

					continuePos = bytecodes.size() - context.currentBytecodePos;

					// Skip
					if (collectionNode != nullptr) {
						collectionNode->putBytecodes(in_data, bytecodes);
					} else {
						if (data->kind == NodeType::VAR) {
							static_cast<AccessNode *>(data)->isStore = false;
						}
						data->putBytecodes(in_data, bytecodes);
					}
					bytecodes.emplace_back(baseClassId ==
					                               DefaultClass::arrayClassId
					                           ? Opcode::FOR_LIST
					                           : Opcode::FOR_SET);
					bytecodes.emplace_back(iteratorNode->declaration->isGlobal
					                           ? Opcode::STORE_GLOBAL
					                           : Opcode::STORE_LOCAL);
					put_opcode_u32(bytecodes, detach->declaration->id);
					put_opcode_u32(bytecodes, iteratorNode->declaration->id);
					jumpIfFalseByte =
					    bytecodes.size() - context.currentBytecodePos;
					put_opcode_u32(bytecodes, 0);
					break;
				}
				case DefaultClass::mapClassId: {
					bytecodes.emplace_back(iteratorNode->declaration->isGlobal
					                           ? Opcode::GLOBAL_STORE_CONST
					                           : Opcode::LOCAL_STORE_CONST);
					put_opcode_u32(bytecodes, iteratorNode->declaration->id);
					put_opcode_u32(bytecodes, 0); // null value

					if (collectionNode != nullptr) {
						if (data->kind == NodeType::VAR) {
							static_cast<AccessNode *>(data)->isStore = false;
						}
						data->putBytecodes(in_data, bytecodes);
						bytecodes.emplace_back(collectionNode->declaration->isGlobal
						                           ? Opcode::STORE_GLOBAL
						                           : Opcode::STORE_LOCAL);
						put_opcode_u32(bytecodes, collectionNode->declaration->id);
					}

					continuePos = bytecodes.size() - context.currentBytecodePos;

					// Skip
					if (collectionNode != nullptr) {
						collectionNode->putBytecodes(in_data, bytecodes);
					} else {
						if (data->kind == NodeType::VAR) {
							static_cast<AccessNode *>(data)->isStore = false;
						}
						data->putBytecodes(in_data, bytecodes);
					}
					if (detachValue == nullptr) {
						bytecodes.emplace_back(Opcode::FOR_MAP_KEY);
						bytecodes.emplace_back(iteratorNode->declaration->isGlobal
						                           ? Opcode::STORE_GLOBAL
						                           : Opcode::STORE_LOCAL);
						put_opcode_u32(bytecodes, detach->declaration->id);
						put_opcode_u32(bytecodes, iteratorNode->declaration->id);
						jumpIfFalseByte =
						    bytecodes.size() - context.currentBytecodePos;
						put_opcode_u32(bytecodes, 0);
					} else {
						bytecodes.emplace_back(Opcode::FOR_MAP_KEY_VALUE);
						bytecodes.emplace_back(iteratorNode->declaration->isGlobal
						                           ? Opcode::STORE_GLOBAL
						                           : Opcode::STORE_LOCAL);
						put_opcode_u32(bytecodes, detach->declaration->id);
						put_opcode_u32(bytecodes, detachValue->declaration->id);
						put_opcode_u32(bytecodes, iteratorNode->declaration->id);
						jumpIfFalseByte =
						    bytecodes.size() - context.currentBytecodePos;
						put_opcode_u32(bytecodes, 0);
					}
					break;
				}
				default: {
					throwError("Cannot iterate over type '" + clazz->getName(compile) + "'\nHint: Only Array, Set, Map, and Range types support iteration in for loops.");
				}
			}
			break;
		}
	}
	// body
	auto lastMustReturnValueNode = context.mustReturnValueNode;
	context.mustReturnValueNode = nullptr;
	body.putBytecodes(in_data, bytecodes);
	context.mustReturnValueNode = lastMustReturnValueNode;
	bytecodes.emplace_back(Opcode::JUMP);
	put_opcode_u32(bytecodes, continuePos);
	if (setupJumpIfFalse) {
		rewrite_opcode_u32(bytecodes.data() + context.currentBytecodePos,
		                   *setupJumpIfFalse,
		                   bytecodes.size() - context.currentBytecodePos);
	}
	rewrite_opcode_u32(bytecodes.data() + context.currentBytecodePos,
	                   jumpIfFalseByte,
	                   bytecodes.size() - context.currentBytecodePos);
	breakPos = bytecodes.size() - context.currentBytecodePos;
}

ForNode::~ForNode() {
	deleteNode(detach);
	deleteNode(data);
	deleteNode(iteratorNode);
	if (detachValue) {
		deleteNode(detachValue);
	}
	if (collectionNode) {
		deleteNode(collectionNode);
	}
}

} // namespace Autolang

#endif
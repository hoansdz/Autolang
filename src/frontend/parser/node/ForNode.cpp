#ifndef FOR_NODE_CPP
#define FOR_NODE_CPP

#include "Node.hpp"
#include "frontend/parser/ParserContext.hpp"

namespace AutoLang {

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

void ForNode::optimize(in_func) {
	detach->optimize(in_data);
	switch (data->kind) {
		case NodeType::RANGE: {
			switch (detach->classId) {
				case AutoLang::DefaultClass::nullClassId: {
					detach->declaration->classId =
					    AutoLang::DefaultClass::intClassId;
					break;
				}
				case AutoLang::DefaultClass::intClassId: {
					break;
				}
				default: {
					throwError("Detach value must be Int");
				}
			}
			auto rangeNode = static_cast<RangeNode *>(data);
			rangeNode->optimize(in_data);
			rangeNode->from->optimize(in_data);
			switch (rangeNode->from->classId) {
				case AutoLang::DefaultClass::intClassId: {
					break;
				}
				default: {
					throwError("From value must be Int");
				}
			}
			rangeNode->to->optimize(in_data);
			switch (rangeNode->to->classId) {
				case AutoLang::DefaultClass::intClassId: {
					break;
				}
				default: {
					throwError("To value must be Int");
				}
			}
			if (rangeNode->to->kind == NodeType::CONST) {
				static_cast<ConstValueNode *>(rangeNode->to)->isLoadPrimary =
				    true;
			}
			break;
		}
		case NodeType::CLASS_ACCESS: {
			throwError("Expected value");
		}
		default: {
			data->optimize(in_data);
			auto classInfo = context.classInfo[data->classId];
			if (classInfo->genericTypeId.empty()) {
				throwError("Cannot loop in " +
				           compile.classes[data->classId]->name);
			}
			auto baseClassId = classInfo->baseClassId;
			switch (baseClassId) {
				case DefaultClass::arrayClassId: {
					ClassId target = *classInfo->genericTypeId[0]->classId;
					switch (detach->classId) {
						case AutoLang::DefaultClass::nullClassId: {
							detach->declaration->classId = target;
							detach->declaration->nullable =
							    classInfo->genericTypeId[0]->nullable;
							break;
						}
						default: {
							if (target == detach->classId ||
							    compile.classes[detach->classId]
							        ->inheritance.get(target)) {
								if (!detach->isNullable() &&
								    classInfo->genericTypeId[0]->nullable) {
									throwError(
									    "Cannot detach non nullable variable");
								}
								break;
							}
							throwError("Type mismatch: expected '" +
							           compile.classes[target]->name +
							           "' but '" +
							           compile.classes[detach->classId]->name +
							           "' found");
							break;
						}
					}
					break;
				}
				default: {
					throwError("Cannot loop in " +
					           compile.classes[data->classId]->name);
				}
			}
			break;
		}
	}
	body.optimize(in_data);
}

ExprNode *ForNode::copy(in_func) {
	return context.forPool.push(
	    line, static_cast<VarNode *>(detach->copy(in_data)),
	    static_cast<HasClassIdNode *>(data->copy(in_data)), iteratorNode);
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
					jumpIfFalseByte = bytecodes.size();
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
				jumpIfFalseByte = bytecodes.size();
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
				jumpIfFalseByte = bytecodes.size();
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
			jumpIfFalseByte = bytecodes.size();
			put_opcode_u32(bytecodes, 0);
			return true;
		}
		case NodeType::CONST: {
			auto rightNode = static_cast<ConstValueNode *>(right);
			if (right->classId == AutoLang::DefaultClass::nullClassId) {
				// throwError("Null must be cleared by optimizer");
				return false;
			}
			if (right->classId == AutoLang::DefaultClass::boolClassId) {
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
			jumpIfFalseByte = bytecodes.size();
			put_opcode_u32(bytecodes, 0);
			return true;
		}
	}
	return false;
}

void ForNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
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
			setupJumpIfFalse = bytecodes.size();
			put_opcode_u32(bytecodes, 0);
			bytecodes.emplace_back(Opcode::JUMP);
			BytecodePos firstSkipByte = bytecodes.size();
			put_opcode_u32(bytecodes, 0);
			// detach++ => skip first
			continuePos = bytecodes.size();
			rewrite_opcode_u32(bytecodes, firstSkipByte, bytecodes.size());
			if (putOptimizedRangeBytecode(in_data, bytecodes, jumpIfFalseByte,
			                              firstSkipByte)) {

			} else {
				// compare
				detach->putBytecodes(in_data, bytecodes);
				bytecodes.emplace_back(Opcode::PLUS_PLUS);
				rangeNode->to->putBytecodes(in_data, bytecodes);
				bytecodes.emplace_back(rangeNode->lessThan
				                           ? Opcode::LESS_THAN
				                           : Opcode::LESS_THAN_EQ);
				bytecodes.emplace_back(Opcode::JUMP_IF_FALSE);
				jumpIfFalseByte = bytecodes.size();
				put_opcode_u32(bytecodes, 0);
			}
			rewrite_opcode_u32(bytecodes, firstSkipByte, bytecodes.size());
			break;
		}
		default: {
			data->optimize(in_data);
			auto classInfo = context.classInfo[data->classId];
			if (classInfo->genericTypeId.empty()) {
				throwError("Cannot loop in " +
				           compile.classes[data->classId]->name);
			}
			auto baseClassId = classInfo->baseClassId;
			switch (baseClassId) {
				case DefaultClass::arrayClassId: {
					bytecodes.emplace_back(Opcode::LOAD_NULL);
					bytecodes.emplace_back(iteratorNode->declaration->isGlobal
					                           ? Opcode::STORE_GLOBAL
					                           : Opcode::STORE_LOCAL);
					put_opcode_u32(bytecodes, iteratorNode->declaration->id);

					continuePos = bytecodes.size();

					// Skip
					static_cast<AccessNode *>(data)->isStore = false;
					data->putBytecodes(in_data, bytecodes);
					bytecodes.emplace_back(Opcode::FOR_LIST);
					bytecodes.emplace_back(iteratorNode->declaration->isGlobal
					                           ? Opcode::STORE_GLOBAL
					                           : Opcode::STORE_LOCAL);
					put_opcode_u32(bytecodes, detach->declaration->id);
					put_opcode_u32(bytecodes, iteratorNode->declaration->id);
					jumpIfFalseByte = bytecodes.size();
					put_opcode_u32(bytecodes, 0);
					break;
				}
				default: {
					throwError("Cannot loop in " +
					           compile.classes[data->classId]->name);
				}
			}
			break;
		}
	}
	// body
	body.putBytecodes(in_data, bytecodes);
	bytecodes.emplace_back(Opcode::JUMP);
	put_opcode_u32(bytecodes, continuePos);
	if (setupJumpIfFalse) {
		rewrite_opcode_u32(bytecodes, *setupJumpIfFalse, bytecodes.size());
	}
	rewrite_opcode_u32(bytecodes, jumpIfFalseByte, bytecodes.size());
	breakPos = bytecodes.size();
}

ForNode::~ForNode() {
	deleteNode(detach);
	deleteNode(data);
	deleteNode(iteratorNode);
}

} // namespace AutoLang

#endif
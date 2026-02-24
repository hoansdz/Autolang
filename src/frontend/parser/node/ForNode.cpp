#ifndef FOR_NODE_CPP
#define FOR_NODE_CPP

#include "Node.hpp"
#include "frontend/parser/ParserContext.hpp"

namespace AutoLang {

ExprNode *ForNode::resolve(in_func) {
	detach = static_cast<AccessNode *>(detach->resolve(in_data));
	switch (detach->kind) {
		case NodeType::VAR:
		case NodeType::GET_PROP: {
			break;
		}
		default: {
			throwError("Invalid assign target");
		}
	}
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
	    line, static_cast<AccessNode *>(detach->copy(in_data)),
	    static_cast<HasClassIdNode *>(data->copy(in_data)), iteratorNode);
}

void ForNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
	BytecodePos jumpIfFalseByte;
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
			bytecodes.emplace_back(Opcode::JUMP);
			BytecodePos firstSkipByte = bytecodes.size();
			put_opcode_u32(bytecodes, 0);
			// detach++ => skip first
			continuePos = bytecodes.size();
			detach->putBytecodes(in_data, bytecodes);
			bytecodes.emplace_back(Opcode::PLUS_PLUS);
			rewrite_opcode_u32(bytecodes, firstSkipByte, bytecodes.size());
			// compare
			rangeNode->to->putBytecodes(in_data, bytecodes);
			bytecodes.emplace_back(rangeNode->lessThan ? Opcode::LESS_THAN
			                                           : Opcode::LESS_THAN_EQ);
			bytecodes.emplace_back(Opcode::JUMP_IF_FALSE);
			jumpIfFalseByte = bytecodes.size();
			put_opcode_u32(bytecodes, 0);
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
					static_cast<AccessNode*>(data)->isStore = false;
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
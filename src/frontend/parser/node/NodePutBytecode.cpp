#ifndef NODE_PUT_BYTECODE_CPP
#define NODE_PUT_BYTECODE_CPP

#include "frontend/parser/node/NodePutBytecode.hpp"
#include "frontend/parser/Debugger.hpp"
#include "frontend/parser/node/CreateNode.hpp"

namespace AutoLang {

void UnaryNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
	value->putBytecodes(in_data, bytecodes);
	switch (op) {
		case Lexer::TokenType::MINUS: {
			bytecodes.emplace_back(Opcode::NEGATIVE);
			break;
		}
		case Lexer::TokenType::NOT: {
			bytecodes.emplace_back(Opcode::NOT);
			break;
		}
		default:
			break;
	}
}

void SkipNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
	bytecodes.emplace_back(Opcode::JUMP);
	jumpBytePos = bytecodes.size();
	put_opcode_u32(bytecodes, 0);
}

void SkipNode::rewrite(in_func, std::vector<uint8_t> &bytecodes) {
	switch (type) {
		case Lexer::TokenType::CONTINUE:
			rewrite_opcode_u32(bytecodes, jumpBytePos, context.continuePos);
			break;
		case Lexer::TokenType::BREAK:
			rewrite_opcode_u32(bytecodes, jumpBytePos, context.breakPos);
			break;
		default:
			break;
	}
}

void ConstValueNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
	if (classId == AutoLang::DefaultClass::nullClassId) {
		bytecodes.emplace_back(Opcode::LOAD_NULL);
		return;
	}
	if (classId == AutoLang::DefaultClass::boolClassId) {
		bytecodes.emplace_back(obj->b ? Opcode::LOAD_TRUE : LOAD_FALSE);
		return;
	}
	bytecodes.emplace_back(isLoadPrimary ? Opcode::LOAD_CONST_PRIMARY
	                                     : Opcode::LOAD_CONST);
	put_opcode_u32(bytecodes, id);
}

void CastNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
	value->putBytecodes(in_data, bytecodes);
	switch (classId) {
		case AutoLang::DefaultClass::intClassId: {
			if (value->classId == AutoLang::DefaultClass::floatClassId) {
				bytecodes.emplace_back(Opcode::FLOAT_TO_INT);
				return;
			}
			if (value->classId == AutoLang::DefaultClass::boolClassId) {
				bytecodes.emplace_back(Opcode::BOOL_TO_INT);
				return;
			}
			bytecodes.emplace_back(Opcode::TO_INT);
			return;
		}
		case AutoLang::DefaultClass::floatClassId: {
			if (value->classId == AutoLang::DefaultClass::intClassId) {
				bytecodes.emplace_back(Opcode::INT_TO_FLOAT);
				return;
			}
			if (value->classId == AutoLang::DefaultClass::boolClassId) {
				bytecodes.emplace_back(Opcode::BOOL_TO_FLOAT);
				return;
			}
			bytecodes.emplace_back(Opcode::TO_FLOAT);
			return;
		}
		default:
			if (value->classId == AutoLang::DefaultClass::intClassId) {
				bytecodes.emplace_back(Opcode::INT_TO_STRING);
				return;
			}
			if (value->classId == AutoLang::DefaultClass::floatClassId) {
				bytecodes.emplace_back(Opcode::FLOAT_TO_STRING);
				return;
			}
			bytecodes.emplace_back(Opcode::TO_STRING);
			return;
	}
}

void GetPropNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
	if (!isStatic) {
		caller->putBytecodes(in_data, bytecodes);
		if (isStore) {
			if (accessNullable) {
				throwError(
				    "Bug: Setnode not ensure store data is non nullable");
			}
			bytecodes.emplace_back(Opcode::STORE_MEMBER);
			put_opcode_u32(bytecodes, id);
			return;
		}
		if (accessNullable) {
			assert(context.jumpIfNullNode != nullptr);
			bytecodes.emplace_back(context.jumpIfNullNode->returnNullIfNull
			                           ? Opcode::LOAD_MEMBER_CAN_RET_NULL
			                           : Opcode::LOAD_MEMBER_IF_NNULL);
		} else {
			bytecodes.emplace_back(Opcode::LOAD_MEMBER);
		}
		put_opcode_u32(bytecodes, id);
		return;
	}
	switch (caller->kind) {
		case NodeType::VAR:
		case NodeType::UNKNOW: {
			break;
		}
		default: {
			caller->putBytecodes(in_data, bytecodes);
			bytecodes.emplace_back(Opcode::POP);
			break;
		}
	}
	if (accessNullable) {
		if (isStore) {
			throwError("Bug: Setnode not ensure store data is non nullable");
		}
		warning(in_data, "Access static variables: we recommend call " +
		                     compile.classes[caller->classId].name + "." +
		                     name);
		accessNullable = false;
	}
	bytecodes.emplace_back(isStore ? Opcode::STORE_GLOBAL
	                               : Opcode::LOAD_GLOBAL);
	put_opcode_u32(bytecodes, id);
}

void GetPropNode::rewrite(in_func, std::vector<uint8_t> &bytecodes) {
	if (context.jumpIfNullNode) {
		caller->rewrite(in_data, bytecodes);
		if (accessNullable && isStore) {
			rewrite_opcode_u32(bytecodes, jumpIfNullPos,
			                   context.jumpIfNullNode->jumpIfNullPos);
		}
	}
}

void IfNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
	if (condition->kind == NodeType::CONST) {
		// Is bool because optimize forbiddened others
		if (static_cast<ConstValueNode *>(condition)->obj->b) {
			ifTrue.putBytecodes(in_data, bytecodes);
		} else if (ifFalse != nullptr) {
			ifFalse->putBytecodes(in_data, bytecodes);
		}
		return;
	}
	condition->putBytecodes(in_data, bytecodes);
	bytecodes.emplace_back(Opcode::JUMP_IF_FALSE);
	size_t jumpIfFalseByte = bytecodes.size();
	put_opcode_u32(bytecodes, 0);
	ifTrue.putBytecodes(in_data, bytecodes);
	if (ifFalse) {
		bytecodes.emplace_back(Opcode::JUMP);
		size_t jumpIfTrueByte = bytecodes.size();
		put_opcode_u32(bytecodes, 0);
		rewrite_opcode_u32(bytecodes, jumpIfFalseByte, bytecodes.size());
		ifFalse->putBytecodes(in_data, bytecodes);
		rewrite_opcode_u32(bytecodes, jumpIfTrueByte, bytecodes.size());
	} else {
		rewrite_opcode_u32(bytecodes, jumpIfFalseByte, bytecodes.size());
	}
}

void IfNode::rewrite(in_func, std::vector<uint8_t> &bytecodes) {
	if (condition->kind == NodeType::CONST) {
		// Is bool because optimize forbiddened others
		if (static_cast<ConstValueNode *>(condition)->obj->b) {
			ifTrue.rewrite(in_data, bytecodes);
		} else if (ifFalse != nullptr) {
			ifFalse->rewrite(in_data, bytecodes);
		}
		return;
	}
	ifTrue.rewrite(in_data, bytecodes);
	if (ifFalse)
		ifFalse->rewrite(in_data, bytecodes);
}

void CanBreakContinueNode::rewrite(in_func, std::vector<uint8_t> &bytecodes) {
	uint32_t lastContinuePos = context.continuePos;
	uint32_t lastBreakPos = context.breakPos;
	context.continuePos = continuePos;
	context.breakPos = breakPos;
	body.rewrite(in_data, bytecodes);
	context.continuePos = lastContinuePos;
	context.breakPos = lastBreakPos;
}

void WhileNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
	continuePos = bytecodes.size();

	if (condition->kind == NodeType::CONST) {
		// Is bool because optimize forbiddened others
		if (!static_cast<ConstValueNode *>(condition)->obj->b) {
			compile.warnings.push_back(
			    "While command won't never be called here");
			return;
		}
		body.putBytecodes(in_data, bytecodes);
		bytecodes.emplace_back(Opcode::JUMP);
		put_opcode_u32(bytecodes, continuePos);
		breakPos = bytecodes.size();
		return;
	}

	condition->putBytecodes(in_data, bytecodes);
	bytecodes.emplace_back(Opcode::JUMP_IF_FALSE);
	size_t jumpIfFalseByte = bytecodes.size();
	put_opcode_u32(bytecodes, 0);
	body.putBytecodes(in_data, bytecodes);
	bytecodes.emplace_back(Opcode::JUMP);
	put_opcode_u32(bytecodes, continuePos);
	rewrite_opcode_u32(bytecodes, jumpIfFalseByte, bytecodes.size());
	breakPos = bytecodes.size();
}

void ForRangeNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
	// for (detach in from..to) { body }
	// detach = from
	from->putBytecodes(in_data, bytecodes);
	detach->isStore = true;
	detach->putBytecodes(in_data, bytecodes);

	// Skip
	detach->isStore = false;
	detach->putBytecodes(in_data, bytecodes);
	bytecodes.emplace_back(Opcode::JUMP);
	size_t firstSkipByte = bytecodes.size();
	put_opcode_u32(bytecodes, 0);
	// detach++ => skip first
	continuePos = bytecodes.size();
	detach->putBytecodes(in_data, bytecodes);
	bytecodes.emplace_back(Opcode::PLUS_PLUS);
	rewrite_opcode_u32(bytecodes, firstSkipByte, bytecodes.size());
	// compare
	to->putBytecodes(in_data, bytecodes);
	bytecodes.emplace_back(isLessThanEq ? Opcode::LESS_THAN_EQ
	                                    : Opcode::LESS_THAN);
	bytecodes.emplace_back(Opcode::JUMP_IF_FALSE);
	size_t jumpIfFalseByte = bytecodes.size();
	put_opcode_u32(bytecodes, 0);
	// body
	body.putBytecodes(in_data, bytecodes);
	bytecodes.emplace_back(Opcode::JUMP);
	put_opcode_u32(bytecodes, continuePos);
	rewrite_opcode_u32(bytecodes, jumpIfFalseByte, bytecodes.size());
	breakPos = bytecodes.size();
}

#define operator_plus_case(type, op)                                           \
	case Lexer::TokenType::type: {                                             \
		auto detach = this->detach;                                            \
		if (detach->kind == NodeType::UNKNOW) {                                \
			detach = static_cast<UnknowNode *>(detach)->correctNode;           \
		}                                                                      \
		auto _node = static_cast<AccessNode *>(detach);                        \
		_node->isStore = false;                                                \
		_node->putBytecodes(in_data, bytecodes);                               \
		value->putBytecodes(in_data, bytecodes);                               \
		bytecodes.emplace_back(Opcode::op);                                    \
		return;                                                                \
	}

void SetNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
	switch (op) {
		operator_plus_case(PLUS_EQUAL, PLUS_EQUAL);
		operator_plus_case(MINUS_EQUAL, MINUS_EQUAL);
		operator_plus_case(STAR_EQUAL, MUL_EQUAL);
		operator_plus_case(SLASH_EQUAL, DIVIDE_EQUAL);
		default: {
			break;
			// throwError("Unexpected op "+ Lexer::Token(0,
			// op).toString(context));
		}
	}
	value->putBytecodes(in_data, bytecodes);
	detach->putBytecodes(in_data, bytecodes);
}

void CallNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
	auto* func = &compile.functions[funcId];
	auto* funcInfo = &context.functionInfo[funcId];
	if (caller) {
		caller->putBytecodes(in_data, bytecodes);
		if (accessNullable) {
			assert(context.jumpIfNullNode != nullptr);
			bytecodes.emplace_back(context.jumpIfNullNode->returnNullIfNull
			                           ? Opcode::JUMP_AND_SET_IF_NULL
			                           : Opcode::JUMP_AND_DELETE_IF_NULL);
			jumpIfNullPos = bytecodes.size();
			put_opcode_u32(bytecodes, 0);
		}
	}
	if (addPopBytecode)
		bytecodes.emplace_back(Opcode::POP);
	if (funcInfo->isConstructor) {
		if (isSuper) {
			bytecodes.emplace_back(Opcode::LOAD_LOCAL);
			put_opcode_u32(bytecodes, 0);
		} else {
			bytecodes.emplace_back(Opcode::CREATE_OBJECT);
			put_opcode_u32(bytecodes, classId);
			put_opcode_u32(bytecodes,
			               compile.classes[classId].memberMap.size());
		}
	}
	for (auto &argument : arguments) {
		argument->putBytecodes(in_data, bytecodes);
	}
	bytecodes.emplace_back(func->returnId == DefaultClass::nullClassId
	                           ? Opcode::CALL_VOID_FUNCTION
	                           : Opcode::CALL_FUNCTION);
	put_opcode_u32(bytecodes, funcId);
	// std::cout<<funcId<<'\n';
}

void CallNode::rewrite(in_func, std::vector<uint8_t> &bytecodes) {
	for (auto argument : arguments) {
		argument->rewrite(in_data, bytecodes);
	}
	if (context.jumpIfNullNode && caller) {
		caller->rewrite(in_data, bytecodes);
		if (accessNullable) {
			rewrite_opcode_u32(bytecodes, jumpIfNullPos,
			                   context.jumpIfNullNode->jumpIfNullPos);
		}
	}
}

void ReturnNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
	if (value) {
		if (value->kind == NodeType::VAR) {
			auto node = static_cast<VarNode *>(value);
			if (node->declaration->isGlobal)
				goto return_global;
			bytecodes.emplace_back(RETURN_LOCAL);
			put_opcode_u32(bytecodes, node->declaration->id);
			return;
		}
	return_global:;
		value->putBytecodes(in_data, bytecodes);
		bytecodes.emplace_back(Opcode::RETURN_VALUE);
		return;
	}
	bytecodes.emplace_back(Opcode::RETURN);
}

void VarNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
	if (declaration->isGlobal) {
		bytecodes.emplace_back(isStore ? Opcode::STORE_GLOBAL
		                               : Opcode::LOAD_GLOBAL);
	} else {
		bytecodes.emplace_back(isStore ? Opcode::STORE_LOCAL
		                               : Opcode::LOAD_LOCAL);
	}
	put_opcode_u32(bytecodes, declaration->id);
}

} // namespace AutoLang

#endif
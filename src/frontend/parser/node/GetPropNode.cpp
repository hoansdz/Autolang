#ifndef GET_PROP_NODE_CPP
#define GET_PROP_NODE_CPP

#include "Node.hpp"
#include "frontend/parser/ParserContext.hpp"
#include "shared/ClassFlags.hpp"

namespace AutoLang {

ExprNode *GetPropNode::resolve(in_func) {
	caller = static_cast<HasClassIdNode *>(caller->resolve(in_data));
	if (caller->kind == NodeType::CLASS_ACCESS) {
		auto *classInfo = context.classInfo[caller->classId];
		auto it = classInfo->staticMember.find(name);
		if (it == classInfo->staticMember.end()) {
			throwError("Cannot find static member name: '" + name + "'");
		}
		auto declarationNode = it->second;
		ExprNode::deleteNode(caller);
		return context.varPool.push(line, declarationNode, isStore, nullable);
	}
	return this;
}

void GetPropNode::optimize(in_func) {
	caller->optimize(in_data);
	if (caller->isNullable()) {
		if (!accessNullable)
			throwError(
			    "You can't use '.' with nullable value, you must use '?.'");
	} else {
		if (accessNullable) {
			warning(in_data,
			        "You should use '.' with non null value instead of '?.'");
			accessNullable = false;
		}
	}
	switch (caller->kind) {
		case NodeType::CALL:
		case NodeType::GET_PROP: {
			break;
		}
		case NodeType::VAR: {
			static_cast<VarNode *>(caller)->isStore = false;
			break;
		}
		default: {
			throwError("Cannot find caller");
		}
	}
	auto clazz = compile.classes[caller->classId];
	auto classInfo = context.classInfo[clazz->id];
	auto it = clazz->memberMap.find(name);
	if (it == clazz->memberMap.end()) {
		// Find static member
		auto it_ = classInfo->staticMember.find(name);
		if (it_ == classInfo->staticMember.end())
			throwError("Cannot find member name '" + name + "' in class " +
			           clazz->name);
		declaration = it_->second;
		if (declaration->accessModifier != Lexer::TokenType::PUBLIC &&
		    (!contextCallClassId || *contextCallClassId != clazz->id)) {
			throwError("Cannot access private -a member name '" + name + "'");
		}
		isStatic = true;
		isVal = declaration->isVal;
		id = declaration->id;
		classId = declaration->classId;
	}
	if (!isStatic) {
		// a.a = ...
		declaration = classInfo->member[it->second];
		isVal = !isInitial && declaration->isVal;
		if (declaration->accessModifier != Lexer::TokenType::PUBLIC &&
		    (!contextCallClassId || *contextCallClassId != clazz->id)) {
			throwError("Cannot access private member name '" + name + "'");
		}
		id = it->second;
		// for (int i = 0; i<clazz->memberId.size(); ++i) {
		// 	printDebug("MemId: "+std::to_string(clazz->memberId[i]));
		// }
		// printDebug("Class " + clazz->name + " GetProp: "+name+" "+" has:
		// "+std::to_string((uintptr_t)declarationNode));
		if (clazz->memberId[id] != declaration->classId)
			clazz->memberId[id] = declaration->classId;
		classId = declaration->classId; // clazz->memberId[id];
		// printDebug("Class " + clazz->name + " GetProp: " + name + " " +
		//            " has id: " + std::to_string(id) + " " +
		//            std::to_string(classId) + " " +
		//            compile.classes[classId]->name);
	}
	if (nullable) {
		nullable = declaration->nullable;
	}
}

ExprNode *GetPropNode::copy(in_func) {
	auto newNode = context.getPropPool.push(
	    line,
	    declaration ? static_cast<DeclarationNode *>(declaration->copy(in_data))
	                : nullptr,
	    contextCallClassId,
	    caller ? static_cast<HasClassIdNode *>(caller->copy(in_data)) : nullptr,
	    name, isInitial, nullable, accessNullable);
	newNode->isStore = isStore;
	return newNode;
}

void GetPropNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
	if (!isStatic) {
		switch (caller->kind) {
			case NodeType::VAR: {
				auto varNode = static_cast<VarNode *>(caller);
				if (!isStore) {
					if (varNode->isNullable()) {
						break;
					}
					bytecodes.emplace_back(varNode->declaration->isGlobal
					                           ? Opcode::GLOBAL_LOAD_MEMBER
					                           : Opcode::LOCAL_LOAD_MEMBER);
					put_opcode_u32(bytecodes, varNode->declaration->id);
					put_opcode_u32(bytecodes, id);
					return;
				}
				if (accessNullable) {
					throwError(
					    "Bug: Setnode not ensure store data is non nullable");
				}
				bytecodes.emplace_back(varNode->declaration->isGlobal
				                           ? Opcode::GLOBAL_LOAD_MEMBER_AND_STORE
				                           : Opcode::LOCAL_LOAD_MEMBER_AND_STORE);
				put_opcode_u32(bytecodes, varNode->declaration->id);
				put_opcode_u32(bytecodes, id);
				return;
			}
		}
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
			bytecodes.emplace_back(
			    context.jumpIfNullNode->returnNullIfNull
			        ? Opcode::LOAD_MEMBER_CAN_RET_NULL_OR_JUMP
			        : Opcode::LOAD_MEMBER_IF_NNULL_OR_JUMP);
			put_opcode_u32(bytecodes, id);
			jumpIfNullPos = bytecodes.size();
			put_opcode_u32(bytecodes, 0);
		} else {
			bytecodes.emplace_back(Opcode::LOAD_MEMBER);
			put_opcode_u32(bytecodes, id);
		}
		return;
	}
	switch (caller->kind) {
		case NodeType::VAR: {
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
		                     compile.classes[caller->classId]->name + "." +
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
		if (!isStatic && accessNullable && !isStore) {
			rewrite_opcode_u32(bytecodes, jumpIfNullPos,
			                   context.jumpIfNullNode->jumpIfNullPos);
		}
	}
}

GetPropNode::~GetPropNode() { deleteNode(caller); }

} // namespace AutoLang

#endif
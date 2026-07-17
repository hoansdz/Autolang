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
		{
			auto itLoadConst = classInfo->constValue.find(nameId);
			if (itLoadConst != classInfo->constValue.end()) {
				return itLoadConst->second;
			}
		}
		auto it = classInfo->staticMember.find(nameId);
		if (it != classInfo->staticMember.end()) {
			// throwError("Cannot find static member name: '" +
			//            context.lexerString[nameId] + "'");
			auto declarationNode = it->second;
			ExprNode::deleteNode(caller);
			return context.varPool.push(line, declarationNode, isStore,
			                            nullable);
		}
	}
	return this;
}

bool GetPropNode::optimizeSkipIfNotFoundMember(in_func) {
	caller->optimize(in_data);
	if (caller->isNullable()) {
		if (!accessNullable) {
			throwError(
			    "You can't use '.' with nullable value, you must use '?.'");
		}
	} else {
		if (accessNullable) {
			warning(in_data,
			        "You should use '.' with non null value instead of '?.'");
			accessNullable = false;
		}
	}
	switch (caller->kind) {
		case NodeType::CLASS_ACCESS: {
			isStatic = true;
			break;
		}
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
	const auto &name = context.lexerString[nameId];
	auto it = clazz->memberMap.find(name);
	if (it == clazz->memberMap.end()) {
		// Find static member
		auto it_ = classInfo->staticMember.find(nameId);
		if (it_ == classInfo->staticMember.end()) {
			return true;
		}
		declaration = it_->second;
		if (declaration->accessModifier != Lexer::TokenType::PUBLIC &&
		    (!contextCallClassId || *contextCallClassId != clazz->id)) {
			throwError("Cannot access private -a member name '" + name + "'");
		}
		isStatic = true;
		isVal = declaration->isVal;
		id = declaration->id;
		classId = declaration->classId;
		if (classId == DefaultClass::functionClassId) {
			classDeclaration = declaration->classDeclaration;
		}
	} else if (isStatic) {
		throwError("Cannot access non static member " + name);
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
		// printDebug("Class " + clazz->getName(compile) + " GetProp: "+name+" "+" has:
		// "+std::to_string((uintptr_t)declarationNode));
		classId = declaration->classId;
		if (classId == DefaultClass::functionClassId) {
			classDeclaration = declaration->classDeclaration;
		}
		// clazz->memberId[id];
		// printDebug("Class " + clazz->getName(compile) + " GetProp: " + name + " " +
		//            " has id: " + std::to_string(id) + " " +
		//            std::to_string(classId) + " " +
		//            compile.classes[classId]->getName(compile));
	}
	if (nullable) {
		nullable = declaration->nullable;
	}
	if (cloneable) {
		switch (classId) {
			case DefaultClass::intClassId:
			case DefaultClass::floatClassId: {
				break;
			}
			default: {
				cloneable = false;
				break;
			}
		}
	}
	return false;
}

void GetPropNode::optimize(in_func) {
	caller->optimize(in_data);
	if (caller->isNullable()) {
		if (!accessNullable) {
			throwError(
			    "You can't use '.' with nullable value, you must use '?.'");
		}
	} else {
		if (accessNullable) {
			warning(in_data,
			        "You should use '.' with non null value instead of '?.'");
			accessNullable = false;
		}
	}
	switch (caller->kind) {
		case NodeType::CLASS_ACCESS: {
			isStatic = true;
			break;
		}
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
	const auto &name = context.lexerString[nameId];
	auto it = clazz->memberMap.find(name);
	if (it == clazz->memberMap.end()) {
		// Find static member
		auto it_ = classInfo->staticMember.find(nameId);
		if (it_ == classInfo->staticMember.end())
			throwError("Cannot find member name '" + name + "' in class " +
			           clazz->getName(compile));
		declaration = it_->second;
		if (declaration->accessModifier != Lexer::TokenType::PUBLIC &&
		    (!contextCallClassId || *contextCallClassId != clazz->id)) {
			throwError("Cannot access private -a member name '" + name + "'");
		}
		isStatic = true;
		isVal = declaration->isVal;
		id = declaration->id;
		classId = declaration->classId;
		if (classId == DefaultClass::functionClassId) {
			classDeclaration = declaration->classDeclaration;
		}
	} else if (isStatic) {
		throwError("Cannot access non static member " + name);
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
		// printDebug("Class " + clazz->getName(compile) + " GetProp: "+name+" "+" has:
		// "+std::to_string((uintptr_t)declarationNode));
		classId = declaration->classId;
		if (classId == DefaultClass::functionClassId) {
			classDeclaration = declaration->classDeclaration;
		}
		// std::cerr << ("Class " + clazz->getName(compile) + " GetProp: " + name + " " +
		//               " has id: " + std::to_string(id) + " " +
		//               std::to_string(classId) + " " +
		//               compile.classes[classId]->getName(compile) +
		//               (declaration->nullable ? "?" : ""))
		//           << "\n";
	}
	if (nullable) {
		nullable = declaration->nullable;
	}
	if (cloneable) {
		switch (classId) {
			case DefaultClass::intClassId:
			case DefaultClass::floatClassId: {
				break;
			}
			default: {
				cloneable = false;
				break;
			}
		}
	}
}

ExprNode *GetPropNode::copy(in_func) {
	DeclarationNode *newDeclaration = nullptr;
	if (declaration) {
		auto funcInfo = context.getCurrentFunctionInfo(in_data);
		auto it = funcInfo->reflectDeclarationMap.find(declaration);
		if (it != funcInfo->reflectDeclarationMap.end()) {
			newDeclaration = it->second;
		} else {
			newDeclaration =
			    static_cast<DeclarationNode *>(declaration->copy(in_data));
		}
	}
	auto newCaller =
	    caller ? static_cast<HasClassIdNode *>(caller->copy(in_data)) : nullptr;
	auto newNode = context.getPropPool.push(
	    line, newDeclaration, contextCallClassId, newCaller, nameId, isInitial,
	    nullable, accessNullable);
	newNode->isStore = isStore;
	newNode->classId = classId;
	newNode->cloneable = cloneable;
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
					if (cloneable) {
						bytecodes.emplace_back(Opcode::CLONE);
					}
					return;
				}
				if (accessNullable) {
					throwError(
					    "Bug: Setnode not ensure store data is non nullable");
				}
				bytecodes.emplace_back(
				    varNode->declaration->isGlobal
				        ? Opcode::GLOBAL_LOAD_MEMBER_AND_STORE
				        : Opcode::LOCAL_LOAD_MEMBER_AND_STORE);
				put_opcode_u32(bytecodes, varNode->declaration->id);
				put_opcode_u32(bytecodes, id);
				return;
			}
			default:
				break;
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
			bytecodes.emplace_back(
			    context.jumpIfNullNode->returnNullIfNull
			        ? Opcode::LOAD_MEMBER_CAN_RET_NULL_OR_JUMP
			        : Opcode::LOAD_MEMBER_IF_NNULL_OR_JUMP);
			put_opcode_u32(bytecodes, id);
			jumpIfNullPos = bytecodes.size() - context.currentBytecodePos;
			put_opcode_u32(bytecodes, 0);
		} else {
			bytecodes.emplace_back(Opcode::LOAD_MEMBER);
			put_opcode_u32(bytecodes, id);
		}
		return;
	}
	caller->putBytecodesIfMustBeCalled(in_data, bytecodes);
	if (accessNullable) {
		if (isStore) {
			throwError("Bug: Setnode not ensure store data is non nullable");
		}
		warning(in_data, "Access static variables: we recommend call " +
		                     compile.classes[caller->classId]->getName(compile) + "." +
		                     context.lexerString[nameId]);
		accessNullable = false;
	}
	if (isStore) {
		bytecodes.emplace_back(Opcode::STORE_GLOBAL);
		put_opcode_u32(bytecodes, id);
	} else {
		bytecodes.emplace_back(Opcode::LOAD_GLOBAL);
		put_opcode_u32(bytecodes, id);
		if (cloneable) {
			bytecodes.emplace_back(Opcode::CLONE);
		}
	}
}

void GetPropNode::rewrite(in_func, uint8_t *bytecodes) {
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
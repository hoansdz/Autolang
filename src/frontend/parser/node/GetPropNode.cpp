#ifndef GET_PROP_NODE_CPP
#define GET_PROP_NODE_CPP

#include "Node.hpp"
#include "frontend/parser/ParserContext.hpp"
#include "shared/ClassFlags.hpp"
#include "shared/Type.hpp"
#include <rapidfuzz/fuzz.hpp>

namespace Autolang {

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
		case NodeType::IF:
		case NodeType::WHEN:
		case NodeType::CREATE_CLOSURE:
		case NodeType::FUNCTION_ACCESS:
		case NodeType::CONST_VAL:
		case NodeType::BINARY:
		case NodeType::CREATE_ARRAY:
		case NodeType::CREATE_MAP:
		case NodeType::CREATE_SET:
		case NodeType::NULL_COALESCING:
		case NodeType::OPTIONAL_ACCESS:
		case NodeType::UNARY:
		case NodeType::CAST:
		case NodeType::RUNTIME_CAST:
		case NodeType::CALL:
		case NodeType::GET_PROP: {
			break;
		}
		case NodeType::VAR: {
			static_cast<VarNode *>(caller)->isStore = false;
			break;
		}
		default: {
			throwError("Cannot resolve target expression for member access");
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
			throwError("Cannot access private member name '" + name + "'");
		}
		isStatic = true;
		isVal = declaration->isVal;
		id = declaration->id;
		classId = declaration->classId;
		if (classId == DefaultClass::functionClassId) {
			classDeclaration = declaration->classDeclaration;
		}
	} else if (isStatic) {
		throwError("Cannot access non-static member '" + name +
		           "' from static context");
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
		// printDebug("Class " + clazz->getName(compile) + " GetProp: "+name+"
		// "+" has:
		// "+std::to_string((uintptr_t)declarationNode));
		classId = declaration->classId;
		if (classId == DefaultClass::functionClassId) {
			classDeclaration = declaration->classDeclaration;
		}
		// clazz->memberId[id];
		// printDebug("Class " + clazz->getName(compile) + " GetProp: " + name +
		// " " +
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
		case NodeType::IF:
		case NodeType::WHEN:
		case NodeType::CREATE_CLOSURE:
		case NodeType::FUNCTION_ACCESS:
		case NodeType::CONST_VAL:
		case NodeType::BINARY:
		case NodeType::CREATE_ARRAY:
		case NodeType::CREATE_MAP:
		case NodeType::CREATE_SET:
		case NodeType::NULL_COALESCING:
		case NodeType::OPTIONAL_ACCESS:
		case NodeType::UNARY:
		case NodeType::CAST:
		case NodeType::RUNTIME_CAST:
		case NodeType::CALL:
		case NodeType::GET_PROP: {
			break;
		}
		case NodeType::VAR: {
			static_cast<VarNode *>(caller)->isStore = false;
			break;
		}
		default: {
			throwError("Cannot resolve target expression for member access");
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
			std::string foundMembers;
			bool hasMember = false;
			for (auto *decl : classInfo->member) {
				if (decl) {
					if (hasMember) {
						foundMembers += "\n";
					}
					foundMembers += decl->toString(in_data, false);
					hasMember = true;
				}
			}
			for (const auto &pair : classInfo->staticMember) {
				if (pair.second) {
					if (hasMember) {
						foundMembers += "\n";
					}
					foundMembers += pair.second->toString(in_data, true);
					hasMember = true;
				}
			}
			std::string foundFunctions;
			bool hasFunction = false;
			for (const auto &[_, funcVec] : classInfo->allFunction) {
				for (FunctionId funcId : funcVec) {
					auto funcInfo = context.functionInfo[funcId];
					if (hasFunction) {
						foundFunctions += "\n";
					}
					foundFunctions += funcInfo->toString(in_data);
					hasFunction = true;
				}
			}
			std::string bestSuggestion;
			double bestScore = 0.0;
			auto checkSuggestion = [&](const std::string &candidate) {
				double score = rapidfuzz::fuzz::ratio(name, candidate);
				if (score > bestScore && score >= 60.0) {
					bestScore = score;
					bestSuggestion = candidate;
				}
			};
			for (auto *decl : classInfo->member) {
				if (decl) {
					checkSuggestion(decl->name);
				}
			}
			for (const auto &pair : classInfo->staticMember) {
				if (pair.second) {
					checkSuggestion(pair.second->name);
				}
			}
			for (const auto &[funcNameId, _] : classInfo->allFunction) {
				checkSuggestion(context.lexerString[funcNameId]);
			}

			std::string errorMsg = "Cannot find member name '" + name +
			                       "' in class '" + clazz->getName(compile) +
			                       "'";
			if (!bestSuggestion.empty()) {
				errorMsg += "\nDid you mean: '" + bestSuggestion + "'?";
			}
			if (hasMember) {
				errorMsg += "\nAvailable members in '" +
				            clazz->getName(compile) + "':\n" + foundMembers;
			} else {
				errorMsg += "\n(No members declared in class '" +
				            clazz->getName(compile) + "')";
			}
			if (hasFunction) {
				errorMsg += "\nAvailable functions in '" +
				            clazz->getName(compile) + "':\n" + foundFunctions;
			}
			errorMsg += "\nHint: Check member name spelling or verify whether "
			            "it is declared in class '" +
			            clazz->getName(compile) + "'.";
			throwError(errorMsg);
		}
		declaration = it_->second;
		if (declaration->accessModifier != Lexer::TokenType::PUBLIC &&
		    (!contextCallClassId || *contextCallClassId != clazz->id)) {
			throwError("Cannot access private member name '" + name + "'");
		}
		isStatic = true;
		isVal = declaration->isVal;
		id = declaration->id;
		classId = declaration->classId;
		if (classId == DefaultClass::functionClassId) {
			classDeclaration = declaration->classDeclaration;
		}
	} else if (isStatic) {
		throwError("Cannot access non-static member '" + name +
		           "' from static context");
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
		// printDebug("Class " + clazz->getName(compile) + " GetProp: "+name+"
		// "+" has:
		// "+std::to_string((uintptr_t)declarationNode));
		classId = declaration->classId;
		if (classId == DefaultClass::functionClassId) {
			classDeclaration = declaration->classDeclaration;
		}
		// std::cerr << ("Class " + clazz->getName(compile) + " GetProp: " +
		// name + " " +
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
	newNode->isForceNonNull = isForceNonNull;
	newNode->cloneable = cloneable;
	return newNode;
}

void GetPropNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
	loadOpcodeLine(in_data, bytecodes);
	if (!isStatic) {
		switch (caller->kind) {
			case NodeType::VAR: {
				auto varNode = static_cast<VarNode *>(caller);
				if (!isStore) {
					if (varNode->isNullable() || varNode->isForceNonNull) {
						break;
					}
					if (declaration->isLateInit) {
						bytecodes.emplace_back(
						    varNode->declaration->isGlobal
						        ? Opcode::GLOBAL_LOAD_LATEINIT_MEMBER
						        : Opcode::LOCAL_LOAD_LATEINIT_MEMBER);
					} else {
						bytecodes.emplace_back(varNode->declaration->isGlobal
						                           ? Opcode::GLOBAL_LOAD_MEMBER
						                           : Opcode::LOCAL_LOAD_MEMBER);
					}
					put_opcode_u32(bytecodes, varNode->declaration->id);
					put_opcode_u32(bytecodes, id);
					if (isForceNonNull) {
						bytecodes.emplace_back(Opcode::CHECK_FORCE_NON_NULL);
					}
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
			if (declaration->isLateInit) {
				bytecodes.emplace_back(Opcode::LOAD_LATEINIT_MEMBER);
			} else {
				bytecodes.emplace_back(Opcode::LOAD_MEMBER);
			}
			put_opcode_u32(bytecodes, id);
			if (isForceNonNull) {
				bytecodes.emplace_back(Opcode::CHECK_FORCE_NON_NULL);
			}
		}
		return;
	}
	caller->putBytecodesIfMustBeCalled(in_data, bytecodes);
	if (accessNullable) {
		if (isStore) {
			throwError("Bug: Setnode not ensure store data is non nullable");
		}
		warning(in_data,
		        "Access static variables: we recommend call " +
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
		if (isForceNonNull) {
			bytecodes.emplace_back(Opcode::CHECK_FORCE_NON_NULL);
		}
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

} // namespace Autolang

#endif
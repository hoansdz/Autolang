#ifndef IF_NODE_CPP
#define IF_NODE_CPP

#include "Node.hpp"
#include "frontend/parser/ParserContext.hpp"

namespace Autolang {

void SmartCastInfo::apply() {
	if (!declaration) return;
	originalClassId = declaration->classId;
	originalClassDeclaration = declaration->classDeclaration;
	originalNullable = declaration->nullable;

	if (hasTargetClass) {
		declaration->classId = targetClassId;
		declaration->classDeclaration = targetClassDeclaration;
	}
	declaration->nullable = targetNullable;
}

void SmartCastInfo::restore() {
	if (!declaration) return;
	declaration->classId = originalClassId;
	declaration->classDeclaration = originalClassDeclaration;
	declaration->nullable = originalNullable;
}

static DeclarationNode *extractDeclaration(in_func, HasClassIdNode *node) {
	if (!node) return nullptr;
	if (node->kind == NodeType::VAR || node->kind == NodeType::GET_PROP) {
		return static_cast<AccessNode *>(node)->declaration;
	}
	if (node->kind == NodeType::UNKNOW) {
		auto unknow = static_cast<UnknowNode *>(node);
		auto found = context.findDeclaration(in_data, unknow->line, unknow->nameId, true);
		if (found && (found->kind == NodeType::VAR || found->kind == NodeType::GET_PROP)) {
			return static_cast<AccessNode *>(found)->declaration;
		}
	}
	return nullptr;
}

static bool isNullNode(in_func, HasClassIdNode *node) {
	if (!node) return false;
	if (node->kind == NodeType::CONST_VAL && node->classId == DefaultClass::nullClassId) {
		return true;
	}
	if (node->kind == NodeType::UNKNOW) {
		auto unknow = static_cast<UnknowNode *>(node);
		if (unknow->nameId == lexerIdnull) {
			return true;
		}
	}
	return false;
}

static bool extractTargetClass(in_func, HasClassIdNode *right, ClassId &outClassId, ClassDeclaration *&outDecl) {
	if (!right) return false;
	if (right->kind == NodeType::CLASS_ACCESS) {
		auto ca = static_cast<ClassAccessNode *>(right);
		outClassId = ca->classId;
		outDecl = context.classDeclarationAllocator.push();
		outDecl->line = ca->line;
		outDecl->classId = ca->classId;
		return true;
	}
	if (right->kind == NodeType::UNKNOW) {
		auto unknow = static_cast<UnknowNode *>(right);
		LexerStringId nameId = unknow->nameId;
		auto itDef = context.defaultClassMap.find(nameId);
		if (itDef != context.defaultClassMap.end()) {
			outClassId = itDef->second;
			outDecl = context.classDeclarationAllocator.push();
			outDecl->line = unknow->line;
			outDecl->baseClassLexerStringId = nameId;
			outDecl->classId = outClassId;
			return true;
		}
		auto itAlias = context.typealiasMap.find(nameId);
		if (itAlias != context.typealiasMap.end()) {
			if (itAlias->second->classDeclaration && itAlias->second->classDeclaration->classId) {
				outClassId = *itAlias->second->classDeclaration->classId;
				outDecl = itAlias->second->classDeclaration;
				return true;
			}
		}
		auto itCls = compile.classMap.find(std::string(context.lexerString[nameId]));
		if (itCls != compile.classMap.end()) {
			outClassId = itCls->second;
			outDecl = context.classDeclarationAllocator.push();
			outDecl->line = unknow->line;
			outDecl->baseClassLexerStringId = nameId;
			outDecl->classId = outClassId;
			return true;
		}
	}
	return false;
}

static void addSmartCast(SmallVector<SmartCastInfo, 2> &casts, DeclarationNode *decl,
                         bool nullable, bool hasClass = false, ClassId classId = 0,
                         ClassDeclaration *classDecl = nullptr) {
	if (!decl) return;
	for (auto &c : casts) {
		if (c.declaration == decl) {
			c.targetNullable = nullable;
			if (hasClass) {
				c.hasTargetClass = true;
				c.targetClassId = classId;
				c.targetClassDeclaration = classDecl;
			}
			return;
		}
	}
	SmartCastInfo info;
	info.declaration = decl;
	info.targetNullable = nullable;
	info.hasTargetClass = hasClass;
	info.targetClassId = classId;
	info.targetClassDeclaration = classDecl;
	casts.push_back(info);
}

void extractSmartCasts(in_func, HasClassIdNode *cond,
                       SmallVector<SmartCastInfo, 2> &trueCasts,
                       SmallVector<SmartCastInfo, 2> &falseCasts) {
	if (!cond) return;
	if (cond->kind == NodeType::BINARY) {
		auto binary = static_cast<BinaryNode *>(cond);
		if (binary->op == Lexer::TokenType::AND_AND || binary->op == Lexer::TokenType::AND) {
			SmallVector<SmartCastInfo, 2> dummy1, dummy2;
			extractSmartCasts(in_data, binary->left, trueCasts, dummy1);
			extractSmartCasts(in_data, binary->right, trueCasts, dummy2);
			return;
		}
		if (binary->op == Lexer::TokenType::OR_OR || binary->op == Lexer::TokenType::OR) {
			SmallVector<SmartCastInfo, 2> dummy1, dummy2;
			extractSmartCasts(in_data, binary->left, dummy1, falseCasts);
			extractSmartCasts(in_data, binary->right, dummy2, falseCasts);
			return;
		}
		if (binary->op == Lexer::TokenType::NOTEQ || binary->op == Lexer::TokenType::NOTEQEQ) {
			if (isNullNode(in_data, binary->right)) {
				auto decl = extractDeclaration(in_data, binary->left);
				if (decl) addSmartCast(trueCasts, decl, false);
			} else if (isNullNode(in_data, binary->left)) {
				auto decl = extractDeclaration(in_data, binary->right);
				if (decl) addSmartCast(trueCasts, decl, false);
			}
			return;
		}
		if (binary->op == Lexer::TokenType::EQEQ || binary->op == Lexer::TokenType::EQEQEQ) {
			if (isNullNode(in_data, binary->right)) {
				auto decl = extractDeclaration(in_data, binary->left);
				if (decl) addSmartCast(falseCasts, decl, false);
			} else if (isNullNode(in_data, binary->left)) {
				auto decl = extractDeclaration(in_data, binary->right);
				if (decl) addSmartCast(falseCasts, decl, false);
			}
			return;
		}
		if (binary->op == Lexer::TokenType::IS) {
			auto decl = extractDeclaration(in_data, binary->left);
			ClassId cid = 0;
			ClassDeclaration *cdecl = nullptr;
			if (decl && extractTargetClass(in_data, binary->right, cid, cdecl)) {
				addSmartCast(trueCasts, decl, false, true, cid, cdecl);
			}
			return;
		}
		if (binary->op == Lexer::TokenType::NOT_IS) {
			auto decl = extractDeclaration(in_data, binary->left);
			ClassId cid = 0;
			ClassDeclaration *cdecl = nullptr;
			if (decl && extractTargetClass(in_data, binary->right, cid, cdecl)) {
				addSmartCast(falseCasts, decl, false, true, cid, cdecl);
			}
			return;
		}
	} else if (cond->kind == NodeType::UNARY) {
		auto unary = static_cast<UnaryNode *>(cond);
		if (unary->op == Lexer::TokenType::EXMARK || unary->op == Lexer::TokenType::NOT) {
			extractSmartCasts(in_data, unary->value, falseCasts, trueCasts);
			return;
		}
	}
}

ExprNode *IfNode::resolve(in_func) {
	condition = static_cast<HasClassIdNode *>(condition->resolve(in_data));
	// if (condition->kind == NodeType::CONST_VAL) {
	// 	if (static_cast<ConstValueNode *>(condition)->classId !=
	// 	    Autolang::DefaultClass::boolClassId) {
	// 		throwError("Cannot use expression of type '" +
	// 		           condition->getClassName(in_data) +
	// 		           "' as a condition, expected 'Bool'");
	// 	}
	// 	// Is bool because optimize forbiddened others
	// 	if (static_cast<ConstValueNode *>(condition)->obj->b) {
	// 		if (ifFalse) {
	// 			warning(in_data, "Else body will never be used");
	// 		}
	// 		auto result = context.blockNodePool.push(ifTrue.line);
	// 		result->nodes = std::move(ifTrue.nodes);
	// 		result->resolve(in_data);
	// 		ExprNode::deleteNode(this);
	// 		return result;
	// 	} else if (ifFalse) {
	// 		auto result = ifFalse;
	// 		result->resolve(in_data);
	// 		ifFalse = nullptr;
	// 		ExprNode::deleteNode(this);
	// 		return result;
	// 	}
	// 	return this;
	// }
	for (auto &cast : trueCasts) cast.apply();
	ifTrue.resolve(in_data);
	for (auto &cast : trueCasts) cast.restore();
	if (!ifTrue.nodes.empty()) {
		auto lastKind = ifTrue.nodes.back()->kind;
		if (lastKind == NodeType::RET || lastKind == NodeType::THROW || lastKind == NodeType::SKIP) {
			trueBranchReturns = true;
		}
	}
	if (ifFalse) {
		for (auto &cast : falseCasts) cast.apply();
		ifFalse->resolve(in_data);
		for (auto &cast : falseCasts) cast.restore();
		if (!ifFalse->nodes.empty()) {
			auto falseLastKind = ifFalse->nodes.back()->kind;
			if (falseLastKind == NodeType::RET || falseLastKind == NodeType::THROW || falseLastKind == NodeType::SKIP) {
				falseBranchReturns = true;
			}
		}
	}
	return this;
}

ExprNode *IfNode::optimize(in_func) {
	condition = static_cast<HasClassIdNode *>(condition->optimize(in_data));
	switch (condition->classId) {
		case Autolang::DefaultClass::intClassId: {
			auto zeroNode =
			    context.constValuePool.push(condition->line, (int64_t)0);
			auto binaryNode = context.binaryNodePool.push(
			    condition->line, 0, context.currentClassId,
			    Lexer::TokenType::NOTEQ, condition, zeroNode);
			auto resolved = binaryNode->resolve(in_data);
			condition =
			    static_cast<HasClassIdNode *>(resolved->optimize(in_data));
			break;
		}
		case Autolang::DefaultClass::floatClassId: {
			auto zeroNode =
			    context.constValuePool.push(condition->line, (double)0.0);
			auto binaryNode = context.binaryNodePool.push(
			    condition->line, 0, context.currentClassId,
			    Lexer::TokenType::NOTEQ, condition, zeroNode);
			auto resolved = binaryNode->resolve(in_data);
			condition =
			    static_cast<HasClassIdNode *>(resolved->optimize(in_data));
			break;
		}
		default:
			break;
	}
	if (condition->classId != Autolang::DefaultClass::boolClassId)
		throwError("Cannot use expression of type '" +
		           condition->getClassName(in_data) +
		           "' as a condition, expected 'Bool'\nHint: Ensure the condition expression evaluates to a 'Bool' value or explicit boolean comparison.");

	auto lastMustReturnValueNode = context.mustReturnValueNode;
	bool loadReturnBlock = mustReturnValue || lastMustReturnValueNode;
	if (loadReturnBlock) {
		context.mustReturnValueNode = this;
	}
	if (!ifFalse || mustReturnValue) {
		for (auto &cast : trueCasts) cast.apply();
		ifTrue.optimize(in_data);
		for (auto &cast : trueCasts) cast.restore();
		ClassId trueClassId = classId;
		if (ifFalse) {
			for (auto &cast : falseCasts) cast.apply();
			ifFalse = static_cast<BlockNode *>(ifFalse->optimize(in_data));
			for (auto &cast : falseCasts) cast.restore();
		} else {
			if (loadReturnBlock && ifTrue.hasValue() && condition->kind == NodeType::CONST_VAL &&
			    static_cast<ConstValueNode *>(condition)->obj->b) {
				mustReturnValue = true;
			}
		}
		if (classId == DefaultClass::floatClassId &&
			trueClassId == DefaultClass::intClassId) {
			ifTrue.autoCastToFloat = true;
		}
	} else {
		for (auto &cast : trueCasts) cast.apply();
		ifTrue.optimize(in_data);
		for (auto &cast : trueCasts) cast.restore();
		ClassId trueClassId = classId;
		if (ifFalse) {
			for (auto &cast : falseCasts) cast.apply();
			ifFalse = static_cast<BlockNode *>(ifFalse->optimize(in_data));
			for (auto &cast : falseCasts) cast.restore();
		}
		if (classId == DefaultClass::floatClassId &&
		    trueClassId == DefaultClass::intClassId) {
			ifTrue.autoCastToFloat = true;
		}
		bool trueEndsEarly = !ifTrue.nodes.empty() && (ifTrue.nodes.back()->kind == NodeType::THROW || ifTrue.nodes.back()->kind == NodeType::RET || ifTrue.nodes.back()->kind == NodeType::SKIP);
		bool falseEndsEarly = ifFalse && !ifFalse->nodes.empty() && (ifFalse->nodes.back()->kind == NodeType::THROW || ifFalse->nodes.back()->kind == NodeType::RET || ifFalse->nodes.back()->kind == NodeType::SKIP);
		if (loadReturnBlock && (ifTrue.hasValue() || trueEndsEarly) && (ifFalse && (ifFalse->hasValue() || falseEndsEarly)) && (ifTrue.hasValue() || (ifFalse && ifFalse->hasValue()))) {
			mustReturnValue = true;
			if (trueEndsEarly && ifFalse && ifFalse->hasValue()) {
				classId = ifFalse->classId;
				classDeclaration = ifFalse->classDeclaration;
				nullable = ifFalse->isNullable();
			} else if (falseEndsEarly && ifTrue.hasValue()) {
				classId = ifTrue.classId;
				classDeclaration = ifTrue.classDeclaration;
				nullable = ifTrue.isNullable();
			}
		}
		if (classId == DefaultClass::anyClassId || (!classDeclaration && classId != DefaultClass::nullClassId)) {
			classDeclaration = ExprNode::getOrCreateClassDeclaration(in_data, classId, line, nullable);
		}
	}
	// std::cerr << getClassName(in_data) << "\n";
	if (loadReturnBlock) {
		if (classId == DefaultClass::nullClassId && nullable) {
			throwError("Cannot infer return type for 'if' expression because "
			           "its body is a null literal\nHint: Explicitly specify the type or ensure the expression body returns a concrete typed value.");
		}
		context.mustReturnValueNode = lastMustReturnValueNode;
	}
	return this;
}

ExprNode *IfNode::copy(in_func) {
	auto newNode = context.ifPool.push(line, mustReturnValue);
	newNode->condition =
	    static_cast<HasClassIdNode *>(condition->copy(in_data));
	newNode->trueCasts = trueCasts;
	newNode->falseCasts = falseCasts;
	newNode->trueBranchReturns = trueBranchReturns;
	newNode->falseBranchReturns = falseBranchReturns;
	auto funcInfo = context.getCurrentFunctionInfo(in_data);
	for (auto &cast : newNode->trueCasts) {
		auto it = funcInfo->reflectDeclarationMap.find(cast.declaration);
		if (it != funcInfo->reflectDeclarationMap.end()) {
			cast.declaration = it->second;
		}
	}
	for (auto &cast : newNode->falseCasts) {
		auto it = funcInfo->reflectDeclarationMap.find(cast.declaration);
		if (it != funcInfo->reflectDeclarationMap.end()) {
			cast.declaration = it->second;
		}
	}
	newNode->ifTrue.nodes.reserve(ifTrue.nodes.size());
	for (auto node : ifTrue.nodes) {
		newNode->ifTrue.nodes.push_back(node->copy(in_data));
	}
	if (ifFalse) {
		newNode->ifFalse = static_cast<BlockNode *>(ifFalse->copy(in_data));
	}
	return newNode;
}

void IfNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
	loadOpcodeLine(in_data, bytecodes);
	condition->putBytecodes(in_data, bytecodes);
	bytecodes.emplace_back(Opcode::JUMP_IF_FALSE);
	size_t jumpIfFalseByte = bytecodes.size() - context.currentBytecodePos;
	put_opcode_u32(bytecodes, 0);
	auto lastMustReturnValueNode = context.mustReturnValueNode;
	if (mustReturnValue) {
		context.mustReturnValueNode = this;
	} else {
		context.mustReturnValueNode = nullptr;
	}
	ifTrue.putBytecodes(in_data, bytecodes);
	if (ifFalse) {
		bytecodes.emplace_back(Opcode::JUMP);
		size_t jumpIfTrueByte = bytecodes.size() - context.currentBytecodePos;
		put_opcode_u32(bytecodes, 0);
		rewrite_opcode_u32(bytecodes.data() + context.currentBytecodePos,
		                   jumpIfFalseByte,
		                   bytecodes.size() - context.currentBytecodePos);
		ifFalse->putBytecodes(in_data, bytecodes);
		rewrite_opcode_u32(bytecodes.data() + context.currentBytecodePos,
		                   jumpIfTrueByte,
		                   bytecodes.size() - context.currentBytecodePos);
	} else {
		rewrite_opcode_u32(bytecodes.data() + context.currentBytecodePos,
		                   jumpIfFalseByte,
		                   bytecodes.size() - context.currentBytecodePos);
	}
	context.mustReturnValueNode = lastMustReturnValueNode;
	BytecodePos endBlock = bytecodes.size() - context.currentBytecodePos;
	for (auto pos : jumpPosition) {
		rewrite_opcode_u32(bytecodes.data() + context.currentBytecodePos, pos,
		                   endBlock);
	}
}

void IfNode::rewrite(in_func, uint8_t *bytecodes) {
	ifTrue.rewrite(in_data, bytecodes);
	if (ifFalse)
		ifFalse->rewrite(in_data, bytecodes);
}

IfNode::~IfNode() {
	deleteNode(condition);
	deleteNode(ifFalse);
}

} // namespace Autolang

#endif
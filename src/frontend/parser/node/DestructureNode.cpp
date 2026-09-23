#ifndef DESTRUCTURE_NODE_CPP
#define DESTRUCTURE_NODE_CPP

#include "Node.hpp"
#include "frontend/parser/ClassDeclaration.hpp"
#include "frontend/parser/ParserContext.hpp"
#include "shared/DefaultClass.hpp"

namespace Autolang {

ExprNode *DestructureNode::resolve(in_func) {
	sourceExpr = static_cast<HasClassIdNode *>(sourceExpr->resolve(in_data));
	return this;
}

ExprNode *DestructureNode::optimize(in_func) {
	if (isResolved) {
		return innerBlock.optimize(in_data);
	}
	sourceExpr = static_cast<HasClassIdNode *>(sourceExpr->optimize(in_data));

	ClassId srcClassId = sourceExpr->classId;
	if (srcClassId == DefaultClass::nullClassId || srcClassId == DefaultClass::voidClassId) {
		throwError("Cannot destructure expression of type " +
		           compile.classes[srcClassId]->getName(compile));
	}

	auto *clazz = compile.classes[srcClassId];
	auto *classInfo = context.classInfo[srcClassId];

	std::string tempName = "__destruct_" + std::to_string(line) + "_" + std::to_string((uintptr_t)this);
	LexerStringId tempNameId = context.createLexerStringIfNotExists(tempName);
	bool isGlobal = context.currentFunctionId == context.mainFunctionId &&
	                !context.currentClosureNode;
	auto *tempDecl = context.makeDeclarationNode(
	    in_data, line, tempNameId, context.lexerString[tempNameId], nullptr,
	    true, isGlobal, sourceExpr->isNullable(), !isGlobal, true);
	tempDecl->classId = srcClassId;
	tempDecl->nullable = sourceExpr->isNullable();
	tempDecl->classDeclaration = sourceExpr->classDeclaration;

	auto *setTemp = context.setValuePool.push(
	    line, context.varPool.push(line, tempDecl, true, true), sourceExpr, false);
	innerBlock.nodes.push_back(setTemp);

	std::vector<LexerStringId> propNames;
	for (auto *memberDecl : classInfo->member) {
		if (memberDecl) {
			propNames.push_back(memberDecl->baseName);
		}
	}
	std::string className = clazz->getName(compile);

	if (targets.size() > propNames.size()) {
		throwError("Destructuring declaration has " + std::to_string(targets.size()) +
		           " variables, but type '" + std::string(className) +
		           "' only has " + std::to_string(propNames.size()) +
		           " properties\nHint: Check number of destructured variables");
	}

	for (size_t k = 0; k < targets.size(); ++k) {
		auto *target = targets[k];
		if (!target) {
			continue;
		}
		LexerStringId propId = propNames[k];
		auto *varTemp = context.varPool.push(line, tempDecl, false, false);
		auto *getProp = context.getPropPool.push(
		    line, nullptr, context.currentClassId, varTemp, propId, false, false, false);
		auto *optimizedProp = static_cast<HasClassIdNode *>(getProp->optimize(in_data));

		target->classId = optimizedProp->classId;
		target->nullable = optimizedProp->isNullable();
		target->classDeclaration = optimizedProp->classDeclaration;

		auto *setTarget = context.setValuePool.push(
		    line, context.varPool.push(line, target, true, true), optimizedProp, false);
		innerBlock.nodes.push_back(setTarget);
	}

	isResolved = true;
	innerBlock.optimize(in_data);
	return this;
}

void DestructureNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
	innerBlock.putBytecodes(in_data, bytecodes);
}

void DestructureNode::rewrite(in_func, uint8_t *bytecodes) {
	innerBlock.rewrite(in_data, bytecodes);
}

ExprNode *DestructureNode::copy(in_func) {
	auto *newNode = context.destructurePool.push(
	    line, static_cast<HasClassIdNode *>(sourceExpr->copy(in_data)), targets);
	return newNode;
}

DestructureNode::~DestructureNode() {
	if (!isResolved) {
		deleteNode(sourceExpr);
	}
}

} // namespace Autolang

#endif

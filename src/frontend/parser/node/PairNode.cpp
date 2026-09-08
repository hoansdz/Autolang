#ifndef PAIR_NODE_CPP
#define PAIR_NODE_CPP

#include "Node.hpp"
#include "frontend/parser/ClassDeclaration.hpp"
#include "frontend/parser/ParserContext.hpp"
#include "shared/DefaultClass.hpp"

namespace Autolang {

ExprNode *PairNode::resolve(in_func) {
	first = static_cast<HasClassIdNode *>(first->resolve(in_data));
	second = static_cast<HasClassIdNode *>(second->resolve(in_data));
	return this;
}

ExprNode *PairNode::optimize(in_func) {
	first = static_cast<HasClassIdNode *>(first->optimize(in_data));
	second = static_cast<HasClassIdNode *>(second->optimize(in_data));

	ClassDeclaration *firstDecl = first->classDeclaration;
	if (!firstDecl) {
		firstDecl = context.classDeclarationAllocator.push();
		firstDecl->classId = first->classId;
		firstDecl->line = line;
		firstDecl->isGeneric = false;
		firstDecl->nullable = first->isNullable();
	}

	ClassDeclaration *secondDecl = second->classDeclaration;
	if (!secondDecl) {
		secondDecl = context.classDeclarationAllocator.push();
		secondDecl->classId = second->classId;
		secondDecl->line = line;
		secondDecl->isGeneric = false;
		secondDecl->nullable = second->isNullable();
	}

	classDeclaration = context.classDeclarationAllocator.push();
	classDeclaration->baseClassLexerStringId = lexerIdPair;
	classDeclaration->inputClassId = {firstDecl, secondDecl};
	classDeclaration->line = line;
	classDeclaration->isGeneric = false;
	classDeclaration->template load<true, false, true>(in_data);
	if (!classDeclaration->classId) {
		throwError("Cannot resolve Pair class for types (" +
		           compile.classes[first->classId]->getName(compile) + ", " +
		           compile.classes[second->classId]->getName(compile) +
		           ")\nHint: Ensure Pair<A, B> is available.");
	}
	classId = *classDeclaration->classId;
	auto classInfo = context.classInfo[classId];
	if (classInfo->primaryConstructor) {
		constructorFuncId = classInfo->primaryConstructor->funcId;
	} else {
		throwError("Pair class does not have primary constructor");
	}
	return this;
}

void PairNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
	loadOpcodeLine(in_data, bytecodes);
	bytecodes.emplace_back(Opcode::CREATE_OBJECT);
	put_opcode_u32(bytecodes, classId);
	put_opcode_u32(bytecodes, compile.classes[classId]->memberMap.size());
	first->putBytecodes(in_data, bytecodes);
	second->putBytecodes(in_data, bytecodes);
	auto func = compile.functions[constructorFuncId];
	if (func->functionFlags & FunctionFlags::FUNC_IS_DATA_CONSTRUCTOR) {
		bytecodes.emplace_back(Opcode::CALL_DATA_CONTRUCTOR);
	} else {
		bytecodes.emplace_back(Opcode::CALL_FUNCTION);
	}
	put_opcode_u32(bytecodes, constructorFuncId);
}

void PairNode::rewrite(in_func, uint8_t *bytecodes) {
	first->rewrite(in_data, bytecodes);
	second->rewrite(in_data, bytecodes);
}

ExprNode *PairNode::copy(in_func) {
	auto newNode = context.pairPool.push(
	    line, static_cast<HasClassIdNode *>(first->copy(in_data)),
	    static_cast<HasClassIdNode *>(second->copy(in_data)));
	newNode->classDeclaration = classDeclaration;
	newNode->classId = classId;
	newNode->constructorFuncId = constructorFuncId;
	return newNode;
}

PairNode::~PairNode() {
	deleteNode(first);
	deleteNode(second);
}

} // namespace Autolang

#endif

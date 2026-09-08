#ifndef TRY_CATCH_CPP
#define TRY_CATCH_CPP

#include "Node.hpp"
#include "frontend/parser/ParserContext.hpp"

namespace Autolang {

ExprNode *TryCatchNode::resolve(in_func) {
	body.resolve(in_data);
	for (auto &clause : catchClauses) {
		clause.body.resolve(in_data);
	}
	if (hasFinally) {
		finallyBody.resolve(in_data);
	}
	return this;
}

ExprNode *TryCatchNode::optimize(in_func) {
	for (auto &clause : catchClauses) {
		if (clause.classDeclaration) {
			if (!clause.classDeclaration->classId) {
				clause.classDeclaration->template load<true>(in_data);
			}
			if (clause.classDeclaration->classId) {
				clause.exceptionClassId = *clause.classDeclaration->classId;
			}
			auto excClass = compile.classes[clause.exceptionClassId];
			if (!(clause.exceptionClassId == DefaultClass::exceptionClassId ||
			      excClass->inheritance.get(DefaultClass::exceptionClassId))) {
				throwError("Catch parameter type must inherit from Exception (got '" +
				           excClass->getName(compile) +
				           "')\nHint: Ensure the caught exception class inherits from Exception.");
			}
			if (clause.exceptionDeclaration) {
				clause.exceptionDeclaration->classId = clause.exceptionClassId;
			}
		}
	}

	auto lastMustReturnValueNode = context.mustReturnValueNode;
	if (mustReturnValue) {
		context.mustReturnValueNode = this;
		body.optimize(in_data);
		ClassId bodyClassId = classId;
		for (auto &clause : catchClauses) {
			context.mustReturnValueNode = this;
			clause.body.optimize(in_data);
		}
		if (hasFinally) {
			context.mustReturnValueNode = nullptr;
			finallyBody.optimize(in_data);
		}
		if (classId == DefaultClass::floatClassId &&
		    bodyClassId == DefaultClass::intClassId) {
			body.autoCastToFloat = true;
		}
		bool bodyEndsEarly = !body.nodes.empty() &&
		                     (body.nodes.back()->kind == NodeType::THROW ||
		                      body.nodes.back()->kind == NodeType::RET);
		bool allCatchesEndEarly = hasCatch && !catchClauses.empty();
		CatchClause *firstCatchWithValue = nullptr;
		for (auto &clause : catchClauses) {
			bool clauseEndsEarly = !clause.body.nodes.empty() &&
			                       (clause.body.nodes.back()->kind == NodeType::THROW ||
			                        clause.body.nodes.back()->kind == NodeType::RET);
			if (!clauseEndsEarly) {
				allCatchesEndEarly = false;
				if (clause.body.hasValue() && !firstCatchWithValue) {
					firstCatchWithValue = &clause;
				}
			}
		}
		if (bodyEndsEarly && firstCatchWithValue) {
			classId = firstCatchWithValue->body.classId;
			classDeclaration = firstCatchWithValue->body.classDeclaration;
			nullable = firstCatchWithValue->body.isNullable();
		} else if (allCatchesEndEarly && body.hasValue()) {
			classId = body.classId;
			classDeclaration = body.classDeclaration;
			nullable = body.isNullable();
		}
	} else {
		context.mustReturnValueNode = nullptr;
		body.optimize(in_data);
		for (auto &clause : catchClauses) {
			clause.body.optimize(in_data);
		}
		if (hasFinally) {
			finallyBody.optimize(in_data);
		}
	}
	context.mustReturnValueNode = lastMustReturnValueNode;
	return this;
}

void TryCatchNode::emitCatchClauses(in_func, std::vector<uint8_t> &bytecodes) {
	if (catchClauses.empty()) {
		return;
	}
	if (catchClauses.size() == 1 && catchClauses[0].isCatchAll) {
		auto decl = catchClauses[0].exceptionDeclaration;
		bytecodes.emplace_back(Opcode::LOAD_EXCEPTION);
		bytecodes.emplace_back(decl->isGlobal ? Opcode::STORE_GLOBAL
		                                      : Opcode::STORE_LOCAL);
		put_opcode_u32(bytecodes, decl->id);
		catchClauses[0].body.putBytecodes(in_data, bytecodes);
		return;
	}

	bytecodes.emplace_back(Opcode::LOAD_EXCEPTION);
	bytecodes.emplace_back(tempExceptionVar->isGlobal ? Opcode::STORE_GLOBAL
	                                                  : Opcode::STORE_LOCAL);
	put_opcode_u32(bytecodes, tempExceptionVar->id);

	std::vector<Offset> jumpsToEnd;
	bool hasCatchAll = false;

	for (size_t k = 0; k < catchClauses.size(); ++k) {
		auto &clause = catchClauses[k];
		if (clause.isCatchAll) {
			hasCatchAll = true;
			bytecodes.emplace_back(tempExceptionVar->isGlobal
			                           ? Opcode::LOAD_GLOBAL
			                           : Opcode::LOAD_LOCAL);
			put_opcode_u32(bytecodes, tempExceptionVar->id);
			bytecodes.emplace_back(clause.exceptionDeclaration->isGlobal
			                           ? Opcode::STORE_GLOBAL
			                           : Opcode::STORE_LOCAL);
			put_opcode_u32(bytecodes, clause.exceptionDeclaration->id);

			clause.body.putBytecodes(in_data, bytecodes);

			bytecodes.emplace_back(Opcode::JUMP);
			jumpsToEnd.push_back(bytecodes.size() - context.currentBytecodePos);
			put_opcode_u32(bytecodes, 0);
			break;
		} else {
			bytecodes.emplace_back(tempExceptionVar->isGlobal
			                           ? Opcode::LOAD_GLOBAL
			                           : Opcode::LOAD_LOCAL);
			put_opcode_u32(bytecodes, tempExceptionVar->id);

			bytecodes.emplace_back(Opcode::IS);
			put_opcode_u32(bytecodes, clause.exceptionClassId);

			bytecodes.emplace_back(Opcode::JUMP_IF_FALSE);
			Offset jumpToNext = bytecodes.size() - context.currentBytecodePos;
			put_opcode_u32(bytecodes, 0);

			bytecodes.emplace_back(tempExceptionVar->isGlobal
			                           ? Opcode::LOAD_GLOBAL
			                           : Opcode::LOAD_LOCAL);
			put_opcode_u32(bytecodes, tempExceptionVar->id);
			bytecodes.emplace_back(clause.exceptionDeclaration->isGlobal
			                           ? Opcode::STORE_GLOBAL
			                           : Opcode::STORE_LOCAL);
			put_opcode_u32(bytecodes, clause.exceptionDeclaration->id);

			clause.body.putBytecodes(in_data, bytecodes);

			bytecodes.emplace_back(Opcode::JUMP);
			jumpsToEnd.push_back(bytecodes.size() - context.currentBytecodePos);
			put_opcode_u32(bytecodes, 0);

			rewrite_opcode_u32(bytecodes.data() + context.currentBytecodePos,
			                   jumpToNext,
			                   bytecodes.size() - context.currentBytecodePos);
		}
	}

	if (!hasCatchAll) {
		bytecodes.emplace_back(tempExceptionVar->isGlobal
		                           ? Opcode::LOAD_GLOBAL
		                           : Opcode::LOAD_LOCAL);
		put_opcode_u32(bytecodes, tempExceptionVar->id);
		bytecodes.emplace_back(Opcode::THROW_EXCEPTION);
	}

	BytecodePos endCatchPos = bytecodes.size() - context.currentBytecodePos;
	for (Offset jmp : jumpsToEnd) {
		rewrite_opcode_u32(bytecodes.data() + context.currentBytecodePos,
		                   jmp, endCatchPos);
	}
}

void TryCatchNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
	auto lastMustReturnValueNode = context.mustReturnValueNode;
	if (mustReturnValue) {
		context.mustReturnValueNode = this;
	} else {
		context.mustReturnValueNode = nullptr;
	}
	if (hasFinally && !hasCatch) {
		bytecodes.emplace_back(Opcode::ADD_FINALLY_BLOCK);
		Offset jumpToFinallyPos = bytecodes.size() - context.currentBytecodePos;
		put_opcode_u32(bytecodes, 0);

		body.putBytecodes(in_data, bytecodes);

		bytecodes.emplace_back(Opcode::REMOVE_FINALLY);
		BytecodePos startFinallyPos = bytecodes.size() - context.currentBytecodePos;
		rewrite_opcode_u32(bytecodes.data() + context.currentBytecodePos,
		                   jumpToFinallyPos, startFinallyPos);
		context.mustReturnValueNode = nullptr;
		finallyBody.putBytecodes(in_data, bytecodes);
		bytecodes.emplace_back(Opcode::END_FINALLY);
	} else if (hasFinally && hasCatch) {
		bytecodes.emplace_back(Opcode::ADD_FINALLY_BLOCK);
		Offset jumpToFinallyPos = bytecodes.size() - context.currentBytecodePos;
		put_opcode_u32(bytecodes, 0);

		bytecodes.emplace_back(Opcode::ADD_TRY_BLOCK);
		Offset jumpToCatchPos = bytecodes.size() - context.currentBytecodePos;
		put_opcode_u32(bytecodes, 0);

		body.putBytecodes(in_data, bytecodes);

		bytecodes.emplace_back(Opcode::REMOVE_TRY_AND_JUMP);
		Offset jumpToAfterCatch = bytecodes.size() - context.currentBytecodePos;
		put_opcode_u32(bytecodes, 0);

		BytecodePos startCatchPos = bytecodes.size() - context.currentBytecodePos;
		emitCatchClauses(in_data, bytecodes);

		rewrite_opcode_u32(bytecodes.data() + context.currentBytecodePos,
		                   jumpToCatchPos, startCatchPos);
		rewrite_opcode_u32(bytecodes.data() + context.currentBytecodePos,
		                   jumpToAfterCatch,
		                   bytecodes.size() - context.currentBytecodePos);

		bytecodes.emplace_back(Opcode::REMOVE_FINALLY);
		BytecodePos startFinallyPos = bytecodes.size() - context.currentBytecodePos;
		rewrite_opcode_u32(bytecodes.data() + context.currentBytecodePos,
		                   jumpToFinallyPos, startFinallyPos);
		context.mustReturnValueNode = nullptr;
		finallyBody.putBytecodes(in_data, bytecodes);
		bytecodes.emplace_back(Opcode::END_FINALLY);
	} else {
		bytecodes.emplace_back(Opcode::ADD_TRY_BLOCK);
		Offset jumpToCatchPos = bytecodes.size() - context.currentBytecodePos;
		put_opcode_u32(bytecodes, 0);
		body.putBytecodes(in_data, bytecodes);
		bytecodes.emplace_back(Opcode::REMOVE_TRY_AND_JUMP);
		Offset jumpToNewCommand = bytecodes.size() - context.currentBytecodePos;
		put_opcode_u32(bytecodes, 0);
		BytecodePos startCatchPos = bytecodes.size() - context.currentBytecodePos;
		emitCatchClauses(in_data, bytecodes);
		rewrite_opcode_u32(bytecodes.data() + context.currentBytecodePos,
		                   jumpToCatchPos, startCatchPos);
		rewrite_opcode_u32(bytecodes.data() + context.currentBytecodePos,
		                   jumpToNewCommand,
		                   bytecodes.size() - context.currentBytecodePos);
	}
	context.mustReturnValueNode = lastMustReturnValueNode;
}

void TryCatchNode::rewrite(in_func, uint8_t *bytecodes) {
	body.rewrite(in_data, bytecodes);
	for (auto &clause : catchClauses) {
		clause.body.rewrite(in_data, bytecodes);
	}
	if (hasFinally) {
		finallyBody.rewrite(in_data, bytecodes);
	}
}

ExprNode *TryCatchNode::copy(in_func) {
	auto newNode = context.tryCatchPool.push(line, mustReturnValue);
	newNode->hasCatch = hasCatch;
	newNode->hasFinally = hasFinally;
	newNode->classId = classId;
	newNode->classDeclaration = classDeclaration;
	newNode->nullable = nullable;
	newNode->tempExceptionVar = tempExceptionVar;
	newNode->exceptionDeclaration = exceptionDeclaration;
	newNode->body.nodes.reserve(body.nodes.size());
	for (auto node : body.nodes) {
		newNode->body.nodes.push_back(node->copy(in_data));
	}
	newNode->catchClauses.reserve(catchClauses.size());
	for (auto &clause : catchClauses) {
		CatchClause newClause(clause.body.line);
		newClause.exceptionDeclaration = clause.exceptionDeclaration;
		newClause.classDeclaration = clause.classDeclaration;
		newClause.exceptionClassId = clause.exceptionClassId;
		newClause.isCatchAll = clause.isCatchAll;
		newClause.body.nodes.reserve(clause.body.nodes.size());
		for (auto node : clause.body.nodes) {
			newClause.body.nodes.push_back(node->copy(in_data));
		}
		newNode->catchClauses.push_back(std::move(newClause));
	}
	if (!newNode->catchClauses.empty()) {
		newNode->exceptionDeclaration = newNode->catchClauses[0].exceptionDeclaration;
		newNode->catchBody.nodes = newNode->catchClauses[0].body.nodes;
	}
	if (hasFinally) {
		newNode->finallyBody.nodes.reserve(finallyBody.nodes.size());
		for (auto node : finallyBody.nodes) {
			newNode->finallyBody.nodes.push_back(node->copy(in_data));
		}
	}
	return newNode;
}

} // namespace Autolang

#endif
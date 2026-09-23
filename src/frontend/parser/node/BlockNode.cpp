#ifndef BLOCK_NODE_CPP
#define BLOCK_NODE_CPP

#include <algorithm>
#include "Node.hpp"
#include "frontend/parser/ParserContext.hpp"

namespace Autolang {

static DeclarationNode *extractAssignmentTarget(in_func, ExprNode *node, bool &outIsNonNull) {
	if (!node || node->kind != NodeType::SET) return nullptr;
	auto setNode = static_cast<SetNode *>(node);
	if (!setNode->detach || !setNode->value) return nullptr;
	DeclarationNode *decl = nullptr;
	if (setNode->detach->kind == NodeType::VAR || setNode->detach->kind == NodeType::GET_PROP) {
		decl = static_cast<AccessNode *>(setNode->detach)->declaration;
	} else if (setNode->detach->kind == NodeType::UNKNOW) {
		auto unknow = static_cast<UnknowNode *>(setNode->detach);
		auto found = context.findDeclaration(in_data, unknow->line, unknow->nameId, true);
		if (found && (found->kind == NodeType::VAR || found->kind == NodeType::GET_PROP)) {
			decl = static_cast<AccessNode *>(found)->declaration;
		}
	}
	if (!decl) return nullptr;
	outIsNonNull = !setNode->value->isNullable();
	return decl;
}

enum class VarBranchStatus {
	UNMODIFIED,
	ASSIGNED_NON_NULL,
	ASSIGNED_NULLABLE,
	EXITS_EARLY
};

template <typename Container>
static VarBranchStatus analyzeVarInBranch(in_func, const Container &nodes, DeclarationNode *targetDecl) {
	if (nodes.empty()) return VarBranchStatus::UNMODIFIED;
	auto lastKind = nodes.back()->kind;
	if (lastKind == NodeType::RET || lastKind == NodeType::THROW || lastKind == NodeType::SKIP) {
		return VarBranchStatus::EXITS_EARLY;
	}
	for (auto it = nodes.rbegin(); it != nodes.rend(); ++it) {
		ExprNode *stmt = *it;
		if (!stmt) continue;
		bool isNonNull = false;
		DeclarationNode *decl = extractAssignmentTarget(in_data, stmt, isNonNull);
		if (decl == targetDecl) {
			return isNonNull ? VarBranchStatus::ASSIGNED_NON_NULL : VarBranchStatus::ASSIGNED_NULLABLE;
		}
	}
	return VarBranchStatus::UNMODIFIED;
}

static void checkAndApplyGuardCasts(in_func, ExprNode *node, SmallVector<SmartCastInfo, 4> &activeGuardCasts) {
	if (!node) return;

	// If statement flow analysis
	if (node->kind == NodeType::IF) {
		auto ifNode = static_cast<IfNode *>(node);
		if (!ifNode->ifFalse) {
			bool returns = ifNode->trueBranchReturns;
			if (!returns && !ifNode->ifTrue.nodes.empty()) {
				auto lastKind = ifNode->ifTrue.nodes.back()->kind;
				returns = (lastKind == NodeType::RET || lastKind == NodeType::THROW || lastKind == NodeType::SKIP);
			}
			for (auto &cast : ifNode->falseCasts) {
				if (!cast.declaration) continue;
				if (returns) {
					cast.apply();
					activeGuardCasts.push_back(cast);
				} else if (!cast.targetNullable) {
					auto status = analyzeVarInBranch(in_data, ifNode->ifTrue.nodes, cast.declaration);
					if (status == VarBranchStatus::ASSIGNED_NON_NULL || status == VarBranchStatus::EXITS_EARLY) {
						cast.apply();
						activeGuardCasts.push_back(cast);
					}
				}
			}
		} else if (ifNode->ifFalse) {
			bool returns = ifNode->falseBranchReturns;
			if (!returns && !ifNode->ifFalse->nodes.empty()) {
				auto falseLastKind = ifNode->ifFalse->nodes.back()->kind;
				returns = (falseLastKind == NodeType::RET || falseLastKind == NodeType::THROW || falseLastKind == NodeType::SKIP);
			}
			if (returns) {
				for (auto &cast : ifNode->trueCasts) {
					cast.apply();
					activeGuardCasts.push_back(cast);
				}
			}

			SmallVector<DeclarationNode *, 4> candidates;
			for (auto &c : ifNode->trueCasts) {
				if (c.declaration && std::find(candidates.begin(), candidates.end(), c.declaration) == candidates.end()) {
					candidates.push_back(c.declaration);
				}
			}
			for (auto &c : ifNode->falseCasts) {
				if (c.declaration && std::find(candidates.begin(), candidates.end(), c.declaration) == candidates.end()) {
					candidates.push_back(c.declaration);
				}
			}
			for (auto *stmt : ifNode->ifTrue.nodes) {
				bool isNonNull = false;
				auto *d = extractAssignmentTarget(in_data, stmt, isNonNull);
				if (d && std::find(candidates.begin(), candidates.end(), d) == candidates.end()) {
					candidates.push_back(d);
				}
			}

			for (auto *decl : candidates) {
				bool trueNonNull = false;
				for (auto &c : ifNode->trueCasts) {
					if (c.declaration == decl && !c.targetNullable) {
						trueNonNull = true;
						break;
					}
				}
				auto trueStatus = analyzeVarInBranch(in_data, ifNode->ifTrue.nodes, decl);
				if (trueStatus == VarBranchStatus::ASSIGNED_NON_NULL || trueStatus == VarBranchStatus::EXITS_EARLY) {
					trueNonNull = true;
				} else if (trueStatus == VarBranchStatus::ASSIGNED_NULLABLE) {
					trueNonNull = false;
				}

				bool falseNonNull = false;
				for (auto &c : ifNode->falseCasts) {
					if (c.declaration == decl && !c.targetNullable) {
						falseNonNull = true;
						break;
					}
				}
				auto falseStatus = analyzeVarInBranch(in_data, ifNode->ifFalse->nodes, decl);
				if (falseStatus == VarBranchStatus::ASSIGNED_NON_NULL || falseStatus == VarBranchStatus::EXITS_EARLY) {
					falseNonNull = true;
				} else if (falseStatus == VarBranchStatus::ASSIGNED_NULLABLE) {
					falseNonNull = false;
				}

				if (trueNonNull && falseNonNull) {
					SmartCastInfo info;
					info.declaration = decl;
					info.targetNullable = false;
					info.apply();
					activeGuardCasts.push_back(info);
				}
			}
		}
	}
}

ExprNode *BlockNode::resolve(in_func) {
	ParserContext::mode = mode;
	size_t i = 0;
	SmallVector<SmartCastInfo, 4> activeGuardCasts;
	while (i < nodes.size()) {
		if (nodes[i] == nullptr) {
			nodes.erase(nodes.begin() + i);
			continue;
		}
		nodes[i] = nodes[i]->resolve(in_data);
		if (nodes[i]->kind == NodeType::BLOCK) {
			auto subBlock = static_cast<BlockNode *>(nodes[i]);
			auto subNodes = std::move(subBlock->nodes);
			nodes.erase(nodes.begin() + i);
			nodes.insert(nodes.begin() + i, subNodes.begin(), subNodes.end());
			continue;
		}
		checkAndApplyGuardCasts(in_data, nodes[i], activeGuardCasts);
		++i;
	}
	for (auto it = activeGuardCasts.rbegin(); it != activeGuardCasts.rend(); ++it) {
		it->restore();
	}
	return this;
}

void BlockNode::rewrite(in_func, uint8_t *bytecodes) {
	for (auto *node : nodes)
		node->rewrite(in_data, bytecodes);
}

void BlockNode::refresh() {
	for (auto *node : nodes) {
		ExprNode::deleteNode(node);
	}
	nodes.clear();
}

void BlockNode::loadReturnValueClassId(in_func, uint32_t line,
                                       std::optional<ClassId> &currentClassId,
                                       ClassId newClassId) {
	if (!currentClassId) {
		// std::cerr << compile.classes[newClassId]->getName(compile) << "\n";
		currentClassId = newClassId;
		return;
	}
	if (*currentClassId == newClassId) {
		return;
	}
	if ((*currentClassId == DefaultClass::intClassId ||
	     *currentClassId == DefaultClass::floatClassId) &&
	    (newClassId == DefaultClass::intClassId ||
	     newClassId == DefaultClass::floatClassId)) {
		currentClassId = DefaultClass::floatClassId;
		autoCastToFloat = true;
		return;
	}
	if (*currentClassId < compile.classes.size() && compile.classes[*currentClassId] &&
	    compile.classes[*currentClassId]->inheritance.get(newClassId)) {
		currentClassId = newClassId;
		return;
	}
	if (newClassId < compile.classes.size() && compile.classes[newClassId] &&
	    compile.classes[newClassId]->inheritance.get(*currentClassId)) {
		return;
	}
	ClassId commonId = ExprNode::getCommonSuperType(in_data, *currentClassId, newClassId);
	currentClassId = commonId;
}

void BlockNode::loadClassNode(in_func, ExprNode *&node,
                              std::optional<ClassId> &currentClassId,
                              bool &nullable, bool &isStatic, bool &hasValue,
                              ClassDeclaration *&newClassDeclaration) {
	switch (node->kind) {
		case NodeType::CALL: {
			node = node->optimize(in_data);

			auto *n = static_cast<HasClassIdNode *>(node);
			if (n->classId == DefaultClass::voidClassId)
				break;
			if (!hasValue) {
				hasValue = true;
			}
			loadReturnValueClassId(in_data, line, currentClassId, n->classId);
			switch (n->classId) {
				case DefaultClass::intClassId: {
					if (currentClassId == DefaultClass::floatClassId) {
						node = static_cast<HasClassIdNode *>(
						    context.castPool
						        .push(n, DefaultClass::floatClassId)
						        ->resolve(in_data));
						node = node->optimize(in_data);
					}
					break;
				}
				case DefaultClass::functionClassId: {
					newClassDeclaration = n->classDeclaration;
					break;
				}
				default: {
					if (n->classDeclaration && !n->classDeclaration->inputClassId.empty()) {
						newClassDeclaration = n->classDeclaration;
					}
					break;
				}
			}
			if (!nullable) {
				nullable = n->isNullable();
			}
			if (isStatic) {
				isStatic = n->isStaticValue();
			}
			break;
		}
		case NodeType::CREATE_SET: {
			auto n = static_cast<HasClassIdNode *>(node);
			if (!hasValue) {
				hasValue = true;
			}
			if (!currentClassId) {
				node = n->optimize(in_data);
				n = static_cast<HasClassIdNode *>(node);
				currentClassId = n->classId;
				break;
			}
			if (n->classDeclaration) {
				node = n->optimize(in_data);
				n = static_cast<HasClassIdNode *>(node);
				loadReturnValueClassId(in_data, line, currentClassId,
				                       n->classId);
				break;
			}
			if (n->classId == DefaultClass::nullClassId) {
				if (compile.classes[*currentClassId]->genericBaseClassId ==
				    DefaultClass::mapClassId) {
					node = context.createMapPool.push(
					    node->line, nullptr,
					    std::vector<
					        std::pair<HasClassIdNode *, HasClassIdNode *>>{});
					n = static_cast<HasClassIdNode *>(node);
				}
				n->classId = *currentClassId;
				node = n->optimize(in_data);
				n = static_cast<HasClassIdNode *>(node);
				break;
			}
			node = n->optimize(in_data);
			n = static_cast<HasClassIdNode *>(node);
			loadReturnValueClassId(in_data, line, currentClassId, n->classId);
			break;
		}
		case NodeType::CREATE_MAP:
		case NodeType::CREATE_ARRAY: {
			auto n = static_cast<HasClassIdNode *>(node);
			if (!hasValue) {
				hasValue = true;
			}
			if (!currentClassId) {
				node = n->optimize(in_data);
				n = static_cast<HasClassIdNode *>(node);
				currentClassId = n->classId;
				break;
			}
			if (n->classDeclaration) {
				node = n->optimize(in_data);
				n = static_cast<HasClassIdNode *>(node);
				loadReturnValueClassId(in_data, line, currentClassId,
				                       n->classId);
				break;
			}
			if (n->classId == DefaultClass::nullClassId) {
				n->classId = *currentClassId;
				node = n->optimize(in_data);
				n = static_cast<HasClassIdNode *>(node);
				return;
			}
			node = n->optimize(in_data);
			n = static_cast<HasClassIdNode *>(node);
			loadReturnValueClassId(in_data, line, currentClassId, n->classId);
			break;
		}
		case NodeType::CREATE_CLOSURE: {
			auto n = static_cast<CreateClosureNode *>(node);
			if (!hasValue) {
				hasValue = true;
			}
			if (currentClassId &&
			    currentClassId != DefaultClass::functionClassId) {
				n->throwError(
				    "Cannot cast 'Function' to '" +
				    compile.classes[*currentClassId]->getName(compile) + "'" +
				    "\nHint: A closure expression cannot be assigned or returned as a non-Function type.");
			}
			if (newClassDeclaration) {
				// if (n->mustInfer) {
				n->inferFrom(in_data, newClassDeclaration);
				// }
				node = node->optimize(in_data);
				n = static_cast<CreateClosureNode *>(node);
				if (!newClassDeclaration->isSame(n->classDeclaration)) {
					n->throwError("Cannot cast '" +
					              newClassDeclaration->getName(in_data) +
					              "' to '" +
					              n->classDeclaration->getName(in_data) + "'" +
					              "\nHint: Ensure closure parameter and return signatures match the expected Function declaration.");
				}
			} else {
				node = node->optimize(in_data);
				n = static_cast<CreateClosureNode *>(node);
				newClassDeclaration = n->classDeclaration;
				currentClassId = DefaultClass::functionClassId;
			}
			if (!nullable) {
				nullable = n->isNullable();
			}
			if (isStatic) {
				isStatic = n->isStaticValue();
			}
			break;
		}
		case NodeType::PAIR:
		case NodeType::CAST:
		case NodeType::RUNTIME_CAST:
		case NodeType::NULL_COALESCING:
		case NodeType::OPTIONAL_ACCESS:
		case NodeType::UNARY:
		case NodeType::CONST_VAL:
		case NodeType::BINARY:
		case NodeType::GET_PROP:
		case NodeType::VAR: {
			if (!hasValue) {
				hasValue = true;
			}
			node = node->optimize(in_data);
			auto n = static_cast<HasClassIdNode *>(node);
			if (n->isNullable()) {
				if (!nullable) {
					nullable = true;
				}
				if (n->kind == NodeType::CONST_VAL) {
					break;
				}
			}
			loadReturnValueClassId(in_data, line, currentClassId, n->classId);
			switch (n->classId) {
				case DefaultClass::intClassId: {
					if (currentClassId == DefaultClass::floatClassId) {
						node = static_cast<HasClassIdNode *>(
						    context.castPool
						        .push(n, DefaultClass::floatClassId)
						        ->resolve(in_data));
						node = node->optimize(in_data);
					}
					break;
				}
				case DefaultClass::functionClassId: {
					newClassDeclaration = n->classDeclaration;
					break;
				}
				default: {
					if (n->classDeclaration && !n->classDeclaration->inputClassId.empty()) {
						newClassDeclaration = n->classDeclaration;
					}
					break;
				}
			}
			if (isStatic) {
				isStatic = n->isStaticValue();
			}
			break;
		}
		case NodeType::WHEN: {
			node = node->optimize(in_data);
			auto *n = static_cast<WhenNode *>(node);

			if (n->classId == DefaultClass::nullClassId) {
				break;
			}

			if (!hasValue && n->ifNode->mustReturnValue) {
				hasValue = true;
			}

			if (autoCastToFloat) {
				n->ifNode->ifTrue.autoCastToFloat = true;
				if (n->ifNode->ifFalse) {
					n->ifNode->ifFalse->autoCastToFloat = true;
				}
			}

			if (n->ifNode->mustReturnValue && n->classId != DefaultClass::voidClassId) {
				loadReturnValueClassId(in_data, line, currentClassId, n->classId);
				if (!nullable) {
					nullable = n->isNullable();
				}
				if (isStatic) {
					isStatic = n->isStaticValue();
				}
			}
			break;
		}
		case NodeType::IF: {
			node = node->optimize(in_data);
			auto *n = static_cast<IfNode *>(node);

			if (n->classId == DefaultClass::nullClassId) {
				break;
			}

			if (!hasValue && n->mustReturnValue) {
				hasValue = true;
			}

			if (autoCastToFloat) {
				n->ifTrue.autoCastToFloat = true;
				if (n->ifFalse) {
					n->ifFalse->autoCastToFloat = true;
				}
			}

			if (n->mustReturnValue && n->classId != DefaultClass::voidClassId) {
				loadReturnValueClassId(in_data, line, currentClassId, n->classId);
				if (!nullable) {
					nullable = n->isNullable();
				}
				if (isStatic) {
					isStatic = n->isStaticValue();
				}
			}
			break;
		}
		case NodeType::TRY_CATCH: {
			auto *tc = static_cast<TryCatchNode *>(node);
			node = tc->optimize(in_data);
			tc = static_cast<TryCatchNode *>(node);
			if (tc->mustReturnValue) {
				if (tc->classId != DefaultClass::voidClassId) {
					if (!hasValue) {
						hasValue = true;
					}
					loadReturnValueClassId(in_data, line, currentClassId, tc->classId);
					if (tc->classDeclaration) {
						newClassDeclaration = tc->classDeclaration;
					}
					if (!nullable) {
						nullable = tc->isNullable();
					}
					if (isStatic) {
						isStatic = tc->isStaticValue();
					}
				}
			} else {
				bool anyCatchHasValue = false;
				for (auto &clause : tc->catchClauses) {
					if (clause.body.hasValue()) {
						anyCatchHasValue = true;
						break;
					}
				}
				if (tc->body.hasValue() || anyCatchHasValue ||
				    (tc->hasFinally && tc->finallyBody.hasValue())) {
					hasValue = true;
				}
				if (context.currentClosureNode && context.currentClosureNode->funcId) {
					auto func = compile.functions[*context.currentClosureNode->funcId];
					if (func->returnId != DefaultClass::nullClassId &&
					    func->returnId != DefaultClass::voidClassId) {
						currentClassId = func->returnId;
					}
				}
			}
			break;
		}
		case NodeType::RET: {
			auto n = static_cast<ReturnNode *>(node);

			if (!context.currentClosureNode) {
				node = node->optimize(in_data);
				break;
			}

			if (context.mustReturnValueNode->kind != NodeType::CREATE_CLOSURE) {
				auto &closureBody = context.currentClosureNode->body;
				if (n->value) {
					if (closureBody.autoCastToFloat) {
						n->value = static_cast<HasClassIdNode *>(
						    context.castPool
						        .push(n->value, DefaultClass::floatClassId)
						        ->resolve(in_data));
						node = n->optimize(in_data);
						break;
					}
					auto value = static_cast<ExprNode *>(n->value);
					bool closureHasValue = closureBody.hasValue();
					loadClassNode(in_data, value,
					              *context.currentClosureCurrentClassId,
					              *context.currentClosureNullable,
					              *context.currentClosureIsStatic,
					              closureHasValue, newClassDeclaration);
					if (closureHasValue && context.currentClosureCurrentClassId->has_value()) {
						closureBody.classId = **context.currentClosureCurrentClassId;
					}
					node = n->optimize(in_data);
					break;
				}
				node = n->optimize(in_data);
				if (*context.currentClosureCurrentClassId) {
					if (*context.currentClosureCurrentClassId !=
					    DefaultClass::voidClassId) {
						throwError(
						    "Cannot cast '" +
						    compile
						        .classes[**context.currentClosureCurrentClassId]
						        ->getName(compile) +
						    "' to 'Void'" +
						    "\nHint: Function with 'Void' return type cannot return a value.");
					}
				} else {
					*context.currentClosureCurrentClassId =
					    DefaultClass::voidClassId;
				}
				break;
			}

			if (!hasValue) {
				hasValue = true;
			}

			if (!n->value) {
				node = node->optimize(in_data);

				if (currentClassId) {
					if (currentClassId != DefaultClass::voidClassId) {
						throwError(
						    "Cannot cast '" +
						    compile.classes[*currentClassId]->getName(compile) +
						    "' to 'Void'" +
						    "\nHint: Function with 'Void' return type cannot return a value.");
					}
				} else {
					currentClassId = DefaultClass::voidClassId;
				}
				break;
			}
			auto value = static_cast<ExprNode *>(n->value);
			loadClassNode(in_data, value, currentClassId, nullable, isStatic,
			              hasValue, newClassDeclaration);
			node = value;
			break;
		}
		default: {
			node = node->optimize(in_data);
			break;
		}
	}
}

void BlockNode::loadClassAndOptimize(in_func) {
	std::optional<ClassId> currentClassId;
	bool nullable = false;
	bool hasValue = false;
	bool isStatic = context.mustReturnValueNode->isStaticValue();
	ClassDeclaration *newClassDeclaration = nullptr;
	switch (context.mustReturnValueNode->kind) {
		case NodeType::CREATE_CLOSURE: {
			auto *n =
			    static_cast<CreateClosureNode *>(context.mustReturnValueNode);
			context.currentClosureCurrentClassId = &currentClassId;
			context.currentClosureNullable = &nullable;
			context.currentClosureIsStatic = &isStatic;
			auto returnClass = n->classDeclaration->inputClassId[0];
			if (returnClass) {
				currentClassId = *returnClass->classId;
				if (returnClass->classId == DefaultClass::functionClassId) {
					newClassDeclaration = returnClass;
				}
				if (returnClass->classId == DefaultClass::floatClassId) {
					autoCastToFloat = true;
				}
			}
			break;
		}
		case NodeType::IF: {
			auto *n = static_cast<IfNode *>(context.mustReturnValueNode);
			if (n->classId == DefaultClass::nullClassId) {
				break;
			}
			if (n->classDeclaration) {
				newClassDeclaration = n->classDeclaration;
			}
			currentClassId = n->classId;
			if (n->classId == DefaultClass::floatClassId) {
				autoCastToFloat = true;
			}
			break;
		}
		case NodeType::TRY_CATCH: {
			auto *n = static_cast<TryCatchNode *>(context.mustReturnValueNode);
			if (n->classId == DefaultClass::nullClassId || n->classId == DefaultClass::voidClassId) {
				break;
			}
			if (n->classDeclaration) {
				newClassDeclaration = n->classDeclaration;
			}
			currentClassId = n->classId;
			if (n->classId == DefaultClass::floatClassId) {
				autoCastToFloat = true;
			}
			break;
		}
	}
	SmallVector<SmartCastInfo, 4> activeGuardCasts;
	for (size_t i = 0; i < nodes.size(); ++i) {
		auto *&node = nodes[i];
		loadClassNode(in_data, node, currentClassId, nullable, isStatic,
		              hasValue, newClassDeclaration);
		checkAndApplyGuardCasts(in_data, node, activeGuardCasts);
	}
	for (auto it = activeGuardCasts.rbegin(); it != activeGuardCasts.rend(); ++it) {
		it->restore();
	}
	this->nullable = nullable;
	this->isStatic = isStatic;
	this->hasValueAllCases = hasValue;
	if (hasValue && currentClassId) {
		this->classId = *currentClassId;
		if (this->classId == DefaultClass::anyClassId || !newClassDeclaration) {
			this->classDeclaration = ExprNode::getOrCreateClassDeclaration(in_data, *currentClassId, line, this->nullable);
		} else {
			this->classDeclaration = newClassDeclaration;
		}
	} else {
		this->classId = DefaultClass::voidClassId;
		this->classDeclaration = newClassDeclaration;
	}

	context.mustReturnValueNode->setNullable(nullable);
	context.mustReturnValueNode->setIsStatic(isStatic);
	switch (context.mustReturnValueNode->kind) {
		case NodeType::IF: {
			auto *n = static_cast<IfNode *>(context.mustReturnValueNode);
			if (nullable) {
				n->nullable = true;
			}
			bool branchEndsEarly = !nodes.empty() && (nodes.back()->kind == NodeType::THROW || nodes.back()->kind == NodeType::RET || nodes.back()->kind == NodeType::SKIP);
			if (!currentClassId) {
				if (!n->mustReturnValue || nullable || branchEndsEarly) {
					return;
				}
				throwError("Expression branch must return a value\nHint: All branches of an 'if' expression must evaluate to a value of a compatible type.");
			}
			if (!this->hasValue() && n->mustReturnValue && !branchEndsEarly) {
				throwError("Expression branch must return a value\nHint: All branches of an 'if' expression must evaluate to a value of a compatible type.");
			}
			if (newClassDeclaration) {
				n->classDeclaration = newClassDeclaration;
			}
			if (n->classId == DefaultClass::nullClassId) {
				n->classId = *currentClassId;
				if (n->classId == DefaultClass::anyClassId || !n->classDeclaration) {
					n->classDeclaration = ExprNode::getOrCreateClassDeclaration(in_data, *currentClassId, line, n->nullable);
				}
				return;
			}
			loadReturnValueClassId(in_data, line, currentClassId, n->classId);
			n->classId = *currentClassId;
			if (n->classId == DefaultClass::anyClassId || !n->classDeclaration) {
				n->classDeclaration = ExprNode::getOrCreateClassDeclaration(in_data, *currentClassId, line, n->nullable);
			}
			return;
		}
		case NodeType::TRY_CATCH: {
			auto *n = static_cast<TryCatchNode *>(context.mustReturnValueNode);
			if (nullable) {
				n->nullable = true;
			}
			bool branchEndsEarly = !nodes.empty() && (nodes.back()->kind == NodeType::THROW || nodes.back()->kind == NodeType::RET || nodes.back()->kind == NodeType::SKIP);
			if (!currentClassId) {
				if (!n->mustReturnValue || nullable || branchEndsEarly) {
					return;
				}
				throwError("Expression branch must return a value\nHint: All branches of a 'try' expression must evaluate to a value of a compatible type.");
			}
			if (!this->hasValue() && n->mustReturnValue && !branchEndsEarly) {
				throwError("Expression branch must return a value\nHint: All branches of a 'try' expression must evaluate to a value of a compatible type.");
			}
			if (newClassDeclaration) {
				n->classDeclaration = newClassDeclaration;
			}
			if (n->classId == DefaultClass::nullClassId || n->classId == DefaultClass::voidClassId) {
				n->classId = *currentClassId;
				if (n->classId == DefaultClass::anyClassId || !n->classDeclaration) {
					n->classDeclaration = ExprNode::getOrCreateClassDeclaration(in_data, *currentClassId, line, n->nullable);
				}
				return;
			}
			loadReturnValueClassId(in_data, line, currentClassId, n->classId);
			n->classId = *currentClassId;
			if (n->classId == DefaultClass::anyClassId || !n->classDeclaration) {
				n->classDeclaration = ExprNode::getOrCreateClassDeclaration(in_data, *currentClassId, line, n->nullable);
			}
			return;
		}
		case NodeType::CREATE_CLOSURE: {
			auto *n =
			    static_cast<HasClassIdNode *>(context.mustReturnValueNode);
			if (!currentClassId ||
			    currentClassId == DefaultClass::voidClassId) {
				if (nullable) {
					throwError("Cannot infer return type for closure because "
					           "its body is a null literal\nHint: Specify an explicit return type or return a typed expression instead of raw 'null'.");
				}
				auto classDeclaration =
				    context.classDeclarationAllocator.push();
				classDeclaration->baseClassLexerStringId = lexerIdVoid;
				classDeclaration->classId = DefaultClass::voidClassId;
				classDeclaration->line = n->classDeclaration->line;
				n->classDeclaration->inputClassId[0] = classDeclaration;
				return;
			}

			if (!this->hasValue()) {
				throwError("Expression branch must return a value\nHint: Closure body must return a value matching the declared function return type.");
			}

			// Because it return function
			if (newClassDeclaration) {
				if (nullable) {
					newClassDeclaration->nullable = true;
				}
				auto returnClass = n->classDeclaration->inputClassId[0];
				if (returnClass && nullable && !returnClass->nullable) {
					throwError("Cannot cast '" +
					           newClassDeclaration->getName<true>(in_data) +
					           "' to '" + returnClass->getName<true>(in_data) +
					           "'\nHint: Closure return type must match or inherit from the target Function signature.");
				}
				n->classDeclaration->inputClassId[0] = newClassDeclaration;
				return;
			}

			auto classDeclaration = context.classDeclarationAllocator.push();
			classDeclaration->baseClassLexerStringId =
			    context.createLexerStringIfNotExists(
			        compile.classes[*currentClassId]->getName(compile));
			classDeclaration->classId = *currentClassId;
			classDeclaration->line = n->classDeclaration->line;
			if (nullable) {
				classDeclaration->nullable = true;
			}
			auto returnClass = n->classDeclaration->inputClassId[0];
			if (returnClass && nullable && !returnClass->nullable) {
				throwError("Cannot cast '" +
				           classDeclaration->getName<true>(in_data) + "' to '" +
				           returnClass->getName<true>(in_data) +
				           "'\nHint: Closure return type must match or inherit from the expected return type.");
			}
			n->classDeclaration->inputClassId[0] = classDeclaration;
			return;
		}
		default:
			break;
	}
}

ExprNode *BlockNode::optimize(in_func) {
	if (context.mustReturnValueNode) {
		loadClassAndOptimize(in_data);
		return this;
	}
	SmallVector<SmartCastInfo, 4> activeGuardCasts;
	for (size_t i = 0; i < nodes.size(); ++i) {
		nodes[i] = nodes[i]->optimize(in_data);
		checkAndApplyGuardCasts(in_data, nodes[i], activeGuardCasts);
	}
	for (auto it = activeGuardCasts.rbegin(); it != activeGuardCasts.rend(); ++it) {
		it->restore();
	}
	if (!nodes.empty()) {
		auto *lastNode = nodes.back();
		switch (lastNode->kind) {
			case NodeType::VAR:
			case NodeType::CONST_VAL:
			case NodeType::CREATE_ARRAY:
			case NodeType::CREATE_MAP:
			case NodeType::CREATE_SET:
			case NodeType::NULL_COALESCING:
			case NodeType::CAST:
			case NodeType::RUNTIME_CAST:
			case NodeType::OPTIONAL_ACCESS:
			case NodeType::UNARY:
			case NodeType::BINARY:
			case NodeType::GET_PROP:
			case NodeType::CREATE_CLOSURE:
			case NodeType::CALL: {
				auto *hasNode = static_cast<HasClassIdNode *>(lastNode);
				if (hasNode->classId != DefaultClass::voidClassId) {
					this->classId = hasNode->classId;
					this->classDeclaration = hasNode->classDeclaration;
					this->nullable = hasNode->isNullable();
					this->isStatic = hasNode->isStaticValue();
					this->hasValueAllCases = true;
				}
				break;
			}
			case NodeType::IF: {
				auto *ifNode = static_cast<IfNode *>(lastNode);
				if (ifNode->mustReturnValue && ifNode->classId != DefaultClass::voidClassId) {
					this->classId = ifNode->classId;
					this->classDeclaration = ifNode->classDeclaration;
					this->nullable = ifNode->isNullable();
					this->isStatic = ifNode->isStaticValue();
					this->hasValueAllCases = true;
				}
				break;
			}
			case NodeType::WHEN: {
				auto *whenNode = static_cast<WhenNode *>(lastNode);
				if (whenNode->ifNode && whenNode->ifNode->mustReturnValue && whenNode->classId != DefaultClass::voidClassId) {
					this->classId = whenNode->classId;
					this->classDeclaration = whenNode->classDeclaration;
					this->nullable = whenNode->isNullable();
					this->isStatic = whenNode->isStaticValue();
					this->hasValueAllCases = true;
				}
				break;
			}
			case NodeType::TRY_CATCH: {
				auto *tcNode = static_cast<TryCatchNode *>(lastNode);
				if (tcNode->mustReturnValue && tcNode->classId != DefaultClass::voidClassId) {
					this->classId = tcNode->classId;
					this->classDeclaration = tcNode->classDeclaration;
					this->nullable = tcNode->isNullable();
					this->isStatic = tcNode->isStaticValue();
					this->hasValueAllCases = true;
				}
				break;
			}
			default:
				break;
		}
	}
	return this;
}

void BlockNode::addJumpPosition(in_func, BytecodePos pos) {
	switch (context.mustReturnValueNode->kind) {
		case NodeType::IF: {
			static_cast<IfNode *>(context.mustReturnValueNode)
			    ->jumpPosition.push_back(pos);
			break;
		}
		case NodeType::CREATE_CLOSURE: {
			static_cast<FunctionAccessNode *>(context.mustReturnValueNode)
			    ->jumpPosition.push_back(pos);
			break;
		}
		default:
			throwError(
			    "Cannot add jump position to non-branching node in BlockNode\nHint: Internal compiler error - jump targets are only supported on conditional or closure nodes.");
	}
}

void BlockNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
	if (context.mustReturnValueNode) {
		for (size_t i = 0; i < nodes.size(); ++i) {
			auto *node = nodes[i];
			if (i < nodes.size() - 1 &&
			    context.mustReturnValueNode->kind != NodeType::IF &&
			    context.mustReturnValueNode->kind != NodeType::TRY_CATCH) {
				node->putBytecodesIfMustBeCalled(in_data, bytecodes);
				continue;
			}
			if (context.mustReturnValueNode->kind == NodeType::IF) {
				auto mustReturnValueNode =
				    static_cast<IfNode *>(context.mustReturnValueNode);
				switch (node->kind) {
					case NodeType::CALL: {
						node->putBytecodes(in_data, bytecodes);
						auto *n = static_cast<CallNode *>(node);
						if (n->classId == DefaultClass::voidClassId)
							break;
						if (n->nullable &&
						    mustReturnValueNode->isForceNonNull) {
							bytecodes.emplace_back(
							    Opcode::CHECK_FORCE_NON_NULL);
						}
						if (autoCastToFloat) {
							bytecodes.emplace_back(Opcode::TO_FLOAT);
						}
						if (i != nodes.size() - 1) {
							bytecodes.emplace_back(Opcode::JUMP);
							static_cast<IfNode *>(context.mustReturnValueNode)
							    ->jumpPosition.push_back(
							        bytecodes.size() -
							        context.currentBytecodePos);
							put_opcode_u32(bytecodes, 0);
						}
						break;
					}

					case NodeType::CREATE_CLOSURE:
					case NodeType::FUNCTION_ACCESS:
					case NodeType::CONST_VAL:
					case NodeType::BINARY:
					case NodeType::CREATE_ARRAY:
					case NodeType::CREATE_MAP:
					case NodeType::CREATE_SET:
					case NodeType::NULL_COALESCING:
					case NodeType::OPTIONAL_ACCESS:
					case NodeType::UNARY: {
						node->putBytecodes(in_data, bytecodes);
						if (static_cast<HasClassIdNode *>(node)->isNullable() &&
						    mustReturnValueNode->isForceNonNull) {
							bytecodes.emplace_back(
							    Opcode::CHECK_FORCE_NON_NULL);
						}
						if (autoCastToFloat) {
							bytecodes.emplace_back(Opcode::TO_FLOAT);
						}
						if (i != nodes.size() - 1) {
							bytecodes.emplace_back(Opcode::JUMP);
							static_cast<IfNode *>(context.mustReturnValueNode)
							    ->jumpPosition.push_back(
							        bytecodes.size() -
							        context.currentBytecodePos);
							put_opcode_u32(bytecodes, 0);
						}
						break;
					}
					case NodeType::TRY_CATCH: {
						auto *tc = static_cast<TryCatchNode *>(node);
						if (!tc->mustReturnValue) {
							node->putBytecodes(in_data, bytecodes);
							break;
						}
						node->putBytecodes(in_data, bytecodes);
						if (tc->isNullable() &&
						    mustReturnValueNode->isForceNonNull) {
							bytecodes.emplace_back(
							    Opcode::CHECK_FORCE_NON_NULL);
						}
						if (autoCastToFloat) {
							bytecodes.emplace_back(Opcode::TO_FLOAT);
						}
						if (i != nodes.size() - 1) {
							bytecodes.emplace_back(Opcode::JUMP);
							static_cast<IfNode *>(context.mustReturnValueNode)
							    ->jumpPosition.push_back(
							        bytecodes.size() -
							        context.currentBytecodePos);
							put_opcode_u32(bytecodes, 0);
						}
						break;
					}
					case NodeType::CAST:
					case NodeType::RUNTIME_CAST:
					case NodeType::GET_PROP:
					case NodeType::VAR: {
						node->putBytecodes(in_data, bytecodes);
						if (static_cast<HasClassIdNode *>(node)->isNullable() &&
						    mustReturnValueNode->isForceNonNull) {
							bytecodes.emplace_back(
							    Opcode::CHECK_FORCE_NON_NULL);
						}
						if (autoCastToFloat &&
						    static_cast<HasClassIdNode *>(node)->classId !=
						        DefaultClass::floatClassId) {
							bytecodes.emplace_back(Opcode::TO_FLOAT);
						}
						if (i != nodes.size() - 1) {
							bytecodes.emplace_back(Opcode::JUMP);
							static_cast<IfNode *>(context.mustReturnValueNode)
							    ->jumpPosition.push_back(
							        bytecodes.size() -
							        context.currentBytecodePos);
							put_opcode_u32(bytecodes, 0);
						}
						break;
					}
					case NodeType::WHEN: {
						auto *n = static_cast<WhenNode *>(node)->ifNode;
						if (mustReturnValueNode->isForceNonNull) {
							n->isForceNonNull = true;
						}
						if (!n->mustReturnValue) {
							if (autoCastToFloat) {
								n->ifTrue.autoCastToFloat = true;

								if (n->ifFalse) {
									n->ifFalse->autoCastToFloat = true;
								}
							}
							node->putBytecodes(in_data, bytecodes);
						} else {
							node->putBytecodes(in_data, bytecodes);
							if (n->nullable &&
							    mustReturnValueNode->isForceNonNull) {
								bytecodes.emplace_back(
								    Opcode::CHECK_FORCE_NON_NULL);
							}
							if (autoCastToFloat) {
								bytecodes.emplace_back(Opcode::TO_FLOAT);
							}
						}

						if (i != nodes.size() - 1) {
							bytecodes.emplace_back(Opcode::JUMP);
							static_cast<IfNode *>(context.mustReturnValueNode)
							    ->jumpPosition.push_back(
							        bytecodes.size() -
							        context.currentBytecodePos);
							put_opcode_u32(bytecodes, 0);
						}
						break;
					}
					case NodeType::IF: {
						auto *n = static_cast<IfNode *>(node);

						if (!n->mustReturnValue) {
							if (autoCastToFloat) {
								n->ifTrue.autoCastToFloat = true;

								if (n->ifFalse) {
									n->ifFalse->autoCastToFloat = true;
								}
							}
							node->putBytecodes(in_data, bytecodes);
						} else {
							node->putBytecodes(in_data, bytecodes);
							if (n->nullable &&
							    mustReturnValueNode->isForceNonNull) {
								bytecodes.emplace_back(
								    Opcode::CHECK_FORCE_NON_NULL);
							}
							if (autoCastToFloat) {
								bytecodes.emplace_back(Opcode::TO_FLOAT);
							}
						}

						if (i != nodes.size() - 1) {
							bytecodes.emplace_back(Opcode::JUMP);
							static_cast<IfNode *>(context.mustReturnValueNode)
							    ->jumpPosition.push_back(
							        bytecodes.size() -
							        context.currentBytecodePos);
							put_opcode_u32(bytecodes, 0);
						}
						break;
					}
					case NodeType::DECLARATION:
					case NodeType::SET:
					case NodeType::FOR:
					case NodeType::THROW:
					case NodeType::RET:
					case NodeType::SKIP: {
						node->putBytecodes(in_data, bytecodes);
						break;
					}
					default: {
						throwError("Unsupported expression node type for block "
						           "return value\nHint: Expression node inside block cannot be evaluated to a return value.");
					}
				}
			} else if (context.mustReturnValueNode->kind == NodeType::TRY_CATCH) {
				auto mustReturnValueNode =
				    static_cast<TryCatchNode *>(context.mustReturnValueNode);
				switch (node->kind) {
					case NodeType::CALL: {
						node->putBytecodes(in_data, bytecodes);
						auto *n = static_cast<CallNode *>(node);
						if (n->classId == DefaultClass::voidClassId)
							break;
						if (autoCastToFloat) {
							bytecodes.emplace_back(Opcode::TO_FLOAT);
						}
						break;
					}
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
					case NodeType::GET_PROP:
					case NodeType::VAR: {
						node->putBytecodes(in_data, bytecodes);
						if (autoCastToFloat &&
						    static_cast<HasClassIdNode *>(node)->classId !=
						        DefaultClass::floatClassId) {
							bytecodes.emplace_back(Opcode::TO_FLOAT);
						}
						break;
					}
					case NodeType::TRY_CATCH: {
						auto *tc = static_cast<TryCatchNode *>(node);
						if (!tc->mustReturnValue) {
							node->putBytecodes(in_data, bytecodes);
							break;
						}
						node->putBytecodes(in_data, bytecodes);
						if (autoCastToFloat &&
						    tc->classId != DefaultClass::floatClassId) {
							bytecodes.emplace_back(Opcode::TO_FLOAT);
						}
						break;
					}
					case NodeType::IF: {
						auto *n = static_cast<IfNode *>(node);
						if (autoCastToFloat) {
							n->ifTrue.autoCastToFloat = true;
							if (n->ifFalse) {
								n->ifFalse->autoCastToFloat = true;
							}
						}
						node->putBytecodes(in_data, bytecodes);
						break;
					}
					case NodeType::WHEN: {
						auto *n = static_cast<WhenNode *>(node)->ifNode;
						if (autoCastToFloat) {
							n->ifTrue.autoCastToFloat = true;
							if (n->ifFalse) {
								n->ifFalse->autoCastToFloat = true;
							}
						}
						node->putBytecodes(in_data, bytecodes);
						break;
					}
					case NodeType::DECLARATION:
					case NodeType::SET:
					case NodeType::FOR:
					case NodeType::THROW:
					case NodeType::RET:
					case NodeType::SKIP: {
						node->putBytecodes(in_data, bytecodes);
						break;
					}
					default: {
						throwError("Unsupported expression node type for block "
						           "return value\nHint: Expression node inside block cannot be evaluated to a return value.");
					}
				}
			} else {
				switch (node->kind) {
					case NodeType::CALL: {
						auto *n = static_cast<CallNode *>(node);
						switch (n->classId) {
							case DefaultClass::voidClassId: {
								node->putBytecodes(in_data, bytecodes);
								break;
							}
							case DefaultClass::intClassId: {
								if (autoCastToFloat) {
									ReturnNode::putOptimizedBytecodes(
									    in_data,
									    context.castPool.push(
									        n, DefaultClass::floatClassId),
									    bytecodes);
									break;
								}
							}
							default: {
								ReturnNode::putOptimizedBytecodes(in_data, n,
								                                  bytecodes);
								break;
							}
						}
						break;
					}
					case NodeType::CREATE_CLOSURE:
					case NodeType::FUNCTION_ACCESS:
					case NodeType::CONST_VAL:
					case NodeType::BINARY:
					case NodeType::GET_PROP:
					case NodeType::VAR:
					case NodeType::CREATE_ARRAY:
					case NodeType::CREATE_MAP:
					case NodeType::CREATE_SET:
					case NodeType::NULL_COALESCING:
					case NodeType::CAST:
					case NodeType::RUNTIME_CAST:
					case NodeType::OPTIONAL_ACCESS:
					case NodeType::UNARY: {
						if (autoCastToFloat) {
							ReturnNode::putOptimizedBytecodes(
							    in_data,
							    context.castPool.push(
							        static_cast<HasClassIdNode *>(node),
							        DefaultClass::floatClassId),
							    bytecodes);
							break;
						}
						ReturnNode::putOptimizedBytecodes(
						    in_data, static_cast<HasClassIdNode *>(node),
						    bytecodes);
						break;
					}
					case NodeType::WHEN: {
						auto *n = static_cast<WhenNode *>(node)->ifNode;
						if (!n->mustReturnValue) {
							if (autoCastToFloat) {
								n->ifTrue.autoCastToFloat = true;

								if (n->ifFalse) {
									n->ifFalse->autoCastToFloat = true;
								}
							}

							node->putBytecodes(in_data, bytecodes);
							break;
						}
						if (autoCastToFloat) {
							ReturnNode::putOptimizedBytecodes(
							    in_data,
							    context.castPool.push(
							        static_cast<HasClassIdNode *>(node),
							        DefaultClass::floatClassId),
							    bytecodes);
							break;
						}
						ReturnNode::putOptimizedBytecodes(
						    in_data, static_cast<HasClassIdNode *>(node),
						    bytecodes);
						break;
					}
					case NodeType::IF: {
						auto *n = static_cast<IfNode *>(node);
						if (!n->mustReturnValue) {
							if (autoCastToFloat) {
								n->ifTrue.autoCastToFloat = true;

								if (n->ifFalse) {
									n->ifFalse->autoCastToFloat = true;
								}
							}

							node->putBytecodes(in_data, bytecodes);
							break;
						}
						if (autoCastToFloat) {
							ReturnNode::putOptimizedBytecodes(
							    in_data,
							    context.castPool.push(
							        static_cast<HasClassIdNode *>(node),
							        DefaultClass::floatClassId),
							    bytecodes);
							break;
						}
						ReturnNode::putOptimizedBytecodes(
						    in_data, static_cast<HasClassIdNode *>(node),
						    bytecodes);
						break;
					}
					default: {
						node->putBytecodes(in_data, bytecodes);
						break;
					}
				}
			}
		}
		return;
	}
	for (auto *node : nodes) {
		node->putBytecodesIfMustBeCalled(in_data, bytecodes);
	}
}

ExprNode *BlockNode::copy(in_func) {
	BlockNode *newNode = context.blockNodePool.push(line);
	newNode->mode = mode;
	newNode->nodes.reserve(nodes.size());
	for (auto &node : nodes) {
		newNode->nodes.push_back(node);
	}
	newNode->hasValueAllCases = hasValueAllCases;
	newNode->autoCastToFloat = autoCastToFloat;
	newNode->classId = classId;
	newNode->classDeclaration = classDeclaration;
	newNode->nullable = nullable;
	newNode->isStatic = isStatic;
	return newNode;
}

BlockNode::~BlockNode() {
	for (auto *node : nodes) {
		deleteNode(node);
	}
}

} // namespace Autolang

#endif
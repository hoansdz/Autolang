#ifndef BLOCK_NODE_CPP
#define BLOCK_NODE_CPP

#include "Node.hpp"
#include "frontend/parser/ParserContext.hpp"

namespace Autolang {

ExprNode *BlockNode::resolve(in_func) {
	ParserContext::mode = mode;
	for (size_t i = 0; i < nodes.size(); ++i) {
		auto &node = nodes[i];
		node = node->resolve(in_data);
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
	if (compile.classes[*currentClassId]->inheritance.get(newClassId)) {
		currentClassId = newClassId;
		return;
	}
	if (compile.classes[newClassId]->inheritance.get(*currentClassId)) {
		return;
	}
	throw ParserError(
	    line,
	    "Cannot cast '" + compile.classes[*currentClassId]->getName(compile) +
	        "' to '" + compile.classes[newClassId]->getName(compile) + "'" +
	        "\nHint: Ensure returned expressions across block branches have compatible types or add an explicit type cast.");
}

void BlockNode::loadClassNode(in_func, ExprNode *&node,
                              std::optional<ClassId> &currentClassId,
                              bool &nullable, bool &isStatic, bool &hasValue,
                              ClassDeclaration *&newClassDeclaration) {
	switch (node->kind) {
		case NodeType::CALL: {
			node->optimize(in_data);

			auto *n = static_cast<CallNode *>(node);
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
						node->optimize(in_data);
					}
					break;
				}
				case DefaultClass::functionClassId: {
					newClassDeclaration = n->classDeclaration;
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
				n->optimize(in_data);
				currentClassId = n->classId;
				break;
			}
			if (n->classDeclaration) {
				n->optimize(in_data);
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
				n->optimize(in_data);
				break;
			}
			n->optimize(in_data);
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
				n->optimize(in_data);
				currentClassId = n->classId;
				break;
			}
			if (n->classDeclaration) {
				n->optimize(in_data);
				loadReturnValueClassId(in_data, line, currentClassId,
				                       n->classId);
				break;
			}
			if (n->classId == DefaultClass::nullClassId) {
				n->classId = *currentClassId;
				n->optimize(in_data);
				return;
			}
			n->optimize(in_data);
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
				node->optimize(in_data);
				if (!newClassDeclaration->isSame(n->classDeclaration)) {
					n->throwError("Cannot cast '" +
					              newClassDeclaration->getName(in_data) +
					              "' to '" +
					              n->classDeclaration->getName(in_data) + "'" +
					              "\nHint: Ensure closure parameter and return signatures match the expected Function declaration.");
				}
			} else {
				node->optimize(in_data);
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
			node->optimize(in_data);
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
						node->optimize(in_data);
					}
					break;
				}
				case DefaultClass::functionClassId: {
					newClassDeclaration = n->classDeclaration;
					break;
				}
			}
			if (isStatic) {
				isStatic = n->isStaticValue();
			}
			break;
		}
		case NodeType::WHEN: {
			auto *n = static_cast<WhenNode *>(node);
			// n->mustReturnValue = true;
			node->optimize(in_data);

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

			loadReturnValueClassId(in_data, line, currentClassId, n->classId);
			if (!nullable) {
				nullable = n->isNullable();
			}
			if (isStatic) {
				isStatic = n->isStaticValue();
			}
			break;
		}
		case NodeType::IF: {
			auto *n = static_cast<IfNode *>(node);
			// n->mustReturnValue = true;
			node->optimize(in_data);

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

			loadReturnValueClassId(in_data, line, currentClassId, n->classId);
			if (!nullable) {
				nullable = n->isNullable();
			}
			if (isStatic) {
				isStatic = n->isStaticValue();
			}
			break;
		}
		case NodeType::RET: {
			auto n = static_cast<ReturnNode *>(node);

			if (!context.currentClosureNode) {
				node->optimize(in_data);
				break;
			}

			if (context.mustReturnValueNode->kind != NodeType::CREATE_CLOSURE) {
				auto &closureBody = context.currentClosureNode->body;
				if (!closureBody.hasValue) {
					closureBody.hasValue = true;
				}
				if (n->value) {
					if (closureBody.autoCastToFloat) {
						n->value = static_cast<HasClassIdNode *>(
						    context.castPool
						        .push(n->value, DefaultClass::floatClassId)
						        ->resolve(in_data));
						n->optimize(in_data);
						break;
					}
					auto value = static_cast<ExprNode *>(n->value);
					loadClassNode(in_data, value,
					              *context.currentClosureCurrentClassId,
					              *context.currentClosureNullable,
					              *context.currentClosureIsStatic,
					              closureBody.hasValue, newClassDeclaration);
					node = value;
					break;
				}
				n->optimize(in_data);
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
				node->optimize(in_data);

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
			node->optimize(in_data);
			break;
		}
	}
}

void BlockNode::loadClassAndOptimize(in_func) {
	std::optional<ClassId> currentClassId;
	bool nullable = false;
	// bool hasValue = false;
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
	}
	for (size_t i = 0; i < nodes.size(); ++i) {
		auto *&node = nodes[i];
		loadClassNode(in_data, node, currentClassId, nullable, isStatic,
		              hasValue, newClassDeclaration);
	}
	context.mustReturnValueNode->setNullable(nullable);
	context.mustReturnValueNode->setIsStatic(isStatic);
	switch (context.mustReturnValueNode->kind) {
		case NodeType::IF: {
			auto *n = static_cast<IfNode *>(context.mustReturnValueNode);
			if (nullable) {
				n->nullable = true;
			}
			if (!currentClassId) {
				if (!n->mustReturnValue || nullable) {
					return;
				}
				throwError("Expression branch must return a value\nHint: All branches of an 'if' expression must evaluate to a value of a compatible type.");
			}
			if (!hasValue && n->mustReturnValue) {
				throwError("Expression branch must return a value\nHint: All branches of an 'if' expression must evaluate to a value of a compatible type.");
			}
			if (newClassDeclaration) {
				n->classDeclaration = newClassDeclaration;
			}
			if (n->classId == DefaultClass::nullClassId) {
				n->classId = *currentClassId;
				return;
			}
			loadReturnValueClassId(in_data, line, currentClassId, n->classId);
			n->classId = *currentClassId;
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

			if (!hasValue) {
				throwError("Expression branch must return a value\nHint: Closure body must return a value matching the declared function return type.");
			}

			// Because it return function
			if (newClassDeclaration) {
				if (nullable) {
					newClassDeclaration->nullable = true;
				}
				auto returnClass = n->classDeclaration->inputClassId[0];
				if (nullable && !returnClass->nullable) {
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
			if (nullable && !returnClass->nullable) {
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

void BlockNode::optimize(in_func) {
	if (context.mustReturnValueNode) {
		loadClassAndOptimize(in_data);
		return;
	}
	for (auto *node : nodes) {
		node->optimize(in_data);
		if (!hasValue) {
			switch (node->kind) {
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
				case NodeType::GET_PROP: {
					hasValue = true;
					break;
				}
				case NodeType::CREATE_CLOSURE: {
					if (*static_cast<CreateClosureNode *>(node)
					         ->classDeclaration->inputClassId[0]
					         ->classId != DefaultClass::voidClassId) {
						hasValue = true;
					}
					break;
				}
				case NodeType::CALL: {
					if (static_cast<CallNode *>(node)->classId !=
					    DefaultClass::voidClassId) {
						hasValue = true;
					}
					break;
				}
				case NodeType::IF: {
					if (static_cast<IfNode *>(node)->mustReturnValue) {
						hasValue = true;
					}
					break;
				}
				case NodeType::WHEN: {
					if (static_cast<WhenNode *>(node)
					        ->ifNode->mustReturnValue) {
						hasValue = true;
					}
					break;
				}
			}
		}
	}
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
			    context.mustReturnValueNode->kind != NodeType::IF) {
				switch (node->kind) {
					case NodeType::CALL: {
						auto currentNode = static_cast<CallNode *>(node);
						node->putBytecodes(in_data, bytecodes);
						if (currentNode->isSuper)
							bytecodes.emplace_back(Opcode::POP_NO_RELEASE);
						break;
					}
					default: {
						node->putBytecodes(in_data, bytecodes);
						break;
					}
				}
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
		switch (node->kind) {
			case NodeType::CALL: {
				auto currentNode = static_cast<CallNode *>(node);
				node->putBytecodes(in_data, bytecodes);
				// if (currentNode->isSuper) break;
				// if (currentNode->classId != DefaultClass::voidClassId) {
				// bytecodes.emplace_back(currentNode->isSuper
				//                            ? Opcode::POP_NO_RELEASE
				//                            : Opcode::POP);
				if (currentNode->isSuper)
					bytecodes.emplace_back(Opcode::POP_NO_RELEASE);
				// }
				break;
			}
				// 	case NodeType::OPTIONAL_ACCESS: {
				// 		auto currentNode = static_cast<OptionalAccessNode
				// *>(node); 		currentNode->returnNullIfNull = false;
				// 		node->putBytecodes(in_data, bytecodes);
				// 		if (currentNode->value->kind != NodeType::CALL ||
				// 		    currentNode->value->classId !=
				// DefaultClass::voidClassId)
				// 			bytecodes.emplace_back(Opcode::POP);
				// 		currentNode->jumpIfNullPos = bytecodes.size() -
				// context.currentBytecodePos; 		break;
				// 	}
			// case NodeType::CONST_VAL:
			// case NodeType::CREATE_CLOSURE:
			// case NodeType::FUNCTION_ACCESS:
			// case NodeType::BINARY:
			// case NodeType::GET_PROP:
			// case NodeType::VAR:
			// case NodeType::CREATE_ARRAY:
			// case NodeType::CREATE_MAP:
			// case NodeType::CREATE_SET:
			// case NodeType::NULL_COALESCING:
			// case NodeType::CAST:
			// case NodeType::RUNTIME_CAST:
			// case NodeType::OPTIONAL_ACCESS:
			// case NodeType::UNARY: {
			// 	node->putBytecodes(in_data, bytecodes);
			// 	if (autoCastToFloat) {
			// 		bytecodes.emplace_back(Opcode::TO_FLOAT);
			// 	}
			// 	break;
			// }
			// 	case NodeType::CLASS_ACCESS:
			// 	case NodeType::CONST_VAL:
			// 	case NodeType::VAR: {
			// 		break;
			// 	}
			default: {
				node->putBytecodes(in_data, bytecodes);
				break;
			}
		}
	}
}

ExprNode *BlockNode::copy(in_func) {
	BlockNode *newNode = context.blockNodePool.push(line);
	newNode->mode = mode;
	newNode->nodes.reserve(nodes.size());
	for (auto &node : nodes) {
		newNode->nodes.push_back(node);
	}
	return newNode;
}

BlockNode::~BlockNode() {
	for (auto *node : nodes) {
		deleteNode(node);
	}
}

} // namespace Autolang

#endif
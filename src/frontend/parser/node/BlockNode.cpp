#ifndef BLOCK_NODE_CPP
#define BLOCK_NODE_CPP

#include "Node.hpp"
#include "frontend/parser/ParserContext.hpp"

namespace AutoLang {

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
		// std::cerr << compile.classes[newClassId]->name << "\n";
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
		return;
	}
	if (compile.classes[*currentClassId]->inheritance.get(newClassId)) {
		currentClassId = newClassId;
		return;
	}
	if (compile.classes[newClassId]->inheritance.get(*currentClassId)) {
		return;
	}
	throw ParserError(line,
	                  "Cannot cast '" + compile.classes[*currentClassId]->name +
	                      "' to '" + compile.classes[newClassId]->name + "'");
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
				if (context.classInfo[*currentClassId]->baseClassId ==
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
				n->throwError("Cannot cast 'Function' to '" +
				              compile.classes[*currentClassId]->name + "'");
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
					              n->classDeclaration->getName(in_data) + "'");
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
		case NodeType::WHEN:
		case NodeType::IF: {
			auto *n = static_cast<NullableNode *>(node);
			// n->mustReturnValue = true;
			node->optimize(in_data);

			if (n->classId == DefaultClass::nullClassId) {
				break;
			}

			if (!hasValue && (node->kind != NodeType::IF ||
			                  static_cast<IfNode *>(node)->mustReturnValue)) {
				hasValue = true;
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

			if (!hasValue) {
				hasValue = true;
			}

			if (context.mustReturnValueNode->kind != NodeType::CREATE_CLOSURE) {
				node->optimize(in_data);
				break;
			}

			auto n = static_cast<ReturnNode *>(node);

			if (!n->value) {
				node->optimize(in_data);

				if (currentClassId) {
					if (currentClassId != DefaultClass::voidClassId) {
						throwError("Cannot cast '" +
						           compile.classes[*currentClassId]->name +
						           "' to Void'");
					}
				} else {
					currentClassId = DefaultClass::voidClassId;
				}
			} else {
				auto value = static_cast<ExprNode *>(n->value);
				loadClassNode(in_data, value, currentClassId, nullable,
				              isStatic, hasValue, newClassDeclaration);
				node = value;
			}
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
	bool hasValue = false;
	bool isStatic = context.mustReturnValueNode->isStaticValue();
	ClassDeclaration *newClassDeclaration = nullptr;
	switch (context.mustReturnValueNode->kind) {
		case NodeType::CREATE_CLOSURE: {
			auto *n =
			    static_cast<CreateClosureNode *>(context.mustReturnValueNode);
			auto returnClass = n->classDeclaration->inputClassId[0];
			if (returnClass) {
				currentClassId = *returnClass->classId;
				if (returnClass->classId == DefaultClass::functionClassId) {
					newClassDeclaration = returnClass;
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
				throwError("Expression branch must return a valueB");
			}
			if (!hasValue && n->mustReturnValue) {
				throwError("Expression branch must return a valueA");
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
					           "its body is a null literal");
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
				throwError("Expression branch must return a value");
			}

			// Because it return function
			if (newClassDeclaration) {
				if (nullable) {
					newClassDeclaration->nullable = true;
				}
				auto returnClass = n->classDeclaration->inputClassId[0];
				if (nullable && !returnClass->nullable) {
					throwError("Cannot cast " +
					           newClassDeclaration->getName<true>(in_data) +
					           " to " + returnClass->getName(in_data));
				}
				n->classDeclaration->inputClassId[0] = newClassDeclaration;
				return;
			}

			auto classDeclaration = context.classDeclarationAllocator.push();
			classDeclaration->baseClassLexerStringId =
			    context.createLexerStringIfNotExists(
			        compile.classes[*currentClassId]->name);
			classDeclaration->classId = *currentClassId;
			classDeclaration->line = n->classDeclaration->line;
			if (nullable) {
				classDeclaration->nullable = true;
			}
			auto returnClass = n->classDeclaration->inputClassId[0];
			if (nullable && !returnClass->nullable) {
				throwError("Cannot cast " +
				           classDeclaration->getName<true>(in_data) + " to " +
				           returnClass->getName(in_data));
			}
			n->classDeclaration->inputClassId[0] = classDeclaration;
			return;
		}
	}
}

void BlockNode::optimize(in_func) {
	if (context.mustReturnValueNode) {
		loadClassAndOptimize(in_data);
		return;
	}
	for (auto *node : nodes) {
		node->optimize(in_data);
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
	}
}

void BlockNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
	if (context.mustReturnValueNode) {
		for (size_t i = 0; i < nodes.size(); ++i) {
			auto *node = nodes[i];
			if (context.mustReturnValueNode->kind == NodeType::IF) {
				node->putBytecodes(in_data, bytecodes);
				switch (node->kind) {
					case NodeType::CALL: {
						auto *n = static_cast<CallNode *>(node);
						if (n->classId == DefaultClass::voidClassId)
							break;
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
					case NodeType::GET_PROP:
					case NodeType::VAR: {
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
						if (!n->mustReturnValue)
							break;
						if (i != nodes.size() - 1) {
							bytecodes.emplace_back(Opcode::JUMP);
							static_cast<IfNode *>(context.mustReturnValueNode)
							    ->jumpPosition.push_back(
							        bytecodes.size() -
							        context.currentBytecodePos);
							put_opcode_u32(bytecodes, 0);
						}
						break;

						break;
					}
					default:
						break;
				}
			} else {
				switch (node->kind) {
					case NodeType::CALL: {
						auto *n = static_cast<CallNode *>(node);
						if (n->classId == DefaultClass::voidClassId) {
							node->putBytecodes(in_data, bytecodes);
							break;
						}
						ReturnNode::putOptimizedBytecodes(in_data, n,
						                                  bytecodes);
						break;
					}
					case NodeType::CREATE_CLOSURE:
					case NodeType::FUNCTION_ACCESS:
					case NodeType::CONST_VAL:
					case NodeType::BINARY:
					case NodeType::GET_PROP:
					case NodeType::VAR: {
						ReturnNode::putOptimizedBytecodes(
						    in_data, static_cast<HasClassIdNode *>(node),
						    bytecodes);
						break;
					}
					case NodeType::IF: {
						auto *n = static_cast<IfNode *>(node);
						if (!n->mustReturnValue) {
							node->putBytecodes(in_data, bytecodes);
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
				// 	case NodeType::UNARY:
				// 	case NodeType::NULL_COALESCING:
				// 	case NodeType::BINARY:
				// 	case NodeType::CAST:
				// 	case NodeType::GET_PROP: {
				// 		node->putBytecodes(in_data, bytecodes);
				// 		bytecodes.emplace_back(Opcode::POP);
				// 		break;
				// 	}
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

} // namespace AutoLang

#endif
#ifndef NODE_OPTIMIZE_CPP
#define NODE_OPTIMIZE_CPP

#include "frontend/parser/node/NodeOptimize.hpp"
#include "frontend/parser/Debugger.hpp"
#include "frontend/parser/node/CreateNode.hpp"

namespace AutoLang {

ExprNode *UnaryNode::resolve(in_func) {
	// if (value->kind == NodeType::UNARY)
	// {
	// 	auto node = static_cast<UnaryNode *>(value);
	// 	if (node->op != op) {
	// 		return nullptr;
	// 	}
	// 	value = node->value;
	// 	node->value = nullptr;
	// 	ExprNode::deleteNode(node);
	// }
	value = static_cast<HasClassIdNode *>(value->resolve(in_data));
	switch (value->kind) {
		case NodeType::CONST: {
			auto value = static_cast<ConstValueNode *>(this->value);
			switch (op) {
				using namespace AutoLang;
				case Lexer::TokenType::PLUS: {
					switch (value->classId) {
						case AutoLang::DefaultClass::intClassId:
						case AutoLang::DefaultClass::floatClassId: {
							auto result = value;
							value = nullptr;
							ExprNode::deleteNode(this);
							return result;
						}
						case AutoLang::DefaultClass::boolClassId: {
							value->classId = AutoLang::DefaultClass::intClassId;
							value->i = static_cast<int64_t>(value->obj->b);
							auto result = value;
							value = nullptr;
							ExprNode::deleteNode(this);
							return result;
						}
						default:
							break;
					}
					break;
				}
				case Lexer::TokenType::MINUS: {
					switch (value->classId) {
						case AutoLang::DefaultClass::intClassId: {
							value->i = -value->i;
							auto result = value;
							value = nullptr;
							ExprNode::deleteNode(this);
							return result;
						}
						case AutoLang::DefaultClass::floatClassId: {
							value->f = -value->f;
							auto result = value;
							value = nullptr;
							ExprNode::deleteNode(this);
							return result;
						}
						case AutoLang::DefaultClass::boolClassId: {
							value->classId = AutoLang::DefaultClass::intClassId;
							value->i = static_cast<int64_t>(-value->obj->b);
							auto result = value;
							value = nullptr;
							ExprNode::deleteNode(this);
							return result;
						}
						default:
							break;
					}
					break;
				}
				case Lexer::TokenType::NOT: {
					if (value->classId == AutoLang::DefaultClass::boolClassId) {
						value->obj = ObjectManager::create(!value->obj->b);
						value->id =
						    context.getBoolConstValuePosition(value->obj->b);
						auto result = value;
						value = nullptr;
						ExprNode::deleteNode(this);
						return result;
					}
					// if (value->classId ==
					// AutoLang::DefaultClass::nullClassId)
					// {
					// 	value->classId = AutoLang::DefaultClass::boolClassId;
					// 	value->obj = ObjectManager::create(true);
					// 	value->id = context.getBoolConstValuePosition(true);
					// 	return value;
					// }
					break;
				}
				default:
					break;
			}
			throwError("Cannot find operator '" +
			           Lexer::Token(0, op).toString(context) + "' with class " +
			           compile.classes[value->classId]->name);
		}
		case NodeType::CAST: {
			switch (op) {
				using namespace AutoLang;
				case Lexer::TokenType::PLUS: {
					switch (value->classId) {
						case AutoLang::DefaultClass::intClassId:
						case AutoLang::DefaultClass::floatClassId: {
							auto result = value;
							value = nullptr;
							ExprNode::deleteNode(this);
							return result;
						}
						case AutoLang::DefaultClass::boolClassId: {
							value->classId = AutoLang::DefaultClass::intClassId;
							auto result = value;
							value = nullptr;
							ExprNode::deleteNode(this);
							return result;
						}
						default:
							break;
					}
					break;
				}
				case Lexer::TokenType::MINUS: {
					switch (value->classId) {
						case AutoLang::DefaultClass::intClassId:
						case AutoLang::DefaultClass::floatClassId:
						case AutoLang::DefaultClass::boolClassId: {
							return this;
						}
						default:
							break;
					}
					break;
				}
				case Lexer::TokenType::NOT: {
					if (value->classId == AutoLang::DefaultClass::boolClassId) {
						return this;
					}
					break;
				}
				default:
					break;
			}
			throwError("Cannot find operator '" +
			           Lexer::Token(0, op).toString(context) + "' with class " +
			           compile.classes[value->classId]->name);
		}
		default: {
		}
	}
	return this;
}

void UnaryNode::optimize(in_func) {
	if (value->kind == NodeType::CONST)
		static_cast<ConstValueNode *>(value)->isLoadPrimary = true;
	if (value->kind == NodeType::CLASS_ACCESS) {
		throwError("Expected value if use operator '" +
		           Lexer::Token(0, op).toString(context) + "'");
	}
	value->optimize(in_data);
	switch (op) {
		case Lexer::TokenType::PLUS:
		case Lexer::TokenType::MINUS: {
			switch (value->classId) {
				case DefaultClass::intClassId:
				case DefaultClass::floatClassId:
				case DefaultClass::boolClassId: {
					break;
				}
				default:
					throwError("Cannot cast class " +
					           compile.classes[value->classId]->name +
					           " to number");
			}
		}
		case Lexer::TokenType::NOT: {
			if (value->classId == DefaultClass::boolClassId)
				break;
			throwError("Cannot cast class " +
			           compile.classes[value->classId]->name + " to Bool");
		}
		default:
			break;
	}
	classId = value->classId;
}

ExprNode *UnaryNode::copy(in_func) {
	return context.unaryNodePool.push(
	    line, op, static_cast<HasClassIdNode *>(value->copy(in_data)));
}

ExprNode *UnknowNode::resolve(in_func) {
	{
		auto it = context.defaultClassMap.find(nameId);
		if (it != context.defaultClassMap.end()) {
			auto result = context.classAccessPool.push(line, it->second);
			return result;
		}
	}

	if (contextCallClassId) {
		auto *clazz = compile.classes[*contextCallClassId];
		auto *classInfo = context.classInfo[*contextCallClassId];

		{
			auto correctNode =
			    classInfo->findDeclaration(in_data, line, nameId);
			if (correctNode) {
				static_cast<AccessNode *>(correctNode)->nullable = nullable;
				ExprNode::deleteNode(this);
				return correctNode;
			}
		}

		{
			auto it = classInfo->constValue.find(nameId);
			if (it != classInfo->constValue.end()) {
				return it->second;
			}
		}

		{
			uint32_t count = 0;
			std::vector<FunctionId> *funcs[2];

			HasClassIdNode *caller = nullptr;

			{
				auto it = classInfo->allFunction.find(nameId);
				if (it != classInfo->allFunction.end()) {
					funcs[count++] = &it->second;
					caller = context.varPool.push(
					    line, classInfo->declarationThis, false, false);
				}
			}

			{
				auto it = context.globalFunction.find(nameId);
				if (it != context.globalFunction.end()) {
					funcs[count++] = &it->second;
				}
			}

			if (count) {
				return context.functionAccessPool.push(
				    line, caller, nameId, count,
				    std::vector<HasClassIdNode *>{}, funcs);
			}
		}
	} else {
		{
			std::vector<FunctionId> *funcs[1];
			auto it = context.globalFunction.find(nameId);
			if (it != context.globalFunction.end()) {
				funcs[0] = &it->second;
				return context.functionAccessPool.push(
				    line, nullptr, nameId, 1, std::vector<HasClassIdNode *>{},
				    funcs);
			}
		}
	}
	throwError("UnknowNode: Variable name: " + context.lexerString[nameId] +
	           " is not be declarated");
}

ExprNode *UnknowNode::copy(in_func) {
	{
		auto funcInfo = context.functionInfo[contextCallFuncId];
		auto genericDeclaration = funcInfo->findGenericDeclaration(nameId);
		if (genericDeclaration) {
			return context.classAccessPool.push(line,
			                                    genericDeclaration->classId);
		}
	}
	if (contextCallClassId) {
		switch (nameId) {
			case lexerId__CLASS__: {
				return context.constValuePool.push(
				    0, context.getCurrentClass(in_data)->name);
			}
		}
		auto classInfo = context.classInfo[*contextCallClassId];
		auto genericDeclaration = classInfo->findGenericDeclaration(nameId);
		if (genericDeclaration) {
			return context.classAccessPool.push(line,
			                                    genericDeclaration->classId);
		}
	}
	return context.unknowNodePool.push(line, context.currentClassId,
	                                   contextCallFuncId, nameId, nullable);
}

void UnknowNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
	throwError("Unknow node can't putbytecodes");
}

ExprNode *WhileNode::resolve(in_func) {
	condition = static_cast<HasClassIdNode *>(condition->resolve(in_data));
	body.resolve(in_data);
	return this;
}

void WhileNode::optimize(in_func) {
	condition->optimize(in_data);
	if (condition->classId != AutoLang::DefaultClass::boolClassId)
		throwError("Cannot use expression of type '" +
		           condition->getClassName(in_data) +
		           "' as a condition, expected 'Bool'");
	body.optimize(in_data);
}

ExprNode *WhileNode::copy(in_func) {
	auto newNode = context.whilePool.push(line);
	newNode->body.nodes.reserve(body.nodes.size());
	for (auto node : body.nodes) {
		newNode->body.nodes.push_back(node->copy(in_data));
	}
	return newNode;
}

ExprNode *ReturnNode::resolve(in_func) {
	if (loaded) {
		return this;
	}
	if (value) {
		auto func = compile.functions[funcId];
		if (func->returnId == DefaultClass::voidClassId) {
			throwError("Cannot return value, function return Void");
		}
		value = static_cast<HasClassIdNode *>(value->resolve(in_data));
		if (func->returnId == DefaultClass::nullClassId) {
			return this;
		}
		if (value->classId == func->returnId) {
			return this;
		}
		if (value->classId == DefaultClass::nullClassId) {
			return this;
		}
		value = context.castPool.push(value, func->returnId);
		// auto castNode = context.castPool.push(value, func->returnId);
		// value = static_cast<HasClassIdNode *>(castNode->resolve(in_data));
	}
	return this;
}

void ReturnNode::optimize(in_func) {
	if (loaded) {
		return;
	}
	auto func = compile.functions[funcId];
	// std::cerr<<"Loading "<<func->name<<"\n";
	if (value) {
		switch (value->kind) {
			case NodeType::CREATE_ARRAY: {
				auto createArrayNode = static_cast<CreateArrayNode *>(value);
				if (func->returnId == DefaultClass::nullClassId) {
					value->optimize(in_data);
					func->returnId = value->classId;
					return;
				}
				value->classId = func->returnId;
				value->optimize(in_data);
				break;
			}
			case NodeType::CREATE_SET: {
				auto createSetNode = static_cast<CreateSetNode *>(value);
				if (func->returnId == DefaultClass::nullClassId) {
					value->optimize(in_data);
					func->returnId = value->classId;
					return;
				}
				value->classId = func->returnId;
				value->optimize(in_data);
				break;
			}
			default: {
				value->optimize(in_data);
				break;
			}
		}
		// Marks auto
		switch (func->returnId) {
			case DefaultClass::anyClassId: {
				return;
			}
			case DefaultClass::nullClassId: {
				// std::cerr << "Loaded " << func->name << "\n";
				func->returnId = value->classId;
				if (value->isNullable()) {
					func->functionFlags |= FunctionFlags::FUNC_RETURN_NULLABLE;
				}
				break;
			}
		}
		if (!(func->functionFlags & FunctionFlags::FUNC_RETURN_NULLABLE)) {
			if (value->classId == AutoLang::DefaultClass::nullClassId) {
				throwError("Cannot return null because functions returns "
				           "nonnull value");
			}
			if (value->isNullable()) {
				std::cerr << value->getNodeType() << "\n";
				throwError("Cannot return nullable variable because functions "
				           "returns nonnull value");
			}
			return;
		}
		return;
	}
	if (func->returnId != AutoLang::DefaultClass::voidClassId) {
		throwError("Must return value");
	}
}

ExprNode *ReturnNode::copy(in_func) {
	return context.returnPool.push(
	    line, context.currentFunctionId,
	    value ? static_cast<HasClassIdNode *>(value->copy(in_data)) : nullptr);
}

void VarNode::optimize(in_func) {
	// std::cerr << "loaded " << declaration->name << " "
	//           << compile.classes[declaration->classId]->name << "\n";
	classId = declaration->classId;
	isVal = declaration->isVal;
	classDeclaration = declaration->classDeclaration;
	if (nullable) {
		nullable = declaration->nullable; // #
	}
}

ExprNode *VarNode::copy(in_func) {
	DeclarationNode *newDeclaration;
	auto funcInfo = context.getCurrentFunctionInfo(in_data);
	auto it = funcInfo->reflectDeclarationMap.find(declaration);
	if (it != funcInfo->reflectDeclarationMap.end()) {
		newDeclaration = it->second;
	} else {
		newDeclaration =
		    static_cast<DeclarationNode *>(declaration->copy(in_data));
	}
	auto newNode =
	    context.varPool.push(line, newDeclaration, isStore, nullable);
	newNode->classId = classId;
	return newNode;
}

bool VarNode::isStaticValue() { return declaration && declaration->isGlobal; }

} // namespace AutoLang

#endif
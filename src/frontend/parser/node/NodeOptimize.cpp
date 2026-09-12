#ifndef NODE_OPTIMIZE_CPP
#define NODE_OPTIMIZE_CPP

#include "frontend/parser/node/NodeOptimize.hpp"
#include "frontend/parser/Debugger.hpp"
#include "frontend/parser/ImplicitConversion.hpp"
#include "frontend/parser/ParserContext.hpp"
#include "frontend/parser/node/CreateNode.hpp"

namespace Autolang {

ExprNode *UnknowNode::resolve(in_func) {
	if (forceGlobal) {
		{
			auto globalNode = context.getMainFunctionInfo(in_data)->findDeclaration(
			    in_data, line, nameId);
			if (globalNode) {
				if (contextCallFuncId != context.mainFunctionId ||
				    !globalNode->declaration ||
				    globalNode->declaration->tokenIndex <= tokenIndex) {
					if (static_cast<AccessNode *>(globalNode)->nullable)
						static_cast<AccessNode *>(globalNode)->nullable = nullable;
					ExprNode::deleteNode(this);
					return globalNode;
				}
			}
		}

		{
			std::vector<FunctionId> *funcs[1];
			auto it = context.globalFunction.find(nameId);
			if (it != context.globalFunction.end()) {
				funcs[0] = &it->second;
				return context.functionAccessPool.push(line, nullptr, nameId, 1,
				                                       nullptr, funcs);
			}
		}

		{
			auto it = context.constValue.find(nameId);
			if (it != context.constValue.end()) {
				return it->second;
			}
		}

		throwError("Cannot find global variable or function '" +
		           context.lexerString[nameId] +
		           "'\nHint: Check symbol spelling or verify whether it is declared in global scope.");
	}

	{
		auto it = context.defaultClassMap.find(nameId);
		if (it != context.defaultClassMap.end()) {
			auto result = context.classAccessPool.push(line, it->second);
			return result;
		}
	}

	{
		auto it = context.typealiasMap.find(nameId);
		if (it != context.typealiasMap.end()) {
			auto result = context.classAccessPool.push(line, *(it->second->classDeclaration->classId));
			return result;
		}
	}

	if (contextCallClassId) {
		auto *clazz = compile.classes[*contextCallClassId];
		auto *classInfo = context.classInfo[*contextCallClassId];

		{
			auto correctNode = classInfo->findDeclaration(in_data, line, nameId,
			                                              justFindStaticMember);
			if (correctNode) {
				// if (justFindStatic &&
				//     contextCallFuncId != context.mainFunctionId) {
				// 	if (correctNode->declaration->accessModifier ==)
				// }
				if (static_cast<AccessNode *>(correctNode)->nullable)
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
				return context.functionAccessPool.push(line, caller, nameId,
				                                       count, nullptr, funcs);
			}
		}

		{
			auto globalNode = context.getMainFunctionInfo(in_data)->findDeclaration(
			    in_data, line, nameId);
			if (globalNode) {
				if (contextCallFuncId != context.mainFunctionId ||
				    !globalNode->declaration ||
				    globalNode->declaration->tokenIndex <= tokenIndex) {
					if (static_cast<AccessNode *>(globalNode)->nullable)
						static_cast<AccessNode *>(globalNode)->nullable = nullable;
					ExprNode::deleteNode(this);
					return globalNode;
				}
			}
		}
	} else {
		{
			std::vector<FunctionId> *funcs[1];
			auto it = context.globalFunction.find(nameId);
			if (it != context.globalFunction.end()) {
				funcs[0] = &it->second;
				return context.functionAccessPool.push(line, nullptr, nameId, 1,
				                                       nullptr, funcs);
			}
		}

		{
			auto it = context.constValue.find(nameId);
			if (it != context.constValue.end()) {
				return it->second;
			}
		}

		{
			auto globalNode = context.getMainFunctionInfo(in_data)->findDeclaration(
			    in_data, line, nameId);
			if (globalNode) {
				if (contextCallFuncId != context.mainFunctionId ||
				    !globalNode->declaration ||
				    globalNode->declaration->tokenIndex <= tokenIndex) {
					if (static_cast<AccessNode *>(globalNode)->nullable)
						static_cast<AccessNode *>(globalNode)->nullable = nullable;
					ExprNode::deleteNode(this);
					return globalNode;
				}
			}
		}
	}
	throwError("Cannot find variable or symbol '" + context.lexerString[nameId] +
	           "'\nHint: Check symbol spelling, ensure it is in scope or declared before usage.");
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
				    0, context.getCurrentClass(in_data)->getName(compile));
			}
		}
		auto classInfo = context.classInfo[*contextCallClassId];
		auto genericDeclaration = classInfo->findGenericDeclaration(nameId);
		if (genericDeclaration) {
			return context.classAccessPool.push(line,
			                                    genericDeclaration->classId);
		}
	}
	return context.unknowNodePool.push(line, tokenIndex, context.currentClassId,
	                                   contextCallFuncId, nameId, nullable,
	                                   justFindStaticMember, forceGlobal);
}

void UnknowNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
	throwError("Cannot generate bytecode for unresolved 'UnknownNode'\nHint: Ensure all symbols and variables are properly declared and resolved during static analysis before bytecode compilation.");
}

ExprNode *WhileNode::resolve(in_func) {
	condition = static_cast<HasClassIdNode *>(condition->resolve(in_data));
	body.resolve(in_data);
	return this;
}

ExprNode *WhileNode::optimize(in_func) {
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
		           "' as a condition, expected 'Bool'\nHint: Ensure the loop condition evaluates to a 'Bool' value.");
	body.optimize(in_data);
	return this;
}

ExprNode *WhileNode::copy(in_func) {
	auto newNode = context.whilePool.push(line);
	newNode->condition =
	    static_cast<HasClassIdNode *>(condition->copy(in_data));
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
			return this;
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
		// value = context.castPool.push(value, func->returnId);
		// auto castNode = context.castPool.push(value, func->returnId);
		// value = static_cast<HasClassIdNode *>(castNode->resolve(in_data));
	}
	return this;
}

ExprNode *ReturnNode::optimize(in_func) {
	if (loaded) {
		return this;
	}
	auto func = compile.functions[funcId];
	auto funcInfo = context.functionInfo[funcId];
	// std::cerr<<"Loading "<<func->getName(compile)<<"\n";
	if (value) {
		if (func->returnId == DefaultClass::voidClassId && throwErrIfVoid) {
			throwError("Cannot return a value from function returning Void\nHint: Remove the return value or change the function return type from Void to match the returned expression.");
		}
		switch (value->kind) {
			case NodeType::CREATE_SET: {
				if (func->returnId == DefaultClass::nullClassId) {
					value = static_cast<HasClassIdNode *>(value->optimize(in_data));
					func->returnId = value->classId;
					return this;
				}
				if (value->classDeclaration) {
					value = static_cast<HasClassIdNode *>(value->optimize(in_data));
					break;
				}
				if (value->classId == DefaultClass::nullClassId) {
					if (compile.classes[func->returnId]->genericBaseClassId ==
					    DefaultClass::mapClassId) {
						value = context.createMapPool.push(
						    value->line, nullptr,
						    std::vector<std::pair<HasClassIdNode *,
						                          HasClassIdNode *>>{});
					} else if (canImplicitConvert(in_data, func->returnId, value)) {
						auto converted = tryImplicitConversion(in_data, func->returnId, value, line);
						if (converted) {
							value = converted;
							return this;
						}
					}
					value->classId = func->returnId;
					value = static_cast<HasClassIdNode *>(value->optimize(in_data));
					return this;
				}
				value = static_cast<HasClassIdNode *>(value->optimize(in_data));
				break;
			}
			case NodeType::CREATE_MAP:
			case NodeType::CREATE_ARRAY: {
				if (func->returnId == DefaultClass::nullClassId) {
					value = static_cast<HasClassIdNode *>(value->optimize(in_data));
					func->returnId = value->classId;
					return this;
				}
				if (value->classDeclaration) {
					value = static_cast<HasClassIdNode *>(value->optimize(in_data));
					break;
				}
				if (value->classId == DefaultClass::nullClassId) {
					if (canImplicitConvert(in_data, func->returnId, value)) {
						auto converted = tryImplicitConversion(in_data, func->returnId, value, line);
						if (converted) {
							value = converted;
							return this;
						}
					}
					value->classId = func->returnId;
					value = static_cast<HasClassIdNode *>(value->optimize(in_data));
					return this;
				}
				value = static_cast<HasClassIdNode *>(value->optimize(in_data));
				break;
			}
			case NodeType::FUNCTION_ACCESS: {
				if (func->returnId != DefaultClass::nullClassId) {
					value->classDeclaration = funcInfo->returnClass;
					value = static_cast<HasClassIdNode *>(value->optimize(in_data));
					return this;
				}
				value = static_cast<HasClassIdNode *>(value->optimize(in_data));
				break;
			}
			case NodeType::CREATE_CLOSURE: {
				auto n = static_cast<CreateClosureNode *>(value);
				if (n->mustInfer &&
				    func->returnId != DefaultClass::nullClassId) {
					n->inferFrom(in_data, funcInfo->returnClass);
					value = static_cast<HasClassIdNode *>(value->optimize(in_data));
					return this;
				}
				value = static_cast<HasClassIdNode *>(value->optimize(in_data));
				break;
			}
			case NodeType::IF: {
				auto n = static_cast<IfNode *>(value);
				if (func->returnId == DefaultClass::nullClassId) {
					value = static_cast<HasClassIdNode *>(value->optimize(in_data));
					func->returnId = value->classId;
					if (n->nullable) {
						func->functionFlags |=
						    FunctionFlags::FUNC_RETURN_NULLABLE;
					}
					return this;
				}
				value->classId = func->returnId;
				n->nullable =
				    func->functionFlags & FunctionFlags::FUNC_RETURN_NULLABLE;

				if (func->returnId == DefaultClass::functionClassId) {
					n->classDeclaration = funcInfo->returnClass;
				}
				value = static_cast<HasClassIdNode *>(value->optimize(in_data));
				break;
			}
			case NodeType::WHEN: {
				auto n = static_cast<WhenNode *>(value);
				if (func->returnId == DefaultClass::nullClassId) {
					value = static_cast<HasClassIdNode *>(value->optimize(in_data));
					func->returnId = value->classId;
					if (n->nullable) {
						func->functionFlags |=
						    FunctionFlags::FUNC_RETURN_NULLABLE;
					}
					return this;
				}
				value->classId = func->returnId;
				n->nullable =
				    func->functionFlags & FunctionFlags::FUNC_RETURN_NULLABLE;
				if (func->returnId == DefaultClass::functionClassId) {
					n->classDeclaration = funcInfo->returnClass;
				}
				value = static_cast<HasClassIdNode *>(value->optimize(in_data));
				break;
			}
			// case NodeType::VAR:
			// case NodeType::GET_PROP: {
			// 	auto node = static_cast<AccessNode *>(value);
			// 	node->cloneable = true;
			// }
			default: {
				value = static_cast<HasClassIdNode *>(value->optimize(in_data));
				break;
			}
		}
		// Marks auto
		switch (func->returnId) {
			case DefaultClass::anyClassId: {
				return this;
			}
			case DefaultClass::nullClassId: {
				// std::cerr << "Loaded " << func->getName(compile) << "\n";
				switch (value->classId) {
					case DefaultClass::nullClassId: {
						throwError("Cannot infer return type for function '" +
						           func->getName(compile) +
						           "' "
						           "because its body is a null literal.\nHint: Explicitly specify the return type for function or return a concrete non-null expression.");
					}
					case DefaultClass::functionClassId: {
						funcInfo->returnClass = value->classDeclaration;
						break;
					}
				}
				func->returnId = value->classId;
				if (value->isNullable()) {
					func->functionFlags |= FunctionFlags::FUNC_RETURN_NULLABLE;
				}
				break;
			}
		}
		if (value->classId == Autolang::DefaultClass::nullClassId) {
		}
		if (!(func->functionFlags & FunctionFlags::FUNC_RETURN_NULLABLE)) {
			if (value->classId == Autolang::DefaultClass::nullClassId) {
				throwError("Cannot return null because function return type is non-nullable\nHint: Mark the function return type as nullable (e.g. Type?) or return a valid non-null object.");
			}
			if (value->isNullable()) {
				throwError("Cannot return nullable variable because function return type is non-nullable\nHint: Mark the function return type as nullable (e.g. Type?) or handle/unwrap the nullable value before returning.");
			}
		} else if (value->classId == Autolang::DefaultClass::nullClassId) {
			return this;
		}
		if (value->classId == func->returnId) {
			return this;
		}
		if (compile.classes[value->classId]->inheritance.get(func->returnId)) {
			return this;
		}
		if (value->classId == DefaultClass::intClassId &&
		    func->returnId == DefaultClass::floatClassId) {
			auto castNode = context.castPool.push(value, func->returnId);
			value = static_cast<HasClassIdNode *>(castNode->resolve(in_data));
			if (value != castNode) {
				value = static_cast<HasClassIdNode *>(value->optimize(in_data));
			}
			return this;
		}
		auto converted = tryImplicitConversion(in_data, func->returnId, value, line);
		if (converted) {
			value = converted;
			return this;
		}
		throwError("Cannot cast " +
		           compile.classes[value->classId]->getName(compile) + " to " +
		           compile.classes[func->returnId]->getName(compile) +
		           "\nHint: Ensure returned expression type matches or can be cast to the declared function return type.");
	}
	if (func->returnId != Autolang::DefaultClass::voidClassId) {
		throwError("Function with non-Void return type must return a value\nHint: Ensure all execution paths return a value matching the function return type, or change return type to Void.");
	}
	return this;
}

ExprNode *ReturnNode::copy(in_func) {
	return context.returnPool.push(
	    line, context.currentFunctionId,
	    value ? static_cast<HasClassIdNode *>(value->copy(in_data)) : nullptr);
}

} // namespace Autolang

#endif
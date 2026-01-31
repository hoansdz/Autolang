/*

@brief Parse and load variable in function and class
			 Build AST Node
@param in_func -> input (context, compile), in_data -> (context, compile)
@param i -> Current position
@details Handle keywords 'val', 'var', 'static', 'public', 'private', 'protected'
@note Use std::unique_ptr to make it safer
			 Use raw pointer release it because it will push to other managers

*/

#ifndef DEBUGGER_DECLARATION_CPP
#define DEBUGGER_DECLARATION_CPP

#include "frontend/parser/Debugger.hpp"
#include "shared/Utils.hpp"

namespace AutoLang
{

HasClassIdNode* loadDeclaration(in_func, size_t& i) {
	auto declaration = &context.tokens[i];
	bool isVal = declaration->type == Lexer::TokenType::VAL;
	bool isGlobal = !context.currentClassId && context.currentFunctionId == context.mainFunctionId;
	bool isInFunction = !context.currentClassId || context.currentFunctionId != context.mainFunctionId;
	bool isStatic = context.modifierflags & ModifierFlags::STATIC;
	if (isStatic) {
		isGlobal = true;
		context.modifierflags &= ~ModifierFlags::STATIC;
	}
	Lexer::TokenType accessModifier = getAndEnsureOneAccessModifier(in_data, i);
	Lexer::Token *token = &context.tokens[i];
	uint32_t firstLine = token->line;
	//Name
	if (!nextTokenSameLine(&token, context.tokens, i, declaration->line) ||
		!expect(token, Lexer::TokenType::IDENTIFIER)) {
		--i;
		throw ParserError(context.tokens[i].line, "Expected name but not found");
	}
	std::string& name = context.lexerString[token->indexData];
	if (context.currentClassId) {
		if (isMapExist(context.getCurrentClass(in_data)->memberMap, name) || 
			isMapExist(context.getCurrentClassInfo(in_data)->staticMember, name))
			throw ParserError(token->line, "Declaration: Redefination variable name \"" + name + "\"");
	}
	//Sugar syntax val a? = 1
	bool nullable = true;
	bool sugarSyntax = false;
	if (!nextTokenSameLine(&token, context.tokens, i, declaration->line)) {
		--i;
		throw ParserError(firstLine, "Expected class name but not found");
	}
	if (expect(token, Lexer::TokenType::QMARK)) {
		sugarSyntax = true;
		//Class name
		if (!nextTokenSameLine(&token, context.tokens, i, declaration->line)) {
			--i;
			throw ParserError(firstLine, "Expected class name but not found");
		}
	} else
	if (expect(token, Lexer::TokenType::EXMARK)) {
		sugarSyntax = true;
		nullable = false;
		//Class name
		if (!nextTokenSameLine(&token, context.tokens, i, declaration->line)) {
			--i;
			throw ParserError(firstLine, "Expected class name but not found");
		}
	}
	HasClassIdNode* value = nullptr;
	std::string className;
	if (expect(token, Lexer::TokenType::COLON)) {
		auto classDeclaration = loadClassDeclaration(in_data, i, declaration->line);
		if (sugarSyntax) {
			throw ParserError(token->line, 
				std::string("Sugar syntax ") + (isVal ? "val " : "var ") + name + (nullable ? "?" : "!") + " can be use when fast declare, cannot use with " + classDeclaration.className + "? " +
				"use " + (isVal ? "val " : "var ") + name + ": " + classDeclaration.className + (nullable ? "?" : "!") + " instead of"
			);
		}
		nullable = classDeclaration.nullable;
		className = std::move(classDeclaration.className);
		if (!nextTokenSameLine(&token, context.tokens, i, declaration->line)) {
			if (nullable) {
				--i;
				value = new ConstValueNode(firstLine, DefaultClass::nullObject, 0);
				goto createNode;
			}
			--i;
			throw ParserError(context.tokens[i].line, className + " must have value");
		}
		// if (!nextTokenSameLine(&token, context.tokens, i, declaration->line) ||
		// 	 !expect(token, Lexer::TokenType::IDENTIFIER)) {
		// 	throw std::runtime_error("Expected class name but not found");
		// }
		// className = context.lexerString[token->indexData];
		// if (className == "Null")
		// 	throw std::runtime_error(name + " cannot declarate variable of type Null");
		// if (!nextTokenSameLine(&token, context.tokens, i, declaration->line)) {
		// 	--i;
		// 	goto createNode;
		// }
	}
	//Value
	if (expect(token, Lexer::TokenType::EQUAL)) {
		if (!nextToken(&token, context.tokens, i)) {
			--i;
			throw ParserError(context.tokens[i].line, "Expected expression after '=' but not found");
		}
		bool lastJustFindStatic = context.justFindStatic;
		context.justFindStatic = isStatic;
		value = loadExpression(in_data, 0, i);
		context.justFindStatic = lastJustFindStatic;
	} else {
		if (className.empty()) {
			throw ParserError(token->line, "Unknow what type of "+name);
		}
		value = new ConstValueNode(firstLine, DefaultClass::nullObject, 0);
		--i;
	}
	createNode:;
	std::string declarationName;
	if (isStatic) {
		if (context.currentFunctionId != context.mainFunctionId)
			declarationName = context.getCurrentFunction(in_data)->name + '.' + name;
		if (context.currentClassId)
			declarationName = context.getCurrentClass(in_data)->name + '.' + name;
	} else declarationName = name;

	if (value->kind == NodeType::CONST && static_cast<ConstValueNode*>(value)->id == 0) {
		//0 is null const object position
		if (!nullable) {
			throw ParserError(token->line, declarationName+ " must nullable to can detach null value");
		}
		if (className.empty()) {
			throw ParserError(token->line, std::string("Ambiguous call nullable ") + (isVal ? "val " : "var ") + name + "? = null");
		}
	}
	auto node = context.makeDeclarationNode(in_data, token->line, false, std::move(declarationName), 
		std::move(className), isVal, isGlobal, nullable, (isInFunction || isStatic));
	node->accessModifier = accessModifier;
	node->mustInferenceNullable = !sugarSyntax && node->className.empty();
	//printDebug(node->name + " is " + (node->accessModifier == Lexer::TokenType::PUBLIC ? "public" : "private"));
	//printDebug((isVal ? "val " : "var ") + name + " has id " + std::to_string(node->id) + " " + (isGlobal ? "1 " : "0 " ) + (isStatic ? "1 " : "0 "));
	if (isStatic) {
		//Function
		if (context.currentFunctionId != context.mainFunctionId) {
			auto it = context.getCurrentFunctionInfo(in_data)->scopes.back().find(name);
			if (it != context.getCurrentFunctionInfo(in_data)->scopes.back().end())
				throw ParserError(token->line, name + " has exist");
			context.getCurrentFunctionInfo(in_data)->scopes.back()[name] = node;
		} else { //Class
			context.getCurrentClassInfo(in_data)->staticMember[name] = node;
		}
		context.staticNode.push_back(context.setValuePool.push(
			firstLine,
			new VarNode(
				firstLine,
				node,
				false,
				true
			), value
		));
		return nullptr;
	}
	//Non static
	if (context.currentClassId && context.currentFunctionId == context.mainFunctionId) {
		uint32_t nodeId = context.getCurrentClass(in_data)->memberMap.size();
		// printDebug(compile.classes[context.currentClassInfo->declarationThis->classId]->name);
		// printDebug((uintptr_t)context.currentClass);
		std::cout << nodeId << " is node id of " << name <<'\n';
		context.getCurrentClass(in_data)->memberMap[node->name] = nodeId;
		context.getCurrentClass(in_data)->memberId.push_back(0);
		//Add member id
		node->id = nodeId;

		//Add Member
		context.getCurrentClassInfo(in_data)->member.push_back(node);
		auto setNode = context.setValuePool.push(
			firstLine,
			new GetPropNode(
				firstLine,
				node,
				context.currentClassId,
				new VarNode(
					firstLine,
					context.getCurrentClassInfo(in_data)->declarationThis,
					false,
					false
				),
				node->name,
				true,
				true,
				false
			),
			value
		);
		return setNode;
	}
	return context.setValuePool.push(
		firstLine,
		new VarNode(
			firstLine,
			node,
			true,
			true
		), 
		value
	);
}

}

#endif
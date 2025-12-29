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

#include "DebuggerDeclaration.hpp"
#include "Utils.hpp"

namespace AutoLang
{

HasClassIdNode* loadDeclaration(in_func, size_t& i) {
	auto declaration = &context.tokens[i];
	bool isVal = declaration->type == Lexer::TokenType::VAL;
	bool isGlobal = !context.currentClass && context.currentFunction == compile.main;
	bool isInFunction = !context.currentClass || context.currentFunction != context.mainFunction;
	bool isStatic = false;
	Lexer::TokenType accessModifier = Lexer::TokenType::PUBLIC;
	if (!context.keywords.empty()) {
		bool hasAccessModifier = false;
		for (auto& keyword : context.keywords) {
			switch (keyword) {
				case Lexer::TokenType::STATIC:
					if (isStatic) break;
					isStatic = true;
					isGlobal = true;
					continue;
				case Lexer::TokenType::PRIVATE:
				case Lexer::TokenType::PROTECTED:
				case Lexer::TokenType::PUBLIC: {
					if (hasAccessModifier) break;
					hasAccessModifier = true;
					accessModifier = keyword;
					continue;
				}
				default:
					throw std::runtime_error("Invalid keyword '"+Lexer::Token(0, keyword).toString(context)+"'");
			}
			throw std::runtime_error("Keyword '"+Lexer::Token(0, keyword).toString(context)+"' has declared");
		}
		if (hasAccessModifier && (!context.currentClass || context.currentFunction != compile.main)) {
			throw std::runtime_error("Cannot declare keyword '"+Lexer::Token(0, accessModifier).toString(context)+"' in function");
		}
	}
	Lexer::Token *token;
	//Name
	if (!nextTokenSameLine(&token, context.tokens, i, declaration->line) ||
		 !expect(token, Lexer::TokenType::IDENTIFIER)) {
		throw std::runtime_error("Expected name but not found");
	}
	std::string& name = context.lexerString[token->indexData];
	if (context.currentClass) {
		if (isMapExist(context.currentClass->memberMap, name) || 
			isMapExist(context.currentClassInfo->staticMember, name))
			throw std::runtime_error("Declaration: Redefination variable name \"" + name + "\"");
	}
	//Nullable
	bool nullable = false;
	if (!nextTokenSameLine(&token, context.tokens, i, declaration->line)) {
		throw std::runtime_error("Expected class name but not found");
	}
	if (expect(token, Lexer::TokenType::QMARK)) {
		nullable = true;
		//Class name
		if (!nextTokenSameLine(&token, context.tokens, i, declaration->line)) {
			throw std::runtime_error("Expected class name but not found");
		}
	}
	HasClassIdNode* value = nullptr;
	std::string className;
	if (expect(token, Lexer::TokenType::COLON)) {
		auto classDeclaration = loadClassDeclaration(in_data, i, declaration->line);
		if (nullable) {
			throw std::runtime_error(
				std::string("Sugar syntax ") + (isVal ? "val " : "var ") + name + "? can be use when fast declare, cannot use with " + classDeclaration.className + "? " +
				"use " + (isVal ? "val " : "var ") + name + ": " + classDeclaration.className + "? instead of"
			);
		}
		nullable = classDeclaration.nullable;
		className = std::move(classDeclaration.className);
		if (!nextTokenSameLine(&token, context.tokens, i, declaration->line)) {
			if (nullable) {
				--i;
				value = new ConstValueNode(DefaultClass::nullObject, 0);
				goto createNode;
			}
			throw std::runtime_error(className + " must have value");
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
			throw std::runtime_error("Expected expression after '=' but not found");
		}
		bool lastJustFindStatic = context.justFindStatic;
		context.justFindStatic = isStatic;
		value = loadExpression(in_data, 0, i);
		context.justFindStatic = lastJustFindStatic;
	} else {
		if (className.empty()) {
			throw std::runtime_error("Unknow what type of "+name);
		}
		--i;
	}
	createNode:;
	std::string declarationName;
	if (isStatic) {
		if (context.currentFunction != context.mainFunction)
			declarationName = context.currentFunction->name + '.' + name;
		if (context.currentClass)
			declarationName = context.currentClass->name + '.' + name;
	} else declarationName = name;

	if (value->kind == NodeType::CONST && static_cast<ConstValueNode*>(value)->id == 0) {
		//0 is null const object position
		if (!nullable) {
			throw std::runtime_error(declarationName+ " must nullable to can detach null value");
		}
		if (className.empty()) {
			throw std::runtime_error(std::string("Ambiguous call nullable ") + (isVal ? "val " : "var ") + name + "? = null");
		}
	}

	auto node = context.makeDeclarationNode(false, std::move(declarationName), 
		std::move(className), isVal, isGlobal, nullable, (isInFunction || isStatic));
	node->accessModifier = accessModifier;
	//printDebug(node->name + " is " + (node->accessModifier == Lexer::TokenType::PUBLIC ? "public" : "private"));
	//printDebug((isVal ? "val " : "var ") + name + " has id " + std::to_string(node->id) + " " + (isGlobal ? "1 " : "0 " ) + (isStatic ? "1 " : "0 "));
	if (isStatic) {
		//Function
		if (context.currentFunction != compile.main) {
			auto it = context.currentFuncInfo->scopes.back().find(name);
			if (it != context.currentFuncInfo->scopes.back().end())
				throw std::runtime_error(name + " has exist");
			context.currentFuncInfo->scopes.back()[name] = node;
		} else { //Class
			context.currentClassInfo->staticMember[name] = node;
		}
		context.staticNode.push_back(new SetNode(
			new VarNode(
				node,
				false
			), value
		));
		return nullptr;
	}
	//Non static
	if (context.currentClass && context.currentFunction == compile.main) {
		uint32_t nodeId = context.currentClass->memberMap.size();
		// printDebug(compile.classes[context.currentClassInfo->declarationThis->classId].name);
		// printDebug((uintptr_t)context.currentClass);
		context.currentClass->memberMap[node->name] = nodeId;
		context.currentClass->memberId.push_back(0);
		//Add member id
		node->id = nodeId;

		//Add Member
		context.currentClassInfo->member.push_back(node);
		auto setNode = new SetNode(
			new GetPropNode(
				node,
				context.getCurrentContextClassId(),
				new VarNode(
					context.currentClassInfo->declarationThis,
					false
				),
				node->name,
				true
			),
			value
		);
		return setNode;
	}
	return new SetNode(new VarNode(
		node,
		true
	), value);
}

}

#endif
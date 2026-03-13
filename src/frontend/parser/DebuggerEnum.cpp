#ifndef DEBUGGER_ENUM_CPP
#define DEBUGGER_ENUM_CPP

#include "Debugger.hpp"
#include "frontend/ACompiler.hpp"
#include "shared/ClassFlags.hpp"

namespace AutoLang {

inline void loadEnumBody(in_func, size_t &i, CreateClassNode *node,
                         uint32_t firstLine, AClass *lastClass) {
	Lexer::Token *token = &context.tokens[i];
	auto *classInfo = context.getCurrentClassInfo(in_data);
	while (true) {
		if (!nextToken(&token, context.tokens, i)) {
			throw ParserError(firstLine,
			                  "Bug: Lexer is not ensure close bracket");
		}
	initial:;
		switch (token->type) {
			case Lexer::TokenType::SEMI_COLON: {
				loadBody<true>(in_data, node->body.nodes, i, false);
				context.gotoClass(lastClass);
				return;
			}
			case Lexer::TokenType::IDENTIFIER: {
			loadValue:;
				auto &name = context.lexerString[token->indexData];
				// auto classDeclaration =
				//     context.classDeclarationAllocator.push();
				// classDeclaration->classId = node->classId;
				// classDeclaration->nullable = false;
				// classDeclaration->line = token->line;
				// auto declarationNode = context.makeDeclarationNode(
				//     in_data, token->line, false, name, classDeclaration,
				//     true, true, false, false);
				// declarationNode->accessModifier = Lexer::TokenType::PUBLIC;
				// declarationNode->mustInferenceNullable = false;
				auto it = classInfo->constValue.find(token->indexData);
				if (it != classInfo->constValue.end()) {
					throw ParserError(
					    token->line, "Duplicat value " +
					                     context.lexerString[token->indexData]);
				}
				Offset id = compile.registerEnumConstPool(node->classId);
				classInfo->constValue[token->indexData] =
				    new ConstValueNode(token->line, compile.constPool[id], id);
				if (!nextToken(&token, context.tokens, i)) {
					throw ParserError(firstLine,
					                  "Bug: Lexer is not ensure close bracket");
				}
				switch (token->type) {
					case Lexer::TokenType::COMMA: {
						if (!nextToken(&token, context.tokens, i) ||
						    !expect(token, Lexer::TokenType::IDENTIFIER)) {
							throw ParserError(
							    firstLine, "Expected value name but not found");
						}
						goto loadValue;
					}
					case Lexer::TokenType::SEMI_COLON: {
						goto initial;
					}
					case Lexer::TokenType::RBRACE: {
						context.gotoClass(lastClass);
						return;
					}
					default: {
						throw ParserError(token->line,
						                  "Expected value but '" +
						                      token->toString(context) +
						                      "' found");
					}
				}
				break;
			}
			case Lexer::TokenType::RBRACE: {
				context.gotoClass(lastClass);
				return;
			}
			default: {
				throw ParserError(token->line, "Expected value but '" +
				                                   token->toString(context) +
				                                   "' found");
			}
		}
	}
}

void loadEnum(in_func, size_t &i) {
	ensureNoKeyword(in_data, i);
	ensureNoAnnotations(in_data, i);
	Lexer::Token *token = &context.tokens[i];
	uint32_t firstLine = token->line;

	uint32_t classFlags =
	    ClassFlags::CLASS_IS_ENUM | ClassFlags::CLASS_NO_EXTENDS;

	// Name
	if (!nextTokenSameLine(&token, context.tokens, i, firstLine) ||
	    !expect(token, Lexer::TokenType::IDENTIFIER)) {
		--i;
		throw ParserError(firstLine, "Expected name but not found");
	}
	LexerStringId nameId = token->indexData;
	const std::string &name = context.lexerString[nameId];

	if (isClassExist(in_data, name)) {
		throw ParserError(firstLine, "Class " + name + " has exists");
	}

	auto node = context.newClasses.push(firstLine, nameId,
	                                    classFlags); // NewClasses managed
	node->pushClass(in_data);
	auto lastClass = context.getCurrentClass(in_data);
	auto clazz = compile.classes[node->classId];
	context.gotoClass(clazz);
	auto declarationThis = context.declarationNodePool.push(
	    firstLine, context.currentClassId, "this", nullptr, true, false, false);
	declarationThis->classId = node->classId;
	//'this' is always input at first position
	declarationThis->id = 0;
	auto classInfo = context.getCurrentClassInfo(in_data);
	classInfo->declarationThis = declarationThis;

	if (!nextToken(&token, context.tokens, i)) {
		--i;
		context.newDefaultClassesMap[node->classId] = node;
		context.gotoClass(lastClass);
		return;
	}
	if (expect(token, Lexer::TokenType::LT)) {
		throw ParserError(firstLine, "Enum doesn't support generics");
	} else {
		context.newDefaultClassesMap[node->classId] = node;
	}

	if (expect(token, Lexer::TokenType::EXTENDS)) {
		throw ParserError(firstLine, "Enum doesn't support extends");
	}
	// std::cerr<<"Clazz: "<<clazz->name<<" "<<declarationThis->id<<"\n";
	// Has PrimaryConstructor
	//  bool hasPrimaryConstructor = false;
	if (expect(token, Lexer::TokenType::LPAREN)) {
		throw ParserError(firstLine,
		                  "Enum doesn't support primary constructor");
	}

	if (expect(token, Lexer::TokenType::LBRACE)) {
		loadEnumBody(in_data, i, node, firstLine, lastClass);
		context.gotoClass(lastClass);
	} else {
		context.gotoClass(lastClass);
		--i;
	}
}

} // namespace AutoLang

#endif
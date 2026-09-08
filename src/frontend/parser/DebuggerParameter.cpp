#ifndef DEBUGGER_PARAMETER_CPP
#define DEBUGGER_PARAMETER_CPP

#include "frontend/ACompiler.hpp"
#include "frontend/parser/Debugger.hpp"
#include "frontend/parser/ParserContext.hpp"

namespace Autolang {

template <Lexer::TokenType closeBracket, bool mustHaveColon,
          bool allowDefaultValue>
Parameter *loadListDeclaration(in_func, size_t &i, bool allowVar) {
	Lexer::Token *token = &context.tokens[i];
	if (!nextToken(&token, context.tokens, i)) {
		--i;
		throw ParserError(context.tokens[i].line,
		                  "Bug: Lexer did not ensure a closing bracket");
	}
	auto parameter = context.parameterPool.push();
	switch (token->type) {
		case closeBracket: {
			parameter->defaultValuePos = 0;
			return parameter;
		}
		case Lexer::TokenType::VAR:
		case Lexer::TokenType::VAL: {
			if (!allowVar)
				throw ParserError(
				    token->line,
				    token->toString(context) +
				        " can't be allowed here\nHint: Do not use 'var' or "
				        "'val' keywords in function parameters unless in "
				        "constructor");
		}
		case Lexer::TokenType::IDENTIFIER:
		case Lexer::TokenType::TO: {
			--i;
			break;
		}
		default:
			throw ParserError(
			    token->line,
			    "Expected parameter name but not found\nHint: Provide a "
			    "parameter name identifier");
	}
	bool addedDefaultValue = false;
	while (nextToken(&token, context.tokens, i)) {
		bool isVal = true;
		if (allowVar) {
			if (!expect(token, Lexer::TokenType::VAR) &&
			    !expect(token, Lexer::TokenType::VAL)) {
				--i;
				throw ParserError(
				    context.tokens[i].line,
				    "Expected var or val but not found\nHint: Primary "
				    "constructor parameters must specify 'var' or 'val'");
			}
			isVal = token->type == Lexer::TokenType::VAL;
			if (!nextToken(&token, context.tokens, i)) {
				--i;
				throw ParserError(
				    context.tokens[i].line,
				    "Expected parameter name but not found\nHint: Provide an "
				    "identifier for parameter name after 'var'/'val'");
			}
		}
		if (!expect(token, Lexer::TokenType::IDENTIFIER) &&
		    !expect(token, Lexer::TokenType::TO)) {
			throw ParserError(
			    token->line,
			    "Expected identifier but not found\nHint: Parameter name "
			    "must be a valid identifier");
		}
		LexerStringId baseName = token->indexData;
		const std::string &name = context.lexerString[token->indexData];
		if (!nextToken(&token, context.tokens, i)) {
			--i;
			throw ParserError(
			    context.tokens[i].line,
			    "Expected ':' after parameter name but not found\nHint: Add "
			    "':' after parameter name to declare parameter type");
		}
		Autolang::ClassDeclaration *classDeclaration = nullptr;
		if (!expect(token, Lexer::TokenType::COLON)) {
			if constexpr (mustHaveColon) {
				throw ParserError(
				    token->line,
				    "Expected ':' after parameter name but not found\nHint: "
				    "Parameter type annotation requires ':'");
			}
		} else {
			classDeclaration =
			    loadClassDeclaration(in_data, i, token->line, false);
			if (!classDeclaration->isGenerics(in_data)) {
				context.allClassDeclarations.push_back(classDeclaration);
			}
			if (!nextToken(&token, context.tokens, i)) {
				--i;
				break;
			}
		}
		// if (classDeclaration) {
		// 	std::cerr << name << ": " <<
		// classDeclaration->getName<true>(in_data) << "\n";
		// }
		auto node = context.makeDeclarationNode(
		    in_data, token->line, baseName, name, classDeclaration, isVal,
		    false, classDeclaration ? classDeclaration->nullable : true, false,
		    false);
		parameter->parameters.push_back(node);
		if (expect(token, Lexer::TokenType::EQUAL)) {
			if constexpr (!allowDefaultValue) {
				throw ParserError(
				    token->line,
				    "Unexpected default value\nHint: Default parameter values "
				    "are not allowed in this declaration");
			}
			if (!nextToken(&token, context.tokens, i)) {
				--i;
				throw ParserError(
				    context.tokens[i].line,
				    "Expected value after '=' but not found\nHint: Provide a "
				    "default value expression after '='");
			}
			if (!addedDefaultValue) {
				addedDefaultValue = true;
				parameter->defaultValuePos = parameter->parameters.size() - 1;
			}
			auto value = loadExpression(in_data, 0, i);
			parameter->parameterDefaultValues.push_back(value);
			if (!nextToken(&token, context.tokens, i)) {
				--i;
				break;
			}
		} else if (addedDefaultValue) {
			throw ParserError(
			    token->line,
			    "Parameter with default value cannot precede parameter "
			    "without default value\nHint: Place all parameters with "
			    "default values at the end of parameter list");
		}
		switch (token->type) {
			using namespace Lexer;
			case closeBracket: {
				if (!addedDefaultValue) {
					parameter->defaultValuePos = parameter->parameters.size();
				}
				return parameter;
			}
			case TokenType::COMMA: {
				break;
			}
			default: {
				throw ParserError(
				    token->line,
				    "Unexpected token '" + token->toString(context) +
				        "'\nHint: Expected ',' or closing bracket in parameter "
				        "list");
			}
		}
	}
	--i;
	throw ParserError(context.tokens[i].line,
	                  "Bug: Lexer did not ensure a closing bracket");
}

template Parameter *
loadListDeclaration<Lexer::TokenType::RPAREN, true, true>(in_func, size_t &i,
                                                          bool allowVar);
template Parameter *
loadListDeclaration<Autolang::Lexer::OR, false, false>(in_func, size_t &i,
                                                       bool allowVar);

} // namespace Autolang

#endif

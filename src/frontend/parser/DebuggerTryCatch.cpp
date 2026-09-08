#ifndef DEBUGGER_TRY_CATCH_CPP
#define DEBUGGER_TRY_CATCH_CPP

#include "frontend/parser/Debugger.hpp"
#include "frontend/parser/ParserContext.hpp"

namespace Autolang {

TryCatchNode *loadTryCatch(in_func, size_t &i, bool mustReturnValue) {
	Lexer::Token *token = &context.tokens[i];
	uint32_t firstLine = token->line;
	TryCatchNode *node =
	    context.tryCatchPool.push(firstLine, mustReturnValue); // WhilePool managed
	// Condition
	if (!nextToken(&token, context.tokens, i) ||
	    !expect(token, Lexer::TokenType::LBRACE)) {
		--i;
		throw ParserError(
		    firstLine,
		    "Expected body open with '{' after 'try' but not found\nHint: Open "
		    "the try block with '{' immediately after 'try'");
	}
	loadBody<false>(in_data, node->body.nodes, i, true);
	if (!nextToken(&token, context.tokens, i)) {
		--i;
		throw ParserError(
		    firstLine,
		    "Expected 'catch' or 'finally' after 'try' but not found\nHint: Add "
		    "a 'catch (e) { ... }' or 'finally { ... }' block after the try block");
	}
	bool hasCatchAll = false;
	while (expect(token, Lexer::TokenType::CATCH)) {
		if (hasCatchAll) {
			--i;
			throw ParserError(
			    token->line,
			    "Catch clause after catch-all 'catch (e)' is unreachable\nHint: Place more specific catch clauses before the catch-all clause");
		}
		node->hasCatch = true;
		CatchClause clause(token->line);
		if (!nextToken(&token, context.tokens, i) ||
		    !expect(token, Lexer::TokenType::LPAREN)) {
			--i;
			throw ParserError(
			    firstLine,
			    "Expected '(' after 'catch' but not found\nHint: Specify exception "
			    "variable in parentheses: catch (e) or catch (e: ExceptionType)");
		}
		if (!nextToken(&token, context.tokens, i) ||
		    !expect(token, Lexer::TokenType::IDENTIFIER)) {
			--i;
			throw ParserError(
			    firstLine,
			    "Expected a variable name after 'catch' but not found\nHint: "
			    "Provide an exception variable identifier inside catch (e)");
		}
		LexerStringId baseName = token->indexData;
		const std::string &name = context.lexerString[token->indexData];
		if (!nextToken(&token, context.tokens, i)) {
			--i;
			throw ParserError(
			    firstLine,
			    "Expected ':' or ')' after catch variable name but not found\nHint: "
			    "Use 'catch (e)' or 'catch (e: ExceptionType)'");
		}
		if (expect(token, Lexer::TokenType::COLON)) {
			clause.classDeclaration =
			    loadClassDeclaration(in_data, i, token->line, false);
			if (!clause.classDeclaration->isGenerics(in_data)) {
				context.allClassDeclarations.push_back(clause.classDeclaration);
			}
			if (!nextToken(&token, context.tokens, i) ||
			    !expect(token, Lexer::TokenType::RPAREN)) {
				--i;
				throw ParserError(
				    firstLine,
				    "Expected ')' after catch type specification but not found\nHint: "
				    "Close catch parameters with ')' after the exception type");
			}
		} else if (expect(token, Lexer::TokenType::RPAREN)) {
			clause.isCatchAll = true;
			hasCatchAll = true;
		} else {
			--i;
			throw ParserError(
			    firstLine,
			    "Expected ':' or ')' after catch variable name but not found\nHint: "
			    "Use 'catch (e)' or 'catch (e: ExceptionType)'");
		}
		if (!nextToken(&token, context.tokens, i) ||
		    !expect(token, Lexer::TokenType::LBRACE)) {
			--i;
			throw ParserError(
			    firstLine,
			    "Expected body to open with '{' after 'catch' but not found\nHint: "
			    "Open the catch block with '{' after catch (...)");
		}
		auto funcInfo = context.getCurrentFunctionInfo(in_data);
		if (context.currentClosureNode) {
			context.currentClosureNode->scopes.emplace_back();
		} else {
			funcInfo->scopes.emplace_back();
		}
		auto declarationNode = context.makeDeclarationNode(
		    in_data, token->line, baseName, name, clause.classDeclaration, true,
		    context.currentFunctionId == context.mainFunctionId &&
		        !context.currentClosureNode,
		    false, true, true);
		if (!clause.classDeclaration) {
			declarationNode->classId = Autolang::DefaultClass::exceptionClassId;
		}
		if (context.currentClosureNode) {
			context.currentClosureNode->scopes.back()[baseName] = declarationNode;
		} else {
			funcInfo->scopes.back()[baseName] = declarationNode;
		}
		clause.exceptionDeclaration = declarationNode;
		loadBody<false>(in_data, clause.body.nodes, i, false);
		if (context.currentClosureNode) {
			context.currentClosureNode->declarationCount -=
			    context.currentClosureNode->scopes.back().size();
			context.currentClosureNode->scopes.pop();
		} else {
			funcInfo->popBackScope();
		}
		node->catchClauses.push_back(std::move(clause));
		if (!nextToken(&token, context.tokens, i)) {
			token = nullptr;
			break;
		}
	}

	if (token && expect(token, Lexer::TokenType::FINALLY)) {
		node->hasFinally = true;
		if (!nextToken(&token, context.tokens, i) ||
		    !expect(token, Lexer::TokenType::LBRACE)) {
			--i;
			throw ParserError(
			    firstLine,
			    "Expected body to open with '{' after 'finally' but not found\nHint: "
			    "Open the finally block with '{' after finally");
		}
		loadBody<false>(in_data, node->finallyBody.nodes, i, true);
	} else if (token) {
		--i;
	}

	if (!node->hasCatch && !node->hasFinally) {
		throw ParserError(
		    firstLine,
		    "Expected 'catch' or 'finally' after 'try' but not found\nHint: Add "
		    "a 'catch (e) { ... }' or 'finally { ... }' block after the try block");
	}

	if (node->hasCatch) {
		node->exceptionDeclaration = node->catchClauses[0].exceptionDeclaration;
		node->catchBody.nodes = node->catchClauses[0].body.nodes;
		node->tempExceptionVar = context.makeDeclarationNode(
		    in_data, firstLine,
		    context.createLexerStringIfNotExists(".exception"), ".exception",
		    nullptr, true,
		    context.currentFunctionId == context.mainFunctionId &&
		        !context.currentClosureNode,
		    false, false, true);
		node->tempExceptionVar->classId = Autolang::DefaultClass::exceptionClassId;
	}

	if (mustReturnValue && !node->hasCatch) {
		throw ParserError(
		    firstLine,
		    "'try' expression must return a value, so it must have a 'catch' branch\nHint: Add a 'catch (e) { ... }' branch to handle exceptions when evaluated as an expression");
	}
	return node;
}

ThrowNode *loadThrow(in_func, size_t &i) {
	Lexer::Token *token = &context.tokens[i];
	uint32_t firstLine = token->line;
	if (!nextTokenSameLine(&token, context.tokens, i, token->line)) {
		throw ParserError(
		    firstLine,
		    "Expected an expression after 'throw' but not found\nHint: Provide "
		    "an Exception expression to throw: throw Exception(\"...\")");
	}
	return context.throwPool.push(firstLine, loadExpression(in_data, 0, i));
}

} // namespace Autolang

#endif
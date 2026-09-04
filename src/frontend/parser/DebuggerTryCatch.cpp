#ifndef DEBUGGER_TRY_CATCH_CPP
#define DEBUGGER_TRY_CATCH_CPP

#include "frontend/parser/Debugger.hpp"
#include "frontend/parser/ParserContext.hpp"

namespace Autolang {

TryCatchNode *loadTryCatch(in_func, size_t &i) {
	Lexer::Token *token = &context.tokens[i];
	uint32_t firstLine = token->line;
	TryCatchNode *node =
	    context.tryCatchPool.push(firstLine); // WhilePool managed
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
	if (expect(token, Lexer::TokenType::CATCH)) {
		node->hasCatch = true;
		if (!nextToken(&token, context.tokens, i) ||
		    !expect(token, Lexer::TokenType::LPAREN)) {
			--i;
			throw ParserError(
			    firstLine,
			    "Expected '(' after 'catch' but not found\nHint: Specify exception "
			    "variable in parentheses: catch (e)");
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
		if (!nextToken(&token, context.tokens, i) ||
		    !expect(token, Lexer::TokenType::RPAREN)) {
			--i;
			throw ParserError(
			    firstLine,
			    "Expected ')' after catch variable name but not found\nHint: Close "
			    "catch parameters with ')' after the exception variable");
		}
		if (!nextToken(&token, context.tokens, i) ||
		    !expect(token, Lexer::TokenType::LBRACE)) {
			--i;
			throw ParserError(
			    firstLine,
			    "Expected body to open with '{' after 'catch' but not found\nHint: "
			    "Open the catch block with '{' after catch (e)");
		}
		auto funcInfo = context.getCurrentFunctionInfo(in_data);
		if (context.currentClosureNode) {
			context.currentClosureNode->scopes.emplace_back();
		} else {
			funcInfo->scopes.emplace_back();
		}
		auto declarationNode = context.makeDeclarationNode(
		    in_data, token->line, baseName, name, nullptr, true,
		    context.currentFunctionId == context.mainFunctionId &&
		        !context.currentClosureNode,
		    false, true, true);
		declarationNode->classId = Autolang::DefaultClass::exceptionClassId;
		if (context.currentClosureNode) {
			context.currentClosureNode->scopes.back()[baseName] = declarationNode;
		} else {
			funcInfo->scopes.back()[baseName] = declarationNode;
		}
		node->exceptionDeclaration = declarationNode;
		loadBody<false>(in_data, node->catchBody.nodes, i, false);
		if (context.currentClosureNode) {
			context.currentClosureNode->declarationCount -=
			    context.currentClosureNode->scopes.back().size();
			context.currentClosureNode->scopes.pop();
		} else {
			funcInfo->popBackScope();
		}
		if (nextToken(&token, context.tokens, i)) {
			if (expect(token, Lexer::TokenType::FINALLY)) {
				node->hasFinally = true;
				if (!nextToken(&token, context.tokens, i) ||
				    !expect(token, Lexer::TokenType::LBRACE)) {
					--i;
					throw ParserError(
					    firstLine,
					    "Expected body to open with '{' after 'finally' but not "
					    "found\nHint: Open the finally block with '{' after "
					    "finally");
				}
				loadBody<false>(in_data, node->finallyBody.nodes, i, true);
			} else {
				--i;
			}
		}
	} else if (expect(token, Lexer::TokenType::FINALLY)) {
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
	} else {
		--i;
		throw ParserError(
		    firstLine,
		    "Expected 'catch' or 'finally' after 'try' but not found\nHint: Add "
		    "a 'catch (e) { ... }' or 'finally { ... }' block after the try block");
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
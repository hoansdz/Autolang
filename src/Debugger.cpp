#ifndef DEBUGGER_CPP
#define DEBUGGER_CPP

#include <chrono>
#include <cctype>
#include <cstdlib>
#include <memory>
#include "Debugger.hpp"
#include <functional>

namespace AutoLang
{

	template <typename T>
	void build(CompiledProgram &compile, T &data)
	{
		auto startCompiler = std::chrono::high_resolution_clock::now();
		ParserContext context;
		context.gotoFunction(compile.mainFunctionId);
		context.mainFunctionId = compile.mainFunctionId;
		context.line = 0;
		size_t i = 0;
		try
		{
			Lexer::Context lexerContext;
			context.tokens = Lexer::load(&context, data, lexerContext);
			auto lexerTime = std::chrono::high_resolution_clock::now();
			std::cout << "Lexer time : " << std::chrono::duration_cast<std::chrono::milliseconds>(lexerTime - startCompiler).count() << " ms" << '\n';
			estimate(in_data, lexerContext);

			context.constValue["null"] = std::pair(DefaultClass::nullObject, 0);
			context.constValue["true"] = std::pair(DefaultClass::trueObject, 1);
			context.constValue["false"] = std::pair(DefaultClass::falseObject, 2);
			// printDebug(context.tokens.size());
			//  Lexer::Token* token;
			//  while (i != context.tokens.size()) {
			//  	token = &context.tokens[i];
			//  	uint32_t line = token->line;
			//  	std::cout<<"["<<i<<"] "<<token->toString(context);
			//  	while (nextTokenSameLine(&token, context.tokens, i, line)) {
			//  		std::cout<<" "<<token->toString(context);
			//  	}
			//  	std::cout<<'\n';
			//  }
			i = 0;
			while (i < context.tokens.size())
			{
				auto node = loadLine(in_data, i);
				++i;
				if (node == nullptr)
				{
					continue;
				}
				context.getCurrentFunctionInfo(in_data)->block.nodes.push_back(node);
			}
			auto parserTime = std::chrono::high_resolution_clock::now();
			std::cout << "Parser time : " << std::chrono::duration_cast<std::chrono::milliseconds>(parserTime - lexerTime).count() << " ms" << '\n';
			/*context.currentFuncInfo->block.nodes.insert(
				context.currentFuncInfo->block.nodes.begin(),
				context.staticNode.begin(),
				context.staticNode.end()
			);*/
			printDebug("-----------------AST Node-----------------\n");
			resolve(in_data);
			auto resolveTime = std::chrono::high_resolution_clock::now();
			std::cout << "Optimize and Putbytecode time : " << std::chrono::duration_cast<std::chrono::milliseconds>(resolveTime - parserTime).count() << " ms" << '\n';
		}
		catch (const std::exception &e)
		{
			std::cout << "[" << context.line << ", " << i << "] : " << e.what() << '\n';
		}
		freeData(in_data);
	}

	void freeData(in_func)
	{
		for (auto &[_, funcInfo] : context.functionInfo)
		{
			funcInfo.block.refresh();
		}
		context.createConstructorPool.refresh();
		context.newClasses.refresh();
		context.newFunctions.refresh();
		// context.binaryNodePool.refresh();
	}

	void estimate(in_func, Lexer::Context &lexerContext)
	{
		uint32_t estimateNewClasses = lexerContext.estimate.classes;
		uint32_t estimateNewConstructorNode = lexerContext.estimate.classes + lexerContext.estimate.constructorNode;
		uint32_t estimateNewFunctions = lexerContext.estimate.functions + lexerContext.estimate.constructorNode;
		uint32_t estimateAllClasses = compile.classes.size() + estimateNewClasses;
		uint32_t estimateAllFunctions = compile.functions.size() + estimateNewFunctions + estimateNewClasses;
		uint32_t estimateDeclaration = lexerContext.estimate.declaration + estimateNewClasses;
		// uint32_t estimateBinaryNode = lexerContext.estimate.binaryNode;

		context.keywords.reserve(5);

		context.createConstructorPool.allocate(estimateNewConstructorNode);
		context.declarationNodePool.allocate(estimateDeclaration);
		context.ifPool.allocate(lexerContext.estimate.ifNode);
		context.whilePool.allocate(lexerContext.estimate.whileNode);
		context.returnPool.allocate(lexerContext.estimate.returnNode + lexerContext.estimate.constructorNode + estimateNewClasses);
		context.setValuePool.allocate(lexerContext.estimate.setNode);
		// context.binaryNodePool.allocate(estimateBinaryNode);

		context.constValue.reserve(
			3 // Const
		);

		context.newClasses.allocate(estimateNewClasses);
		context.newFunctions.allocate(estimateNewFunctions);

		context.classInfo.reserve(estimateAllClasses);
		context.functionInfo.reserve(estimateAllFunctions);

		compile.classes.reserve(estimateAllClasses);
		compile.classMap.reserve(estimateAllClasses);
		compile.functions.reserve(estimateAllFunctions);
		compile.funcMap.reserve(estimateAllFunctions);

		printDebug("Estimate Declarations: " + std::to_string(estimateDeclaration));
		printDebug("Estimate New classes: " + std::to_string(estimateNewClasses));
		printDebug("Estimate New functions: " + std::to_string(estimateNewFunctions));
		printDebug("Estimate All classes: " + std::to_string(estimateAllClasses));
		printDebug("Estimate All functions: " + std::to_string(estimateAllFunctions));
	}

	void resolve(in_func)
	{
		printDebug(context.getMainFunction(in_data)->bytecodes.size());
		printDebug("Start optimize declaration nodes in functions");
		for (int i = 0; i < context.declarationNodePool.index; ++i)
		{
			context.declarationNodePool.objects[i].optimize(in_data);
		}
		for (auto *node : context.declarationNodePool.vecs)
		{
			node->optimize(in_data);
		}
		printDebug("Start optimize constructor nodes");
		size_t sizeNewClasses = context.newClasses.getSize();
		for (int i = 0; i < sizeNewClasses; ++i)
		{
			auto *node = context.newClasses[i];
			auto clazz = &compile.classes[node->classId];
			auto classInfo = &context.classInfo[clazz->id];
			if (classInfo->primaryConstructor)
			{
				classInfo->primaryConstructor->optimize(in_data);
			}
			else
			{
				for (auto *constructor : classInfo->secondaryConstructor)
				{
					constructor->optimize(in_data);
				}
			}
		}
		printDebug("Start optimize classes");
		for (int i = 0; i < sizeNewClasses; ++i)
		{
			context.newClasses[i]->optimize(in_data);
		}

		printDebug("Start optimize static nodes");
		for (auto &node : context.staticNode)
		{
			node->optimize(in_data);
		}
		printDebug("Start optimize functions");
		size_t sizeNewFunctions = context.newFunctions.getSize();
		for (int i = 0; i < sizeNewFunctions; ++i)
		{
			context.newFunctions[i]->optimize(in_data);
		}
		printDebug("Start put bytecodes constructor");
		for (int i = 0; i < sizeNewClasses; ++i)
		{
			auto *node = context.newClasses[i];
			auto classInfo = &context.classInfo[compile.classes[node->classId].id];
			if (classInfo->primaryConstructor)
			{
				// Put initial bytecodes, example val a = 5 => SetNode
				//  node->body.optimize(in_data);
				auto func = &compile.functions[classInfo->primaryConstructor->funcId];
				node->body.putBytecodes(in_data, func->bytecodes);
				node->body.rewrite(in_data, func->bytecodes);
			}
			else
			{
				for (auto &constructor : classInfo->secondaryConstructor)
				{
					auto func = &compile.functions[constructor->funcId];
					// Put initial bytecodes, example val a = 5 => SetNode
					//  node->body.optimize(in_data);
					node->body.putBytecodes(in_data, func->bytecodes);
					node->body.rewrite(in_data, func->bytecodes);
					// Put constructor bytecodes
					constructor->body.optimize(in_data);
					constructor->body.putBytecodes(in_data, func->bytecodes);
					constructor->body.rewrite(in_data, func->bytecodes);
				}
			}
		}
		printDebug("Start put bytecodes static nodes");
		for (auto &node : context.staticNode)
		{
			node->putBytecodes(in_data, context.getMainFunction(in_data)->bytecodes);
			node->rewrite(in_data, context.getMainFunction(in_data)->bytecodes);
		}
		context.getMainFunctionInfo(in_data)->block.optimize(in_data);
		printDebug("Start put bytecodes in functions");
		for (int i = 0; i < sizeNewFunctions; ++i)
		{
			auto *node = context.newFunctions[i];
			auto func = &compile.functions[node->id];
			node->body.optimize(in_data);
			node->body.putBytecodes(in_data, func->bytecodes);
			node->body.rewrite(in_data, func->bytecodes);
		}
		printDebug("Start put bytecodes in main");
		context.getMainFunctionInfo(in_data)->block.putBytecodes(in_data, context.getMainFunction(in_data)->bytecodes);
		context.getMainFunctionInfo(in_data)->block.rewrite(in_data, context.getMainFunction(in_data)->bytecodes);

		printDebug("Real Declarations: " + std::to_string(context.declarationNodePool.index + context.declarationNodePool.vecs.size()));
		printDebug("Real New classes: " + std::to_string(context.newClasses.getSize()));
		printDebug("Real New functions: " + std::to_string(context.newFunctions.getSize()));
		printDebug("Real ClassInfo: " + std::to_string(context.classInfo.size()));
		printDebug("Real FunctionInfo: " + std::to_string(context.functionInfo.size()));

		printDebug("Real Classes: " + std::to_string(compile.classes.size()));
		printDebug("Real Functions: " + std::to_string(compile.functions.size()));
		printDebug("Real ClassMap: " + std::to_string(compile.classMap.size()));
		printDebug("Real FuncMap: " + std::to_string(compile.funcMap.size()));
		// size_t total = 0;
		// for (auto& [k, v] : compile.funcMap)
		// 	++total;

		// printDebug("TOTAL FUNC IN MAP: " + std::to_string(total));
	}

	ClassDeclaration loadClassDeclaration(in_func, size_t &i, uint32_t line)
	{ // Has check
		ClassDeclaration result;
		Lexer::Token *token;
		if (!nextTokenSameLine(&token, context.tokens, i, line) ||
			!expect(token, Lexer::TokenType::IDENTIFIER))
		{
			throw std::runtime_error("Expected class name but not found");
		}
		result.className = context.lexerString[token->indexData];
		if (result.className == "Null")
			throw std::runtime_error(result.className + " cannot declarate variable of type Null");
		if (!nextTokenSameLine(&token, context.tokens, i, line) ||
			!expect(token, Lexer::TokenType::QMARK))
		{
			--i;
			return result;
		}
		result.nullable = true;
		return result;
	}

	ExprNode *loadLine(in_func, size_t &i)
	{
		Lexer::Token *token = &context.tokens[i];
		context.keywords.clear();
	initial:;
		context.line = token->line;
		bool isInFunction = !context.currentClassId || context.currentFunctionId != context.mainFunctionId;
		switch (token->type)
		{
		case Lexer::TokenType::LBRACE:
		{
			if (context.keywords.size() == 1 && context.keywords[0] == Lexer::TokenType::STATIC)
			{
				if (isInFunction)
				{
					// std::cerr<<context.currentFunction->name<<'\n';
					goto err_call_class;
				}
				// loadClassInit(in_data, i);
				return nullptr;
			}
		}
		case Lexer::TokenType::LPAREN:
		case Lexer::TokenType::LBRACKET:
		case Lexer::TokenType::NUMBER:
		case Lexer::TokenType::STRING:
		case Lexer::TokenType::IDENTIFIER:
		{
			if (!isInFunction)
			{
				throw std::runtime_error("Cannot");
				goto err_call_func;
			}
			return parsePrimary(in_data, i);
		}
		case Lexer::TokenType::VAR:
		case Lexer::TokenType::VAL:
		{
			auto node = loadDeclaration(in_data, i);
			/*if (context.currentClass != nullptr) {
				context.currentClassInfo->constructor->body.nodes.push_back(node);
				return nullptr;
			}*/
			return node;
		}
		case Lexer::TokenType::BREAK:
		case Lexer::TokenType::CONTINUE:
		{
			if (!isInFunction)
				goto err_call_func;
			if (!context.canBreakContinue)
				throw std::runtime_error("Cannot use " + Lexer::Token(0, token->type).toString(context) + " here");
			return new SkipNode(token->type);
		}
		case Lexer::TokenType::IF:
		{
			if (!isInFunction)
				goto err_call_func;
			return loadIf(in_data, i);
		}
		case Lexer::TokenType::FOR:
		{
			if (!isInFunction)
				goto err_call_func;
			bool lastCanBreakContinue = context.canBreakContinue;
			context.canBreakContinue = true;
			auto node = loadFor(in_data, i);
			context.canBreakContinue = lastCanBreakContinue;
			return node;
		}
		case Lexer::TokenType::WHILE:
		{
			if (!isInFunction)
				goto err_call_func;
			bool lastCanBreakContinue = context.canBreakContinue;
			context.canBreakContinue = true;
			auto node = loadWhile(in_data, i);
			context.canBreakContinue = lastCanBreakContinue;
			return node;
		}
		case Lexer::TokenType::FUNC:
		{
			if (context.currentFunctionId != context.mainFunctionId)
			{
				throw std::runtime_error("Cannot declare function in function");
			}
			auto node = loadFunc(in_data, i);
			return nullptr;
		}
		case Lexer::TokenType::CONSTRUCTOR:
		{
			if (isInFunction)
			{
				throw std::runtime_error("Cannot declare constructor in function");
			}
			loadConstructor(in_data, i);
			return nullptr;
		}
		case Lexer::TokenType::CLASS:
		{
			if (context.currentClassId)
			{
				throw std::runtime_error("Cannot declare class in class");
			}
			auto node = loadClass(in_data, i);
			return nullptr;
		}
		case Lexer::TokenType::RETURN:
		{
			if (!isInFunction)
				throw std::runtime_error("Cannot call return outside function");
			return loadReturn(in_data, i);
		}
		case Lexer::TokenType::PUBLIC:
		case Lexer::TokenType::PRIVATE:
		case Lexer::TokenType::PROTECTED:
		{
			if (isInFunction)
				throw std::runtime_error("Cannot call keyword '" + token->toString(context) + "'' in function, call it in class ");
			// PUSH BACK IN NEXT
		}
		case Lexer::TokenType::STATIC:
		{
			context.keywords.emplace_back(token->type);
			if (!nextToken(&token, context.tokens, i))
			{
				throw std::runtime_error("Unexpeted keyword " + token->toString(context));
			}
			goto initial;
		}
		default:
			throw std::runtime_error("C1 Unexpected token " + token->toString(context));
		}
		++i;
		return nullptr;
	err_call_func:;
		printDebug(context.currentClassId ? compile.classes[*context.currentClassId].name : "None");
		throw std::runtime_error("Cannot call outside function ");
	err_call_class:;
		throw std::runtime_error("Cannot call outside class ");
	}

	void loadBody(in_func, std::vector<ExprNode *> &nodes, size_t &i, bool createScope)
	{
		Lexer::Token *token = &context.tokens[i];
		if (createScope)
			context.getCurrentFunctionInfo(in_data)->scopes.emplace_back();
		if (token->type != Lexer::TokenType::LBRACE)
		{
			nodes.push_back(loadLine(in_data, i));
			if (createScope)
				context.getCurrentFunctionInfo(in_data)->popBackScope();
			return;
		}
		while (nextToken(&token, context.tokens, i))
		{
			switch (token->type)
			{
			case Lexer::TokenType::RBRACE:
			{
				if (createScope)
					context.getCurrentFunctionInfo(in_data)->popBackScope();
				return;
			}
			default:
				break;
			}
			auto node = loadLine(in_data, i);
			if (node == nullptr)
				continue;
			nodes.push_back(node);
		}
		throw std::runtime_error("Expected }");
	}

	HasClassIdNode *loadExpression(in_func, int minPrecedence, size_t &i)
	{
		std::unique_ptr<HasClassIdNode> left(parsePrimary(in_data, i));
		Lexer::Token *token;
		while (nextToken(&token, context.tokens, i))
		{
			switch (token->type)
			{
			case Lexer::TokenType::COMMA:
			case Lexer::TokenType::RPAREN:
			case Lexer::TokenType::RBRACKET:
			case Lexer::TokenType::RBRACE:
			{
				--i;
				return left.release();
			}
			default:
				break;
			};
			int precedence = getPrecedence(token->type);
			if (precedence == -1 || precedence < minPrecedence)
				break;
			Lexer::TokenType op = token->type;
			if (!nextToken(&token, context.tokens, i))
			{
				throw std::runtime_error("Expected name but not found");
			}
			HasClassIdNode *right = loadExpression(in_data, precedence + 1, i);
			// auto binaryNode = context.binaryNodePool.push(op, left, right);
			auto binaryNode = std::make_unique<BinaryNode>(op, left.release(), right);
			if (minPrecedence == 0)
			{
				auto node = binaryNode->calculate(in_data);
				if (node != nullptr)
				{
					// binaryNode->left = nullptr;
					// binaryNode->right = nullptr;
					left.reset(node);
					continue;
				}
			}
			left.reset(binaryNode.release());
			// std::cout<<"op "<<binaryNode<<":"<<Lexer::Token(0, binaryNode->op, "").toString()<<'\n';
		}
		--i;
		return left.release();
	}

	std::vector<HasClassIdNode *> loadListArgument(in_func, size_t &i)
	{
		Lexer::Token *token = &context.tokens[i];
		char openBracket = getOpenBracket(token->type);
		if (openBracket == '\0')
			throw std::runtime_error("C2Unexpected token " + token->toString(context));
		nextToken(&token, context.tokens, i);
		token = &context.tokens[i];
		std::vector<HasClassIdNode *> nodes;
		switch (token->type)
		{
		case Lexer::TokenType::RPAREN:
		case Lexer::TokenType::RBRACKET:
		case Lexer::TokenType::RBRACE:
		{
			if (!isCloseBracket(openBracket, token->type)) {
				for (auto* node : nodes) delete node;
				throw std::runtime_error(std::string("Expected token '") + getCloseBracket(openBracket));
			}
			return nodes;
		}
		default:
			nodes.push_back(loadExpression(in_data, 0, i));
			// std::cout<<"A "<<i<<'\n';
			break;
		}
		while (nextToken(&token, context.tokens, i))
		{
			switch (token->type)
			{
				using namespace Lexer;
			case Lexer::TokenType::RPAREN:
			case Lexer::TokenType::RBRACKET:
			case Lexer::TokenType::RBRACE:
			{
				if (!isCloseBracket(openBracket, token->type)) {
					for (auto* node : nodes) delete node;
					throw std::runtime_error(std::string("Expected token '") + getCloseBracket(openBracket));
				}
				return nodes;
			}
			case TokenType::COMMA:
			{
				if (!nextToken(&token, context.tokens, i))
					goto expectedCloseBracket;
				nodes.push_back(loadExpression(in_data, 0, i));
				break;
			}
			default:
				goto expectedCloseBracket;
			}
		}
		expectedCloseBracket:
		for (auto* node : nodes) delete node;
		throw std::runtime_error(std::string("Expected token '") + getCloseBracket(openBracket) + "'");
	}

	std::vector<DeclarationNode *> loadListDeclaration(in_func, size_t &i, bool allowVar)
	{
		Lexer::Token *token = &context.tokens[i];
		std::vector<DeclarationNode *> nodes;
		if (!nextToken(&token, context.tokens, i))
			goto expectedCloseBracket;
		switch (token->type)
		{
		case Lexer::TokenType::RPAREN:
		{
			return nodes;
		}
		case Lexer::TokenType::VAR:
		case Lexer::TokenType::VAL:
		{
			if (!allowVar)
				throw std::runtime_error(token->toString(context) + " isn't allowed here");
		}
		case Lexer::TokenType::IDENTIFIER:
		{
			--i;
			break;
		}
		default:
			throw std::runtime_error("Expected name but not found");
		}
		while (nextToken(&token, context.tokens, i))
		{
			bool isVal = true;
			if (allowVar)
			{
				if (!expect(token, Lexer::TokenType::VAR) &&
					!expect(token, Lexer::TokenType::VAL))
				{
					throw std::runtime_error("Expected var or val but not found");
				}
				isVal = token->type == Lexer::TokenType::VAL;
				if (!nextToken(&token, context.tokens, i))
					throw std::runtime_error("Expected name but not found");
			}
			if (!expect(token, Lexer::TokenType::IDENTIFIER))
			{
				throw std::runtime_error("Expected identifier but not found");
			}
			std::string &name = context.lexerString[token->indexData];
			if (!nextToken(&token, context.tokens, i) ||
				!expect(token, Lexer::TokenType::COLON))
			{
				throw std::runtime_error("Expected ':' but not found");
			}
			auto classDeclaration = loadClassDeclaration(in_data, i, token->line);
			if (!nextToken(&token, context.tokens, i))
				break;
			auto node = context.makeDeclarationNode(in_data, false, name, std::move(classDeclaration.className), isVal, false, classDeclaration.nullable, false);
			nodes.push_back(node);
			switch (token->type)
			{
				using namespace Lexer;
			case Lexer::TokenType::RPAREN:
			{
				return nodes;
			}
			case TokenType::COMMA:
			{
				break;
			}
			default:
				goto expectedCloseBracket;
			}
		}
	expectedCloseBracket:
		throw std::runtime_error("Expected token ')' but not found");
	}

	HasClassIdNode *parsePrimary(in_func, size_t &i)
	{
		Lexer::Token *token = &context.tokens[i];
		std::unique_ptr<HasClassIdNode> node;
		switch (token->type)
		{
		case Lexer::TokenType::IDENTIFIER:
			node.reset(loadIdentifier(in_data, i));
			break;
		case Lexer::TokenType::PLUS:
		{
			if (!nextToken(&token, context.tokens, i))
				throw std::runtime_error(std::string("Expected value after '+'"));
			return parsePrimary(in_data, i);
		}
		case Lexer::TokenType::NOT:
		case Lexer::TokenType::MINUS:
		{
			auto op = token->type;
			if (!nextToken(&token, context.tokens, i))
				throw std::runtime_error(std::string("Expected value after ") + Lexer::Token(0, op).toString(context));
			auto unaryNode = new UnaryNode(op, parsePrimary(in_data, i));
			auto calculated = unaryNode->calculate(in_data);
			if (calculated == nullptr)
				return unaryNode;
			unaryNode->value = nullptr;
			ExprNode::deleteNode(unaryNode);
			return calculated;
		}
		case Lexer::TokenType::NUMBER:
		{
			node.reset(loadNumber(in_data, i));
			break;
		}
		case Lexer::TokenType::STRING:
		{
			node.reset(new ConstValueNode(context.lexerString[token->indexData]));
			break;
		}
		case Lexer::TokenType::LPAREN:
		case Lexer::TokenType::LBRACKET:
		case Lexer::TokenType::LBRACE:
		{
			auto list = loadListArgument(in_data, i);
			if (list.size() != 1)
			{
				for (auto *i : list)
					ExprNode::deleteNode(i);
				throw std::runtime_error("Expected value");
			}
			node.reset(list[0]);
			break;
		}
		default:
			throw std::runtime_error("Expected value");
		}
		while (true)
		{
			if (!nextToken(&token, context.tokens, i))
				return node.release();
			switch (token->type)
			{
			case Lexer::TokenType::DOT:
			{
				if (!nextToken(&token, context.tokens, i) ||
					!expect(token, Lexer::TokenType::IDENTIFIER))
				{
					throw std::runtime_error("Expected identifier after '.' but not found");
				}
				auto temp = loadIdentifier(in_data, i, false);
				// switch (temp->kind) {
				// 	case NodeType::VAR:
				// 		printDebug("VAR");
				// 		break;
				// 	case NodeType::UNKNOW:
				// 		printDebug("UNKNOW");
				// 		break;
				// 	case NodeType::GET_PROP:
				// 		printDebug("GET_PROP");
				// 		printDebug(static_cast<GetPropNode*>(temp)->name);
				// 		printDebug(static_cast<GetPropNode*>(temp)->caller ? static_cast<VarNode*>(static_cast<GetPropNode*>(temp)->caller)->declaration->name : "No caller");
				// 		break;
				// 	case NodeType::CONST:
				// 		printDebug("CONST");
				// 		break;
				// 	case NodeType::CALL:
				// 		printDebug("CALL");
				// 		break;
				// }

				if (temp->kind == NodeType::VAR || temp->kind == NodeType::UNKNOW)
				{
					node.reset(new GetPropNode(nullptr, context.currentClassId, node.release(), context.lexerString[token->indexData], false, !nextTokenIfMarkNonNull(in_data, i)));
					ExprNode::deleteNode(temp);
					break;
				}
				static_cast<CallNode *>(temp)->caller = node.release();
				node.reset(temp);
				break;
			}
			case Lexer::TokenType::PLUS_EQUAL:
			case Lexer::TokenType::MINUS_EQUAL:
			case Lexer::TokenType::STAR_EQUAL:
			case Lexer::TokenType::SLASH_EQUAL:
			case Lexer::TokenType::EQUAL:
			{
				Lexer::TokenType op = token->type;
				if (!context.keywords.empty())
					throw std::runtime_error("Invalid keyword");
				if (!nextToken(&token, context.tokens, i))
				{
					throw std::runtime_error("Expected expression after '=' but not found");
				}
				auto value = loadExpression(in_data, 0, i);
				switch (node->kind)
				{
				case NodeType::VAR:
				{
					auto varNode = static_cast<VarNode *>(node.release());
					if (varNode->declaration->isVal)
					{
						ExprNode::deleteNode(value);
						throw std::runtime_error(varNode->declaration->name + " cannot be changed because val");
					}
					return context.setValuePool.push(varNode, value, op);
				}
				case NodeType::UNKNOW:
				{
					return context.setValuePool.push(node.release(), value, op);
				}
				case NodeType::GET_PROP:
				{
					return context.setValuePool.push(node.release(), value, op);
				}
				default:
					break;
				}
				ExprNode::deleteNode(value);
				throw std::runtime_error("Invalid assignment target...");
			}
			default:
				goto ret;
			}
		}
		ret:
		--i;
		return node.release();
	}

	bool nextTokenIfMarkNonNull(in_func, size_t& i) {
		Lexer::Token *token = &context.tokens[i];
		if (nextTokenSameLine(&token, context.tokens, i, token->line) && 
			expect(token, Lexer::TokenType::EXMARK)) {
			return true;
		}
		--i;
		return false;
	}

	HasClassIdNode *loadIdentifier(in_func, size_t &i, bool allowAddThis)
	{
		Lexer::Token *identifier = &context.tokens[i];
		Lexer::Token *token;
		if (!nextToken(&token, context.tokens, i))
		{
			if (!allowAddThis)
			{
				return new UnknowNode(context.currentClassId, context.lexerString[identifier->indexData], true);
			}
			return findIdentifierNode(in_data, context.lexerString[identifier->indexData], true);
		}
		bool nullable = true;
		switch (token->type)
		{
		case Lexer::TokenType::LPAREN:
		{
			auto arguments = loadListArgument(in_data, i);
			return new CallNode(
				context.currentClassId, nullptr, context.lexerString[identifier->indexData] + "()", std::move(arguments), context.justFindStatic, !nextTokenIfMarkNonNull(in_data, i)
			);
		}
		case Lexer::TokenType::LBRACKET:
		{
			auto varNode = findVarNode(in_data, context.lexerString[identifier->indexData], true);
			if (varNode->kind != AutoLang::NodeType::VAR) {
				delete varNode;
				throw std::runtime_error("Invalid assignment target");
			}
			auto arguments = loadListArgument(in_data, i);
			return new CallNode(
				context.currentClassId, static_cast<AccessNode *>(varNode), "[]", arguments, context.justFindStatic, !nextTokenIfMarkNonNull(in_data, i)
			);
		}
		case Lexer::TokenType::LBRACE:
		{
			break;
		}
		case Lexer::TokenType::EXMARK:
		{
			++i;
			nullable = false;
			break;
		}
		default:
			break;
		}
		--i;
		if (!allowAddThis)
		{
			return new UnknowNode(context.currentClassId, context.lexerString[identifier->indexData], nullable);
		}
		return findIdentifierNode(in_data, context.lexerString[identifier->indexData], nullable);
	}

	IfNode *loadIf(in_func, size_t &i)
	{
		if (!context.keywords.empty())
			throw std::runtime_error("Invalid keyword");
		IfNode *node = context.ifPool.push(); // IfPool managed
		Lexer::Token *token;
		if (!nextToken(&token, context.tokens, i) ||
			!expect(token, Lexer::TokenType::LPAREN))
		{
			throw std::runtime_error("Expected ( after if but not found");
		}
		if (!nextToken(&token, context.tokens, i))
			throw std::runtime_error("Expected expression after if but not found");
		node->condition = loadExpression(in_data, 0, i);
		if (!nextToken(&token, context.tokens, i) ||
			!expect(token, Lexer::TokenType::RPAREN))
		{
			throw std::runtime_error("Expected ) but not found");
		}
		if (!nextToken(&token, context.tokens, i))
			throw std::runtime_error("Expected command after if but not found");
		loadBody(in_data, node->ifTrue.nodes, i);
		if (!nextToken(&token, context.tokens, i))
			return node;
		if (!expect(token, Lexer::TokenType::ELSE))
		{
			--i;
			return node;
		}
		if (!nextToken(&token, context.tokens, i))
			throw std::runtime_error("Expected command after else but not found");
		node->ifFalse = new BlockNode();
		loadBody(in_data, node->ifFalse->nodes, i);
		return node;
	}

	HasClassIdNode *findIdentifierNode(in_func, std::string &name, bool nullable)
	{
		auto varNode = findVarNode(in_data, name, nullable);
		return varNode ? varNode : new UnknowNode(context.currentClassId, name, nullable);
	}

	HasClassIdNode *findVarNode(in_func, std::string &name, bool nullable)
	{
		auto constValueNode = findConstValueNode(in_data, name);
		if (constValueNode != nullptr)
			return constValueNode;
		auto node = context.findDeclaration(in_data, name, true);
		if (!node) return nullptr;
		if (static_cast<AccessNode*>(node)->nullable) //#
			static_cast<AccessNode*>(node)->nullable = nullable;
		return node;
	}

	ConstValueNode *findConstValueNode(in_func, std::string &name)
	{
		auto it = context.constValue.find(name);
		if (it == context.constValue.end())
		{
			return nullptr;
		}
		return new ConstValueNode(it->second.first, it->second.second);
	}

	char getOpenBracket(Lexer::TokenType type)
	{
		switch (type)
		{
		case Lexer::TokenType::LPAREN:
			return '(';
		case Lexer::TokenType::LBRACKET:
			return '[';
		case Lexer::TokenType::LBRACE:
			return '{';
		default:
			return '\0';
		}
	}

	bool isCloseBracket(char openBracket, Lexer::TokenType closeBracket)
	{
		switch (openBracket)
		{
		case '(':
			return closeBracket == Lexer::TokenType::RPAREN;
		case '[':
			return closeBracket == Lexer::TokenType::RBRACKET;
		case '{':
			return closeBracket == Lexer::TokenType::RBRACE;
		default:
			return false;
		}
	}

	std::string logLine(Lexer::Token *token)
	{
		return std::string("In line ") + std::to_string(token->line) + " : ";
	}

	int getPrecedence(Lexer::TokenType type)
	{
		switch (type)
		{
		case Lexer::TokenType::PLUS:
		case Lexer::TokenType::MINUS:
		{
			return 10;
		}
		case Lexer::TokenType::STAR:
		case Lexer::TokenType::PERCENT:
		case Lexer::TokenType::SLASH:
		case Lexer::TokenType::AND:
		case Lexer::TokenType::OR:
		{
			return 20;
		}
		case Lexer::TokenType::EQEQ:
		case Lexer::TokenType::NOTEQ:
		case Lexer::TokenType::EQEQEQ:
		case Lexer::TokenType::NOTEQEQ:
		case Lexer::TokenType::LTE:
		case Lexer::TokenType::GTE:
		case Lexer::TokenType::LT:
		case Lexer::TokenType::GT:
		{
			return 7;
		}
		case Lexer::TokenType::AND_AND:
		{
			return 3;
		}
		case Lexer::TokenType::OR_OR:
		{
			return 2;
		}
		default:
			return -1;
		}
	}

	char getCloseBracket(char chr)
	{
		switch (chr)
		{
		case '(':
			return ')';
		case '[':
			return ']';
		case '{':
			return '}';
		case '<':
			return '>';
		}
		return '\0';
	}

	ConstValueNode *loadNumber(in_func, size_t &i)
	{
		Lexer::Token *token = &context.tokens[i];
		uint32_t type = AutoLang::DefaultClass::INTCLASSID;
		std::string &data = context.lexerString[token->indexData];
		const char *s = data.c_str();
		while (*s)
		{
			switch (*s)
			{
			case '.':
			case 'e':
			case 'E':
			{
				type = AutoLang::DefaultClass::FLOATCLASSID;
				goto foundFlag;
			}
			}
			++s;
		}
	foundFlag:
		return type == AutoLang::DefaultClass::INTCLASSID ? new ConstValueNode(static_cast<int64_t>(std::stoll(data))) : new ConstValueNode(static_cast<double>(std::stod(data)));
	}

}

#endif
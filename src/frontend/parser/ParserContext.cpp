#ifndef PARSER_CONTEXT_CPP
#define PARSER_CONTEXT_CPP

#include "frontend/parser/ParserContext.hpp"
#include "frontend/ACompiler.hpp"
#include "frontend/parser/Debugger.hpp"

namespace AutoLang {

LibraryData *ParserContext::mode = nullptr;

void ParserContext::init(CompiledProgram &compile) {
	static ConstValueNode constValues[3] = {
	    ConstValueNode(0, DefaultClass::nullObject, 0),
	    ConstValueNode(0, DefaultClass::trueObject, 1),
	    ConstValueNode(0, DefaultClass::falseObject, 2)};
	gotoFunction(compile.mainFunctionId);
	mainFunctionId = compile.mainFunctionId;

	lexerString.emplace_back("super");
	lexerString.emplace_back("Int");
	lexerString.emplace_back("Float");
	lexerString.emplace_back("Bool");
	lexerString.emplace_back("Null");
	lexerString.emplace_back("Void");
	lexerString.emplace_back("null");
	lexerString.emplace_back("true");
	lexerString.emplace_back("false");
	lexerString.emplace_back("__FILE__");
	lexerString.emplace_back("__LINE__");
	lexerString.emplace_back("__FUNC__");
	lexerString.emplace_back("__CLASS__");

	lexerStringMap["super"] = lexerIdsuper;
	lexerStringMap["Int"] = lexerIdInt;
	lexerStringMap["Float"] = lexerIdFloat;
	lexerStringMap["Bool"] = lexerIdBool;
	lexerStringMap["Null"] = lexerIdNull;
	lexerStringMap["Void"] = lexerIdVoid;
	lexerStringMap["null"] = lexerIdnull;
	lexerStringMap["true"] = lexerIdtrue;
	lexerStringMap["false"] = lexerIdfalse;
	lexerStringMap["__FILE__"] = lexerId__FILE__;
	lexerStringMap["__LINE__"] = lexerId__LINE__;
	lexerStringMap["__FUNC__"] = lexerId__FUNC__;
	lexerStringMap["__CLASS__"] = lexerId__CLASS__;

	constValue[lexerIdnull] = &constValues[0];
	constValue[lexerIdtrue] = &constValues[1];
	constValue[lexerIdfalse] = &constValues[2];

	annotationMetadata.reserve(1);

	{
		using namespace AutoLang::DefaultClass;
		using TT = AutoLang::Lexer::TokenType;
		binaryOpResultType = {
		    // int, float, bool
		    {makeTuple(intClassId, intClassId, (uint8_t)TT::PLUS), intClassId},
		    {makeTuple(intClassId, intClassId, (uint8_t)TT::MINUS), intClassId},
		    {makeTuple(intClassId, intClassId, (uint8_t)TT::STAR), intClassId},
		    {makeTuple(intClassId, intClassId, (uint8_t)TT::SLASH), intClassId},
		    {makeTuple(intClassId, intClassId, (uint8_t)TT::PERCENT),
		     intClassId},
		    {makeTuple(intClassId, intClassId, (uint8_t)TT::AND), intClassId},
		    {makeTuple(intClassId, intClassId, (uint8_t)TT::OR), intClassId},
		    {makeTuple(intClassId, intClassId, (uint8_t)TT::LT), boolClassId},
		    {makeTuple(intClassId, intClassId, (uint8_t)TT::GT), boolClassId},
		    {makeTuple(intClassId, intClassId, (uint8_t)TT::LTE), boolClassId},
		    {makeTuple(intClassId, intClassId, (uint8_t)TT::GTE), boolClassId},
		    {makeTuple(intClassId, intClassId, (uint8_t)TT::EQEQ), boolClassId},
		    {makeTuple(intClassId, intClassId, (uint8_t)TT::NOTEQ),
		     boolClassId},

		    {makeTuple(floatClassId, intClassId, (uint8_t)TT::PLUS),
		     floatClassId},
		    {makeTuple(floatClassId, intClassId, (uint8_t)TT::MINUS),
		     floatClassId},
		    {makeTuple(floatClassId, intClassId, (uint8_t)TT::STAR),
		     floatClassId},
		    {makeTuple(floatClassId, intClassId, (uint8_t)TT::SLASH),
		     floatClassId},
		    {makeTuple(floatClassId, intClassId, (uint8_t)TT::PERCENT),
		     floatClassId},
		    {makeTuple(floatClassId, intClassId, (uint8_t)TT::LT), boolClassId},
		    {makeTuple(floatClassId, intClassId, (uint8_t)TT::GT), boolClassId},
		    {makeTuple(floatClassId, intClassId, (uint8_t)TT::LTE),
		     boolClassId},
		    {makeTuple(floatClassId, intClassId, (uint8_t)TT::GTE),
		     boolClassId},
		    {makeTuple(floatClassId, intClassId, (uint8_t)TT::EQEQ),
		     boolClassId},
		    {makeTuple(floatClassId, intClassId, (uint8_t)TT::NOTEQ),
		     boolClassId},

		    {makeTuple(floatClassId, floatClassId, (uint8_t)TT::PLUS),
		     floatClassId},
		    {makeTuple(floatClassId, floatClassId, (uint8_t)TT::MINUS),
		     floatClassId},
		    {makeTuple(floatClassId, floatClassId, (uint8_t)TT::STAR),
		     floatClassId},
		    {makeTuple(floatClassId, floatClassId, (uint8_t)TT::SLASH),
		     floatClassId},
		    {makeTuple(floatClassId, floatClassId, (uint8_t)TT::PERCENT),
		     floatClassId},
		    {makeTuple(floatClassId, floatClassId, (uint8_t)TT::LT),
		     boolClassId},
		    {makeTuple(floatClassId, floatClassId, (uint8_t)TT::GT),
		     boolClassId},
		    {makeTuple(floatClassId, floatClassId, (uint8_t)TT::LTE),
		     boolClassId},
		    {makeTuple(floatClassId, floatClassId, (uint8_t)TT::GTE),
		     boolClassId},
		    {makeTuple(floatClassId, floatClassId, (uint8_t)TT::EQEQ),
		     boolClassId},
		    {makeTuple(floatClassId, floatClassId, (uint8_t)TT::NOTEQ),
		     boolClassId},

		    {makeTuple(boolClassId, intClassId, (uint8_t)TT::PLUS), intClassId},
		    {makeTuple(boolClassId, intClassId, (uint8_t)TT::MINUS),
		     intClassId},
		    {makeTuple(boolClassId, intClassId, (uint8_t)TT::STAR), intClassId},
		    {makeTuple(boolClassId, intClassId, (uint8_t)TT::SLASH),
		     intClassId},
		    {makeTuple(boolClassId, intClassId, (uint8_t)TT::PERCENT),
		     intClassId},

		    {makeTuple(boolClassId, floatClassId, (uint8_t)TT::PLUS),
		     floatClassId},
		    {makeTuple(boolClassId, floatClassId, (uint8_t)TT::MINUS),
		     floatClassId},
		    {makeTuple(boolClassId, floatClassId, (uint8_t)TT::STAR),
		     floatClassId},
		    {makeTuple(boolClassId, floatClassId, (uint8_t)TT::SLASH),
		     floatClassId},
		    {makeTuple(boolClassId, floatClassId, (uint8_t)TT::PERCENT),
		     floatClassId},

		    // string + int/float
		    {makeTuple(stringClassId, intClassId, (uint8_t)TT::PLUS),
		     stringClassId},
		    {makeTuple(stringClassId, floatClassId, (uint8_t)TT::PLUS),
		     stringClassId},
		    {makeTuple(stringClassId, stringClassId, (uint8_t)TT::PLUS),
		     stringClassId},

		    {makeTuple(stringClassId, stringClassId, (uint8_t)TT::EQEQ),
		     boolClassId},
		    {makeTuple(stringClassId, stringClassId, (uint8_t)TT::NOTEQ),
		     boolClassId},

		    {makeTuple(boolClassId, boolClassId, (uint8_t)TT::PLUS),
		     intClassId},
		    {makeTuple(boolClassId, boolClassId, (uint8_t)TT::MINUS),
		     intClassId},
		    {makeTuple(boolClassId, boolClassId, (uint8_t)TT::STAR),
		     intClassId},
		    {makeTuple(boolClassId, boolClassId, (uint8_t)TT::SLASH),
		     intClassId},
		    {makeTuple(boolClassId, boolClassId, (uint8_t)TT::AND_AND),
		     boolClassId},
		    {makeTuple(boolClassId, boolClassId, (uint8_t)TT::OR_OR),
		     boolClassId},
		    {makeTuple(boolClassId, boolClassId, (uint8_t)TT::EQEQ),
		     boolClassId},
		    {makeTuple(boolClassId, boolClassId, (uint8_t)TT::NOTEQ),
		     boolClassId},

		};
	}

	operatorTable = {{Lexer::TokenType::PLUS, OP_PLUS},
	                 {Lexer::TokenType::MINUS, OP_MINUS},
	                 {Lexer::TokenType::STAR, OP_MUL},
	                 {Lexer::TokenType::SLASH, OP_DIV},
	                 {Lexer::TokenType::PERCENT, OP_MOD},

	                 {Lexer::TokenType::AND, OP_BIT_AND},
	                 {Lexer::TokenType::OR, OP_BIT_OR},

	                 {Lexer::TokenType::AND_AND, OP_AND_AND},
	                 {Lexer::TokenType::OR_OR, OP_OR_OR},

	                 {Lexer::TokenType::NOTEQEQ, OP_NOT_EQ_POINTER},
	                 {Lexer::TokenType::EQEQEQ, OP_EQ_POINTER},

	                 {Lexer::TokenType::NOTEQ, OP_NOT_EQ},
	                 {Lexer::TokenType::EQEQ, OP_EQEQ},

	                 {Lexer::TokenType::LT, OP_LESS},
	                 {Lexer::TokenType::GT, OP_GREATER},
	                 {Lexer::TokenType::GTE, OP_GREATER_EQ},
	                 {Lexer::TokenType::LTE, OP_LESS_EQ}};
}

void ParserContext::refresh(CompiledProgram &compile) {
	gotoFunction(compile.mainFunctionId);
	mainFunctionId = compile.mainFunctionId;

	loadingLibs.clear();
	hasError = false;
	tokens.clear();
	importMap.clear();

	for (auto &funcInfo : functionInfo) {
		funcInfo->block.refresh();
	}

	for (auto *node : staticNode) {
		ExprNode::deleteNode(node);
	}
	staticNode.clear();
	allClassDeclarations.clear();

	mustInferenceFunctionType.clear();

	newFunctions.refresh();
	newClasses.refresh();
	newDefaultClassesMap.clear();
	newGenericClassesMap.clear();

	hasError = false;
	canBreakContinue = false;
	justFindStatic = false;

	continuePos = 0;
	breakPos = 0;
	jumpIfNullNode = nullptr;

	functionInfoAllocator.destroy();
	classInfoAllocator.destroy();
	functionInfo.clear();
	for (auto *funcInfo : functionInfo) {
		delete[] funcInfo->nullableArgs;
	}
	classInfo.clear();

	createConstructorPool.destroy();
	ifPool.destroy();
	whilePool.destroy();
	tryCatchPool.destroy();

	declarationNodePool.refresh();
	returnPool.destroy();
	setValuePool.destroy();

	throwPool.destroy();

	returnPool.destroy();
	setValuePool.destroy();
	binaryNodePool.destroy();
	castPool.destroy();
	runtimeCastPool.destroy();
	varPool.destroy();
	getPropPool.destroy();
	unknowNodePool.destroy();
	optionalAccessNodePool.destroy();
	nullCoalescingPool.destroy();
	blockNodePool.destroy();
	callNodePool.destroy();
	classAccessPool.destroy();
	constValuePool.destroy();
	unaryNodePool.destroy();
	forPool.destroy();
	classDeclarationAllocator.destroy();

	currentClassId = std::nullopt;

	constIntMap.clear();
	constFloatMap.clear();
	for (auto [str, _] : constStringMap) {
		delete str;
	}
	constStringMap.clear();
}

void ParserContext::logMessage(uint32_t line, const std::string &message) {
	std::cerr << mode->path << ":" << line << ": " << message << std::endl;
}

void ParserContext::warning(uint32_t line, const std::string &message) {
	std::cerr << mode->path << ":" << line << ": Warning " << message
	          << std::endl;
}

HasClassIdNode *ParserContext::findDeclaration(in_func, uint32_t line,
                                               std::string &name,
                                               bool inGlobal) {
	AccessNode *node = getCurrentFunctionInfo(in_data)->findDeclaration(
	    in_data, line, name, justFindStatic);
	auto func = getCurrentFunction(in_data);
	if (node)
		return node;
	if (currentClassId) {
		node = getCurrentClassInfo(in_data)->findDeclaration(
		    in_data, line, name, justFindStatic);
		// Static in function is VarNode, NonStatic is GetPropNode
		if ((func->functionFlags & FunctionFlags::FUNC_IS_STATIC) && node &&
		    node->kind == NodeType::GET_PROP)
			goto isNotStatic;
		if (node)
			return node;
	}
	if (!inGlobal || currentFunctionId == mainFunctionId)
		return node;
	node = getMainFunctionInfo(in_data)->findDeclaration(in_data, line, name);
	if (node == nullptr)
		return node;
	if (justFindStatic)
		goto isNotStatic;
	return node;
isNotStatic:;
	throw ParserError(line, name + " is not static");
}

DeclarationNode *ParserContext::makeDeclarationNode(
    in_func, uint32_t line, bool isTemp, std::string name,
    ClassDeclaration *classDeclaration, bool isVal, bool isGlobal,
    bool nullable, bool pushToScope) {
	auto func =
	    isGlobal ? getMainFunction(in_data) : getCurrentFunction(in_data);
	auto funcInfo = isGlobal ? getMainFunctionInfo(in_data)
	                         : getCurrentFunctionInfo(in_data);
	if (pushToScope) {
		auto it = funcInfo->scopes.back().find(name);
		if (it != funcInfo->scopes.back().end())
			throw ParserError(line, name + " has exist");
	}
	DeclarationNode *node =
	    declarationNodePool.push(line, context.currentClassId, std::move(name),
	                             classDeclaration, isVal, isGlobal, nullable);
	node->classId = AutoLang::DefaultClass::nullClassId;
	node->id = pushToScope ? funcInfo->declaration++ : 0;
	if (context.currentClassId) {
		auto classInfo = context.classInfo[*context.currentClassId];
		classInfo->allDeclarationNode.push_back(node);
	}
	// funcInfo->declarationNodes.push_back(node);
	if (pushToScope) {
		size_t newSize = funcInfo->declaration;
		if (newSize > func->maxDeclaration)
			func->maxDeclaration = newSize;
		funcInfo->scopes.back()[node->name] = node;
	}
	return node;
}

ParserContext::~ParserContext() {
	// for (auto* node : staticNode) { //Deleted
	// 	ExprNode::deleteNode(node);
	// }
}

} // namespace AutoLang

#endif
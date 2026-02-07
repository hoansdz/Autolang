#ifndef PARSER_CONTEXT_CPP
#define PARSER_CONTEXT_CPP

#include "frontend/parser/ParserContext.hpp"
#include "frontend/parser/Debugger.hpp"

namespace AutoLang {

AVMReadFileMode *ParserContext::mode = nullptr;

void ParserContext::init(CompiledProgram &compile) {
	gotoFunction(compile.mainFunctionId);
	mainFunctionId = compile.mainFunctionId;

	constValue["null"] = std::pair(DefaultClass::nullObject, 0);
	constValue["true"] = std::pair(DefaultClass::trueObject, 1);
	constValue["false"] = std::pair(DefaultClass::falseObject, 2);

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
}

void ParserContext::refresh(CompiledProgram &compile) {
	gotoFunction(compile.mainFunctionId);
	mainFunctionId = compile.mainFunctionId;

	hasError = false;
	tokens.clear();
	sources.clear();

	for (auto &[_, funcInfo] : functionInfo) {
		funcInfo.block.refresh();
	}

	for (auto node : staticNode) {
		ExprNode::deleteNode(node);
	}

	newFunctions.refresh();
	newClasses.refresh();
	newClassesMap.clear();

	hasError = false;
	canBreakContinue = false;
	justFindStatic = false;

	continuePos = 0;
	breakPos = 0;
	jumpIfNullNode = nullptr;

	functionInfo.clear();
	classInfo.clear();

	createConstructorPool.refresh();
	ifPool.refresh();
	whilePool.refresh();
	tryCatchPool.refresh();

	declarationNodePool.refresh();
	returnPool.refresh();
	setValuePool.refresh();

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

DeclarationNode *
ParserContext::makeDeclarationNode(in_func, uint32_t line, bool isTemp,
                                   std::string name, std::string className,
                                   bool isVal, bool isGlobal, bool nullable,
                                   bool pushToScope) {
	auto func =
	    isGlobal ? getMainFunction(in_data) : getCurrentFunction(in_data);
	auto funcInfo = isGlobal ? getMainFunctionInfo(in_data)
	                         : getCurrentFunctionInfo(in_data);
	if (pushToScope) {
		auto it = funcInfo->scopes.back().find(name);
		if (it != funcInfo->scopes.back().end())
			throw ParserError(line, name + " has exist");
	}
	DeclarationNode *node = declarationNodePool.push(
	    line, std::move(name), std::move(className), isVal, isGlobal, nullable);
	node->classId = AutoLang::DefaultClass::nullClassId;
	node->id = pushToScope ? funcInfo->declaration++ : 0;
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
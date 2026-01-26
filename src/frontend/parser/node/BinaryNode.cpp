#ifndef BINARY_NODE_CPP
#define BINARY_NODE_CPP

#include "Node.hpp"
#include "frontend/parser/ParserContext.hpp"

namespace AutoLang {

#define optimizeNode(token, func)                                              \
	case Lexer::TokenType::token:                                              \
		return func(l, r);

ConstValueNode *BinaryNode::calculate(in_func) {
	// std::cout<<"op "<<this<<":"<<Lexer::Token(0, op,
	// "").toString(context)<<'\n';
	ConstValueNode *l;
	switch (left->kind) {
	case NodeType::CONST:
		l = static_cast<ConstValueNode *>(left);
		break;
	case NodeType::BINARY: {
		auto binaryNode = static_cast<BinaryNode *>(left);
		l = binaryNode->calculate(in_data);
		if (l == nullptr)
			return nullptr;
		// binaryNode->left = nullptr;
		// binaryNode->right = nullptr;
		ExprNode::deleteNode(binaryNode);
		left = l;
		break;
	}
	default:
		return nullptr;
	}
	ConstValueNode *r;
	switch (right->kind) {
	case NodeType::CONST:
		r = static_cast<ConstValueNode *>(right);
		break;
	case NodeType::BINARY: {
		auto binaryNode = static_cast<BinaryNode *>(right);
		r = binaryNode->calculate(in_data);
		if (r == nullptr)
			return nullptr;
		// binaryNode->left = nullptr;
		// binaryNode->right = nullptr;
		ExprNode::deleteNode(binaryNode);
		right = r;
		break;
	}
	default:
		return nullptr;
	}
	try {
		switch (op) {
			using namespace AutoLang;
		optimizeNode(PLUS, plus) optimizeNode(MINUS, minus) optimizeNode(
		    STAR, mul) optimizeNode(SLASH, divide) optimizeNode(PERCENT, mod)
		    optimizeNode(AND, bitwise_and) optimizeNode(OR, bitwise_or)
		        optimizeNode(EQEQ, op_eqeq) optimizeNode(NOTEQ, op_not_eq)
		            optimizeNode(LTE, op_less_than_eq) optimizeNode(
		                GTE, op_greater_than_eq) optimizeNode(LT, op_less_than)
		                optimizeNode(
		                    GT, op_greater_than) case Lexer::TokenType::NOTEQEQ:
		case Lexer::TokenType::EQEQEQ: {
			const bool result = op == Lexer::TokenType::EQEQEQ;
			if (l->classId == AutoLang::DefaultClass::boolClassId) {
				if (r->classId == AutoLang::DefaultClass::boolClassId) {
					const bool equal = (l->obj->b == r->obj->b);
					return new ConstValueNode(line, result ? equal : !equal);
				} else if (r->classId == AutoLang::DefaultClass::nullClassId)
					return new ConstValueNode(line, !result);
			} else if (l->classId == AutoLang::DefaultClass::nullClassId) {
				if (r->classId == AutoLang::DefaultClass::nullClassId) {
					return new ConstValueNode(line, result);
				} else if (r->classId == AutoLang::DefaultClass::boolClassId)
					return new ConstValueNode(line, !result);
			}
			throwError("What happen");
		}
		case Lexer::TokenType::AND_AND:
			return new ConstValueNode(line, l->obj->b && r->obj->b);
		case Lexer::TokenType::OR_OR:
			return new ConstValueNode(line, l->obj->b || r->obj->b);
			default : throwError("What happen");
		}
	} catch (const std::runtime_error &err) {
		throwError("Cannot use " + Lexer::Token(0, op).toString(context) +
		           " operator with " + compile.classes[l->classId].name +
		           " and " + compile.classes[r->classId].name);
	}
}

void BinaryNode::optimize(in_func) {
	left->optimize(in_data);
	right->optimize(in_data);
	if (left->kind == NodeType::CONST)
		static_cast<ConstValueNode *>(left)->isLoadPrimary = true;
	if (right->kind == NodeType::CONST)
		static_cast<ConstValueNode *>(right)->isLoadPrimary = true;
	switch (op) {
	case Lexer::TokenType::PLUS:
	case Lexer::TokenType::MINUS:
	case Lexer::TokenType::STAR:
	case Lexer::TokenType::SLASH: {
		if (left->classId == AutoLang::DefaultClass::boolClassId) {
			left = CastNode::createAndOptimize(
			    in_data, left, AutoLang::DefaultClass::intClassId);
		}
		// std::cout<<compile.classes[left->classId].name<<'\n';

		if (right->classId == AutoLang::DefaultClass::boolClassId) {
			right = CastNode::createAndOptimize(
			    in_data, right, AutoLang::DefaultClass::intClassId);
		}
		if (left->isNullable() || right->isNullable())
			throwError("Cannot use operator '" +
			           Lexer::Token(0, op).toString(context) +
			           "' with nullable value");
		break;
	}
	case Lexer::TokenType::EQEQ: {
		if (left->classId == AutoLang::DefaultClass::nullClassId ||
		    right->classId == AutoLang::DefaultClass::nullClassId) {
			op = Lexer::TokenType::EQEQEQ;
			classId = AutoLang::DefaultClass::boolClassId;
			return;
		}
		break;
	}
	case Lexer::TokenType::NOTEQ: {
		if (left->classId == AutoLang::DefaultClass::nullClassId ||
		    right->classId == AutoLang::DefaultClass::nullClassId) {
			op = Lexer::TokenType::NOTEQEQ;
			classId = AutoLang::DefaultClass::boolClassId;
			return;
		}
		break;
	}
	case Lexer::TokenType::NOTEQEQ:
	case Lexer::TokenType::EQEQEQ: {
		return;
	}
	default:
		if (left->isNullable() || right->isNullable())
			throwError("Cannot use operator '" +
			           Lexer::Token(0, op).toString(context) +
			           "' with nullable value");
		break;
	}
	if (context.getTypeResult(left->classId, right->classId,
	                          static_cast<uint8_t>(op), classId))
		return;
	throwError(std::string("Cannot use '") +
	           Lexer::Token(0, op).toString(context) + "' between " +
	           compile.classes[left->classId].name + " and " +
	           compile.classes[right->classId].name);
}

void BinaryNode::putBytecodes(in_func, std::vector<uint8_t> &bytecodes) {
	if (left->classId == DefaultClass::nullClassId ||
	    right->classId == DefaultClass::nullClassId) {
		if (left->classId != DefaultClass::nullClassId) {
			left->putBytecodes(in_data, bytecodes);
		} else {
			right->putBytecodes(in_data, bytecodes);
		}
		switch (op) {
		case Lexer::TokenType::EQEQEQ:
			bytecodes.emplace_back(AutoLang::Opcode::IS_NULL);
			return;
		case Lexer::TokenType::NOTEQEQ:
			bytecodes.emplace_back(AutoLang::Opcode::IS_NON_NULL);
			return;
		default:
			throwError("Wrong, this can't happen");
		}
		return;
	}
	left->putBytecodes(in_data, bytecodes);
	right->putBytecodes(in_data, bytecodes);
	switch (op) {
	case Lexer::TokenType::PLUS:
		bytecodes.emplace_back(AutoLang::Opcode::PLUS);
		return;
	case Lexer::TokenType::MINUS:
		bytecodes.emplace_back(AutoLang::Opcode::MINUS);
		return;
	case Lexer::TokenType::STAR:
		bytecodes.emplace_back(AutoLang::Opcode::MUL);
		return;
	case Lexer::TokenType::SLASH:
		bytecodes.emplace_back(AutoLang::Opcode::DIVIDE);
		return;
	case Lexer::TokenType::PERCENT:
		bytecodes.emplace_back(AutoLang::Opcode::MOD);
		return;
	case Lexer::TokenType::AND:
		bytecodes.emplace_back(AutoLang::Opcode::BITWISE_AND);
		return;
	case Lexer::TokenType::OR:
		bytecodes.emplace_back(AutoLang::Opcode::BITWISE_OR);
		return;
	case Lexer::TokenType::AND_AND:
		bytecodes.emplace_back(AutoLang::Opcode::AND_AND);
		return;
	case Lexer::TokenType::OR_OR:
		bytecodes.emplace_back(AutoLang::Opcode::OR_OR);
		return;
	case Lexer::TokenType::NOTEQEQ:
		bytecodes.emplace_back(AutoLang::Opcode::NOTEQ_POINTER);
		return;
	case Lexer::TokenType::EQEQEQ:
		bytecodes.emplace_back(AutoLang::Opcode::EQUAL_POINTER);
		return;
	case Lexer::TokenType::NOTEQ:
		bytecodes.emplace_back(AutoLang::Opcode::NOTEQ_VALUE);
		return;
	case Lexer::TokenType::EQEQ:
		bytecodes.emplace_back(AutoLang::Opcode::EQUAL_VALUE);
		return;
	case Lexer::TokenType::LT:
		bytecodes.emplace_back(AutoLang::Opcode::LESS_THAN);
		return;
	case Lexer::TokenType::GT:
		bytecodes.emplace_back(AutoLang::Opcode::GREATER_THAN);
		return;
	case Lexer::TokenType::GTE:
		bytecodes.emplace_back(AutoLang::Opcode::GREATER_THAN_EQ);
		return;
	case Lexer::TokenType::LTE:
		bytecodes.emplace_back(AutoLang::Opcode::LESS_THAN_EQ);
		return;
	default:
		// std::cout<<this<<'\n';
		throwError(std::string("Cannot find operator '") +
		           Lexer::Token(0, op).toString(context) + "'");
	}
}

} // namespace AutoLang

#endif
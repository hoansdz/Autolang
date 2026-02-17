#ifndef OPTIMIZE_NODE_CPP
#define OPTIMIZE_NODE_CPP

#include <charconv>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

#include "frontend/parser/Debugger.hpp"
#include "frontend/parser/node/Node.hpp"
#include "frontend/parser/node/OptimizeNode.hpp"
#include "frontend/parser/ParserContext.hpp"
#include "shared/CompiledProgram.hpp"
#include "shared/AObject.hpp"

namespace AutoLang {
void toInt(in_func, ConstValueNode *value);
void toFloat(in_func, ConstValueNode *value);
void toBool(in_func, ConstValueNode *value);
void toString(in_func, ConstValueNode *value);

static void prepareOperands(in_func, ConstValueNode *left, ConstValueNode *right) {
	if (left->classId == AutoLang::DefaultClass::boolClassId) {
		toInt(in_data, left);
	}
	if (right->classId == AutoLang::DefaultClass::boolClassId) {
		toInt(in_data, right);
	}
}

ConstValueNode *plus(in_func, ConstValueNode *left, ConstValueNode *right) {
	prepareOperands(in_data, left, right);

	switch (left->classId) {
		case AutoLang::DefaultClass::intClassId:
			switch (right->classId) {
				case AutoLang::DefaultClass::intClassId:
					return context.constValuePool.push(left->line, left->i + right->i);
				case AutoLang::DefaultClass::floatClassId:
					return context.constValuePool.push(left->line, left->i + right->f);
				case AutoLang::DefaultClass::stringClassId:
					return context.constValuePool.push(
					    left->line,
					    std::to_string(left->i) +
					        *static_cast<std::string *>(right->str));
				default:
					break;
			}
			break;

		case AutoLang::DefaultClass::floatClassId:
			switch (right->classId) {
				case AutoLang::DefaultClass::intClassId:
					return context.constValuePool.push(left->line, left->f + right->i);
				case AutoLang::DefaultClass::floatClassId:
					return context.constValuePool.push(left->line, left->f + right->f);
				case AutoLang::DefaultClass::stringClassId:
					return context.constValuePool.push(
					    left->line,
					    std::to_string(left->f) +
					        *static_cast<std::string *>(right->str));
				default:
					break;
			}
			break;

		case AutoLang::DefaultClass::stringClassId: {
			std::string &strLeft = *static_cast<std::string *>(left->str);
			switch (right->classId) {
				case AutoLang::DefaultClass::intClassId:
					return context.constValuePool.push(
					    left->line, strLeft + std::to_string(right->i));
				case AutoLang::DefaultClass::floatClassId:
					return context.constValuePool.push(
					    left->line, strLeft + std::to_string(right->f));
				case AutoLang::DefaultClass::stringClassId:
					return context.constValuePool.push(
					    left->line,
					    strLeft + *static_cast<std::string *>(right->str));
				default:
					break;
			}
			break;
		}
		default:
			break;
	}
	throw ParserError(left->line, "Invalid types for operator +");
}

ConstValueNode *minus(in_func, ConstValueNode *left, ConstValueNode *right) {
	prepareOperands(in_data, left, right);

	switch (left->classId) {
		case AutoLang::DefaultClass::intClassId:
			switch (right->classId) {
				case AutoLang::DefaultClass::intClassId:
					return context.constValuePool.push(left->line, left->i - right->i);
				case AutoLang::DefaultClass::floatClassId:
					return context.constValuePool.push(left->line, left->i - right->f);
				default:
					break;
			}
			break;
		case AutoLang::DefaultClass::floatClassId:
			switch (right->classId) {
				case AutoLang::DefaultClass::intClassId:
					return context.constValuePool.push(left->line, left->f - right->i);
				case AutoLang::DefaultClass::floatClassId:
					return context.constValuePool.push(left->line, left->f - right->f);
				default:
					break;
			}
			break;
		default:
			break;
	}
	throw ParserError(left->line, "Invalid types for operator -");
}

ConstValueNode *mul(in_func, ConstValueNode *left, ConstValueNode *right) {
	prepareOperands(in_data, left, right);

	switch (left->classId) {
		case AutoLang::DefaultClass::intClassId:
			switch (right->classId) {
				case AutoLang::DefaultClass::intClassId:
					return context.constValuePool.push(left->line, left->i * right->i);
				case AutoLang::DefaultClass::floatClassId:
					return context.constValuePool.push(left->line, left->i * right->f);
				default:
					break;
			}
			break;
		case AutoLang::DefaultClass::floatClassId:
			switch (right->classId) {
				case AutoLang::DefaultClass::intClassId:
					return context.constValuePool.push(left->line, left->f * right->i);
				case AutoLang::DefaultClass::floatClassId:
					return context.constValuePool.push(left->line, left->f * right->f);
				default:
					break;
			}
			break;
		default:
			break;
	}
	throw ParserError(left->line, "Invalid types for operator *");
}

ConstValueNode *divide(in_func, ConstValueNode *left, ConstValueNode *right) {
	prepareOperands(in_data, left, right);

	if (right->classId == AutoLang::DefaultClass::intClassId && right->i == 0) {
		throw ParserError(right->line, "Division by zero");
	}
	if (right->classId == AutoLang::DefaultClass::floatClassId &&
	    right->f == 0.0) {
		throw ParserError(right->line, "Division by zero");
	}

	switch (left->classId) {
		case AutoLang::DefaultClass::intClassId:
			switch (right->classId) {
				case AutoLang::DefaultClass::intClassId:
					return context.constValuePool.push(left->line, left->i / right->i);
				case AutoLang::DefaultClass::floatClassId:
					return context.constValuePool.push(left->line, left->i / right->f);
				default:
					break;
			}
			break;
		case AutoLang::DefaultClass::floatClassId:
			switch (right->classId) {
				case AutoLang::DefaultClass::intClassId:
					return context.constValuePool.push(left->line, left->f / right->i);
				case AutoLang::DefaultClass::floatClassId:
					return context.constValuePool.push(left->line, left->f / right->f);
				default:
					break;
			}
			break;
		default:
			break;
	}
	throw ParserError(left->line, "Invalid types for operator /");
}

ConstValueNode *mod(in_func, ConstValueNode *left, ConstValueNode *right) {
	if (right->classId == AutoLang::DefaultClass::intClassId && right->i == 0) {
		throw ParserError(right->line, "Modulo by zero");
	}
	if (right->classId == AutoLang::DefaultClass::floatClassId &&
	    right->f == 0.0) {
		throw ParserError(right->line, "Modulo by zero");
	}

	switch (left->classId) {
		case AutoLang::DefaultClass::intClassId:
			switch (right->classId) {
				case AutoLang::DefaultClass::intClassId:
					return context.constValuePool.push(left->line, left->i % right->i);
				case AutoLang::DefaultClass::floatClassId:
					return context.constValuePool.push(
					    left->line,
					    static_cast<double>(std::fmod(left->i, right->f)));
				default:
					break;
			}
			break;
		case AutoLang::DefaultClass::floatClassId:
			switch (right->classId) {
				case AutoLang::DefaultClass::intClassId:
					return context.constValuePool.push(
					    left->line,
					    static_cast<double>(std::fmod(left->f, right->i)));
				case AutoLang::DefaultClass::floatClassId:
					return context.constValuePool.push(
					    left->line,
					    static_cast<double>(std::fmod(left->f, right->f)));
				default:
					break;
			}
			break;
		default:
			break;
	}
	throw ParserError(left->line, "Invalid types for operator %");
}

ConstValueNode *bitwise_and(in_func, ConstValueNode *left, ConstValueNode *right) {
	if (left->classId == AutoLang::DefaultClass::intClassId &&
	    right->classId == AutoLang::DefaultClass::intClassId) {
		return context.constValuePool.push(left->line, left->i & right->i);
	}
	throw ParserError(left->line, "Invalid types for operator &");
}

ConstValueNode *bitwise_or(in_func, ConstValueNode *left, ConstValueNode *right) {
	if (left->classId == AutoLang::DefaultClass::intClassId &&
	    right->classId == AutoLang::DefaultClass::intClassId) {
		return context.constValuePool.push(left->line, left->i | right->i);
	}
	throw ParserError(left->line, "Invalid types for operator |");
}

ConstValueNode *op_eqeq(in_func, ConstValueNode *left, ConstValueNode *right) {
	switch (left->classId) {
		case AutoLang::DefaultClass::intClassId:
			switch (right->classId) {
				case AutoLang::DefaultClass::intClassId:
					return context.constValuePool.push(left->line, left->i == right->i);
				case AutoLang::DefaultClass::floatClassId:
					return context.constValuePool.push(left->line, left->i == right->f);
				default:
					break;
			}
			break;
		case AutoLang::DefaultClass::floatClassId:
			switch (right->classId) {
				case AutoLang::DefaultClass::intClassId:
					return context.constValuePool.push(left->line, left->f == right->i);
				case AutoLang::DefaultClass::floatClassId:
					return context.constValuePool.push(left->line, left->f == right->f);
				default:
					break;
			}
			break;
		default:
			break;
	}

	if (left->classId == AutoLang::DefaultClass::boolClassId &&
	    right->classId == AutoLang::DefaultClass::boolClassId) {
		return context.constValuePool.push(left->line, left->obj->b == right->obj->b);
	}
	if (left->classId == AutoLang::DefaultClass::stringClassId &&
	    right->classId == AutoLang::DefaultClass::stringClassId) {
		return context.constValuePool.push(left->line,
		                          *static_cast<std::string *>(left->str) ==
		                              *static_cast<std::string *>(right->str));
	}

	throw ParserError(left->line, "Invalid types for operator ==");
}

ConstValueNode *op_not_eq(in_func, ConstValueNode *left, ConstValueNode *right) {
	switch (left->classId) {
		case AutoLang::DefaultClass::intClassId:
			switch (right->classId) {
				case AutoLang::DefaultClass::intClassId:
					return context.constValuePool.push(left->line, left->i != right->i);
				case AutoLang::DefaultClass::floatClassId:
					return context.constValuePool.push(left->line, left->i != right->f);
				default:
					break;
			}
			break;
		case AutoLang::DefaultClass::floatClassId:
			switch (right->classId) {
				case AutoLang::DefaultClass::intClassId:
					return context.constValuePool.push(left->line, left->f != right->i);
				case AutoLang::DefaultClass::floatClassId:
					return context.constValuePool.push(left->line, left->f != right->f);
				default:
					break;
			}
			break;
		default:
			break;
	}

	if (left->classId == AutoLang::DefaultClass::boolClassId &&
	    right->classId == AutoLang::DefaultClass::boolClassId) {
		return context.constValuePool.push(left->line, left->obj->b != right->obj->b);
	}
	if (left->classId == AutoLang::DefaultClass::stringClassId &&
	    right->classId == AutoLang::DefaultClass::stringClassId) {
		return context.constValuePool.push(left->line,
		                          *static_cast<std::string *>(left->str) !=
		                              *static_cast<std::string *>(right->str));
	}

	throw ParserError(left->line, "Invalid types for operator !=");
}

ConstValueNode *op_greater_than(in_func, ConstValueNode *left, ConstValueNode *right) {
	prepareOperands(in_data, left, right);
	switch (left->classId) {
		case AutoLang::DefaultClass::intClassId:
			switch (right->classId) {
				case AutoLang::DefaultClass::intClassId:
					return context.constValuePool.push(left->line, left->i > right->i);
				case AutoLang::DefaultClass::floatClassId:
					return context.constValuePool.push(left->line, left->i > right->f);
				default:
					break;
			}
			break;
		case AutoLang::DefaultClass::floatClassId:
			switch (right->classId) {
				case AutoLang::DefaultClass::intClassId:
					return context.constValuePool.push(left->line, left->f > right->i);
				case AutoLang::DefaultClass::floatClassId:
					return context.constValuePool.push(left->line, left->f > right->f);
				default:
					break;
			}
			break;
		default:
			break;
	}
	throw ParserError(left->line, "Invalid types for operator >");
}

ConstValueNode *op_less_than(in_func, ConstValueNode *left, ConstValueNode *right) {
	prepareOperands(in_data, left, right);
	switch (left->classId) {
		case AutoLang::DefaultClass::intClassId:
			switch (right->classId) {
				case AutoLang::DefaultClass::intClassId:
					return context.constValuePool.push(left->line, left->i < right->i);
				case AutoLang::DefaultClass::floatClassId:
					return context.constValuePool.push(left->line, left->i < right->f);
				default:
					break;
			}
			break;
		case AutoLang::DefaultClass::floatClassId:
			switch (right->classId) {
				case AutoLang::DefaultClass::intClassId:
					return context.constValuePool.push(left->line, left->f < right->i);
				case AutoLang::DefaultClass::floatClassId:
					return context.constValuePool.push(left->line, left->f < right->f);
				default:
					break;
			}
			break;
		default:
			break;
	}
	throw ParserError(left->line, "Invalid types for operator <");
}

ConstValueNode *op_greater_than_eq(in_func,
                                   ConstValueNode *left,
                                   ConstValueNode *right) {
	prepareOperands(in_data, left, right);
	switch (left->classId) {
		case AutoLang::DefaultClass::intClassId:
			switch (right->classId) {
				case AutoLang::DefaultClass::intClassId:
					return context.constValuePool.push(left->line, left->i >= right->i);
				case AutoLang::DefaultClass::floatClassId:
					return context.constValuePool.push(left->line, left->i >= right->f);
				default:
					break;
			}
			break;
		case AutoLang::DefaultClass::floatClassId:
			switch (right->classId) {
				case AutoLang::DefaultClass::intClassId:
					return context.constValuePool.push(left->line, left->f >= right->i);
				case AutoLang::DefaultClass::floatClassId:
					return context.constValuePool.push(left->line, left->f >= right->f);
				default:
					break;
			}
			break;
		default:
			break;
	}
	throw ParserError(left->line, "Invalid types for operator >=");
}

ConstValueNode *op_less_than_eq(in_func, ConstValueNode *left, ConstValueNode *right) {
	prepareOperands(in_data, left, right);
	switch (left->classId) {
		case AutoLang::DefaultClass::intClassId:
			switch (right->classId) {
				case AutoLang::DefaultClass::intClassId:
					return context.constValuePool.push(left->line, left->i <= right->i);
				case AutoLang::DefaultClass::floatClassId:
					return context.constValuePool.push(left->line, left->i <= right->f);
				default:
					break;
			}
			break;
		case AutoLang::DefaultClass::floatClassId:
			switch (right->classId) {
				case AutoLang::DefaultClass::intClassId:
					return context.constValuePool.push(left->line, left->f <= right->i);
				case AutoLang::DefaultClass::floatClassId:
					return context.constValuePool.push(left->line, left->f <= right->f);
				default:
					break;
			}
			break;
		default:
			break;
	}
	throw ParserError(left->line, "Invalid types for operator <=");
}

void toInt(in_func, ConstValueNode *value) {
	switch (value->classId) {
		case AutoLang::DefaultClass::intClassId:
			return;
		case AutoLang::DefaultClass::floatClassId:
			value->classId = AutoLang::DefaultClass::intClassId;
			value->i = static_cast<int64_t>(value->f);
			return;
		case AutoLang::DefaultClass::boolClassId:
			value->classId = AutoLang::DefaultClass::intClassId;
			value->i = static_cast<int64_t>(value->obj->b);
			return;
		default:
			break;
	}
	throw ParserError(value->line, "Cannot convert to Int");
}

void toFloat(in_func, ConstValueNode *value) {
	switch (value->classId) {
		case AutoLang::DefaultClass::intClassId:
			value->classId = AutoLang::DefaultClass::floatClassId;
			value->f = static_cast<double>(value->i);
			return;
		case AutoLang::DefaultClass::floatClassId:
			return;
		case AutoLang::DefaultClass::boolClassId:
			value->classId = AutoLang::DefaultClass::floatClassId;
			value->f = static_cast<double>(value->obj->b);
			return;
		default:
			break;
	}
	throw ParserError(value->line, "Cannot convert to Float");
}

void toBool(in_func, ConstValueNode *value) {
	switch (value->classId) {
		case AutoLang::DefaultClass::intClassId: {
			value->classId = AutoLang::DefaultClass::boolClassId;
			bool b = value->i != 0;
			value->obj = ObjectManager::createBoolObject(b);
			value->id = ConstValueNode::getBoolId(b);
			return;
		}
		case AutoLang::DefaultClass::floatClassId: {
			value->classId = AutoLang::DefaultClass::boolClassId;
			bool b = value->f != 0;
			value->obj = ObjectManager::createBoolObject(b);
			value->id = ConstValueNode::getBoolId(b);
			return;
		}
		case AutoLang::DefaultClass::boolClassId:
			return;
		default:
			break;
	}
	throw ParserError(value->line, "Cannot convert to Bool");
}

void toString(in_func, ConstValueNode *value) {
	switch (value->classId) {
		case AutoLang::DefaultClass::intClassId:
			value->classId = AutoLang::DefaultClass::stringClassId;
			value->str = new std::string(std::to_string(value->i));
			return;
		case AutoLang::DefaultClass::floatClassId:
			value->classId = AutoLang::DefaultClass::stringClassId;
			value->str = new std::string(std::to_string(value->f));
			return;
		case AutoLang::DefaultClass::boolClassId:
			value->classId = AutoLang::DefaultClass::stringClassId;
			value->str = new std::string(value->obj->b ? "true" : "false");
			return;
		case AutoLang::DefaultClass::stringClassId:
			return;
		default:
			break;
	}
	throw ParserError(value->line, "Cannot convert to String");
}
} // namespace AutoLang

#endif

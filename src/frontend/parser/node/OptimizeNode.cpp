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
#include "shared/AObject.hpp"

namespace AutoLang {
void toInt(ConstValueNode *value);
void toFloat(ConstValueNode *value);
void toBool(ConstValueNode *value);
void toString(ConstValueNode *value);

static void prepareOperands(ConstValueNode *left, ConstValueNode *right) {
	if (left->classId == AutoLang::DefaultClass::boolClassId) {
		toInt(left);
	}
	if (right->classId == AutoLang::DefaultClass::boolClassId) {
		toInt(right);
	}
}

ConstValueNode *plus(ConstValueNode *left, ConstValueNode *right) {
	prepareOperands(left, right);

	switch (left->classId) {
		case AutoLang::DefaultClass::intClassId:
			switch (right->classId) {
				case AutoLang::DefaultClass::intClassId:
					return new ConstValueNode(left->line, left->i + right->i);
				case AutoLang::DefaultClass::floatClassId:
					return new ConstValueNode(left->line, left->i + right->f);
				case AutoLang::DefaultClass::stringClassId:
					return new ConstValueNode(
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
					return new ConstValueNode(left->line, left->f + right->i);
				case AutoLang::DefaultClass::floatClassId:
					return new ConstValueNode(left->line, left->f + right->f);
				case AutoLang::DefaultClass::stringClassId:
					return new ConstValueNode(
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
					return new ConstValueNode(
					    left->line, strLeft + std::to_string(right->i));
				case AutoLang::DefaultClass::floatClassId:
					return new ConstValueNode(
					    left->line, strLeft + std::to_string(right->f));
				case AutoLang::DefaultClass::stringClassId:
					return new ConstValueNode(
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
	throw std::runtime_error("Invalid types for operator +");
}

ConstValueNode *minus(ConstValueNode *left, ConstValueNode *right) {
	prepareOperands(left, right);

	switch (left->classId) {
		case AutoLang::DefaultClass::intClassId:
			switch (right->classId) {
				case AutoLang::DefaultClass::intClassId:
					return new ConstValueNode(left->line, left->i - right->i);
				case AutoLang::DefaultClass::floatClassId:
					return new ConstValueNode(left->line, left->i - right->f);
				default:
					break;
			}
			break;
		case AutoLang::DefaultClass::floatClassId:
			switch (right->classId) {
				case AutoLang::DefaultClass::intClassId:
					return new ConstValueNode(left->line, left->f - right->i);
				case AutoLang::DefaultClass::floatClassId:
					return new ConstValueNode(left->line, left->f - right->f);
				default:
					break;
			}
			break;
		default:
			break;
	}
	throw std::runtime_error("Invalid types for operator -");
}

ConstValueNode *mul(ConstValueNode *left, ConstValueNode *right) {
	prepareOperands(left, right);

	switch (left->classId) {
		case AutoLang::DefaultClass::intClassId:
			switch (right->classId) {
				case AutoLang::DefaultClass::intClassId:
					return new ConstValueNode(left->line, left->i * right->i);
				case AutoLang::DefaultClass::floatClassId:
					return new ConstValueNode(left->line, left->i * right->f);
				default:
					break;
			}
			break;
		case AutoLang::DefaultClass::floatClassId:
			switch (right->classId) {
				case AutoLang::DefaultClass::intClassId:
					return new ConstValueNode(left->line, left->f * right->i);
				case AutoLang::DefaultClass::floatClassId:
					return new ConstValueNode(left->line, left->f * right->f);
				default:
					break;
			}
			break;
		default:
			break;
	}
	throw std::runtime_error("Invalid types for operator *");
}

ConstValueNode *divide(ConstValueNode *left, ConstValueNode *right) {
	prepareOperands(left, right);

	if (right->classId == AutoLang::DefaultClass::intClassId && right->i == 0) {
		throw std::runtime_error("Division by zero");
	}
	if (right->classId == AutoLang::DefaultClass::floatClassId &&
	    right->f == 0.0) {
		throw std::runtime_error("Division by zero");
	}

	switch (left->classId) {
		case AutoLang::DefaultClass::intClassId:
			switch (right->classId) {
				case AutoLang::DefaultClass::intClassId:
					return new ConstValueNode(left->line, left->i / right->i);
				case AutoLang::DefaultClass::floatClassId:
					return new ConstValueNode(left->line, left->i / right->f);
				default:
					break;
			}
			break;
		case AutoLang::DefaultClass::floatClassId:
			switch (right->classId) {
				case AutoLang::DefaultClass::intClassId:
					return new ConstValueNode(left->line, left->f / right->i);
				case AutoLang::DefaultClass::floatClassId:
					return new ConstValueNode(left->line, left->f / right->f);
				default:
					break;
			}
			break;
		default:
			break;
	}
	throw std::runtime_error("Invalid types for operator /");
}

ConstValueNode *mod(ConstValueNode *left, ConstValueNode *right) {
	if (right->classId == AutoLang::DefaultClass::intClassId && right->i == 0) {
		throw std::runtime_error("Modulo by zero");
	}
	if (right->classId == AutoLang::DefaultClass::floatClassId &&
	    right->f == 0.0) {
		throw std::runtime_error("Modulo by zero");
	}

	switch (left->classId) {
		case AutoLang::DefaultClass::intClassId:
			switch (right->classId) {
				case AutoLang::DefaultClass::intClassId:
					return new ConstValueNode(left->line, left->i % right->i);
				case AutoLang::DefaultClass::floatClassId:
					return new ConstValueNode(
					    left->line,
					    static_cast<double>(std::fmod(left->i, right->f)));
				default:
					break;
			}
			break;
		case AutoLang::DefaultClass::floatClassId:
			switch (right->classId) {
				case AutoLang::DefaultClass::intClassId:
					return new ConstValueNode(
					    left->line,
					    static_cast<double>(std::fmod(left->f, right->i)));
				case AutoLang::DefaultClass::floatClassId:
					return new ConstValueNode(
					    left->line,
					    static_cast<double>(std::fmod(left->f, right->f)));
				default:
					break;
			}
			break;
		default:
			break;
	}
	throw std::runtime_error("Invalid types for operator %");
}

ConstValueNode *bitwise_and(ConstValueNode *left, ConstValueNode *right) {
	if (left->classId == AutoLang::DefaultClass::intClassId &&
	    right->classId == AutoLang::DefaultClass::intClassId) {
		return new ConstValueNode(left->line, left->i & right->i);
	}
	throw std::runtime_error("Invalid types for operator &");
}

ConstValueNode *bitwise_or(ConstValueNode *left, ConstValueNode *right) {
	if (left->classId == AutoLang::DefaultClass::intClassId &&
	    right->classId == AutoLang::DefaultClass::intClassId) {
		return new ConstValueNode(left->line, left->i | right->i);
	}
	throw std::runtime_error("Invalid types for operator |");
}

ConstValueNode *op_eqeq(ConstValueNode *left, ConstValueNode *right) {
	switch (left->classId) {
		case AutoLang::DefaultClass::intClassId:
			switch (right->classId) {
				case AutoLang::DefaultClass::intClassId:
					return new ConstValueNode(left->line, left->i == right->i);
				case AutoLang::DefaultClass::floatClassId:
					return new ConstValueNode(left->line, left->i == right->f);
				default:
					break;
			}
			break;
		case AutoLang::DefaultClass::floatClassId:
			switch (right->classId) {
				case AutoLang::DefaultClass::intClassId:
					return new ConstValueNode(left->line, left->f == right->i);
				case AutoLang::DefaultClass::floatClassId:
					return new ConstValueNode(left->line, left->f == right->f);
				default:
					break;
			}
			break;
		default:
			break;
	}

	if (left->classId == AutoLang::DefaultClass::boolClassId &&
	    right->classId == AutoLang::DefaultClass::boolClassId) {
		return new ConstValueNode(left->line, left->obj->b == right->obj->b);
	}
	if (left->classId == AutoLang::DefaultClass::stringClassId &&
	    right->classId == AutoLang::DefaultClass::stringClassId) {
		return new ConstValueNode(left->line,
		                          *static_cast<std::string *>(left->str) ==
		                              *static_cast<std::string *>(right->str));
	}

	throw std::runtime_error("Invalid types for operator ==");
}

ConstValueNode *op_not_eq(ConstValueNode *left, ConstValueNode *right) {
	switch (left->classId) {
		case AutoLang::DefaultClass::intClassId:
			switch (right->classId) {
				case AutoLang::DefaultClass::intClassId:
					return new ConstValueNode(left->line, left->i != right->i);
				case AutoLang::DefaultClass::floatClassId:
					return new ConstValueNode(left->line, left->i != right->f);
				default:
					break;
			}
			break;
		case AutoLang::DefaultClass::floatClassId:
			switch (right->classId) {
				case AutoLang::DefaultClass::intClassId:
					return new ConstValueNode(left->line, left->f != right->i);
				case AutoLang::DefaultClass::floatClassId:
					return new ConstValueNode(left->line, left->f != right->f);
				default:
					break;
			}
			break;
		default:
			break;
	}

	if (left->classId == AutoLang::DefaultClass::boolClassId &&
	    right->classId == AutoLang::DefaultClass::boolClassId) {
		return new ConstValueNode(left->line, left->obj->b != right->obj->b);
	}
	if (left->classId == AutoLang::DefaultClass::stringClassId &&
	    right->classId == AutoLang::DefaultClass::stringClassId) {
		return new ConstValueNode(left->line,
		                          *static_cast<std::string *>(left->str) !=
		                              *static_cast<std::string *>(right->str));
	}

	throw std::runtime_error("Invalid types for operator !=");
}

ConstValueNode *op_greater_than(ConstValueNode *left, ConstValueNode *right) {
	prepareOperands(left, right);
	switch (left->classId) {
		case AutoLang::DefaultClass::intClassId:
			switch (right->classId) {
				case AutoLang::DefaultClass::intClassId:
					return new ConstValueNode(left->line, left->i > right->i);
				case AutoLang::DefaultClass::floatClassId:
					return new ConstValueNode(left->line, left->i > right->f);
				default:
					break;
			}
			break;
		case AutoLang::DefaultClass::floatClassId:
			switch (right->classId) {
				case AutoLang::DefaultClass::intClassId:
					return new ConstValueNode(left->line, left->f > right->i);
				case AutoLang::DefaultClass::floatClassId:
					return new ConstValueNode(left->line, left->f > right->f);
				default:
					break;
			}
			break;
		default:
			break;
	}
	throw std::runtime_error("Invalid types for operator >");
}

ConstValueNode *op_less_than(ConstValueNode *left, ConstValueNode *right) {
	prepareOperands(left, right);
	switch (left->classId) {
		case AutoLang::DefaultClass::intClassId:
			switch (right->classId) {
				case AutoLang::DefaultClass::intClassId:
					return new ConstValueNode(left->line, left->i < right->i);
				case AutoLang::DefaultClass::floatClassId:
					return new ConstValueNode(left->line, left->i < right->f);
				default:
					break;
			}
			break;
		case AutoLang::DefaultClass::floatClassId:
			switch (right->classId) {
				case AutoLang::DefaultClass::intClassId:
					return new ConstValueNode(left->line, left->f < right->i);
				case AutoLang::DefaultClass::floatClassId:
					return new ConstValueNode(left->line, left->f < right->f);
				default:
					break;
			}
			break;
		default:
			break;
	}
	throw std::runtime_error("Invalid types for operator <");
}

ConstValueNode *op_greater_than_eq(ConstValueNode *left,
                                   ConstValueNode *right) {
	prepareOperands(left, right);
	switch (left->classId) {
		case AutoLang::DefaultClass::intClassId:
			switch (right->classId) {
				case AutoLang::DefaultClass::intClassId:
					return new ConstValueNode(left->line, left->i >= right->i);
				case AutoLang::DefaultClass::floatClassId:
					return new ConstValueNode(left->line, left->i >= right->f);
				default:
					break;
			}
			break;
		case AutoLang::DefaultClass::floatClassId:
			switch (right->classId) {
				case AutoLang::DefaultClass::intClassId:
					return new ConstValueNode(left->line, left->f >= right->i);
				case AutoLang::DefaultClass::floatClassId:
					return new ConstValueNode(left->line, left->f >= right->f);
				default:
					break;
			}
			break;
		default:
			break;
	}
	throw std::runtime_error("Invalid types for operator >=");
}

ConstValueNode *op_less_than_eq(ConstValueNode *left, ConstValueNode *right) {
	prepareOperands(left, right);
	switch (left->classId) {
		case AutoLang::DefaultClass::intClassId:
			switch (right->classId) {
				case AutoLang::DefaultClass::intClassId:
					return new ConstValueNode(left->line, left->i <= right->i);
				case AutoLang::DefaultClass::floatClassId:
					return new ConstValueNode(left->line, left->i <= right->f);
				default:
					break;
			}
			break;
		case AutoLang::DefaultClass::floatClassId:
			switch (right->classId) {
				case AutoLang::DefaultClass::intClassId:
					return new ConstValueNode(left->line, left->f <= right->i);
				case AutoLang::DefaultClass::floatClassId:
					return new ConstValueNode(left->line, left->f <= right->f);
				default:
					break;
			}
			break;
		default:
			break;
	}
	throw std::runtime_error("Invalid types for operator <=");
}

void toInt(ConstValueNode *value) {
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
	throw std::runtime_error("Cannot convert to Int");
}

void toFloat(ConstValueNode *value) {
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
	throw std::runtime_error("Cannot convert to Float");
}

void toBool(ConstValueNode *value) {
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
	throw std::runtime_error("Cannot convert to Bool");
}

void toString(ConstValueNode *value) {
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
	throw std::runtime_error("Cannot convert to String");
}
} // namespace AutoLang

#endif
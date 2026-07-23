#ifndef OPTIMIZE_NODE_CPP
#define OPTIMIZE_NODE_CPP

#include <charconv>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

#include "frontend/parser/Debugger.hpp"
#include "frontend/parser/ParserContext.hpp"
#include "frontend/parser/node/Node.hpp"
#include "frontend/parser/node/OptimizeNode.hpp"
#include "shared/AObject.hpp"
#include "shared/ClassFlags.hpp"
#include "shared/CompiledProgram.hpp"

namespace Autolang {
ConstValueNode *toInt(in_func, ConstValueNode *value);
ConstValueNode *toFloat(in_func, ConstValueNode *value);
ConstValueNode *toBool(in_func, ConstValueNode *value);
ConstValueNode *toString(in_func, ConstValueNode *value);

static void prepareOperands(in_func, ConstValueNode *&left,
                            ConstValueNode *&right) {
	if (left->classId == Autolang::DefaultClass::boolClassId) {
		left = toInt(in_data, left);
	}
	if (right->classId == Autolang::DefaultClass::boolClassId) {
		right = toInt(in_data, right);
	}
}

inline void throwInvalidCompare(in_func, ConstValueNode *left,
                                ConstValueNode *right, const char *op) {
	throw ParserError(left->line,
	                  std::string("Cannot apply operator '") + op +
	                      "' between '" + compile.classes[left->classId]->getName(compile) +
	                      "' and '" + compile.classes[right->classId]->getName(compile) +
	                      "'");
}

ConstValueNode *plus(in_func, ConstValueNode *left, ConstValueNode *right) {
	prepareOperands(in_data, left, right);

	switch (left->classId) {
		case Autolang::DefaultClass::intClassId:
			switch (right->classId) {
				case Autolang::DefaultClass::intClassId:
					return context.constValuePool.push(left->line,
					                                   left->i + right->i);
				case Autolang::DefaultClass::floatClassId:
					return context.constValuePool.push(left->line,
					                                   left->i + right->f);
				case Autolang::DefaultClass::stringClassId:
					return context.constValuePool.push(
					    left->line,
					    std::to_string(left->i) +
					        *static_cast<std::string *>(right->str));
				default:
					break;
			}
			break;

		case Autolang::DefaultClass::floatClassId:
			switch (right->classId) {
				case Autolang::DefaultClass::intClassId:
					return context.constValuePool.push(left->line,
					                                   left->f + right->i);
				case Autolang::DefaultClass::floatClassId:
					return context.constValuePool.push(left->line,
					                                   left->f + right->f);
				case Autolang::DefaultClass::stringClassId:
					return context.constValuePool.push(
					    left->line,
					    std::to_string(left->f) +
					        *static_cast<std::string *>(right->str));
				default:
					break;
			}
			break;

		case Autolang::DefaultClass::stringClassId: {
			std::string &strLeft = *static_cast<std::string *>(left->str);
			switch (right->classId) {
				case Autolang::DefaultClass::intClassId:
					return context.constValuePool.push(
					    left->line, strLeft + std::to_string(right->i));
				case Autolang::DefaultClass::floatClassId:
					return context.constValuePool.push(
					    left->line, strLeft + std::to_string(right->f));
				case Autolang::DefaultClass::stringClassId:
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
		case Autolang::DefaultClass::intClassId:
			switch (right->classId) {
				case Autolang::DefaultClass::intClassId:
					return context.constValuePool.push(left->line,
					                                   left->i - right->i);
				case Autolang::DefaultClass::floatClassId:
					return context.constValuePool.push(left->line,
					                                   left->i - right->f);
				default:
					break;
			}
			break;
		case Autolang::DefaultClass::floatClassId:
			switch (right->classId) {
				case Autolang::DefaultClass::intClassId:
					return context.constValuePool.push(left->line,
					                                   left->f - right->i);
				case Autolang::DefaultClass::floatClassId:
					return context.constValuePool.push(left->line,
					                                   left->f - right->f);
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
		case Autolang::DefaultClass::intClassId:
			switch (right->classId) {
				case Autolang::DefaultClass::intClassId:
					return context.constValuePool.push(left->line,
					                                   left->i * right->i);
				case Autolang::DefaultClass::floatClassId:
					return context.constValuePool.push(left->line,
					                                   left->i * right->f);
				default:
					break;
			}
			break;
		case Autolang::DefaultClass::floatClassId:
			switch (right->classId) {
				case Autolang::DefaultClass::intClassId:
					return context.constValuePool.push(left->line,
					                                   left->f * right->i);
				case Autolang::DefaultClass::floatClassId:
					return context.constValuePool.push(left->line,
					                                   left->f * right->f);
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

	if (right->classId == Autolang::DefaultClass::intClassId && right->i == 0) {
		throw ParserError(right->line, "Division by zero");
	}
	if (right->classId == Autolang::DefaultClass::floatClassId &&
	    right->f == 0.0) {
		throw ParserError(right->line, "Division by zero");
	}

	switch (left->classId) {
		case Autolang::DefaultClass::intClassId:
			switch (right->classId) {
				case Autolang::DefaultClass::intClassId:
					return context.constValuePool.push(left->line,
					                                   left->i / right->i);
				case Autolang::DefaultClass::floatClassId:
					return context.constValuePool.push(left->line,
					                                   left->i / right->f);
				default:
					break;
			}
			break;
		case Autolang::DefaultClass::floatClassId:
			switch (right->classId) {
				case Autolang::DefaultClass::intClassId:
					return context.constValuePool.push(left->line,
					                                   left->f / right->i);
				case Autolang::DefaultClass::floatClassId:
					return context.constValuePool.push(left->line,
					                                   left->f / right->f);
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
	if (right->classId == Autolang::DefaultClass::intClassId && right->i == 0) {
		throw ParserError(right->line, "Modulo by zero");
	}
	if (right->classId == Autolang::DefaultClass::floatClassId &&
	    right->f == 0.0) {
		throw ParserError(right->line, "Modulo by zero");
	}

	switch (left->classId) {
		case Autolang::DefaultClass::intClassId:
			switch (right->classId) {
				case Autolang::DefaultClass::intClassId:
					return context.constValuePool.push(left->line,
					                                   left->i % right->i);
				case Autolang::DefaultClass::floatClassId:
					return context.constValuePool.push(
					    left->line,
					    static_cast<double>(std::fmod(left->i, right->f)));
				default:
					break;
			}
			break;
		case Autolang::DefaultClass::floatClassId:
			switch (right->classId) {
				case Autolang::DefaultClass::intClassId:
					return context.constValuePool.push(
					    left->line,
					    static_cast<double>(std::fmod(left->f, right->i)));
				case Autolang::DefaultClass::floatClassId:
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

ConstValueNode *bitwise_and(in_func, ConstValueNode *left,
                            ConstValueNode *right) {
	if (left->classId == Autolang::DefaultClass::intClassId &&
	    right->classId == Autolang::DefaultClass::intClassId) {
		return context.constValuePool.push(left->line, left->i & right->i);
	}
	throw ParserError(left->line, "Invalid types for operator &");
}

ConstValueNode *bitwise_or(in_func, ConstValueNode *left,
                           ConstValueNode *right) {
	if (left->classId == Autolang::DefaultClass::intClassId &&
	    right->classId == Autolang::DefaultClass::intClassId) {
		return context.constValuePool.push(left->line, left->i | right->i);
	}
	throw ParserError(left->line, "Invalid types for operator |");
}

ConstValueNode *op_eqeq(in_func, ConstValueNode *left, ConstValueNode *right) {
	switch (left->classId) {
		case Autolang::DefaultClass::intClassId:
			switch (right->classId) {
				case Autolang::DefaultClass::intClassId:
					return context.constValuePool.push(left->line,
					                                   left->i == right->i);
				case Autolang::DefaultClass::floatClassId:
					return context.constValuePool.push(left->line,
					                                   left->i == right->f);
				default:
					break;
			}
			break;
		case Autolang::DefaultClass::floatClassId:
			switch (right->classId) {
				case Autolang::DefaultClass::intClassId:
					return context.constValuePool.push(left->line,
					                                   left->f == right->i);
				case Autolang::DefaultClass::floatClassId:
					return context.constValuePool.push(left->line,
					                                   left->f == right->f);
				default:
					break;
			}
			break;
		case Autolang::DefaultClass::boolClassId: {
			if (right->classId == Autolang::DefaultClass::boolClassId) {
				return context.constValuePool.push(
				    left->line, left->obj->b == right->obj->b);
			}
			throwInvalidCompare(in_data, left, right, "==");
		}
		case Autolang::DefaultClass::stringClassId: {
			if (right->classId == Autolang::DefaultClass::stringClassId) {
				return context.constValuePool.push(
				    left->line, *static_cast<std::string *>(left->str) ==
				                    *static_cast<std::string *>(right->str));
			}
		}
		default:
			break;
	}

	if (left->classId == right->classId &&
	    compile.classes[left->classId]->classFlags &
	        ClassFlags::CLASS_IS_ENUM) {
		return context.constValuePool.push(left->line, left == right);
	}

	throw ParserError(left->line, "Invalid types for operator == between " +
	                                  compile.classes[left->classId]->getName(compile) +
	                                  " and " +
	                                  compile.classes[right->classId]->getName(compile));
}

ConstValueNode *op_not_eq(in_func, ConstValueNode *left,
                          ConstValueNode *right) {
	switch (left->classId) {
		case Autolang::DefaultClass::intClassId:
			switch (right->classId) {
				case Autolang::DefaultClass::intClassId:
					return context.constValuePool.push(left->line,
					                                   left->i != right->i);
				case Autolang::DefaultClass::floatClassId:
					return context.constValuePool.push(left->line,
					                                   left->i != right->f);
				default:
					break;
			}
			break;
		case Autolang::DefaultClass::floatClassId:
			switch (right->classId) {
				case Autolang::DefaultClass::intClassId:
					return context.constValuePool.push(left->line,
					                                   left->f != right->i);
				case Autolang::DefaultClass::floatClassId:
					return context.constValuePool.push(left->line,
					                                   left->f != right->f);
				default:
					break;
			}
			break;
		case Autolang::DefaultClass::boolClassId: {
			if (right->classId == Autolang::DefaultClass::boolClassId) {
				return context.constValuePool.push(
				    left->line, left->obj->b != right->obj->b);
			}
			throwInvalidCompare(in_data, left, right, "!=");
		}
		case Autolang::DefaultClass::stringClassId: {
			if (right->classId == Autolang::DefaultClass::stringClassId) {
				return context.constValuePool.push(
				    left->line, *static_cast<std::string *>(left->str) !=
				                    *static_cast<std::string *>(right->str));
			}
		}
		default:
			break;
	}

	if (left->classId == right->classId &&
	    compile.classes[left->classId]->classFlags &
	        ClassFlags::CLASS_IS_ENUM) {
		return context.constValuePool.push(left->line, left != right);
	}

	throw ParserError(left->line, "Invalid types for operator !=");
}

ConstValueNode *op_greater_than(in_func, ConstValueNode *left,
                                ConstValueNode *right) {
	prepareOperands(in_data, left, right);
	switch (left->classId) {
		case Autolang::DefaultClass::intClassId:
			switch (right->classId) {
				case Autolang::DefaultClass::intClassId:
					return context.constValuePool.push(left->line,
					                                   left->i > right->i);
				case Autolang::DefaultClass::floatClassId:
					return context.constValuePool.push(left->line,
					                                   left->i > right->f);
				default:
					break;
			}
			break;
		case Autolang::DefaultClass::floatClassId:
			switch (right->classId) {
				case Autolang::DefaultClass::intClassId:
					return context.constValuePool.push(left->line,
					                                   left->f > right->i);
				case Autolang::DefaultClass::floatClassId:
					return context.constValuePool.push(left->line,
					                                   left->f > right->f);
				default:
					break;
			}
			break;
		default:
			break;
	}
	throw ParserError(left->line, "Invalid types for operator >");
}

ConstValueNode *op_less_than(in_func, ConstValueNode *left,
                             ConstValueNode *right) {
	prepareOperands(in_data, left, right);
	switch (left->classId) {
		case Autolang::DefaultClass::intClassId:
			switch (right->classId) {
				case Autolang::DefaultClass::intClassId:
					return context.constValuePool.push(left->line,
					                                   left->i < right->i);
				case Autolang::DefaultClass::floatClassId:
					return context.constValuePool.push(left->line,
					                                   left->i < right->f);
				default:
					break;
			}
			break;
		case Autolang::DefaultClass::floatClassId:
			switch (right->classId) {
				case Autolang::DefaultClass::intClassId:
					return context.constValuePool.push(left->line,
					                                   left->f < right->i);
				case Autolang::DefaultClass::floatClassId:
					return context.constValuePool.push(left->line,
					                                   left->f < right->f);
				default:
					break;
			}
			break;
		default:
			break;
	}
	throw ParserError(left->line, "Invalid types for operator <");
}

ConstValueNode *op_greater_than_eq(in_func, ConstValueNode *left,
                                   ConstValueNode *right) {
	prepareOperands(in_data, left, right);
	switch (left->classId) {
		case Autolang::DefaultClass::intClassId:
			switch (right->classId) {
				case Autolang::DefaultClass::intClassId:
					return context.constValuePool.push(left->line,
					                                   left->i >= right->i);
				case Autolang::DefaultClass::floatClassId:
					return context.constValuePool.push(left->line,
					                                   left->i >= right->f);
				default:
					break;
			}
			break;
		case Autolang::DefaultClass::floatClassId:
			switch (right->classId) {
				case Autolang::DefaultClass::intClassId:
					return context.constValuePool.push(left->line,
					                                   left->f >= right->i);
				case Autolang::DefaultClass::floatClassId:
					return context.constValuePool.push(left->line,
					                                   left->f >= right->f);
				default:
					break;
			}
			break;
		default:
			break;
	}
	throw ParserError(left->line, "Invalid types for operator >=");
}

ConstValueNode *op_less_than_eq(in_func, ConstValueNode *left,
                                ConstValueNode *right) {
	prepareOperands(in_data, left, right);
	switch (left->classId) {
		case Autolang::DefaultClass::intClassId:
			switch (right->classId) {
				case Autolang::DefaultClass::intClassId:
					return context.constValuePool.push(left->line,
					                                   left->i <= right->i);
				case Autolang::DefaultClass::floatClassId:
					return context.constValuePool.push(left->line,
					                                   left->i <= right->f);
				default:
					break;
			}
			break;
		case Autolang::DefaultClass::floatClassId:
			switch (right->classId) {
				case Autolang::DefaultClass::intClassId:
					return context.constValuePool.push(left->line,
					                                   left->f <= right->i);
				case Autolang::DefaultClass::floatClassId:
					return context.constValuePool.push(left->line,
					                                   left->f <= right->f);
				default:
					break;
			}
			break;
		default:
			break;
	}
	throw ParserError(left->line, "Invalid types for operator <=");
}

ConstValueNode *toInt(in_func, ConstValueNode *value) {
	switch (value->classId) {
		case Autolang::DefaultClass::intClassId:
			return value;

		case Autolang::DefaultClass::floatClassId:
			return context.constValuePool.push(value->line,
			                                   static_cast<int64_t>(value->f));

		case Autolang::DefaultClass::boolClassId:
			return context.constValuePool.push(
			    value->line, static_cast<int64_t>(value->obj->b));

		default:
			break;
	}
	throw ParserError(value->line, "Cannot convert to Int");
}

ConstValueNode *toFloat(in_func, ConstValueNode *value) {
	switch (value->classId) {
		case Autolang::DefaultClass::intClassId:
			return context.constValuePool.push(value->line,
			                                   static_cast<double>(value->i));

		case Autolang::DefaultClass::floatClassId:
			return value;

		case Autolang::DefaultClass::boolClassId:
			return context.constValuePool.push(
			    value->line, static_cast<double>(value->obj->b));

		default:
			break;
	}
	throw ParserError(value->line, "Cannot convert to Float");
}

ConstValueNode *toBool(in_func, ConstValueNode *value) {
	switch (value->classId) {
		case Autolang::DefaultClass::intClassId:
			return context.constValuePool.push(value->line, value->i != 0);

		case Autolang::DefaultClass::floatClassId:
			return context.constValuePool.push(value->line, value->f != 0);

		case Autolang::DefaultClass::boolClassId:
			return value;

		default:
			break;
	}
	throw ParserError(value->line, "Cannot convert to Bool");
}

ConstValueNode *toString(in_func, ConstValueNode *value) {
	switch (value->classId) {
		case Autolang::DefaultClass::intClassId:
			return context.constValuePool.push(value->line,
			                                   std::to_string(value->i));

		case Autolang::DefaultClass::floatClassId:
			return context.constValuePool.push(value->line,
			                                   std::to_string(value->f));

		case Autolang::DefaultClass::boolClassId:
			return context.constValuePool.push(
			    value->line, value->obj->b ? "true" : "false");

		case Autolang::DefaultClass::stringClassId:
			return value;

		default:
			break;
	}
	throw ParserError(value->line, "Cannot convert to String");
}

} // namespace Autolang

#endif

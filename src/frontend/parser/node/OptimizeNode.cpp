#ifndef OPTIMIZE_NODE_CPP
#define OPTIMIZE_NODE_CPP

#include <iostream>
#include "frontend/parser/node/OptimizeNode.hpp"
#include "frontend/parser/node/Node.hpp"
#include "shared/AObject.hpp"
#include "frontend/parser/Debugger.hpp"

namespace AutoLang
{

#define create_operator_func_plus_node(name, op)                                                                \
	ConstValueNode *name(ConstValueNode *left, ConstValueNode *right)                                           \
	{                                                                                                           \
		auto obj1 = left;                                                                                       \
		if (obj1->classId == AutoLang::DefaultClass::boolClassId)                                               \
		{                                                                                                       \
			toInt(left);                                                                                        \
			return name(left, right);                                                                           \
		}                                                                                                       \
		auto obj2 = right;                                                                                      \
		if (obj2->classId == AutoLang::DefaultClass::boolClassId)                                               \
		{                                                                                                       \
			toInt(right);                                                                                       \
			return name(left, right);                                                                           \
		}                                                                                                       \
		switch (obj1->classId)                                                                                  \
		{                                                                                                       \
		case AutoLang::DefaultClass::intClassId:                                                                \
		{                                                                                                       \
			switch (obj2->classId)                                                                              \
			{                                                                                                   \
			case AutoLang::DefaultClass::intClassId:                                                            \
				return new ConstValueNode(left->line, (obj1->i)op(obj2->i));                                                \
			case AutoLang::DefaultClass::floatClassId:                                                          \
				return new ConstValueNode(left->line, (obj1->i)op(obj2->f));                                                \
			default:                                                                                            \
				if (obj2->classId != AutoLang::DefaultClass::stringClassId)                                     \
					break;                                                                                      \
				return new ConstValueNode(left->line, (std::to_string(obj1->i))op(*static_cast<std::string *>(obj2->str))); \
			}                                                                                                   \
			break;                                                                                              \
		}                                                                                                       \
		case AutoLang::DefaultClass::floatClassId:                                                              \
		{                                                                                                       \
			switch (obj2->classId)                                                                              \
			{                                                                                                   \
			case AutoLang::DefaultClass::intClassId:                                                            \
				return new ConstValueNode(left->line, (obj1->f)op(obj2->i));                                                \
			case AutoLang::DefaultClass::floatClassId:                                                          \
				return new ConstValueNode(left->line, (obj1->f)op(obj2->f));                                                \
			default:                                                                                            \
				if (obj2->classId != AutoLang::DefaultClass::stringClassId)                                     \
					break;                                                                                      \
				return new ConstValueNode(left->line, (std::to_string(obj1->f))op(*static_cast<std::string *>(obj2->str))); \
			}                                                                                                   \
			break;                                                                                              \
		}                                                                                                       \
		default:                                                                                                \
		{                                                                                                       \
			std::string &str = *static_cast<std::string *>(obj1->str);                                          \
			switch (obj2->classId)                                                                              \
			{                                                                                                   \
			case AutoLang::DefaultClass::intClassId:                                                            \
				return new ConstValueNode(left->line, (str)op(std::to_string(obj2->i)));                                    \
			case AutoLang::DefaultClass::floatClassId:                                                          \
				return new ConstValueNode(left->line, (str)op(std::to_string(obj2->f)));                                    \
			default:                                                                                            \
				if (obj2->classId != AutoLang::DefaultClass::stringClassId)                                     \
					break;                                                                                      \
				return new ConstValueNode(left->line, (str)op(*static_cast<std::string *>(obj2->str)));                     \
			}                                                                                                   \
			break;                                                                                              \
		}                                                                                                       \
		}                                                                                                       \
		throw std::runtime_error("h");                                                                           \
	}

#define create_operator_number_node(name, op)                         \
	ConstValueNode *name(ConstValueNode *left, ConstValueNode *right) \
	{                                                                 \
		auto obj1 = left;                                             \
		if (obj1->classId == AutoLang::DefaultClass::boolClassId)     \
		{                                                             \
			toInt(left);                                              \
			return name(left, right);                                 \
		}                                                             \
		auto obj2 = right;                                            \
		if (obj2->classId == AutoLang::DefaultClass::boolClassId)     \
		{                                                             \
			toInt(right);                                             \
			return name(left, right);                                 \
		}                                                             \
		switch (obj1->classId)                                        \
		{                                                             \
		case AutoLang::DefaultClass::intClassId:                      \
		{                                                             \
			switch (obj2->classId)                                    \
			{                                                         \
			case AutoLang::DefaultClass::intClassId:                  \
				return new ConstValueNode(left->line, (obj1->i)op(obj2->i));      \
			case AutoLang::DefaultClass::floatClassId:                \
				return new ConstValueNode(left->line, (obj1->i)op(obj2->f));      \
			default:                                                  \
				break;                                                \
			}                                                         \
			break;                                                    \
		}                                                             \
		case AutoLang::DefaultClass::floatClassId:                    \
		{                                                             \
			switch (obj2->classId)                                    \
			{                                                         \
			case AutoLang::DefaultClass::intClassId:                  \
				return new ConstValueNode(left->line, (obj1->f)op(obj2->i));      \
			case AutoLang::DefaultClass::floatClassId:                \
				return new ConstValueNode(left->line, (obj1->f)op(obj2->f));      \
			default:                                                  \
				break;                                                \
			}                                                         \
			break;                                                    \
		}                                                             \
		default:                                                      \
			break;                                                    \
		}                                                             \
		throw std::runtime_error("g");                                 \
	}

#define create_operator_compare(name, op)                       \
	ConstValueNode *name(ConstValueNode *left, ConstValueNode *right)  \
	{                                                                  \
		auto obj1 = left;                                              \
		auto obj2 = right;                                             \
		switch (obj1->classId)                                         \
		{                                                              \
		case AutoLang::DefaultClass::intClassId:                       \
		{                                                              \
			switch (obj2->classId)                                     \
			{                                                          \
			case AutoLang::DefaultClass::intClassId:                   \
				return new ConstValueNode(left->line, (obj1->i)op(obj2->i));       \
			case AutoLang::DefaultClass::floatClassId:                 \
				return new ConstValueNode(left->line, (obj1->i)op(obj2->f));       \
			default:                                                   \
				break;                                                 \
			}                                                          \
			break;                                                     \
		}                                                              \
		case AutoLang::DefaultClass::floatClassId:                     \
		{                                                              \
			switch (obj2->classId)                                     \
			{                                                          \
			case AutoLang::DefaultClass::intClassId:                   \
				return new ConstValueNode(left->line, (obj1->f)op(obj2->i));       \
			case AutoLang::DefaultClass::floatClassId:                 \
				return new ConstValueNode(left->line, (obj1->f)op(obj2->f));       \
			default:                                                   \
				break;                                                 \
			}                                                          \
			break;                                                     \
		}                                                              \
		default:                                                       \
			break;                                                     \
		}                                                              \
		if (obj1->classId == AutoLang::DefaultClass::boolClassId &&    \
			obj2->classId == AutoLang::DefaultClass::boolClassId)      \
		{                                                              \
			return new ConstValueNode(left->line, (obj1->obj->b)op(obj2->obj->b)); \
		}                                                              \
		if (obj1->classId == AutoLang::DefaultClass::stringClassId &&  \
			obj2->classId == AutoLang::DefaultClass::stringClassId)    \
		{                                                              \
			return new ConstValueNode(left->line, (obj1->str)op(obj2->str));       \
		}                                                              \
		throw std::runtime_error("f");                                  \
	}

	ConstValueNode *bitwise_and(ConstValueNode *left, ConstValueNode *right)
	{
		auto obj1 = left;
		auto obj2 = right;
		switch (obj1->classId)
		{
		case AutoLang::DefaultClass::intClassId:
		{
			switch (obj2->classId)
			{
			case AutoLang::DefaultClass::intClassId:
				return new ConstValueNode(left->line, (obj1->i) & (obj2->i));
			default:
				break;
			}
			break;
		}
		default:
			break;
		}
		throw std::runtime_error("e");
	}

	ConstValueNode *bitwise_or(ConstValueNode *left, ConstValueNode *right)
	{
		auto obj1 = left;
		auto obj2 = right;
		switch (obj1->classId)
		{
		case AutoLang::DefaultClass::intClassId:
		{
			switch (obj2->classId)
			{
			case AutoLang::DefaultClass::intClassId:
				return new ConstValueNode(left->line, (obj1->i) | (obj2->i));
			default:
				break;
			}
			break;
		}
		default:
			break;
		}
		throw std::runtime_error("d");
	}

	ConstValueNode *mod(ConstValueNode *left, ConstValueNode *right)
	{
		auto obj1 = left;
		auto obj2 = right;
		switch (obj1->classId)
		{
		case AutoLang::DefaultClass::intClassId:
		{
			switch (obj2->classId)
			{
			case AutoLang::DefaultClass::intClassId:
				return new ConstValueNode(left->line, (obj1->i) % (obj2->i));
			case AutoLang::DefaultClass::floatClassId:
				return new ConstValueNode(left->line, static_cast<double>(std::fmod((obj1->i), (obj2->f))));
			default:
				break;
			}
			break;
		}
		case AutoLang::DefaultClass::floatClassId:
		{
			switch (obj2->classId)
			{
			case AutoLang::DefaultClass::intClassId:
				return new ConstValueNode(left->line, static_cast<double>(std::fmod((obj1->f), (obj2->i))));
			case AutoLang::DefaultClass::floatClassId:
				return new ConstValueNode(left->line, static_cast<double>(std::fmod((obj1->f), (obj2->f))));
			default:
				break;
			}
			break;
		}
		default:
			break;
		}
		throw std::runtime_error("c");
	}

	create_operator_func_plus_node(plus, +);
	create_operator_number_node(minus, -);
	create_operator_number_node(mul, *);
	create_operator_number_node(divide, /);
	create_operator_compare(op_eqeq, ==);
	create_operator_compare(op_not_eq, !=);

	create_operator_number_node(op_greater_than, >);
	create_operator_number_node(op_less_than, <);
	create_operator_number_node(op_greater_than_eq, >=);
	create_operator_number_node(op_less_than_eq, <=);

	void toInt(ConstValueNode *value)
	{
		switch (value->classId)
		{
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
		throw std::runtime_error("");
	}

	void toFloat(ConstValueNode *value)
	{
		switch (value->classId)
		{
		case AutoLang::DefaultClass::intClassId:
			value->classId = AutoLang::DefaultClass::floatClassId;
			value->f = static_cast<double>(value->i);
			return;
		case AutoLang::DefaultClass::floatClassId:
			return;
		default:
			break;
		}
		if (value->classId == AutoLang::DefaultClass::boolClassId)
		{
			value->classId = AutoLang::DefaultClass::floatClassId;
			value->f = static_cast<double>(value->obj->b);
			return;
		}
		throw std::runtime_error("a");
	}

}

#endif
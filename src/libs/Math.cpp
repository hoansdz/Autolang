#ifndef LIB_MATH_CPP
#define LIB_MATH_CPP

#include "./Math.hpp"

namespace AutoLang
{
	namespace Libs
	{
		namespace Math
		{

			void init(CompiledProgram &compile)
			{
				auto math = &compile.classes[compile.registerClass("Math")];
				compile.registerFunction(
					math,
					true,
					"round()",
					{AutoLang::DefaultClass::floatClassId},
					{false},
					AutoLang::DefaultClass::intClassId,
					false,
					&round);
				compile.registerFunction(
					math,
					true,
					"floor()",
					{AutoLang::DefaultClass::floatClassId},
					{false},
					AutoLang::DefaultClass::intClassId,
					false,
					&floor);
				compile.registerFunction(
					math,
					true,
					"ceil()",
					{AutoLang::DefaultClass::floatClassId},
					{false},
					AutoLang::DefaultClass::intClassId,
					false,
					&ceil);
				compile.registerFunction(
					math,
					true,
					"trunc()",
					{AutoLang::DefaultClass::floatClassId},
					{false},
					AutoLang::DefaultClass::intClassId,
					false,
					&trunc);
				compile.registerFunction(
					math,
					true,
					"pow()",
					{AutoLang::DefaultClass::intClassId, AutoLang::DefaultClass::intClassId},
					{false, false},
					AutoLang::DefaultClass::intClassId,
					false,
					&pow);
				compile.registerFunction(
					math,
					true,
					"pow()",
					{AutoLang::DefaultClass::floatClassId, AutoLang::DefaultClass::floatClassId},
					{false, false},
					AutoLang::DefaultClass::floatClassId,
					false,
					&pow);
				compile.registerFunction(
					math,
					true,
					"abs()",
					{AutoLang::DefaultClass::intClassId},
					{false},
					AutoLang::DefaultClass::intClassId,
					false,
					&abs);
				compile.registerFunction(
					math,
					true,
					"abs()",
					{AutoLang::DefaultClass::floatClassId},
					{false},
					AutoLang::DefaultClass::floatClassId,
					false,
					&abs);
				compile.registerFunction(
					math,
					true,
					"sin()",
					{AutoLang::DefaultClass::floatClassId},
					{false},
					AutoLang::DefaultClass::floatClassId,
					false,
					&sin);
				compile.registerFunction(
					math,
					true,
					"cos()",
					{AutoLang::DefaultClass::floatClassId},
					{false},
					AutoLang::DefaultClass::floatClassId,
					false,
					&cos);
				compile.registerFunction(
					math,
					true,
					"tan()",
					{AutoLang::DefaultClass::floatClassId},
					{false},
					AutoLang::DefaultClass::floatClassId,
					false,
					&tan);
			}

			AObject *abs(NativeFuncInData)
			{
				if (size != 1)
					return nullptr;
				auto obj = stackAllocator[0];
				switch (obj->type)
				{
				case AutoLang::DefaultClass::intClassId:
					return manager.create(static_cast<int64_t>(std::abs(obj->i)));
				case AutoLang::DefaultClass::floatClassId:
					return manager.create(std::abs(obj->f));
				default:
					throw std::runtime_error("Cannot run with this type");
				}
			}

			AObject *pow(NativeFuncInData)
			{
				if (size != 2)
					return nullptr;
				// base
				auto obj1 = stackAllocator[0];
				// input
				auto obj2 = stackAllocator[1];
				switch (obj1->type)
				{
				case AutoLang::DefaultClass::intClassId:
					switch (obj2->type)
					{
					case AutoLang::DefaultClass::intClassId:
						return manager.create(integer_pow(obj1->i, obj2->i));
					case AutoLang::DefaultClass::floatClassId:
						return manager.create(std::pow(obj1->i, obj2->f));
					default:
						throw std::runtime_error("Cannot run with this type");
					}
				case AutoLang::DefaultClass::floatClassId:
					switch (obj2->type)
					{
					case AutoLang::DefaultClass::intClassId:
						return manager.create(std::pow(obj1->f, obj2->i));
					case AutoLang::DefaultClass::floatClassId:
						return manager.create(std::pow(obj1->f, obj2->f));
					default:
						throw std::runtime_error("Cannot run with this type");
					}
				default:
					throw std::runtime_error("Cannot run with this type");
				}
			}

#define in_num_out_int(name, func)                                     \
	AObject *name(NativeFuncInData)                                    \
	{                                                                  \
		if (size != 1)                                                 \
			return nullptr;                                            \
		auto obj = stackAllocator[0];                                  \
		switch (obj->type)                                             \
		{                                                              \
		case AutoLang::DefaultClass::intClassId:                       \
			return manager.create(static_cast<int64_t>(func(obj->i))); \
		case AutoLang::DefaultClass::floatClassId:                     \
			return manager.create(static_cast<int64_t>(func(obj->f))); \
		default:                                                       \
			throw std::runtime_error("Cannot run with this type");     \
		}                                                              \
	}

#define in_num_out_float(name, func)                                  \
	AObject *name(NativeFuncInData)                                   \
	{                                                                 \
		if (size != 1)                                                \
			return nullptr;                                           \
		auto obj = stackAllocator[0];                                 \
		switch (obj->type)                                            \
		{                                                             \
		case AutoLang::DefaultClass::intClassId:                      \
			return manager.create(static_cast<double>(func(obj->i))); \
		case AutoLang::DefaultClass::floatClassId:                    \
			return manager.create(static_cast<double>(func(obj->f))); \
		default:                                                      \
			throw std::runtime_error("Cannot run with this type");    \
		}                                                             \
	}

			in_num_out_int(round, std::round);
			in_num_out_int(floor, std::floor);
			in_num_out_int(ceil, std::ceil);
			in_num_out_float(sin, std::sin);
			in_num_out_float(cos, std::cos);
			in_num_out_float(tan, std::tan);

			AObject *trunc(NativeFuncInData)
			{
				if (size != 1)
					return nullptr;
				auto obj = stackAllocator[0];
				switch (obj->type)
				{
				case AutoLang::DefaultClass::intClassId:
					return manager.create(obj->i);
				case AutoLang::DefaultClass::floatClassId:
					return manager.create(static_cast<int64_t>(std::trunc(obj->f)));
				default:
					throw std::runtime_error("Cannot run with this type");
				}
			}

			AObject *fmod(NativeFuncInData)
			{
				if (size != 2)
					return nullptr;
				// base
				auto obj1 = stackAllocator[0];
				// input
				auto obj2 = stackAllocator[1];
				switch (obj1->type)
				{
				case AutoLang::DefaultClass::intClassId:
					switch (obj1->type)
					{
					case AutoLang::DefaultClass::intClassId:
						return manager.create(static_cast<int64_t>(std::fmod(obj1->i, obj2->i)));
					case AutoLang::DefaultClass::floatClassId:
						return manager.create(std::fmod(obj1->i, obj2->f));
					default:
						throw std::runtime_error("Cannot run with this type");
					}
				case AutoLang::DefaultClass::floatClassId:
					switch (obj1->type)
					{
					case AutoLang::DefaultClass::intClassId:
						return manager.create(std::fmod(obj1->f, obj2->i));
					case AutoLang::DefaultClass::floatClassId:
						return manager.create(std::fmod(obj1->f, obj2->f));
					default:
						throw std::runtime_error("Cannot run with this type");
					}
				default:
					throw std::runtime_error("Cannot run with this type");
				}
			}

			int64_t integer_pow(int64_t base, int64_t exp)
			{
				if (exp < 0)
					throw std::runtime_error("Exponent must be non-negative");

				int64_t result = 1;
				while (exp > 0)
				{
					if (exp & 1)
						result *= base;
					base *= base;
					exp >>= 1;
				}
				return result;
			}

		}
	}
}
#endif
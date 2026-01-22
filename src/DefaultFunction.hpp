#ifndef DEFAULT_FUNCTION_HPP
#define DEFAULT_FUNCTION_HPP

#include <iostream>
#include <cstdlib>
#include "Debugger.hpp"
#include "DefaultClass.hpp"
#include "libs/Math.hpp"

namespace AutoLang
{
	namespace DefaultFunction
	{

		void init(CompiledProgram &compile);
		inline AObject *data_constructor(NativeFuncInData);
		inline AObject *print(NativeFuncInData);
		inline AObject *println(NativeFuncInData);
		inline AObject *get_refcount(NativeFuncInData);
		inline AObject *string_constructor(NativeFuncInData);
		inline AObject *to_int(NativeFuncInData);
		inline AObject *to_float(NativeFuncInData);
		inline AObject *to_string(NativeFuncInData);
		inline AObject *get_string_size(NativeFuncInData);

		AObject *data_constructor(NativeFuncInData)
		{
			AObject *obj = stackAllocator[0];
			obj->refCount = 0;
			for (size_t i = 1; i < size; ++i)
			{
				AObject **last = &obj->member->data[i - 1];
				*last = stackAllocator[i];
				(*last)->retain();
			}
			stackAllocator[0] = nullptr;
			return obj;
		}

		AObject *print(NativeFuncInData)
		{
			AObject *obj = stackAllocator[0];
			uint32_t type = obj->type;
			switch (type)
			{
			case AutoLang::DefaultClass::intClassId:
				std::cout << obj->i;
				break;
			case AutoLang::DefaultClass::floatClassId:
				std::cout << obj->f;
				break;
			default:
				if (type == DefaultClass::stringClassId)
				{
					std::cout << obj->str->data;
				}
				else if (type == DefaultClass::nullClassId)
				{
					std::cout << "null";
				}
				else if (type == DefaultClass::boolClassId)
				{
					std::cout << (obj == DefaultClass::trueObject ? "true" : "false");
				}
				break;
			}
			return nullptr;
		}

		AObject *println(NativeFuncInData)
		{
			print(manager, stackAllocator, size);
			std::cout << '\n';
			return nullptr;
		}

		AObject *get_refcount(NativeFuncInData)
		{
			return manager.createIntObject(static_cast<int64_t>(stackAllocator[0]->refCount - 1));
		}

		AObject *to_int(NativeFuncInData)
		{
			if (size != 1)
				return nullptr;
			auto obj = stackAllocator[0];
			switch (obj->type)
			{
			case AutoLang::DefaultClass::intClassId:
				return manager.createIntObject(obj->i);
			case AutoLang::DefaultClass::floatClassId:
				return manager.createIntObject(static_cast<int64_t>(obj->f));
			default:
			{
				if (obj->type == AutoLang::DefaultClass::boolClassId)
					return manager.createIntObject(static_cast<int64_t>(obj->b));
				// Check error but unused
				char *end;
				return manager.createIntObject(std::strtoll(obj->str->data, &end, 10));
			}
			}
		}

		AObject *to_float(NativeFuncInData)
		{
			if (size != 1)
				return nullptr;
			auto obj = stackAllocator[0];
			switch (obj->type)
			{
			case AutoLang::DefaultClass::intClassId:
				return manager.createFloatObject(static_cast<double>(obj->i));
			case AutoLang::DefaultClass::floatClassId:
				return manager.createFloatObject(obj->f);
			default:
			{
				if (obj->type == AutoLang::DefaultClass::boolClassId)
					return manager.createFloatObject(static_cast<double>(obj->b));
				// Check error but unused
				char *end;
				return manager.createFloatObject(std::strtod(obj->str->data, &end));
			}
			}
		}

		AObject *to_string(NativeFuncInData)
		{
			if (size != 1)
				return nullptr;
			auto obj = stackAllocator[0];
			switch (obj->type)
			{
			case AutoLang::DefaultClass::intClassId:
				return manager.create(AString::from(obj->i));
			case AutoLang::DefaultClass::floatClassId:
				return manager.create(AString::from(obj->f));
			default:
				return manager.create(AString::copy(obj->str));
			}
		}

		AObject *string_constructor(NativeFuncInData)
		{
			// To string
			if (size == 1)
			{
				return to_string(manager, stackAllocator, size);
			}
			//"hi",3 => "hihihi"
			if (size == 2)
			{
				int64_t count = stackAllocator[1]->i;
				AString *oldAStr = stackAllocator[0]->str;
				if (count <= 0 || oldAStr->size == 0)
				{
					char *newStr = new char[1];
					newStr[0] = '\0';
					return manager.createStringObject(new AString(newStr, 0));
				}
				size_t newSize = oldAStr->size * count;
				char *newStr = new char[newSize + 1];
				for (int i = 0; i < count; ++i)
				{
					memcpy(&newStr[i * oldAStr->size], oldAStr->data, oldAStr->size);
				}
				newStr[newSize] = '\0';
				return manager.createStringObject(new AString(newStr, newSize));
			}
			return nullptr;
		}

		AObject *get_string_size(NativeFuncInData)
		{
			return manager.createIntObject(static_cast<int64_t>(stackAllocator[0]->str->size));
		}

	}
}

#endif
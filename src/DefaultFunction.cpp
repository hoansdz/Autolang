#ifndef DEFAULT_FUNCTION_CPP
#define DEFAULT_FUNCTION_CPP

#include "DefaultFunction.hpp"
#include "DefaultClass.hpp"

namespace AutoLang {
namespace DefaultFunction {

void init(CompiledProgram& compile) {
	compile.registerFunction( 
		nullptr,
		true,
		"print()",
		{ AutoLang::DefaultClass::anyClassId },
		AutoLang::DefaultClass::nullClassId,
		&print
	);
	compile.registerFunction( 
		nullptr,
		true,
		"println()",
		{ AutoLang::DefaultClass::anyClassId }, 
		AutoLang::DefaultClass::nullClassId,
		&println
	);
	compile.registerFunction( 
		nullptr,
		true,
		"getRefCount()",
		{ AutoLang::DefaultClass::anyClassId }, 
		AutoLang::DefaultClass::INTCLASSID,
		&get_refcount
	);
	auto integer= &compile.classes[AutoLang::DefaultClass::INTCLASSID];
	compile.registerFunction( 
		integer,
		false,
		"toString()",
		{  }, 
		AutoLang::DefaultClass::stringClassId,
		&to_string
	);
	compile.registerFunction( 
		integer,
		false,
		"toFloat()",
		{  }, 
		AutoLang::DefaultClass::FLOATCLASSID,
		&AutoLang::DefaultFunction::to_float
	);
	auto Float= &compile.classes[AutoLang::DefaultClass::FLOATCLASSID];
	compile.registerFunction( 
		Float,
		false,
		"toInt()",
		{  }, 
		AutoLang::DefaultClass::INTCLASSID,
		&AutoLang::DefaultFunction::to_int
	);
	compile.registerFunction( 
		Float,
		false,
		"toString()",
		{  }, 
		AutoLang::DefaultClass::stringClassId,
		&AutoLang::DefaultFunction::to_string
	);
	auto string = &compile.classes[AutoLang::DefaultClass::stringClassId];
	compile.registerFunction( 
		string,
		false,
		"toInt()",
		{  }, 
		AutoLang::DefaultClass::INTCLASSID,
		&AutoLang::DefaultFunction::to_int
	);
	compile.registerFunction( 
		string,
		false,
		"size()",
		{  }, 
		AutoLang::DefaultClass::INTCLASSID,
		&AutoLang::DefaultFunction::get_string_size
	);
	compile.registerFunction( 
		string,
		true,
		"String()",
		{ AutoLang::DefaultClass::stringClassId }, 
		AutoLang::DefaultClass::stringClassId,
		&AutoLang::DefaultFunction::string_constructor
	);
	compile.registerFunction( 
		string,
		true,
		"String()",
		{ AutoLang::DefaultClass::stringClassId,
		   AutoLang::DefaultClass::INTCLASSID }, 
		AutoLang::DefaultClass::stringClassId,
		&AutoLang::DefaultFunction::string_constructor
	);
}

/*AObject* print(NativeFuncInData) {
	AObject* obj = stackAllocator[0];
	uint32_t type = obj->type;
	switch (type) {
		case AutoLang::DefaultClass::INTCLASSID:
			std::cout<<obj->i;
			break;
		case AutoLang::DefaultClass::FLOATCLASSID:
			std::cout<<obj->f;
			break;
		default:
			if (type == DefaultClass::stringClassId) {
				std::cout<<*static_cast<std::string*>(obj->ref);
			} else 
			if (type == DefaultClass::nullClassId) {
				std::cout<<"null";
			} else 
			if (type == DefaultClass::boolClassId) {
				std::cout<<(obj == DefaultClass::trueObject ? "true" : "false");
			} else {
				std::cout<<obj->ref;
			}
			break;
	}
	return nullptr;
}

AObject* println(NativeFuncInData) {
	print(manager, stackAllocator, size);
	std::cout<<'\n';
	return nullptr;
}

AObject* to_int(NativeFuncInData) {
	if (size != 1) return nullptr;
	auto obj = stackAllocator[0];
	switch (obj->type) {
		case AutoLang::DefaultClass::INTCLASSID:
			return manager.create(obj->i);
		case AutoLang::DefaultClass::FLOATCLASSID:
			return manager.create(static_cast<long long>(obj->f));
		default:
			return manager.create(std::stoll(*static_cast<std::string*>(obj->ref)));
	}
}

AObject* to_float(NativeFuncInData) {
	if (size != 1) return nullptr;
	auto obj = stackAllocator[0];
	switch (obj->type) {
		case AutoLang::DefaultClass::INTCLASSID:
			return manager.create(static_cast<double>(obj->i));
		case AutoLang::DefaultClass::FLOATCLASSID:
			return manager.create(obj->f);
		default:
			return manager.create(std::stod(*static_cast<std::string*>(obj->ref)));
	}
}

AObject* to_string(NativeFuncInData) {
	if (size != 1) return nullptr;
	auto obj = stackAllocator[0];
	switch (obj->type) {
		case AutoLang::DefaultClass::INTCLASSID:
			return manager.create(std::to_string(obj->i));
		case AutoLang::DefaultClass::FLOATCLASSID:
			return manager.create(std::to_string(obj->f));
		default:
			return manager.create(*static_cast<std::string*>(obj->ref));
	}
}

AObject* string_constructor(NativeFuncInData) {
	//To string
	if (size == 1) {
		return to_string(manager, stackAllocator, size);
	}
	//"hi",3 => "hihihi"
	if (size == 2) {
		AObject* newObj = manager.create("");
		std::string& newStr=*static_cast<std::string*>(newObj->ref);
		std::string& str=*static_cast<std::string*>(stackAllocator[0]->ref);
		auto times = stackAllocator[1]->i;
		for (int i=0; i<times; ++i){
			newStr+=str;
		}
		return newObj;
	}
	return nullptr;
}

#define create_operator_func_plus(name, op) AObject* name(NativeFuncInData) {\
	if (size != 2) return nullptr;\
	auto obj1 = stackAllocator[0];\
	switch (obj1->type) {\
		case AutoLang::DefaultClass::INTCLASSID: {\
			auto obj2 = stackAllocator[1];\
			switch (obj2->type) {\
				case AutoLang::DefaultClass::INTCLASSID:\
					return manager.create((obj1->i) op (obj2->i));\
				case AutoLang::DefaultClass::FLOATCLASSID:\
					return manager.create((obj1->i) op (obj2->f));\
				default:\
					return manager.create((std::to_string(obj1->i)) op (*static_cast<std::string*>(obj2->ref)));\
			}\
		}\
		case AutoLang::DefaultClass::FLOATCLASSID: {\
			auto obj2 = stackAllocator[1];\
			switch (obj2->type) {\
				case AutoLang::DefaultClass::INTCLASSID:\
					return manager.create((obj1->f) op (obj2->i));\
				case AutoLang::DefaultClass::FLOATCLASSID:\
					return manager.create((obj1->f) op (obj2->f));\
				default:\
					return manager.create((std::to_string(obj1->f)) op (*static_cast<std::string*>(obj2->ref)));\
			}\
		}\
		default: {\
			auto obj2 = stackAllocator[1];\
			std::string& str = *static_cast<std::string*>(obj1->ref);\
			switch (obj2->type) {\
				case AutoLang::DefaultClass::INTCLASSID:\
					return manager.create((str) op (std::to_string(obj2->i)));\
				case AutoLang::DefaultClass::FLOATCLASSID:\
					return manager.create((str) op (std::to_string(obj2->f)));\
				default:\
					return manager.create((str) op (*static_cast<std::string*>(obj2->ref)));\
			}\
		}\
	}\
	return nullptr;\
}

#define create_operator_number(name, op) AObject* name(NativeFuncInData) {\
	if (size != 2) return nullptr;\
	auto obj1 = stackAllocator[0];\
	switch (obj1->type) {\
		case AutoLang::DefaultClass::INTCLASSID: {\
			auto obj2 = stackAllocator[1];\
			switch (obj2->type) {\
				case AutoLang::DefaultClass::INTCLASSID:\
					return manager.create((obj1->i) op (obj2->i));\
				case AutoLang::DefaultClass::FLOATCLASSID:\
					return manager.create((obj1->i) op (obj2->f));\
				default:\
					break;\
			}\
		}\
		case AutoLang::DefaultClass::FLOATCLASSID: {\
			auto obj2 = stackAllocator[1];\
			switch (obj2->type) {\
				case AutoLang::DefaultClass::INTCLASSID:\
					return manager.create((obj1->f) op (obj2->i));\
				case AutoLang::DefaultClass::FLOATCLASSID:\
					return manager.create((obj1->f) op (obj2->f));\
				default:\
					break;\
			}\
		}\
		default: \
			break;\
	}\
	return nullptr;\
}

AObject* mod(NativeFuncInData) {
	auto obj1 = stackAllocator[0];
	auto obj2 = stackAllocator[1];
	switch (obj1->type) {
		case AutoLang::DefaultClass::INTCLASSID: {
			switch (obj2->type) {
				case AutoLang::DefaultClass::INTCLASSID:
					return manager.create((obj1->i) % (obj2->i));
				case AutoLang::DefaultClass::FLOATCLASSID:
					return manager.create(static_cast<double>(std::fmod((obj1->i), (obj2->f))));
				default:
					break;
			}
			break;
		}
		case AutoLang::DefaultClass::FLOATCLASSID: {
			switch (obj2->type) {
				case AutoLang::DefaultClass::INTCLASSID:
					return manager.create(static_cast<double>(std::fmod((obj1->f), (obj2->i))));
				case AutoLang::DefaultClass::FLOATCLASSID:
					return manager.create(static_cast<double>(std::fmod((obj1->f), (obj2->f))));
				default:
					break;
			}
			break;
		}
		default:
			break;
	}
	throw std::runtime_error("Cannot mod 2 variable");
}

create_operator_func_plus(plus, +);
create_operator_number(minus, -);
create_operator_number(mul, *);
create_operator_number(divide, /);

AObject* negative(NativeFuncInData) {
	auto obj = stackAllocator[0];
	switch (obj->type) {
		case AutoLang::DefaultClass::INTCLASSID:
			return manager.create(-obj->i);
		case AutoLang::DefaultClass::FLOATCLASSID:
			return manager.create(-obj->f);
		default:
			if (obj->type == AutoLang::DefaultClass::boolClassId)
				return manager.create(-static_cast<long long>(obj->b));
			throw std::runtime_error("Cannot negative");
	}
}

AObject* op_not(NativeFuncInData) {
	auto obj = stackAllocator[0];
	if (obj->type == AutoLang::DefaultClass::boolClassId)
		return manager.create(!obj->b);
	if (obj->type == AutoLang::DefaultClass::nullClassId)
		return manager.create(true);
	throw std::runtime_error("Cannot use not oparator");
}

AObject* op_and_and(NativeFuncInData) {
	auto obj1 = stackAllocator[0];
	auto obj2 = stackAllocator[1];
	return ObjectManager::create(obj1->b && obj2->b);
}

AObject* op_or_or(NativeFuncInData) {
	auto obj1 = stackAllocator[0];
	auto obj2 = stackAllocator[1];
	return ObjectManager::create(obj1->b || obj2->b);
}

AObject* op_eq_pointer(NativeFuncInData) {
	auto obj1 = stackAllocator[0];
	auto obj2 = stackAllocator[1];
	return ObjectManager::create(obj1 == obj2);
}

AObject* op_not_eq_pointer(NativeFuncInData) {
	auto obj1 = stackAllocator[0];
	auto obj2 = stackAllocator[1];
	return ObjectManager::create(obj1 != obj2);
}

create_operator_number(op_less_than, <);
create_operator_number(op_greater_than, >);
create_operator_number(op_less_than_eq, <=);
create_operator_number(op_greater_than_eq, >=);
create_operator_number(op_eqeq, ==);
create_operator_number(op_not_eq, !=);*/

}
}

#endif
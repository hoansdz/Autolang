#ifndef DEFAULT_FUNCTION_CPP
#define DEFAULT_FUNCTION_CPP

#include "shared/DefaultFunction.hpp"
#include "shared/DefaultClass.hpp"

namespace AutoLang {
namespace DefaultFunction {

void init(CompiledProgram &compile) {
	compile.registerFunction(nullptr, true, "print()",
	                         new ClassId[1]{AutoLang::DefaultClass::anyClassId},
	                         {true}, &print);
	compile.registerFunction(nullptr, true, "println()",
	                         new ClassId[1]{AutoLang::DefaultClass::anyClassId},
	                         {true}, &println);
	compile.registerFunction(nullptr, true, "getRefCount()",
	                         new ClassId[1]{AutoLang::DefaultClass::anyClassId},
	                         {true}, AutoLang::DefaultClass::intClassId, false,
	                         &get_refcount);
	auto integer = compile.classes[AutoLang::DefaultClass::intClassId];
	compile.registerFunction(integer, false, "toString()",
	                         new ClassId[1]{AutoLang::DefaultClass::intClassId},
	                         {false}, AutoLang::DefaultClass::stringClassId,
	                         false, &to_string);
	compile.registerFunction(integer, false, "toFloat()",
	                         new ClassId[1]{AutoLang::DefaultClass::intClassId},
	                         {false}, AutoLang::DefaultClass::floatClassId,
	                         false, &AutoLang::DefaultFunction::to_float);
	auto Float = compile.classes[AutoLang::DefaultClass::floatClassId];
	compile.registerFunction(
	    Float, false, "toInt()",
	    new ClassId[1]{AutoLang::DefaultClass::floatClassId}, {false},
	    AutoLang::DefaultClass::intClassId, false,
	    &AutoLang::DefaultFunction::to_int);
	compile.registerFunction(
	    Float, false, "toString()",
	    new ClassId[1]{AutoLang::DefaultClass::floatClassId}, {false},
	    AutoLang::DefaultClass::stringClassId, false,
	    &AutoLang::DefaultFunction::to_string);
	auto string = compile.classes[AutoLang::DefaultClass::stringClassId];
	compile.registerFunction(
	    string, false, "toInt()",
	    new ClassId[1]{AutoLang::DefaultClass::stringClassId}, {false},
	    AutoLang::DefaultClass::intClassId, false,
	    &AutoLang::DefaultFunction::to_int);
	compile.registerFunction(
	    string, false, "size()",
	    new ClassId[1]{AutoLang::DefaultClass::stringClassId}, {false},
	    AutoLang::DefaultClass::intClassId, false,
	    &AutoLang::DefaultFunction::get_string_size);
	compile.registerFunction(
	    string, true, "String()",
	    new ClassId[2]{AutoLang::DefaultClass::stringClassId,
	                   AutoLang::DefaultClass::stringClassId},
	    {false, false}, AutoLang::DefaultClass::stringClassId, false,
	    &AutoLang::DefaultFunction::string_constructor);
	compile.registerFunction(
	    string, true, "String()",
	    new ClassId[3]{AutoLang::DefaultClass::stringClassId,
	                   AutoLang::DefaultClass::stringClassId,
	                   AutoLang::DefaultClass::intClassId},
	    {false, false, false}, AutoLang::DefaultClass::stringClassId, false,
	    &AutoLang::DefaultFunction::string_constructor);
}
} // namespace DefaultFunction
} // namespace AutoLang

#endif
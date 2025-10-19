#ifndef DEFAULT_CLASS_CPP
#define DEFAULT_CLASS_CPP

#include "Debugger.hpp"
#include "DefaultClass.hpp"
#include "libs/Math.hpp"

namespace AutoLang {
namespace DefaultClass {

uint32_t boolClassId;
uint32_t stringClassId = 0;
uint32_t nullClassId = 0;
uint32_t anyClassId = 0;
AObject* nullObject = nullptr;
AObject* trueObject = nullptr;
AObject* falseObject = nullptr;

void init(CompiledProgram& compile) {
	compile.registerClass("Int");
	compile.registerClass("Float");
	boolClassId = compile.registerClass("Bool");
	nullClassId = compile.registerClass("Null");
	stringClassId = compile.registerClass("String");
	anyClassId = compile.registerClass("Any");
	nullObject = new AObject(nullClassId);
	nullObject->refCount = 9999;
	compile.constPool.push_back(nullObject);
	trueObject = new AObject(boolClassId);
	trueObject->b = true;
	trueObject->refCount = 9999;
	compile.constPool.push_back(trueObject);
	falseObject = new AObject(boolClassId);
	falseObject->b = false;
	falseObject->refCount = 9999;
	compile.constPool.push_back(falseObject);

	AutoLang::Libs::Math::init(compile);
}

}
}

#endif
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
uint32_t voidClassId = 0;
AObject* nullObject = nullptr;
AObject* trueObject = nullptr;
AObject* falseObject = nullptr;

void init(CompiledProgram& compile) {
	static FixedPool<AObject> constObjects;
	compile.registerClass("Int");
	compile.registerClass("Float");
	boolClassId = compile.registerClass("Bool");
	nullClassId = compile.registerClass("Null");
	stringClassId = compile.registerClass("String");
	anyClassId = compile.registerClass("Any");
	voidClassId = compile.registerClass("Void");

	if (!constObjects.objects) {
		constObjects.allocate(builtInObjectSize);
		nullObject = constObjects.push(nullClassId);
		nullObject->refCount = refCountForGlobal;

		trueObject = constObjects.push(boolClassId);
		trueObject->b = true;
		trueObject->refCount = refCountForGlobal;

		falseObject = constObjects.push(boolClassId);
		falseObject->b = false;
		falseObject->refCount = refCountForGlobal;
	} else {
		nullObject  = constObjects[0];
		trueObject  = constObjects[1];
		falseObject = constObjects[2];
	}

	AutoLang::Libs::Math::init(compile);
}

}
}

#endif
#ifndef DEFAULT_CLASS_CPP
#define DEFAULT_CLASS_CPP

#include "frontend/parser/Debugger.hpp"
#include "shared/DefaultClass.hpp"
#include "frontend/libs/Math.hpp"

namespace AutoLang {
namespace DefaultClass {

AObject* nullObject = nullptr;
AObject* trueObject = nullptr;
AObject* falseObject = nullptr;

void init(CompiledProgram& compile) {
	static FixedPool<AObject> constObjects;
	compile.registerClass("Int");
	compile.registerClass("Float");
	compile.registerClass("Bool");
	compile.registerClass("String");
	compile.registerClass("Null");
	compile.registerClass("Any");
	// voidClassId = compile.registerClass("Void");

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
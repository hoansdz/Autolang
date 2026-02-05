#ifndef DEFAULT_CLASS_CPP
#define DEFAULT_CLASS_CPP

#include "frontend/parser/Debugger.hpp"
#include "shared/DefaultClass.hpp"
#include "shared/DefaultFunction.hpp"
#include "frontend/ACompiler.hpp"

namespace AutoLang {
namespace DefaultClass {

AObject* nullObject = nullptr;
AObject* trueObject = nullptr;
AObject* falseObject = nullptr;

void init(ACompiler& compiler) {
	static AObject constObjects[builtInObjectSize] = {
		AObject(nullClassId), AObject(boolClassId), AObject(boolClassId)
	};
	// auto& compile = compiler.vm.data;
	// compile.registerClass("Int");
	// compile.registerClass("Float");
	// compile.registerClass("Bool");
	// compile.registerClass("String");
	// compile.registerClass("Null");
	// compile.registerClass("Any");
	// voidClassId = compile.registerClass("Void");

	if (!trueObject) {
		nullObject  = &constObjects[0];
		trueObject  = &constObjects[1];
		falseObject = &constObjects[2];
		trueObject->b = true;
		falseObject->b = false;
	} else {
		nullObject  = &constObjects[0];
		trueObject  = &constObjects[1];
		falseObject = &constObjects[2];
	}

	nullObject->refCount = refCountForGlobal;
	trueObject->refCount = refCountForGlobal;
	falseObject->refCount = refCountForGlobal;
}

}
}

#endif
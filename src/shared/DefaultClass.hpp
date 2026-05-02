#ifndef DEFAULT_CLASS_HPP
#define DEFAULT_CLASS_HPP

#include <iostream>
#include "shared/Type.hpp"

namespace AutoLang {

class ACompiler;
struct AObject;

namespace DefaultClass {

constexpr ClassId intClassId = 0;
constexpr ClassId floatClassId = 1;
constexpr ClassId boolClassId = 2;
constexpr ClassId stringClassId = 3;
constexpr ClassId bytesClassId = 4;
constexpr ClassId nullClassId = 5;
constexpr ClassId anyClassId = 6;
constexpr ClassId voidClassId = 7;
constexpr ClassId functionClassId = 8;
constexpr ClassId exceptionClassId = 9;
constexpr ClassId arrayClassId = 10;
constexpr ClassId setClassId = 11;
constexpr ClassId mapClassId = 12;
extern AObject* nullObject;
extern AObject* trueObject;
extern AObject* falseObject;
constexpr uint32_t builtInObjectSize = 3;
constexpr uint32_t refCountForGlobal = 2'000'000;
void init(ACompiler& compiler);

}
}

#endif
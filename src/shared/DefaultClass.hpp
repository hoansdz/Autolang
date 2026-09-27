#ifndef DEFAULT_CLASS_HPP
#define DEFAULT_CLASS_HPP

#include <iostream>
#include "shared/Type.hpp"

namespace Autolang {

class ACompiler;
struct AObject;

namespace DefaultClass {

constexpr ClassId intClassId = 0;
constexpr ClassId floatClassId = 1;
constexpr ClassId boolClassId = 2;
constexpr ClassId charClassId = 3;
constexpr ClassId stringClassId = 4;
constexpr ClassId bytesClassId = 5;
constexpr ClassId nullClassId = 6;
constexpr ClassId anyClassId = 7;
constexpr ClassId voidClassId = 8;
constexpr ClassId functionClassId = 9;
constexpr ClassId exceptionClassId = 10;
constexpr ClassId arrayClassId = 11;
constexpr ClassId setClassId = 12;
constexpr ClassId mapClassId = 13;
constexpr ClassId jsonClassId = 15;
constexpr ClassId jsObjectClassId = 16;
constexpr ClassId pyObjectClassId = 16;
extern AObject* nullObject;
extern AObject* trueObject;
extern AObject* falseObject;
constexpr uint32_t builtInObjectSize = 3;
constexpr uint32_t refCountForGlobal = 2'000'000;
void init(ACompiler& compiler);

}
}

#endif
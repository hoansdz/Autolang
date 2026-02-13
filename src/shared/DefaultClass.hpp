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
constexpr ClassId nullClassId = 4;
constexpr ClassId anyClassId = 5;
constexpr ClassId voidClassId = 6;
constexpr ClassId exceptionClassId = 7;
extern AObject* nullObject;
extern AObject* trueObject;
extern AObject* falseObject;
constexpr uint32_t builtInObjectSize = 3;
constexpr uint32_t refCountForGlobal = 2'000'000;
void init(ACompiler& compiler);

}
}

#endif
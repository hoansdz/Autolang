#ifndef DEFAULT_CLASS_HPP
#define DEFAULT_CLASS_HPP

#include <iostream>

struct CompiledProgram;
struct AObject;

namespace AutoLang {
namespace DefaultClass {

constexpr uint32_t intClassId = 0;
constexpr uint32_t floatClassId = 1;
constexpr uint32_t boolClassId = 2;
constexpr uint32_t stringClassId = 3;
constexpr uint32_t nullClassId = 4;
constexpr uint32_t anyClassId = 5;
extern AObject* nullObject;
extern AObject* trueObject;
extern AObject* falseObject;
constexpr uint32_t builtInObjectSize = 3;
constexpr uint32_t refCountForGlobal = 2'000'000;
void init(CompiledProgram& compile);

}
}

#endif
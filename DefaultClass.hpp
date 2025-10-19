#ifndef DEFAULT_CLASS_HPP
#define DEFAULT_CLASS_HPP

#include <iostream>

struct CompiledProgram;
struct AObject;

namespace AutoLang {
namespace DefaultClass {

constexpr uint32_t INTCLASSID = 0;
constexpr uint32_t FLOATCLASSID = 1;
extern uint32_t boolClassId;
extern uint32_t stringClassId;
extern uint32_t nullClassId;
extern uint32_t anyClassId;
extern AObject* nullObject;
extern AObject* trueObject;
extern AObject* falseObject;
void init(CompiledProgram& compile);

}
}

#endif
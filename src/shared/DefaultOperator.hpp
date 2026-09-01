#ifndef DEFAULT_OPERATOR_HPP
#define DEFAULT_OPERATOR_HPP

#include "backend/vm/ANotifier.hpp"
#include "shared/DefaultClass.hpp"
#include "shared/DefaultFunction.hpp"

namespace Autolang {
namespace DefaultFunction {

AObject *plus_plus(NativeFuncInData);
AObject *minus_minus(NativeFuncInData);
AObject *plus(NativeFuncInData);
AObject *plus_eq(NativeFuncInData);
AObject *minus(NativeFuncInData);
AObject *minus_eq(NativeFuncInData);
AObject *mul(NativeFuncInData);
AObject *mul_eq(NativeFuncInData);
AObject *divide(NativeFuncInData);
AObject *divide_eq(NativeFuncInData);
AObject *mod(NativeFuncInData);
AObject *mod_eq(NativeFuncInData);
AObject *bitwise_and(NativeFuncInData);
AObject *bitwise_or(NativeFuncInData);
AObject *negative(NativeFuncInData);
AObject *op_not(NativeFuncInData);
AObject *op_and_and(NativeFuncInData);
AObject *op_or_or(NativeFuncInData);
AObject *op_less_than(NativeFuncInData);
AObject *op_greater_than(NativeFuncInData);
AObject *op_less_than_eq(NativeFuncInData);
AObject *op_greater_than_eq(NativeFuncInData);
AObject *op_eqeq(NativeFuncInData);
AObject *op_not_eq(NativeFuncInData);
AObject *op_eq_pointer(NativeFuncInData);
AObject *op_not_eq_pointer(NativeFuncInData);
bool op_eqeq(AObject *obj1, AObject *obj2);

} // namespace DefaultFunction
} // namespace Autolang

#endif

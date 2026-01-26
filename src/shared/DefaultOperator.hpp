#ifndef DEFAULT_OPERATOR_HPP
#define DEFAULT_OPERATOR_HPP

#include "frontend/parser/Debugger.hpp"
#include "shared/DefaultClass.hpp"
#include <cmath>
#include <iostream>

namespace AutoLang {
namespace DefaultFunction {

inline AObject *plus_plus(NativeFuncInData);
inline AObject *minus_minus(NativeFuncInData);
inline AObject *plus(NativeFuncInData);
inline AObject *plus_eq(NativeFuncInData);
inline AObject *minus(NativeFuncInData);
inline AObject *minus_eq(NativeFuncInData);
inline AObject *mul(NativeFuncInData);
inline AObject *mul_eq(NativeFuncInData);
inline AObject *divide(NativeFuncInData);
inline AObject *divide_eq(NativeFuncInData);
inline AObject *mod(NativeFuncInData);
inline AObject *bitwise_and(NativeFuncInData);
inline AObject *bitwise_or(NativeFuncInData);
inline AObject *negative(NativeFuncInData);
inline AObject *op_not(NativeFuncInData);
inline AObject *op_and_and(NativeFuncInData);
inline AObject *op_or_or(NativeFuncInData);
inline AObject *op_less_than(NativeFuncInData);
inline AObject *op_greater_than(NativeFuncInData);
inline AObject *op_less_than_eq(NativeFuncInData);
inline AObject *op_greater_than_eq(NativeFuncInData);
inline AObject *op_eqeq(NativeFuncInData);
inline AObject *op_not_eq(NativeFuncInData);
inline AObject *op_eq_pointer(NativeFuncInData);
inline AObject *op_not_eq_pointer(NativeFuncInData);

AObject *plus(NativeFuncInData) {
  if (size != 2)
    return nullptr;
  auto obj1 = stackAllocator[0];
  auto obj2 = stackAllocator[1];
  switch (obj1->type) {
  case AutoLang::DefaultClass::intClassId: {
    switch (obj2->type) {
    case AutoLang::DefaultClass::intClassId:
      return manager.create((obj1->i) + (obj2->i));
    case AutoLang::DefaultClass::floatClassId:
      return manager.create((obj1->i) + (obj2->f));
    default:
      if (obj2->type == AutoLang::DefaultClass::stringClassId)
        return manager.create(AString::plus((obj1->i), (obj2->str)));
      else
        break;
    }
    break;
  }
  case AutoLang::DefaultClass::floatClassId: {
    switch (obj2->type) {
    case AutoLang::DefaultClass::intClassId:
      return manager.create((obj1->f) + (obj2->i));
    case AutoLang::DefaultClass::floatClassId:
      return manager.create((obj1->f) + (obj2->f));
    default:
      if (obj2->type == AutoLang::DefaultClass::stringClassId)
        return manager.create(AString::plus((obj1->f), (obj2->str)));
      else
        break;
    }
    break;
  }
  case AutoLang::DefaultClass::stringClassId: {
    switch (obj2->type) {
    case AutoLang::DefaultClass::intClassId:
      return manager.create((*obj1->str) + (obj2->i));
    case AutoLang::DefaultClass::floatClassId:
      return manager.create((*obj1->str) + (obj2->f));
    case AutoLang::DefaultClass::stringClassId:
      return manager.create((*obj1->str) + (obj2->str));
    default:
      break;
    }
  }
  default:
    throw std::runtime_error("Cannot plus two class");
    break;
  }
  return nullptr;
}

#define create_operator_number(name, op)                                       \
  AObject *name(NativeFuncInData) {                                            \
    if (size != 2)                                                             \
      return nullptr;                                                          \
    auto obj1 = stackAllocator[0];                                             \
    switch (obj1->type) {                                                      \
    case AutoLang::DefaultClass::intClassId: {                                 \
      auto obj2 = stackAllocator[1];                                           \
      switch (obj2->type) {                                                    \
      case AutoLang::DefaultClass::intClassId:                                 \
        return manager.create((obj1->i)op(obj2->i));                           \
      case AutoLang::DefaultClass::floatClassId:                               \
        return manager.create((obj1->i)op(obj2->f));                           \
      default:                                                                 \
        break;                                                                 \
      }                                                                        \
      break;                                                                   \
    }                                                                          \
    case AutoLang::DefaultClass::floatClassId: {                               \
      auto obj2 = stackAllocator[1];                                           \
      switch (obj2->type) {                                                    \
      case AutoLang::DefaultClass::intClassId:                                 \
        return manager.create((obj1->f)op(obj2->i));                           \
      case AutoLang::DefaultClass::floatClassId:                               \
        return manager.create((obj1->f)op(obj2->f));                           \
      default:                                                                 \
        break;                                                                 \
      }                                                                        \
      break;                                                                   \
    } break;                                                                   \
    }                                                                          \
    return nullptr;                                                            \
  }

#define create_operator_eq_value(name, op)                                     \
  AObject *name(NativeFuncInData) {                                            \
    if (size != 2)                                                             \
      return nullptr;                                                          \
    auto obj1 = stackAllocator[0];                                             \
    switch (obj1->type) {                                                      \
    case AutoLang::DefaultClass::intClassId: {                                 \
      auto obj2 = stackAllocator[1];                                           \
      switch (obj2->type) {                                                    \
      case AutoLang::DefaultClass::intClassId:                                 \
        return manager.create((obj1->i)op(obj2->i));                           \
      case AutoLang::DefaultClass::floatClassId:                               \
        return manager.create((obj1->i)op(obj2->f));                           \
      default:                                                                 \
        break;                                                                 \
      }                                                                        \
      break;                                                                   \
    }                                                                          \
    case AutoLang::DefaultClass::floatClassId: {                               \
      auto obj2 = stackAllocator[1];                                           \
      switch (obj2->type) {                                                    \
      case AutoLang::DefaultClass::intClassId:                                 \
        return manager.create((obj1->f)op(obj2->i));                           \
      case AutoLang::DefaultClass::floatClassId:                               \
        return manager.create((obj1->f)op(obj2->f));                           \
      default:                                                                 \
        break;                                                                 \
      }                                                                        \
      break;                                                                   \
    } break;                                                                   \
    }                                                                          \
    auto obj2 = stackAllocator[1];                                             \
    if (obj1->type == AutoLang::DefaultClass::boolClassId &&                   \
        obj2->type == AutoLang::DefaultClass::boolClassId) {                   \
      return manager.create((obj1->b)op(obj2->b));                             \
    }                                                                          \
    if (obj1->type == AutoLang::DefaultClass::stringClassId &&                 \
        obj2->type == AutoLang::DefaultClass::stringClassId) {                 \
      return manager.create((*obj1->str)op(obj2->str));                        \
    }                                                                          \
    return nullptr;                                                            \
  }

AObject *mod(NativeFuncInData) {
  auto obj1 = stackAllocator[0];
  auto obj2 = stackAllocator[1];
  switch (obj1->type) {
  case AutoLang::DefaultClass::intClassId: {
    switch (obj2->type) {
    case AutoLang::DefaultClass::intClassId:
      return manager.create((obj1->i) % (obj2->i));
    case AutoLang::DefaultClass::floatClassId:
      return manager.create(
          static_cast<double>(std::fmod((obj1->i), (obj2->f))));
    default:
      break;
    }
    break;
  }
  case AutoLang::DefaultClass::floatClassId: {
    switch (obj2->type) {
    case AutoLang::DefaultClass::intClassId:
      return manager.create(
          static_cast<double>(std::fmod((obj1->f), (obj2->i))));
    case AutoLang::DefaultClass::floatClassId:
      return manager.create(
          static_cast<double>(std::fmod((obj1->f), (obj2->f))));
    default:
      break;
    }
    break;
  }
  default:
    break;
  }
  throw std::runtime_error("Cannot mod 2 variable");
}

AObject *bitwise_and(NativeFuncInData) {
  auto obj1 = stackAllocator[0];
  auto obj2 = stackAllocator[1];
  switch (obj1->type) {
  case AutoLang::DefaultClass::intClassId: {
    switch (obj2->type) {
    case AutoLang::DefaultClass::intClassId:
      return manager.create((obj1->i) & (obj2->i));
    default:
      break;
    }
    break;
  }
  default:
    break;
  }
  throw std::runtime_error("Cannot bitwise and 2 variable");
}

AObject *bitwise_or(NativeFuncInData) {
  auto obj1 = stackAllocator[0];
  auto obj2 = stackAllocator[1];
  switch (obj1->type) {
  case AutoLang::DefaultClass::intClassId: {
    switch (obj2->type) {
    case AutoLang::DefaultClass::intClassId:
      return manager.create((obj1->i) | (obj2->i));
    default:
      break;
    }
    break;
  }
  default:
    break;
  }
  throw std::runtime_error("Cannot bitwise and 2 variable");
}

create_operator_number(minus, -);
create_operator_number(mul, *);
create_operator_number(divide, /);

#define create_operator_func_plus_equal(name, op)                              \
  AObject *name(NativeFuncInData) {                                            \
    if (size != 2)                                                             \
      return nullptr;                                                          \
    auto obj1 = stackAllocator[0];                                             \
    auto obj2 = stackAllocator[1];                                             \
    switch (obj1->type) {                                                      \
    case AutoLang::DefaultClass::intClassId: {                                 \
      switch (obj2->type) {                                                    \
      case AutoLang::DefaultClass::intClassId:                                 \
        obj1->i op obj2->i;                                                    \
        return nullptr;                                                        \
      case AutoLang::DefaultClass::floatClassId:                               \
        obj1->i op obj2->f;                                                    \
        return nullptr;                                                        \
      default:                                                                 \
        obj1->i op obj2->b;                                                    \
        return nullptr;                                                        \
      }                                                                        \
      break;                                                                   \
    }                                                                          \
    case AutoLang::DefaultClass::floatClassId: {                               \
      switch (obj2->type) {                                                    \
      case AutoLang::DefaultClass::intClassId:                                 \
        obj1->f op obj2->i;                                                    \
        return nullptr;                                                        \
      case AutoLang::DefaultClass::floatClassId:                               \
        obj1->f op obj2->f;                                                    \
        return nullptr;                                                        \
      default:                                                                 \
        obj1->f op obj2->b;                                                    \
        return nullptr;                                                        \
      }                                                                        \
      break;                                                                   \
    }                                                                          \
    default: {                                                                 \
      return nullptr;                                                          \
    }                                                                          \
    }                                                                          \
    return nullptr;                                                            \
  }

#define create_operator_func_number_equal(name, op)                            \
  AObject *name(NativeFuncInData) {                                            \
    if (size != 2)                                                             \
      return nullptr;                                                          \
    auto obj1 = stackAllocator[0];                                             \
    auto obj2 = stackAllocator[1];                                             \
    switch (obj1->type) {                                                      \
    case AutoLang::DefaultClass::intClassId: {                                 \
      switch (obj2->type) {                                                    \
      case AutoLang::DefaultClass::intClassId:                                 \
        obj1->i op obj2->i;                                                    \
        return nullptr;                                                        \
      case AutoLang::DefaultClass::floatClassId:                               \
        obj1->i op obj2->f;                                                    \
        return nullptr;                                                        \
      default:                                                                 \
        obj1->i op obj2->b;                                                    \
        return nullptr;                                                        \
      }                                                                        \
      break;                                                                   \
    }                                                                          \
    case AutoLang::DefaultClass::floatClassId: {                               \
      switch (obj2->type) {                                                    \
      case AutoLang::DefaultClass::intClassId:                                 \
        obj1->f op obj2->i;                                                    \
        return nullptr;                                                        \
      case AutoLang::DefaultClass::floatClassId:                               \
        obj1->f op obj2->f;                                                    \
        return nullptr;                                                        \
      default:                                                                 \
        obj1->f op obj2->b;                                                    \
        return nullptr;                                                        \
      }                                                                        \
      break;                                                                   \
    }                                                                          \
    }                                                                          \
    return nullptr;                                                            \
  }

create_operator_func_plus_equal(plus_eq, +=);
create_operator_func_number_equal(minus_eq, -=);
create_operator_func_number_equal(mul_eq, *=);
create_operator_func_number_equal(divide_eq, /=);

AObject *plus_plus(NativeFuncInData) {
  auto obj = stackAllocator[0];
  switch (obj->type) {
  case AutoLang::DefaultClass::intClassId:
    ++obj->i;
    break;
  case AutoLang::DefaultClass::floatClassId:
    ++obj->f;
    break;
  default:
    throw std::runtime_error("Cannot plus plus ");
  }
  return obj;
}

AObject *minus_minus(NativeFuncInData) {
  auto obj = stackAllocator[0];
  switch (obj->type) {
  case AutoLang::DefaultClass::intClassId:
    --obj->i;
    break;
  case AutoLang::DefaultClass::floatClassId:
    --obj->f;
    break;
  default:
    throw std::runtime_error("Cannot plus plus ");
  }
  return obj;
}

AObject *negative(NativeFuncInData) {
  auto obj = stackAllocator[0];
  switch (obj->type) {
  case AutoLang::DefaultClass::intClassId:
    return manager.create(-obj->i);
  case AutoLang::DefaultClass::floatClassId:
    return manager.create(-obj->f);
  default:
    if (obj->type == AutoLang::DefaultClass::boolClassId)
      return manager.create(-static_cast<int64_t>(obj->b));
    throw std::runtime_error("Cannot negative");
  }
}

AObject *op_not(NativeFuncInData) {
  auto obj = stackAllocator[0];
  if (obj->type == AutoLang::DefaultClass::boolClassId)
    return manager.createBoolObject(!obj->b);
  if (obj->type == AutoLang::DefaultClass::nullClassId)
    return manager.createBoolObject(true);
  throw std::runtime_error("Cannot use not oparator");
}

AObject *op_and_and(NativeFuncInData) {
  return ObjectManager::createBoolObject(stackAllocator[0]->b &&
                                         stackAllocator[1]->b);
}

AObject *op_or_or(NativeFuncInData) {
  return ObjectManager::createBoolObject(stackAllocator[0]->b ||
                                         stackAllocator[1]->b);
}

AObject *op_eq_pointer(NativeFuncInData) {
  return ObjectManager::createBoolObject(stackAllocator[0] ==
                                         stackAllocator[1]);
}

AObject *op_not_eq_pointer(NativeFuncInData) {
  return ObjectManager::createBoolObject(stackAllocator[0] !=
                                         stackAllocator[1]);
}

create_operator_number(op_less_than, <);
create_operator_number(op_greater_than, >);
create_operator_number(op_less_than_eq, <=);
create_operator_number(op_greater_than_eq, >=);
create_operator_eq_value(op_eqeq, ==);
create_operator_eq_value(op_not_eq, !=);

} // namespace DefaultFunction
} // namespace AutoLang

#endif
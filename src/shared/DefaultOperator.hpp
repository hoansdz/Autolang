#ifndef DEFAULT_OPERATOR_HPP
#define DEFAULT_OPERATOR_HPP

#include "frontend/parser/Debugger.hpp"
#include "shared/DefaultClass.hpp"
#include "backend/vm/ANotifier.hpp"
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
	auto obj1 = args[0];
	auto obj2 = args[1];
	switch (obj1->type) {
		case AutoLang::DefaultClass::intClassId: {
			switch (obj2->type) {
				case AutoLang::DefaultClass::intClassId:
					return notifier.createInt((obj1->i) + (obj2->i));
				case AutoLang::DefaultClass::floatClassId:
					return notifier.createFloat((obj1->i) + (obj2->f));
				case AutoLang::DefaultClass::boolClassId:
					return notifier.createInt((obj1->i) + (obj2->b));
				case AutoLang::DefaultClass::stringClassId:
					return notifier.createString(
					    AString::plus((obj1->i), (obj2->str)));
				default:
					break;
			}
			break;
		}
		case AutoLang::DefaultClass::floatClassId: {
			switch (obj2->type) {
				case AutoLang::DefaultClass::intClassId:
					return notifier.createFloat((obj1->f) + (obj2->i));
				case AutoLang::DefaultClass::floatClassId:
					return notifier.createFloat((obj1->f) + (obj2->f));
				case AutoLang::DefaultClass::boolClassId:
					return notifier.createFloat((obj1->f) + (obj2->b));
				case AutoLang::DefaultClass::stringClassId:
					return notifier.createString(
					    AString::plus((obj1->f), (obj2->str)));
				default:
					break;
			}
			break;
		}
		case AutoLang::DefaultClass::boolClassId: {
			switch (obj2->type) {
				case AutoLang::DefaultClass::intClassId:
					return notifier.createInt((obj1->b) + (obj2->i));
				case AutoLang::DefaultClass::floatClassId:
					return notifier.createFloat((obj1->b) + (obj2->f));
				case AutoLang::DefaultClass::boolClassId:
					return notifier.createInt((obj1->b) + (obj2->b));
				case AutoLang::DefaultClass::stringClassId:
					return notifier.createString(AString::plus(
					    (obj1->b ? "true" : "false"), (obj2->str)));
				default:
					break;
			}
			break;
		}
		case AutoLang::DefaultClass::stringClassId: {
			switch (obj2->type) {
				case AutoLang::DefaultClass::intClassId:
					return notifier.createString((*obj1->str) + (obj2->i));
				case AutoLang::DefaultClass::floatClassId:
					return notifier.createString((*obj1->str) + (obj2->f));
				case AutoLang::DefaultClass::boolClassId:
					return notifier.createString((*obj1->str) +
					                             (obj2->b ? "true" : "false"));
				case AutoLang::DefaultClass::stringClassId:
					return notifier.createString((*obj1->str) + (obj2->str));
				default:
					break;
			}
		}
		default:
			break;
	}
	notifier.throwException(
	    "Cannot plus " + notifier.vm->data.classes[obj1->type]->name + " and " +
	    notifier.vm->data.classes[obj2->type]->name);
	return nullptr;
}

AObject *minus(NativeFuncInData) {
	auto obj1 = args[0];
	auto obj2 = args[1];
	switch (obj1->type) {
		case AutoLang::DefaultClass::intClassId: {
			switch (obj2->type) {
				case AutoLang::DefaultClass::intClassId:
					return notifier.createInt((obj1->i) - (obj2->i));
				case AutoLang::DefaultClass::floatClassId:
					return notifier.createFloat((obj1->i) - (obj2->f));
				case AutoLang::DefaultClass::boolClassId:
					return notifier.createInt((obj1->i) - (obj2->b));
				default:
					break;
			}
			break;
		}
		case AutoLang::DefaultClass::floatClassId: {
			switch (obj2->type) {
				case AutoLang::DefaultClass::intClassId:
					return notifier.createFloat((obj1->f) - (obj2->i));
				case AutoLang::DefaultClass::floatClassId:
					return notifier.createFloat((obj1->f) - (obj2->f));
				case AutoLang::DefaultClass::boolClassId:
					return notifier.createFloat((obj1->f) - (obj2->b));
				default:
					break;
			}
			break;
		}
		case AutoLang::DefaultClass::boolClassId: {
			switch (obj2->type) {
				case AutoLang::DefaultClass::intClassId:
					return notifier.createInt((obj1->b) - (obj2->i));
				case AutoLang::DefaultClass::floatClassId:
					return notifier.createFloat((obj1->b) - (obj2->f));
				case AutoLang::DefaultClass::boolClassId:
					return notifier.createInt((obj1->b) - (obj2->b));
				default:
					break;
			}
			break;
		}
		default:
			break;
	}
	notifier.throwException(
	    "Cannot minus " + notifier.vm->data.classes[obj1->type]->name +
	    " and " + notifier.vm->data.classes[obj2->type]->name);
	return nullptr;
}

AObject *mul(NativeFuncInData) {
	auto obj1 = args[0];
	auto obj2 = args[1];
	switch (obj1->type) {
		case AutoLang::DefaultClass::intClassId: {
			switch (obj2->type) {
				case AutoLang::DefaultClass::intClassId:
					return notifier.createInt((obj1->i) * (obj2->i));
				case AutoLang::DefaultClass::floatClassId:
					return notifier.createFloat((obj1->i) * (obj2->f));
				case AutoLang::DefaultClass::boolClassId:
					return notifier.createInt((obj1->i) * (obj2->b));
				default:
					break;
			}
			break;
		}
		case AutoLang::DefaultClass::floatClassId: {
			switch (obj2->type) {
				case AutoLang::DefaultClass::intClassId:
					return notifier.createFloat((obj1->f) * (obj2->i));
				case AutoLang::DefaultClass::floatClassId:
					return notifier.createFloat((obj1->f) * (obj2->f));
				case AutoLang::DefaultClass::boolClassId:
					return notifier.createFloat((obj1->f) * (obj2->b));
				default:
					break;
			}
			break;
		}
		case AutoLang::DefaultClass::boolClassId: {
			switch (obj2->type) {
				case AutoLang::DefaultClass::intClassId:
					return notifier.createInt((obj1->b) * (obj2->i));
				case AutoLang::DefaultClass::floatClassId:
					return notifier.createFloat((obj1->b) * (obj2->f));
				case AutoLang::DefaultClass::boolClassId:
					return notifier.createInt((obj1->b) * (obj2->b));
				default:
					break;
			}
			break;
		}
		default:
			break;
	}
	notifier.throwException(
	    "Cannot multiply " + notifier.vm->data.classes[obj1->type]->name +
	    " and " + notifier.vm->data.classes[obj2->type]->name);
	return nullptr;
}

AObject *divide(NativeFuncInData) {
	auto obj1 = args[0];
	auto obj2 = args[1];
	switch (obj1->type) {
		case AutoLang::DefaultClass::intClassId: {
			switch (obj2->type) {
				case AutoLang::DefaultClass::intClassId: {
					if (obj2->i == 0)
						goto divideByZero;
					return notifier.createInt((obj1->i) / (obj2->i));
				}
				case AutoLang::DefaultClass::floatClassId: {
					if (obj2->f == 0)
						goto divideByZero;
					return notifier.createFloat((obj1->i) / (obj2->f));
				}
				case AutoLang::DefaultClass::boolClassId: {
					if (obj2->b == false)
						goto divideByZero;
					return notifier.createInt((obj1->i) / (obj2->b));
				}
				default:
					break;
			}
			break;
		}
		case AutoLang::DefaultClass::floatClassId: {
			switch (obj2->type) {
				case AutoLang::DefaultClass::intClassId: {
					if (obj2->i == 0)
						goto divideByZero;
					return notifier.createFloat((obj1->f) / (obj2->i));
				}
				case AutoLang::DefaultClass::floatClassId: {
					if (obj2->f == 0)
						goto divideByZero;
					return notifier.createFloat((obj1->f) / (obj2->f));
				}
				case AutoLang::DefaultClass::boolClassId: {
					if (obj2->b == false)
						goto divideByZero;
					return notifier.createFloat((obj1->f) / (obj2->b));
				}
				default:
					break;
			}
			break;
		}
		case AutoLang::DefaultClass::boolClassId: {
			switch (obj2->type) {
				case AutoLang::DefaultClass::intClassId: {
					if (obj2->i == 0)
						goto divideByZero;
					return notifier.createInt((obj1->b) / (obj2->i));
				}
				case AutoLang::DefaultClass::floatClassId: {
					if (obj2->f == 0)
						goto divideByZero;
					return notifier.createFloat((obj1->b) / (obj2->f));
				}
				case AutoLang::DefaultClass::boolClassId: {
					if (obj2->b == false)
						goto divideByZero;
					return notifier.createInt((obj1->b) / (obj2->b));
				}
				default:
					break;
			}
			break;
		}
		default:
			break;
	}
	notifier.throwException(
	    "Cannot divide " + notifier.vm->data.classes[obj1->type]->name +
	    " and " + notifier.vm->data.classes[obj2->type]->name);
	return nullptr;
divideByZero:;
	notifier.throwException("Cannot divide by zero");
	return nullptr;
}

AObject *op_eqeq(NativeFuncInData) {
    auto obj1 = args[0];
    auto obj2 = args[1];
    switch (obj1->type) {
        case AutoLang::DefaultClass::intClassId: {
            switch (obj2->type) {
                case AutoLang::DefaultClass::intClassId:
                    return notifier.createBool(obj1->i == obj2->i);
                case AutoLang::DefaultClass::floatClassId:
                    return notifier.createBool(obj1->i == obj2->f);
                case AutoLang::DefaultClass::boolClassId:
                    return notifier.createBool(obj1->i == obj2->b);
                default: break;
            }
            break;
        }
        case AutoLang::DefaultClass::floatClassId: {
            switch (obj2->type) {
                case AutoLang::DefaultClass::intClassId:
                    return notifier.createBool(obj1->f == obj2->i);
                case AutoLang::DefaultClass::floatClassId:
                    return notifier.createBool(obj1->f == obj2->f);
                case AutoLang::DefaultClass::boolClassId:
                    return notifier.createBool(obj1->f == obj2->b);
                default: break;
            }
            break;
        }
        case AutoLang::DefaultClass::boolClassId: {
            switch (obj2->type) {
                case AutoLang::DefaultClass::intClassId:
                    return notifier.createBool(obj1->b == obj2->i);
                case AutoLang::DefaultClass::floatClassId:
                    return notifier.createBool(obj1->b == obj2->f);
                case AutoLang::DefaultClass::boolClassId:
                    return notifier.createBool(obj1->b == obj2->b);
                default: break;
            }
            break;
        }
        case AutoLang::DefaultClass::stringClassId: {
            if (obj2->type == AutoLang::DefaultClass::stringClassId) {
                return notifier.createBool(*obj1->str == obj2->str);
            }
            break;
        }
        default: break;
    }
    return notifier.createBool(false);
}

AObject *op_not_eq(NativeFuncInData) {
    auto obj1 = args[0];
    auto obj2 = args[1];
    switch (obj1->type) {
        case AutoLang::DefaultClass::intClassId: {
            switch (obj2->type) {
                case AutoLang::DefaultClass::intClassId:
                    return notifier.createBool(obj1->i != obj2->i);
                case AutoLang::DefaultClass::floatClassId:
                    return notifier.createBool(obj1->i != obj2->f);
                case AutoLang::DefaultClass::boolClassId:
                    return notifier.createBool(obj1->i != obj2->b);
                default: break;
            }
            break;
        }
        case AutoLang::DefaultClass::floatClassId: {
            switch (obj2->type) {
                case AutoLang::DefaultClass::intClassId:
                    return notifier.createBool(obj1->f != obj2->i);
                case AutoLang::DefaultClass::floatClassId:
                    return notifier.createBool(obj1->f != obj2->f);
                case AutoLang::DefaultClass::boolClassId:
                    return notifier.createBool(obj1->f != obj2->b);
                default: break;
            }
            break;
        }
        case AutoLang::DefaultClass::boolClassId: {
            switch (obj2->type) {
                case AutoLang::DefaultClass::intClassId:
                    return notifier.createBool(obj1->b != obj2->i);
                case AutoLang::DefaultClass::floatClassId:
                    return notifier.createBool(obj1->b != obj2->f);
                case AutoLang::DefaultClass::boolClassId:
                    return notifier.createBool(obj1->b != obj2->b);
                default: break;
            }
            break;
        }
         case AutoLang::DefaultClass::stringClassId: {
            if (obj2->type == AutoLang::DefaultClass::stringClassId) {
                return notifier.createBool(*obj1->str != obj2->str);
            }
            break;
        }
        default: break;
    }
    return notifier.createBool(true);
}

AObject *op_less_than(NativeFuncInData) {
    auto obj1 = args[0];
    auto obj2 = args[1];
    switch (obj1->type) {
        case AutoLang::DefaultClass::intClassId: {
            switch (obj2->type) {
                case AutoLang::DefaultClass::intClassId:
                    return notifier.createBool(obj1->i < obj2->i);
                case AutoLang::DefaultClass::floatClassId:
                    return notifier.createBool(obj1->i < obj2->f);
                case AutoLang::DefaultClass::boolClassId:
                    return notifier.createBool(obj1->i < obj2->b);
                default: break;
            }
            break;
        }
        case AutoLang::DefaultClass::floatClassId: {
            switch (obj2->type) {
                case AutoLang::DefaultClass::intClassId:
                    return notifier.createBool(obj1->f < obj2->i);
                case AutoLang::DefaultClass::floatClassId:
                    return notifier.createBool(obj1->f < obj2->f);
                case AutoLang::DefaultClass::boolClassId:
                    return notifier.createBool(obj1->f < obj2->b);
                default: break;
            }
            break;
        }
        case AutoLang::DefaultClass::boolClassId: {
            switch (obj2->type) {
                case AutoLang::DefaultClass::intClassId:
                    return notifier.createBool(obj1->b < obj2->i);
                case AutoLang::DefaultClass::floatClassId:
                    return notifier.createBool(obj1->b < obj2->f);
                case AutoLang::DefaultClass::boolClassId:
                    return notifier.createBool(obj1->b < obj2->b);
                default: break;
            }
            break;
        }
        default: break;
    }
    notifier.throwException(
        "Cannot compare < between " + notifier.vm->data.classes[obj1->type]->name +
        " and " + notifier.vm->data.classes[obj2->type]->name);
    return nullptr;
}

AObject *op_greater_than(NativeFuncInData) {
    auto obj1 = args[0];
    auto obj2 = args[1];
    switch (obj1->type) {
        case AutoLang::DefaultClass::intClassId: {
            switch (obj2->type) {
                case AutoLang::DefaultClass::intClassId:
                    return notifier.createBool(obj1->i > obj2->i);
                case AutoLang::DefaultClass::floatClassId:
                    return notifier.createBool(obj1->i > obj2->f);
                case AutoLang::DefaultClass::boolClassId:
                    return notifier.createBool(obj1->i > obj2->b);
                default: break;
            }
            break;
        }
        case AutoLang::DefaultClass::floatClassId: {
            switch (obj2->type) {
                case AutoLang::DefaultClass::intClassId:
                    return notifier.createBool(obj1->f > obj2->i);
                case AutoLang::DefaultClass::floatClassId:
                    return notifier.createBool(obj1->f > obj2->f);
                case AutoLang::DefaultClass::boolClassId:
                    return notifier.createBool(obj1->f > obj2->b);
                default: break;
            }
            break;
        }
        case AutoLang::DefaultClass::boolClassId: {
            switch (obj2->type) {
                case AutoLang::DefaultClass::intClassId:
                    return notifier.createBool(obj1->b > obj2->i);
                case AutoLang::DefaultClass::floatClassId:
                    return notifier.createBool(obj1->b > obj2->f);
                case AutoLang::DefaultClass::boolClassId:
                    return notifier.createBool(obj1->b > obj2->b);
                default: break;
            }
            break;
        }
        default: break;
    }
    notifier.throwException(
        "Cannot compare > between " + notifier.vm->data.classes[obj1->type]->name +
        " and " + notifier.vm->data.classes[obj2->type]->name);
    return nullptr;
}

AObject *op_less_than_eq(NativeFuncInData) {
    auto obj1 = args[0];
    auto obj2 = args[1];
    switch (obj1->type) {
        case AutoLang::DefaultClass::intClassId: {
            switch (obj2->type) {
                case AutoLang::DefaultClass::intClassId:
                    return notifier.createBool(obj1->i <= obj2->i);
                case AutoLang::DefaultClass::floatClassId:
                    return notifier.createBool(obj1->i <= obj2->f);
                case AutoLang::DefaultClass::boolClassId:
                    return notifier.createBool(obj1->i <= obj2->b);
                default: break;
            }
            break;
        }
        case AutoLang::DefaultClass::floatClassId: {
            switch (obj2->type) {
                case AutoLang::DefaultClass::intClassId:
                    return notifier.createBool(obj1->f <= obj2->i);
                case AutoLang::DefaultClass::floatClassId:
                    return notifier.createBool(obj1->f <= obj2->f);
                case AutoLang::DefaultClass::boolClassId:
                    return notifier.createBool(obj1->f <= obj2->b);
                default: break;
            }
            break;
        }
        case AutoLang::DefaultClass::boolClassId: {
            switch (obj2->type) {
                case AutoLang::DefaultClass::intClassId:
                    return notifier.createBool(obj1->b <= obj2->i);
                case AutoLang::DefaultClass::floatClassId:
                    return notifier.createBool(obj1->b <= obj2->f);
                case AutoLang::DefaultClass::boolClassId:
                    return notifier.createBool(obj1->b <= obj2->b);
                default: break;
            }
            break;
        }
        default: break;
    }
    notifier.throwException(
        "Cannot compare <= between " + notifier.vm->data.classes[obj1->type]->name +
        " and " + notifier.vm->data.classes[obj2->type]->name);
    return nullptr;
}

AObject *op_greater_than_eq(NativeFuncInData) {
    auto obj1 = args[0];
    auto obj2 = args[1];
    switch (obj1->type) {
        case AutoLang::DefaultClass::intClassId: {
            switch (obj2->type) {
                case AutoLang::DefaultClass::intClassId:
                    return notifier.createBool(obj1->i >= obj2->i);
                case AutoLang::DefaultClass::floatClassId:
                    return notifier.createBool(obj1->i >= obj2->f);
                case AutoLang::DefaultClass::boolClassId:
                    return notifier.createBool(obj1->i >= obj2->b);
                default: break;
            }
            break;
        }
        case AutoLang::DefaultClass::floatClassId: {
            switch (obj2->type) {
                case AutoLang::DefaultClass::intClassId:
                    return notifier.createBool(obj1->f >= obj2->i);
                case AutoLang::DefaultClass::floatClassId:
                    return notifier.createBool(obj1->f >= obj2->f);
                case AutoLang::DefaultClass::boolClassId:
                    return notifier.createBool(obj1->f >= obj2->b);
                default: break;
            }
            break;
        }
        case AutoLang::DefaultClass::boolClassId: {
            switch (obj2->type) {
                case AutoLang::DefaultClass::intClassId:
                    return notifier.createBool(obj1->b >= obj2->i);
                case AutoLang::DefaultClass::floatClassId:
                    return notifier.createBool(obj1->b >= obj2->f);
                case AutoLang::DefaultClass::boolClassId:
                    return notifier.createBool(obj1->b >= obj2->b);
                default: break;
            }
            break;
        }
        default: break;
    }
    notifier.throwException(
        "Cannot compare >= between " + notifier.vm->data.classes[obj1->type]->name +
        " and " + notifier.vm->data.classes[obj2->type]->name);
    return nullptr;
}

AObject *mod(NativeFuncInData) {
	auto obj1 = args[0];
	auto obj2 = args[1];
	switch (obj1->type) {
		case AutoLang::DefaultClass::intClassId: {
			switch (obj2->type) {
				case AutoLang::DefaultClass::intClassId: {
					if (obj2->i == 0)
						goto divideByZero;
					return notifier.createInt((obj1->i) % (obj2->i));
				}
				case AutoLang::DefaultClass::floatClassId: {
					if (obj2->f == 0)
						goto divideByZero;
					return notifier.createFloat(fmod((obj1->i), (obj2->f)));
				}
				case AutoLang::DefaultClass::boolClassId: {
					if (obj2->b == false)
						goto divideByZero;
					return notifier.createInt(obj1->i);
				}
				default:
					break;
			}
			break;
		}
		case AutoLang::DefaultClass::floatClassId: {
			switch (obj2->type) {
				case AutoLang::DefaultClass::intClassId: {
					if (obj2->i == 0)
						goto divideByZero;
					return notifier.createFloat(fmod((obj1->f), (obj2->i)));
				}
				case AutoLang::DefaultClass::floatClassId: {
					if (obj2->f == 0)
						goto divideByZero;
					return notifier.createFloat(fmod((obj1->f), (obj2->f)));
				}
				case AutoLang::DefaultClass::boolClassId: {
					if (obj2->b == false)
						goto divideByZero;
					return notifier.createFloat(obj1->f);
				}
				default:
					break;
			}
			break;
		}
		case AutoLang::DefaultClass::boolClassId: {
			switch (obj2->type) {
				case AutoLang::DefaultClass::intClassId: {
					if (obj2->i == 0)
						goto divideByZero;
					return notifier.createInt((obj1->b) % (obj2->i));
				}
				case AutoLang::DefaultClass::floatClassId: {
					if (obj2->f == 0)
						goto divideByZero;
					return notifier.createFloat(fmod((obj1->b), (obj2->f)));
				}
				case AutoLang::DefaultClass::boolClassId: {
					if (obj2->b == false)
						goto divideByZero;
					return notifier.createInt((obj1->b) % (obj2->b));
				}
				default:
					break;
			}
			break;
		}
		default:
			break;
	}
	notifier.throwException(
	    "Cannot mod " + notifier.vm->data.classes[obj1->type]->name +
	    " and " + notifier.vm->data.classes[obj2->type]->name);
	return nullptr;
divideByZero:;
	notifier.throwException("Cannot divide by zero");
	return nullptr;
}

AObject *bitwise_and(NativeFuncInData) {
	auto obj1 = args[0];
	auto obj2 = args[1];
	switch (obj1->type) {
		case AutoLang::DefaultClass::intClassId: {
			switch (obj2->type) {
				case AutoLang::DefaultClass::intClassId:
					return notifier.createInt((obj1->i) & (obj2->i));
				default:
					break;
			}
			break;
		}
		default:
			break;
	}
	notifier.throwException(
	    "Cannot bitwise and " + notifier.vm->data.classes[obj1->type]->name +
	    " and " + notifier.vm->data.classes[obj2->type]->name);
	return nullptr;
}

AObject *bitwise_or(NativeFuncInData) {
	auto obj1 = args[0];
	auto obj2 = args[1];
	switch (obj1->type) {
		case AutoLang::DefaultClass::intClassId: {
			switch (obj2->type) {
				case AutoLang::DefaultClass::intClassId:
					return notifier.createInt((obj1->i) | (obj2->i));
				default:
					break;
			}
			break;
		}
		default:
			break;
	}
	notifier.throwException(
	    "Cannot bitwise or " + notifier.vm->data.classes[obj1->type]->name +
	    " and " + notifier.vm->data.classes[obj2->type]->name);
	return nullptr;
}

AObject *op_and_and(NativeFuncInData) {
    auto obj1 = args[0];
    auto obj2 = args[1];
    
    if (obj1->type == AutoLang::DefaultClass::boolClassId && 
        obj2->type == AutoLang::DefaultClass::boolClassId) {
        return notifier.createBool(obj1->b && obj2->b);
    }

    notifier.throwException(
        "Cannot use && between " + notifier.vm->data.classes[obj1->type]->name +
        " and " + notifier.vm->data.classes[obj2->type]->name);
    return nullptr;
}

AObject *op_or_or(NativeFuncInData) {
    auto obj1 = args[0];
    auto obj2 = args[1];

    if (obj1->type == AutoLang::DefaultClass::boolClassId && 
        obj2->type == AutoLang::DefaultClass::boolClassId) {
        return notifier.createBool(obj1->b || obj2->b);
    }

    notifier.throwException(
        "Cannot use || between " + notifier.vm->data.classes[obj1->type]->name +
        " and " + notifier.vm->data.classes[obj2->type]->name);
    return nullptr;
}

AObject *op_eq_pointer(NativeFuncInData) {
    return notifier.createBool(args[0] == args[1]);
}

AObject *op_not_eq_pointer(NativeFuncInData) {
    return notifier.createBool(args[0] != args[1]);
}

AObject *plus_eq(NativeFuncInData) {
    auto obj1 = args[0];
    auto obj2 = args[1];
    switch (obj1->type) {
        case AutoLang::DefaultClass::intClassId: {
            switch (obj2->type) {
                case AutoLang::DefaultClass::intClassId:
                    obj1->i += obj2->i; return nullptr;
                case AutoLang::DefaultClass::floatClassId:
                    obj1->i += obj2->f; return nullptr;
                case AutoLang::DefaultClass::boolClassId:
                    obj1->i += obj2->b; return nullptr;
                default: break;
            }
            break;
        }
        case AutoLang::DefaultClass::floatClassId: {
            switch (obj2->type) {
                case AutoLang::DefaultClass::intClassId:
                    obj1->f += obj2->i; return nullptr;
                case AutoLang::DefaultClass::floatClassId:
                    obj1->f += obj2->f; return nullptr;
                case AutoLang::DefaultClass::boolClassId:
                    obj1->f += obj2->b; return nullptr;
                default: break;
            }
            break;
        }
        default: break;
    }
    notifier.throwException(
        "Cannot use += between " + notifier.vm->data.classes[obj1->type]->name +
        " and " + notifier.vm->data.classes[obj2->type]->name);
    return nullptr;
}

AObject *minus_eq(NativeFuncInData) {
    auto obj1 = args[0];
    auto obj2 = args[1];
    switch (obj1->type) {
        case AutoLang::DefaultClass::intClassId: {
            switch (obj2->type) {
                case AutoLang::DefaultClass::intClassId:
                    obj1->i -= obj2->i; return nullptr;
                case AutoLang::DefaultClass::floatClassId:
                    obj1->i -= obj2->f; return nullptr;
                case AutoLang::DefaultClass::boolClassId:
                    obj1->i -= obj2->b; return nullptr;
                default: break;
            }
            break;
        }
        case AutoLang::DefaultClass::floatClassId: {
            switch (obj2->type) {
                case AutoLang::DefaultClass::intClassId:
                    obj1->f -= obj2->i; return nullptr;
                case AutoLang::DefaultClass::floatClassId:
                    obj1->f -= obj2->f; return nullptr;
                case AutoLang::DefaultClass::boolClassId:
                    obj1->f -= obj2->b; return nullptr;
                default: break;
            }
            break;
        }
        default: break;
    }
    notifier.throwException(
        "Cannot use -= between " + notifier.vm->data.classes[obj1->type]->name +
        " and " + notifier.vm->data.classes[obj2->type]->name);
    return nullptr;
}

AObject *mul_eq(NativeFuncInData) {
    auto obj1 = args[0];
    auto obj2 = args[1];
    switch (obj1->type) {
        case AutoLang::DefaultClass::intClassId: {
            switch (obj2->type) {
                case AutoLang::DefaultClass::intClassId:
                    obj1->i *= obj2->i; return nullptr;
                case AutoLang::DefaultClass::floatClassId:
                    obj1->i *= obj2->f; return nullptr;
                case AutoLang::DefaultClass::boolClassId:
                    obj1->i *= obj2->b; return nullptr;
                default: break;
            }
            break;
        }
        case AutoLang::DefaultClass::floatClassId: {
            switch (obj2->type) {
                case AutoLang::DefaultClass::intClassId:
                    obj1->f *= obj2->i; return nullptr;
                case AutoLang::DefaultClass::floatClassId:
                    obj1->f *= obj2->f; return nullptr;
                case AutoLang::DefaultClass::boolClassId:
                    obj1->f *= obj2->b; return nullptr;
                default: break;
            }
            break;
        }
        default: break;
    }
    notifier.throwException(
        "Cannot use *= between " + notifier.vm->data.classes[obj1->type]->name +
        " and " + notifier.vm->data.classes[obj2->type]->name);
    return nullptr;
}

AObject *divide_eq(NativeFuncInData) {
    auto obj1 = args[0];
    auto obj2 = args[1];
    switch (obj1->type) {
        case AutoLang::DefaultClass::intClassId: {
            switch (obj2->type) {
                case AutoLang::DefaultClass::intClassId:
                    if (obj2->i == 0) goto divideByZero;
                    obj1->i /= obj2->i; return nullptr;
                case AutoLang::DefaultClass::floatClassId:
                    if (obj2->f == 0) goto divideByZero;
                    obj1->i /= obj2->f; return nullptr;
                case AutoLang::DefaultClass::boolClassId:
                    if (obj2->b == false) goto divideByZero;
                    obj1->i /= obj2->b; return nullptr;
                default: break;
            }
            break;
        }
        case AutoLang::DefaultClass::floatClassId: {
            switch (obj2->type) {
                case AutoLang::DefaultClass::intClassId:
                    if (obj2->i == 0) goto divideByZero;
                    obj1->f /= obj2->i; return nullptr;
                case AutoLang::DefaultClass::floatClassId:
                    if (obj2->f == 0) goto divideByZero;
                    obj1->f /= obj2->f; return nullptr;
                case AutoLang::DefaultClass::boolClassId:
                    if (obj2->b == false) goto divideByZero;
                    obj1->f /= obj2->b; return nullptr;
                default: break;
            }
            break;
        }
        default: break;
    }
    notifier.throwException(
        "Cannot use /= between " + notifier.vm->data.classes[obj1->type]->name +
        " and " + notifier.vm->data.classes[obj2->type]->name);
    return nullptr;

divideByZero:;
    notifier.throwException("Cannot divide by zero in /= operation");
    return nullptr;
}

AObject *plus_plus(NativeFuncInData) {
    auto obj = args[0];
    switch (obj->type) {
        case AutoLang::DefaultClass::intClassId:
            ++obj->i;
            return obj;
        case AutoLang::DefaultClass::floatClassId:
            ++obj->f;
            return obj;
        default: break;
    }
    notifier.throwException(
        "Cannot use ++ operator on " + notifier.vm->data.classes[obj->type]->name);
    return nullptr;
}

AObject *minus_minus(NativeFuncInData) {
    auto obj = args[0];
    switch (obj->type) {
        case AutoLang::DefaultClass::intClassId:
            --obj->i;
            return obj;
        case AutoLang::DefaultClass::floatClassId:
            --obj->f;
            return obj;
        default: break;
    }
    notifier.throwException(
        "Cannot use -- operator on " + notifier.vm->data.classes[obj->type]->name);
    return nullptr;
}

AObject *negative(NativeFuncInData) {
    auto obj = args[0];
    switch (obj->type) {
        case AutoLang::DefaultClass::intClassId:
            return notifier.createInt(-obj->i);
        case AutoLang::DefaultClass::floatClassId:
            return notifier.createFloat(-obj->f);
        case AutoLang::DefaultClass::boolClassId:
            return notifier.createInt(-static_cast<int64_t>(obj->b));
        default: break;
    }
    notifier.throwException(
        "Cannot use negative operator on " + notifier.vm->data.classes[obj->type]->name);
    return nullptr;
}

AObject *op_not(NativeFuncInData) {
    auto obj = args[0];
    if (obj->type == AutoLang::DefaultClass::boolClassId) {
        return notifier.createBool(!obj->b);
    }
    notifier.throwException(
        "Cannot use ! operator on " + notifier.vm->data.classes[obj->type]->name);
    return nullptr;
}

} // namespace DefaultFunction
} // namespace AutoLang

#endif
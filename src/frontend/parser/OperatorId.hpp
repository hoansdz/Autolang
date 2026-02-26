#ifndef OPERATOR_ID_HPP
#define OPERATOR_ID_HPP

#include <iostream>

namespace AutoLang {

enum OperatorId : uint8_t {
    OP_PLUS_PLUS = 0,    // ++
    OP_MINUS_MINUS,      // --

    OP_PLUS,             // +
    OP_CAL_EQ,           // += (Khớp với plus_eq)
    OP_MINUS,            // -
    OP_MINUS_EQ,         // -=
    OP_MUL,              // *
    OP_MUL_EQ,           // *=
    OP_DIV,              // /
    OP_DIV_EQ,           // /=

    OP_MOD,              // %

    OP_BIT_AND,          // &
    OP_BIT_OR,           // |

    OP_NEGATIVE,         // unary -
    OP_NOT,              // !

    OP_AND_AND,          // &&
    OP_OR_OR,            // ||

    OP_LESS,             // <
    OP_GREATER,          // >
    OP_LESS_EQ,          // <=
    OP_GREATER_EQ,       // >=

    OP_EQEQ,             // ==
    OP_NOT_EQ,           // !=

    OP_EQ_POINTER,       // ===
    OP_NOT_EQ_POINTER,   // !==
};

}

#endif
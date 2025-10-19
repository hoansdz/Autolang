#ifndef DEBUGGER_LOOP_HPP
#define DEBUGGER_LOOP_HPP

#include "Debugger.hpp"

namespace AutoLang
{

ExprNode* loadFor(in_func, size_t& i);
WhileNode* loadWhile(in_func, size_t& i);

}

#endif
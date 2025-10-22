#ifndef DEBUGGER_FUNCTION_HPP
#define DEBUGGER_FUNCTION_HPP

#include "Debugger.hpp"

namespace AutoLang
{

CreateFuncNode* loadFunc(in_func, size_t& i);
ReturnNode* loadReturn(in_func, size_t& i);

}

#endif
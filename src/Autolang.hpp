#ifndef AUTOLANG_HPP
#define AUTOLANG_HPP

#include "./vm/Interpreter.cpp"
#include "Debugger.cpp"
#include "./vm/AObject.cpp"
#include "./vm/CompiledProgram.cpp"
#include "DebuggerConditionStatement.cpp"
#include "DebuggerClass.cpp"
#include "DebuggerDeclaration.cpp"
#include "DebuggerFunction.cpp"
#include "DebuggerLoop.cpp"
#include "DefaultClass.cpp"
#include "DefaultFunction.cpp"
#include "OptimizeNode.cpp"
#include "Lexer.cpp"
#include "Node.cpp"
#include "CreateNode.cpp"
#include "NodeOptimize.cpp"
#include "NodePutbytecode.cpp"
#include "ParserContext.cpp"
#include "./vm/StackAllocator.cpp"
#include "./vm/ObjectManager.cpp"
#include "./vm/AreaAllocator.cpp"
#include "libs/Math.cpp"

#endif
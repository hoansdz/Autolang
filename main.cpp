#include <iostream>
#include "Interpreter.cpp"
#include "Debugger.cpp"
#include "AObject.cpp"
#include "CompiledProgram.cpp"
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
#include "StackAllocator.cpp"
#include "libs/Math.cpp"

int main(int argc, char *argv[])
{
	try{
		AVM i = AVM("source.txt");
	}
	catch (const std::exception& e) {
		std::cerr<<e.what();
	}
}
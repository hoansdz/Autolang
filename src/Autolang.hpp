#ifndef AUTOLANG_HPP
#define AUTOLANG_HPP

#ifndef NO_INCLUDE_LIBS_FILE
#include "frontend/libs/file.cpp"
#endif
#ifndef NO_INCLUDE_LIBS_DATE
#include "frontend/libs/date.cpp"
#endif
#ifndef NO_INCLUDE_LIBS_REGEX
#include "frontend/libs/regex.cpp"
#endif
#ifndef NO_INCLUDE_LIBS_JSON
#include "frontend/libs/json.cpp"
#endif
#ifndef NO_INCLUDE_LIBS_HTTP
#include "frontend/libs/http.cpp"
#endif
#include "backend/libs/array.cpp"
#include "backend/libs/map.cpp"
#include "backend/libs/set.cpp"
#include "backend/vm/ANotifier.cpp"
#include "backend/vm/AVM.cpp"
#if defined(__GNUC__) || defined(__clang__)
#define AUTOLANG_USE_COMPUTED_GOTO_
#endif
#ifdef AUTOLANG_USE_COMPUTED_GOTO
#include "backend/vm/AVM_run_computed_goto.cpp"
#else
#include "backend/vm/AVM_run_switch.cpp"
#endif
#include "backend/vm/AVMLoader.cpp"
#include "backend/vm/AVMLog.cpp"
#include "frontend/ACompiler.cpp"
#include "frontend/ACompiler.hpp"
#include "frontend/lexer/Lexer.cpp"
#include "frontend/libs/bytes.cpp"
#include "frontend/libs/math.cpp"
#include "frontend/libs/stdlib.cpp"
#include "frontend/libs/time.cpp"
#include "frontend/libs/vm.cpp"
#include "frontend/parser/ClassDeclaration.cpp"
#include "frontend/parser/ClassInfo.cpp"
#include "frontend/parser/Debugger.cpp"
#include "frontend/parser/DebuggerAnnotations.cpp"
#include "frontend/parser/DebuggerClass.cpp"
#include "frontend/parser/DebuggerConditionStatement.cpp"
#include "frontend/parser/DebuggerDeclaration.cpp"
#include "frontend/parser/DebuggerEnum.cpp"
#include "frontend/parser/DebuggerFunction.cpp"
#include "frontend/parser/DebuggerGeneric.cpp"
#include "frontend/parser/DebuggerImport.cpp"
#include "frontend/parser/DebuggerLoop.cpp"
#include "frontend/parser/DebuggerTryCatch.cpp"
#include "frontend/parser/DebuggerWhen.cpp"
#include "frontend/parser/FunctionInfo.cpp"
#include "frontend/parser/Parameter.cpp"
#include "frontend/parser/ParserContext.cpp"
#include "frontend/parser/node/BinaryNode.cpp"
#include "frontend/parser/node/BlockNode.cpp"
#include "frontend/parser/node/CallNode.cpp"
#include "frontend/parser/node/CastNode.cpp"
#include "frontend/parser/node/ConstValueNode.cpp"
#include "frontend/parser/node/CreateArrayNode.cpp"
#include "frontend/parser/node/CreateClosureNode.cpp"
#include "frontend/parser/node/CreateFuncNode.cpp"
#include "frontend/parser/node/CreateMapNode.cpp"
#include "frontend/parser/node/CreateNode.cpp"
#include "frontend/parser/node/CreateSetNode.cpp"
#include "frontend/parser/node/ForNode.cpp"
#include "frontend/parser/node/FunctionAccessNode.cpp"
#include "frontend/parser/node/GetPointerNode.cpp"
#include "frontend/parser/node/GetPropNode.cpp"
#include "frontend/parser/node/IfNode.cpp"
#include "frontend/parser/node/Node.cpp"
#include "frontend/parser/node/NodeOptimize.cpp"
#include "frontend/parser/node/NodePutBytecode.cpp"
#include "frontend/parser/node/NullCoalescingNode.cpp"
#include "frontend/parser/node/OptimizeNode.cpp"
#include "frontend/parser/node/OptionalAccessNode.cpp"
#include "frontend/parser/node/RangeNode.cpp"
#include "frontend/parser/node/SetNode.cpp"
#include "frontend/parser/node/ThrowNode.cpp"
#include "frontend/parser/node/TryCatchNode.cpp"
#include "frontend/parser/node/UnaryNode.cpp"
#include "frontend/parser/node/VarNode.cpp"
#include "frontend/parser/node/WhenNode.cpp"
#include "shared/AreaAllocator.cpp"
#include "shared/CompiledProgram.cpp"
#include "shared/DefaultClass.cpp"
#include "shared/DefaultFunction.cpp"
#include "shared/ObjectManager.cpp"
#include "shared/StackAllocator.cpp"

#endif
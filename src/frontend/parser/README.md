# Syntax Analyzer, Semantic Checking, & AST (`src/frontend/parser`)

This directory contains the components responsible for parsing the token stream into an Abstract Syntax Tree (AST), checking static semantics (types, scopes, declarations), and preparing the AST for bytecode generation.

## Subdirectories

- **[node](file:///d:/code/AutoLangC/src/frontend/parser/node)**: Houses the definition and specific implementations of all AST Nodes representing Autolang expressions, statements, and declarations.

---

## Core Compiler Frontend State & Semantic Checker

- **`ParserContext.cpp` / `ParserContext.hpp`**: Manages the state of the parser, compiling errors, constant pools, local scopes, and compiler warning messages.
- **`Debugger.cpp` / `Debugger.hpp`**: The main static semantic analyzer. It performs type inference, validates declarations, enforces scope visibility rules, and verifies type compatibility.
- **`Debugger*.cpp` files**: Specific sub-modules of the semantic checker, specializing in validating different language features:
  - `DebuggerAnnotations.cpp`: Validates compile-time metadata/annotations.
  - `DebuggerClass.cpp` / `DebuggerEnum.cpp`: Verifies class hierarchies, member accesses, and enum configurations.
  - `DebuggerDeclaration.cpp`: Validates variable declarations (`val`, `var`) and static types.
  - `DebuggerFunction.cpp` / `DebuggerGeneric.cpp`: Resolves method overloads, parameter validation, and generic type parameters/instantiations.
  - `DebuggerLoop.cpp` / `DebuggerTryCatch.cpp` / `DebuggerWhen.cpp` / `DebuggerConditionStatement.cpp`: Checks correctness of loops, try-catch handlers, match statements, and conditional branches.
- **`ClassDeclaration.cpp` / `ClassDeclaration.hpp`**: Structures representing declared classes, interfaces, generic parameters, and member field offsets.
- **`FunctionInfo.cpp` / `FunctionInfo.hpp`**: Stores function signatures, flags (static, native, etc.), parameters, and return types.
- **`GenericCaller.cpp` / `GenericCaller.hpp` / `GenericData.hpp`**: Utilities for matching, substituting, and instantiating generic parameters.
- **`Parameter.cpp` / `Parameter.hpp`**: Manages formal parameter declarations and default arguments.
- **`OperatorId.hpp`**: Maps language operators to internal identifiers.

---

## AST Nodes System (`src/frontend/parser/node`)

Every syntactic construct is represented by an AST Node inheriting from the base `ExprNode` defined in **[Node.hpp](file:///d:/code/AutoLangC/src/frontend/parser/node/Node.hpp)**.

### Base Classes (`Node.hpp`)
- **`ExprNode`**: The root of the AST hierarchy. Declares virtual methods for:
  - `resolve()`: Performs contextual type checking and AST transformation.
  - `optimize()`: Implements compile-time evaluation (constant folding, dead code elimination).
  - `putBytecodes()`: Converts the node directly into VM instructions.
  - `rewrite()`: Modifies jump targets or variable offsets dynamically during compilation.
- **`HasClassIdNode`**: Base node for expressions that produce a typed value at runtime.
- **`NullableNode`**: Extends typed nodes with nullability metadata.
- **`AccessNode`**: Base class for variables and properties that can be read or written.

### AST Node Types and Files
The **[node/](file:///d:/code/AutoLangC/src/frontend/parser/node)** folder contains specific implementations of nodes:

- **`ConstValueNode.cpp`**: Represents literals such as integers, floats, booleans, strings, or `null`.
- **`VarNode.cpp`**: Resolves local variable access and tracking.
- **`BinaryNode.cpp`**: Handles binary expressions (`+`, `-`, `*`, `/`, `%`, `==`, `&&`, `||`, etc.) and short-circuit evaluation.
- **`UnaryNode.cpp`**: Unary expressions like negation (`-`), logical NOT (`!`), etc.
- **`BlockNode.cpp`**: Sequential compound statements enclosed in braces.
- **`IfNode.cpp`**: Represents conditional expressions/statements.
- **`ForNode.cpp`** / **`RangeNode.cpp`**: Handles `for-in` loop constructs and range operations (`from..to`, `from..<to`).
- **`WhenNode.cpp`**: Pattern matching conditional switch (`when`).
- **`CallNode.cpp`**: Handles function, method, and constructor invocations, supporting overload resolution and generic parameters.
- **`CreateClosureNode.cpp`**: Represents lambdas/closures capturing surrounding variables.
- **`CreateFuncNode.cpp`** / **`CreateNode.cpp`**: Defines functions, classes, constructors, and instance creations.
- **`CreateArrayNode.cpp`** / **`CreateSetNode.cpp`** / **`CreateMapNode.cpp`**: Supports collection literal instantiations.
- **`GetPropNode.cpp`** / **`SetNode.cpp`**: Handles field accesses (`object.field`) and assignments (`dest = value`).
- **`GetPointerNode.cpp`**: Handles pointer-of operations (`&var`).
- **`CastNode.cpp`** / **`CastNode.cpp`** / **`CastNode.cpp`**: Handles compile-time and runtime safe/unsafe casts (`value # Type` or `value as Type`).
- **`TryCatchNode.cpp`** / **`ThrowNode.cpp`**: Exception handling and propagation logic.
- **`OptionalAccessNode.cpp`** / **`NullCoalescingNode.cpp`**: Handles safe navigation (`?.`) and null-coalescing (`??`) operations.
- **`NodeOptimize.cpp`** / **`OptimizeNode.cpp`**: AST-level optimization passes.
- **`NodePutBytecode.cpp`**: Helper code-gen operations.

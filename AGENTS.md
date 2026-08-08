# AGENTS.md - Developer & Agent Guide for AutoLangC

This document guides AI Coding Agents (Cursor, Copilot, Antigravity, Claude Code, Windsurf) on how to understand, build, modify, and integrate **AutoLangC**.

## Project Context
- **Language Standard**: C++17 (`set(CMAKE_CXX_STANDARD 17)` required).
- **Core Architecture**:
  - `ACompiler`: Parses source text into AST and emits VM bytecode instructions.
  - `AVirtualMachine`: Stack-based execution engine with register frame support.
  - `ANotifier`: Handles VM exceptions, memory allocations, and Python interop runtime state.
  - `AObject`: Unified tagged union representing primitive types (`int64_t`, `double`, `std::string`, `bool`), generic objects (Array, Map, Set), and Python PyObject references.
- **Dependencies**: `pybind11` (Python interop), `libcurl` (HTTP requests, fetched via CMake `FetchContent`), `nlohmann_json` (JSON parsing).

## Code Style & Rules
1. **Memory Allocation**: Always use `ANotifier` for object creation (`createInt`, `createString`, `createMemberObject`) to ensure correct reference counting and GC tracking.
2. **Nullability Checks**: Verify nullable flags in generic types (`allGenericTypeNullable`, `allMemberNullable`) before unwrapping `AObject*`.
3. **C++ Standard**: Use modern C++17 constructs (`std::string_view`, `auto` structured bindings).
4. **Header Directives**: Include headers relative to `src/` (e.g., `#include "shared/PYFunction.hpp"`).

## Building & Testing Commands
- **Configure**: `cmake -B build -S .`
- **Build**: `cmake --build build --config Release`
- **Run Tests**: `./build/autolang` (or `build\Release\autolang.exe` on Windows).

## TypeScript / JS Integration Rules (`autolang-compiler`)
1. **Native Library Registration**: Use `compiler.registerBuiltInLibrary(name, headerSource, config, implementations)`.
2. **Object Wrapper (`@js_object`)**: Annotate host wrapper classes with `@js_object`. Native implementation methods MUST use standard ES6 method shorthand or `function` syntax (not arrow functions `() => {}`) so the VM can bind `this` to the native JS object instance.
3. **Parameter Order**: Native JS functions receive Autolang arguments sequentially. Instance methods receive the bound instance via `this`.

## Python & Native Library Interop Rules
1. **Parameter Binding**: Parameter mapping in native libraries mirrors TypeScript/JavaScript bindings. Non-static method calls pass the target instance as the 1st argument.
2. **Python `self` Convention**: In Python interop (via `pybind11`), non-static member functions or standalone native callbacks acting on objects MUST accept `self` as the first argument (`def my_func(self, *args)`), enabling the VM to bind the caller instance correctly.


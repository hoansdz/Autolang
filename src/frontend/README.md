# Frontend Directory (`src/frontend`)

This directory houses the frontend compiler suite of Autolang, responsible for converting raw source code into Compiled Program bytecode structure.

## Subdirectories

- **[lexer](file:///d:/code/AutoLangC/src/frontend/lexer)**: Lexical analyzer (tokenizer) converting source characters into token streams.
- **[libs](file:///d:/code/AutoLangC/src/frontend/libs)**: Implementation of built-in standard library components (HTTP, JSON, File IO, Regex, Time, Math, etc.) exposed to the language. (Array, Set, Map is special case, implemented in backend/libs/ to make VM can boost performance and convert other language objects to AObject more easily)
- **[parser](file:///d:/code/AutoLangC/src/frontend/parser)**: Syntax analyzer and AST generation components, along with static error analyzers.
- **[structure](file:///d:/code/AutoLangC/src/frontend/structure)**: Utility data structures specifically designed for frontend allocations.

## Key Files

- **`ACompiler.cpp` / `ACompiler.hpp`**: Main compiler driver class coordinating Lexer, Parser, AST generation, semantic resolution, and bytecode generation.

# Lexical Analyzer (`src/frontend/lexer`)

This directory contains the Lexer (tokenizer) responsible for scanning the source characters of Autolang and producing a stream of tokens.

## Key Files

- **`Lexer.cpp` / `Lexer.hpp`**: Scans characters, handles keywords, identifier recognition, string literals (including raw strings), number literals, and operators, producing a clean token stream for the parser.

# AutoLang Test Suite (`tests`)

This directory contains verification and diagnostic suites for checking the correct behavior, performance, and cross-platform builds of AutoLang.

## Subdirectories

- **[benchmark](file:///d:/code/AutoLangC/tests/benchmark)**: Evaluates execution performance and resource usage compared against other languages (e.g. Python, Lua).
- **[correctness](file:///d:/code/AutoLangC/tests/correctness)**: Tests validating syntax constructs, built-ins, and standard library behaviors.
- **[release](file:///d:/code/AutoLangC/tests/release)**: Release mode build verifications.
- **[wasm](file:///d:/code/AutoLangC/tests/wasm)**: Tests targeting WebAssembly (WASM) compiler runs and compatibility.

## Key Files

- **`main.cpp`**: Main entry point for running tests.
- **`add_program.py`**: Helper script to generate/register new test cases.
- **`*.atl` files**: Scratchpad AutoLang code files used for manual tests.

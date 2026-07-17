# Frontend Data Structures (`src/frontend/structure`)

This directory contains utility data structures used during the compilation phase to optimize memory usage or speed up compilation.

## Key Files

- **`NonReallocatePool.hpp`**: A memory allocation pool that guarantees pointers to its elements remain stable (i.e. not reallocated or shifted in memory) when new items are added, which is essential for AST and compiler metadata safety.

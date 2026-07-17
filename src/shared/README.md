# Shared Core Directory (`src/shared`)

This directory houses the common structures, data representations, memory allocators, and compiled binary layouts shared between the compiler frontend and the virtual machine backend.

## Key Components

### 1. Object Model & Type System
- **`AObject.hpp`**: Base representation of dynamic values in the runtime.
- **`AClass.hpp`**: Representation of a runtime class instance layout.
- **`AString.hpp`**: Custom string object.
- **`Type.hpp`**: Types system definition (integers, strings, functions, structures).
- **`ANativeFunctionData.hpp` / `JSFunction.hpp`**: Wrappers for native function binding and JS bridge.

### 2. Custom Memory Management
- **`AreaAllocator.cpp` / `AreaAllocator.hpp`**: High performance arena/region-based memory allocator.
- **`ChunkArena.hpp`**: Manages allocated memory blocks sequentially.
- **`FixedPool.hpp` / `FixedPoolLoaded.hpp`**: Fixed-size object pools for optimized allocations.
- **`StackAllocator.cpp` / `StackAllocator.hpp`**: Fast stack allocation routines.
- **`StringArena.hpp`**: Dedicated pool for string allocations.
- **`ObjectManager.cpp` / `ObjectManager.hpp`**: Tracks object lifetimes, reference counts, and garbage collection/cleanup phases.

### 3. Program Representation
- **`CompiledProgram.cpp` / `CompiledProgram.hpp`**: Defines the format of compiled bytecode, including constant pools, global variables, class declarations, and compiled function instructions.
- **`Bytecodes.hpp`**: Metadata about bytecode offsets and sizes.

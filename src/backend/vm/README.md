# Autolang Virtual Machine (`src/backend/vm`)

This directory contains the core runtime interpreter and execution manager for Autolang bytecode.

## Key Components

- **`AVM.cpp` / `AVM.hpp`**: The main virtual machine class, managing stack allocations, call frames, error boundaries, state, and execution options.
- **`AVM_run_switch.cpp`**: Bytecode interpreter execution loop using standard `switch-case` dispatching.
- **`AVM_run_computed_goto.cpp`**: Optimized interpreter execution loop using compiler-specific `computed goto` dispatching (direct threaded code).
- **`AVMLoader.cpp`**: Responsible for loading and linking CompiledProgram structures into the VM environment.
- **`AVMLog.cpp`**: Diagnostic logging utilities for tracking instructions and VM states.
- **`Opcode.hpp`**: Enumeration of all virtual machine bytecode instructions.
- **`ANotifier.cpp` / `ANotifier.hpp`**: Interface for VM state updates and event dispatching.

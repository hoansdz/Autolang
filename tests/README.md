# Autolang

> A statically typed scripting language and virtual machine for safely executing AI-generated code.

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Documentation](https://img.shields.io/badge/docs-online-blue)](https://autolang.vercel.app/docs)

Autolang is designed for one specific problem:

> **Allow AI to generate executable code without exposing your entire runtime.**

Instead of letting an LLM execute Python or JavaScript directly, you expose only the functions you choose. AI writes the workflow, while your existing backend performs the actual work.

---

## Why Autolang?

Modern LLMs are increasingly capable of generating code.

The challenge is not code generation—it is execution.

Running AI-generated Python or JavaScript means exposing a large runtime with unrestricted APIs, dynamic imports, filesystem access, networking, and unpredictable memory usage. Even with Docker or MicroVMs, every agent still carries the cost of a full runtime.

Autolang approaches the problem differently.

Instead of sandboxing an operating system, it sandboxes the language itself.

Scripts can only call APIs that you explicitly register.

```
AI
 │
 ▼
Autolang Compiler
 │
 ▼
Type Checking
 │
 ▼
Bytecode
 │
 ▼
Autolang VM
 │
 ▼
Registered JS / C++ Functions
```

This makes execution predictable, lightweight and suitable for large numbers of concurrent AI agents.

---

## Features

- Static type checking
- Custom bytecode virtual machine
- No GC
- No JIT
- Opcode execution limits
- Null safety
- Native JS bindings
- Native C++ bindings
- `@js_object` interoperability
- Per-library language restrictions
- Compile-time diagnostics
- Fast startup
- Small memory footprint

---

## When should you use it?

Autolang is a good fit if:

- your application lets AI generate code
- you need to control what AI can access
- your backend already exists
- scripts are short and executed frequently
- startup latency matters
- memory usage matters

Typical examples:

- AI Agents
- Internal automation
- Workflow engines
- Business rule execution
- Embedded scripting
- Multi-agent systems

---

## When should you NOT use it?

Autolang is **not** intended to replace Python, JavaScript or C++.

It is probably not the right choice if:

- you need a general-purpose language
- your programs are thousands of lines long
- you require unrestricted OS access
- your application does not execute AI-generated code

---

## Performance

Measured on:

- Windows 11
- Intel Core i5 12th Gen
- 16GB RAM

| Metric | Result |
|---------|--------|
| Native cold start | ~10 ms |
| Node.js cold start | ~20 ms |
| Warm execution | ~1–2 ms |
| Core runtime | ~0.5 MB (0 script line) |
| Full stdlib | ~0.61 MB (0 script line) |

Autolang optimizes total execution time:

```
Compile
      +
Execute
      =
Fast response
```

This is especially useful for AI-generated scripts, which are usually short and executed many times.

---

## Installation

### npm

```bash
npm install autolang-compiler
```

### Native

```bash
clang++ tests/main.cpp -O2 -std=c++17
```

Requires a C++17 compiler.

---

## Quick Example

Register a native function:

```ts
compiler.registerBuiltInLibrary("example", `
    @native("hello")
    fun hello(name: String): String
`, {}, {
    hello(name) {
        return "Hello " + name;
    }
});
```

Run a script:

```kotlin
@import("example")

println(hello("Autolang"))
```

Output:

```
Hello Autolang
```

The script cannot access anything except the APIs you registered.

---

## AI Agent Example

Instead of asking an LLM to repeatedly call tools:

```
LLM
 ↓
Tool
 ↓
LLM
 ↓
Tool
 ↓
LLM
```

Autolang allows the model to generate an entire workflow once:

```
LLM
 ↓
Autolang Script
 ↓
VM
 ↓
Registered APIs
```

This reduces:

- latency
- token usage
- repeated reasoning
- unnecessary API round trips

while keeping execution inside a restricted environment.

---
## Language

Autolang uses a Kotlin-inspired syntax designed to be easy for both developers and LLMs.

### Variables

```kotlin
val name = "Autolang"
var count = 10
```

### Null safety

```kotlin
var user: User?

println(user?.name ?? "Unknown")
```

### Collections

```kotlin
val numbers = <Int>[1, 2, 3, 4]

val even = numbers.filter {|v| v % 2 == 0 }
```

### Classes

```kotlin
class Animal {
    fun sound() = "..."
}

class Cat extends Animal {
    @override
    fun sound() = "Meow"
}
```

More examples are available in the documentation.

---

## Native Bindings

Autolang does not replace your backend.

Instead, it allows you to expose existing functions to AI through native bindings.

```kotlin
@native("read_user")
fun readUser(id: Int): User
```

The implementation remains inside your application.

Scripts can only call the functions you explicitly register.

---

## JS Object Interoperability

Complex JavaScript objects can be wrapped using `@js_object`.

This allows AI-generated scripts to use fluent APIs while the actual object remains entirely on the host side.

```kotlin
@js_object
class QueryBuilder {

    @native("where")
    fun where(field: String, value: String): QueryBuilder

    @native("execute")
    fun execute(): Array<Order>

}
```

Example:

```kotlin
Database.createQuery()
    .where("status", "completed")
    .execute()
```

This makes existing ORMs and query builders accessible without exposing JavaScript itself.

---

## Memory Model

Autolang uses:

- Reference Counting
- Hot Restart

Instead of relying on a garbage collector, memory is reset after each script execution, providing predictable execution costs and consistent latency.

---

## Security Model

Autolang assumes AI-generated code is untrusted.

Security is enforced before and during execution.

Built-in protections include:

- Static type checking
- Restricted language features
- Opcode execution limits
- Managed memory limits
- Registered APIs only
- Disabled filesystem by default
- Disabled networking by default
- Domain allowlists
- File path allowlists
- Per-library permissions

Autolang is a language-level sandbox.

It complements—but does not replace—OS-level isolation when stronger security guarantees are required.

---

## Documentation

Documentation includes:

- [Getting Started](https://autolang.vercel.app/docs)
- [Philosophy & Vision](https://autolang.vercel.app/docs/philosophy-vision)
- [Language Guide](https://autolang.vercel.app/docs/language-guide/syntax)
- [Standard Library](https://autolang.vercel.app/docs/language-guide/standard-library)
- [Native Bindings](https://autolang.vercel.app/docs/integration-npm)
- [Security](https://autolang.vercel.app/docs/integration-npm/security-sandbox)
- [Examples](https://autolang.vercel.app/docs/integration-npm/examples)
- [API Reference](https://autolang.vercel.app/docs/integration-npm/type-reference)
- [Live Playground](https://autolang.vercel.app/docs/editor)

---

## Roadmap

Current development focuses on:

- Better error messages
- Additional standard library modules
- Performance optimizations

---

## Contributing

Contributions are welcome.

If you discover a bug or have an idea for improving Autolang, feel free to open an issue or submit a pull request.

Please read the documentation before contributing to understand the project architecture and design philosophy.

---

## License

MIT License © 2026 Autolang Project
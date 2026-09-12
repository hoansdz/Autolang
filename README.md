# Autolang

> A safe, embeddable scripting language with Kotlin-like syntax, designed for strict execution boundaries and seamless host integration.

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Documentation](https://img.shields.io/badge/docs-online-blue)](https://autolang.vercel.app/docs)

---

## Contents

- [Why Autolang?](#why-autolang)
- [How it works](#how-it-works)
- [Language at a glance](#language-at-a-glance)
- [Quickstart](#quickstart)
- [Real-world Workflow](#real-world-workflow)
- [Why not tool calling?](#why-not-tool-calling)
- [When to use (and when not to)](#when-to-use)
- [Security & Resource Governance](#security--resource-governance)
- [Standard Library](#standard-library)
- [Documentation](#documentation)
- [Roadmap](#roadmap)
- [Sponsors](#sponsors)
- [License](#license)

---

- Kotlin-compatible syntax: Familiar language constructs with static typing and compile-time verification.
- Safe execution sandbox: Scripts can only invoke functions and capabilities explicitly exposed by the host.
- Resource protection: Automatically prevents runaway loops and reclaims all memory upon script completion.
- Multi-platform embedding: Embed directly into TypeScript, JavaScript, C++, and Python hosts.

---

## Language at a glance

**Null safety and expressions:**

```kotlin
var result: String? = null
val display = result?.uppercase() ?? "Default Value"
println(display)
```

**Pattern matching:**

```kotlin
val status = when (code) {
    200 -> "Success"
    404 -> "Not Found"
    else -> "Unknown"
}
```

**Collections and closures:**

```kotlin
val numbers = arrayOf(1, 2, 3, 4, 5, 6)
val evenSquares = numbers
    .filter { n -> n % 2 == 0 }
    .map { n -> n * n }

println(evenSquares) // [4, 16, 36]
```

**Error handling:**

```kotlin
try {
    val result = processData()
    println(result)
} catch (e) {
    println("Error encountered: " + e)
}
```

---

## Quickstart

### Installation

Via npm (TypeScript / JavaScript):

```bash
npm install autolang-compiler
```

Via native C++ build:

```bash
cmake -B build -S .
cmake --build build --config Release
```

### Basic Usage (TypeScript)

Expose host functions and execute a script:

```typescript
import { ACompiler } from "autolang-compiler";

const compiler = await ACompiler.create();

// Register a host capability
compiler.registerBuiltInLibrary("system/notification", `
    @native("notify")
    fun notify(recipient: String, message: String): Bool
`, {}, {
    notify(recipient, message) {
        console.log(`Sending notice to ${recipient}: ${message}`);
        return true;
    }
});

// Run script logic
await compiler.compileAndRun("workflow.atl", `
    @import("system/notification")

    val success = notify("Operations", "Task finished")
    println(success)
`);
```

### Basic Usage (C++)

```cpp
#include <Autolang.hpp>

int main() {
    Autolang::ACompiler compiler;

    bool ok = compiler.compileAndRun("script.atl", R"(
        val items = arrayOf("A", "B", "C")
        items.forEach { item -> println(item) }
    )");

    return ok ? 0 : 1;
}
```

---

## Standard Library

Autolang includes a focused set of built-in types and utilities:

| Module | Description |
|---|---|
| `Array<T>` | Ordered collection with `filter`, `map`, `forEach`, `sort`, `find` |
| `Set<T>` | Unique-element collection with membership testing |
| `Map<K, V>` | Key-value dictionary with iteration support |
| `String` | Substring, split, replace, trim, casing, and string interpolation |
| `Math` | Standard arithmetic functions (`min`, `max`, `round`, `floor`, `ceil`, `sqrt`, `pow`) |
| `Json` | Serialization and deserialization of structured data |
| `Regex` | Pattern matching and extraction |
| `Date` / `Time` | Timestamp generation, formatting, and duration arithmetic |

---

## Testing

Run the test suite to verify the local build:

```bash
# Run the complete test suite
./build/autolang tests/testCorrectness.atl

# Run an individual test file
./build/autolang tests/correctness/basic/generics.atl
```

---

## Documentation

Full documentation and language specifications are available at [autolang.vercel.app/docs](https://autolang.vercel.app/docs):

- [Getting Started](https://autolang.vercel.app/docs/introduction)
- [Language Reference & Syntax](https://autolang.vercel.app/docs/language-guide/syntax)
- [Host Integration Guide](https://autolang.vercel.app/docs/integration-npm)
- [Interactive Playground](https://autolang.vercel.app/docs/editor)

---

## Sponsors

Autolang is sponsored by:

<p align="left">
  <a href="https://adagroup.com.vn/" target="_blank" rel="noopener noreferrer">
    <img src="assets/sponsor-logo.jpg" alt="ADA GROUP" height="48" />
  </a>
</p>

Special thanks to **[ADA GROUP](https://adagroup.com.vn/)** for supporting project development.

---

## License

MIT License (c) 2026 Autolang Project
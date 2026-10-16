# Autolang

A statically typed, embeddable scripting language and VM engineered for validating and executing AI-generated workflows against explicitly granted host capabilities.

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Status: Beta](https://img.shields.io/badge/Status-Beta%20(v0.8.x)-orange)](https://github.com/hoansdz/Autolang)
[![Documentation](https://img.shields.io/badge/docs-online-blue)](https://autolang.vercel.app/docs)
[![npm version](https://img.shields.io/npm/v/autolang-compiler.svg)](https://www.npmjs.com/package/autolang-compiler)

[Documentation](https://autolang.vercel.app/docs) · [Getting Started](https://autolang.vercel.app/docs/introduction) · [Interactive Playground](https://autolang.vercel.app/docs/editor)

---

## Why Autolang?

When software agents run multi-step workflows (charging accounts, updating databases, sending notifications), traditional execution strategies have critical flaws:

- **Multi-turn Tool Calling**: Slow network round-trips for every step. If step 3 fails on a hallucinated argument, steps 1 and 2 have already executed and mutated external state.
- **Dynamic Scripting (JS / Python / Lua)**: Sequential runtime execution. If a typo or wrong argument type occurs on line 3, earlier lines have already made irreversible changes.
- **Autolang (Compile First, Execute Second)**: The entire script is type-checked and compiled into bytecode in **< 1 ms**. Statically detectable errors reject the program before execution starts. **Zero host capabilities are invoked, and no partial mutations occur.**

```kotlin
// Example: Multi-step capability workflow
val customer = findCustomer("cust_102")
val invoice = createInvoice(customer.id, 250.0)
reserveInventory(invoice.itemSku, invoice.quantity)
sendReceipt(customer.email, invoice.id)
```

---

## 30-Second Demo: Compile-Time Diagnostics

Suppose the host exposes a capability:

```typescript
compiler.registerBuiltInLibrary("analytics", `
    @native("calculateMetrics")
    fun calculateMetrics(period: String): Int
`, { autoImport: true }, {
    calculateMetrics(period) { return 42; }
});
```

If the model produces a script with a typo (`calculateMetrcs`):

```kotlin
@import("analytics")
val result = calculateMetrcs("quarterly")
```

**Autolang Compiler Output:**

```text
workflow.atl:2: Cannot find function name 'calculateMetrcs'
Did you mean: 'calculateMetrics'?
Hint: Verify the function name is spelled correctly and declared or imported in current scope.
```

- **Execution**: Blocked immediately (`compileAndRun` returns `false`).
- **Side effects**: **Zero.**
- **Self-healing**: The compiler outputs exact fuzzy suggestions (`Did you mean...?`) and type signatures so the model can auto-correct on the next turn.

---

## How Autolang Compares

| Solution | Compile-Time Check | Latency | Default-Deny Sandbox | Opcode & Memory Quotas | Syntax |
|---|---|---|---|---|---|
| **Autolang** | **Full AST & Types** | **< 1 ms (In-process)** | **Built-in** | **Built-in** | **Kotlin-like (closures, pipelines)** |
| **TypeScript (Go `tsc` + Isolate)** | Full Types | 30 ms – 150 ms | Requires custom hardening | Requires V8 isolate hooks | JS / TS |
| **Embedded Python / Lua / QuickJS** | No (Runtime only) | 1 ms – 5 ms | Requires manual stripping | Varies | Python / Lua / JS |
| **Starlark / CEL** | Partial | < 5 ms | Built-in | Limited | Restrictive (no closures/pipelines) |
| **Multi-Turn Tool Calling** | No (Fails mid-run) | Hundreds of ms / turn | Prompt-governed | Token limit only | JSON tool calls |

- **Why not TypeScript + V8 Isolate?** Even with Microsoft's Go rewrite of `tsc`, type-checking and execution remain decoupled across two heavy systems. Hardening JavaScript runtimes against prototype pollution (`Object.prototype`) is fragile. Autolang compiles and executes inside a single in-process C++ engine with built-in opcode metering.
- **Why not Lua / QuickJS?** Dynamically typed engines cannot catch missing functions or invalid argument types before line 1 begins executing external side effects.
- **Why not Starlark?** Starlark is designed for build configs (Bazel) and lacks modern workflow ergonomics like closures, rich collection pipelines (`filter`/`map`), and static null safety.

---

## Three Layers of Defense

1. **Pre-Execution Validation**: Full AST analysis, static type-checking, and capability signature matching in < 1 ms.
2. **Explicit Capability Sandbox**: Strict default-deny architecture. Scripts have zero ambient access to the filesystem, network, or OS unless explicitly granted.
3. **Resource Governance**: Per-script opcode budgets (`maxOpcodeCount`), managed memory limits (`maxMemoryBytes`), and execution timeouts (`executionTimeoutMs`).

---

## LLM-Tolerant Syntax

Autolang incorporates deliberate compiler tolerances so minor generation habits don't break workflows:

- **Type Absorption**: Zero-cost typealiases transparently accept cross-language habits (`List<T>`, `ArrayList<T>`, `Vector<T>` → `Array<T>`; `HashMap<K, V>`, `dict` → `Map<K, V>`; `int`, `string`, `bool`, `Double`).
- **Method Aliasing**: Hallucinated `.push()` or `.append()` calls on arrays automatically normalize to `.add()` with a compiler warning.
- **EOF Recovery**: If an LLM response gets truncated by token limits, `autoCloseBracketsOnEof` balances unclosed braces to salvage valid blocks.
- **Import Normalization**: Automatically handles `import crm.*` by resolving via internal `@import("crm")`.

---

## Quickstart

### Installation

```bash
# npm (TypeScript / Node.js)
npm install autolang-compiler

# Python
pip install autolang
```

### TypeScript / Node.js

```typescript
import { ACompiler } from "autolang-compiler";

const compiler = await ACompiler.create({
    limits: { maxOpcodeCount: 500000, maxMemoryBytes: 16 * 1024 * 1024, executionTimeoutMs: 3000 }
});

// Register host capabilities
compiler.registerBuiltInLibrary("crm", `
    @native("findCustomer")
    fun findCustomer(id: String): String
`, { autoImport: true }, {
    findCustomer: (id) => JSON.stringify({ id, name: "Alice", tier: "enterprise" })
});

// Compile and run
const ok = await compiler.compileAndRun("workflow.atl", `
    @import("crm")
    val customer = findCustomer("cust_102")
    println(customer)
`);

console.log(compiler.getOutput());
```

### Python

```python
import autolang

compiler = autolang.ACompiler({
    "limits": {"maxOpcodeCount": 500000, "maxMemoryBytes": 16 * 1024 * 1024, "executionTimeoutMs": 3000}
})

compiler.registerBuiltInLibrary("crm", """
    @native("findCustomer")
    fun findCustomer(id: String): String
""", {"autoImport": True}, {
    "findCustomer": lambda cid: f'{{"id": "{cid}", "status": "active"}}'
})

ok = compiler.compileAndRun("workflow.atl", """
    @import("crm")
    val customer = findCustomer("cust_102")
    println(customer)
""")

print(compiler.getOutput())
```

---

## Language at a Glance

```kotlin
// Null safety & Elvis operator
var name: String? = null
val greeting = "Hello, " + (name ?? "Guest")

// Pattern matching
val status = when (code) {
    200 -> "Success"
    404 -> "Not Found"
    else -> "Unknown"
}

// Collections & functional pipelines
val numbers = arrayOf(1, 2, 3, 4, 5, 6)
val evenSquares = numbers.filter { n -> n % 2 == 0 }.map { n -> n * n }

// Defensive preconditions & error handling
require(numbers.size() > 0, "Collection must not be empty")
try {
    processData()
} catch (e) {
    println("Caught: " + e)
} finally {
    cleanup()
}
```

---

## Trade-offs & Boundaries

- **Not an OS Sandbox**: Autolang is a language-level policy and validation layer. It does not replace containers (Docker, Firecracker, gVisor) for untrusted native binaries.
- **Static Detection vs Runtime Failure**: Autolang catches syntax errors, missing identifiers, and type mismatches before execution. External runtime errors (downstream API 500s, network drops) can still occur; Autolang does not automatically roll back completed external side effects.
- **Host Callback Memory**: The VM manages its own heap quota; memory allocated natively inside host callbacks remains under the host platform's runtime.
- **Determinism**: Script logic is deterministic unless the host exposes non-deterministic capabilities (system clock, random, network).

---

## Benchmarks & Evaluation

Evaluated across 4 frontier LLMs (**Claude 3.5 Sonnet, GPT-4o, DeepSeek V3, Qwen 2.5**) over **350 workflow generations per environment**:

- **0 Unhandled Runtime Crashes**: In synthetic capability benchmarks, 100% of missing function and type mismatch errors were caught before execution.
- **First-Pass Generation Success**: On par with Python and JavaScript for equivalent orchestration tasks.
- *(Note: Closed-loop capability evaluation. Workflows calling live third-party APIs remain subject to operational service errors).*

Run local benchmarks:

```bash
./build/autolang --benchmark tests/correctness/main.atl
```

---

## Standard Library

| Module | Features |
|---|---|
| `Array<T>` | `filter`, `map`, `forEach`, `sort`, `find`, `reduce`, `slice` |
| `Set<T>` | Unique-element collection with fast membership testing |
| `Map<K, V>` | Key-value dictionary with iteration and lookup support |
| `String` | Substring, split, replace, trim, casing, and template interpolation |
| `StringBuilder` | Dynamic string construction (`append`, `appendLine`, `appendRange`) |
| Builders & Checks | `buildList`, `buildMap`, `buildSet`, `require`, `check`, `requireNotNull` |
| `Math`, `Json`, `Regex` | Arithmetic (`clamp`, `round`), JSON serialization, regex matching |
| `Date` / `Time` | ISO formatting, timestamp arithmetic, duration calculation |

---

## Status & Documentation

- **Status**: `Beta (v0.8.x)`. Core C++ engine is mature (102/102 test suites passing with Kotlin stdlib parity). Host bindings are stabilizing toward v1.0.
- **Documentation**: Full API reference, syntax guides, and architecture specs at [autolang.vercel.app/docs](https://autolang.vercel.app/docs).

---

## Sponsors

<p align="left">
  <a href="https://adagroup.com.vn/" target="_blank" rel="noopener noreferrer">
    <img src="assets/sponsor-logo.jpg" alt="ADA GROUP" height="48" />
  </a>
</p>

Special thanks to **[ADA GROUP](https://adagroup.com.vn/)** for supporting project development.

---

## License

MIT License (c) 2026 Autolang Project
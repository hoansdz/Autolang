import { ACompiler, AutolangNativeFunc, LibraryConfig } from 'autolang-compiler';

async function main() {
    console.log("=== AutoLang TypeScript / Native Library Example ===");

    // 1. Create Compiler Instance
    const compiler = await ACompiler.create();

    // 2. Define Native Function Implementations
    const greet: AutolangNativeFunc = (name) => {
        return `Hello, ${name}!`;
    };

    const add: AutolangNativeFunc = (a, b) => {
        return (a as number) + (b as number);
    };

    // 3. Register Native Library with @native annotations
    const config: LibraryConfig = {
        autoImport: true,
        allowLateinitKeyword: false,
        allowNonNullAssertion: false,
    };

    compiler.registerBuiltInLibrary(
        "my/utils",
        `
            @native("greet")
            fun greet(name: String): String

            @native("add")
            fun add(a: Int, b: Int): Int
        `,
        config,
        { greet, add }
    );

    // 4. Compile and Run Autolang Script
    await compiler.compileAndRun("main.atl", `
        println(greet("World from TS"))
        println(add(10, 32))
    `);

    console.log("Output:");
    console.log(compiler.getOutput());
}

main().catch(console.error);

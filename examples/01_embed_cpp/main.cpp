#include <iostream>
#include <Autolang.hpp>

int main() {
    std::cout << "=== AutoLangC Embedding Example ===" << std::endl;

    // 1. Initialize Compiler
    Autolang::ACompiler compiler;

    // 2. Sample AutoLang Script
    const char* sourceCode = R"(
        var msg = "Hello from AutoLangC Embedded Engine!";
        println(msg);
    )";

    // 3. Compile and Run inline source code string
    std::cout << "Compiling and executing script..." << std::endl;
    bool success = compiler.compileAndRun("main.atl", sourceCode);

    if (!success) {
        std::cerr << "Execution failed!" << std::endl;
        return 1;
    }

    std::cout << "Execution completed successfully!" << std::endl;
    return 0;
}

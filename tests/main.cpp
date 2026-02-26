#include <Autolang.hpp>
#include <functional>
#include <iostream>

int main(int argc, char *argv[]) {
	auto start = std::chrono::high_resolution_clock::now();
	try {
		try {
			AutoLang::ACompiler compiler;
			// compiler.loadMainSource("tests/testCorrectness.atl", {{"hi", [](NativeFuncInput) -> AutoLang::AObject* {
			// 	std::cerr<<"Duoc roi ne!!!\n";
			// 	return nullptr;
			// }}});
			// if (compiler.getState() == AutoLang::CompilerState::ERROR) {
			// 	return 0;
			// }
			// compiler.generateBytecodes();
			// compiler.run();
			// compiler.refresh();
			compiler.loadMainSource("tests/benchmark/fib.atl", {{"hi", [](NativeFuncInput) -> AutoLang::AObject* {
				std::cerr<<"Duoc roi ne!!!\n";
				return nullptr;
			}}});
			if (compiler.getState() == AutoLang::CompilerState::ERROR) {
				return 0;
			}
			compiler.generateBytecodes();
			if (compiler.getState() == AutoLang::CompilerState::ERROR) {
				return 0;
			}
			compiler.run();
		} catch (const std::logic_error &err) {
			std::cerr << err.what();
		}
		// if (compiler.vm->state != VMState::ERROR) {
		// 	compiler.vm->start();
		// } else {
		// compiler.vm->log();
		// }
	} catch (const std::exception &e) {
		std::cerr << e.what() << '\n';
	}
	auto end = std::chrono::high_resolution_clock::now();
	auto duration =
	    std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	std::cout << '\n'
	          << "Total time : " << duration.count() << " ms" << '\n';
}
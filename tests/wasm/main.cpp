#include <Autolang.hpp>
#include <emscripten/emscripten.h>
#include <iostream>
#include <string>

extern "C" {
EMSCRIPTEN_KEEPALIVE
void runCompiler(const char *path, const char *data) {
	if (!path) {
		std::cerr << "Path must non null" << std::endl;
		return;
	}
	if (!data) {
		std::cerr << "Data must non null" << std::endl;
		return;
	}
	auto start = std::chrono::high_resolution_clock::now();
	try {
		try {
			AutoLang::ACompiler compiler;
			compiler.registerFromSource(path, true, data, nativeMap);
			if (compiler.getState() == AutoLang::CompilerState::ERROR) {
				return;
			}
			compiler.generateBytecodes();
			if (compiler.getState() == AutoLang::CompilerState::ERROR) {
				return;
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
	std::cout << '\n' << "Total time : " << duration.count() << " ms" << '\n';
}
}
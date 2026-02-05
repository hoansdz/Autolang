#include <Autolang.hpp>
#include <functional>
#include <iostream>

int main(int argc, char *argv[]) {
	try {
		AutoLang::AVMReadFileMode mode = {
		    "tests/test.txt", nullptr, 0, true,
		    ANativeMap({{"hi", [](NativeFuncInput) -> AutoLang::AObject * {
			                 std::cerr << "Ok nha\n";
			                 std::cerr << "Xin chao tat ca cac ban\n";
			                 return nullptr;
		                 }}})};
		auto nativeMap = ANativeMap({{"hi", [](NativeFuncInput) -> AutoLang::AObject * {
			                              std::cerr << "Ok nha\n";
			                              std::cerr
			                                  << "Xin chao tat ca cac ban\n";
			                              return nullptr;
		                              }}});
		try {
			AutoLang::ACompiler compiler;
			auto start = std::chrono::high_resolution_clock::now();
			compiler.registerFromSource("tests/test.txt", false, nativeMap);
			if (compiler.getState() == AutoLang::CompilerState::ERROR) {
				return 0;
			}
			compiler.generateBytecodes();
			if (compiler.getState() == AutoLang::CompilerState::ERROR) {
				return 0;
			}
			compiler.run();
			auto end = std::chrono::high_resolution_clock::now();
			auto duration =
				std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
			std::cout << '\n'
	          << "Total runtime : " << duration.count() << " ms" << '\n';
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
}
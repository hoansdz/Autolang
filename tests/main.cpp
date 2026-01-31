#include <Autolang.hpp>
#include <iostream>

int main(int argc, char *argv[]) {
	try {
		AVMReadFileMode mode = {"tests/test.txt", nullptr, 0, true};
		AutoLang::ACompiler compiler(new AVM(false));
		compiler.registerSourceFromFile(mode);
		if (compiler.vm->state != VMState::ERROR) {
			compiler.vm->start();
		} else {
			// compiler.vm->log();
		}
	} catch (const std::exception &e) {
		std::cerr << e.what() << '\n';
	}
}
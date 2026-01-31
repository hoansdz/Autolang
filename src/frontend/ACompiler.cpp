#ifndef ACOMPILER_CPP
#define ACOMPILER_CPP

#include <chrono>
#include "ACompiler.hpp"

namespace AutoLang {

void ACompiler::registerSourceFromFile(AVMReadFileMode &mode) {
	if (AutoLang::build(vm->data, mode)) {
		vm->data.main = vm->data.functions[vm->data.mainFunctionId];
	} else {
		vm->data.main = vm->data.functions[vm->data.mainFunctionId];
		vm->state = VMState::ERROR;
	}
}

ACompiler::ACompiler(AVM *vm) {
	this->vm = vm;
	auto startCompiler = std::chrono::high_resolution_clock::now();

	AutoLang::DefaultClass::init(vm->data);
	AutoLang::DefaultFunction::init(vm->data);
	vm->data.mainFunctionId =
	    vm->data.registerFunction(nullptr, false, ".main", {}, {}, nullptr);

	std::cout << "Init time : "
	          << std::chrono::duration_cast<std::chrono::milliseconds>(
	                 std::chrono::high_resolution_clock::now() - startCompiler)
	                 .count()
	          << " ms" << '\n';
}

ACompiler::~ACompiler() {
	delete vm;
}

} // namespace AutoLang

#endif
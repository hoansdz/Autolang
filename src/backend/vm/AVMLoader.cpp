#ifndef AVM_LOADER_CPP
#define AVM_LOADER_CPP

#include "ANotifier.hpp"
#include "AVM.hpp"
#include <chrono>
#include <iostream>
#include <sstream>

namespace AutoLang {

AVM::AVM(bool allowDebug)
    : allowDebug(allowDebug), notifier(new ANotifier(this)) {
	data.manager.notifier = notifier;
	data.allBytecodes.reserve(256);
}

void AVM::start() {
	if (state == VMState::ERR) {
		throw std::runtime_error("VM returns error");
	}
	state = VMState::RUNNING;

	data.main = data.functions[data.mainFunctionId];
	// if (!globalVariables) {
	initGlobalVariables();
	// }
	run();
	// log();
	// allowDebug = true;
	while (allowDebug) {
		std::string command;
		std::getline(std::cin, command);
		std::istringstream iss(command);
		std::string word;
		if (iss >> word) {
			if (word == "log") {
				if (iss >> word) {
					std::string name = std::move(word);
					{
						auto it = data.classMap.find(name);
						if (it != data.classMap.end()) {
							data.classes[it->second]->log(data);
							continue;
						}
					}
					auto &vec = data.funcMap[name];
					if (vec.size() == 0) {
						std::cerr << "Cannot find " << name << "\n";
						continue;
					}
					if (vec.size() == 1) {
						log(data.functions[vec[0]]);
						std::cerr << '\n';
						continue;
					}
					for (auto pos : vec) {
						std::cerr << data.functions[pos]->toString(data)
						          << "\n";
					}
					uint32_t at;
					std::cerr << "Has " << vec.size() << ", log at: ";
					std::cin >> at;
					if (at <= vec.size()) {
						log(data.functions[vec[at]]);
						std::cerr << '\n';
						continue;
					}
				} else {
					std::cerr << "Please log function" << '\n';
					continue;
				}
			} else if (word == "e") {
				return;
			}
		} else {
			std::cerr << "wtf" << '\n';
		}
	}
}

void AVM::restart() {
	// auto start = std::chrono::high_resolution_clock::now();
	state = VMState::READY;
	stack.index = 0;
	callFrames.index = 0;
	stackAllocator.restart();
	for (size_t i = 0; i < data.main->maxDeclaration; ++i) {
		globalVariables[i] = nullptr;
	}
	data.manager.refresh();
	// auto end = std::chrono::high_resolution_clock::now();
	// auto duration =
	//     std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	// std::cerr << '\n'
	//           << "Restart time : " << duration.count() << " ms" << '\n';
}

AVM::~AVM() {
	delete notifier;
	delete[] tempAllocateArea;
	if (globalVariables)
		delete[] globalVariables;
}

} // namespace AutoLang

#endif
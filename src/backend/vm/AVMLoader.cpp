#ifndef AVM_LOADER_CPP
#define AVM_LOADER_CPP

#include "ANotifier.hpp"
#include "AVM.hpp"
#include <chrono>
#include <iostream>
#include <sstream>

namespace Autolang {

AVM::AVM(bool allowDebug)
    : allowDebug(allowDebug), notifier(new ANotifier(this)) {
	data.manager.notifier = notifier;
	data.allBytecodes.reserve(256);
	data.allGenericType.reserve(32);
	data.allGenericTypeNullable.reserve(32);
	data.allMemberId.reserve(16);
	data.allMemberNullable.reserve(16);
	data.allCatchPosition.reserve(16);
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

std::pair<uint32_t, const char *> AVM::searchMainLine(Function *func,
                                                      uint32_t opcodeIndex) {
	int i = 1;
	while (i < data.allMainFunctionOpcodeLines.size() &&
	       data.allMainFunctionOpcodeLines[i].opcodeIndex < opcodeIndex) {
		// std::cerr << data.allMainFunctionOpcodeLines[i].opcodeIndex << " "
		//           << data.allMainFunctionOpcodeLines[i].path << ":"
		//           << data.allMainFunctionOpcodeLines[i].line << "\n";
		++i;
	}

	return {data.allMainFunctionOpcodeLines[i - 1].line,
	        data.allMainFunctionOpcodeLines[i - 1].path};
}

uint32_t AVM::searchLine(Function *func, uint32_t opcodeIndex) {
	// int left = func->opcodeIndex;
	// int right = 0;
	int i = func->opcodeIndex + 1;
	while (i < data.allOpcodeLines.size() &&
	       data.allOpcodeLines[i].opcodeIndex < opcodeIndex &&
	       data.allOpcodeLines[i].opcodeIndex >=
	           data.allOpcodeLines[i - 1].opcodeIndex) {
		++i;
	}
	return data.allOpcodeLines[i - 1].line;
}

AVM::~AVM() {
	delete notifier;
	delete[] tempAllocateArea;
	if (globalVariables)
		delete[] globalVariables;
#ifndef NO_INCLUDE_LIBS_HTTP
	if (allowedDomainsRegex) {
		delete allowedDomainsRegex;
	}
#endif
#ifndef NO_INCLUDE_LIBS_FILE
	if (allowedFilePathsRegex) {
		delete allowedFilePathsRegex;
	}
#endif
}

} // namespace Autolang

#endif
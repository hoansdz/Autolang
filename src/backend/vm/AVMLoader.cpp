#ifndef AVM_LOADER_CPP
#define AVM_LOADER_CPP

#include "AVM.hpp"
#include <chrono>
#include <iostream>
#include <sstream>

namespace AutoLang {

AVM::AVM(bool allowDebug) : allowDebug(allowDebug) {}

void AVM::start() {
	if (state == VMState::ERROR) {
		throw std::runtime_error("VM returns error");
	}
	state = VMState::RUNNING;
	
	initGlobalVariables();
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
						std::cout << "Cannot find " << name << "\n";
						continue;
					}
					if (vec.size() == 1) {
						log(data.functions[vec[0]]);
						std::cout << '\n';
						continue;
					}
					for (auto pos : vec) {
						std::cout << data.functions[pos]->toString(data) << "\n";
					}
					uint32_t at;
					std::cout << "Has " << vec.size() << ", log at: ";
					std::cin >> at;
					if (at <= vec.size()) {
						log(data.functions[vec[at]]);
						std::cout << '\n';
						continue;
					}
				} else {
					std::cout << "Please log function" << '\n';
					continue;
				}
			} else if (word == "e") {
				return;
			}
		} else {
			std::cout << "wtf" << '\n';
		}
	}
}

}

#endif
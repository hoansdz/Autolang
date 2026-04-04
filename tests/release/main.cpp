#include <functional>
#include <iostream>
#include "../Autolang.hpp"

#ifdef _WIN32

#include <windows.h>
#include <psapi.h>

void printMemoryUsage() {
	PROCESS_MEMORY_COUNTERS info;

	GetProcessMemoryInfo(GetCurrentProcess(), &info, sizeof(info));

	std::cout << "RAM used: " << info.WorkingSetSize / 1024 << " KB\n";

	std::cout << "Peak RAM: " << info.PeakWorkingSetSize / 1024 << " KB\n";
}

#endif

#include <string>

bool isAbsolute(const std::string &path) {
#ifdef _WIN32
	return path.size() > 1 && path[1] == ':'; // D:/...
#else
	return !path.empty() && path[0] == '/';
#endif
}

std::string normalizePath(const char *input) {
	std::string path = input;

	if (path.rfind("./", 0) == 0 || path.rfind("../", 0) == 0) {
		return path;
	}

	if (isAbsolute(path)) {
		return path;
	}

	return "./" + path;
}

int main(int argc, char *argv[]) {
	auto start = std::chrono::high_resolution_clock::now();
	try {
		try {
			if (argc < 2) {
				std::cout << "Usage: atl <file.atl>\n";
				return 0;
			}
			std::string file = normalizePath(argv[1]);
			AutoLang::ACompiler compiler;
			compiler.loadMainSource(
			    file.c_str(),
			    {{"hi", [](NativeFuncInput) -> AutoLang::AObject * {
				      std::cerr << "Duoc roi ne!!!\n";
				      return nullptr;
			      }}});
			if (compiler.getState() == AutoLang::CompilerState::CT_ERROR) {
				return 0;
			}
			compiler.generateBytecodes();
			if (compiler.getState() == AutoLang::CompilerState::CT_ERROR) {
				return 0;
			}
			compiler.run();
		} catch (const std::logic_error &err) {
			std::cerr << err.what();
		}
	} catch (const std::exception &e) {
		std::cerr << e.what() << '\n';
	}
	auto end = std::chrono::high_resolution_clock::now();
	auto duration =
	    std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	std::cout << '\n'
	          << "Total time (Compiler time + runtime) : " << duration.count()
	          << " ms" << '\n';
#ifdef _WIN32
	printMemoryUsage();
#endif
}
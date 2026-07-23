#define AUTOLANG_LIMIT_OPCODE
// #define NO_INCLUDE_LIBS_HTTP
#include <Autolang.hpp>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>

#ifdef _WIN32

#include <windows.h>
#include <psapi.h>

struct MemoryInfo {
	SIZE_T workingSet;
	SIZE_T privateBytes;
};

MemoryInfo getMemoryUsage() {
	PROCESS_MEMORY_COUNTERS info;
	if (GetProcessMemoryInfo(GetCurrentProcess(), &info, sizeof(info))) {
		return {info.WorkingSetSize, info.PagefileUsage};
	}
	return {0, 0};
}

void printMemoryUsage(const MemoryInfo &base, const MemoryInfo &current) {
	std::cout << "RAM used by Autolang instance (Working Set): "
	          << (current.workingSet > base.workingSet
	                  ? (current.workingSet - base.workingSet)
	                  : 0) /
	                 (float)(1024 * 1024)
	          << " MB\n";
	std::cout << "RAM used by Autolang instance (Private Bytes): "
	          << (current.privateBytes > base.privateBytes
	                  ? (current.privateBytes - base.privateBytes)
	                  : 0) /
	                 (float)(1024 * 1024)
	          << " MB\n";
}

#endif
int main(int argc, char *argv[]) {
	auto start = std::chrono::high_resolution_clock::now();
	try {
		try {
			for (int i = 0; i < 1; ++i) {
#ifdef _WIN32
				MemoryInfo baseMem = getMemoryUsage();
#endif
				Autolang::ACompiler compiler;
				compiler.setLimitOpcodeCount(1000000);
				if (compiler.compileAndRun(
				        "./tests/testCorrectness.atl",
				    Autolang::LibraryConfig(false, true, true))) {
#ifdef _WIN32
					MemoryInfo currentMem = getMemoryUsage();
					printMemoryUsage(baseMem, currentMem);
#endif
				}
// 				compiler.compileAndRun(
// 				    "./tests/a.atl",
// 				    AutoLang::LibraryConfig(false, true, true));
// 				#ifdef _WIN32
// 					MemoryInfo currentMem = getMemoryUsage();
// 					printMemoryUsage(baseMem, currentMem);
// #				endif
			}
		} catch (const std::logic_error &err) {
			std::cerr << err.what();
		}
	} catch (const std::exception &e) {
		std::cerr << e.what() << '\n';
	}
	auto end = std::chrono::high_resolution_clock::now();
	auto duration =
	    std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	std::cout << '\n' << "Total time : " << duration.count() << " ms" << '\n';
	// Memory usage was measured and printed inside the loop when the instance
	// was alive.
}
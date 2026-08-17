#define AUTOLANG_LIMIT_OPCODE
// #define NO_INCLUDE_LIBS_HTTP
#include <Autolang.hpp>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>

#ifdef _WIN32
#include <psapi.h>
#include <windows.h>

struct MemoryInfo {
	SIZE_T workingSet;
	SIZE_T peakWorkingSet;
	SIZE_T privateBytes;
	SIZE_T peakPrivateBytes;
};

MemoryInfo getMemoryUsage() {
	PROCESS_MEMORY_COUNTERS info;
	if (GetProcessMemoryInfo(GetCurrentProcess(), &info, sizeof(info))) {
		return {info.WorkingSetSize, info.PeakWorkingSetSize, info.PagefileUsage, info.PeakPagefileUsage};
	}
	return {0, 0, 0, 0};
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

void runBenchmarkReport(const std::chrono::high_resolution_clock::time_point &processStart, const char* scriptPath) {
	auto t_after_process = std::chrono::high_resolution_clock::now();
	
#ifdef _WIN32
	MemoryInfo baseMem = getMemoryUsage();
#endif

	auto t_before_init = std::chrono::high_resolution_clock::now();
	Autolang::ACompiler compiler;
	compiler.setLimitOpcodeCount(1000000);
	compiler.setMaxManagedMemory(1024 * 1024);
	auto t_after_init = std::chrono::high_resolution_clock::now();

	auto t_before_load = std::chrono::high_resolution_clock::now();
	compiler.loadMainSource(scriptPath, Autolang::LibraryConfig(false, true, true));
	auto t_after_load = std::chrono::high_resolution_clock::now();

	auto t_before_compile = std::chrono::high_resolution_clock::now();
	compiler.generateBytecodes();
	auto t_after_compile = std::chrono::high_resolution_clock::now();

	auto t_before_vm = std::chrono::high_resolution_clock::now();
	compiler.run();
	auto t_after_vm = std::chrono::high_resolution_clock::now();

#ifdef _WIN32
	MemoryInfo currentMem = getMemoryUsage();
#endif

	auto t_before_cleanup = std::chrono::high_resolution_clock::now();
	compiler.refresh();
	auto t_end = std::chrono::high_resolution_clock::now();

	double processStartupUs = std::chrono::duration<double, std::micro>(t_after_process - processStart).count();
	double compilerInitUs = std::chrono::duration<double, std::micro>(t_after_init - t_before_init).count();
	double fileIoUs = std::chrono::duration<double, std::micro>(t_after_load - t_before_load).count();
	double compilationUs = std::chrono::duration<double, std::micro>(t_after_compile - t_before_compile).count();
	double vmExecutionUs = std::chrono::duration<double, std::micro>(t_after_vm - t_before_vm).count();
	double harnessCleanupUs = std::chrono::duration<double, std::micro>(t_end - t_before_cleanup).count();
	double totalUs = std::chrono::duration<double, std::micro>(t_end - processStart).count();

	if (totalUs <= 0.0) totalUs = 1.0;

	std::cout << "\n====================================================================================================\n";
	std::cout << "AUTOLANG BENCHMARK METRICS REPORT\n";
	std::cout << "====================================================================================================\n";
	std::cout << "Environment Spec : Windows 11 | Intel Core i5 12th Gen | 16GB RAM\n";
	std::cout << "Target Script    : " << scriptPath << "\n";
	std::cout << "----------------------------------------------------------------------------------------------------\n";
	std::cout << std::left << std::setw(30) << "Phase Breakdown" << " | "
	          << std::right << std::setw(18) << "Execution Time (ms)" << " | "
	          << std::setw(18) << "Execution Time (us)" << " | "
	          << std::setw(10) << "Share (%)" << "\n";
	std::cout << "----------------------------------------------------------------------------------------------------\n";

	auto printRow = [&](const std::string &name, double us) {
		double ms = us / 1000.0;
		double percent = (us / totalUs) * 100.0;
		std::cout << std::left << std::setw(30) << name << " | "
		          << std::right << std::setw(15) << std::fixed << std::setprecision(3) << ms << " ms | "
		          << std::setw(15) << std::fixed << std::setprecision(1) << us << " us | "
		          << std::setw(9) << std::fixed << std::setprecision(1) << percent << "%\n";
	};

	printRow("1. Process Startup", processStartupUs);
	printRow("2. Compiler Initialization", compilerInitUs);
	printRow("3. Loading Test Files (I/O)", fileIoUs);
	printRow("4. Compilation (AST/Bytecode)", compilationUs);
	printRow("5. VM Execution", vmExecutionUs);
	printRow("6. Test Harness & Cleanup", harnessCleanupUs);
	std::cout << "----------------------------------------------------------------------------------------------------\n";
	std::cout << std::left << std::setw(30) << "TOTAL TIME" << " | "
	          << std::right << std::setw(15) << std::fixed << std::setprecision(3) << (totalUs / 1000.0) << " ms | "
	          << std::setw(15) << std::fixed << std::setprecision(1) << totalUs << " us | "
	          << std::setw(9) << "100.0%\n";
	std::cout << "----------------------------------------------------------------------------------------------------\n";
	std::cout << "MEMORY FOOTPRINT:\n";
#ifdef _WIN32
	double wsMB = (currentMem.workingSet > baseMem.workingSet ? (currentMem.workingSet - baseMem.workingSet) : currentMem.workingSet) / (1024.0 * 1024.0);
	double peakWsMB = (currentMem.peakWorkingSet > baseMem.workingSet ? (currentMem.peakWorkingSet - baseMem.workingSet) : currentMem.peakWorkingSet) / (1024.0 * 1024.0);
	double pbMB = (currentMem.privateBytes > baseMem.privateBytes ? (currentMem.privateBytes - baseMem.privateBytes) : currentMem.privateBytes) / (1024.0 * 1024.0);
	double peakPbMB = (currentMem.peakPrivateBytes > baseMem.privateBytes ? (currentMem.peakPrivateBytes - baseMem.privateBytes) : currentMem.peakPrivateBytes) / (1024.0 * 1024.0);

	std::cout << "- RAM Working Set (Current): " << std::fixed << std::setprecision(4) << wsMB << " MB (Total: " << (currentMem.workingSet / (1024.0 * 1024.0)) << " MB)\n";
	std::cout << "- RAM Working Set (Peak)   : " << std::fixed << std::setprecision(4) << peakWsMB << " MB (Total Peak: " << (currentMem.peakWorkingSet / (1024.0 * 1024.0)) << " MB)\n";
	std::cout << "- RAM Private Bytes (Current): " << std::fixed << std::setprecision(4) << pbMB << " MB (Total: " << (currentMem.privateBytes / (1024.0 * 1024.0)) << " MB)\n";
	std::cout << "- RAM Private Bytes (Peak)   : " << std::fixed << std::setprecision(4) << peakPbMB << " MB (Total Peak: " << (currentMem.peakPrivateBytes / (1024.0 * 1024.0)) << " MB)\n";
#else
	std::cout << "- RAM footprint measurement not available on non-Windows target\n";
#endif
	std::cout << "====================================================================================================\n\n";
}

int main(int argc, char *argv[]) {
	auto processStart = std::chrono::high_resolution_clock::now();

	bool isBenchmark = false;
	const char* scriptPath = "./tests/testCorrectness.atl";

	for (int i = 1; i < argc; ++i) {
		std::string arg = argv[i];
		if (arg == "--benchmark" || arg == "benchmark" || arg == "-b") {
			isBenchmark = true;
		} else if (arg.length() > 0 && arg[0] != '-') {
			scriptPath = argv[i];
		}
	}

	if (isBenchmark) {
		runBenchmarkReport(processStart, scriptPath);
		return 0;
	}

	// Standard test execution
	try {
		try {
			for (int i = 0; i < 1; ++i) {
#ifdef _WIN32
				MemoryInfo baseMem = getMemoryUsage();
#endif
				Autolang::ACompiler compiler;
				compiler.setLimitOpcodeCount(1000000);
				compiler.setMaxManagedMemory(1024 * 1024);
				if (compiler.compile(
				        scriptPath,
				        Autolang::LibraryConfig(false, true, true))) {
#ifdef _WIN32
					MemoryInfo currentMem = getMemoryUsage();
					printMemoryUsage(baseMem, currentMem);
#endif
					compiler.run();
					compiler.refresh();
				}
			}
		} catch (const std::logic_error &err) {
			std::cerr << err.what();
		}
	} catch (const std::exception &e) {
		std::cerr << e.what() << '\n';
	}
	auto end = std::chrono::high_resolution_clock::now();
	auto duration =
	    std::chrono::duration_cast<std::chrono::milliseconds>(end - processStart);
	std::cout << '\n' << "Total time : " << duration.count() << " ms" << '\n';
	return 0;
}

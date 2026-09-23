#define AUTOLANG_LIMIT_OPCODE
// #define NO_INCLUDE_LIBS_HTTP
#include <Autolang.hpp>
#include "shared/Profiler.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

namespace AutolangTests {

inline bool runCompileTimeRuleTest(Autolang::ACompiler &compiler, const std::string &filepath) {
	bool success = false;
	try {
		success = compiler.compile(filepath.c_str(),
		                           Autolang::LibraryConfig(false, true, true));
	} catch (...) {
		compiler.refresh();
		std::cerr << "[Compile Rule] Exception threw during compile: " << filepath << "\n";
		return false;
	}
	bool isExpectedError = !success || compiler.hasError();
	if (!isExpectedError) {
		std::cerr << "[Compile Rule] Expected compile error but it compiled successfully: " << filepath << "\n";
	}
	compiler.refresh();
	return isExpectedError;
}

inline bool runRuntimeRuleTest(Autolang::ACompiler &compiler, const std::string &filepath) {
	try {
		bool compiled = compiler.compile(filepath.c_str(),
		                                 Autolang::LibraryConfig(false, true, true));
		if (!compiled || compiler.hasError()) {
			std::cerr << "[Runtime Rule] Expected compile success but got compile error: " << filepath << "\n";
			compiler.refresh();
			return false;
		}

		std::streambuf* oldCerr = std::cerr.rdbuf();
		std::ostringstream nullStream;
		std::cerr.rdbuf(nullStream.rdbuf());
		
		try {
			compiler.run();
		} catch (...) {
			std::cerr.rdbuf(oldCerr);
			throw;
		}
		
		std::cerr.rdbuf(oldCerr);

		bool hasExc = compiler.hasException();
		if (!hasExc) {
			std::cerr << "[Runtime Rule] Expected runtime exception but none occurred: " << filepath << "\n";
		}
		compiler.refresh();
		return hasExc;
	} catch (...) {
		std::cerr << "[Runtime Rule] Exception threw during compile/run: " << filepath << "\n";
		compiler.refresh();
		return false;
	}
}

inline size_t runAllCompileTimeRules(Autolang::ACompiler &compiler, const std::string &dirPath, size_t &passedCount) {
	size_t total = 0;
	if (!std::filesystem::exists(dirPath)) return 0;
	for (const auto &entry : std::filesystem::directory_iterator(dirPath)) {
		if (entry.is_regular_file() && entry.path().extension() == ".atl") {
			total++;
			bool passed = runCompileTimeRuleTest(compiler, entry.path().string());
			if (passed) {
				std::cout << "Passed [CompileTime] " << entry.path().filename().string() << '\n';
				passedCount++;
			} else {
				std::cerr << "Failed [CompileTime] " << entry.path().filename().string() << '\n';
			}
		}
	}
	return total;
}

inline size_t runAllRuntimeRules(Autolang::ACompiler &compiler, const std::string &dirPath, size_t &passedCount) {
	size_t total = 0;
	if (!std::filesystem::exists(dirPath)) return 0;
	for (const auto &entry : std::filesystem::directory_iterator(dirPath)) {
		if (entry.is_regular_file() && entry.path().extension() == ".atl") {
			total++;
			bool passed = runRuntimeRuleTest(compiler, entry.path().string());
			if (passed) {
				std::cout << "Passed [Runtime] " << entry.path().filename().string() << '\n';
				passedCount++;
			} else {
				std::cerr << "Failed [Runtime] " << entry.path().filename().string() << '\n';
			}
		}
	}
	return total;
}
} // namespace AutolangTests

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>

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

bool runCorrectnessTest(Autolang::ACompiler &compiler, const char *scriptPath) {
	try {
#ifdef _WIN32
		MemoryInfo baseMem = getMemoryUsage();
#endif
		if (!compiler.compile(scriptPath, Autolang::LibraryConfig(false, true, true))) {
			compiler.refresh();
			return false;
		}
#ifdef _WIN32
		MemoryInfo currentMem = getMemoryUsage();
		printMemoryUsage(baseMem, currentMem);
#endif
		compiler.run();
		if (compiler.exceptionMessage) {
			std::cerr << "Uncaught exception: " << compiler.exceptionMessage << '\n';
			compiler.refresh();
			return false;
		}
		compiler.refresh();
		return true;
	} catch (const std::exception &e) {
		std::cerr << e.what() << '\n';
		compiler.refresh();
		return false;
	}
}

void runBenchmarkReport(const std::chrono::high_resolution_clock::time_point &processStart, const char* scriptPath) {
	AUTOLANG_PROFILE_START("0. Process Startup");

	AUTOLANG_PROFILE_MARK("1. Compiler Instantiation");
	Autolang::ACompiler compiler;
	compiler.setLimitOpcodeCount(1000000);
	compiler.setMaxManagedMemory(1024 * 1024);

	AUTOLANG_PROFILE_MARK("2. Load Main Source & Libs");
	compiler.loadMainSource(scriptPath, Autolang::LibraryConfig(false, true, true));

	AUTOLANG_PROFILE_MARK("3. Bytecode Generation");
	compiler.generateBytecodes();

	AUTOLANG_PROFILE_MARK("4. VM Execution");
	compiler.run();

	AUTOLANG_PROFILE_MARK("5. Cleanup & Refresh");
	compiler.refresh();

	AUTOLANG_PROFILE_MARK("6. Completed");

	std::cout << "\nEnvironment Spec : Windows 11 | Intel Core i5 12th Gen | 16GB RAM\n";
	std::cout << "Target Script    : " << scriptPath << "\n";
	AUTOLANG_PROFILE_REPORT("AUTOLANG BENCHMARK PERFORMANCE & RAM REPORT");
}

int main(int argc, char *argv[]) {
	auto processStart = std::chrono::high_resolution_clock::now();

	bool isBenchmark = false;
	const char* scriptPath = "./tests/testCorrectness.atl";
	bool isSingleCustomScript = false;

	for (int i = 1; i < argc; ++i) {
		std::string arg = argv[i];
		if (arg == "--benchmark" || arg == "benchmark" || arg == "-b" || arg == "--profile" || arg == "-p") {
			isBenchmark = true;
		} else if (arg.length() > 0 && arg[0] != '-') {
			scriptPath = argv[i];
			isSingleCustomScript = true;
		}
	}

	if (isBenchmark) {
		runBenchmarkReport(processStart, scriptPath);
		return 0;
	}

	if (isSingleCustomScript) {
		Autolang::ACompiler customCompiler;
		customCompiler.setLimitOpcodeCount(1000000);
		customCompiler.setMaxManagedMemory(1024 * 1024);
		try {
			if (!customCompiler.compile(scriptPath, Autolang::LibraryConfig(false, true, true))) {
				std::cerr << "Compilation failed: " << scriptPath << '\n';
				return 1;
			}
			customCompiler.run();
			if (customCompiler.exceptionMessage) {
				std::cerr << "Uncaught exception: " << customCompiler.exceptionMessage << '\n';
				return 1;
			}
		} catch (const std::exception &e) {
			std::cerr << "Error: " << e.what() << '\n';
			return 1;
		}
		auto end = std::chrono::high_resolution_clock::now();
		auto duration =
		    std::chrono::duration_cast<std::chrono::milliseconds>(end - processStart);
		std::cout << '\n' << "Total time : " << duration.count() << " ms" << '\n';
		return 0;
	}

	// Full test suite execution: Correctness + CompileTime Rules + Runtime Rules
	Autolang::ACompiler sharedCompiler;
	sharedCompiler.setLimitOpcodeCount(1000000);
	sharedCompiler.setMaxManagedMemory(1024 * 1024);

	bool correctnessPassed = runCorrectnessTest(sharedCompiler, scriptPath);

	// Set silent error handler when transitioning to rule violation tests
	sharedCompiler.setOnError(new Autolang::FunctionEvent([](std::string_view) {}));

	size_t passedCount = correctnessPassed ? 1 : 0;
	size_t totalCount = 1;

	totalCount += AutolangTests::runAllCompileTimeRules(sharedCompiler, "tests/rule/compile_time", passedCount);
	totalCount += AutolangTests::runAllRuntimeRules(sharedCompiler, "tests/rule/runtime", passedCount);

	auto end = std::chrono::high_resolution_clock::now();
	auto duration =
	    std::chrono::duration_cast<std::chrono::milliseconds>(end - processStart);
	std::cout << '\n' << "Total time : " << duration.count() << " ms" << '\n';

	std::cout << "Test summary: " << passedCount << "/" << totalCount << " passed.\n";

	if (passedCount != totalCount) {
		return 1;
	}

	return 0;
}

#define AUTOLANG_LIMIT_OPCODE
// #define NO_INCLUDE_LIBS_HTTP
#include <Autolang.hpp>
#include "backend/vm/ANotifier.hpp"
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
		compiler.setStrictMode(true);
		success = compiler.compile(filepath.c_str(),
		                           Autolang::LibraryConfig(false, true, true));
		compiler.setStrictMode(false);
	} catch (...) {
		compiler.setStrictMode(false);
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
	const char *targetScript = (scriptPath != nullptr && scriptPath[0] != '\0')
	    ? scriptPath
	    : "./tests/correctness/main.atl";
	try {
#ifdef _WIN32
		MemoryInfo baseMem = getMemoryUsage();
#endif
		if (!compiler.compile(targetScript, Autolang::LibraryConfig(false, true, true))) {
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

void printHelp(const char *programName) {
	std::cout << "AutoLang Programming Language\n"
	          << "Usage:\n"
	          << "  " << programName << " [options] [script.atl]\n"
	          << "  " << programName << " [options] -e \"<code>\"\n"
	          << "  " << programName << " [options] --test\n\n"
	          << "Options:\n"
	          << "  -h, --help             Show this help message and exit\n"
	          << "  -v, --version          Show version information and exit\n"
	          << "  -e, --eval <code>      Execute inline source code directly\n"
	          << "  -s, --strict           Enable strict compilation mode\n"
	          << "  -b, --benchmark        Run with benchmark performance and memory profiling\n"
	          << "      --test             Run the full correctness and rule test suite\n\n"
	          << "Examples:\n"
	          << "  " << programName << " script.atl\n"
	          << "  " << programName << " -e 'println(\"Hello, World!\")'\n"
	          << "  " << programName << " -b script.atl\n"
	          << "  " << programName << " --test\n";
}

void printVersion() {
	std::cout << "AutoLang 1.0.0 (C++17)\n";
}

int main(int argc, char *argv[]) {
	auto processStart = std::chrono::high_resolution_clock::now();

	bool isBenchmark = false;
	bool isStrict = false;
	bool isRunTests = false;
	std::string evalCode;
	bool hasEvalCode = false;
	const char* scriptPath = nullptr;

	for (int i = 1; i < argc; ++i) {
		std::string arg = argv[i];
		if (arg == "-h" || arg == "--help" || arg == "help") {
			printHelp(argv[0]);
			return 0;
		} else if (arg == "-v" || arg == "--version" || arg == "version") {
			printVersion();
			return 0;
		} else if (arg == "-e" || arg == "--eval") {
			if (i + 1 < argc) {
				evalCode = argv[++i];
				hasEvalCode = true;
			} else {
				std::cerr << "Error: Option '" << arg << "' requires an argument.\n";
				return 1;
			}
		} else if (arg == "-s" || arg == "--strict") {
			isStrict = true;
		} else if (arg == "--benchmark" || arg == "benchmark" || arg == "-b" || arg == "--profile" || arg == "-p") {
			isBenchmark = true;
		} else if (arg == "--test" || arg == "test") {
			isRunTests = true;
		} else if (arg.length() > 0 && arg[0] != '-') {
			if (arg == "run" && i + 1 < argc && argv[i + 1][0] != '-') {
				scriptPath = argv[++i];
			} else {
				scriptPath = argv[i];
			}
		} else {
			std::cerr << "Unknown option: '" << arg << "'\nUse '--help' to view available options.\n";
			return 1;
		}
	}

	if (hasEvalCode) {
		Autolang::ACompiler evalCompiler;
		evalCompiler.setLimitOpcodeCount(1000000);
		evalCompiler.setMaxManagedMemory(1024 * 1024);
		if (isStrict) {
			evalCompiler.setStrictMode(true);
		}
		bool success = evalCompiler.runSource(evalCode);
		if (!success) {
			if (!evalCompiler.getLastError().empty()) {
				std::cerr << evalCompiler.getLastError() << '\n';
			} else if (evalCompiler.exceptionMessage) {
				std::cerr << "Uncaught exception: " << evalCompiler.exceptionMessage << '\n';
			} else {
				std::cerr << "Execution failed.\n";
			}
			return 1;
		}
		return 0;
	}

	if (scriptPath != nullptr) {
		if (!std::filesystem::exists(scriptPath)) {
			std::cerr << "Error: File not found: '" << scriptPath << "'\n";
			return 1;
		}

		if (isBenchmark) {
			runBenchmarkReport(processStart, scriptPath);
			return 0;
		}

		Autolang::ACompiler customCompiler;
		customCompiler.setLimitOpcodeCount(1000000);
		customCompiler.setMaxManagedMemory(1024 * 1024);
		if (isStrict) {
			customCompiler.setStrictMode(true);
		}
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

	if (isBenchmark) {
		runBenchmarkReport(processStart, "./tests/correctness/main.atl");
		return 0;
	}

	// Full test suite execution: Correctness + CompileTime Rules + Runtime Rules
	Autolang::ACompiler sharedCompiler;
	sharedCompiler.setLimitOpcodeCount(1000000);
	sharedCompiler.setMaxManagedMemory(1024 * 1024);

	bool correctnessPassed = runCorrectnessTest(sharedCompiler, scriptPath);

	// Set silent error handler when transitioning to rule violation tests
	sharedCompiler.setOnError([](std::string_view) {});

	size_t passedCount = correctnessPassed ? 1 : 0;
	size_t totalCount = 1;

	// Verify new ergonomic compiler APIs
	{
		totalCount++;
		Autolang::ACompiler apiCompiler;
		std::string capturedError;
		apiCompiler.setOnError([&](std::string_view err) {
			capturedError = std::string(err);
		});
		bool compileFail = apiCompiler.compileSource("val a: Int = \"invalid string\"");
		bool errorRecorded = !compileFail && !apiCompiler.getLastError().empty() && !capturedError.empty();

		apiCompiler.reset();
		bool ran = apiCompiler.runSource("val x = 10 + 20");

		apiCompiler.reset();
		int hostCalled = 0;
		apiCompiler.registerFunction("hostAdd", [&](NativeFuncInData) -> Autolang::AObject* {
			hostCalled++;
			return notifier.createInt(100);
		});
		bool hostRan = apiCompiler.runSource("@native(\"hostAdd\") fun hostAdd(): Int\nval res = hostAdd()");

		if (errorRecorded && ran && hostRan && hostCalled == 1) {
			passedCount++;
			std::cout << "Passed testCompilerConvenienceApi\n";
		} else {
			std::cerr << "Failed testCompilerConvenienceApi: errorRecorded=" << errorRecorded
			          << " ran=" << ran << " hostRan=" << hostRan << " hostCalled=" << hostCalled
			          << " exception=" << apiCompiler.getException()
			          << " lastError=" << apiCompiler.getLastError() << "\n";
		}
	}

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

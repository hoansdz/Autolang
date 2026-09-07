// #define NO_INCLUDE_LIBS_HTTP
#include <Autolang.hpp>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <vector>
#include <sstream>

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
		compiler.refresh();
		return true;
	} catch (const std::exception &e) {
		std::cerr << e.what() << '\n';
		compiler.refresh();
		return false;
	}
}

int main(int argc, char *argv[]) {
	auto processStart = std::chrono::high_resolution_clock::now();

	// Đặt thành true để chạy riêng một script độc lập
	bool runSingleCustomScript = false;
	const char *customScriptPath = "tests/test.atl";
	if (argc > 1 && argv[1][0] != '-') {
		runSingleCustomScript = true;
		customScriptPath = argv[1];
	}

	if (runSingleCustomScript) {
		Autolang::ACompiler customCompiler;
		customCompiler.setLimitOpcodeCount(1000000);
		customCompiler.setMaxManagedMemory(1024 * 1024);
		try {
			if (!customCompiler.compile(customScriptPath, Autolang::LibraryConfig(false, true, true))) {
				std::cerr << "Compilation failed: " << customScriptPath << '\n';
				return 1;
			}
			customCompiler.run();
		} catch (const std::exception &e) {
			std::cerr << "Error: " << e.what() << '\n';
			return 1;
		}
		return 0;
	}

	bool isBenchmark = false;
	const char* scriptPath = "tests/testCorrectness.atl";

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

	// Khởi tạo một thể hiện ACompiler dùng chung để thực hiện stress test khả năng reload/refresh
	Autolang::ACompiler sharedCompiler;
	sharedCompiler.setLimitOpcodeCount(1000000);
	sharedCompiler.setMaxManagedMemory(1024 * 1024);

	// 1. Thực thi kiểm thử tính đúng đắn (Correctness)
	bool correctnessPassed = runCorrectnessTest(sharedCompiler, scriptPath);

	// Thiết lập bộ bắt thông báo lỗi yên lặng khi chuyển sang các kiểm thử quy tắc vi phạm
	sharedCompiler.setOnError(new Autolang::FunctionEvent([](std::string_view) {}));

	size_t passedCount = correctnessPassed ? 1 : 0;
	size_t totalCount = 1;

	totalCount += AutolangTests::runAllCompileTimeRules(sharedCompiler, "tests/rule/compile_time", passedCount);
	totalCount += AutolangTests::runAllRuntimeRules(sharedCompiler, "tests/rule/runtime", passedCount);

	auto end = std::chrono::high_resolution_clock::now();
	auto duration =
	    std::chrono::duration_cast<std::chrono::milliseconds>(end - processStart);
	std::cout << '\n' << "Total time : " << duration.count() << " ms" << '\n';

	if (passedCount != totalCount) {
		std::cerr << "Test summary: " << passedCount << "/" << totalCount << " passed.\n";
		return 1;
	}

	return 0;
}

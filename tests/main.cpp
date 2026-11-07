#define AUTOLANG_LIMIT_OPCODE
// #define NO_INCLUDE_LIBS_HTTP
#include <Autolang.hpp>
#include "backend/vm/ANotifier.hpp"
#include "shared/Profiler.hpp"
#include <algorithm>
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
		ANativeMap customNatives;
		customNatives["hostAdd"] = Autolang::ANativeFunctionData([&](NativeFuncInData) -> Autolang::AObject* {
			hostCalled++;
			return notifier.createInt(100);
		});
		bool hostRan = apiCompiler.runSource("@native(\"hostAdd\") fun hostAdd(): Int\nval res = hostAdd()", "main.atl", Autolang::LibraryConfig(), customNatives);

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

	// Verify strict mode differential rules (strictMode=true vs strictMode=false)
	{
		totalCount++;
		Autolang::ACompiler strictTestCompiler;
		strictTestCompiler.setLimitOpcodeCount(1000000);
		strictTestCompiler.setMaxManagedMemory(1024 * 1024);
		strictTestCompiler.setOnError([](std::string_view) {});

		bool allPassed = true;

		struct StrictCase {
			const char *name;
			const char *code;
		};

		StrictCase cases[] = {
			{"uninit_class_member", "class StrictBox {\nval name: String\nconstructor(n: String) { this.name = n }\n}"},
			{"val_reassign_local", "fun testLocal(): Void {\nval x = 10\nx = 20\n}"},
			{"val_reassign_member", "class ValMember(val v: Int)\nfun testMem(m: ValMember): Void {\nm.v = 20\n}"},
			{"float_to_int_assign", "fun testFloat(): Void {\nvar a: Int = 10\na = 3.14\n}"},
			{"for_without_parentheses", "fun testFor(): Void {\nvar sum = 0\nfor i in 1..5 {\nsum += i\n}\n}"}
		};

		for (const auto &c : cases) {
			// In strict mode (true): must fail compilation
			{
				Autolang::ACompiler strictComp;
				strictComp.setOnError([](std::string_view) {});
				strictComp.setStrictMode(true);
				bool strictSuccess = strictComp.compileSource(c.code);
				if (strictSuccess) {
					std::cerr << "Strict mode rule failed: Expected compilation failure in strict mode for '" << c.name << "' but it succeeded.\n";
					allPassed = false;
				}
			}

			// In relaxed mode (false): must compile successfully
			{
				Autolang::ACompiler relaxedComp;
				relaxedComp.setOnError([](std::string_view) {});
				relaxedComp.setStrictMode(false);
				bool relaxedSuccess = relaxedComp.compileSource(c.code);
				if (!relaxedSuccess) {
					std::cerr << "Strict mode rule failed: Expected success in non-strict mode for '" << c.name
					          << "' but got error: " << relaxedComp.getLastError() << "\n";
					allPassed = false;
				}
			}
		}

		if (allPassed) {
			passedCount++;
			std::cout << "Passed testStrictModeDualRules\n";
		}
	}

	{
		totalCount++;
		Autolang::ACompiler timeoutCompiler;
		timeoutCompiler.setExecutionTimeoutMs(50);
		timeoutCompiler.setLimitOpcodeCount(UINT32_MAX);
		timeoutCompiler.runSource("while (true) {}");
		if (timeoutCompiler.hasException() && timeoutCompiler.exceptionMessage &&
		    std::string(timeoutCompiler.exceptionMessage).find("Execution timed out") != std::string::npos) {
			passedCount++;
			std::cout << "Passed testExecutionTimeout\n";
		} else {
			std::cerr << "Failed testExecutionTimeout: Expected 'Execution timed out' exception.\n";
		}
	}

	{
		totalCount++;
		Autolang::ACompiler availCompiler;
		bool ok = true;
		if (availCompiler.isAvailableGlobalVariable("g_test_var")) ok = false;
		availCompiler.compileSource("val g_test_var = 123;\nfun myHelper(): Void {}\n");

		if (!availCompiler.isAvailableClass("String") || !availCompiler.isAvailableClass("Int") ||
		    !availCompiler.isAvailableClass("Array") || !availCompiler.isAvailableClass("Map")) {
			ok = false;
		}
		if (availCompiler.isAvailableClass("NonExistentClassXYZ")) ok = false;
		if (!availCompiler.isAvailableType("Int") || !availCompiler.isAvailableType("Int?") ||
		    !availCompiler.isAvailableType("String?") || !availCompiler.isAvailableType("Array<Int>")) {
			ok = false;
		}
		if (availCompiler.isAvailableType("NonExistentTypeXYZ")) ok = false;
		if (!availCompiler.isAvailableFunction("println") || !availCompiler.isAvailableFunction("print") ||
		    !availCompiler.isAvailableFunction("range")) {
			ok = false;
		}
		if (availCompiler.isAvailableFunction("non_existent_func_xyz")) ok = false;
		if (!availCompiler.isAvailableGlobalVariable("g_test_var")) ok = false;
		if (availCompiler.isAvailableGlobalVariable("local_var")) ok = false;
		if (!availCompiler.isAvailableFunction("myHelper")) ok = false;

		if (ok) {
			passedCount++;
			std::cout << "Passed testAvailabilityApis\n";
		} else {
			std::cerr << "Failed testAvailabilityApis: availability checks returned unexpected results.\n";
		}
	}

	{
		totalCount++;
		Autolang::ACompiler reflCompiler;
		bool ok = true;
		reflCompiler.compileSource(
		    "@import(\"std/time\")\n"
		    "class User {\n"
		    "    var age: Int = 20;\n"
		    "    static var tag: String = \"admin\";\n"
		    "    fun getAge(): Int { return age; }\n"
		    "    static fun create(): User { return User(); }\n"
		    "}\n"
		    "val g_const_val: Int = 100;\n"
		    "var g_mut_var: String = \"hello\";\n"
		    "fun doAction(cb: (Int) -> Void): Void {}\n"
		);

		#define CHECK(cond, msg) if (!(cond)) { std::cerr << "FAIL: " << msg << "\n"; ok = false; }
		// 1. Members
		uint32_t ageId = reflCompiler.getMemberId("User", "age");
		CHECK(ageId != UINT32_MAX, "ageId != UINT32_MAX");
		CHECK(reflCompiler.isAvailableMember("User", "age"), "isAvailableMember User.age");
		CHECK(!reflCompiler.isMemberMethod(ageId), "!isMemberMethod ageId");
		CHECK(!reflCompiler.isMemberStatic(ageId), "!isMemberStatic ageId");
		CHECK(reflCompiler.getMemberParamCount(ageId) == -1, "getMemberParamCount ageId == -1");
		CHECK(std::string_view(reflCompiler.getMemberReturnType(ageId)) == "Int", "getMemberReturnType ageId == Int");

		uint32_t tagId = reflCompiler.getMemberId("User", "tag");
		CHECK(tagId != UINT32_MAX, "tagId != UINT32_MAX");
		CHECK(reflCompiler.isMemberStatic(tagId), "isMemberStatic tagId");

		uint32_t getAgeId = reflCompiler.getMemberId("User", "getAge");
		CHECK(getAgeId != UINT32_MAX, "getAgeId != UINT32_MAX");
		CHECK(reflCompiler.isMemberMethod(getAgeId), "isMemberMethod getAgeId");
		CHECK(!reflCompiler.isMemberStatic(getAgeId), "!isMemberStatic getAgeId");
		CHECK(reflCompiler.getMemberParamCount(getAgeId) == 0, "getMemberParamCount getAgeId == 0");
		CHECK(std::string_view(reflCompiler.getMemberReturnType(getAgeId)) == "Int", "getMemberReturnType getAgeId == Int");

		CHECK(reflCompiler.getMemberId("User", "nonExistentMember") == UINT32_MAX, "nonExistentMember getMemberId");
		CHECK(!reflCompiler.isAvailableMember("User", "nonExistentMember"), "nonExistentMember isAvailableMember");

		auto members = reflCompiler.getMembersForType("User");
		CHECK(!members.empty(), "User members not empty");

		// 2. Global variables
		uint32_t constValId = reflCompiler.getGlobalVariableId("g_const_val");
		CHECK(constValId != UINT32_MAX, "constValId != UINT32_MAX");
		CHECK(reflCompiler.isGlobalVariableConst(constValId), "isGlobalVariableConst constValId");
		CHECK(std::string_view(reflCompiler.getGlobalVariableType(constValId)) == "Int", "getGlobalVariableType constValId == Int");

		uint32_t mutVarId = reflCompiler.getGlobalVariableId("g_mut_var");
		CHECK(mutVarId != UINT32_MAX, "mutVarId != UINT32_MAX");
		CHECK(!reflCompiler.isGlobalVariableConst(mutVarId), "!isGlobalVariableConst mutVarId");
		CHECK(std::string_view(reflCompiler.getGlobalVariableType(mutVarId)) == "String", "getGlobalVariableType mutVarId == String");

		CHECK(reflCompiler.getGlobalVariableId("nonExistentVar") == UINT32_MAX, "nonExistentVar");

		// 3. Functions
		uint32_t funcId = reflCompiler.getFunctionId("doAction");
		CHECK(funcId != UINT32_MAX, "funcId != UINT32_MAX");
		CHECK(reflCompiler.getFunctionParamCount("doAction") == 1, "getFunctionParamCount doAction == 1");
		CHECK(reflCompiler.getFunctionReturnType("doAction") == "Void", "getFunctionReturnType doAction == Void");
		CHECK(reflCompiler.hasTrailingClosure("doAction"), "hasTrailingClosure doAction");

		CHECK(reflCompiler.getFunctionId("nonExistentFunc") == UINT32_MAX, "nonExistentFunc getFunctionId");
		CHECK(reflCompiler.getFunctionParamCount("nonExistentFunc") == -1, "nonExistentFunc getFunctionParamCount");

		// 4. New P0, P1, P2, P3 API checks
		uint32_t userTypeId = reflCompiler.getTypeId("User");
		CHECK(userTypeId != UINT32_MAX, "userTypeId != UINT32_MAX");
		CHECK(reflCompiler.getTypeId("Array") != UINT32_MAX, "Array getTypeId != UINT32_MAX");
		CHECK(reflCompiler.getTypeId("NonExistentType") == UINT32_MAX, "NonExistentType getTypeId == UINT32_MAX");

		uint32_t userAgeIdByType = reflCompiler.getMemberIdByType(userTypeId, "age");
		CHECK(userAgeIdByType == ageId, "userAgeIdByType == ageId");
		CHECK(reflCompiler.getMemberIdByType(UINT32_MAX, "age") == UINT32_MAX, "getMemberIdByType UINT32_MAX == UINT32_MAX");

		CHECK(std::string_view(reflCompiler.getMemberNameById(ageId)) == "age", "getMemberNameById ageId == age");
		CHECK(std::string_view(reflCompiler.getMemberNameById(UINT32_MAX)) == "", "getMemberNameById UINT32_MAX == empty");

		CHECK(reflCompiler.getMemberCountByType(userTypeId) > 0, "getMemberCountByType userTypeId > 0");
		CHECK(reflCompiler.getMemberCountByType(UINT32_MAX) == 0, "getMemberCountByType UINT32_MAX == 0");

		uint32_t arrayTypeId = reflCompiler.getTypeId("Array");
		uint32_t forEachId = reflCompiler.getMemberIdByType(arrayTypeId, "forEach");
		CHECK(forEachId != UINT32_MAX, "forEachId != UINT32_MAX");
		CHECK(reflCompiler.isMemberTrailingClosure(forEachId), "isMemberTrailingClosure forEachId");

		uint32_t mathTypeId = reflCompiler.getTypeId("Math");
		if (mathTypeId != UINT32_MAX) {
			uint32_t sqrtId = reflCompiler.getMemberIdByType(mathTypeId, "sqrt");
			if (sqrtId != UINT32_MAX) {
				CHECK(!reflCompiler.isMemberTrailingClosure(sqrtId), "!isMemberTrailingClosure sqrtId");
			}
		}

		CHECK(!reflCompiler.isMemberTrailingClosure(ageId), "!isMemberTrailingClosure ageId");
		CHECK(!reflCompiler.isMemberTrailingClosure(UINT32_MAX), "!isMemberTrailingClosure UINT32_MAX");

		CHECK(reflCompiler.isFunctionTrailingClosure(funcId), "isFunctionTrailingClosure funcId");
		CHECK(!reflCompiler.isFunctionTrailingClosure(UINT32_MAX), "!isFunctionTrailingClosure UINT32_MAX");
		CHECK(reflCompiler.getFunctionParamCountById(funcId) == 1, "getFunctionParamCountById funcId == 1");
		CHECK(reflCompiler.getFunctionParamCountById(UINT32_MAX) == -1, "getFunctionParamCountById UINT32_MAX == -1");
		CHECK(std::string_view(reflCompiler.getFunctionReturnTypeById(funcId)) == "Void", "getFunctionReturnTypeById funcId == Void");
		CHECK(std::string_view(reflCompiler.getFunctionReturnTypeById(UINT32_MAX)) == "", "getFunctionReturnTypeById UINT32_MAX == empty");

		uint32_t repeatId = reflCompiler.getFunctionId("repeat");
		if (repeatId != UINT32_MAX) {
			CHECK(reflCompiler.isFunctionTrailingClosure(repeatId), "isFunctionTrailingClosure repeatId");
			CHECK(reflCompiler.isFunctionTrailingClosure(repeatId) == reflCompiler.hasTrailingClosure("repeat"), "repeat isFunctionTrailingClosure == hasTrailingClosure");
			CHECK(reflCompiler.getFunctionParamCountById(repeatId) == reflCompiler.getFunctionParamCount("repeat"), "repeat getFunctionParamCountById == getFunctionParamCount");
			CHECK(std::string_view(reflCompiler.getFunctionReturnTypeById(repeatId)) == reflCompiler.getFunctionReturnType("repeat"), "repeat getFunctionReturnTypeById == getFunctionReturnType");
		}

		// 5. P5 global function and type enumeration
		Autolang::ACompiler catalogCompiler;
		CHECK(catalogCompiler.compile("init_preload.al", ""),
		      "compile built-in preload catalog");
		std::vector<std::string> functionNames;
		for (uint32_t i = 0; i < catalogCompiler.getFunctionCount(); ++i) {
			uint32_t id = catalogCompiler.getFunctionIdAt(i);
			const char *name = catalogCompiler.getFunctionNameById(id);
			CHECK(id != UINT32_MAX, "getFunctionIdAt valid index");
			CHECK(name[0] != '\0', "getFunctionNameById valid id");
			CHECK(catalogCompiler.getFunctionId(name) == id, "function id/name round trip");
			functionNames.emplace_back(name);
		}
		for (const char *name : {"print", "println", "repeat"}) {
			CHECK(std::find(functionNames.begin(), functionNames.end(), name) != functionNames.end(),
			      std::string("global function list contains ") + name);
		}
		CHECK(catalogCompiler.getFunctionId("println") != UINT32_MAX,
		      "println getFunctionId != UINT32_MAX");
		CHECK(!catalogCompiler.isFunctionTrailingClosure(
		          catalogCompiler.getFunctionId("println")),
		      "println is not a trailing closure");
		CHECK(catalogCompiler.isFunctionTrailingClosure(
		          catalogCompiler.getFunctionId("repeat")),
		      "repeat is a trailing closure");
		CHECK(catalogCompiler.getFunctionIdAt(catalogCompiler.getFunctionCount()) ==
		          UINT32_MAX,
		      "getFunctionIdAt out of range");
		CHECK(std::string_view(
		          catalogCompiler.getFunctionNameById(UINT32_MAX)).empty(),
		      "getFunctionNameById invalid id is empty");

		std::vector<std::string> typeNames;
		for (uint32_t i = 0; i < catalogCompiler.getTypeCount(); ++i) {
			uint32_t id = catalogCompiler.getTypeIdAt(i);
			const char *name = catalogCompiler.getTypeNameById(id);
			CHECK(id != UINT32_MAX, "getTypeIdAt valid index");
			CHECK(name[0] != '\0', "getTypeNameById valid id");
			CHECK(catalogCompiler.getTypeId(name) == id,
			      std::string("type id/name round trip for ") + name +
			          " (id=" + std::to_string(id) + ", got=" +
			          std::to_string(catalogCompiler.getTypeId(name)) + ")");
			typeNames.emplace_back(name);
		}
		for (const char *name : {"Math", "Array", "String", "Int"}) {
			CHECK(std::find(typeNames.begin(), typeNames.end(), name) != typeNames.end(),
			      std::string("type list contains ") + name);
		}
		CHECK(catalogCompiler.getTypeIdAt(catalogCompiler.getTypeCount()) ==
		          UINT32_MAX,
		      "getTypeIdAt out of range");
		CHECK(std::string_view(catalogCompiler.getTypeNameById(UINT32_MAX)).empty(),
		      "getTypeNameById invalid id is empty");

		// 6. P4 member enumeration
		for (const char *typeName : {"Array", "String", "Map", "Set", "Math",
		                             "Json", "Time", "File"}) {
			uint32_t typeId = reflCompiler.getTypeId(typeName);
			CHECK(typeId != UINT32_MAX,
			      std::string("P4 type exists: ") + typeName);
			auto expectedMembers = reflCompiler.getMembersForType(typeName);
			uint32_t memberCount = reflCompiler.getMemberCountByType(typeId);
			CHECK(memberCount == expectedMembers.size(),
			      std::string("P4 member count matches legacy API: ") + typeName);
			for (uint32_t i = 0; i < memberCount; ++i) {
				uint32_t memberId = reflCompiler.getMemberIdAt(typeId, i);
				CHECK(memberId != UINT32_MAX,
				      std::string("P4 member id exists: ") + typeName);
				const char *memberName = reflCompiler.getMemberNameById(memberId);
				bool found = false;
				for (const auto &expected : expectedMembers) {
					if (expected == memberName) {
						found = true;
						break;
					}
				}
				CHECK(found, std::string("P4 member set matches legacy API: ") +
				                typeName);
				CHECK(reflCompiler.getMemberIdByType(typeId, memberName) ==
				          memberId,
				      std::string("P4 member id/name round trip: ") + typeName);
			}
			CHECK(reflCompiler.getMemberIdAt(typeId, memberCount) == UINT32_MAX,
			      std::string("P4 member index out of range: ") + typeName);
		}
		CHECK(reflCompiler.getMemberIdAt(UINT32_MAX, 0) == UINT32_MAX,
		      "P4 invalid type id");

		if (ok) {
			passedCount++;
			std::cout << "Passed testReflectionApis\n";
		} else {
			std::cerr << "Failed testReflectionApis\n";
		}
		#undef CHECK
	}

	// -------------------------------------------------------------
	// P7: API phản chiếu metadata đầy đủ (getLibraries, getClasses, getClass, getFunctions, getFunction, getAliases)
	// -------------------------------------------------------------
	{
		totalCount++;
		bool ok = true;
		#define CHECK(cond, msg) \
			if (!(cond)) { \
				std::cerr << "FAIL [testReflectionMetadataApisP7]: " << (msg) << "\n"; \
				ok = false; \
			}

		Autolang::ACompiler p7Compiler;
		// 1. getLibraries trước khi nạp
		auto libsBefore = p7Compiler.getLibraries();
		CHECK(!libsBefore.empty(), "getLibraries is not empty before compile");
		bool foundStdlib = false;
		bool foundStdMath = false;
		for (const auto &lib : libsBefore) {
			if (lib.name == "stdlib") {
				foundStdlib = true;
				CHECK(lib.autoImport == true, "stdlib autoImport is true");
				CHECK(lib.loaded == false, "stdlib loaded is false before compile");
			} else if (lib.name == "std/math") {
				foundStdMath = true;
				CHECK(lib.autoImport == false, "std/math autoImport is false");
				CHECK(lib.loaded == false, "std/math loaded is false before compile");
			}
		}
		CHECK(foundStdlib, "stdlib registered in getLibraries");
		CHECK(foundStdMath, "std/math registered in getLibraries");

		// 2. Biên dịch script kiểm thử P7
		const char *p7Script = R"###(
@import("std/math")
@import("std/time")
class Box<T extends Math, R> {
    val x: Int = 10
    var y: String? = null
    lateinit var z: Array<Int>
    var m: Map<String, Array<Int>>? = null
    fun getX(): Int = x
}

enum Color {
    RED, GREEN, BLUE;
}

var m: Map<String, Array<Int>>? = null
fun customGlobalFunc(a: Int, b: String = "default", mapParam: Map<String, Array<Int>>? = null): Void {}
)###";

		bool compOk = p7Compiler.compile("p7_main.al", p7Script);
		CHECK(compOk, "p7Script compiled successfully");
		CHECK(!p7Compiler.hasError(), "p7Script no compiler errors");

		// Kiểm tra getLibraries sau khi nạp
		auto libsAfter = p7Compiler.getLibraries();
		for (const auto &lib : libsAfter) {
			if (lib.name == "stdlib") {
				CHECK(lib.loaded == true, "stdlib loaded is true after compile");
			} else if (lib.name == "std/math") {
				CHECK(lib.loaded == true, "std/math loaded is true after import");
			} else if (lib.name == "std/time") {
				CHECK(lib.loaded == true, "std/time loaded is true after import");
			}
		}

		// 3. Kiểm tra Math và các hàm nạp chồng
		uint32_t mathId = p7Compiler.getTypeId("Math");
		CHECK(mathId != UINT32_MAX, "Math class id resolved");
		auto mathMetaOpt = p7Compiler.getClass(mathId);
		CHECK(mathMetaOpt.has_value(), "getClass(mathId) found Math");
		if (mathMetaOpt.has_value()) {
			const auto &mathMeta = *mathMetaOpt;
			CHECK(mathMeta.name == "Math", "Math name matches");
			CHECK(mathMeta.library == "stdlib", "Math class library is stdlib");

			auto itSin = mathMeta.functions.find("sin");
			CHECK(itSin != mathMeta.functions.end(), "Math has sin function");
			if (itSin != mathMeta.functions.end()) {
				CHECK(itSin->second.size() >= 2, "Math.sin has at least 2 overloads");
				for (const auto &sinFn : itSin->second) {
					CHECK(sinFn.library == "std/math", "Math.sin library is std/math");
					CHECK(sinFn.ownerClassId.has_value() && *sinFn.ownerClassId == mathId,
					      "Math.sin ownerClassId is Math");
					CHECK(sinFn.isInfix == false, "Math.sin isInfix == false");
					CHECK(sinFn.infixPrecedence == -1, "Math.sin infixPrecedence == -1");
				}
			}

			// Kiểm tra mọi hàm khai báo trong math.cpp có mặt trong ClassMeta của Math
			const std::vector<std::string> mathCppFunctions = {
			    "roundToInt", "roundToLong", "trunc", "truncate", "exp", "log", "ln", "log2", "log10",
			    "sin", "cos", "tan", "asin", "acos", "atan", "atan2", "sinh", "cosh", "tanh",
			    "asinh", "acosh", "atanh", "hypot", "IEEErem", "cbrt", "expm1", "ln1p",
			    "withSign", "nextUp", "nextDown", "nextAfter", "ulp", "fmod", "random"
			};
			for (const auto &fnName : mathCppFunctions) {
				auto itFn = mathMeta.functions.find(fnName);
				CHECK(itFn != mathMeta.functions.end(), std::string("Math has function from math.cpp: ") + fnName);
				if (itFn != mathMeta.functions.end()) {
					bool hasStdMathLib = false;
					for (const auto &fn : itFn->second) {
						if (fn.library == "std/math") {
							hasStdMathLib = true;
							break;
						}
					}
					CHECK(hasStdMathLib, std::string("Math function ") + fnName + " has overload with library std/math");
				}
			}

			// Kiểm tra hàm cơ bản của Math mang library = "stdlib"
			for (const char *basicMathFn : {"abs", "min", "max"}) {
				auto itBasic = mathMeta.functions.find(basicMathFn);
				if (itBasic != mathMeta.functions.end()) {
					for (const auto &fn : itBasic->second) {
						CHECK(fn.library == "stdlib", std::string("Basic Math.") + basicMathFn + " has library stdlib");
					}
				}
			}
		}

		// 4. Kiểm tra hasDefault, isVal, isLateinit
		auto globalFuncs = p7Compiler.getFunctions();
		CHECK(!globalFuncs.empty(), "getFunctions is not empty");
		bool foundPrintln = false;
		bool foundCustomGlobal = false;
		for (const auto &fn : globalFuncs) {
			if (fn.name == "println") {
				foundPrintln = true;
				CHECK(!fn.params.empty(), "println has params");
				if (!fn.params.empty()) {
					CHECK(fn.params[0].hasDefault == true, "println param 0 hasDefault == true");
				}
				CHECK(!fn.ownerClassId.has_value(), "println ownerClassId is nullopt");
				CHECK(fn.isInfix == false, "println isInfix == false");
				CHECK(fn.infixPrecedence == -1, "println infixPrecedence == -1");
			} else if (fn.name == "customGlobalFunc") {
				foundCustomGlobal = true;
				CHECK(fn.params.size() == 3, "customGlobalFunc has 3 params");
				if (fn.params.size() == 3) {
					const auto &pMap = fn.params[2];
					CHECK(pMap.name == "mapParam", "customGlobalFunc param 2 is mapParam");
					CHECK(pMap.hasDefault == true, "customGlobalFunc param 2 hasDefault == true");
					CHECK(pMap.type.name == "Map", "param mapParam type is Map");
					CHECK(pMap.type.nullable == true, "param mapParam is nullable");
					CHECK(pMap.type.inputs.size() == 2, "param mapParam inputs size 2");
					if (pMap.type.inputs.size() == 2) {
						CHECK(pMap.type.inputs[0].name == "String", "param mapParam inputs[0] is String");
						CHECK(pMap.type.inputs[1].name == "Array", "param mapParam inputs[1] is Array");
					}
				}
			}
		}
		CHECK(foundPrintln, "found println in globalFuncs");
		CHECK(foundCustomGlobal, "found customGlobalFunc in globalFuncs");

		// joinToString: 5 tham số đều có mặc định
		uint32_t arrId = p7Compiler.getTypeId("Array");
		auto arrMetaOpt = p7Compiler.getClass(arrId);
		CHECK(arrMetaOpt.has_value(), "getClass(Array) found");
		if (arrMetaOpt.has_value()) {
			auto itJoin = arrMetaOpt->functions.find("joinToString");
			CHECK(itJoin != arrMetaOpt->functions.end(), "Array has joinToString");
			if (itJoin != arrMetaOpt->functions.end() && !itJoin->second.empty()) {
				const auto &joinFn = itJoin->second[0];
				CHECK(joinFn.params.size() == 5, "joinToString has 5 parameters");
				for (size_t i = 0; i < joinFn.params.size(); ++i) {
					CHECK(joinFn.params[i].hasDefault == true,
					      "joinToString param hasDefault == true");
				}
			}

			// withIndex
			auto itWithIndex = arrMetaOpt->functions.find("withIndex");
			CHECK(itWithIndex != arrMetaOpt->functions.end(), "Array has withIndex");
			if (itWithIndex != arrMetaOpt->functions.end() && !itWithIndex->second.empty()) {
				const auto &wiFn = itWithIndex->second[0];
				CHECK(!wiFn.params.empty(), "withIndex has params");
				if (!wiFn.params.empty()) {
					CHECK(wiFn.params[0].hasDefault == true, "withIndex param 0 hasDefault == true");
				}
			}
		}

		// indexOf(str, startIndex = 0): chỉ tham số 2
		uint32_t sbId = p7Compiler.getTypeId("StringBuilder");
		auto sbMetaOpt = p7Compiler.getClass(sbId);
		CHECK(sbMetaOpt.has_value(), "getClass(StringBuilder) found");
		if (sbMetaOpt.has_value()) {
			auto itIdx = sbMetaOpt->functions.find("indexOf");
			CHECK(itIdx != sbMetaOpt->functions.end(), "StringBuilder has indexOf");
			if (itIdx != sbMetaOpt->functions.end()) {
				bool foundStrIndexOf = false;
				for (const auto &fn : itIdx->second) {
					if (fn.params.size() == 2 && fn.params[0].name == "str") {
						foundStrIndexOf = true;
						CHECK(fn.params[0].hasDefault == false, "indexOf param 0 hasDefault == false");
						CHECK(fn.params[1].hasDefault == true, "indexOf param 1 hasDefault == true");
					}
				}
				CHECK(foundStrIndexOf, "found StringBuilder.indexOf(str, startIndex)");
			}
		}

		// Kiểm tra cờ isInfix và infixPrecedence cho Int.downTo và Array.step
		uint32_t intId = p7Compiler.getTypeId("Int");
		auto intMetaOpt = p7Compiler.getClass(intId);
		CHECK(intMetaOpt.has_value(), "getClass(Int) found");
		if (intMetaOpt.has_value()) {
			auto itDownTo = intMetaOpt->functions.find("downTo");
			CHECK(itDownTo != intMetaOpt->functions.end(), "Int has downTo");
			if (itDownTo != intMetaOpt->functions.end() && !itDownTo->second.empty()) {
				for (const auto &fn : itDownTo->second) {
					CHECK(fn.isInfix == true, "Int.downTo isInfix == true");
					CHECK(fn.infixPrecedence == 9, "Int.downTo infixPrecedence == 9");
				}
			}
		}

		if (arrMetaOpt.has_value()) {
			auto itStep = arrMetaOpt->functions.find("step");
			CHECK(itStep != arrMetaOpt->functions.end(), "Array has step");
			if (itStep != arrMetaOpt->functions.end() && !itStep->second.empty()) {
				for (const auto &fn : itStep->second) {
					CHECK(fn.isInfix == true, "Array.step isInfix == true");
					CHECK(fn.infixPrecedence == 8, "Array.step infixPrecedence == 8");
				}
			}
		}

		// 5. Kiểm tra Box: generics <T extends Math, R> và properties
		uint32_t boxId = p7Compiler.getTypeId("Box");
		CHECK(boxId != UINT32_MAX, "Box class id resolved");
		auto boxMetaOpt = p7Compiler.getClass(boxId);
		CHECK(boxMetaOpt.has_value(), "getClass(Box) found");
		if (boxMetaOpt.has_value()) {
			const auto &boxMeta = *boxMetaOpt;
			CHECK(boxMeta.name == "Box", "Box name matches");
			CHECK(boxMeta.library.empty(), "Box library is empty (user code)");

			// generics
			CHECK(boxMeta.generics.size() == 2, "Box has 2 generic params");
			if (boxMeta.generics.size() == 2) {
				CHECK(boxMeta.generics[0].name == "T", "generic 0 is T");
				CHECK(boxMeta.generics[0].mustBeExtends == true, "generic T mustBeExtends is true");
				CHECK(boxMeta.generics[0].extendClassId.has_value() && *boxMeta.generics[0].extendClassId == mathId,
				      "generic T extendClassId is Math");

				CHECK(boxMeta.generics[1].name == "R", "generic 1 is R");
				CHECK(boxMeta.generics[1].mustBeExtends == false, "generic R mustBeExtends is false");
				CHECK(!boxMeta.generics[1].extendClassId.has_value(), "generic R extendClassId is nullopt");
			}

			// properties
			bool foundX = false, foundY = false, foundZ = false, foundM = false;
			for (const auto &p : boxMeta.properties) {
				if (p.name == "x") {
					foundX = true;
					CHECK(p.isVal == true, "Box.x isVal == true");
					CHECK(p.isLateinit == false, "Box.x isLateinit == false");
					CHECK(p.isStatic == false, "Box.x isStatic == false");
					CHECK(p.type.name == "Int", "Box.x type is Int");
					CHECK(p.type.nullable == false, "Box.x is not nullable");
				} else if (p.name == "y") {
					foundY = true;
					CHECK(p.isVal == false, "Box.y isVal == false");
					CHECK(p.isLateinit == false, "Box.y isLateinit == false");
					CHECK(p.type.name == "String", "Box.y type is String");
					CHECK(p.type.nullable == true, "Box.y is nullable");
				} else if (p.name == "z") {
					foundZ = true;
					CHECK(p.isVal == false, "Box.z isVal == false");
					CHECK(p.isLateinit == true, "Box.z isLateinit == true");
					CHECK(p.type.name == "Array", "Box.z type is Array");
					CHECK(p.type.inputs.size() == 1, "Box.z inputs has size 1");
					if (!p.type.inputs.empty()) {
						CHECK(p.type.inputs[0].name == "Int", "Box.z inner type is Int");
					}
				} else if (p.name == "m") {
					foundM = true;
					CHECK(p.isVal == false, "Box.m isVal == false");
					CHECK(p.isLateinit == false, "Box.m isLateinit == false");
					CHECK(p.isStatic == false, "Box.m isStatic == false");
					CHECK(p.type.name == "Map", "Box.m type is Map");
					CHECK(p.type.nullable == true, "Box.m is nullable");
					CHECK(p.type.isGeneric == false, "Box.m is not generic");
					CHECK(p.type.inputs.size() == 2, "Box.m inputs size == 2");
					if (p.type.inputs.size() == 2) {
						CHECK(p.type.inputs[0].name == "String", "Box.m inputs[0] is String");
						CHECK(p.type.inputs[0].nullable == false, "Box.m inputs[0] not nullable");
						CHECK(p.type.inputs[0].inputs.empty(), "Box.m inputs[0] inputs empty");
						CHECK(p.type.inputs[1].name == "Array", "Box.m inputs[1] is Array");
						CHECK(p.type.inputs[1].nullable == false, "Box.m inputs[1] not nullable");
						CHECK(p.type.inputs[1].inputs.size() == 1, "Box.m inputs[1] inputs size 1");
						if (!p.type.inputs[1].inputs.empty()) {
							CHECK(p.type.inputs[1].inputs[0].name == "Int", "Box.m inputs[1][0] is Int");
							CHECK(p.type.inputs[1].inputs[0].nullable == false, "Box.m inputs[1][0] not nullable");
							CHECK(p.type.inputs[1].inputs[0].inputs.empty(), "Box.m inputs[1][0] inputs empty");
						}
					}
				}
			}
			CHECK(foundX, "Box.x property found");
			CHECK(foundY, "Box.y property found");
			CHECK(foundZ, "Box.z property found");
			CHECK(foundM, "Box.m property found");
		}

		// 6. Kiểm tra enumEntries
		uint32_t colorId = p7Compiler.getTypeId("Color");
		CHECK(colorId != UINT32_MAX, "Color enum id resolved");
		auto colorMetaOpt = p7Compiler.getClass(colorId);
		CHECK(colorMetaOpt.has_value(), "getClass(Color) found");
		if (colorMetaOpt.has_value()) {
			const auto &colorMeta = *colorMetaOpt;
			CHECK(colorMeta.enumEntries.size() == 3, "Color enumEntries size == 3");
			std::vector<std::string> expectedColors = {"RED", "GREEN", "BLUE"};
			for (const auto &exp : expectedColors) {
				CHECK(std::find(colorMeta.enumEntries.begin(), colorMeta.enumEntries.end(), exp) !=
				          colorMeta.enumEntries.end(),
				      std::string("Color contains ") + exp);
			}
		}

		// 7. Kiểm tra getAliases()
		auto aliases = p7Compiler.getAliases();
		CHECK(!aliases.empty(), "getAliases is not empty");
		bool foundKotlinMath = false;
		for (const auto &alias : aliases) {
			if (alias.name == "kotlin.math") {
				foundKotlinMath = true;
				CHECK(alias.target == "Math", "kotlin.math target is Math");
			}
		}
		CHECK(foundKotlinMath, "found kotlin.math in getAliases()");

		// 8. Kiểm tra tương thích dữ liệu API cũ
		for (const char *typeName : {"Array", "String", "Map", "Set", "Math", "Json", "Time", "File"}) {
			uint32_t tid = p7Compiler.getTypeId(typeName);
			CHECK(tid != UINT32_MAX, std::string("Type exists: ") + typeName);
			auto cMetaOpt = p7Compiler.getClass(tid);
			CHECK(cMetaOpt.has_value(), std::string("ClassMeta exists: ") + typeName);
			if (cMetaOpt.has_value()) {
				auto expectedMembers = p7Compiler.getMembersForType(typeName);
				for (const auto &expected : expectedMembers) {
					bool hasMember = false;
					for (const auto &p : cMetaOpt->properties) {
						if (p.name == expected) {
							hasMember = true;
							break;
						}
					}
					if (!hasMember) {
						if (cMetaOpt->functions.find(expected) != cMetaOpt->functions.end()) {
							hasMember = true;
						}
					}
					CHECK(hasMember, std::string("P7 ClassMeta contains member: ") + expected + " in " + typeName);
				}
			}
		}

		// 9. Không thấy id -> None/nullopt, không ném lỗi
		auto nonExistentClass = p7Compiler.getClass(999999);
		CHECK(!nonExistentClass.has_value(), "getClass invalid id returns nullopt");
		auto nonExistentFunc = p7Compiler.getFunction(999999);
		CHECK(!nonExistentFunc.has_value(), "getFunction invalid id returns nullopt");

		if (ok) {
			passedCount++;
			std::cout << "Passed testReflectionMetadataApisP7\n";
		} else {
			std::cerr << "Failed testReflectionMetadataApisP7\n";
		}
		#undef CHECK
	}

	totalCount++;
	{
		bool ok = true;
		#define CHECK_INT(cond, msg) \
			if (!(cond)) { \
				std::cerr << "FAIL [testIntrinsicFunctionsReflection]: " << (msg) << "\n"; \
				ok = false; \
			}

		Autolang::ACompiler compiler;
		auto intrinsics = compiler.getIntrinsicFunctions();
		CHECK_INT(!intrinsics.empty(), "getIntrinsicFunctions is not empty");
		CHECK_INT(intrinsics.size() == 39, "getIntrinsicFunctions contains 39 intrinsics");

		ankerl::unordered_dense::map<std::string, Autolang::IntrinsicMeta> map;
		for (const auto &item : intrinsics) {
			map[item.name] = item;
		}

		// 1. listOf
		CHECK_INT(map.find("listOf") != map.end(), "listOf exists");
		if (map.find("listOf") != map.end()) {
			CHECK_INT(map["listOf"].returnClass == "Array", "listOf returnClass is Array");
			CHECK_INT(map["listOf"].elementFromArguments == true, "listOf elementFromArguments is true");
			CHECK_INT(map["listOf"].minArgs == 0, "listOf minArgs is 0");
			CHECK_INT(map["listOf"].maxArgs == -1, "listOf maxArgs is -1");
		}

		// 2. emptyList
		CHECK_INT(map.find("emptyList") != map.end(), "emptyList exists");
		if (map.find("emptyList") != map.end()) {
			CHECK_INT(map["emptyList"].returnClass == "Array", "emptyList returnClass is Array");
			CHECK_INT(map["emptyList"].elementFromArguments == false, "emptyList elementFromArguments is false");
			CHECK_INT(map["emptyList"].minArgs == 0, "emptyList minArgs is 0");
			CHECK_INT(map["emptyList"].maxArgs == 0, "emptyList maxArgs is 0");
		}

		// 3. mapOf
		CHECK_INT(map.find("mapOf") != map.end(), "mapOf exists");
		if (map.find("mapOf") != map.end()) {
			CHECK_INT(map["mapOf"].returnClass == "Map", "mapOf returnClass is Map");
			CHECK_INT(map["mapOf"].elementFromArguments == true, "mapOf elementFromArguments is true");
			CHECK_INT(map["mapOf"].minArgs == 0, "mapOf minArgs is 0");
			CHECK_INT(map["mapOf"].maxArgs == -1, "mapOf maxArgs is -1");
		}

		// 4. setOf
		CHECK_INT(map.find("setOf") != map.end(), "setOf exists");
		if (map.find("setOf") != map.end()) {
			CHECK_INT(map["setOf"].returnClass == "Set", "setOf returnClass is Set");
			CHECK_INT(map["setOf"].elementFromArguments == true, "setOf elementFromArguments is true");
			CHECK_INT(map["setOf"].minArgs == 0, "setOf minArgs is 0");
			CHECK_INT(map["setOf"].maxArgs == -1, "setOf maxArgs is -1");
		}

		// 5. intArrayOf
		CHECK_INT(map.find("intArrayOf") != map.end(), "intArrayOf exists");
		if (map.find("intArrayOf") != map.end()) {
			CHECK_INT(map["intArrayOf"].returnClass == "Array", "intArrayOf returnClass is Array");
			CHECK_INT(map["intArrayOf"].elementFromArguments == true, "intArrayOf elementFromArguments is true");
			CHECK_INT(map["intArrayOf"].minArgs == 0, "intArrayOf minArgs is 0");
			CHECK_INT(map["intArrayOf"].maxArgs == -1, "intArrayOf maxArgs is -1");
		}

		// 6. Đối chiếu với kết quả compile thực tế của compiler
		const char *testScript = R"###(
val resListOf = listOf(1, 2)
val resEmptyList = emptyList<Int>()
val resMapOf = mapOf(1 to 2)
val resSetOf = setOf(1, 2)
val resIntArrayOf = intArrayOf(1, 2)
val resPairOf = pairOf(1, 2)
val resSetOfNotNull = setOfNotNull(1, null)
)###";
		bool compOk = compiler.compile("test_intrinsics.al", testScript);
		CHECK_INT(compOk, "compiler compiled testScript successfully");

		auto checkVarClass = [&](const std::string &varName, const std::string &expectedClass) {
			uint32_t vid = compiler.getGlobalVariableId(varName);
			CHECK_INT(vid != UINT32_MAX, varName + " variable found");
			std::string typeStr = compiler.getGlobalVariableType(vid);
			CHECK_INT(typeStr.rfind(expectedClass, 0) == 0, varName + " resolved type begins with " + expectedClass + ", got: " + typeStr);
		};

		checkVarClass("resListOf", map["listOf"].returnClass);
		checkVarClass("resEmptyList", map["emptyList"].returnClass);
		checkVarClass("resMapOf", map["mapOf"].returnClass);
		checkVarClass("resSetOf", map["setOf"].returnClass);
		checkVarClass("resIntArrayOf", map["intArrayOf"].returnClass);
		checkVarClass("resPairOf", map["pairOf"].returnClass);
		checkVarClass("resSetOfNotNull", map["setOfNotNull"].returnClass);

		if (ok) {
			passedCount++;
			std::cout << "Passed testIntrinsicFunctionsReflection\n";
		} else {
			std::cerr << "Failed testIntrinsicFunctionsReflection\n";
		}
		#undef CHECK_INT
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

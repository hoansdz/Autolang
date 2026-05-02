#include <Autolang.hpp>
#include <functional>
#include <iostream>

#ifdef _WIN32

#include <windows.h>
#include <psapi.h>

void printMemoryUsage() {
	PROCESS_MEMORY_COUNTERS info;

	GetProcessMemoryInfo(GetCurrentProcess(), &info, sizeof(info));

	std::cout << "RAM used: " << info.WorkingSetSize / (float)(1024 * 1024)
	          << " MB\n";

	std::cout << "Peak RAM: " << info.PeakWorkingSetSize / (float)(1024 * 1024)
	          << " MB\n";
}

#endif

int main(int argc, char *argv[]) {
	auto start = std::chrono::high_resolution_clock::now();
	try {
		try {
			for (int i = 0; i < 3; ++i) {
				AutoLang::ACompiler compiler;
				// ANativeMap nativeMap = {
				// 	{"hi", (ANativeFunction)[](NativeFuncInput) ->
				// AutoLang::AObject * { 		std::cerr << "Hello world!!!\n";
				// return nullptr;
				// 	}}};
				// compiler.setOnWarning(new AutoLang::FunctionEvent(
				//     [](std::string_view message) -> void {

				//     }));
				// if (compiler.compile("./tests/test.atl")) {
				// 	compiler.run();
				// 	compiler.refresh();
				// }
				if (compiler.compile("./tests/testCorrectness.atl")) {
					compiler.run();
					compiler.refresh();
				}
				if (compiler.compile("./tests/testCorrectness.atl")) {
					compiler.run();
					compiler.refresh();
				}
				// if (compiler.compile("./tests/testCorrectness.atl")) {
				// 	compiler.run();
				// }
			}
		} catch (const std::logic_error &err) {
			std::cerr << err.what();
		}
		// if (compiler.vm->state != VMState::ERROR) {
		// 	compiler.vm->start();
		// } else {
		// compiler.vm->log();
		// }
	} catch (const std::exception &e) {
		std::cerr << e.what() << '\n';
	}
	auto end = std::chrono::high_resolution_clock::now();
	auto duration =
	    std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	std::cout << '\n' << "Total time : " << duration.count() << " ms" <<
	'\n';
#ifdef _WIN32
	printMemoryUsage();
#endif
}
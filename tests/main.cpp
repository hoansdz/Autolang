#define AUTOLANG_LIMIT_OPCODE
#define NO_INCLUDE_LIBS_HTTP
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
			for (int i = 0; i < 1; ++i) {
				AutoLang::ACompiler compiler;
				compiler.setLimitOpcodeCount(1000000);
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
				unsigned char raw_bytes[] = {
				    64,  105, 109, 112, 111, 114, 116, 40,  34,  99,  111, 109,
				    112, 97,  110, 121, 47,  112, 114, 111, 100, 117, 99,  116,
				    115, 34,  41,  10,  10,  118, 97,  108, 32,  112, 114, 111,
				    100, 117, 99,  116, 115, 32,  61,  32,  68,  97,  116, 97,
				    98,  97,  115, 101, 46,  103, 101, 116, 95,  112, 114, 111,
				    100, 117, 99,  116, 115, 40,  41,  32,  10,  118, 97,  108,
				    32,  98,  117, 100, 103, 101, 116, 32,  61,  32,  49,  48,
				    48,  48,  10,  10,  47,  47,  32,  100, 112, 91,  105, 93,
				    32,  115, 225, 186, 189, 32,  108, 198, 176, 117, 32,  116,
				    114, 225, 187, 141, 110, 103, 32,  108, 198, 176, 225, 187,
				    163, 110, 103, 32,  108, 225, 187, 155, 110, 32,  110, 104,
				    225, 186, 165, 116, 32,  196, 145, 225, 186, 161, 116, 32,
				    196, 144, 198, 176, 225, 187, 163, 99,  32,  118, 225, 187,
				    155, 105, 32,  110, 103, 195, 162, 110, 32,  115, 195, 161,
				    99,  104, 32,  105, 32,  10,  118, 97,  114, 32,  100, 112,
				    32,  61,  32,  65,  114, 114, 97,  121, 60,  70,  108, 111,
				    97,  116, 62,  40,  41,  10,  102, 111, 114, 32,  40,  105,
				    32,  105, 110, 32,  48,  46,  46,  98,  117, 100, 103, 101,
				    116, 41,  32,  123, 10,  32,  32,  32,  32,  100, 112, 46,
				    97,  100, 100, 40,  48,  46,  48,  41,  10,  125, 10,  10,
				    47,  47,  32,  84,  104, 117, 225, 186, 173, 116, 32,  116,
				    111, 195, 161, 110, 32,  75,  110, 97,  112, 115, 97,  99,
				    107, 32,  99,  104, 111, 32,  112, 104, 195, 169, 112, 32,
				    109, 117, 97,  32,  110, 104, 105, 225, 187, 129, 117, 32,
				    115, 225, 186, 163, 110, 103, 32,  112, 104, 225, 186, 169,
				    109, 32,  109, 225, 187, 151, 105, 32,  108, 111, 225, 186,
				    161, 105, 32,  40,  85,  110, 98,  111, 117, 110, 100, 101,
				    100, 32,  75,  110, 97,  112, 115, 97,  99,  107, 41,  32,
				    10,  102, 111, 114, 32,  40,  112, 32,  105, 110, 32,  112,
				    114, 111, 100, 117, 99,  116, 115, 41,  32,  123, 32,  10,
				    32,  32,  32,  32,  105, 102, 32,  40,  112, 46,  105, 110,
				    83,  116, 111, 99,  107, 41,  32,  123, 32,  10,  32,  32,
				    32,  32,  32,  32,  32,  32,  102, 111, 114, 32,  40,  98,
				    32,  105, 110, 32,  112, 46,  112, 114, 105, 99,  101, 46,
				    46,  98,  117, 100, 103, 101, 116, 41,  32,  123, 32,  10,
				    32,  32,  32,  32,  32,  32,  32,  32,  32,  32,  32,  32,
				    118, 97,  108, 32,  119, 101, 105, 103, 104, 116, 87,  105,
				    116, 104, 80,  114, 111, 100, 117, 99,  116, 32,  61,  32,
				    100, 112, 91,  98,  32,  45,  32,  112, 46,  112, 114, 105,
				    99,  101, 93,  32,  43,  32,  112, 46,  119, 101, 105, 103,
				    104, 116, 32,  10,  32,  32,  32,  32,  32,  32,  32,  32,
				    32,  32,  32,  32,  105, 102, 32,  40,  119, 101, 105, 103,
				    104, 116, 87,  105, 116, 104, 80,  114, 111, 100, 117, 99,
				    116, 32,  62,  32,  100, 112, 91,  98,  93,  41,  32,  123,
				    32,  10,  32,  32,  32,  32,  32,  32,  32,  32,  32,  32,
				    32,  32,  32,  32,  32,  32,  100, 112, 91,  98,  93,  32,
				    61,  32,  119, 101, 105, 103, 104, 116, 87,  105, 116, 104,
				    80,  114, 111, 100, 117, 99,  116, 32,  10,  32,  32,  32,
				    32,  32,  32,  32,  32,  32,  32,  32,  32,  125, 32,  10,
				    32,  32,  32,  32,  32,  32,  32,  32,  125, 32,  10,  32,
				    32,  32,  32,  125, 32,  10,  125, 10,  10,  112, 114, 105,
				    110, 116, 108, 110, 40,  34,  84,  114, 225, 187, 141, 110,
				    103, 32,  108, 198, 176, 225, 187, 163, 110, 103, 32,  116,
				    225, 187, 145, 105, 32,  196, 145, 97,  58,  32,  36,  123,
				    100, 112, 91,  98,  117, 100, 103, 101, 116, 93,  125, 34,
				    41,  0};

				// Tạo con trỏ kiểu char* trỏ trực tiếp vào vùng mảng byte thô
				char *source_code = reinterpret_cast<char *>(raw_bytes);
				compiler.registerBuiltInLibrary("company/products", R"###(
					@import("std/json")
					class Product (
						val name: String,
						val price: Int,
						val weight: Float,
						val inStock: Bool
					)
					class Database {
						// Returns a raw JSON string containing mock data for 10 vegetable products
						static private func _get_products(): String {
							return "{\"data\":[{\"name\":\"Dalat Carrot\",\"price\":25,\"weight\":0.5,\"inStock\":true},{\"name\":\"Highland Potato\",\"price\":30,\"weight\":1.0,\"inStock\":false},{\"name\":\"Broccoli\",\"price\":45,\"weight\":0.4,\"inStock\":true},{\"name\":\"Cherry Tomato\",\"price\":35,\"weight\":0.3,\"inStock\":true},{\"name\":\"Onion\",\"price\":18,\"weight\":1.0,\"inStock\":false},{\"name\":\"Pumpkin\",\"price\":22,\"weight\":1.5,\"inStock\":true},{\"name\":\"Purple Cabbage\",\"price\":40,\"weight\":0.8,\"inStock\":true},{\"name\":\"Old Ginger\",\"price\":60,\"weight\":0.2,\"inStock\":false},{\"name\":\"Seedless Lime\",\"price\":15,\"weight\":0.5,\"inStock\":true},{\"name\":\"Lyson Garlic\",\"price\":120,\"weight\":0.2,\"inStock\":true}]}"
						}

						// Parses JSON and returns an Array of Product instances
						static func get_products(): Array<Product> {
							val jsonStr = _get_products()
							val json = Json.parse(jsonStr)
							val products = json["data"]
							return jsonToArrayClass<Product>(products)
						}
					}
				)###");
				// if (compiler.compile(
				//         "./tests/source.atl", source_code,
				//         AutoLang::LibraryConfig(false, true, true))) {
				// 	compiler.run();
				// 	compiler.refresh();
				// }
				if (compiler.compile(
				        "./tests/a.atl", 
						// source_code,
				        AutoLang::LibraryConfig(false, true, true))) {
					compiler.run();
					compiler.refresh();
				}
				// if (compiler.compile("./tests/testCorrectness.atl")) {
				// 	compiler.run();
				// 	compiler.refresh();
				// }
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
	std::cout << '\n' << "Total time : " << duration.count() << " ms" << '\n';
#ifdef _WIN32
	printMemoryUsage();
#endif
}
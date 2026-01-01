#include <emscripten/emscripten.h>
#include <iostream>
#include <Autolang.hpp>
#include <string>

extern "C" {
	EMSCRIPTEN_KEEPALIVE
	void runCompiler(const char* str) {
		try{
			auto pair = std::pair<const char*, size_t>(str, std::strlen(str));
			AVM i = AVM(pair, false);
		}
		catch (const std::exception& e) {
			std::cerr<<e.what()<<'\n';
		}
	}
}
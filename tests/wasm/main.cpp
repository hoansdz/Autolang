#include <emscripten/emscripten.h>
#include <iostream>
#include <Autolang.hpp>
#include <string>

extern "C" {
	EMSCRIPTEN_KEEPALIVE
	void runCompiler(const char* path, const char* data) {
		try{
			if (!path) {
				std::cerr<<"Path must non null"<<std::endl;
				return;
			}
			if (!data) {
				std::cerr<<"Data must non null"<<std::endl;
				return;
			}
			AVMReadFileMode mode = {
				path,
				data,
				std::strlen(data),
				false
			};
			AVM i = AVM(mode, false);
		}
		catch (const std::exception& e) {
			std::cerr<<e.what()<<'\n';
		}
	}
}
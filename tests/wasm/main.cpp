#include "shared/ANativeFunctionData.hpp"
#include <Autolang.hpp>
#include <emscripten/bind.h>
#include <emscripten/emscripten.h>
#include <iostream>
#include <string>

using namespace emscripten;

class CompilerWrapper {
  public:
	AutoLang::ACompiler compiler;

	CompilerWrapper() {}

	void setOnError(val func) {
		if (!func.as<bool>()) {
			compiler.setOnError(nullptr);
			return;
		}
		compiler.setOnError(new AutoLang::FunctionEvent(func));
	}

	void setOnWarning(val func) {
		if (!func.as<bool>()) {
			compiler.setOnWarning(nullptr);
			return;
		}
		compiler.setOnWarning(new AutoLang::FunctionEvent(func));
	}

	void registerBuiltInLibrary(std::string name, std::string data,
	                            bool autoImport, val mapFunction) {
		if (!mapFunction.as<bool>()) {
			compiler.registerBuiltInLibrary(name.c_str(), data.c_str(),
			                                autoImport);
			return;
		}
		val keys = val::global("Object").call<val>("keys", mapFunction);
		int length = keys["length"].as<int>();
		ANativeMap nativeMap;
		nativeMap.reserve(length);
		for (int i = 0; i < length; ++i) {
			std::string key = keys[i].as<std::string>();
			val value = mapFunction[key];

			if (!value.as<bool>()) {
				continue;
			}

			if (value.typeOf().as<std::string>() == "function") {
				nativeMap[key] = new val(value);
			}
		}
		auto lib = compiler.registerBuiltInLibrary(name.c_str(), data.c_str(),
		                                           autoImport, nativeMap);
		lib->flags |= AutoLang::LibraryFlags::IS_JS_BRIDGE;
	}

	bool compileAndRun(std::string path, std::string data) {
		try {
			if (compiler.compile(path.c_str(), data.c_str())) {
				compiler.run();
				compiler.refresh();
				return true;
			}
		} catch (const std::exception &e) {
			std::cout << e.what() << "\n";
		}
		compiler.refresh();
		return false;
	}

	bool compile(std::string path, std::string data) {
		return compiler.compile(path.c_str(), data.c_str());
	}

	void refresh() { compiler.refresh(); }

	bool run() {
		try {
			compiler.run();
			compiler.refresh();
		} catch (const std::exception &e) {
			return false;
		}
		return true;
	}

	bool hasError() { return compiler.hasError(); }
};

EMSCRIPTEN_BINDINGS(autolang_module) {
	class_<CompilerWrapper>("ACompiler")
	    .constructor<>()
	    .function("compileAndRun", &CompilerWrapper::compileAndRun)
	    .function("run", &CompilerWrapper::run)
	    .function("compile", &CompilerWrapper::compile)
	    .function("refresh", &CompilerWrapper::refresh)
		.function("setOnError", &CompilerWrapper::setOnError)
		.function("setOnWarning", &CompilerWrapper::setOnWarning)
	    .function("registerBuiltInLibrary",
	              &CompilerWrapper::registerBuiltInLibrary)
	    .function("hasError", &CompilerWrapper::hasError);
}

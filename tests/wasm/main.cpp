#define AUTOLANG_LIMIT_OPCODE

#include "shared/ANativeFunctionData.hpp"
#include <Autolang.hpp>
#include <cstdio>
#include <emscripten/bind.h>
#include <emscripten/emscripten.h>
#include <iostream>
#include <string>

using namespace emscripten;

class CompilerWrapper {
  public:
	AutoLang::ACompiler compiler;
	AutoLang::LibraryConfig mainSourceConfig;
	std::stringstream buffer;

	CompilerWrapper() { setvbuf(stderr, NULL, _IONBF, 0); }

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
	                            bool autoImport, bool allowLateinitKeyword,
	                            bool allowNonNullAssertion, val mapFunction) {
		if (!mapFunction.as<bool>()) {
			compiler.registerBuiltInLibrary(
			    name.c_str(), data.c_str(),
			    AutoLang::LibraryConfig(autoImport, allowLateinitKeyword,
			                            allowNonNullAssertion));
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
		auto lib = compiler.registerBuiltInLibrary(
		    name.c_str(), data.c_str(),
		    AutoLang::LibraryConfig(autoImport, allowLateinitKeyword,
		                            allowNonNullAssertion),
		    nativeMap);
		lib->flags |= AutoLang::LibraryFlags::IS_JS_BRIDGE;
	}

	void setLimitOpcodeCount(uint32_t count) {
		compiler.setLimitOpcodeCount(count);
	}

	void setMainSourceConfig(bool allowLateinitKeyword,
	                         bool allowNonNullAssertion) {
		mainSourceConfig.allowLateinitKeyword = allowLateinitKeyword;
		mainSourceConfig.allowNonNullAssertion = allowNonNullAssertion;
	}

	bool compileAndRun(std::string path, std::string data) {
		std::streambuf *old = std::cerr.rdbuf(buffer.rdbuf());
		try {
			if (compiler.compile(path.c_str(), data.c_str(),
			                     mainSourceConfig)) {
				compiler.run();
				compiler.refresh();
				std::cerr.rdbuf(old);
				return true;
			}
		} catch (const std::exception &e) {
			std::cerr << e.what() << "\n";
		}
		compiler.refresh();
		std::cerr.rdbuf(old);
		return false;
	}

	bool compile(std::string path, std::string data) {
		std::streambuf *old = std::cerr.rdbuf(buffer.rdbuf());
		try {
			bool result =
			    compiler.compile(path.c_str(), data.c_str(), mainSourceConfig);
			std::cerr.rdbuf(old);
			return result;
		} catch (const std::exception &e) {
			std::cerr.rdbuf(old);
			return false;
		}
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

	std::string getOutput() {
		// std::string s = buffer.str();
		// if (!s.empty() && s.back() != '\n') {
		// 	s += '\n';
		// }
		return buffer.str();
	}

	void clearOutput() {
		buffer.str("");
		buffer.clear();
	}

	bool hasCompilerError() { return compiler.hasError(); }

	bool hasException() {
		if (compiler.vm.callFrames.getSize() == 0) {
			return false;
		}
		return compiler.vm.callFrames.top()->exception;
	}

	val getException() {
		if (compiler.vm.callFrames.getSize() == 0 ||
		    !compiler.vm.callFrames.top()->exception) {
			return val::null();
		}
		val obj = val::object();
		obj.set("message", std::string(compiler.vm.callFrames.top()
		                                   ->exception->member->data[0]
		                                   ->str->data));
		return obj;
	}

	void throwException(std::string message) {
		if (compiler.vm.callFrames.getSize() == 0) {
			return;
		}
		compiler.vm.notifier->throwException(message);
	}
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
	    .function("setMainSourceConfig", &CompilerWrapper::setMainSourceConfig)
	    .function("setLimitOpcodeCount", &CompilerWrapper::setLimitOpcodeCount)
	    .function("getOutput", &CompilerWrapper::getOutput)
	    .function("clearOutput", &CompilerWrapper::clearOutput)
	    .function("registerBuiltInLibrary",
	              &CompilerWrapper::registerBuiltInLibrary)
	    .function("hasCompilerError", &CompilerWrapper::hasCompilerError)
	    .function("throwException", &CompilerWrapper::throwException)
	    .function("getException", &CompilerWrapper::getException)
	    .function("hasException", &CompilerWrapper::hasException);
}

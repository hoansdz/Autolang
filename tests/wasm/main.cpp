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
	Autolang::ACompiler compiler;
	Autolang::LibraryConfig mainSourceConfig;
	std::stringstream buffer;
#ifndef NO_INCLUDE_LIBS_HTTP
	std::vector<Autolang::AllowRule> pendingDomainRules;
#endif
#ifndef NO_INCLUDE_LIBS_FILE
	std::vector<Autolang::AllowRule> pendingPathRules;
#endif

	CompilerWrapper(bool addStdFile, bool addStdRegex, bool addStdJson,
	                bool addStdHttp, bool addStdMath, bool addStdBytes,
	                bool addStdDate)
	    : compiler(Autolang::ACompilerConfig{.addStdFile = addStdFile,
	                                         .addStdRegex = addStdRegex,
	                                         .addStdJson = addStdJson,
	                                         .addStdMath = addStdMath,
	                                         .addStdDate = addStdDate,
	                                         .addStdBytes = addStdBytes,
	                                         .addStdHttp = addStdHttp}) {
		setvbuf(stderr, NULL, _IONBF, 0);
	}

	void setOnError(val func) {
		if (!func.as<bool>()) {
			compiler.setOnError(nullptr);
			return;
		}
		compiler.setOnError(new Autolang::FunctionEvent(func));
	}

	void setOnWarning(val func) {
		if (!func.as<bool>()) {
			compiler.setOnWarning(nullptr);
			return;
		}
		compiler.setOnWarning(new Autolang::FunctionEvent(func));
	}

	void registerBuiltInLibrary(std::string name, std::string data,
	                            bool autoImport, bool allowLateinitKeyword,
	                            bool allowNonNullAssertion, val mapFunction) {
		if (!mapFunction.as<bool>()) {
			compiler.registerBuiltInLibrary(
			    name.c_str(), data.c_str(),
			    Autolang::LibraryConfig(autoImport, allowLateinitKeyword,
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
		    Autolang::LibraryConfig(autoImport, allowLateinitKeyword,
		                            allowNonNullAssertion),
		    nativeMap);
		lib->flags |= Autolang::LibraryFlags::IS_JS_BRIDGE;
	}

	void setLimitOpcodeCount(uint32_t count) {
		compiler.setLimitOpcodeCount(count);
	}

	uint32_t getLimitOpcodeCount() { return compiler.getLimitOpcodeCount(); }

	// ---- Domain whitelist builder (avoids emscripten::val as parameter) ----
	void clearDomainRules() {
#ifndef NO_INCLUDE_LIBS_HTTP
		pendingDomainRules.clear();
#endif
	}
	void addDomainRule(int type, const std::string &value) {
#ifndef NO_INCLUDE_LIBS_HTTP
		pendingDomainRules.push_back(
		    {type == 0 ? Autolang::AllowRuleType::PLAIN_PREFIX
		               : Autolang::AllowRuleType::REGEX,
		     value});
#endif
	}
	void applyDomainRules() {
#ifndef NO_INCLUDE_LIBS_HTTP
		compiler.setAllowedDomainsRules(pendingDomainRules);
		pendingDomainRules.clear();
#endif
	}

	void setAllowFileRead(bool allow) {
#ifndef NO_INCLUDE_LIBS_FILE
		compiler.setAllowFileRead(allow);
#endif
	}

	void setAllowFileWrite(bool allow) {
#ifndef NO_INCLUDE_LIBS_FILE
		compiler.setAllowFileWrite(allow);
#endif
	}

	void setAllowFileDelete(bool allow) {
#ifndef NO_INCLUDE_LIBS_FILE
		compiler.setAllowFileDelete(allow);
#endif
	}

	// ---- File path whitelist builder (avoids emscripten::val as parameter)
	// ----
	void clearPathRules() {
#ifndef NO_INCLUDE_LIBS_FILE
		pendingPathRules.clear();
#endif
	}
	void addPathRule(int type, const std::string &value) {
#ifndef NO_INCLUDE_LIBS_FILE
		pendingPathRules.push_back({type == 0
		                                ? Autolang::AllowRuleType::PLAIN_PREFIX
		                                : Autolang::AllowRuleType::REGEX,
		                            value});
#endif
	}
	void applyPathRules() {
#ifndef NO_INCLUDE_LIBS_FILE
		compiler.setAllowedFilePathsRules(pendingPathRules);
		pendingPathRules.clear();
#endif
	}

	void setFileBasePath(const std::string &path) {
#ifndef NO_INCLUDE_LIBS_FILE
		compiler.setFileBasePath(path);
#endif
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
			}
			compiler.refresh();
			std::cerr.rdbuf(old);
			return true;
		} catch (const std::exception &e) {
			std::cerr << e.what() << "\n";
		}
		compiler.refresh();
		std::cerr.rdbuf(old);
		return false;
	}

	void loadBuiltInFunctions() {
		if (!compiler.loadedBuiltIn) {
			loadBuiltInFunctions();
		}
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
		} catch (const std::exception &e) {
			compiler.refresh();
			return false;
		}
		compiler.refresh();
		return true;
	}

	std::string getOutput() {
		// std::string s = buffer.str();
		// if (!s.empty() && s.back() != '\n') {
		// 	s += '\n';
		// }
		return buffer.str();
	}

	void setOutput(std::string output) {
		buffer.str(output);
		buffer.clear();
	}

	void clearOutput() {
		buffer.str("");
		buffer.clear();
	}

	bool hasCompilerError() { return compiler.hasError(); }

	bool hasException() {
		if (compiler.vm.callFrames.getSize() == 0) {
			if (compiler.vm.callFrames.objects[0].exception) {
				return true;
			}
			return false;
		}
		return false;
	}

	val getException() {
		if (compiler.vm.callFrames.getSize() == 0) {
			if (compiler.vm.callFrames.objects[0].exception) {
				val obj = val::object();
				auto exception = compiler.exceptionMessage;
				obj.set("message", std::string(exception));
				return obj;
			}
			return val::null();
		}
		return val::null();
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
	    .constructor<bool, bool, bool, bool, bool, bool, bool>()
	    .function("compileAndRun", &CompilerWrapper::compileAndRun)
	    .function("run", &CompilerWrapper::run)
	    .function("compile", &CompilerWrapper::compile)
	    .function("refresh", &CompilerWrapper::refresh)
	    .function("setOnError", &CompilerWrapper::setOnError)
	    .function("setOnWarning", &CompilerWrapper::setOnWarning)
	    .function("setMainSourceConfig", &CompilerWrapper::setMainSourceConfig)
	    .function("setLimitOpcodeCount", &CompilerWrapper::setLimitOpcodeCount)
	    .function("getLimitOpcodeCount", &CompilerWrapper::getLimitOpcodeCount)
	    .function("clearDomainRules", &CompilerWrapper::clearDomainRules)
	    .function("addDomainRule", &CompilerWrapper::addDomainRule)
	    .function("applyDomainRules", &CompilerWrapper::applyDomainRules)
	    .function("loadBuiltInLibraries",
	              &CompilerWrapper::loadBuiltInFunctions)

	    .function("setAllowFileRead", &CompilerWrapper::setAllowFileRead)
	    .function("setAllowFileWrite", &CompilerWrapper::setAllowFileWrite)
	    .function("setAllowFileDelete", &CompilerWrapper::setAllowFileDelete)
	    .function("clearPathRules", &CompilerWrapper::clearPathRules)
	    .function("addPathRule", &CompilerWrapper::addPathRule)
	    .function("applyPathRules", &CompilerWrapper::applyPathRules)
	    .function("setFileBasePath", &CompilerWrapper::setFileBasePath)
	    .function("getOutput", &CompilerWrapper::getOutput)
	    .function("setOutput", &CompilerWrapper::setOutput)
	    .function("clearOutput", &CompilerWrapper::clearOutput)
	    .function("registerBuiltInLibrary",
	              &CompilerWrapper::registerBuiltInLibrary)
	    .function("hasCompilerError", &CompilerWrapper::hasCompilerError)
	    .function("throwException", &CompilerWrapper::throwException)
	    .function("getException", &CompilerWrapper::getException)
	    .function("hasException", &CompilerWrapper::hasException);
}

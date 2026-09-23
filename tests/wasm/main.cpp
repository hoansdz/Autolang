#ifndef AUTOLANG_LIMIT_OPCODE
#define AUTOLANG_LIMIT_OPCODE
#endif

#include "backend/vm/ANotifier.hpp"
#include "shared/ANativeFunctionData.hpp"
#include <Autolang.hpp>
#include <cstdio>
#include <emscripten/bind.h>
#include <emscripten/emscripten.h>
#include <iostream>
#include <sstream>
#include <string>
#include <third_party/nlohmann/json.hpp>

using namespace emscripten;

static Autolang::ACompilerConfig parseCompilerConfig(const nlohmann::json &j) {
	Autolang::ACompilerConfig cfg;
	if (j.contains("strictMode") && j["strictMode"].is_boolean()) {
		cfg.strictMode = j["strictMode"].get<bool>();
	}
	if (j.contains("showWarnings") && j["showWarnings"].is_boolean()) {
		cfg.showWarnings = j["showWarnings"].get<bool>();
	} else if (j.contains("enableWarnings") && j["enableWarnings"].is_boolean()) {
		cfg.showWarnings = j["enableWarnings"].get<bool>();
	}
	if (j.contains("compatibility") && j["compatibility"].is_object()) {
		const auto &compat = j["compatibility"];
		if (compat.contains("strictMode") && compat["strictMode"].is_boolean()) {
			cfg.strictMode = compat["strictMode"].get<bool>();
		}
		if (compat.contains("allowValReassign") && compat["allowValReassign"].is_boolean()) {
			if (compat["allowValReassign"].get<bool>()) {
				cfg.strictMode = false;
			}
		}
		if (compat.contains("enableKotlinCompat") && compat["enableKotlinCompat"].is_boolean()) {
			cfg.enableKotlinCompat = compat["enableKotlinCompat"].get<bool>();
		}
	}
	cfg.autoCloseBracketsOnEof = !cfg.strictMode;
	cfg.allowImplicitVarDeclaration = !cfg.strictMode;
	if (j.contains("compatibility") && j["compatibility"].is_object()) {
		const auto &compat = j["compatibility"];
		if (compat.contains("autoCloseBracketsOnEof") && compat["autoCloseBracketsOnEof"].is_boolean()) {
			cfg.autoCloseBracketsOnEof = compat["autoCloseBracketsOnEof"].get<bool>();
		}
		if (compat.contains("allowImplicitVarDeclaration") && compat["allowImplicitVarDeclaration"].is_boolean()) {
			cfg.allowImplicitVarDeclaration = compat["allowImplicitVarDeclaration"].get<bool>();
		}
	}
	if (j.contains("autoCloseBracketsOnEof") && j["autoCloseBracketsOnEof"].is_boolean()) {
		cfg.autoCloseBracketsOnEof = j["autoCloseBracketsOnEof"].get<bool>();
	}
	if (j.contains("allowImplicitVarDeclaration") && j["allowImplicitVarDeclaration"].is_boolean()) {
		cfg.allowImplicitVarDeclaration = j["allowImplicitVarDeclaration"].get<bool>();
	}
	if (j.contains("libraries") && j["libraries"].is_object()) {
		const auto &libs = j["libraries"];
#ifndef NO_INCLUDE_LIBS_FILE
		if (libs.contains("file") && libs["file"].is_object()) {
			cfg.addStdFile = libs["file"].value("enabled", true);
		}
#endif
#ifndef NO_INCLUDE_LIBS_REGEX
		if (libs.contains("regex") && libs["regex"].is_object()) {
			cfg.addStdRegex = libs["regex"].value("enabled", true);
		}
#endif
#ifndef NO_INCLUDE_LIBS_JSON
		if (libs.contains("json") && libs["json"].is_object()) {
			cfg.addStdJson = libs["json"].value("enabled", true);
		}
#endif
#ifndef NO_INCLUDE_LIBS_HTTP
		if (libs.contains("http") && libs["http"].is_object()) {
			cfg.addStdHttp = libs["http"].value("enabled", true);
		}
#endif
		if (libs.contains("math") && libs["math"].is_object()) {
			cfg.addStdMath = libs["math"].value("enabled", true);
		}
		if (libs.contains("bytes") && libs["bytes"].is_object()) {
			cfg.addStdBytes = libs["bytes"].value("enabled", true);
		}
#ifndef NO_INCLUDE_LIBS_DATE
		if (libs.contains("date") && libs["date"].is_object()) {
			cfg.addStdDate = libs["date"].value("enabled", true);
		}
#endif
	}
	return cfg;
}

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

	CompilerWrapper(std::string configJson = "{}")
	    : compiler([&]() {
			nlohmann::json j;
			try {
				if (!configJson.empty()) {
					j = nlohmann::json::parse(configJson);
				}
			} catch (...) {}
			return parseCompilerConfig(j);
		}()) {
		setvbuf(stderr, NULL, _IONBF, 0);
		nlohmann::json j;
		try {
			if (!configJson.empty()) {
				j = nlohmann::json::parse(configJson);
			}
		} catch (...) {}

		// 1. Resource Limits (Defaults: 32MB RAM, 1,000,000 opcodes)
		uint32_t maxOpcodeCount = 1000000;
		size_t maxMemoryBytes = 32 * 1024 * 1024; // 32MB
		if (j.contains("limits") && j["limits"].is_object()) {
			const auto &limits = j["limits"];
			if (limits.contains("maxOpcodeCount") && limits["maxOpcodeCount"].is_number()) {
				maxOpcodeCount = limits["maxOpcodeCount"].get<uint32_t>();
			}
			if (limits.contains("maxMemoryBytes") && limits["maxMemoryBytes"].is_number()) {
				maxMemoryBytes = limits["maxMemoryBytes"].get<size_t>();
			}
		}
#ifdef AUTOLANG_LIMIT_OPCODE
		compiler.setLimitOpcodeCount(maxOpcodeCount);
#endif
		compiler.setMaxManagedMemory(maxMemoryBytes);

		// 2. Compatibility settings
		bool ignoreForeignImports = true;
		bool allowLateinit = true;
		bool allowNonNullAssertion = true;
		bool enableKotlinCompat = true;
		bool strictMode = false;
		if (j.contains("strictMode") && j["strictMode"].is_boolean()) {
			strictMode = j["strictMode"].get<bool>();
		}
		if (j.contains("compatibility") && j["compatibility"].is_object()) {
			const auto &compat = j["compatibility"];
			if (compat.contains("strictMode") && compat["strictMode"].is_boolean()) {
				strictMode = compat["strictMode"].get<bool>();
			}
			if (compat.contains("allowValReassign") && compat["allowValReassign"].is_boolean()) {
				if (compat["allowValReassign"].get<bool>()) {
					strictMode = false;
				}
			}
			if (compat.contains("ignoreForeignImports") && compat["ignoreForeignImports"].is_boolean()) {
				ignoreForeignImports = compat["ignoreForeignImports"].get<bool>();
			}
			if (compat.contains("allowLateinit") && compat["allowLateinit"].is_boolean()) {
				allowLateinit = compat["allowLateinit"].get<bool>();
			}
			if (compat.contains("allowNonNullAssertion") && compat["allowNonNullAssertion"].is_boolean()) {
				allowNonNullAssertion = compat["allowNonNullAssertion"].get<bool>();
			}
			if (compat.contains("enableKotlinCompat") && compat["enableKotlinCompat"].is_boolean()) {
				enableKotlinCompat = compat["enableKotlinCompat"].get<bool>();
			}
		}
		compiler.setStrictMode(strictMode);
		if (j.contains("compatibility") && j["compatibility"].is_object()) {
			const auto &compat = j["compatibility"];
			if (compat.contains("autoCloseBracketsOnEof") && compat["autoCloseBracketsOnEof"].is_boolean()) {
				compiler.setAutoCloseBracketsOnEof(compat["autoCloseBracketsOnEof"].get<bool>());
			}
			if (compat.contains("allowImplicitVarDeclaration") && compat["allowImplicitVarDeclaration"].is_boolean()) {
				compiler.setAllowImplicitVarDeclaration(compat["allowImplicitVarDeclaration"].get<bool>());
			}
		}
		if (j.contains("autoCloseBracketsOnEof") && j["autoCloseBracketsOnEof"].is_boolean()) {
			compiler.setAutoCloseBracketsOnEof(j["autoCloseBracketsOnEof"].get<bool>());
		}
		if (j.contains("allowImplicitVarDeclaration") && j["allowImplicitVarDeclaration"].is_boolean()) {
			compiler.setAllowImplicitVarDeclaration(j["allowImplicitVarDeclaration"].get<bool>());
		}
		compiler.setIgnoreForeignImports(ignoreForeignImports);
		compiler.setKotlinCompat(enableKotlinCompat);
		bool showWarnings = false;
		if (j.contains("showWarnings") && j["showWarnings"].is_boolean()) {
			showWarnings = j["showWarnings"].get<bool>();
		} else if (j.contains("enableWarnings") && j["enableWarnings"].is_boolean()) {
			showWarnings = j["enableWarnings"].get<bool>();
		}
		compiler.setShowWarnings(showWarnings);
		mainSourceConfig.allowLateinitKeyword = allowLateinit;
		mainSourceConfig.allowNonNullAssertion = allowNonNullAssertion;

		// 3. Library specific security and autoImport settings
		if (j.contains("libraries") && j["libraries"].is_object()) {
			const auto &libs = j["libraries"];
			auto handleAutoImport = [&](const std::string &libKey, const std::string &stdPath) {
				if (libs.contains(libKey) && libs[libKey].is_object()) {
					const auto &item = libs[libKey];
					if (item.contains("autoImport") && item["autoImport"].is_boolean()) {
						bool autoImport = item["autoImport"].get<bool>();
						if (!autoImport) {
							compiler.autoImportMap.erase(stdPath);
						} else {
							auto it = compiler.builtInLibrariesMap.find(stdPath);
							if (it != compiler.builtInLibrariesMap.end() && it->second < compiler.builtInLibraries.size()) {
								compiler.autoImportMap[stdPath] = compiler.builtInLibraries[it->second];
							}
						}
					}
				}
			};

			handleAutoImport("file", "std/file");
			handleAutoImport("http", "std/http");
			handleAutoImport("json", "std/json");
			handleAutoImport("math", "std/math");
			handleAutoImport("regex", "std/regex");
			handleAutoImport("bytes", "std/bytes");
			handleAutoImport("date", "std/date");

#ifndef NO_INCLUDE_LIBS_FILE
			if (libs.contains("file") && libs["file"].is_object()) {
				const auto &f = libs["file"];
				if (f.contains("allowRead") && f["allowRead"].is_boolean()) {
					compiler.setAllowFileRead(f["allowRead"].get<bool>());
				}
				if (f.contains("allowWrite") && f["allowWrite"].is_boolean()) {
					compiler.setAllowFileWrite(f["allowWrite"].get<bool>());
				}
				if (f.contains("allowDelete") && f["allowDelete"].is_boolean()) {
					compiler.setAllowFileDelete(f["allowDelete"].get<bool>());
				}
				if (f.contains("allowedPaths") && f["allowedPaths"].is_array()) {
					std::vector<Autolang::AllowRule> fileRules;
					for (const auto &item : f["allowedPaths"]) {
						if (item.is_string()) {
							fileRules.push_back({Autolang::AllowRuleType::PLAIN_PREFIX, item.get<std::string>()});
						}
					}
					compiler.setAllowedFilePathsRules(fileRules);
				}
			}
#endif
#ifndef NO_INCLUDE_LIBS_HTTP
			if (libs.contains("http") && libs["http"].is_object()) {
				const auto &h = libs["http"];
				if (h.contains("allowedDomains") && h["allowedDomains"].is_array()) {
					std::vector<Autolang::AllowRule> domainRules;
					for (const auto &item : h["allowedDomains"]) {
						if (item.is_string()) {
							domainRules.push_back({Autolang::AllowRuleType::PLAIN_PREFIX, item.get<std::string>()});
						}
					}
					compiler.setAllowedDomainsRules(domainRules);
				}
			}
#endif
		}
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

	void setKotlinCompat(bool enable) {
		compiler.setKotlinCompat(enable);
	}

	void setStrictMode(bool enable) {
		compiler.setStrictMode(enable);
	}

	bool getStrictMode() {
		return compiler.getStrictMode();
	}

	void setShowWarnings(bool enable) {
		compiler.setShowWarnings(enable);
	}

	bool getShowWarnings() {
		return compiler.getShowWarnings();
	}

	void setAutoCloseBracketsOnEof(bool enable) {
		compiler.setAutoCloseBracketsOnEof(enable);
	}

	bool getAutoCloseBracketsOnEof() {
		return compiler.getAutoCloseBracketsOnEof();
	}

	void setAllowImplicitVarDeclaration(bool enable) {
		compiler.setAllowImplicitVarDeclaration(enable);
	}

	bool getAllowImplicitVarDeclaration() {
		return compiler.getAllowImplicitVarDeclaration();
	}

	void setLimitOpcodeCount(uint32_t count) {
		compiler.setLimitOpcodeCount(count);
	}

	uint32_t getLimitOpcodeCount() { return compiler.getLimitOpcodeCount(); }

	void setMaxManagedMemory(size_t limit) {
		compiler.setMaxManagedMemory(limit);
	}

	size_t getMaxManagedMemory() { return compiler.getMaxManagedMemory(); }

	size_t getCurrentManagedMemory() {
		return compiler.getCurrentManagedMemory();
	}

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
			bool result = compiler.compileAndRun(
			    path.c_str(), data.c_str(), mainSourceConfig);
			std::cerr.rdbuf(old);
			return result;
		} catch (const std::exception &e) {
			std::cerr << e.what() << "\n";
		}
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
	    .constructor<std::string>()
	    .function("compileAndRun", &CompilerWrapper::compileAndRun)
	    .function("run", &CompilerWrapper::run)
	    .function("compile", &CompilerWrapper::compile)
	    .function("refresh", &CompilerWrapper::refresh)
	    .function("setOnError", &CompilerWrapper::setOnError)
	    .function("setOnWarning", &CompilerWrapper::setOnWarning)
	    .function("setKotlinCompat", &CompilerWrapper::setKotlinCompat)
	    .function("setStrictMode", &CompilerWrapper::setStrictMode)
	    .function("getStrictMode", &CompilerWrapper::getStrictMode)
	    .function("setShowWarnings", &CompilerWrapper::setShowWarnings)
	    .function("getShowWarnings", &CompilerWrapper::getShowWarnings)
	    .function("setAutoCloseBracketsOnEof", &CompilerWrapper::setAutoCloseBracketsOnEof)
	    .function("getAutoCloseBracketsOnEof", &CompilerWrapper::getAutoCloseBracketsOnEof)
	    .function("setAllowImplicitVarDeclaration", &CompilerWrapper::setAllowImplicitVarDeclaration)
	    .function("getAllowImplicitVarDeclaration", &CompilerWrapper::getAllowImplicitVarDeclaration)
	    .function("setMainSourceConfig", &CompilerWrapper::setMainSourceConfig)
	    .function("setLimitOpcodeCount", &CompilerWrapper::setLimitOpcodeCount)
	    .function("getLimitOpcodeCount", &CompilerWrapper::getLimitOpcodeCount)
	    .function("setMaxManagedMemory", &CompilerWrapper::setMaxManagedMemory)
	    .function("getMaxManagedMemory", &CompilerWrapper::getMaxManagedMemory)
	    .function("getCurrentManagedMemory",
	              &CompilerWrapper::getCurrentManagedMemory)
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

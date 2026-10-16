#ifndef AUTOLANG_LIMIT_OPCODE
#define AUTOLANG_LIMIT_OPCODE
#endif
#define __PYBIND11__ 1

#include "backend/vm/ANotifier.hpp"
#include "shared/ANativeFunctionData.hpp"
#include <Autolang.hpp>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <third_party/nlohmann/json.hpp>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

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
		uint32_t executionTimeoutMs = 0;
		if (j.contains("limits") && j["limits"].is_object()) {
			const auto &limits = j["limits"];
			if (limits.contains("maxOpcodeCount") && limits["maxOpcodeCount"].is_number()) {
				maxOpcodeCount = limits["maxOpcodeCount"].get<uint32_t>();
			}
			if (limits.contains("maxMemoryBytes") && limits["maxMemoryBytes"].is_number()) {
				maxMemoryBytes = limits["maxMemoryBytes"].get<size_t>();
			}
			if (limits.contains("executionTimeoutMs") && limits["executionTimeoutMs"].is_number()) {
				executionTimeoutMs = limits["executionTimeoutMs"].get<uint32_t>();
			}
		}
#ifdef AUTOLANG_LIMIT_OPCODE
		compiler.setLimitOpcodeCount(maxOpcodeCount);
#endif
		compiler.setMaxManagedMemory(maxMemoryBytes);
		compiler.setExecutionTimeoutMs(executionTimeoutMs);

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
								compiler.autoImportMap[stdPath] = compiler.builtInLibraries[it->second].get();
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

	void setOnError(py::object func) {
		if (func.is_none()) {
			compiler.setOnError(nullptr);
			return;
		}
		compiler.setOnError(new Autolang::FunctionEvent(func));
	}

	void setOnWarning(py::object func) {
		if (func.is_none()) {
			compiler.setOnWarning(nullptr);
			return;
		}
		compiler.setOnWarning(new Autolang::FunctionEvent(func));
	}

	void registerBuiltInLibrary(std::string name, std::string data,
	                            bool autoImport, bool allowLateinitKeyword,
	                            bool allowNonNullAssertion, py::object mapFunction) {
		if (mapFunction.is_none()) {
			compiler.registerBuiltInLibrary(
			    name.c_str(), data.c_str(),
			    Autolang::LibraryConfig(autoImport, allowLateinitKeyword,
			                            allowNonNullAssertion));
			return;
		}

		py::dict pyMap = mapFunction.cast<py::dict>();
		ANativeMap nativeMap;
		nativeMap.reserve(pyMap.size());

		for (auto item : pyMap) {
			std::string key = item.first.cast<std::string>();
			py::object value = py::reinterpret_borrow<py::object>(item.second);

			if (value.is_none()) {
				continue;
			}

			if (py::hasattr(value, "__call__")) {
				Autolang::ANativeFunctionData funcData;
				funcData.type = Autolang::ANativeFunctionType::PY_FUNCTION;
				funcData.pyFunction = new py::object(value);
				nativeMap[key] = funcData;
			}
		}

		auto lib = compiler.registerBuiltInLibrary(
		    name.c_str(), data.c_str(),
		    Autolang::LibraryConfig(autoImport, allowLateinitKeyword,
		                            allowNonNullAssertion),
		    nativeMap);
		lib->flags |= Autolang::LibraryFlags::IS_PY_BRIDGE;
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

	void setExecutionTimeoutMs(uint32_t ms) {
		compiler.setExecutionTimeoutMs(ms);
	}

	uint32_t getExecutionTimeoutMs() {
		return compiler.getExecutionTimeoutMs();
	}

	// ---- Domain whitelist builder ----
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

	// ---- File path whitelist builder ----
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
			return result && !compiler.hasException();
		} catch (const std::exception &e) {
			std::cerr << e.what() << "\n";
		}
		std::cerr.rdbuf(old);
		return false;
	}

	void loadBuiltInFunctions() {
		if (!compiler.loadedBuiltIn) {
			compiler.loadBuiltInFunctions();
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
			return !compiler.hasException();
		} catch (const std::exception &e) {
			return false;
		}
	}

	std::string getOutput() {
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
		if (compiler.hasException()) {
			return true;
		}
		if (compiler.vm.callFrames.getSize() == 0) {
			if (compiler.vm.callFrames.objects[0].exception) {
				return true;
			}
			return false;
		}
		return false;
	}

	py::object getException() {
		if (compiler.hasException()) {
			py::dict obj;
			obj["message"] = std::string(compiler.exceptionMessage ? compiler.exceptionMessage : "");
			return obj;
		}
		if (compiler.vm.callFrames.getSize() == 0) {
			if (compiler.vm.callFrames.objects[0].exception) {
				py::dict obj;
				auto exception = compiler.exceptionMessage;
				obj["message"] = std::string(exception ? exception : "");
				return obj;
			}
			return py::none();
		}
		return py::none();
	}

	void throwException(std::string message) {
		if (compiler.vm.callFrames.getSize() == 0) {
			return;
		}
		compiler.vm.notifier->throwException(message);
	}
};

PYBIND11_MODULE(autolang, m) {
	py::class_<CompilerWrapper>(m, "ACompiler")
	    .def(py::init([](py::object config) {
		    std::string configJson = "{}";
		    if (!config.is_none()) {
			    if (py::isinstance<py::str>(config)) {
				    configJson = config.cast<std::string>();
			    } else if (py::isinstance<py::dict>(config)) {
				    py::object json_mod = py::module_::import("json");
				    configJson = json_mod.attr("dumps")(config).cast<std::string>();
			    }
		    }
		    return new CompilerWrapper(configJson);
	    }), py::arg("config") = "{}")
	    .def("compileAndRun", &CompilerWrapper::compileAndRun,
	         py::arg("path"), py::arg("data"))
	    .def("run", &CompilerWrapper::run)
	    .def("compile", &CompilerWrapper::compile,
	         py::arg("path"), py::arg("data"))
	    .def("refresh", &CompilerWrapper::refresh)
	    .def("setOnError", &CompilerWrapper::setOnError,
	         py::arg("func"))
	    .def("setOnWarning", &CompilerWrapper::setOnWarning,
	         py::arg("func"))
	    .def("setKotlinCompat", &CompilerWrapper::setKotlinCompat,
	         py::arg("enable"))
	    .def("setStrictMode", &CompilerWrapper::setStrictMode,
	         py::arg("enable"))
	    .def("getStrictMode", &CompilerWrapper::getStrictMode)
	    .def("setShowWarnings", &CompilerWrapper::setShowWarnings,
	         py::arg("enable"))
	    .def("getShowWarnings", &CompilerWrapper::getShowWarnings)
	    .def("setAutoCloseBracketsOnEof", &CompilerWrapper::setAutoCloseBracketsOnEof,
	         py::arg("enable"))
	    .def("getAutoCloseBracketsOnEof", &CompilerWrapper::getAutoCloseBracketsOnEof)
	    .def("setAllowImplicitVarDeclaration", &CompilerWrapper::setAllowImplicitVarDeclaration,
	         py::arg("enable"))
	    .def("getAllowImplicitVarDeclaration", &CompilerWrapper::getAllowImplicitVarDeclaration)
	    .def("setMainSourceConfig", &CompilerWrapper::setMainSourceConfig,
	         py::arg("allowLateinitKeyword"), py::arg("allowNonNullAssertion"))
	    .def("setLimitOpcodeCount", &CompilerWrapper::setLimitOpcodeCount,
	         py::arg("count"))
	    .def("getLimitOpcodeCount", &CompilerWrapper::getLimitOpcodeCount)
	    .def("setMaxManagedMemory", &CompilerWrapper::setMaxManagedMemory,
	         py::arg("limit"))
	    .def("getMaxManagedMemory", &CompilerWrapper::getMaxManagedMemory)
	    .def("getCurrentManagedMemory", &CompilerWrapper::getCurrentManagedMemory)
	    .def("setExecutionTimeoutMs", &CompilerWrapper::setExecutionTimeoutMs,
	         py::arg("ms"))
	    .def("getExecutionTimeoutMs", &CompilerWrapper::getExecutionTimeoutMs)
	    .def("clearDomainRules", &CompilerWrapper::clearDomainRules)
	    .def("addDomainRule", &CompilerWrapper::addDomainRule,
	         py::arg("type"), py::arg("value"))
	    .def("applyDomainRules", &CompilerWrapper::applyDomainRules)
	    .def("loadBuiltInLibraries", &CompilerWrapper::loadBuiltInFunctions)
	    .def("setAllowFileRead", &CompilerWrapper::setAllowFileRead,
	         py::arg("allow"))
	    .def("setAllowFileWrite", &CompilerWrapper::setAllowFileWrite,
	         py::arg("allow"))
	    .def("setAllowFileDelete", &CompilerWrapper::setAllowFileDelete,
	         py::arg("allow"))
	    .def("clearPathRules", &CompilerWrapper::clearPathRules)
	    .def("addPathRule", &CompilerWrapper::addPathRule,
	         py::arg("type"), py::arg("value"))
	    .def("applyPathRules", &CompilerWrapper::applyPathRules)
	    .def("setFileBasePath", &CompilerWrapper::setFileBasePath,
	         py::arg("path"))
	    .def("getOutput", &CompilerWrapper::getOutput)
	    .def("setOutput", &CompilerWrapper::setOutput,
	         py::arg("output"))
	    .def("clearOutput", &CompilerWrapper::clearOutput)
	    .def("registerBuiltInLibrary", &CompilerWrapper::registerBuiltInLibrary,
	         py::arg("name"), py::arg("data"),
	         py::arg("autoImport") = false,
	         py::arg("allowLateinitKeyword") = false,
	         py::arg("allowNonNullAssertion") = false,
	         py::arg("mapFunction") = py::none())
	    .def("hasCompilerError", &CompilerWrapper::hasCompilerError)
	    .def("throwException", &CompilerWrapper::throwException,
	         py::arg("message"))
	    .def("getException", &CompilerWrapper::getException)
	    .def("hasException", &CompilerWrapper::hasException);
}
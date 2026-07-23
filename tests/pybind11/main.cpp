#define AUTOLANG_LIMIT_OPCODE
#define __PYBIND11__ 1

#include "shared/ANativeFunctionData.hpp"
#include <Autolang.hpp>
#include <cstdio>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

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

    CompilerWrapper(bool addStdFile = true, bool addStdRegex = true, bool addStdJson = true,
                    bool addStdHttp = true, bool addStdMath = true, bool addStdBytes = true,
                    bool addStdDate = true)
        : compiler(Autolang::ACompilerConfig{.addStdFile = addStdFile,
                                             .addStdRegex = addStdRegex,
                                             .addStdJson = addStdJson,
                                             .addStdHttp = addStdHttp,
                                             .addStdMath = addStdMath,
                                             .addStdBytes = addStdBytes,
                                             .addStdDate = addStdDate}) {
        setvbuf(stderr, NULL, _IONBF, 0);
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

            if (py::hasattr(value, "__call__")) { // Kiểm tra xem object có phải là callable/function không
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

    void setLimitOpcodeCount(uint32_t count) {
        compiler.setLimitOpcodeCount(count);
    }

    uint32_t getLimitOpcodeCount() { return compiler.getLimitOpcodeCount(); }

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
        } catch (const std::exception &e) {
            compiler.refresh();
            return false;
        }
        compiler.refresh();
        return true;
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
        if (compiler.vm.callFrames.getSize() == 0) {
            if (compiler.vm.callFrames.objects[0].exception) {
                return true;
            }
            return false;
        }
        return false;
    }

    py::object getException() {
        if (compiler.vm.callFrames.getSize() == 0) {
            if (compiler.vm.callFrames.objects[0].exception) {
                py::dict obj;
                auto exception = compiler.exceptionMessage;
                obj["message"] = std::string(exception);
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

// Khai báo Module pybind11 thay thế EMSCRIPTEN_BINDINGS
PYBIND11_MODULE(autolang, m) {
    py::class_<CompilerWrapper>(m, "ACompiler")
        .def(py::init<bool, bool, bool, bool, bool, bool, bool>(),
             py::arg("addStdFile") = true,
             py::arg("addStdRegex") = true,
             py::arg("addStdJson") = true,
             py::arg("addStdHttp") = true,
             py::arg("addStdMath") = true,
             py::arg("addStdBytes") = true,
             py::arg("addStdDate") = true)
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
        .def("setMainSourceConfig", &CompilerWrapper::setMainSourceConfig,
             py::arg("allowLateinitKeyword"), py::arg("allowNonNullAssertion"))
        .def("setLimitOpcodeCount", &CompilerWrapper::setLimitOpcodeCount,
             py::arg("count"))
        .def("getLimitOpcodeCount", &CompilerWrapper::getLimitOpcodeCount)
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
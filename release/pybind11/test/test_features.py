import os
import sys

if os.path.exists(r"D:\mingw64\bin"):
    os.add_dll_directory(r"D:\mingw64\bin")

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.abspath(os.path.join(BASE_DIR, "../../.."))
PYD_BUILD_DIR = os.path.abspath(os.path.join(PROJECT_ROOT, "build/lib.win-amd64-cpython-313"))
sys.path.insert(0, PROJECT_ROOT)
sys.path.insert(0, PYD_BUILD_DIR)

import autolang

def test_all():
    print("=== Test 1: Default constructor and Getters/Setters ===")
    compiler = autolang.ACompiler()
    
    compiler.setStrictMode(True)
    assert compiler.getStrictMode() == True
    compiler.setStrictMode(False)
    assert compiler.getStrictMode() == False
    
    compiler.setKotlinCompat(True)
    compiler.setKotlinCompat(False)
    
    compiler.setShowWarnings(True)
    assert compiler.getShowWarnings() == True
    compiler.setShowWarnings(False)
    assert compiler.getShowWarnings() == False
    
    compiler.setAutoCloseBracketsOnEof(True)
    assert compiler.getAutoCloseBracketsOnEof() == True
    compiler.setAutoCloseBracketsOnEof(False)
    assert compiler.getAutoCloseBracketsOnEof() == False
    
    compiler.setAllowImplicitVarDeclaration(True)
    assert compiler.getAllowImplicitVarDeclaration() == True
    compiler.setAllowImplicitVarDeclaration(False)
    assert compiler.getAllowImplicitVarDeclaration() == False
    
    compiler.setExecutionTimeoutMs(5000)
    assert compiler.getExecutionTimeoutMs() == 5000
    
    compiler.setLimitOpcodeCount(10000)
    assert compiler.getLimitOpcodeCount() == 10000
    
    compiler.setMaxManagedMemory(2000000)
    assert compiler.getMaxManagedMemory() == 2000000
    assert isinstance(compiler.getCurrentManagedMemory(), int)
    print("Test 1 Passed!")

    print("=== Test 2: Dict Configuration (matching NPM wrapper JSON format) ===")
    cfg = {
        "showWarnings": True,
        "limits": {
            "maxOpcodeCount": 50000,
            "executionTimeoutMs": 2500,
            "maxMemoryBytes": 10485760
        },
        "compatibility": {
            "strictMode": False,
            "enableKotlinCompat": True,
            "autoCloseBracketsOnEof": False,
            "allowImplicitVarDeclaration": True
        },
        "libraries": {
            "http": {"enabled": True},
            "file": {"enabled": False}
        }
    }
    c2 = autolang.ACompiler(cfg)
    assert c2.getLimitOpcodeCount() == 50000
    assert c2.getExecutionTimeoutMs() == 2500
    assert c2.getMaxManagedMemory() == 10485760
    assert c2.getStrictMode() == False
    assert c2.getShowWarnings() == True
    assert c2.getAutoCloseBracketsOnEof() == False
    assert c2.getAllowImplicitVarDeclaration() == True
    print("Test 2 Passed!")

    print("=== Test 3: Output manipulation ===")
    c2.clearOutput()
    assert c2.getOutput() == ""
    c2.setOutput("test output\n")
    assert c2.getOutput() == "test output\n"
    c2.clearOutput()
    assert c2.getOutput() == ""
    print("Test 3 Passed!")

    print("=== Test 4: compile() and run() separate ===")
    c3 = autolang.ACompiler()
    compile_ok = c3.compile("main.al", 'var a = 15; var b = 25; print("Sum: " + (a + b));')
    assert compile_ok == True, "Compilation failed"
    assert c3.hasCompilerError() == False
    run_ok = c3.run()
    assert run_ok == True, "Execution failed"
    print("Test 4 Passed!")

    print("=== Test 4b: compileAndRun buffer capture ===")
    c3b = autolang.ACompiler()
    res = c3b.compileAndRun("main.al", 'print("Buffer Captured!");')
    assert res == True
    out = c3b.getOutput()
    assert "Buffer Captured!" in out
    print("Test 4b Passed!")

    print("=== Test 5: Error and Exception Handling ===")
    c4 = autolang.ACompiler()
    bad_code = 'var x = undefined_function_call();'
    res = c4.compileAndRun("err.al", bad_code)
    assert res == False
    assert c4.hasCompilerError() == True
    print("Compile error verified!")

    # Runtime exception
    c4.reset if hasattr(c4, 'reset') else None
    c4_runtime = autolang.ACompiler()
    runtime_err_code = 'var a = [1, 2]; var b = a[10];'
    res2 = c4_runtime.compileAndRun("runtime_err.al", runtime_err_code)
    assert res2 == False
    assert c4_runtime.hasException() == True
    ex = c4_runtime.getException()
    print("Captured Runtime Exception:", ex)
    assert isinstance(ex, dict)
    assert "message" in ex
    print("Test 5 Passed!")

    print("=== Test 6: registerBuiltInLibrary with Python callbacks ===")
    c5 = autolang.ACompiler()
    native_funcs = {
        "nativeAdd": lambda a, b: a + b
    }
    header_data = """@no_constructor
class CustomMath {
    @native("nativeAdd")
    static fun add(a: Int, b: Int): Int
}
"""
    c5.registerBuiltInLibrary("custom_lib", header_data, True, False, False, native_funcs)
    al_code = """import custom_lib.*
var res = CustomMath.add(10, 20)
print("Native Add: " + res)
"""
    res = c5.compileAndRun("custom_test.al", al_code)
    print("res:", res, "err:", c5.hasCompilerError(), "exc:", c5.getException(), "out:", c5.getOutput())
    assert res == True, "Custom library run failed"
    out = c5.getOutput()
    assert "Native Add: 30" in out
    print("Test 6 Passed!")

    print("\nALL 6 TESTS PASSED SUCCESSFULLY!")

if __name__ == "__main__":
    test_all()

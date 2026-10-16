import os
import sys

# Add MinGW bin directory to DLL search path for Python 3.8+ on Windows
if os.path.exists(r"D:\mingw64\bin"):
    os.add_dll_directory(r"D:\mingw64\bin")

# Add build folder to sys.path for importing .pyd
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.abspath(os.path.join(BASE_DIR, "../../.."))
PYD_BUILD_DIR = os.path.abspath(os.path.join(PROJECT_ROOT, "build/lib.win-amd64-cpython-313"))
sys.path.insert(0, PROJECT_ROOT)
sys.path.insert(0, PYD_BUILD_DIR)

try:
    import autolang
    print("Import autolang successful!")
    print("Module autolang:", autolang)
    print("Attributes list:", dir(autolang))
    
    # Initialize ACompiler using config JSON (NPM wrapper format)
    compiler = autolang.ACompiler('{"libraries": {"http": {"enabled": true}, "file": {"enabled": false}}}')
    print("Initialize ACompiler successful!")
    
    # Test compile and run AutoLang code using keyword arguments
    code = 'print("Hello from AutoLang pybind11!")'
    print("Compiling and running AutoLang code...")
    success = compiler.compileAndRun(path="main.al", data=code)
    print("Execution result:", "SUCCESS" if success else "FAILED")
    
    # Print the output buffer
    print("AutoLang Output:")
    print(compiler.getOutput())
    
except Exception as e:
    print("Error during import or execution:", e)

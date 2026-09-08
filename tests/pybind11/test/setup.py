import os
import sys
from setuptools import setup

try:
    from pybind11.setup_helpers import Pybind11Extension, build_ext
except ImportError:
    print("Please install pybind11 first: pip install pybind11")
    sys.exit(1)

# Project root directory (../../ from tests/pybind11/test/)
BASE_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), "../../.."))

# --- Fix mixing MSVC and MinGW flags ---
class build_ext_mingw_fix(build_ext):
    def build_extensions(self):
        compiler_type = self.compiler.compiler_type
        for ext in self.extensions:
            new_args = []
            for arg in ext.extra_compile_args:
                if compiler_type == 'mingw32':
                    # MinGW: remove MSVC flags (/) and -std= flag (re-added below)
                    if not arg.startswith('/') and not arg.startswith('-std='):
                        new_args.append(arg)
                else:
                    # GCC/Clang on Linux/macOS: remove Windows-only flags
                    if not arg.startswith('/') and not arg.startswith('-Wa,') and not arg.startswith('-std='):
                        new_args.append(arg)

            ext.extra_compile_args = ["-std=c++17"] + new_args

        super().build_extensions()

main_cpp = os.path.join(BASE_DIR, "tests/pybind11/main.cpp").replace("\\", "/")
src_dir = os.path.join(BASE_DIR, "src").replace("\\", "/")

source_files = [main_cpp]

include_dirs = [
    src_dir,
    os.path.join(src_dir, "third_party/curl/include").replace("\\", "/"),
    os.path.join(src_dir, "third_party").replace("\\", "/"),
]

extra_compile_args = [
    "-std=c++17",
    "-O2",
    "-DNOMINMAX",
    "-DNO_INCLUDE_LIBS_HTTP",   # Disable HTTP library, curl/libcurl not needed
    "-Wno-unused-parameter",
    "-Wno-unused-variable",
    "-Wno-switch",
    "-Wno-sign-compare",
    "-Wno-reorder",
    "-Wa,-mbig-obj",            # Windows only (g++), filtered out on Linux/macOS
]

extra_objects = []
libraries = []

if sys.platform == "win32":
    libraries.extend(["ws2_32", "wldap32", "crypt32", "advapi32", "bcrypt"])
# else: nothing needed on Linux/macOS since HTTP is disabled

ext_modules = [
    Pybind11Extension(
        "autolang",
        source_files,
        include_dirs=include_dirs,
        extra_compile_args=extra_compile_args,
        extra_objects=extra_objects,
        libraries=libraries,
        cxx_std=None,
    ),
]

setup(
    name="autolang",
    version="0.1.0",
    description="Python binding for AutoLang",
    ext_modules=ext_modules,
    cmdclass={"build_ext": build_ext_mingw_fix},
    zip_safe=False,
)

# Manual build command on Windows (MinGW):
#   python tests/pybind11/test/setup.py build_ext --inplace --compiler=mingw32 --force
#
# Manual build command on Linux/macOS:
#   python tests/pybind11/test/setup.py build_ext --inplace --force

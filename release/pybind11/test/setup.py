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

main_cpp = os.path.join(BASE_DIR, "release/pybind11/main.cpp").replace("\\", "/")
src_dir = os.path.join(BASE_DIR, "src").replace("\\", "/")

source_files = [main_cpp]
for root, _, files in os.walk(src_dir):
    for f in files:
        if f.endswith(".cpp") and not f.startswith("AVM_run_"):
            source_files.append(os.path.join(root, f).replace("\\", "/"))

include_dirs = [
    src_dir,
    os.path.join(src_dir, "third_party/curl/include").replace("\\", "/"),
    os.path.join(src_dir, "third_party").replace("\\", "/"),
    os.path.join(BASE_DIR, "build/_deps/rapidfuzz-src").replace("\\", "/"),
]

extra_compile_args = [
    "-std=c++17",
    "-O2",
    "-DNOMINMAX",
    "-DCURL_STATICLIB",
    "-D__PYBIND11__=1",
    "-DAUTOLANG_LIMIT_OPCODE",
    "-Wno-unused-parameter",
    "-Wno-unused-variable",
    "-Wno-switch",
    "-Wno-sign-compare",
    "-Wno-reorder",
    "-Wa,-mbig-obj",            # Windows only (g++), filtered out on Linux/macOS
]

libcurl_d = os.path.join(BASE_DIR, "build/_deps/curl-build/lib/libcurl-d.a").replace("\\", "/")
libcurl = os.path.join(BASE_DIR, "build/_deps/curl-build/lib/libcurl.a").replace("\\", "/")
curl_lib = libcurl_d if os.path.exists(libcurl_d) else libcurl
extra_objects = [curl_lib] if os.path.exists(curl_lib) else []
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

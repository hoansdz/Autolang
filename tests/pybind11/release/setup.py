import os
import sys
from setuptools import setup

try:
    from pybind11.setup_helpers import Pybind11Extension, build_ext
except ImportError:
    print("Please install pybind11 first: pip install pybind11")
    sys.exit(1)

# Project root directory (3 levels up from tests/pybind11/release/)
BASE_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), "../../.."))

# ---------------------------------------------------------------------------
# Build class: Filter and adjust compile flags per compiler
# ---------------------------------------------------------------------------
class build_ext_fix(build_ext):
    def build_extensions(self):
        compiler_type = self.compiler.compiler_type
        for ext in self.extensions:
            if compiler_type == "msvc":
                # MSVC: strip GCC-style flags, add equivalent MSVC flags
                ext.extra_compile_args = [
                    "/std:c++17",
                    "/bigobj",
                    "/DNOMINMAX",
                    "/DNO_INCLUDE_LIBS_HTTP",
                    "/wd4100",  # unused parameter
                    "/wd4101",  # unused variable
                    "/wd4065",  # switch with no case
                ]
            elif compiler_type == "mingw32":
                # MinGW on Windows: remove MSVC flags and -std= flag, then re-add
                new_args = [
                    arg for arg in ext.extra_compile_args
                    if not arg.startswith("/") and not arg.startswith("-std=")
                ]
                ext.extra_compile_args = ["-std=c++17"] + new_args
            else:
                # GCC / Clang on Linux and macOS: remove Windows-only flags
                new_args = [
                    arg for arg in ext.extra_compile_args
                    if not arg.startswith("/")
                    and not arg.startswith("-Wa,")
                    and not arg.startswith("-std=")
                ]
                ext.extra_compile_args = ["-std=c++17"] + new_args

        super().build_extensions()


# ---------------------------------------------------------------------------
# Source files and includes
# ---------------------------------------------------------------------------
main_cpp = os.path.join(BASE_DIR, "tests/pybind11/main.cpp").replace("\\", "/")
src_dir  = os.path.join(BASE_DIR, "src").replace("\\", "/")

source_files = [main_cpp]

include_dirs = [
    src_dir,
    os.path.join(src_dir, "third_party/curl/include").replace("\\", "/"),
    os.path.join(src_dir, "third_party").replace("\\", "/"),
]

# ---------------------------------------------------------------------------
# Default compiler flags (GCC/MinGW style — filtered in build_ext_fix)
# ---------------------------------------------------------------------------
extra_compile_args = [
    "-std=c++17",
    "-O2",
    "-DNOMINMAX",
    "-DNO_INCLUDE_LIBS_HTTP",
    "-Wno-unused-parameter",
    "-Wno-unused-variable",
    "-Wno-switch",
    "-Wno-sign-compare",
    "-Wno-reorder",
    "-Wa,-mbig-obj",  # Windows MinGW only, filtered on Linux/macOS
]

extra_objects = []
libraries = []

if sys.platform == "win32":
    libraries.extend(["ws2_32", "wldap32", "crypt32", "advapi32", "bcrypt"])

# ---------------------------------------------------------------------------
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

readme_path = os.path.join(BASE_DIR, "README.md")
long_description = (
    open(readme_path, "r", encoding="utf-8", errors="ignore").read()
    if os.path.exists(readme_path) else ""
)

setup(
    name="autolang",
    version="0.1.0",
    description="Python binding for AutoLang - lightweight, high-performance programming language",
    long_description=long_description,
    long_description_content_type="text/markdown",
    ext_modules=ext_modules,
    cmdclass={"build_ext": build_ext_fix},
    zip_safe=False,
    classifiers=[
        "Programming Language :: Python :: 3",
        "Programming Language :: C++",
        "Operating System :: OS Independent",
        "License :: OSI Approved :: MIT License",
    ],
    python_requires=">=3.8",
)

# Manual build command on Windows (MinGW):
#   python tests/pybind11/release/setup.py build_ext --inplace --compiler=mingw32 --force
#
# Manual build command on Linux/macOS:
#   python tests/pybind11/release/setup.py build_ext --inplace --force

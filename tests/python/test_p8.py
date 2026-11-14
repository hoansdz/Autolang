import os
import sys
import subprocess
import pytest

if os.path.exists(r"D:\mingw64\bin"):
    os.add_dll_directory(r"D:\mingw64\bin")

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.abspath(os.path.join(BASE_DIR, "../.."))
PYD_BUILD_DIR = os.path.abspath(os.path.join(PROJECT_ROOT, "build/lib.win-amd64-cpython-313"))
PYD_P9_DIR = os.path.abspath(os.path.join(PROJECT_ROOT, "build_p9"))
sys.path.insert(0, PROJECT_ROOT)
sys.path.insert(0, PYD_BUILD_DIR)
if os.path.exists(PYD_P9_DIR):
    sys.path.insert(0, PYD_P9_DIR)

import autolang
print(f"Loaded autolang from: {autolang.__file__}")


def test_p8_minimal_when():
    """1. Ca tối giản: when { a < t -> 'LT'; else -> 'GE' }"""
    compiler = autolang.ACompiler()
    source = """
val a = 1
val t = 50
val r = when {
    a < t -> "LT"
    else -> "GE"
}
assert(r == "LT", "minimal when a < t")
"""
    res = compiler.compileAndRun("test_p8_minimal.al", source)
    assert res is True, "compileAndRun minimal when case must succeed"


def test_p8_c14_benchmark_and_nullable_or():
    """2. when { a == null || a < t -> ... } với a: Int? và đề compare C14"""
    compiler = autolang.ACompiler()
    source = """
fun getProfileId(name: String): Int? {
    if (name == "Alice") return 123
    return null
}
fun getProfileScore(id: Int): Int? {
    if (id == 123) return 40
    return null
}

val stepIdResult = getProfileId("Alice")
val stepScoreResult = if (stepIdResult != null) getProfileScore(stepIdResult) else null
val thresholdScore = 50
val finalResult = when {
    stepIdResult == null -> "NO_PROFILE"
    stepScoreResult == null || stepScoreResult < thresholdScore -> "FAIL"
    else -> "PASS"
}
assert(finalResult == "FAIL", "C14 finalResult == FAIL")
"""
    res = compiler.compileAndRun("test_p8_c14.al", source)
    assert res is True, "compileAndRun C14 benchmark case must succeed"


def test_p8_casual_comparisons():
    """3. if (a < t), a < t && t > a, println(a < t)"""
    compiler = autolang.ACompiler()
    source = """
val a = 1
val t = 50

val rIf = if (a < t) "YES" else "NO"
assert(rIf == "YES", "if a < t")

val rLogic = a < t && t > a
assert(rLogic == true, "a < t && t > a")

fun checkBool(b: Bool): Bool = b
assert(checkBool(a < t) == true, "call with a < t")
"""
    res = compiler.compileAndRun("test_p8_casual.al", source)
    assert res is True, "compileAndRun casual comparison cases must succeed"


def test_p8_generics_constructs():
    """4. Mọi cú pháp generic hiện có: Array<Int>(), Map<String, Array<Int>>(), Set<String>(), class Box<T, R>, typealias"""
    compiler = autolang.ACompiler()
    source = """
typealias IntList = Array<Int>
class Box<T, R>(val item: T, val tag: R)
class NumBox<T extends Box<Int, String>, R>(val box: T, val extra: R)

fun <T> idGeneric(x: T): T = x

val arr = Array<Int>()
arr.push(42)
assert(arr[0] == 42, "Array<Int>()")

val map = Map<String, Array<Int>>()
map["key"] = arr
assert(map["key"] != null && map["key"]![0] == 42, "Map<String, Array<Int>>()")

val s = Set<String>()
s.add("autolang")
assert(s.has("autolang"), "Set<String>()")

val b = Box<Int, String>(10, "tag")
assert(b.item == 10 && b.tag == "tag", "Box<T, R>")

val nb = NumBox<Box<Int, String>, Bool>(b, true)
assert(nb.box.item == 10 && nb.extra == true, "NumBox with extends")

val genericCallRes = idGeneric<Int>(99)
assert(genericCallRes == 99, "idGeneric<Int>")

val idAliased: IntList = [1, 2, 3]
assert(idAliased.size() == 3, "typealias IntList")
"""
    res = compiler.compileAndRun("test_p8_generics.al", source)
    assert res is True, "compileAndRun generic constructs must succeed"


@pytest.mark.parametrize("bad_code", [
    "val x = Array<Int>",                    # Type without constructor call
    "val y = when {\n    a < < -> 1\n}",      # Malformed operator in when condition
    "val z = if (a < ) 1 else 2",             # Incomplete comparison operand in if condition
    "val w = 10 < < 20",                      # Double less-than without operand
])
def test_p8_syntax_errors_in_subprocess_no_crash(bad_code):
    """5. Lỗi biên dịch không làm sập tiến trình, chạy trong tiến trình con và kiểm mã thoát"""
    subproc_code = f"""
import os
import sys

if os.path.exists(r"D:\\mingw64\\bin"):
    os.add_dll_directory(r"D:\\mingw64\\bin")

PROJECT_ROOT = r"{PROJECT_ROOT}"
PYD_BUILD_DIR = r"{PYD_BUILD_DIR}"
sys.path.insert(0, PROJECT_ROOT)
sys.path.insert(0, PYD_BUILD_DIR)

import autolang

c = autolang.ACompiler()
source = {repr(bad_code)}
res = c.compileAndRun("bad.al", source)
assert res is False, f"Expected compileAndRun to return False for syntax error, got {{res}}"
"""
    proc = subprocess.run(
        [sys.executable, "-c", subproc_code],
        capture_output=True,
        text=True,
        timeout=10,
    )
    # Không bị crash (SIGSEGV / Access Violation / exit code âm hoặc không phải 0)
    assert proc.returncode == 0, (
        f"Subprocess crashed or failed for code: {bad_code}\n"
        f"Returncode: {proc.returncode}\n"
        f"Stderr: {proc.stderr}\n"
        f"Stdout: {proc.stdout}"
    )

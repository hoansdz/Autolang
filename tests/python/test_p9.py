import os
import sys
import pytest

if os.path.exists(r"D:\mingw64\bin"):
    os.add_dll_directory(r"D:\mingw64\bin")

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.abspath(os.path.join(BASE_DIR, "../.."))
PYD_P9_DIR = os.path.abspath(os.path.join(PROJECT_ROOT, "build_p9"))

# Ensure build_p9 is first in sys.path
sys.path.insert(0, PYD_P9_DIR)

import autolang

print(f"Loaded autolang from: {autolang.__file__}")
assert os.path.abspath(autolang.__file__).lower().startswith(PYD_P9_DIR.lower()), (
    f"Must load autolang from {PYD_P9_DIR}, but got {autolang.__file__}"
)


def test_p9_minimal_with_parens():
    """1a. Ca tối giản in 'ab' với ngoặc quanh Elvis"""
    compiler = autolang.ACompiler()
    compiler.clearOutput()
    source = """
val r = "ab"
var m: String? = null
if (r.length > (m?.length ?: 0)) {
    m = r
}
println(m)
"""
    res = compiler.compileAndRun("test_p9_minimal_parens.al", source)
    assert res is True, "compileAndRun must succeed"
    assert not compiler.hasException(), "must not have exception"
    out = compiler.getOutput().strip()
    assert out == "ab", f"Expected 'ab', got: '{out}'"


def test_p9_minimal_without_parens():
    """1b. Cùng ca tối giản bỏ ngoặc in 'ab'"""
    compiler = autolang.ACompiler()
    compiler.clearOutput()
    source = """
val r = "ab"
var m: String? = null
if (r.length > m?.length ?: 0) {
    m = r
}
println(m)
"""
    res = compiler.compileAndRun("test_p9_minimal_no_parens.al", source)
    assert res is True, "compileAndRun must succeed"
    assert not compiler.hasException(), "must not have exception"
    out = compiler.getOutput().strip()
    assert out == "ab", f"Expected 'ab', got: '{out}'"


def test_p9_street_longest_in_for_loop():
    """1c. Đặt if trong for duyệt mảng ['Le Loi', 'Hai Ba Trung', 'Ba Dinh'] tìm chuỗi dài nhất"""
    compiler = autolang.ACompiler()
    compiler.clearOutput()
    source = """
val streets = ["Le Loi", "Hai Ba Trung", "Ba Dinh"]
var longest: String? = null
for (s in streets) {
    if (longest == null || s.length > (longest?.length ?: 0)) {
        longest = s
    }
}
println(longest)
"""
    res = compiler.compileAndRun("test_p9_street_loop.al", source)
    assert res is True, "compileAndRun must succeed"
    assert not compiler.hasException(), "must not have exception"
    out = compiler.getOutput().strip()
    assert out == "Hai Ba Trung", f"Expected 'Hai Ba Trung', got: '{out}'"


def test_p9_while_with_safe_call():
    """1d. while ((x?.length ?: 0) < 3) chạy đúng"""
    compiler = autolang.ACompiler()
    compiler.clearOutput()
    source = """
var x: String? = "a"
var count = 0
while ((x?.length ?: 0) < 3) {
    x = x + "a"
    count = count + 1
}
println(count)
println(x)
"""
    res = compiler.compileAndRun("test_p9_while.al", source)
    assert res is True, "compileAndRun must succeed"
    assert not compiler.hasException(), "must not have exception"
    lines = compiler.getOutput().strip().splitlines()
    assert lines[0].strip() == "2", f"Expected count 2, got {lines[0]}"
    assert lines[1].strip() == "aaa", f"Expected 'aaa', got {lines[1]}"


def test_p9_when_with_safe_call():
    """1e. when có nhánh điều kiện chứa ?."""
    compiler = autolang.ACompiler()
    compiler.clearOutput()
    source = """
var s: String? = "hello"
val res = when {
    (s?.length ?: 0) > 3 -> "LONG"
    else -> "SHORT"
}
println(res)
"""
    res = compiler.compileAndRun("test_p9_when.al", source)
    assert res is True, "compileAndRun must succeed"
    assert not compiler.hasException(), "must not have exception"
    out = compiler.getOutput().strip()
    assert out == "LONG", f"Expected 'LONG', got '{out}'"


def test_p9_regressions_and_safe_features():
    """1f. Các trường hợp đang đúng vẫn phải đúng: val ... = a > b?.c ?: 0, ?.let, chuỗi ?., ?: gặp return"""
    compiler = autolang.ACompiler()
    compiler.clearOutput()
    source = """
class Item(val score: Int?)
class Wrapper(val item: Item?)

val w: Wrapper? = Wrapper(Item(10))
val score = w?.item?.score ?: 0
println(score)

val letRes = w?.item?.let { it.score ?: 0 } ?: -1
println(letRes)

fun testElvisReturn(arg: String?): Int {
    val len = arg?.length ?: return -1
    return len
}
println(testElvisReturn(null))
println(testElvisReturn("test"))
"""
    res = compiler.compileAndRun("test_p9_regressions.al", source)
    assert res is True, "compileAndRun must succeed"
    assert not compiler.hasException(), "must not have exception"
    lines = [l.strip() for l in compiler.getOutput().strip().splitlines()]
    assert lines == ["10", "10", "-1", "4"], f"Unexpected output: {lines}"


def test_p9_vm_crash_recovery_and_no_corrupt_compiler():
    """2. VM sập không làm hỏng compiler:
    - hasException() là True, getException() trả thông báo ngắn;
    - không in bytecode vào getOutput();
    - stack và trạng thái VM được reset để lần chạy sau trên CÙNG compiler chạy đúng (println(1 + 2) in 3).
    """
    compiler = autolang.ACompiler()

    # 1. Chạy ca sập (runtime exception do chia 0)
    compiler.clearOutput()
    crash_source = """
var zero = 0
val x = 100 / zero
"""
    res_crash = compiler.compileAndRun("test_crash.al", crash_source)
    assert res_crash is False, "Lần chạy sập phải trả về False"
    assert compiler.hasException() is True, "hasException() phải là True khi VM sập"

    exc = compiler.getException()
    assert isinstance(exc, dict), f"getException() phải trả về dict, nhận được {type(exc)}"
    assert "message" in exc, f"getException() dict phải có trường 'message', nhận được {exc}"
    assert len(exc["message"]) > 0, "Thông báo ngoại lệ không được rỗng"

    crash_output = compiler.getOutput()
    assert "Function .main, bytecode at position" not in crash_output, (
        f"getOutput() không được chứa bản dump bytecode, nhưng lại có: {crash_output}"
    )

    # 2. Chạy tiếp println(1 + 2) trên CÙNG compiler instance
    compiler.clearOutput()
    ok_source = """
println(1 + 2)
"""
    res_ok = compiler.compileAndRun("test_ok.al", ok_source)
    assert res_ok is True, f"Lần chạy kế tiếp trên cùng compiler phải thành công, exc: {compiler.getException()}"
    assert compiler.hasException() is False, "hasException() lần kế tiếp phải là False"
    assert compiler.getException() is None, f"getException() lần kế tiếp phải là None, got {compiler.getException()}"

    ok_output = compiler.getOutput().strip()
    assert ok_output == "3", f"Expected '3', got '{ok_output}'"

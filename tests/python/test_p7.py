import os
import sys

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

def test_p7_full():
    print("=== Test P7: Full Reflection API ===")
    compiler = autolang.ACompiler()

    # 1. Test getLibraries trước khi nạp
    libs = compiler.getLibraries()
    assert isinstance(libs, list)
    lib_map = {l["name"]: l for l in libs}
    assert "stdlib" in lib_map
    assert "std/math" in lib_map
    assert "std/time" in lib_map
    assert lib_map["stdlib"]["autoImport"] == True
    assert lib_map["std/math"]["autoImport"] == False
    assert lib_map["std/math"]["loaded"] == False

    # 2. Biên dịch kịch bản với @import("std/math") và @import("std/time")
    code = """@import("std/math")
@import("std/time")
class Box<T extends Math, R> {
    val x: Int = 10
    var y: String? = null
    lateinit var z: List<Int>
    var m: Map<String, Array<Int>>? = null
    fun getX(): Int = x
    fun testGenericT(item: T): T = item
}

enum Color {
    RED, GREEN, BLUE;
}

var m: Map<String, Array<Int>>? = null
fun customGlobalFunc(a: Int, b: String = "default", mapParam: Map<String, Array<Int>>? = null): Void {}
"""
    res = compiler.compile("main.al", code)
    print("res compile:", res, "hasErr:", compiler.hasCompilerError(), "exc:", compiler.getException())
    print("compiler output:", compiler.getOutput())
    assert res == True, "Compilation failed"

    # Kiểm tra trạng thái thư viện sau nạp
    libs_after = compiler.getLibraries()
    lib_map_after = {l["name"]: l for l in libs_after}
    assert lib_map_after["stdlib"]["loaded"] == True
    assert lib_map_after["std/math"]["loaded"] == True
    assert lib_map_after["std/time"]["loaded"] == True

    # 3. Kiểm tra mọi hàm khai báo trong math.cpp có mặt trong ClassMeta của Math
    math_id = compiler.getTypeId("Math")
    assert math_id is not None and math_id != 4294967295
    math_meta = compiler.getClass(math_id)
    assert isinstance(math_meta, dict)
    assert math_meta["name"] == "Math"
    assert math_meta["library"] == "stdlib"

    math_cpp_functions = [
        "roundToInt", "roundToLong", "trunc", "truncate", "exp", "log", "ln", "log2", "log10",
        "sin", "cos", "tan", "asin", "acos", "atan", "atan2", "sinh", "cosh", "tanh",
        "asinh", "acosh", "atanh", "hypot", "IEEErem", "cbrt", "expm1", "ln1p",
        "withSign", "nextUp", "nextDown", "nextAfter", "ulp", "fmod", "random"
    ]
    for fn_name in math_cpp_functions:
        assert fn_name in math_meta["functions"], f"Math function {fn_name} declared in math.cpp must be in Math functions"
        has_std_math = any(f["library"] == "std/math" for f in math_meta["functions"][fn_name])
        assert has_std_math, f"Math function {fn_name} must have overload with library std/math"

    # Kiểm tra bản nạp chồng của sin
    sin_funcs = math_meta["functions"]["sin"]
    assert len(sin_funcs) >= 2, f"Expected at least 2 overloads of sin, got {len(sin_funcs)}"
    for sf in sin_funcs:
        assert sf["library"] == "std/math"
        assert sf["ownerClassId"] == math_id
        assert sf["isInfix"] == False
        assert sf["infixPrecedence"] == -1

    # Kiểm tra hàm cơ bản của Math mang library = "stdlib"
    for basic_name in ["abs", "min", "max"]:
        assert basic_name in math_meta["functions"]
        for bf in math_meta["functions"][basic_name]:
            assert bf["library"] == "stdlib"

    # 4. Kiểm tra hasDefault cho hàm toàn cục và hàm thành viên
    funcs = compiler.getFunctions()
    assert isinstance(funcs, list)
    func_map = {}
    for f in funcs:
        func_map.setdefault(f["name"], []).append(f)

    # println(value: Any? = "")
    assert "println" in func_map
    println_f = func_map["println"][0]
    assert len(println_f["params"]) == 1
    assert println_f["params"][0]["hasDefault"] == True
    assert println_f["isInfix"] == False
    assert println_f["infixPrecedence"] == -1

    # customGlobalFunc(a, b = "default", mapParam: Map<String, Array<Int>>? = null)
    assert "customGlobalFunc" in func_map
    custom_f = func_map["customGlobalFunc"][0]
    assert len(custom_f["params"]) == 3
    assert custom_f["params"][0]["hasDefault"] == False
    assert custom_f["params"][1]["hasDefault"] == True
    assert custom_f["params"][2]["hasDefault"] == True
    # Kiểm tra TypeMeta của tham số mapParam: Map<String, Array<Int>>?
    p_map = custom_f["params"][2]["type"]
    assert p_map["name"] == "Map"
    assert p_map["nullable"] == True
    assert p_map["isGeneric"] == False
    assert len(p_map["inputs"]) == 2
    assert p_map["inputs"][0]["name"] == "String"
    assert p_map["inputs"][0]["nullable"] == False
    assert p_map["inputs"][1]["name"] in ["Array", "List"]
    assert p_map["inputs"][1]["nullable"] == False
    assert len(p_map["inputs"][1]["inputs"]) == 1
    assert p_map["inputs"][1]["inputs"][0]["name"] == "Int"

    # joinToString: 5 tham số đều có mặc định
    arr_id = compiler.getTypeId("Array")
    arr_meta = compiler.getClass(arr_id)
    assert "joinToString" in arr_meta["functions"]
    join_f = arr_meta["functions"]["joinToString"][0]
    assert len(join_f["params"]) == 5
    for p in join_f["params"]:
        assert p["hasDefault"] == True

    # indexOf(str, startIndex = 0): chỉ tham số 2
    sb_id = compiler.getTypeId("StringBuilder")
    sb_meta = compiler.getClass(sb_id)
    assert "indexOf" in sb_meta["functions"]
    matched_indexOf = None
    for io in sb_meta["functions"]["indexOf"]:
        if len(io["params"]) == 2 and io["params"][0]["name"] == "str":
            matched_indexOf = io
            break
    assert matched_indexOf is not None, "Expected indexOf(str, startIndex) in StringBuilder"
    assert matched_indexOf["params"][0]["hasDefault"] == False
    assert matched_indexOf["params"][1]["hasDefault"] == True

    # withIndex
    assert "withIndex" in arr_meta["functions"]
    with_index_f = arr_meta["functions"]["withIndex"][0]
    assert len(with_index_f["params"]) >= 1
    assert with_index_f["params"][0]["hasDefault"] == True

    # Kiểm tra cờ isInfix và infixPrecedence
    int_id = compiler.getTypeId("Int")
    int_meta = compiler.getClass(int_id)
    assert "downTo" in int_meta["functions"]
    for dt in int_meta["functions"]["downTo"]:
        assert dt["isInfix"] == True
        assert dt["infixPrecedence"] == 9

    assert "step" in arr_meta["functions"]
    for st in arr_meta["functions"]["step"]:
        assert st["isInfix"] == True
        assert st["infixPrecedence"] == 8

    # 5. Kiểm tra Box generics <T extends Math, R> và thuộc tính (val, var, lateinit var)
    box_id = compiler.getTypeId("Box")
    box_meta = compiler.getClass(box_id)
    assert box_meta["name"] == "Box"
    assert box_meta["library"] == "" # Script người dùng

    # Generics
    assert len(box_meta["generics"]) == 2
    g_T = box_meta["generics"][0]
    assert g_T["name"] == "T"
    assert g_T["mustBeExtends"] == True
    assert g_T["extendClassId"] == math_id

    g_R = box_meta["generics"][1]
    assert g_R["name"] == "R"
    assert g_R["mustBeExtends"] == False
    assert g_R["extendClassId"] is None

    # Properties
    props = {p["name"]: p for p in box_meta["properties"]}
    assert "x" in props
    assert props["x"]["isVal"] == True
    assert props["x"]["isLateinit"] == False
    assert props["x"]["type"]["name"] == "Int"
    assert props["x"]["type"]["nullable"] == False
    assert props["x"]["type"]["isGeneric"] == False
    assert props["x"]["type"]["inputs"] == []

    assert "y" in props
    assert props["y"]["isVal"] == False
    assert props["y"]["isLateinit"] == False
    assert props["y"]["type"]["name"] == "String"
    assert props["y"]["type"]["nullable"] == True
    assert props["y"]["type"]["isGeneric"] == False

    assert "z" in props
    assert props["z"]["isVal"] == False
    assert props["z"]["isLateinit"] == True
    assert props["z"]["type"]["name"] in ["Array", "List"]
    assert props["z"]["type"]["nullable"] == False
    assert len(props["z"]["type"]["inputs"]) == 1
    assert props["z"]["type"]["inputs"][0]["name"] == "Int"

    # Kiểm tra TypeMeta của biến m: Map<String, Array<Int>>? trong Box
    assert "m" in props
    assert props["m"]["isVal"] == False
    assert props["m"]["isLateinit"] == False
    assert props["m"]["type"]["name"] == "Map"
    assert props["m"]["type"]["nullable"] == True
    assert props["m"]["type"]["isGeneric"] == False
    assert len(props["m"]["type"]["inputs"]) == 2
    assert props["m"]["type"]["inputs"][0]["name"] == "String"
    assert props["m"]["type"]["inputs"][0]["nullable"] == False
    assert props["m"]["type"]["inputs"][0]["inputs"] == []
    assert props["m"]["type"]["inputs"][1]["name"] in ["Array", "List"]
    assert props["m"]["type"]["inputs"][1]["nullable"] == False
    assert len(props["m"]["type"]["inputs"][1]["inputs"]) == 1
    assert props["m"]["type"]["inputs"][1]["inputs"][0]["name"] == "Int"
    assert props["m"]["type"]["inputs"][1]["inputs"][0]["nullable"] == False

    # Tham số và kiểu trả về generic T
    assert "testGenericT" in box_meta["functions"]
    tgt_f = box_meta["functions"]["testGenericT"][0]
    assert tgt_f["params"][0]["type"]["name"] == "T"
    assert tgt_f["params"][0]["type"]["isGeneric"] == True
    assert tgt_f["returnType"]["name"] == "T"
    assert tgt_f["returnType"]["isGeneric"] == True

    # 6. Kiểm tra enumEntries
    color_id = compiler.getTypeId("Color")
    color_meta = compiler.getClass(color_id)
    assert color_meta["enumEntries"] == ["RED", "GREEN", "BLUE"]

    # 7. Kiểm tra Aliases
    aliases = compiler.getAliases()
    alias_dict = {a["name"]: a["target"] for a in aliases}
    assert "kotlin.math" in alias_dict
    assert alias_dict["kotlin.math"] == "Math"

    # 8. Kiểm tra id không tồn tại trả về None
    assert compiler.getClass(999999) is None
    assert compiler.getFunction(999999) is None

    # 9. Kiểm tra tương thích dữ liệu API cũ trên các lớp stdlib
    for type_name in ["Array", "String", "Map", "Set", "Math", "Json", "Time", "File"]:
        tid = compiler.getTypeId(type_name)
        assert tid is not None and tid != 4294967295
        cm = compiler.getClass(tid)
        assert cm is not None
        expected_members = compiler.getMembersForType(type_name)
        actual_prop_names = {p["name"] for p in cm["properties"]}
        actual_func_names = set(cm["functions"].keys())
        for exp in expected_members:
            has_member = (exp in actual_prop_names) or (exp in actual_func_names)
            assert has_member, f"Class {type_name} missing member {exp} in ClassMeta (properties or functions)"

    # 10. Kiểm tra getIntrinsicFunctions
    intrinsics = compiler.getIntrinsicFunctions()
    assert isinstance(intrinsics, list)
    assert len(intrinsics) == 39
    int_map = {item["name"]: item for item in intrinsics}

    # Kiểm tra cấu trúc các trường
    for item in intrinsics:
        for key in ["name", "returnClass", "nullable", "elementFromArguments", "minArgs", "maxArgs"]:
            assert key in item, f"Key '{key}' missing in intrinsic entry: {item}"

    # listOf -> returnClass đúng là Array
    assert "listOf" in int_map
    assert int_map["listOf"]["returnClass"] == "Array"
    assert int_map["listOf"]["elementFromArguments"] == True
    assert int_map["listOf"]["minArgs"] == 0
    assert int_map["listOf"]["maxArgs"] == -1

    # emptyList -> maxArgs 0
    assert "emptyList" in int_map
    assert int_map["emptyList"]["returnClass"] == "Array"
    assert int_map["emptyList"]["elementFromArguments"] == False
    assert int_map["emptyList"]["minArgs"] == 0
    assert int_map["emptyList"]["maxArgs"] == 0

    # mapOf
    assert "mapOf" in int_map
    assert int_map["mapOf"]["returnClass"] == "Map"
    assert int_map["mapOf"]["elementFromArguments"] == True
    assert int_map["mapOf"]["minArgs"] == 0
    assert int_map["mapOf"]["maxArgs"] == -1

    # setOf
    assert "setOf" in int_map
    assert int_map["setOf"]["returnClass"] == "Set"
    assert int_map["setOf"]["elementFromArguments"] == True
    assert int_map["setOf"]["minArgs"] == 0
    assert int_map["setOf"]["maxArgs"] == -1

    # intArrayOf
    assert "intArrayOf" in int_map
    assert int_map["intArrayOf"]["returnClass"] == "Array"
    assert int_map["intArrayOf"]["elementFromArguments"] == True
    assert int_map["intArrayOf"]["minArgs"] == 0
    assert int_map["intArrayOf"]["maxArgs"] == -1

    # pairOf
    assert "pairOf" in int_map
    assert int_map["pairOf"]["returnClass"] == "Pair"
    assert int_map["pairOf"]["elementFromArguments"] == True
    assert int_map["pairOf"]["minArgs"] == 2
    assert int_map["pairOf"]["maxArgs"] == 2

    # Đối chiếu kết quả biên dịch
    test_code = """
val pyListOf = listOf(1, 2)
val pyEmptyList = emptyList<Int>()
val pyMapOf = mapOf(1 to 2)
val pySetOf = setOf(1, 2)
val pyIntArrayOf = intArrayOf(1, 2)
"""
    assert compiler.compile("py_intrinsics.al", test_code)
    for vname, expected_cls in [
        ("pyListOf", int_map["listOf"]["returnClass"]),
        ("pyEmptyList", int_map["emptyList"]["returnClass"]),
        ("pyMapOf", int_map["mapOf"]["returnClass"]),
        ("pySetOf", int_map["setOf"]["returnClass"]),
        ("pyIntArrayOf", int_map["intArrayOf"]["returnClass"])
    ]:
        vid = compiler.getGlobalVariableId(vname)
        assert vid != 4294967295
        tstr = compiler.getGlobalVariableType(vid)
        assert tstr.startswith(expected_cls), f"{vname} expected prefix {expected_cls}, got {tstr}"

    print("\nALL P7 PYTHON TESTS PASSED SUCCESSFULLY!")

def test_generic_parsing_cases():
    print("\n=== Test Generic Parsing Cases ===")
    compiler = autolang.ACompiler()

    # Invalid cases should produce compiler error and not crash
    invalid_cases = [
        ("val a: Array<?Int> = [1]\nprintln(a)", "Array<?Int>"),
        ("val a: Array<,Int> = [1]\nprintln(a)", "Array<,Int>"),
        ("val a: Array<1> = [1]\nprintln(a)", "Array<1>"),
        ("val a: Array<>Int = [1]\nprintln(a)", "Array<>Int"),
        ("val a: Array<Int??> = [1]\nprintln(a)", "Array<Int??>"),
    ]
    for code, desc in invalid_cases:
        res = compiler.compile("test.al", code)
        assert res == False or compiler.hasCompilerError(), f"Expected compile error for {desc}"
        compiler.refresh()

    # Valid cases should compile without error
    valid_cases = [
        ("val a: Array<Float?> = [1.0]\nprintln(a)", "Array<Float?>"),
        ("val a: Array<Int?>? = [1]\nprintln(a)", "Array<Int?>?"),
        ("val a: Map<String, Int?> = mapOf()\nprintln(a)", "Map<String, Int?>"),
        ("val a: Array<Array<Int>> = [[1]]\nprintln(a)", "Array<Array<Int>>"),
        ("val a: Int? = 1\nprintln(a)", "Int?"),
    ]
    for code, desc in valid_cases:
        res = compiler.compile("test.al", code)
        assert res == True and not compiler.hasCompilerError(), f"Expected compile success for {desc}"
        compiler.refresh()

    print("ALL GENERIC PARSING TESTS PASSED SUCCESSFULLY!")

if __name__ == "__main__":
    test_p7_full()
    test_generic_parsing_cases()

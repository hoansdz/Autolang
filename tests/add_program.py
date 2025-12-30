# stress_1000_classes.py
NUM_CLASSES = 10000

with open("stress_1000_classes.txt", "w", encoding="utf-8") as f:
    for i in range(1, NUM_CLASSES + 1):
        class_name = f"Class{i}"
        func_name = f"func{i}"
        f.write(f"class {class_name} {{\n")
        f.write(f"    func {func_name}() {{\n")
        f.write(f"        var x = {i} * 2\n")
        f.write(f"        var y = x + {i}\n")
        f.write(f"    }}\n")
        f.write(f"}}\n\n")

    # thêm main function để file hợp lệ
    f.write("func main() {\n")
    f.write("var a? = 0\n")
    for i in range(1, 40):
        f.write("if (a == 1) {a = 2} else {}    \n")
        f.write("for (a in 0..5) {}    \n")
    f.write("}\n")

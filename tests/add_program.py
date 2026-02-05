# stress_10k_flat.py
NUM_CLASSES = 100000

with open("tests/stress_10k_flat.txt", "w", encoding="utf-8") as f:
    # 1. Base Class duy nhất
    f.write("class RootBase {\n")
    f.write("    var id: Int = 0\n")
    f.write("    constructor(id: Int) { this.id = id }\n")
    f.write("    func rootInfo() { println(\"RootBase\") }\n")
    f.write("}\n\n")

    # 2. 10,000 Classes kế thừa ngang hàng từ RootBase
    for i in range(1, NUM_CLASSES + 1):
        class_name = f"Class{i}"
        f.write(f"class {class_name} extends RootBase {{\n")
        f.write(f"    var val{i}: Int = {i}\n") # Tên biến duy nhất để tránh lỗi redeclared
        
        f.write(f"    constructor() {{\n")
        f.write(f"        super({i})\n")
        f.write(f"    }}\n")
        
        f.write(f"    func info{i}() {{\n")
        f.write(f"        println(\"I am class {i}\")\n")
        f.write(f"    }}\n")
        f.write(f"}}\n\n")

    # 3. Main function
    f.write("func main() {\n")
    f.write(f"    println(\"Stress test 10k classes starting...\")\n")
    f.write(f"    val obj = Class{NUM_CLASSES}()\n")
    f.write(f"    println(\"Created obj with val: \" + obj.val{NUM_CLASSES}.toString())\n")
    f.write("}\n")
    f.write("main()\n")
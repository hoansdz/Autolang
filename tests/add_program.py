# stress_10k_flat.py
NUM_CLASSES = 100000

with open("tests/stress_10k_flat.txt", "w", encoding="utf-8") as f:
    # 1. Single Base Class
    f.write("class RootBase {\n")
    f.write("    var id: Int = 0\n")
    f.write("    constructor(id: Int) { this.id = id }\n")
    f.write("    fun rootInfo() { println(\"RootBase\") }\n")
    f.write("}\n\n")

    # 2. Classes inheriting in parallel from RootBase
    for i in range(1, NUM_CLASSES + 1):
        class_name = f"Class{i}"
        f.write(f"class {class_name} extends RootBase {{\n")
        f.write(f"    var val{i}: Int = {i}\n") # Unique variable name to avoid redeclared error
        
        f.write(f"    constructor() {{\n")
        f.write(f"        super({i})\n")
        f.write(f"    }}\n")
        
        f.write(f"    fun info{i}() {{\n")
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
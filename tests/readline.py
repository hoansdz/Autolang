import os
import re

def count_lines(directory):
    total_all_lines = 0
    total_non_empty_lines = 0
    total_code_lines = 0
    file_stats = []
    
    for root, dirs, files in os.walk(directory):
        for file in files:
            if file.endswith('.atl'):
                filepath = os.path.join(root, file)
                try:
                    with open(filepath, 'r', encoding='utf-8') as f:
                        raw_content = f.read()
                    
                    lines = raw_content.splitlines()
                    all_lines_count = len(lines)
                    non_empty_lines_count = len([l for l in lines if l.strip()])
                    
                    # 1. Loai bo block comment /* ... */ qua nhieu dong
                    content_no_block = re.sub(r'/\*.*?\*/', '', raw_content, flags=re.DOTALL)
                    
                    # 2. Dem cac dong code thuc te (da loai bo // va #)
                    code_lines_count = 0
                    for line in content_no_block.splitlines():
                        # Cat bo phan single line comment // hoặc #
                        line_clean = line.split('//')[0].split('#')[0].strip()
                        if line_clean:
                            code_lines_count += 1
                            
                    file_stats.append((filepath, all_lines_count, non_empty_lines_count, code_lines_count))
                    total_all_lines += all_lines_count
                    total_non_empty_lines += non_empty_lines_count
                    total_code_lines += code_lines_count
                except Exception as e:
                    print(f"Error reading {filepath}: {e}")
                    
    # Sap xep cac file theo so dong code thuc te giam dan
    file_stats.sort(key=lambda x: x[3], reverse=True)
    
    print("LINE COUNT STATISTICS FOR CORRECTNESS TESTS:")
    print("-" * 110)
    print(f"{'File Name':<45} | {'Total Lines':>12} | {'Non-Empty':>12} | {'Code Lines':>12}")
    print("-" * 110)
    for path, total, non_empty, code in file_stats:
        rel_path = os.path.relpath(path, directory)
        print(f"{rel_path:<45} | {total:>12} | {non_empty:>12} | {code:>12}")
    print("-" * 110)
    print(f"TOTAL: {len(file_stats)} files | {total_all_lines:>12} total | {total_non_empty_lines:>12} non-empty | {total_code_lines:>12} code lines")

if __name__ == '__main__':
    # target_dir is the 'correctness' directory inside the parent directory of this script
    script_dir = os.path.dirname(os.path.abspath(__file__))
    target_dir = os.path.join(script_dir, "correctness")
    count_lines(target_dir)

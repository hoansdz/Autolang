import os

def count_lines(directory):
    total_all_lines = 0
    total_non_empty_lines = 0
    file_stats = []
    
    for root, dirs, files in os.walk(directory):
        for file in files:
            if file.endswith('.atl'):
                filepath = os.path.join(root, file)
                try:
                    with open(filepath, 'r', encoding='utf-8') as f:
                        lines = f.readlines()
                    all_lines_count = len(lines)
                    non_empty_lines_count = len([line for line in lines if line.strip()])
                    file_stats.append((filepath, all_lines_count, non_empty_lines_count))
                    total_all_lines += all_lines_count
                    total_non_empty_lines += non_empty_lines_count
                except Exception as e:
                    print(f"Error reading {filepath}: {e}")
                    
    # Sap xep cac file theo so dong khong rong giam dan
    file_stats.sort(key=lambda x: x[2], reverse=True)
    
    print("LINE COUNT STATISTICS FOR CORRECTNESS TESTS:")
    print("-" * 90)
    print(f"{'File Name':<50} | {'Total Lines':>15} | {'Non-Empty Lines':>15}")
    print("-" * 90)
    for path, total, non_empty in file_stats:
        rel_path = os.path.relpath(path, directory)
        print(f"{rel_path:<50} | {total:>15} | {non_empty:>15}")
    print("-" * 90)
    print(f"TOTAL: {len(file_stats)} files | {total_all_lines:>15} total lines | {total_non_empty_lines:>15} non-empty lines")

if __name__ == '__main__':
    # target_dir is the 'correctness' directory inside the parent directory of this script
    script_dir = os.path.dirname(os.path.abspath(__file__))
    target_dir = os.path.join(script_dir, "correctness")
    count_lines(target_dir)

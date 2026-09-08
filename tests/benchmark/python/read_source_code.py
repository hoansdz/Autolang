import os
import time
from collections import defaultdict

class FileAnalysis:
    def __init__(self, path):
        if not os.path.exists(path):
            raise Exception(f"{path} not exists")
        
        self.total_size = 0
        self.total_file = 0
        self.total_folder = 0
        self.file_extension = defaultdict(int)
        self.cplusplus_lines = 0
        self.autolang_lines = 0
        
        self.analysis(path)

    def analysis(self, root_path):
        # os.walk traverses directory tree safely without double recursion
        for dirpath, dirnames, filenames in os.walk(root_path):
            
            # SKIP ignored directories (customizable)
            if 'build' in dirnames:
                dirnames.remove('build')
            if '.git' in dirnames:
                dirnames.remove('.git')
                
            self.total_folder += 1
            
            for filename in filenames:
                self.total_file += 1
                filepath = os.path.join(dirpath, filename)
                
                # Calculate file size
                try:
                    self.total_size += os.path.getsize(filepath)
                except OSError:
                    continue # Skip if file is locked or permission error
                
                # Get file extension
                _, extension = os.path.splitext(filename)
                extension = extension.lower()
                self.file_extension[extension] += 1
                
                # Count lines
                if extension == '.atl':
                    self.autolang_lines += self.get_count_line(filepath)
                elif extension in ('.hpp', '.cpp', '.h'):
                    self.cplusplus_lines += self.get_count_line(filepath)

    def get_count_line(self, filepath):
        count = 0
        try:
            # errors='ignore' to prevent crashes on binary files with mistaken extensions
            with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
                for line in f:
                    if line.strip():  # Matches newStr.isEmpty() logic
                        count += 1
        except Exception:
            pass
        return count

def main():
    # Use perf_counter for precise timing in Python
    start_time = time.perf_counter()
    
    target_path = r"./"
    print(f"Analyzing: {target_path}...\n")
    
    a = FileAnalysis(target_path)
    
    print(f"Total file: {a.total_file}")
    print(f"Total folder: {a.total_folder - 1}") # -1 to exclude root directory
    print(f"Total size: {a.total_size / 1024 / 1024:.2f} MB")
    
    # Print extensions list sorted descending
    for ext, count in sorted(a.file_extension.items(), key=lambda item: item[1], reverse=True):
        print(f"Extension {ext if ext else '[No Extension]'}: {count}")
        
    print(f"Total c++ line: {a.cplusplus_lines}")
    print(f"Total autolang line: {a.autolang_lines}")
    
    end_time = time.perf_counter()
    print(f"\nTime: {end_time - start_time:.4f} seconds")

if __name__ == "__main__":
    main()
import os

# Directory to search
search_directory = '.'

# Find all .cpp and .h files
cpp_h_files = []
for root, dirs, files in os.walk(search_directory):
    for file in files:
        if file.endswith('.cpp') or file.endswith('.h'):
            full_path = os.path.join(root, file)
            relative_path = os.path.relpath(full_path, search_directory)
            cpp_h_files.append(relative_path)

cpp_h_files.sort()

print(cpp_h_files)

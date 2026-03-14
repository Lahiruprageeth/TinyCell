import os
import glob
paths = ['src/**/*.cpp', 'src/**/*.h', 'lib/LPCell/**/*.cpp', 'lib/LPCell/**/*.h', 'lib/LPCell/**/*.json', 'lib/LPCell/**/*.ino', 'lib/LPCell/**/*.md']
files = []
for p in paths:
    files.extend(glob.glob(p, recursive=True))

for f in files:
    with open(f, 'r', encoding='utf-8') as file:
        content = file.read()
    
    new_content = content.replace('TinyCell', 'LPCell').replace('tinycell', 'lpcell')
    
    if new_content != content:
        with open(f, 'w', encoding='utf-8') as file:
            file.write(new_content)
        print(f"Updated {f}")
print("Done")

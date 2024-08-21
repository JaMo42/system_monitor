# Retrieves all custom types from our sources, `indent` must be told about them
# manually to identify them as types, so for example a pointer will be
# indentified as a pointer instead of multiplication and be properly aligned.
import re
from sys import argv, stdout

FPTR = re.compile(r"(\w[\w\s\*]*\(\s*\*\s*\w+\s*\))\s*\(\s*([^\)]*)\s*\)")

def get_types(code: str):
    END = len(code)
    types = []
    for typedef in re.finditer(r"typedef", code):
        space = typedef.end()
        depth = 0
        is_struct_def = False
        for index in range(typedef.start(), END):
            if code[index] == ' ':
                space = index
            elif code[index] == ';' and depth == 0:
                line = code[typedef.end():index]
                if is_struct_def or not FPTR.search(line):
                    types.append(code[space+1:index])
                #else:
                #    print("Filtered function pointer:", line)
                break
            elif code[index] == '{':
                is_struct_def = True
                depth += 1
            elif code[index] == '}':
                depth -= 1
    return types


all_types = []
for file in argv[1:]:
#for file in ["src/input.h"]:
    with open(file) as f:
        code = f.read().strip()
        types = get_types(code)
        #print(file, types)
        all_types += types

for t in all_types:
    stdout.write(f" -T {t}")

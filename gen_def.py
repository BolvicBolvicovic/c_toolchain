#!/usr/bin/python3

import os
import sys

KEYWORDS_TYPES = ('struct', 'enum', 'union')
KEYWORD_FUNC = 'function'

def parse_file(filepath):
    found_types = []
    found_funcs = []

    if not os.path.exists(filepath):
        return found_types, found_funcs

    with open(filepath, 'r') as f:
        lines = f.readlines()

    i = 0
    while i < len(lines):
        line = lines[i].strip()

        matched_kw = None
        if any(line.startswith(kw) for kw in KEYWORDS_TYPES):
            matched_kw = 'type'
        elif line.startswith(KEYWORD_FUNC):
            matched_kw = 'func'

        if matched_kw:
            full_sig = ""
            # Capture until '{'
            while i < len(lines):
                current_line = lines[i].strip()
                full_sig += " " + current_line
                if '{' in current_line:
                    break
                i += 1
            
            clean_sig = full_sig.split('{')[0].strip()

            if matched_kw == 'type':
                parts = clean_sig.split()
                if len(parts) >= 2:
                    # Capture kind (struct/etc) and name
                    found_types.append((parts[0], parts[1]))
            
            elif matched_kw == 'func':
                # Strip 'function' keyword for the prototype
                func_declaration = clean_sig[len(KEYWORD_FUNC):].strip()
                found_funcs.append(func_declaration)

        i += 1
    return found_types, found_funcs

def generate_def_file(input_path):
    # Get directory and base filename (e.g., 'packer' from 'src/packer.c')
    file_dir = os.path.dirname(input_path)
    file_raw_name = os.path.splitext(os.path.basename(input_path))[0]
    
    # Requirement: Filename in Uppercase for the Guard
    guard_name = f"{file_raw_name.upper()}_DEF_H"
    
    base_path = os.path.join(file_dir, file_raw_name)
    c_types, c_funcs = parse_file(f"{base_path}.c")
    h_types, h_funcs = parse_file(f"{base_path}.h")

    # Combine unique items
    all_types = list(dict.fromkeys(c_types + h_types))
    all_funcs = list(dict.fromkeys(c_funcs + h_funcs))

    output_path = f"{base_path}DEF.h"
    
    with open(output_path, 'w') as f:
        # Uppercase Include Guard
        f.write(f"#ifndef {guard_name}\n")
        f.write(f"#define {guard_name}\n\n")

        # Function Macro Check
        f.write("#ifndef function\n")
        f.write("# define function\n")
        f.write("#endif\n\n")

        f.write("#ifdef GEN_DEF\n")

        f.write("// struct typedefs\n")
        for kind, name in all_types:
            f.write(f"typedef {kind} {name} {name};\n")

        f.write("\n// function defs\n")
        for func in all_funcs:
            f.write(f"{func};\n")

        f.write("#endif // GEN_DEF\n")
        f.write(f"#endif // {guard_name}\n")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        sys.exit(1)
    generate_def_file(sys.argv[1])

#!/usr/bin/python3
import os
import sys

def generate_makefile():
    # 1. Handle Binary Name from argv
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <binary_name>")
        sys.exit(1)
    
    bin_name = sys.argv[1]
    src_dir = "src"

    # 2. Bootstrap: Create src/ and main.c if missing
    if not os.path.exists(src_dir):
        os.makedirs(src_dir)
        print(f"Created directory: {src_dir}")

    main_path = os.path.join(src_dir, "main.c")
    if not os.path.exists(main_path):
        with open(main_path, "w") as f:
            f.write('#include <stdio.h>\n\nint\nmain(/*int argc, char** argv*/)\n{\n\tprintf("Hello, World!\\n");\n\treturn 0;\n}\n')
        print(f"Generated default: {main_path}")

    # 3. Dynamic Source Discovery
    # We gather all .c files to build the SRCS list
    c_files = [f"src/{f}" for f in os.listdir(src_dir) if f.endswith('.c')]
    src_list = " ".join(c_files)

    # 4. Construct the Makefile
    makefile_content = f"""BIN={bin_name}
CC=gcc
CFLAGS=-Werror -Wextra -Wall
INC=-I./src -I./c_toolchain
GEN_TOOL=./c_toolchain/gen_def.h

SRCS={src_list}
OBJS=$(SRCS:.c=.o)

all: $(BIN)

# Link the final binary
$(BIN): $(OBJS)
\t$(CC) $(CFLAGS) $(OBJS) -o $@

# Specific rule for main.o: Compiles WITHOUT running the GEN_TOOL
src/main.o: src/main.c
\t$(CC) $(CFLAGS) $(INC) -c $< -o $@

# General Pattern Rule: Compiles .c to .o and runs the GEN_TOOL
%.o: %.c
\t$(GEN_TOOL) $<
\t$(CC) $(CFLAGS) $(INC) -c $< -o $@

clean:
\trm -rf $(OBJS) $(BIN)

.PHONY: all clean
"""

    with open("Makefile", "w") as f:
        f.write(makefile_content)
    
    print(f"Successfully generated Makefile for '{bin_name}'.")

if __name__ == "__main__":
    generate_makefile()

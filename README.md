# llvm-compiler-projects

Compiler construction projects built with the LLVM/Clang tooling API.

---

## source-to-source

A source-to-source compiler implemented as a **Recursive AST Visitor** in Clang. It takes a C program as input and produces a transformed C program that injects a `Controller` struct, allowing control flow (if, else, while, for, do-while, break, continue) and function calls to be overridden at runtime.

**Usage:**
```bash
source-to-source input.c -o output.c --
```

**Examples:**
- `examples/before.c` — original C program before transformation
- `examples/after.c` — transformed output, use this to verify your result

---

## ir-generator

A code generator implemented as a **non-recursive AST Visitor** in Clang. It takes a C++ source file and generates a simple **three-address code IR in SSA form**, handling expressions, control flow, loops, and function calls.

**Usage:**
```bash
ir-generator input.cpp -o output.ir --
```

**Examples:**
- `examples/test.cpp` — C++ input covering all supported language features
- `examples/test.ir` — expected IR output, use this to verify your result

---

## Requirements

- LLVM source code downloaded and built
- Clang (built as part of LLVM)
- CMake

## Build

Both tools are built as Clang tools inside the LLVM source tree.

```bash
# From the LLVM source root
cd clang/tools
mkdir source-to-source        # or ir-generator
# Copy .cpp and CMakeLists.txt into that directory
# Add to clang/tools/CMakeLists.txt:
#   add_clang_subdirectory(source-to-source)

# From your build directory:
make -j$(nproc)
```

---

## Repository Structure

```
llvm-compiler-projects/
├── README.md
├── source-to-source/
│   ├── SourceToSource.cpp    # Source-to-source compiler implementation
│   ├── CMakeLists.txt
│   └── examples/
│       ├── before.c          # Input C program before transformation
│       └── after.c           # Output C program after transformation
└── ir-generator/
    ├── IRGenerator.cpp       # IR code generator implementation
    ├── CMakeLists.txt
    └── examples/
        ├── test.cpp          # Example C++ input covering all supported features
        └── test.ir           # Expected IR output for test.cpp
```

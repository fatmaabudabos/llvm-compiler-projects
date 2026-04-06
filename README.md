# llvm-compiler-projects

Compiler construction projects built with the LLVM/Clang tooling API.

---

## source-to-source

A Clang source-to-source compiler that transforms a C program to inject a `Controller` struct, which can override control flow (if, else, while, for, do-while, break, continue) and function calls at runtime.

**Usage:**
```bash
source-to-source input.c -o output.c --
```

See [`source-to-source/examples/before.c`](source-to-source/examples/before.c) and [`source-to-source/examples/after.c`](source-to-source/examples/after.c).

---

## ir-generator

A Clang code generator that compiles a C++ source file into a simple three-address code IR in SSA form.

**Usage:**
```bash
ir-generator input.cpp -o output.ir --
```

See [`ir-generator/examples/test.cpp`](ir-generator/examples/test.cpp) and [`ir-generator/examples/test.ir`](ir-generator/examples/test.ir).

---

## Build

Both tools are built as Clang tools inside the LLVM source tree. Requires LLVM/Clang source and CMake.

```bash
# From the LLVM source root
cd clang/tools
mkdir source-to-source       # or ir-generator
# Copy the .cpp and CMakeLists.txt into that directory
# Add to clang/tools/CMakeLists.txt:
#   add_clang_subdirectory(source-to-source)

# Then from your build directory:
make -j$(nproc)
```

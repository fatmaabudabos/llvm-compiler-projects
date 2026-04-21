# llvm-compiler-projects
Compiler construction projects built with the LLVM/Clang tooling API.

---

## source-to-source
A source-to-source compiler implemented as a Recursive AST Visitor in Clang. It takes a C program as input and produces a transformed C program that injects a Controller struct, allowing control flow (if, else, while, for, do-while, break, continue) and function calls to be overridden at runtime.


### Examples
- `examples/before.c` — original C program before transformation  
- `examples/after.c` — transformed output (used to verify correctness)

---

## ir-generator
A code generator implemented as a non-recursive AST Visitor in Clang. It takes a C++ source file and generates a simple three-address code IR in SSA form, handling expressions, control flow, loops, and function calls.

### Examples
- `examples/test.cpp` — C++ input covering supported language features  
- `examples/test.ir` — expected IR output  

---

## adce-pass
An LLVM optimization pass implementing **Aggressive Dead Code Elimination (ADCE)**. The pass operates on LLVM IR in SSA form and removes instructions that do not contribute to observable program behavior.

### The pass:
- Marks trivially live instructions (terminators, memory writes, and side-effecting operations)  
- Propagates liveness backward through operand dependencies using a worklist  
- Eliminates all non-live instructions in reachable basic blocks  


### Examples
- `examples/test1.ll` — input IR before optimization  
- `examples/test1.opt.ll` — expected optimized output  
- `examples/test2.ll` — input IR  
- `examples/test2.opt.ll` — expected optimized output  

---

## Requirements
- LLVM source code downloaded and built  
- Clang (built as part of LLVM)  
- CMake
- 
## Repository Structure

llvm-compiler-projects/
├── README.md
├── adce-pass/
│   ├── ADCEPass.cpp
│   ├── ADCEPass.h
│   ├── test1.ll
│   ├── test1.opt.ll
│   ├── test2.ll
│   └── test2.opt.ll
├── ir-generator/
│   ├── CMakeLists.txt
│   ├── IRGenerator.cpp
│   └── examples/
│       ├── test.cpp
│       └── test.ir
└── source-to-source/
    ├── CMakeLists.txt
    ├── SourceToSource.cpp
    └── examples/
        ├── after.c
        └── before.c

---

## Build
Both tools are built as Clang tools inside the LLVM source tree.






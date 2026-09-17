# Custom Two-Pass Assembler in ANSI C

![C](https://img.shields.io/badge/Language-ANSI%20C%20(C90)-blue.svg)
![Build](https://img.shields.io/badge/Build-Makefile-green.svg)
![Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20Ubuntu-orange.svg)
![Memory](https://img.shields.io/badge/Memory-Valgrind%20Clean-brightgreen.svg)

A modular software assembler implemented in standard ANSI C that translates simulated assembly source code into machine code (binary/hexadecimal format). The project handles macro expansions, validates syntactic and semantic correctness across multi-pass stages, and manages simulated memory architectures with dynamic data structures.

---

##  Key Features

* **Pre-Assembler Macro Engine:** Parses custom source files (`.as`) and dynamically unpacks multi-line macros into clean intermediate representations (`.am`).
* **Two-Pass Compilation:** Accurately separates label discovery and memory assignment (Pass 1) from operand encoding and relocation tracking (Pass 2).
* **Defensive Error Handling:** Detects invalid syntax, illegal addressing modes, and undefined jump destinations with precise file-and-line reporting.
* **Leak-Free Memory Management:** Implemented via custom linked lists and dynamic symbol lookups, verified clean using Valgrind.
* **Batch Execution:** Capable of processing multiple source files in sequence while keeping state isolated per file.

---

##  Technologies & Tools

| Category | Tools & Standards |
| :--- | :--- |
| **Language** | ANSI C (C90 standard, strict compilation flags) |
| **Build System** | GNU Make (Makefile) |
| **Debugging & Profiling** | GDB, Valgrind (Memcheck) |
| **Target Architecture** | Custom CPU emulator (Opcode bitfields, ARE encoding) |

---

##  How It Works (The Engine)

The assembler breaks compilation down into three distinct stages:

### Macro Expansion (Pre-Pass)
Scans for macro declarations (`mcro` / `mcroend`), caches macro bodies in memory, and writes out the `.am` source with expanded inline calls.

### First Pass (Pass 1)
* Scans the expanded code and parses directives (`.data`, `.string`, `.entry`, `.extern`) versus instructions (`mov`, `add`, `jmp`, `bne`, etc.).
* Calculates memory offsets using an **Instruction Counter (IC)** and **Data Counter (DC)**.
* Populates the **Symbol Table** and validates label uniqueness.

### Second Pass (Pass 2)
* Translates instructions into target machine words.
* Encodes register references, addressing modes, and **A/R/E** (Absolute / Relocatable / External) flag bits.
* Resolves forward jump branches and flags unresolved symbols.

---

##  Error Detection & Validation

The assembler operates defensively. If an error is detected, execution continues for diagnostic feedback across the rest of the file, but final binary outputs are withheld to avoid corrupt builds.

* **Syntax Errors:** Extra commas, missing commas, invalid characters, malformed numbers.
* **Semantic Errors:** Jumps to non-existent labels, using registers where immediate values are expected, re-defining existing symbols.
* **Format Checks:** Missing string quotation marks, out-of-range numerical constants.

---

## Input & Output Example

### Input (`example.as`)
```assembly
MAIN:    mov    r3, LENGTH
LOOP:    jmp    END
         prn    #-5
         sub    r1, r4
         bne    LOOP
END:     stop
LENGTH:  .data  6, -9, 15

##  How to Run

### Prerequisites
Ensure you have `gcc` and `make` installed on your Linux / Ubuntu environment:
```bash
sudo apt update && sudo apt install build-essential

### Build
Clone the repository and compile:
```bash
git clone [https://github.com/Yael-Ovadya/c-two-pass-assembler.git](https://github.com/Yael-Ovadya/c-two-pass-assembler.git)
cd c-two-pass-assembler
make

### Execution
Run the assembler with input files (omit the `.as` extension):
```bash
./assembler example

Clean Build Files
make clean

 Authors
Yael Ovadya

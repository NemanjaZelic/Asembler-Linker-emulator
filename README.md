


1. Assembler

Translates assembly code into relocatable object files.

Features:
- Two-pass assembly with symbol table
- Literal pool management with PC-relative addressing
- Section support (`.section`, `.global`, `.extern`)
- Data directives (`.word`, `.skip`, `.ascii`)
- Relocation entries for external symbols

Usage:*
```bash
make                          
./asembler -o output.o input.s
```

Assembly Syntax:
```asm
.extern handler
.global my_start

.section my_code
my_start:
    ld $0x12345678, %sp       # Load immediate
    ld $handler, %r1          # Load symbol address
    csrwr %r1, %handler       # Write CSR
    call mathAdd              # Function call
    halt
```

2. Linker

Links multiple object files into a single executable hex file.

Features:
- Symbol resolution and relocation
- Section placement at specified addresses (`-place=section@address`)
- Hex output format for emulator
- Byte order conversion (big-endian object → little-endian hex)
- Initialization table generation for global variables

Usage:
```bash
cd linker && make            
./linker -hex \
  -place=my_code@0x40000000 \
  -place=math@0xF0000000 \
  -o program.hex \
  file1.o file2.o file3.o
```

 3. Emulator

Cycle-accurate simulator for the custom ISA.

Features:
- Full instruction set support
- Interrupt handling with CSR operations
- Memory-mapped I/O
- Debug trace output (PC, decoded instructions, register changes)
- Initialization table processing

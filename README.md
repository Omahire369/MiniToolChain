# MiniToolchain

A complete toolchain for a custom 64-bit virtual instruction set:
assembler, linker, object and executable formats, optimizer, virtual CPU,
disassembler and debugger — written from scratch in C++23, with no
compiler framework underneath.

```
 assembly source
       |
   lexer -> parser -> IR -> optimizer
       |
   two-pass assembler
       |
   .mobj  ->  linker  ->  .mexe
                             |
                +------------+------------+
                |                         |
           virtual CPU              disassembler
                |
            debugger
```

## Status

| Milestone | State |
|---|---|
| V0 — project foundation | done |
| V1 — ISA specification | done, **frozen** |
| V2 — encoder / decoder | done |
| V3 — lexer | next |
| V4–V16 | planned, see `docs/` |

Currently green: 50 tests across Debug, ASan and UBSan builds, with
`-Werror` and the full warning set.

## Build

```bash
cmake --preset debug
cmake --build build/debug
ctest --preset debug
```

Presets: `debug`, `release`, `asan`, `ubsan`, `coverage`.
Requires CMake 3.24+, Ninja, and GCC 13+ or Clang 17+. GoogleTest is
fetched automatically if it is not installed.

## Try it

```bash
$ ./build/debug/src/minitool isa
op     mnemonic format   notes
0x00   NOP      none
0x01   HALT     none     terminator
0x11   MOVI     regimm
...

$ ./build/debug/src/minitool decode 0x111000000000000A
111000000000000A  MOVI R1, 10

$ ./build/debug/src/minitool decode 0x7F00000000000000
error: unknown opcode
```

## The ISA in one paragraph

Sixteen 64-bit general-purpose registers, `PC`, `SP` and `FLAGS`. Every
instruction is exactly 64 bits: an 8-bit opcode, two 4-bit register
fields, and a 48-bit immediate. Load/store architecture, little-endian,
36 instructions. Branch displacements are relative to the instruction
*after* the branch. The full specification is
[`docs/isa.md`](docs/isa.md); it is frozen, and changes need an ADR.

## Design guarantees

The encoder and decoder are a **bijection**: every instruction has
exactly one encoding, and every word the decoder accepts re-encodes to
itself, bit for bit. Both directions are property-tested over hundreds of
thousands of cases. Reserved fields are validated rather than ignored,
out-of-range immediates are errors rather than truncations, and no 64-bit
input can make the decoder crash or invoke undefined behaviour — the last
of those is checked under both sanitizers.

There is one decoder. The VM and the disassembler share it, so they
cannot drift apart.

## Documentation

| Document | Contents |
|---|---|
| [`docs/isa.md`](docs/isa.md) | Frozen ISA: registers, encoding, all 36 instructions, flags, calling convention |
| [`docs/architecture.md`](docs/architecture.md) | Layering, module responsibilities, invariants, error model |
| [`docs/testing.md`](docs/testing.md) | Test layers and the properties currently proven |
| [`docs/development-log.md`](docs/development-log.md) | Bugs, causes, fixes, lessons |
| [`docs/adr/`](docs/adr/) | Architecture decision records |

## Limitations

Honest list, kept current: there is no assembler, linker, VM or debugger
yet — V0–V2 are complete and the rest is specified but unimplemented.
Immediates are limited to 48 bits. There is no immediate-operand
arithmetic (see [ADR-006](docs/adr/ADR-006-no-immediate-arithmetic.md)).
No performance numbers are published because none have been measured.

## License

MIT.

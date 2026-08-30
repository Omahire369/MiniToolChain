# Architecture

## Layering

Dependencies point strictly downwards. A lower layer never includes a
header from a higher one, and nothing below `cli/` knows that a command
line exists.

```
CLI  (src/cli)
 |
 v
Toolchain orchestration
 |
 v
Assembler / Linker / Optimizer / VM / Debugger
 |
 v
Parser / IR / Object / Executable
 |
 v
Lexer / ISA / binary utilities
 |
 v
Common / Diagnostics
```

## Modules as of v0.2.0

| Module | Header | Responsibility |
|---|---|---|
| `common` | `minitool/common/types.hpp` | Fixed-width integer aliases used everywhere. |
| `common` | `minitool/common/byte_order.hpp` | The only sanctioned way to move integers in and out of byte buffers. Little-endian, host-independent, plus range checks. |
| `common` | `minitool/common/source_manager.hpp` | Owns source text; `SourceLocation` stays a 16-byte value. |
| `common` | `minitool/common/print.hpp` | `std::format`-based console output (`<print>` is not available on every supported standard library). |
| `diagnostics` | `minitool/diagnostics/*.hpp` | Severities, stable error codes, caret rendering. Holds no global state and never writes to a stream itself. |
| `isa` | `minitool/isa/registers.hpp` | Register numbering, naming, parsing. |
| `isa` | `minitool/isa/opcode.hpp` | The frozen opcode table and per-opcode metadata. |
| `isa` | `minitool/isa/instruction.hpp` | Decoded instruction value type. |
| `isa` | `minitool/isa/encoding.hpp` | The single canonical encoder/decoder, shared by assembler, disassembler and VM. |

## Invariants enforced today

1. `decode(encode(i)) == i` for every canonical instruction.
2. `encode(decode(w)) == w` for every word the decoder accepts.
3. No 64-bit input can make the decoder crash or invoke undefined
   behaviour; every rejection is a typed error.
4. Reserved encoding fields are validated, never ignored.
5. Out-of-range immediates are errors, never truncated.
6. No global mutable state anywhere in the libraries.
7. Errors are returned as `std::expected`; nothing in the core throws for
   ordinary invalid input.

## Error handling policy

* Invalid **input** (bad source, malformed binary, out-of-range value) is
  a value: `std::expected<T, E>` or a `Diagnostic`.
* Broken **internal invariants** are assertions; they indicate a bug in
  the toolchain, not in the user's program.
* Nothing is ever silently ignored, truncated, or clamped.

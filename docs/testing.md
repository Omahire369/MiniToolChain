# Testing strategy

Run everything:

```bash
cmake --preset debug && cmake --build build/debug && ctest --preset debug
cmake --preset asan  && cmake --build build/asan  && ctest --preset asan
cmake --preset ubsan && cmake --build build/ubsan && ctest --preset ubsan
```

## Layers

| Layer | Location | What it proves |
|---|---|---|
| Unit | `tests/unit/` | Each component behaves as specified, including its failure modes. |
| Property | `tests/property/` | Algebraic laws hold over large random input sets. |
| Golden | `tests/golden/` | Binary output is byte-for-byte stable across builds. *(arrives with the object format, V7)* |
| Integration | `tests/integration/` | assemble → link → run pipelines. *(V9)* |
| Fuzz | `tests/fuzz/` | Malformed input never crashes or hangs. *(V16)* |

## Rules

* Every feature ships with a **negative** test, not only a happy path.
  Roughly half of the current cases assert on rejection behaviour.
* Property tests use a **fixed seed**, printed in the failure message, so
  every failure is reproducible.
* A test is never modified to accommodate an implementation bug. If a
  test is wrong, that is a specification change and needs an ADR.
* Golden binaries are never regenerated to make a diff go away.

## Current properties

| Property | Test |
|---|---|
| `decode(encode(i)) == i` | `EncodingProperties.DecodeUndoesEncode` (200k cases) |
| `encode(decode(w)) == w` | `EncodingProperties.EncodeUndoesDecodeForAcceptedWords` (200k cases) |
| Decoder is total and safe | `EncodingProperties.DecoderNeverMisbehavesOnArbitraryInput` (500k random words, run under ASan and UBSan) |
| Only defined opcodes decode | `EncodingProperties.OnlyDefinedOpcodesDecode` (exhaustive over all 256 opcode bytes) |
| Buffer codec matches word codec | `EncodingProperties.ByteBufferRoundTripMatchesWordRoundTrip` |
| Branch arithmetic is invertible | `EncodingProperties.BranchArithmeticIsInvertible` (100k cases) |

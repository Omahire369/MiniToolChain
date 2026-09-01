# Development log

Format for every entry: problem, cause, detection, fix, regression test,
lesson.

---

## 2026-09-01 — The playground reported link errors twice

**Problem.** A program with no `_start` produced a report whose
`diagnostics` said `error: entry point '_start' is not defined in any
object` and whose `error` field said exactly the same sentence. The UI
renders those as two separate blocks under two different headings, so
the page showed the same failure twice.

**Cause.** The linker reports through the `DiagnosticEngine` *and*
returns the reason in its `std::expected` error. The session copied both
into the report without noticing they were the same text. The assembler
does not behave this way — its returned error is a stage summary
("semantic analysis failed") that complements the caret diagnostic
rather than repeating it — so the pattern looked correct where it was
written and only duplicated one stage later.

**Detection.** Exercising the four failure stages against the running
server by hand, before writing the UI. Not by a test: every assertion
worth writing at that point was about *whether* the error appeared, and
it did — twice.

**Fix.** `setError` in `session.cpp` skips the headline error when the
rendered diagnostics already contain that text.

**Regression test.**
`Playground.ReportsALinkErrorWithoutRepeatingItself` asserts the
diagnostics carry the message and that `error` is empty.

**Lesson.** Two components each reporting a failure correctly can still
compose into a wrong result. "Did the error appear?" is the assertion
that passes either way; "how many times?" is the one that catches this.
Aggregating layers need tests about the shape of what they aggregate,
not just its presence.

---

## 2026-09-01 — MSVC failed to load its own front end

**Problem.** A full build under `pwsh` died with
`cl : Command line error D8027 : cannot execute
'...\HostX64d\c1xx.dll'`. The identical command had just succeeded
under `powershell`, and succeeded again on retry.

**Cause.** Environmental, not in the project: the compiler could not load
`c1xx.dll`, which on this machine is a repository living in a
OneDrive-synced directory with antivirus scanning it. Nothing in the
toolchain or the build script was involved.

**Detection.** A clean rebuild, immediately after another clean rebuild.

**Fix.** None applied. It is recorded here so the next person who sees it
does not go looking for a build-script bug.

**Regression test.** None possible.

**Lesson.** Worth stating plainly because the instinct is to debug it: a
failure that does not reproduce on an unchanged input is evidence about
the environment, not about the code. Retry once before investigating.

---

## 2026-08-30 — `-Wsign-conversion` fired only in the UBSan build

**Problem.** `byteorder::store<u16>` compiled cleanly in the Debug preset
but failed with `-Werror=sign-conversion` under the UBSan preset.

**Cause.** `value >> (8U * i)` promotes a `u16` operand to `int`, and
`int & unsigned` then triggers the sign-conversion diagnostic. The Debug
build folded the expression before the warning could be emitted;
sanitizer instrumentation suppressed that folding, so the same code
warned in one configuration and not the other.

**Detection.** The UBSan build in the standard three-configuration run.
A single-configuration CI would have shipped it.

**Fix.** Widen to `u64` once at the top of the function and do all
shifting in that type, so no integer promotion is involved.

**Regression test.** `ByteOrder.RoundTripsAllWidths` exercises the `u16`
path; it now compiles in every configuration, which is the actual
assertion.

**Lesson.** "Builds clean" means clean in *every* configuration. Warning
behaviour is not configuration-independent, so sanitizer builds must run
before a version is called done, not after.

---

## 2026-09-01 — Half the toolchain had never been compiled

**Problem.** The V3–V15 sources existed and looked plausible, but the
project had never been built. `sema.cpp` and `assembler.cpp` referenced
an AST that did not exist (`RegisterOperand`, `dir.name`, `label.loc`,
`diag_.reportError`), so `minitool_core` could not link. Several other
files compiled but were semantically wrong.

**Cause.** No build had been run — there was no CMake and no GCC on the
machine, so nothing forced the question. Code that is never compiled
accumulates plausible-looking errors at a rate that is easy to
underestimate.

**Detection.** The first attempt to build anything at all.

**Fix.** Stood up a build that works with the toolchain that *is*
installed (`tools/build.ps1`, MSVC via `vswhere`), then rebuilt the front
end, back end and runtime against it, compiling after every module rather
than at the end.

**Regression test.** The whole suite, and specifically the requirement
that `pwsh tools/build.ps1` builds and runs everything on a machine with
no CMake and no network ([ADR-010](adr/ADR-010-test-framework.md)).

**Lesson.** A build you cannot run is not a build. Getting *something*
compiling on the available toolchain is the first task, not a later one —
every hour of writing code without a compiler is an hour of writing
untested guesses.

---

## 2026-09-01 — The relocation model could not express a branch

**Problem.** The draft emitted `PCREL32` for `JMP`/`CALL`, patching four
bytes at the instruction's offset, and computed the displacement as
`symbol + addend - place`. Both halves were wrong.

**Cause.** Two independent mistakes that happen to cancel in the easy
case:

1. The displacement lives in the *48-bit immediate field* of a 64-bit
   instruction word. Writing 32 bits over the low half works for a small
   positive displacement and leaves bits 32–47 as zero for a negative
   one — after which the decoder's sign extension yields a completely
   different address.
2. `isa::branchTarget` defines the branch base as the instruction
   *after* the branch. A displacement computed against the branch itself
   is off by exactly 8.

A forward branch to a nearby label would have appeared to work.

**Detection.** Reading the frozen `docs/isa.md` §3 against the draft
relocation code before running it. The encoding table says the immediate
is 48 bits; `PCREL32` says 32.

**Fix.** [ADR-007](adr/ADR-007-relocation-model.md): added `IMM48` and
`PCREL48`, which patch the instruction's immediate field by decoding the
word, replacing the immediate and re-encoding it. `PCREL48` is defined as
`isa::branchDisplacement(P, S + A)` and *calls that function*, so the
branch rule exists in one place.

**Regression test.** `Relocation.PcRel48MatchesTheIsaBranchRule` asserts
`branchTarget(P, applied) == S + A` — the linker and the VM agreeing, not
just the linker being self-consistent — and
`Relocation.PcRel48HandlesBackwardBranches` covers the negative case that
the original would have silently mangled.

**Lesson.** When a value shares a word with other fields, "patch N bytes
at an offset" is the wrong abstraction. Go through the encoder: it
already knows the layout, and it already range-checks.

---

## 2026-09-01 — An optimizer that could not have been correct

**Problem.** The draft optimizer took a `std::span<const isa::Instruction>`
and returned a shorter vector. Every pass that removed an instruction
silently invalidated every PC-relative displacement after it, and every
symbol offset in the section.

**Cause.** The representation had already thrown away what the fix-up
would need. This is not a bug that can be repaired by fixing a pass: with
displacements baked in and no record of which instruction each one points
at, there is nothing to recompute from.

**Detection.** Reading the pass list against the encoding. The
`eliminateUnreachable` pass even reconstructed branch targets by assuming
`addr + 8 + imm`, which is the shape of the right idea without the
information to act on it.

**Fix.** [ADR-008](adr/ADR-008-optimizer-ir.md): an addressless IR where
branches name labels, offsets are assigned once by the assembler *after*
all transformations, and a label is never deleted.

**Regression test.**
`OptimizerProperties.OptimizationPreservesObservableBehaviour` runs 120
generated programs at both levels and compares output, exit code, all
sixteen registers and `SP`. `Optimizer.NeverFoldsAcrossALabel` and
`Optimizer.RemovesUnreachableCode` pin the two specific hazards.

**Lesson.** When a transformation is unsafe, look at the data structure
before looking at the algorithm. "Can this pass be written correctly on
this representation?" is a cheaper question than debugging it later.

---

## 2026-09-01 — `-0x10` lexed as zero

**Problem.** `MOVI R1, -0x10` assembled to `MOVI R1, 0` followed by a
parse error on a stray identifier `x10`.

**Cause.** The base-prefix check read the character after the cursor. For
`0x10` the cursor sits on `x` (the first digit was already consumed); for
`-0x10` it still sits on `0`, because the consumed character was the
sign. The check therefore saw `0`, kept base 10, parsed `0`, and stopped
at `x`.

**Detection.** Writing `Lexer.ParsesEveryIntegerBase` and deciding that
"every base" ought to include the negative forms.

**Fix.** Compute the prefix position from whether a sign was consumed,
rather than assuming a fixed offset.

**Regression test.** `Lexer.ParsesNegativeNonDecimalLiterals`, which
covers `-0x10` and `-0b101` specifically.

**Lesson.** A cursor whose position depends on which branch got you there
needs the branch to be a parameter, not an assumption. The bug was
invisible in the positive case, which is the case everyone tests first.

---

## 2026-09-01 — Flag liveness so conservative that folding never fired

**Problem.** Constant folding was implemented, tested, and did nothing.
`MOVI R1, 40 / MOVI R2, 2 / ADD R1, R2 / HALT` folded zero instructions.

**Cause.** Rewriting `ADD` into `MOVI` drops a FLAGS write, so it is only
legal when the flags are dead. The first rule was "dead if a later
instruction *in this block* overwrites FLAGS first, otherwise assume
live". Sound — and nearly always "live", because a block usually ends
before it rewrites its own flags. Here the block ends at `HALT`.

**Detection.** `Optimizer.FoldsConstantArithmetic` failed. A weaker test
that only checked "the program still works" would have passed
comfortably.

**Fix.** Build the section's control-flow graph and solve backward FLAGS
liveness to a fixed point, treating an unknown successor (a `CALL`, a
literal displacement, a branch to another file's label) as live.

**Regression test.** `Optimizer.FoldsWhenTheFlagsAreDeadAcrossBlocks`
requires folding *through* a block boundary, and
`Optimizer.DoesNotFoldWhenTheFlagsAreLive` requires not folding when a
`JE` depends on the result.

**Lesson.** "Conservative" is not automatically "fine". A conservative
answer that is almost always wrong turns a feature into decoration — and
only a test that asserts the optimization *happened* will notice.

---

## 2026-09-01 — Four header bytes could be flipped with no effect

**Problem.** `Object.SurvivesEveryOneByteCorruption` flips each byte of a
valid `.mobj` in turn and requires that at most one corrupted file is
still accepted. Four were.

**Cause.** The header's reserved `flags` word was read and never checked.
Any value passed, so those four bytes carried no meaning and no
protection — and they sit before the region the CRC-32 covers, so nothing
else caught them either.

**Detection.** The exhaustive single-byte corruption test. Hand-written
corruption tests would have targeted the fields that *do* something.

**Fix.** Reject a non-zero `flags`. That also keeps the field usable: a
future flag can now mean something, because no existing file sets it.

**Regression test.** The same test, which now sees exactly zero accepted
corruptions.

**Lesson.** Exhaustive beats representative for validation tests. "Flip
every byte" is a few lines and finds the fields you forgot; a list of
hand-picked cases finds only the ones you remembered.

---

## 2026-09-01 — A peephole that read a field it had just overwritten

**Problem.** The `PUSH rX / POP rY -> MOV rY, rX` rewrite produced
`MOV rY, rY`.

**Cause.** The rewrite reused the `PUSH` instruction in place: it set
`dst = rY` and *then* read the pushed register from `dst`, which by then
was already `rY`.

**Detection.** `Optimizer.CollapsesPushPopPairs` checked that the result
moved into `R2`, and the value moved was wrong.

**Fix.** Read the pushed register into a local before overwriting the
field.

**Regression test.** The same test, extended to check the source register
of the resulting `MOV` rather than only its destination.

**Lesson.** In-place rewrites need their inputs captured first. The
general form of this bug — read-after-write on a structure being edited —
is worth looking for in every pass that mutates rather than rebuilds.

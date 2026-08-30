# Development log

Format for every entry: problem, cause, detection, fix, regression test,
lesson.

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

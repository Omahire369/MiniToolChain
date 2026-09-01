// SPDX-License-Identifier: MIT
#pragma once

/// The playground's single page, embedded in the binary.
///
/// It is embedded rather than read from disk so that `minitool serve` works
/// from any working directory and from a copied binary, and so the server has
/// no filesystem surface at all: it can only ever serve this one string. There
/// are no external stylesheets, fonts or scripts, because the toolchain is
/// meant to work with no network (ADR-010).

#include <string_view>

namespace minitool::playground {

inline constexpr std::string_view kIndexHtml = R"PAGE(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>MiniToolchain Playground</title>
<style>
  :root {
    --bg: #14161a;
    --panel: #1a1d23;
    --panel-2: #21252d;
    --border: #2c313a;
    --text: #d7dce3;
    --dim: #8b95a5;
    --accent: #7aa2f7;
    --ok: #9ece6a;
    --err: #f7768e;
    --warn: #e0af68;
    --mono: ui-monospace, "Cascadia Mono", "SF Mono", Menlo, Consolas, monospace;
  }
  * { box-sizing: border-box; }
  html, body { height: 100%; margin: 0; }
  body {
    background: var(--bg);
    color: var(--text);
    font: 14px/1.5 system-ui, -apple-system, Segoe UI, sans-serif;
    display: flex;
    flex-direction: column;
  }

  header {
    display: flex;
    align-items: center;
    gap: 12px;
    padding: 10px 14px;
    background: var(--panel);
    border-bottom: 1px solid var(--border);
    flex-wrap: wrap;
  }
  header h1 {
    font-size: 14px;
    font-weight: 600;
    margin: 0 8px 0 0;
    letter-spacing: .2px;
  }
  header h1 span { color: var(--accent); }
  .spacer { flex: 1; }

  select, button {
    font: inherit;
    color: var(--text);
    background: var(--panel-2);
    border: 1px solid var(--border);
    border-radius: 6px;
    padding: 6px 10px;
    cursor: pointer;
  }
  select:hover, button:hover { border-color: #3d4452; }
  button.run {
    background: var(--accent);
    border-color: var(--accent);
    color: #10131a;
    font-weight: 600;
    min-width: 104px;
  }
  button.run:disabled { opacity: .55; cursor: default; }
  kbd {
    font: 11px/1 var(--mono);
    color: var(--dim);
    border: 1px solid var(--border);
    border-bottom-width: 2px;
    border-radius: 4px;
    padding: 3px 5px;
  }

  main {
    flex: 1;
    display: grid;
    grid-template-columns: 1fr 1fr;
    min-height: 0;
  }
  @media (max-width: 900px) { main { grid-template-columns: 1fr; } }

  .pane { display: flex; flex-direction: column; min-width: 0; min-height: 0; }
  .pane + .pane { border-left: 1px solid var(--border); }
  .pane-head {
    display: flex;
    align-items: center;
    gap: 2px;
    padding: 0 8px;
    background: var(--panel);
    border-bottom: 1px solid var(--border);
    min-height: 36px;
  }
  .pane-head .label {
    font-size: 11px;
    text-transform: uppercase;
    letter-spacing: .8px;
    color: var(--dim);
    padding: 0 6px;
  }
  .tab {
    background: none;
    border: none;
    border-bottom: 2px solid transparent;
    border-radius: 0;
    color: var(--dim);
    padding: 8px 10px;
    font-size: 12px;
  }
  .tab.active { color: var(--text); border-bottom-color: var(--accent); }

  /* editor */
  .editor-wrap {
    flex: 1;
    display: flex;
    overflow: auto;
    background: var(--bg);
    min-height: 0;
  }
  .gutter {
    padding: 12px 8px 12px 12px;
    text-align: right;
    color: #4a5364;
    font: 13px/1.55 var(--mono);
    user-select: none;
    white-space: pre;
  }
  #source {
    flex: 1;
    resize: none;
    border: none;
    outline: none;
    background: transparent;
    color: var(--text);
    font: 13px/1.55 var(--mono);
    padding: 12px 12px 12px 4px;
    white-space: pre;
    overflow: hidden;
    min-height: 100%;
  }

  /* output */
  .view { flex: 1; overflow: auto; padding: 12px 14px; min-height: 0; }
  .view pre {
    margin: 0;
    font: 12.5px/1.55 var(--mono);
    white-space: pre-wrap;
    word-break: break-word;
  }
  .empty { color: var(--dim); font-style: italic; }
  .block-label {
    font-size: 11px;
    text-transform: uppercase;
    letter-spacing: .8px;
    color: var(--dim);
    margin: 0 0 6px;
  }
  .block + .block { margin-top: 18px; padding-top: 16px; border-top: 1px solid var(--border); }
  .diag { color: var(--err); }
  .stdout { color: var(--text); }
  .muted { color: var(--dim); }

  table.regs { border-collapse: collapse; font: 12.5px/1.5 var(--mono); }
  table.regs td { padding: 2px 14px 2px 0; }
  table.regs td.name { color: var(--accent); }
  table.regs td.dec { color: var(--dim); }

  #stdin-row { display: none; padding: 8px 14px; background: var(--panel); border-bottom: 1px solid var(--border); }
  #stdin-row.on { display: block; }
  #stdin {
    width: 100%;
    background: var(--panel-2);
    color: var(--text);
    border: 1px solid var(--border);
    border-radius: 6px;
    font: 12.5px/1.5 var(--mono);
    padding: 6px 8px;
    resize: vertical;
    min-height: 44px;
  }

  footer {
    display: flex;
    align-items: center;
    gap: 14px;
    padding: 7px 14px;
    background: var(--panel);
    border-top: 1px solid var(--border);
    font: 12px/1.4 var(--mono);
    color: var(--dim);
    flex-wrap: wrap;
  }
  .dot { width: 8px; height: 8px; border-radius: 50%; background: var(--dim); display: inline-block; }
  .dot.ok { background: var(--ok); }
  .dot.err { background: var(--err); }
  .dot.busy { background: var(--warn); }
</style>
</head>
<body>

<header>
  <h1>MiniToolchain <span>Playground</span></h1>
  <select id="examples" title="Load an example">
    <option value="">Load example...</option>
  </select>
  <select id="opt" title="Optimization level">
    <option value="0">-O0</option>
    <option value="1">-O1</option>
  </select>
  <button id="toggle-stdin" title="Provide input for the read syscall">stdin</button>
  <div class="spacer"></div>
  <kbd>Ctrl</kbd><kbd>Enter</kbd>
  <button id="run" class="run">Run</button>
</header>

<div id="stdin-row">
  <textarea id="stdin" placeholder="Input for the read syscall (fd 0)"></textarea>
</div>

<main>
  <section class="pane">
    <div class="pane-head"><span class="label">playground.asm</span></div>
    <div class="editor-wrap" id="editor-wrap">
      <div class="gutter" id="gutter">1</div>
      <textarea id="source" spellcheck="false" autocomplete="off"
                autocapitalize="off" autocorrect="off"></textarea>
    </div>
  </section>

  <section class="pane">
    <div class="pane-head">
      <button class="tab active" data-view="output">Output</button>
      <button class="tab" data-view="disasm">Disassembly</button>
      <button class="tab" data-view="regs">Registers</button>
    </div>
    <div class="view" id="view">
      <p class="empty">Press Run to assemble, link and execute.</p>
    </div>
  </section>
</main>

<footer>
  <span><span class="dot" id="dot"></span> <span id="status">ready</span></span>
  <span id="metrics"></span>
</footer>

<script>
"use strict";

const EXAMPLES = {
  "Hello, World!": `; hello.asm - MiniToolchain "Hello, World!" example
; Demonstrates: syscalls, string data, basic control flow

.section .data

message:
    .asciz "Hello, World!\\n"

.section .text

.global _start

_start:
    ; syscall write(fd=1, buf=message, len=14)
    MOVI R1, 1              ; fd = stdout
    LEA  R2, message        ; buffer address (relocated)
    MOVI R3, 14             ; length
    SYSCALL 1               ; write

    ; syscall exit(0)
    MOVI R1, 0              ; exit code
    SYSCALL 0               ; exit
`,

  "Factorial (loops)": `; Iterative factorial - result lands in R14
; Demonstrates: loops, comparison, conditional branching

.section .text

.global _start

_start:
    MOVI R1, 10         ; compute factorial(10)
    CALL factorial
    MOV  R14, R1        ; result in R14
    HALT

factorial:
    MOVI R2, 1          ; accumulator = 1

factorial_loop:
    MOVI R3, 1
    CMP  R1, R3         ; if n <= 1, done
    JLE  factorial_done
    MUL  R2, R1         ; accumulator *= n
    DEC  R1             ; n--
    JMP  factorial_loop

factorial_done:
    MOV  R1, R2         ; result = accumulator
    RET
`,

  "Arithmetic": `; Every ALU instruction, with the result left in a register.
.section .text
.global _start

_start:
    MOVI R1, 40
    MOVI R2, 2
    ADD  R1, R2         ; R1 = 42
    MOVI R3, 100
    SUB  R3, R1         ; R3 = 58
    MOVI R4, 6
    MOVI R5, 7
    MUL  R4, R5         ; R4 = 42
    MOVI R6, 84
    MOVI R7, 2
    DIV  R6, R7         ; R6 = 42
    MOVI R8, 100
    MOVI R9, 58
    MOD  R8, R9         ; R8 = 42
    INC  R9             ; R9 = 59
    DEC  R9             ; R9 = 58
    NEG  R9             ; R9 = -58
    HALT
`,

  "Optimizer (-O1 folds this)": `; Build this at -O0 and again at -O1 and compare
; the disassembly and the instruction count.
.section .text
.global _start

_start:
    MOVI R1, 2
    MOVI R2, 3
    ADD  R1, R2         ; constant-foldable
    MOV  R3, R3         ; identity
    PUSH R4
    POP  R4             ; push/pop pair
    MOVI R5, 1
    MOVI R5, 2          ; dead store
    JMP  done
    MOVI R6, 99         ; unreachable
done:
    HALT
`,

  "Runtime error (divide by zero)": `; The VM traps rather than crashing the host.
.section .text
.global _start

_start:
    MOVI R1, 42
    MOVI R2, 0
    DIV  R1, R2         ; trap: division by zero
    HALT
`,

  "Assembly error (bad operand)": `; MOV needs two registers; this points a caret at the mistake.
.section .text
.global _start

_start:
    MOVI R1, 5
    MOV  R1, 7
    HALT
`
};

const $ = (id) => document.getElementById(id);
const sourceEl = $("source");
const gutterEl = $("gutter");
const viewEl = $("view");
const statusEl = $("status");
const metricsEl = $("metrics");
const dotEl = $("dot");
const runBtn = $("run");

let currentView = "output";
let lastReport = null;

/* ---------------------------------------------------------------- editor -- */

function syncGutter() {
  const lines = sourceEl.value.split("\n").length;
  let text = "";
  for (let i = 1; i <= lines; i++) text += i + "\n";
  gutterEl.textContent = text;
  sourceEl.style.height = "auto";
  sourceEl.style.height = sourceEl.scrollHeight + "px";
}

sourceEl.addEventListener("input", syncGutter);

// Tab inserts four spaces instead of leaving the textarea.
sourceEl.addEventListener("keydown", (e) => {
  if (e.key === "Tab") {
    e.preventDefault();
    const start = sourceEl.selectionStart;
    const end = sourceEl.selectionEnd;
    sourceEl.value = sourceEl.value.slice(0, start) + "    " + sourceEl.value.slice(end);
    sourceEl.selectionStart = sourceEl.selectionEnd = start + 4;
    syncGutter();
  }
});

document.addEventListener("keydown", (e) => {
  if ((e.ctrlKey || e.metaKey) && e.key === "Enter") {
    e.preventDefault();
    run();
  }
});

/* ----------------------------------------------------------------- tabs -- */

for (const tab of document.querySelectorAll(".tab")) {
  tab.addEventListener("click", () => {
    for (const t of document.querySelectorAll(".tab")) t.classList.remove("active");
    tab.classList.add("active");
    currentView = tab.dataset.view;
    render();
  });
}

$("toggle-stdin").addEventListener("click", () => {
  $("stdin-row").classList.toggle("on");
});

/* -------------------------------------------------------------- examples -- */

const exSelect = $("examples");
for (const name of Object.keys(EXAMPLES)) {
  const option = document.createElement("option");
  option.value = name;
  option.textContent = name;
  exSelect.appendChild(option);
}
exSelect.addEventListener("change", () => {
  const chosen = EXAMPLES[exSelect.value];
  if (chosen === undefined) return;
  sourceEl.value = chosen;
  exSelect.value = "";
  syncGutter();
  run();
});

/* ---------------------------------------------------------------- render -- */

function escapeHtml(text) {
  return text.replace(/[&<>]/g, (c) => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;" }[c]));
}

function block(label, body, cls) {
  return `<div class="block"><p class="block-label">${label}</p>` +
         `<pre class="${cls || ""}">${escapeHtml(body)}</pre></div>`;
}

function hex64(value) {
  return "0x" + BigInt(value).toString(16).padStart(16, "0").toUpperCase();
}

function signed64(value) {
  let n = BigInt(value);
  if (n >= (1n << 63n)) n -= (1n << 64n);
  return n.toString();
}

function renderRegisters(r) {
  let html = '<div class="block"><table class="regs">';
  for (let i = 0; i < r.registers.length; i++) {
    html += `<tr><td class="name">R${i}</td><td>${hex64(r.registers[i])}</td>` +
            `<td class="dec">${signed64(r.registers[i])}</td></tr>`;
  }
  html += `<tr><td class="name">PC</td><td>${hex64(r.pc)}</td><td></td></tr>`;
  html += `<tr><td class="name">SP</td><td>${hex64(r.sp)}</td><td></td></tr>`;
  html += `<tr><td class="name">FLAGS</td><td>${hex64(r.flags)}</td><td></td></tr>`;
  html += "</table></div>";
  return html;
}

function render() {
  const r = lastReport;
  if (!r) {
    viewEl.innerHTML = '<p class="empty">Press Run to assemble, link and execute.</p>';
    return;
  }

  if (currentView === "disasm") {
    viewEl.innerHTML = r.disassembly
      ? block("linked image", r.disassembly)
      : '<p class="empty">No disassembly: the program did not get as far as linking.</p>';
    return;
  }

  if (currentView === "regs") {
    viewEl.innerHTML = r.ran
      ? renderRegisters(r)
      : '<p class="empty">No machine state: the program did not run.</p>';
    return;
  }

  let html = "";
  if (r.diagnostics) html += block("diagnostics", r.diagnostics, "diag");
  if (r.error) html += block(r.stage + " error", r.error, "diag");
  if (r.output) {
    html += block("program output", r.output + (r.output_truncated ? "\n[output truncated]" : ""),
                  "stdout");
  }
  if (r.ok && !r.output) html += block("program output", "(no output)", "muted");
  if (r.stats && r.stats.total > 0) {
    html += block("optimizer", r.stats.summary, "muted");
  }
  viewEl.innerHTML = html || '<p class="empty">Nothing to show.</p>';
}

/* ------------------------------------------------------------------- run -- */

function setStatus(text, kind) {
  statusEl.textContent = text;
  dotEl.className = "dot" + (kind ? " " + kind : "");
}

async function run() {
  runBtn.disabled = true;
  setStatus("running...", "busy");
  metricsEl.textContent = "";

  const params = new URLSearchParams({
    opt: $("opt").value,
    stdin: $("stdin").value
  });

  try {
    const response = await fetch("/api/run?" + params.toString(), {
      method: "POST",
      headers: { "Content-Type": "text/plain; charset=utf-8" },
      body: sourceEl.value
    });
    if (!response.ok) throw new Error("server returned " + response.status);
    lastReport = await response.json();
  } catch (err) {
    lastReport = {
      ok: false, stage: "server", error: String(err && err.message ? err.message : err),
      diagnostics: "", output: "", disassembly: "", ran: false
    };
  }

  const r = lastReport;
  if (r.ok) {
    setStatus("finished - exit " + r.exit_code, "ok");
  } else {
    setStatus(r.stage + " failed", "err");
  }
  metricsEl.textContent = r.ran
    ? `${r.instructions} instructions executed`
    : "";

  runBtn.disabled = false;
  render();
}

runBtn.addEventListener("click", run);

/* ----------------------------------------------------------------- start -- */

sourceEl.value = EXAMPLES["Hello, World!"];
syncGutter();
run();
</script>
</body>
</html>
)PAGE";

}  // namespace minitool::playground

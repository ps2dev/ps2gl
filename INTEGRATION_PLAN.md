# PS2 OpenGL Toolchain — Current State & Plan

_Last reviewed: 2026-05-11_

## 0. TL;DR — pick this up here next session

**Where we are.** The open pipeline (`openvcl + masp + dvp-as`) builds all 13
ps2gl renderers and produces ELFs that boot in PCSX2. Triangle-based samples
render correctly. **`GL_QUADS`-based shaders render blank** because openvcl
emits wrong ADC-bit values — bug is localized, has a workaround, and has a
minimal repro committed.

**Headline open items, in priority order:**

1. 🔴 **Quad-renderer bug** in openvcl (`general_quad`, `general_pv_diff_quad`,
   `general_nospec_quad`). Workaround: build ps2gl with
   `-DPS2GL_USE_SCE_VSM=ON`. Minimal repro: `openvcl/test/repro/quad_adc_bug.vcl`.
   See §2.1.
2. 🟡 **Dual-pipe scheduler** in openvcl. Headline performance feature.
   Currently openvcl produces 0.34-0.73× Sony's instruction count per
   renderer. Multi-week. See §2.2.
3. 🟡 **masp polish** — most of §2.3 closed 2026-05-11: FIXMEs in
   `src/macro.c` rewritten as NOTEs, stale `build_ps2/` removed.
   Remaining: README expand, and the dormant `change_base` trailing-`'`
   bug discovered during test densification. See §2.3.
4. 🟡 **More unit tests** for masp + openvcl, especially per-module coverage.
   masp side: sb / hash / number-prefix landed 2026-05-11 (3 new test
   binaries, 57 cases). openvcl side: 17 → 56 cases — tokenizer suite
   (`openvcl@edc5b76`) + Parser operand-family / error-recovery suite
   (`openvcl@acc1ba6`) + RA uninit-read propagation fix and its
   regression test (`openvcl@66b4486`) all landed 2026-05-11. See §2.4.

**Workarounds & infrastructure landed this session:**
- `-DPS2GL_USE_SCE_VSM=ON` — bypasses openvcl, assembles Sony's reference VSMs
  directly. Diagnostic + temporary workaround. (`ps2gl@cmake` commit `316cd91`)
- `vsm_diff.py` semantic diff harness + per-renderer CTest entries. All 12
  WILL_FAIL today; XPASS-flips as renderers converge. (`ps2gl@cmake` commit
  `9138e1f`)
- openvcl unit + integration test framework. 56 tests (17 originally,
  +25 from the Tokenizer suite, +13 from the Parser family/error suite,
  +1 regression for the RA uninit-read propagation fix), 0 failures.
  (`openvcl@ps2gl` commits `7f1db90`, `de7f1f8`, `bec6b7f`, `edc5b76`,
  `acc1ba6`, `66b4486`)

---

## 1. Big picture

We are eliminating the legacy / proprietary tools from the ps2gl shader
pipeline so any homebrew developer can build ps2gl from source with **only
open-source** tooling.

```
        legacy pipeline                                  target pipeline
   ┌──────────────────────────┐                  ┌──────────────────────────┐
   │  .vcl  ─►  gasp  ─►  vcl │                  │  .vcl  ─►  masp  ─►openvcl│
   │           (GNU)   (Sony) │      ──►         │          (ours)   (ours) │
   │   ─►  dvp-as  ─►  .vo    │                  │   ─►  dvp-as  ─►  .vo    │
   └──────────────────────────┘                  └──────────────────────────┘
                                       │
                                       └─ dvp-as stays (PS2DEV)
```

Three repos in this workspace, all symlinks into `~/Projects/<name>`:

| Repo      | Role                                                      | Replaces |
| --------- | --------------------------------------------------------- | -------- |
| `masp`    | Assembler preprocessor (macros, conditionals, directives) | `gasp`   |
| `openvcl` | VCL → VSM transpiler (register allocation, pipelining)    | `vcl`    |
| `ps2gl`   | OpenGL-style API for PS2; producer of `.vcl` shaders      | —        |

---

## 2. Open work

### 2.1 Quad-renderer rendering bug 🔴

**Symptom.** Any ps2gl example that draws with `GL_QUADS` renders blank.
`box` shows only the clear color; `nehe_lesson04`/`05` show their triangles
but not their quads. Triangle-only samples (`lesson02`/`03`) render
correctly.

**Localized cause.** openvcl writes the wrong ADC bit on vertices 3 and 4 of
each quad. Captured via PCSX2 memory dump at `0x1100D420` and `0x1100D450`:

| Vertex | Sony (works) W field | openvcl (broken) W field |
|--------|----------------------|--------------------------|
| v1     | `00 80 FF FF` (skip) | `00 80 FF FF` (skip)     |
| v2     | `00 80 FF FF` (skip) | `00 80 FF FF` (skip)     |
| **v3** | **`FF 7F 00 00` (draw)** | **`00 80 FF FF` (skip)** ❌ |
| **v4** | **`FF 7F 00 00` (draw)** | **`00 80 FF FF` (skip)** ❌ |

All 4 vertices end up tagged "skip drawing" → GS draws nothing.

**Where the wrong value comes from.** The source's `ior new_adc_bit, vi01,
z_sign; iaddiu new_adc_bit, new_adc_bit, 0x7fff` chain ends up with
`new_adc_bit = 0x8000` for in-frustum vertices. The z_sign side is silenced
(z_sign_mask in VI05 is loaded as 0 by `ilw.w`), so the bug is purely on the
`vi01` side -- `fcand` is returning non-zero clip flags for vertices that
should pass clipping. Either openvcl's `clipw.xyz` sequence is producing
non-zero CLIP entries for in-frustum vertices, or `fcand` is reading a stale
entry from before the current iteration.

**Workaround.** `cmake -DPS2GL_USE_SCE_VSM=ON` — bypasses openvcl,
assembles Sony's reference VSMs directly. All examples render correctly
with this on.

**Status.** Bisected to baseline (`e407703`); **not** a regression from any
of the recent openvcl work (`bc41a56` / `303c528` / `41dff12` / `5c0227b` /
`a2a7d9d`). Filed in `openvcl/TODO` (commit `c6e82f5`). Minimal repro at
`openvcl/test/repro/quad_adc_bug.vcl` (commit `07a3196`).

**To make progress next:** single-step the minimal repro in PCSX2's
debugger (or run a hand-built "known-correct" reference VSM through PCSX2
side-by-side and compare CLIP register state cycle-by-cycle). Use the
repro to bisect which clipw / scheduling decision is wrong; tweak,
rebuild, re-emit (sub-second turnaround).

### 2.2 Dual-pipe instruction scheduler 🟡

VU1 issues two instructions per cycle: one upper-pipe (FMAC) + one
lower-pipe (LSU / integer / branch). Sony's vcl reorders independent
instructions to fill both pipes. openvcl currently emits NOP on the free
pipe most of the time.

**Concrete evidence:**
```
# Sony   vu1/sce_general_vcl.vsm  — one source word, both pipes filled
addi.xy  VF05, VF00, I            loi  0x45000000

# openvcl  build-test/vu1/general_vcl.vsm  — two source words, lots of NOPs
nop                                loi  0x44fff000
addi.xy  VF05, VF00, i             nop
```

Sony's `general.vsm` has ~22 instructions per main loop; openvcl's ~39 —
**roughly a 2× VU1 throughput regression**. Per-renderer ratios captured
2026-05-11 (instr count: openvcl / Sony):

| Renderer              | Ratio | Renderer             | Ratio |
| --------------------- | ----- | -------------------- | ----- |
| fast_nolights         | 0.72  | general_pv_diff      | 0.37  |
| fast                  | 0.73  | general_quad         | 0.34  |
| general_nospec_quad   | 0.38  | general_tri          | 0.38  |
| general_nospec_tri    | 0.46  | general              | 0.38  |
| general_nospec        | 0.45  | indexed              | 0.34  |
| general_pv_diff_quad  | 0.39  |                      |       |
| general_pv_diff_tri   | 0.38  |                      |       |

**Status of the rescheduler in code:**
- `src/Token.h` already has a `PREORDERED` flag (= "do not reschedule") —
  the hook exists, the pass doesn't.
- `Dependency.{cpp,h,inl}` looks like scaffolding for the eventual scheduler.
- README explicitly lists rescheduling as future work.

**Sub-steps when this work starts:**
1. Read all `vu1/sce_*.vsm` to characterise Sony's pairing patterns
   (learning pass before coding).
2. Audit `Dependency.{cpp,h,inl}` — what dep info is already captured?
3. Add a list-scheduler between code-gen and emission: topological order
   by data deps, greedy pair upper+lower per cycle, respect VU1 hazard
   rules (RAW latency on FMAC X/Y/Z/W, FDIV/EFU long-latency pipes,
   branch-delay slot).
4. Respect the existing `PREORDERED` flag on tokens that must stay put
   (branches, XGKICK, FCSET / FCAND control-flow ops).
5. Re-run `vsm_diff.py`; iterate until openvcl-vs-Sony ratio is within
   ~10 % on all 13 renderers.

**Risks worth flagging up front:** branch-delay slot, XGKICK timing,
FDIV/EFU long pipes, and `fcand` register liveness are where Sony's tool
will be subtly cleverer.

### 2.3 masp polish 🟡

| Item                                       | Effort | Notes |
| ------------------------------------------ | ------ | ----- |
| ~~Address 2× `FIXME` in `src/macro.c`~~    | ✅ done | Both were inherited gasp doc-FIXMEs, not bugs. Rewritten as NOTE explanations. masp@`b5de42a`. |
| ~~Decide fate of `build_ps2/`~~            | ✅ done | Stale local CMake dir from a past PS2 cross-compile experiment; deleted (was untracked). |
| README expand to brief user manual         | ½ day  | Acknowledged gap in README itself. |
| Fix `change_base` trailing-`'` bug         | ½ day  | Discovered 2026-05-11 while densifying tests: GASP-style `B'1010'` leaves the closing `'` in the output. Dormant — ps2gl uses `masp_syntax=1` path (`change_base2`). Tests pin the buggy behaviour in `masp@816b4d8`; fix is to bump idx past the closing `'` after `sb_strtol`. |

### 2.4 Test densification 🟡

The frameworks exist; coverage is shallow.

**masp** — was 2 CTest entries, now 5 (32 + 25 = 57 new cases as of
2026-05-11). Pattern links source files into a test binary so
per-module tests are easy. Targets:

| Module                   | What to cover | Status |
| ------------------------ | ------------- | ------ |
| `sb.c` (string buffer)   | append / reset / grow / overflow / null handling | ✅ 18 cases in `test_sb` (masp@`607196e`) |
| `hash.c` (hash table)    | insert / lookup / delete / collision / resize    | ✅ 14 cases in `test_hash` (masp@`607196e`) — exercises key-copy ownership and the move-to-front cache over 5000 keys |
| Number-prefix parser     | `0b` / `0q` / `0h` / `0d` / `0a` vs GASP `B'…`   | ✅ 25 cases in `test_number_prefix` (masp@`816b4d8`) covering `is_base`, `sb_strtol`, `change_base`, `change_base2` |
| `macro.c`                | macro defs, recursive expansion, comma-arg splitting | open |
| Directive prefix         | `-P/--prefixchar` default `\`, conflicts        | open |
| Mode switching           | `\masp` / `\gasp` toggles, nested ifmode         | open |
| Conditional assembly     | `\ifmode` / `\ifm` / `\endifm` truth tables      | open |
| Golden files             | Re-run every `ps2gl/vu1/*.vcl` through masp; compare to checked-in expected output | open |

**openvcl** — has 56 tests across `unit/` and `integration/` (was 17
before 2026-05-11). Hand-rolled harness in `test/include/test_harness.h`
(TEST_CASE / CHECK / REQUIRE / EXPECTED_FAIL, auto-registered via
static init). Subprocess runner in `test/include/openvcl_runner.h` for
end-to-end checks. Targets to expand:

| Area                                              | Effort | Status |
| ------------------------------------------------- | ------ | ------ |
| Tokenizer: comments, fields, bit-flags, labels    | ½ day  | ✅ 25 cases in `test_tokenizer.cpp` (`openvcl@edc5b76`) — case-insensitive mnemonic lookup and `.xyzw`→0 normalisation pinned with comments |
| Tokenizer: argument-list parsing for FMAC/LSU forms | ½ day  | partial — broadcast (`MULw`), post-inc (`(vi++)`) and `imm(vi)` addressing covered by Parser family tests (`openvcl@acc1ba6`); still open: pre-dec, `i`/`q`/`p`/`r` immediate operands, indirect `(vi)` zero-form |
| Parser: operand templates, error recovery         | ½ day  | ✅ 13 cases in `test_parser_families.cpp` (`openvcl@acc1ba6`) — one positive per VU family (FMAC, FDIV, LSU, IALU, BRU, RANDU, EFU) + negatives for unknown mnemonic, wrong arg count, out-of-range register, family mismatch |
| Expression: edge cases (some landed)              | started | partial |
| CodeGenerator golden files per mnemonic family    | 1 day  | open  |
| CommandLine: every flag in README                 | ½ day  | open  |
| Scheduler dependency-matrix suite                 | open until §2.2 lands | n/a |

### 2.5 ps2gl hygiene 🟢 (low priority)

- Legacy `Makefile` still present alongside CMake — decide keep-or-delete.
- `examples/tricked_out/billboard_renderer.vcl` exists but the build
  links the pre-built `_vcl.vsm`, skipping openvcl on this app-level
  shader. Wire it through the full pipeline once the quad bug is fixed
  (tricked_out uses GL_QUADS-like billboard rendering).
- Add CI that runs `ctest -L vsm-diff` so renderer regressions surface
  in PRs.
- Once openvcl reaches parity, drop the `TEMP: Sony reference VSMs`
  commit via `git rebase -i` (the `vu1/sce_*_vcl.vsm` files were
  committed as ground truth and are marked temporary in their commit
  message).

---

## 3. What landed this session (2026-05-11)

All pushed to `fjtrujy/openvcl@ps2gl` and `ps2dev/ps2gl@cmake`.

| Repo    | Commit     | Description |
| ------- | ---------- | ----------- |
| openvcl | `bc41a56`  | LOI IEEE-754 hex + expression-evaluated address offsets + SCE-matching VSM header |
| openvcl | `303c528`  | Loop-body live-range extension in the register allocator |
| openvcl | `f8b3ff2`  | examples Makefile parallel-build targets |
| openvcl | `41dff12`  | Defensive checks + stable token pointer in RA |
| openvcl | `7f1db90`  | Bootstrap unit-test framework (CMake + in-tree harness) |
| openvcl | `de7f1f8`  | Integration subprocess runner + 2 TODO tests pinned |
| openvcl | `5c0227b`  | Error::HasErrors propagation into exit code (fixes silent CLIP) |
| openvcl | `a2a7d9d`  | `.init_vf`/`.init_vi` register-range shorthand (`vfXX-vfYY`) |
| openvcl | `bec6b7f`  | LOI hex regression tests + Expression edge tests |
| openvcl | `c6e82f5`  | File the ps2gl quad-renderer bug in TODO |
| openvcl | `07a3196`  | `test/repro/quad_adc_bug.vcl` minimal repro |
| ps2gl   | `b74d303`  | TEMP: add Sony reference VSMs (12 files, ground truth) |
| ps2gl   | `a4c5e06`  | Docs + dead code: all 13 renderers build with openvcl+masp |
| ps2gl   | `9138e1f`  | Semantic VSM-diff CTest harness (`vsm_diff.py` + 12 entries) |
| ps2gl   | `316cd91`  | `PS2GL_USE_SCE_VSM` diagnostic build option (quad-bug workaround) |

**Closed openvcl TODO items:** 4 of 6 — LOI hex, line-based register
allocator, CLIP validation, `.init_vf` range. All with regression tests.

**Open openvcl TODO items:**
- Output-parameters only applied at "proper" branch exits (vague spec)
- GASP preparsing doesn't track filenames (low priority)
- *Plus the new quad-renderer bug* — filed, repro committed.
- ~~**Error propagation gap** for RegisterAllocator uninit-register
  reads~~ — ✅ fixed 2026-05-11 (`openvcl@66b4486`).  The seven RA
  paths (`float/integer/accumulator/Q/P/R/I`) now route through
  `Error::Display(Error(msg, token, *i))` so they participate in
  exit-code propagation, mirroring the `5c0227b` CLIP fix.  Guarded
  by `RegisterAllocator: uninit-read produces a non-zero exit` in
  `test_parser_families.cpp`.

---

## 4. Runtime validation via ps2gl samples

The ps2gl example apps are the **functional** regression suite. Each
exercises a different slice of the library + VU1 renderers.

### Sample matrix (built artifacts in `build-test/examples/`)

| Sample            | Renderer(s)              | Renders with openvcl? | Renders with `PS2GL_USE_SCE_VSM=ON`? |
| ----------------- | ------------------------ | --------------------- | ------------------------------------- |
| `nehe_lesson02`   | `general*`               | ✅                    | ✅                                    |
| `nehe_lesson03`   | `general_pv_diff*`       | ✅                    | ✅                                    |
| `nehe_lesson04`   | `general*` + `_quad`     | 🟡 triangle yes, quad no | ✅                                  |
| `nehe_lesson05`   | `general*` + `_quad`     | 🟡 triangle yes, quad no | ✅                                  |
| `box`             | `general*`, `_quad`      | ❌ all-quad cube blank | ✅                                    |
| `logo`            | `general*` + texturing   | (not yet tested)      | (not yet tested)                      |
| `performance`     | `fast*`, `general_quad/tri` | (not yet tested)   | (not yet tested)                      |
| `tricked_out`     | own VU1 + `general*`     | (not yet tested; built from pre-built `_vcl.vsm`) | n/a |

### Defining "done"

A sample passes when:
1. ELF builds with the all-open pipeline (`openvcl + masp + dvp-as`).
2. It boots in PCSX2 to the rendering loop (no early `SIF crash` / TLB miss).
3. Screenshot matches the golden within tolerance (image-diff with
   PSNR / SSIM threshold; bitwise compare fails on emulator jitter).
4. (After §2.2 lands) Frame time within ~10 % of legacy-pipeline ELF.

Steps (1)–(3) are functional parity; (4) is the perf milestone tied to the
scheduler.

### Suggested automation (not yet built)

```
for sample in build-test/examples/*.elf; do
  mcp__pcsx2.reset_vm
  mcp__pcsx2.boot_elf $sample
  # wait N frames OR watch for known framebuffer signature
  mcp__pcsx2.screenshot > validation/$(basename $sample)_openvcl.png
  mcp__pcsx2.shutdown_vm
done
# image-diff each PNG vs validation/<name>_golden.png
```

---

## 5. Fast iteration loop — `/ps2dev` + PCSX2

The standing dev loop is the `/ps2dev` skill (build / SDK / toolchain) glued
to `mcp__pcsx2.*` runtime tools. Use it whenever changes touch `vu1/`,
`openvcl/src`, `masp/src`, or anything that links into `libps2gl.a`.

| Step                | Tool                                                                                  |
| ------------------- | ------------------------------------------------------------------------------------- |
| Build               | `/ps2dev` skill                                                                       |
| Boot ELF            | `mcp__pcsx2.boot_elf` (or `ps2link_execee` for hardware)                              |
| Confirm rendering   | `mcp__pcsx2.screenshot` + image-diff against golden                                   |
| Skip past boot      | `mcp__pcsx2.save_state` / `load_state` once a sample reaches its render loop          |
| Inspect VU1 / GS    | `mcp__pcsx2.pause_vm` + `read_memory_range` (VU1 data at `0x1100C000`, 16 KB)         |
| Reset between runs  | `mcp__pcsx2.reset_vm` / `shutdown_vm`                                                 |

Useful PCSX2 address landmarks for ps2gl debugging:
- VU1 data RAM at `0x1100C000` (16 KB; first ~1.5 KB is constants/matrices,
  rest is output buffer + scratch).
- Sony's working quad outputs vertex positions at `0x1100D390`+ — useful
  reference for the quad-bug diff.

---

## 6. Quick reference — commands

```bash
# Build openvcl + install to toolchain
cd ~/Projects/openvcl
make openvcl
cp openvcl ~/toolchains/ps2/ps2dev/bin/openvcl

# Run openvcl on a single shader (smoke test)
./openvcl --gasp masp general.vcl > /tmp/out.vsm

# Run openvcl test suite
cmake --build test/build && ./test/build/openvcl_unit_tests

# Build ps2gl with the open pipeline
cd ~/Projects/ps2gl
cmake -B build-test -DBUILD_EXAMPLES=ON
cmake --build build-test

# Build ps2gl with the SCE-VSM bypass (workaround for the quad bug)
cmake -B build-sce -DPS2GL_USE_SCE_VSM=ON -DBUILD_EXAMPLES=ON
cmake --build build-sce

# Run the semantic VSM-diff suite (all WILL_FAIL today)
ctest --test-dir build-test -L vsm-diff

# Detailed VSM diff for one renderer
python3 cmake/vsm_diff.py vu1/sce_general_vcl.vsm build-test/vu1/general_vcl.vsm

# Boot a sample in PCSX2 from CLI
/Applications/PCSX2.app/Contents/MacOS/PCSX2 -fastboot -- \
    build-test/examples/nehe_lesson04.elf

# Iterate on the quad-bug minimal repro
cd ~/Projects/openvcl
./openvcl test/repro/quad_adc_bug.vcl -o /tmp/quad_repro.vsm
diff -u test/repro/quad_adc_bug.vsm.openvcl-output /tmp/quad_repro.vsm
```

---

## 7. Open questions to resolve

1. **Legacy `Makefile` in `ps2gl/`**: do any downstream consumers (PS2DEV
   ports, package recipes) still depend on it? If not, delete.
2. **vsm-diff target**: as openvcl converges with Sony, do we accept
   "semantically equivalent but allocator chose different regs", or chase
   byte-equivalent VSM?
3. **Golden screenshots for runtime validation**: do we have any from a
   legacy-pipeline build (CI artifacts, checked-in PNGs, etc.) we can use
   as the ground truth? If not, the first open-pipeline run becomes the
   de-facto golden after manual visual verification.
4. **Quad-bug priority vs scheduler**: fix the quad bug first
   (correctness — open pipeline works for all 13 renderers), or land the
   scheduler first (performance — open pipeline approaches Sony's perf)?
   The workaround (`PS2GL_USE_SCE_VSM=ON`) means we don't *have* to
   choose immediately, but at some point one path needs picking.

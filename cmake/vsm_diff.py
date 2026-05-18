#!/usr/bin/env python3
"""
Compare two VU1 .vsm files at the semantic level.

Used as a CTest target in ps2gl to verify that openvcl produces the same
*set* of operations as Sony's proprietary vcl for each VU1 renderer.
Differences in pipe-pairing, register-allocator choices, and whitespace
are intentionally ignored -- the goal is to surface real divergences
(missing instructions, wrong opcodes, missing labels) and to track how
close openvcl is getting to the reference as the dual-pipe scheduler
matures.

Usage:
  vsm_diff.py <reference.vsm> <openvcl.vsm>

Exit codes:
  0 = histograms and labels match.
  1 = real divergence (different opcode set, different label set).
  2 = file read / parse error.

The script is intentionally permissive about pipe placement: only the
opcode mix matters.  A separate "instruction-count delta" line is printed
to track scheduler progress over time.
"""

import re
import sys
from collections import Counter

# Lines that are not real instructions and should be skipped entirely.
_DIRECTIVE_PREFIXES = (".vu", ".align", ".global", ".name", ".end")

# Sony's reference output includes annotation comments like
#   ; === __LP__ ...
#   ; _LNOPT_w=[...] ...
# openvcl emits no such comments.  Both should be dropped from the
# semantic comparison.
_COMMENT_RE = re.compile(r"^\s*;.*")

# A label line: identifier ending with ':' optionally followed by a comment.
_LABEL_RE = re.compile(r"^\s*([A-Za-z_][A-Za-z0-9_]*)\s*:\s*(?:;.*)?$")

# An instruction line carries upper-pipe + lower-pipe ops separated by a
# wide whitespace gap.  Sony's reference left-pads the mnemonic into a
# ~14-char column and uses commas (no spaces) between operands, so within
# a single pipe there's never more than ~13 contiguous spaces.  The gap
# between pipes is always 20+ spaces in practice.  15 is the safest
# threshold that catches both styles (openvcl + reference) without
# splitting through an operand list.
_PIPE_SPLIT_RE = re.compile(r"\s{15,}")

# Flag suffixes the assembler writes after the mnemonic: NOP[E], NOP[I],
# NOP[D], NOP[T].  Captured separately from the bare mnemonic so we can
# verify control-flow tags independently of the surrounding ops.
_FLAG_RE = re.compile(r"^([a-z0-9.]+?)(\[[A-Za-z]+\])?$")


def _normalize_mnemonic(tok: str) -> str:
    """Lowercase the mnemonic and strip any [E]/[I]/[D]/[T] flag suffix.

    Keep dest fields (`.xyz`, `.w`) attached to the mnemonic so we can
    distinguish `addi.xy` from `addi.xyz` -- they're semantically
    different operations on different fields.
    """
    m = _FLAG_RE.match(tok.lower())
    return m.group(1) if m else tok.lower()


def _extract_flag(tok: str) -> str:
    """Return the flag suffix (e.g. "[E]") if present, else ""."""
    m = _FLAG_RE.match(tok.lower())
    return m.group(2) or "" if m else ""


def _is_mnemonic_token(tok: str) -> bool:
    """True iff `tok` looks like a VU1 mnemonic (as opposed to a register
    or immediate operand).

    Sony's reference output uses uppercase mnemonics with comma-joined
    operands and no space between them; openvcl's output uses lowercase
    mnemonics with space-separated operands.  A consistent classifier
    over both is to bucket each whitespace-separated token by what it
    looks like:

      mnemonic:  starts with a letter, is not a register name
      register:  V[FI]<digit>... or ACC[component] or single-letter I/Q/P/R
      immediate: starts with a digit (incl. 0x...)
      indirect:  contains '(' (e.g. 62(VI00))
      label-ref: trailing ':' -- handled before this function gets called
    """
    if not tok:
        return False
    # Strip any leading punctuation that the assembler emits with the
    # token (none expected for mnemonics, but harmless).
    if not tok[0].isalpha():
        return False
    # Indirect access embedded in a mnemonic isn't a thing -- those are
    # always operands like "62(VI00)" which start with a digit anyway,
    # but defend against weirdness.
    if "(" in tok:
        return False
    upper = tok.upper()
    # Register names: VF<digits> / VI<digits>, optionally with a field
    # suffix like VF15w.
    if len(tok) > 2 and upper[:2] in ("VF", "VI") and tok[2].isdigit():
        return False
    # The accumulator operand prefix (ACC, ACCxyz, ...).
    if upper.startswith("ACC"):
        return False
    # Single-letter pseudo-registers used as operands.
    if upper in ("I", "Q", "P", "R"):
        return False
    return True


def parse_vsm(path: str):
    """Return (opcode_histogram, flag_histogram, label_set, instr_count).

    instr_count is the total number of pipe slots filled with anything
    other than `nop` -- a rough "work-per-cycle" signal for the scheduler.
    """
    opcodes = Counter()
    flags = Counter()
    labels = set()
    instr_count = 0

    with open(path) as f:
        for raw in f:
            line = raw.rstrip("\n")

            if not line.strip():
                continue
            if _COMMENT_RE.match(line):
                continue
            stripped = line.strip()
            if stripped.startswith(_DIRECTIVE_PREFIXES):
                continue

            label_match = _LABEL_RE.match(line)
            if label_match:
                labels.add(label_match.group(1))
                continue

            # Real instruction line: split into upper-pipe / lower-pipe
            # halves on a 15+ whitespace gap.  Within each half the
            # mnemonic is the first token; the rest is operands and
            # would otherwise alias as bogus "opcodes" if we treated
            # every token equally.
            halves = _PIPE_SPLIT_RE.split(line.strip(), maxsplit=1)
            for half in halves:
                if not half:
                    continue
                tok = half.split()[0]
                if not _is_mnemonic_token(tok):
                    continue
                op = _normalize_mnemonic(tok)
                flag = _extract_flag(tok)
                opcodes[op] += 1
                if flag:
                    flags[flag] += 1
                if op != "nop":
                    instr_count += 1

    return opcodes, flags, labels, instr_count


def _diff_counters(a: Counter, b: Counter):
    """Return dict {key: (a, b)} for keys where a and b disagree."""
    diffs = {}
    for k in sorted(set(a) | set(b)):
        if a[k] != b[k]:
            diffs[k] = (a[k], b[k])
    return diffs


def main(argv) -> int:
    if len(argv) != 3:
        print(f"usage: {argv[0]} <reference.vsm> <openvcl.vsm>", file=sys.stderr)
        return 2

    ref_path, ovc_path = argv[1], argv[2]

    try:
        ref_ops, ref_flags, ref_labels, ref_count = parse_vsm(ref_path)
        ovc_ops, ovc_flags, ovc_labels, ovc_count = parse_vsm(ovc_path)
    except FileNotFoundError as e:
        print(f"missing file: {e.filename}", file=sys.stderr)
        return 2

    op_diff = _diff_counters(ref_ops, ovc_ops)
    flag_diff = _diff_counters(ref_flags, ovc_flags)
    only_in_ref = ref_labels - ovc_labels
    only_in_ovc = ovc_labels - ref_labels

    histogram_ok = not op_diff
    flags_ok = not flag_diff
    labels_ok = not (only_in_ref or only_in_ovc)

    # The scheduler-progress line: a single ratio that should approach 1.0
    # as openvcl learns to pair pipes.  Values are non-nop pipe slots.
    if ref_count == 0:
        ratio = float("inf") if ovc_count else 1.0
    else:
        ratio = ovc_count / ref_count

    print(f"=== vsm_diff: {ref_path}  vs  {ovc_path}")
    print(f"  non-nop slots:  reference={ref_count}  openvcl={ovc_count}  ratio={ratio:.2f}")
    print(f"  unique opcodes: reference={len(ref_ops)}  openvcl={len(ovc_ops)}")
    print(f"  labels:         reference={len(ref_labels)}  openvcl={len(ovc_labels)}")
    print(f"  histogram_ok={histogram_ok}  flags_ok={flags_ok}  labels_ok={labels_ok}")

    if op_diff:
        print("  opcode count mismatches  (op: reference -> openvcl):")
        for op, (ra, oa) in op_diff.items():
            print(f"    {op:<14}  {ra:>4} -> {oa}")
    if flag_diff:
        print("  flag count mismatches  ([X]: reference -> openvcl):")
        for fl, (ra, oa) in flag_diff.items():
            print(f"    {fl:<6}  {ra} -> {oa}")
    if only_in_ref:
        print("  labels only in reference:")
        for l in sorted(only_in_ref):
            print(f"    - {l}")
    if only_in_ovc:
        print("  labels only in openvcl:")
        for l in sorted(only_in_ovc):
            print(f"    + {l}")

    return 0 if (histogram_ok and labels_ok) else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))

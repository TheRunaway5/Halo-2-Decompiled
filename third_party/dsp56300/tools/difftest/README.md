# Differential testing against oracle implementations

Compares the JIT emulator's architectural state against an oracle running
the same program. Two oracles exist: Motorola's official `sim56300.exe`
(driven under wine) and **real MCPX silicon** (an original Xbox console
running xbtest; see the xbtest repo's `client/run_corpus_hw.py`).
The harness is engine-agnostic — anything that can produce the canonical
dump format below can be compared.

## Canonical dump format

Both engines emit one dump per case/checkpoint:

```
case <name>
steps <n>
pc <6-hex>
sr <6-hex>
... (all core registers: sr omr la lc sp ssh ssl ep sz sc vba,
     x0 x1 y0 y1 a0 a1 a2 b0 b1 b2, r0-r7 n0-n7 m0-m7)
cyc <n>        (informational; cycle models are not compared yet)
end
```

## Memory end-state comparison

Register snapshots alone cannot see write-path bugs (wrong address,
wrong space, wrong stored value) unless a case happens to read the
cell back. Both engines therefore support `--dump-mem`: the emulator
(`difftest corpus ... --dump-mem`) and the hardware runner
(`run_corpus_hw.py --dump-mem`) append the words of the sanctioned
operand windows (X:$0000-$0BFF minus the harness window $0400-$04FF,
Y:$0000-$07FF, P:$07C0-$07FF) that deviate from their init value
after each case's registers, as `xm<addr>/ym<addr>/pm<addr>` keys.
`diff.py` defaults a missing memory key to the file's own init (zero,
or the affine fill under a `fill` header — below), so sparse dumps
compare exactly. Old goldens (captured without the flag) simply lack
the keys and keep working; compare memory only between dumps produced
with matching flags.

`--dump-stack` (separate flag, same auto-detection in check_goldens)
dumps the 15 hardware stack slots (`sh<slot>` / `sl<slot>` keys, slot
1-15, deviation-encoded against the seed): the
hardware snapshot walks the stack descending from slot 15 (SSL
in-place read, then SSH pop — never reading at sp=0, and with a nop
after the SP write for the view-refresh shadow), and the emulator
dumps `s.stack` directly. This closes the frames-at-depth blind spot:
slot contents survive pops, so a wrong value pushed and popped again
was invisible to register and memory snapshots.

## Dirty-state banks

Against the historical zero-fill, a wrong-address *read* of a cell
nobody wrote is self-consistent (every untouched cell holds the same
value, 0). A bank can therefore declare `Bank.fill = (kx, cx, ky, cy,
sshb, sslb)` in `banks.py`: both engines pre-fill X:$0000-$0BFF with
`(kx*addr+cx) & $FFFFFF`, Y:$0000-$07FF with `(ky*addr+cy) & $FFFFFF`
(K odd, so every address holds a unique word and any wrong read/write
diverges instantly), and seed hardware stack slot j (1..15) with
SSH=`sshb+j-1` / SSL=`sslb+j-1`. The tuple is emitted as the meta's
first line (`fill kx cx ky cy sshb sslb`), so it is covered by the
sealed fingerprint and everything downstream self-configures from the
meta alone: `difftest` applies the fill and prints the same `fill`
header in its dumps, the hardware runner switches its boot preamble to
an affine-fill walk plus a stack-ramp seed, and both sides emit memory
dumps as deviations from the fill. P memory is never filled (the
hardware bootstrap image already owns it). First user: bank b15
(`cases_dirty.py`).

## Corpus flow

```
python3 tools/difftest/corpus.py            # generate + assemble the corpus
cargo run -p dsp56300-emu --bin difftest -- \
    corpus tools/difftest/out/corpus.lod tools/difftest/out/corpus.meta \
    > tools/difftest/out/results_emu.txt    # our emulator
python3 tools/difftest/run_sim.py           # oracle (sim56300 under wine)
python3 tools/difftest/diff.py tools/difftest/out/results_emu.txt \
    tools/difftest/out/results_sim.txt      # compare
```

Cases are short self-initializing assembly programs in fixed P-memory
slots (see `corpus_pkg/registry.py` for the authoring rules). Both engines execute the
cases in the same order from the same load image, so state carried across
cases stays comparable; each case's `end` PC and dynamic instruction count
come from our emulator, and the sim executes the same count (`change pc` +
`step k`), so a control-flow divergence shows up as a `pc` mismatch.

The corpus lives in `corpus_pkg/`, organized by **theme**: cases are
authored in `cases_alu.py`, `cases_loops.py`, `cases_stack.py`, and so
on, while **bank membership and order** live in `corpus_pkg/banks.py`.
Banks are a packing artifact, not a taxonomy: each bank is one bootable
image sized to the hardware runner's single 0x800-word bootstrap DMA
window (P:$0100 up to the snapshot code at P:$0720), and case order
inside a bank is load-bearing (images replay sequentially on hardware,
carrying register state across cases).

Captured banks are **sealed**: `corpus_pkg/sealed.json` records a
fingerprint over the canonical content both engines consume (memory
words + meta), and the generator refuses to emit a changed image for a
sealed bank - edit-by-accident becomes a hard error naming the bank.
New cases go to the open bank (see `corpus.py --status`); after a
hardware capture session the new bank is sealed with
`corpus.py --seal <bank>`. Editing a sealed case means cloning it into
the open bank under a new name and recapturing.

Status: **all cases in all sealed banks match real MCPX silicon**
(`cargo test` gates every bank in both modes through
`crates/emu/tests/goldens.rs`; `check_goldens.py` does the same offline
and additionally cross-checks the manifests and sealed fingerprints). Bank 0 also matches sim56300 (pre-slot-repack
layout); the newer banks have not been run against the sim yet (see the
wine caveat below).

The corpus keeps test data at or below X:$05FF: with the sound engine
enabled (the hardware runner needs it), GP X:$0C00-$0FFF is the sound
engine's live mixbuffer (aliased at X:$1400) and is rewritten every
audio frame, so stores there appear not to land.

## Execution modes: validate BOTH

The emulator has **two distinct codegen paths**: `--mode=step` drives
`execute_one()` (each instruction compiled as its own function, cached
by opcode) and `--mode=block` drives `run()` the way an embedder does
(multi-instruction basic blocks, inline loops, deferred flags). Bugs
regularly hide in exactly one of them — the deferred-CCR family, the
inline-loop boundary family, and the stale-SM-marker bug were all
block-only; loop-back modeling differs by construction. **Every
capture, golden check, and probe must be compared in both modes.**

The `goldens` cargo test runs both. `check_goldens.py` runs only the
default mode unless given `--mode=block`; a per-bank sweep is:

```
for d in tools/difftest/goldens/b*; do
  args="corpus $d/corpus.lod $d/corpus.meta --boot=hw --mode=block"
  grep -qE "^(xm|ym|pm)[0-9a-f]" $d/results_hw.txt && args="$args --dump-mem"
  grep -qE "^(sh|sl)[0-9a-f]{2} " $d/results_hw.txt && args="$args --dump-stack"
  target/release/difftest $args > /tmp/blk.txt
  python3 tools/difftest/diff.py /tmp/blk.txt $d/results_hw.txt
done
```

## Step-vs-block fuzzing (no console required)

`fuzz.py` generates seeded random programs and uses the two codegen
paths as each other's oracle:

```
python3 tools/difftest/fuzz.py {mixed|flow|flags|agu|stack} <n> <seed_base>
```

Divergences are minimized and saved under `out/findings/` (`.asm` +
`.txt` with the differing registers). Read EVERY pool's
`round ... done` line — a single divergence is a finding, not noise.
Re-run all pools after any emit-layer change. Before treating a
finding as a regression, rebuild the pre-change revision in a worktree
(`git worktree add ... <rev>` + `CARGO_TARGET_DIR=target/fixcheck`)
and re-run the minimal repro: pre-existing findings get recorded and
silicon-arbitrated (run the repro as an isolated hardware case to
learn which mode is right), not blamed on the current change. This
technique found nine codegen bugs in one sweep plus several since.

## Hardware probe sessions (isolated cases)

The workhorse for characterizing silicon outside the corpus: a
1-case lod+meta pair, one fresh boot per run.

1. Write `out/probe_X.asm`: `org p:$0100`, first line `andi #$00,ccr`.
   The `end` directive must be the LAST line — a mid-file `end`
   silently drops later `org` sections from the lod.
2. Assemble: `dsp56300-asm -f lod -o out/probe_X.lod out/probe_X.asm`;
   the meta end PC is the last case-region P address + 1.
3. Meta: optional `fill kx cx ky cy sshb sslb` first line (seeded
   memory + stack ramp makes stale-vs-fresh distinguishable), then
   `case probe_X 0100 <end> <steps>`.
4. Emulator, both modes: `difftest corpus <lod> <meta> --boot=hw
   --mode=step|block --dump-mem --dump-stack`.
5. Hardware: `run_corpus_hw.py <lod> <meta> --dump-mem --dump-stack
   --out <hw.txt>` (xbtest repo), then `diff.py hw emu`.

Wedge-suspect probes run in isolation, scariest last, with a
`dspctl.py ping` after each; a boot that never reaches its end PC
reports `did not reach end pc (magic ... != ...)` and the next boot
recovers. A wedge in an isolated boot is itself an answer.

### Fault probes (VBA redirect)

Error conditions become observable by redirecting the interrupt
vector base into the image: the case executes `movec #>$000200,vba`,
the file places `jsr >$000300` at `org p:$0202` (stack-error vector =
VBA:$02) and a handler at `$0300` that records state (`movec sp,n6` /
`movec sr,n7`) and resumes with `rti` or jumps to the end sled. The
exception frame is visible in the stack-slot dump; the frame's saved
PC is readable even from slot 0 via a deliberate handler-side pop
(the SE latch suppresses re-faults). This is how the stack-error
delivery model was pinned — see ARCHITECTURE-NOTES "Stack-Error
Exception Delivery" for the semantics (anchor + 6-word shadow window,
per-class anchors, annulment, frames, SR mask).

## Fault banks (Bank.aux)

Error-path cases live in the corpus like any others (`cases_faults.py`,
bank b17): a bank can declare `Bank.aux = (asm lines with org
directives)` in `banks.py` — appended to the combined image after the
cases and covered by the sealed fingerprint — to hold a vector page
and shared handler. Fault cases keep the delivery shadow window
branch-free and SSH/SP-write-free, and end with a cleanup tail
(SP, SC, VBA, and SR if touched) so the next case starts clean; see
the authoring rules in `corpus_pkg/registry.py`.

## Coverage accounting

`difftest coverage <lod> <meta> [...]` statically decodes every case
range and reports covered vs. missing OPCODE_TABLE entries, parallel-ALU
opcode bytes, and parallel-move classes. Current standing over all
banks: **142/185 distinct table entries covered, 43 excluded, 0
missing**, **253/253 ALU bytes**, and every silicon-testable
parallel-move class. The 43 exclusions are printed with
reasons by the tool: peripheral-space forms (bit ops/branches on pp/qq
and all movep variants - live APU registers give nondeterministic
dumps), the cache ops (pflush/pflushun/pfree/plock/plockr/punlock/
punlockr - **verified to wedge MCPX silicon**, isolated probes
), DEBUG (halts the core awaiting OnCE; debugcc with a false
condition is safe and covered), and the power/peripheral-state group
(reset/stop/wait). ILLEGAL and TRAP/TRAPcc are covered through the
fault banks (`Bank.aux`). ENDDO outside a loop (LF=0) also wedges
silicon and is excluded by authoring rules.

## Real-hardware goldens (offline regression)

`goldens/` holds the corpus and the real-silicon dumps captured for it
(provenance in `goldens/<bank>/PROVENANCE.md`), so the JIT can be regression-tested
against real-hardware truth without a console:

```
cargo test -p dsp56300-emu --test goldens   # both modes, part of CI
python3 tools/difftest/check_goldens.py     # cargo-runs difftest, diffs goldens
```

`MANIFEST.sha256` binds the goldens to the exact corpus that produced them;
`check_goldens.py` refuses to run on a mismatch. Regenerate goldens (console
required) only when the corpus changes — see `goldens/PROVENANCE.md`.

The live-hardware flow (console reachable) is:

```
python3 $XBTEST/client/run_corpus_hw.py tools/difftest/out/corpus.lod \
    tools/difftest/out/corpus.meta --out results_hw.txt
cargo run -p dsp56300-emu --bin difftest -- \
    corpus tools/difftest/out/corpus.lod tools/difftest/out/corpus.meta \
    --boot=hw > results_emu_hw.txt
python3 tools/difftest/diff.py results_emu_hw.txt results_hw.txt
```

`--boot=hw` mirrors silicon boot state where the hardware runner leaves it
alone (OMR=$30F from the mode pins, VBA=$FF0000).

## Running a whole-state snapshot (`run-to`)

An embedder can write the DSP's whole state - P, X and Y memory, the
register file, the hardware stack and the PC - as a LOD with `P|X|Y <addr>
<word>` memory records plus `R <idx> <word>`, `SSH|SSL <slot> <word>` and
`PC <word>` lines. The runner executes it from there:

```
difftest run-to <snapshot.lod> <step|block> <max_cycles> <stop_pc>
```

runs until the PC reaches `stop_pc` (or the cycle cap) on the step or the
block engine and prints the final PC, cycles, registers, X:$0000-$011F and
Y:$0000-$00FF as `name value` lines. A hardware runner that emits the
same lines from the console lets a DMA-free segment of a real program be
diffed three ways: step, block and silicon. `DIFFTEST_TRACE_PCS=<path>`
traces every step as `pc opcode next sp`; `DIFFTEST_PHASE_ALL_X=1` prints
all of X.

## Driving sim56300 (`simdrv.py`)

The simulator is an interactive console TUI; it cannot be scripted via
stdin piping but works on a pty with quiescence-based pacing (~0.3s per
command). Verified facts are documented in `simdrv.py`'s header (LOD
loading, `step`/breakpoint behavior, `change` syntax, what `input`
actually is). Parse the raw pty stream, not the `log` file (the log can
truncate after error redraws).

## Future work

- Cycle-count comparison: the sim's `cyc` counter behaves non-monotonically
  around debugger `change pc` pokes; measuring cycles needs uninterrupted
  `go` runs between breakpoints instead of per-case pc rewrites.
- Optionally simplify the staged-movec corpus cases (`agu_modulo`,
  `agu_bitrev`) and the hw runner's preamble back to `movec #imm,mN`
  spellings now that the assembler is fixed (recapture goldens on
  hardware — the P image changes).
- Re-run the sim56300 comparison on the repacked (0x18-word-slot) corpus
  layout to keep the three-way sim/JIT/silicon status current.
- sim56300 under wine currently ignores pty input in a fresh wine prefix
  (banner appears, commands echo, no response) -- debug the wine console
  handling before rerunning run_sim.py.

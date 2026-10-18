# Bank b15 goldens provenance

Captured 2026-08-24 from real MCPX APU GP silicon (xbtest oracle, one fresh
boot per case, `--dump-mem`). First **dirty-state bank**: the corpus meta
carries a `fill` header
(`fill 00a50d 03157e 00c3a9 2b0c99 0000a0 0000c0`), so both engines
and the hardware preamble pre-fill X:$0000-$0BFF / Y:$0000-$07FF with
the affine pattern `(K*addr+C) & $FFFFFF` and seed the 15 hardware
stack slots with an SSH/SSL ramp, instead of the historical
zero-fill. Reads of never-written cells are deterministic and
address-unique, so wrong-address/wrong-space reads and writes diverge
instantly; memory dumps are encoded as deviations from the fill.

The 13 cases deliberately read memory they never wrote: direct fill
probes at the window edges (X and Y), the stack-slot ramp popped
through a descending SP walk, dirty read-modify-write, modulo-wrap /
bit-reverse / dual-parallel / L-move reads over dirty cells, a
zero-store visibility check, an off-by-one neighbor read, a loop
count and an indexed offset harvested (masked) from dirty cells, and
a control-register load from a dirty cell.

Finding from this capture (first divergence of the dirty-state
methodology, invisible under zero-fill): a direct SP write refreshes
the silicon's SSH/SSL top-of-stack views one instruction late - reads
in the shadow return the stale pre-write view, and a shadowed SSH pop
loses its SP decrement. Characterized with a five-probe matrix
(documented in docs/ARCHITECTURE-NOTES.md); the corpus authoring
rules now ban stack-view reads in the instruction after a direct SP
write, and `dirty_stack_ramp` carries a load-bearing `nop` spacer.
Silicon matches the JIT in both `--mode=step` and `--mode=block`,
registers and memory end-state, 13/13.

Recaptured same day with the stack-walk epilogue: `--dump-mem` now
also records the 15 hardware stack slots per case (`sh`/`sl` keys,
deviation-encoded against the seed ramp), captured by a descending
SSL/SSH walk from slot 15 in the snapshot. Registers and memory were
byte-identical to the first capture; the slot data (e.g. the DO frame
residue `dirty_do_dyn` leaves in slot 1) matches the JIT's modeled
stack in both modes.

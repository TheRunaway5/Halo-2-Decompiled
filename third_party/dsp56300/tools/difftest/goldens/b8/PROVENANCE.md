# Bank b8 goldens provenance

Captured 2026-08-24 from real MCPX APU GP silicon (xbtest oracle, one fresh
boot per case). Gap fill from the four-agent coverage analysis: the
qq_reg_mulshift register rows, minus-form (k=1) multiplies across the whole
family, ADC/SBC with carry-in 0, $800000 x $800000 through every multiply
class, Tcc accumulator transfers and the six never-exercised condition codes,
bit-test-branch EA-update commit semantics, MOVEC read direction and SP/VBA
writes, CMPU source codes, DO count sources (SP, accumulators, Y space),
out-of-range register shift counts {31,32,40,55,56,63}, absolute-address
parallel moves, pm4/pm8 residue, AGU bit-reverse and multi-wrap corners,
accumulator limiting rails, and DO bodies exercising each inline-analyzer
reject guard.

Wedge-suspect cases (SP/VBA writes, out-of-range shifts, M=0 plain
updates, DO SP) lead the bank and were probed as isolated single-case
boots before the full capture.

The capture caught three emulator defects, fixed before these goldens
were recorded (silicon verdicts in docs/ARCHITECTURE-NOTES.md):

- **M=0 plain (Rn)+ / (Rn)- toggle bit 0** of Rn, independent of N and
  direction; the N-derived reverse-carry walk applies only to the
  (Rn)+Nn forms, and (Rn)+Nn with Nn=0 is a no-op. The JIT previously
  ran the N-derived walk for the plain forms.
- **DO/DOR with SP as count source loads LC = SP**, not the manual's
  SP+1 (p.13-56); probed at SP=1 and SP=3.
- **Multiply-with-shift immediate count #0 multiplies by zero**:
  mpy/mpyr #0 store 0 with zero-result flags, mac/macr #0 leave the
  accumulator unchanged; the JIT previously computed S * 2^0.

All 36 cases match the JIT in both --mode=step and --mode=block after
the fixes.

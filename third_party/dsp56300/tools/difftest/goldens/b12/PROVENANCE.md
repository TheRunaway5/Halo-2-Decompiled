# Bank b12 goldens provenance

Captured 2026-08-24 from real MCPX APU GP silicon (xbtest oracle, one fresh
boot per case). Second bank of gap-analysis round 2: MOVEM register-class
fan-out (control/M/accumulator classes through P space) and P-ea update modes,
MOVEC through Y space + update/indexed modes + register-form SR write +
short-immediate destination classes, X-space long-displacement moves (the
reclaimed jclr-collision code points), short-displacement accumulator
operands, Y-space aa forms across all twelve bit-op/bit-test-branch families,
bit ops on control registers and FULL accumulators, jump-family and
bit-test-branch EA-update commits, DO/DOR/REP EA-mode and accumulator-count
sources, DO FOREVER nested in a finite body, ENDDO restoring FV=1, pmove mode
residue (pm8 +Nn both sides, pm0/pm1/pm2 modes, pm4 indexed, pm3 dests, VSL
variants), IFcc breadth, shift count-source fan-out, ASR high immediates, ROR
carry-source discriminator, and a 70-op straight-line block.

The capture caught TWO emulator defects, fixed before these goldens
were recorded (verdicts in docs/ARCHITECTURE-NOTES.md):

- **Full-accumulator bit ops go through the move path**: limited
  (scaled) 24-bit read, RMW on the limited value, sign-extended
  write-back clearing the low word, L set by the read. The emulator
  operated on a raw register slot.
- **The S flag's scaling bit-pairs were swapped**: scale-down watches
  bits 47^46 and scale-up 45^44 (the pair moves WITH the scaling
  direction). Surfaced by b11's srm_scl_s_flag, fixed together with
  this capture.

All 32 cases match the JIT in both --mode=step and --mode=block.

# Bank b4 goldens provenance

Captured 2026-08-23 from real MCPX APU GP silicon (xbtest oracle, one fresh
boot per case). Coverage-gap bank: remaining ALU immediate entries,
extract-immediate, register-only Tcc, the DOR source matrix, REP ea plus the
REP LC=0 register/aa/ea zero-execution edges, movec aa short forms, the
bit-test-branch register/aa/ea matrix with high (16-23) bit numbers pinning
the 5-bit decode-template fix, debugcc-not-taken, movem P-space writes
(P:$07C0+ scratch past the snapshot code), A2/B2 sign-extending move reads,
L-flag conditions, parallel-move class gaps, a 15-deep stack fill/drain,
remaining DMAC pairs, and the ADDL/SUBL carry-edge matrix (C from the add/sub
stage only - silicon-verified).

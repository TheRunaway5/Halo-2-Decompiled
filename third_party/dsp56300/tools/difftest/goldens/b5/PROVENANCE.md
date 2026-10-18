# Bank b5 goldens provenance

Captured 2026-08-23 from real MCPX APU GP silicon (xbtest oracle, one fresh
boot per case). Exhaustive parallel-ALU opcode byte sweep generated from
`difftest alu-dump`: every defined ALU byte ($01-$FF, 252 ops after excluding
plain `move`) executes at least once against fixed register patterns. This
bank caught the ADDL/SUBL carry rule divergence on first capture.

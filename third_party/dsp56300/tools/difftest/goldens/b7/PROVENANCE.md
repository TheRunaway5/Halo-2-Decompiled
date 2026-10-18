# Bank b7 goldens provenance

Captured 2026-08-24 from real MCPX APU GP silicon (xbtest oracle, one fresh
boot per case). Parallel-move class completion from the coverage gate's pmove
report: register stores to Y memory through update EA modes, the XY dual-read
form (alone and under an ALU op), and A/B as the transfer register in the
single-space (pm1) and dual-space (pm8) classes. Extended same-day with
X:R-class (pm1) A/B transfer cases and the pm8 cross combinations, then
recaptured; all seven cases matched silicon on first capture. A related
isolated probe (not bankable: the assembler never emits the word) pinned the
immediate-move X/Y space bit as a don't-care alias - see
docs/ARCHITECTURE-NOTES.md.

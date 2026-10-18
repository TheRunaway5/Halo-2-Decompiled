# Bank b6 goldens provenance

Captured 2026-08-24 from real MCPX APU GP silicon (xbtest oracle, one fresh
boot per case). Coverage-driven gap fill: emit paths the unit suite exercised
that no hardware case had validated - movem aa-form, the L-register store/load
matrix (including limited stores), short-aa and parallel-ALU L moves, LSL #0,
ORI-into-MR, SSH bit-test semantics, and REP/terminator bodies inside DO.

First capture caught three JIT defects, fixed before these goldens
were recorded: BTST on SSH pops the stack; JCLR/JSET/JSCLR/JSSET on
SSH pop as well (the manual documents pop only for the BR*/BS*
variants); and BSET/BCHG/BCLR on SSH rewrite the top stack slot in
place - the JIT previously updated only the SSH mirror, so the
modification vanished at the next push/pop.

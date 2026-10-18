# Bank b18 goldens provenance

Captured 2026-08-25 from real MCPX APU GP silicon (xbtest oracle, fresh boot,
`--dump-mem --dump-stack`, zero-fill).

Pins the silicon arbitration of fuzz finding `flow_seed92015` (a
pre-existing step-vs-block JIT divergence): a limiting L-move
(`move ab,l:`) inside a DO body whose next iteration begins with a
flag-writing ALU op (`asr`). The limiter sets the sticky L flag via
the `jit_read_accu24` helper's in-memory SR write; block mode's
lazily-invalidated promoted SR variable rode the loop backedge stale,
and iteration >= 2's flag flush wrote the pre-limit SR back over
memory, losing L. Silicon says L survives (SR `c00354`, matching step
mode); the fix reloads helper-written promoted registers eagerly
(`refresh_promoted_reg`) so the backedge phi carries the fresh value.

Silicon matches the JIT in both `--mode=step` and `--mode=block` -
registers, memory end-state, and stack slots, 1/1 on the first
capture after the fix.

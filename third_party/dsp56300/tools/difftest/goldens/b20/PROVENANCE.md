# Bank b20 goldens provenance

Captured 2026-08-25 from real MCPX APU GP silicon (xbtest oracle, fresh boot,
`--dump-mem --dump-stack`, zero-fill).

Pins the silicon arbitration of fuzz finding `flags_seed630038` (a
pre-existing step-vs-block JIT divergence, verified against two
pre-session binaries): an SM-mode `asl` saturates and leaves its flag
computation pending, a direct `movec #,sr` write discards the pending
computation, and the next flush (`cmp`) ORed the orphaned
`sm_needs_sat` V/L marker into the freshly written SR in block mode.
Silicon says the clean SR (`c00b31`, matching step mode) is right;
the fix zeroes the marker whenever a direct SR store discards a
pending computation. Sibling of b16's `ifcc_sm_stale_sat` (there the
pending computation flushed; here it is discarded).

Silicon matches the JIT in both `--mode=step` and `--mode=block` -
registers, memory end-state, and stack slots, 1/1 on the first
capture after the fix.

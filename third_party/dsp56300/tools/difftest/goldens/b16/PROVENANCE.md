# Bank b16 goldens provenance

Captured 2026-08-24 from real MCPX APU GP silicon (xbtest oracle, fresh boot,
`--dump-mem --dump-stack`, dirty-state fill per the meta header). Single
regression shape for the stale-SM-marker block-codegen bug found by the
post-SSL-fix fuzz sweep (pool `flags`, seed 82565):

- `ifcc_sm_stale_sat`: under SM, a plain-IFcc'd ASL saturates (CCR
  update architecturally suppressed), then a non-saturating `cmp`
  whose pending-flag kind includes the SM term flushes into an SR
  stash. Block mode ORed the suppressed op's leftover `needs_sat`
  into V/L (`sr` d00356 vs silicon/step d00314); a second hole let a
  plain IFcc's pending flag computation materialize on top of the
  restored SR. Both fixed (emu: consume-and-zero the marker in
  `emit_sm_vl_deferred`, discard pending flags on the IFcc restore
  path) before this capture; the isolated fuzz program was run on
  silicon first to establish that step mode was the correct side.

Silicon matches the JIT in both `--mode=step` and `--mode=block` -
registers, memory end-state, and stack slots, 1/1.

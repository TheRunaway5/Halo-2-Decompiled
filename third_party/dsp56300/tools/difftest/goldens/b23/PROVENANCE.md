# Bank b23 goldens provenance

Captured 2026-09-01 from real MCPX APU GP silicon (xbtest oracle,
fresh boot, `--dump-mem --dump-stack`, dirty-state fill per the meta
header). 17/17 first capture in both `--mode=step` and `--mode=block`.

Pins the armed core-fault window's interaction with REP, previously a
blind spot: the three-way loop-path differential's two open seeds
minimized to a fault delivering mid-REP with the step and block legs
disagreeing about the live REP context, and no prior probe or golden
had made the delivery position observable across a REP.

Silicon model, measured here:

- **REP locks the fetch stream.** An armed window never delivers
  inside a REP complex, and the iterations consume no stream words:
  `rep #20` fully inside a movec-pop's 7-word window runs all 20
  iterations (`inc a` counts them) and the handler's recorded LC
  (aux vector page stores LC -> n5) is the restored value in every
  shape - delivery always waits for the REP to retire.
- **The complex charges two stream words** (the REP and its one-word
  target, fetched once whatever the count - zero included), and a
  complex that does not fit the remaining window is annulled at the
  REP: the offset sweep (fault_rep_probe_d0..d7) pins annul at rep+3
  for d0..d4, the original armer+8 boundary for d5, and the REP
  itself for d6/d7.
- **Retirement truncates the window** to at most one more stream word
  (annul at rep+3 unless the original boundary comes sooner).
- **A movec SSH pop delivers at start+8 whatever the destination** -
  the register-destination form (`movec ssh,r0`) measures the same +8
  as the memory forms. The bit-op SSH reads stay at start+7 (b21).

Observability trick: each case's armer (`movec ssh,r0` at sp=0) pops
slot-0 storage, which still holds the PREVIOUS case's fault-frame
saved PC - the annul positions chain through r0 case to case, and the
terminal reader case exposes the last one.

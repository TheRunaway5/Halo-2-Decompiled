# Bank b14 goldens provenance

Captured 2026-08-24 from real MCPX APU GP silicon (xbtest oracle, one fresh
boot per case). Regression shapes for the inline-loop codegen bug family found
by step-vs-block differential fuzzing (~3,540 random programs; gap-analysis
round 3):

- `do_annul_regs`: annulled DO (runtime LC=0 from zeroed memory) with
  a pre-DO register write and a memory-EA body - block mode lost the
  pre-DO write (annul path bypassed every flush point).
- `do_annul_zero_clobber`: register written only inside the annulled
  body, block forced to start at the DO - block mode flushed a
  never-defined (zero-initialized) variable over live state.
- `rep_zero_pm_body`: zero-count REP whose body's limited pm read
  invalidates/reloads SR - block mode zero-clobbered registers at
  block end via the same never-defined-variable hazard.
- `tcc_loop_carried`: conditional transfer + accumulate as a
  loop-carried pair - block mode resurrected a stale accumulator from
  memory on arm-skipping iterations from the second iteration on.

All four bugs (plus the LC-backedge staleness their fix uncovered)
were fixed (making inline-loop boundaries memory-authoritative) before
this capture; silicon matches
the fixed JIT in both --mode=step and --mode=block, and 1,000 fresh
fuzz programs across all five pools show zero divergences post-fix.
These cases guard the fixes on both engines from now on.

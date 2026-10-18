# Bank b13 goldens provenance

Captured 2026-08-24 from real MCPX APU GP silicon (xbtest oracle, one fresh
boot per case). Regression shapes for the JIT sequence-state bug family found
by step-vs-block codegen self-difftest (gap-analysis round 3):

- `sm_chain_deferred_l`: back-to-back SM ops where only the first
  saturates, SR stashed only after both - pins the deferred
  sm_needs_sat_var sticky-L chain.
- `rep_ssh_body_pops`: SSH pops inside a REP body, SP read after the
  loop - pins scope-wide invalidation after in-loop helper calls.
- `do_limit_store_sr` / `rep_limit_store_jcc`: limiting accumulator
  stores inside DO/REP bodies with post-loop SR consumption - the
  second via a conditional branch that block mode previously took the
  WRONG way off the stale L flag.

All four bugs were fixed (extern-call invalidation reaching every open scope,
and the TRAPcc conditional-arm merge) before this capture; silicon matches the
fixed JIT in both --mode=step and
--mode=block. These cases guard the fixes on both engines from now on.

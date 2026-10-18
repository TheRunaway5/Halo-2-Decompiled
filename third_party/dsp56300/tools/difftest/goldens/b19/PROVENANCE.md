# Bank b19 goldens provenance

Captured 2026-08-25 from real MCPX APU GP silicon (xbtest oracle, fresh boot,
`--dump-mem --dump-stack`, dirty-state fill per the meta header).

Branch-class and SP-write stack-error delivery (second fault bank;
same vector-page aux pattern as b17). Pins the stream-word budget
model probed the same day with 13 isolated VBA-redirect boots
(rounds 1-5; see ARCHITECTURE-NOTES "Stack-Error Exception
Delivery"):

- JSR push overflow at SP=15: the branch COMPLETES, the shadow rides
  at the target (budget 9 - len), frame in slot 1 with saved PC =
  target+7 (dump-visible as sh01),
- SP writes that set the SE bit are themselves stack errors
  (boundary start+6 flat, 1-word and 2-word forms), while UF-only
  writes do not fault,
- RTS and RTI underflow at sp=0: the branch to slot-0 SSH storage
  executes (RTI also pops SR from slot-0 SSL), then a 2-word window
  runs; pinned with flag-guarded two-phase cases whose phase-1 fault
  frame seeds slot-0 with a known saved PC.

Silicon matches the JIT in both `--mode=step` and `--mode=block` -
registers, memory end-state, and stack slots, 5/5 on the FIRST
capture (the emulator's stream-budget Armed model was implemented
against the isolated probes before this bank was authored).

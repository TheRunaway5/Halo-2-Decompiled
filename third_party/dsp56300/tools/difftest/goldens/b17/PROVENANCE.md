# Bank b17 goldens provenance

Captured 2026-08-25 from real MCPX APU GP silicon (xbtest oracle, fresh boot,
`--dump-mem --dump-stack`, dirty-state fill per the meta header). First fault
bank: stack-error exception delivery via VBA redirect (cases_faults.py; the
bank's aux blob at P:$0600 is the vector page - `jsr >handler` at VBA:$02 -
plus the shared handler that records SP -> n6 / SR -> n7 and resumes with
RTI).

Covers the silicon-pinned delivery model (ARCHITECTURE-NOTES
"Stack-Error Exception Delivery", probed the same day with 13
isolated VBA-redirect boots):

- underflow pops at sp=0 (btst and movec reads; slot-0 frame, SP $30,
  RTI pops the slot-0 frame back),
- in-place SSH write at sp=0 (SP $00 -> $30 UF|SE, frame in slot 1),
- SR entry mask (S0 cleared at handler entry, restored by RTI),
- boundary annulment (a 2-word move straddling anchor+6 is annulled
  pre-vector and executed exactly once after RTI),
- push overflow at SP=15 (anchor start+3, length-independent),
- memory-destination pop (anchor one word later than register pops).

Silicon matches the JIT in both `--mode=step` and `--mode=block` -
registers, memory end-state, and stack slots, 7/7 on the first
capture (the emulator's Armed shadow-delivery model was implemented
against the isolated probes before this bank was authored).

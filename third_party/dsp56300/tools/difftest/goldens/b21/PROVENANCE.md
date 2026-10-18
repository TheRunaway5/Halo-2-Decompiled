# Bank b21 goldens provenance

Captured 2026-08-25 from real MCPX APU GP silicon (xbtest oracle, fresh boot,
`--dump-mem --dump-stack`, dirty-state fill per the meta header).

ILLEGAL/TRAP interrupt map plus remaining fault corners (third fault
bank; b17-style long-vector aux page at $0600 plus a fast-vector page
at $0680). Pins the model probed the same day with 13 isolated
VBA-redirect boots (see ARCHITECTURE-NOTES "Core-Fault Map: ILLEGAL,
TRAP, DO/ENDDO, Fast Vectors"):

- ILLEGAL vectors at VBA:$04, TRAP and taken TRAPcc at VBA:$08
  (Table 2-2; the ILLEGAL page's "P:$3E" is a DSP56000 holdover),
  all with a ZERO stream-word budget - the next instruction is
  annulled and RTI resumes AFTER the faulting instruction. TRAPcc
  with a false condition is a plain nop.
- Fast-vector shapes: ILLEGAL through two plain vector words executes
  both, pushes no frame, and resumes at F+len; a fast-vectored stack
  error still annuls at its budget boundary and resumes at the
  ANNULLED address with SP's error bits intact.
- DO push overflow: budget 3 stream words after the 2-word DO
  (start+5), LA/LC stay loaded, frames in slots 1/2, resumed loop
  rides out its iterations and exits by popping the wrapped slot-0
  (old LA/LC) frame.
- ENDDO pop underflow: budget 5 (start+6 flat), SP $00->$3F->$3E,
  LA/LC restored from slot-14 fill, dispatch frame in slot 15.
- Bit-op SP writes match the movec classes: bset #4,sp (SE) faults
  (SpWrite class), bset #5,sp (UF only) does not.
- JSR overflow at SP=15: the overflowing push itself wraps and LANDS
  in slot 0 (seeded $abcd SSL overwritten with the pushed SR).

Silicon matches the JIT in both `--mode=step` and `--mode=block` -
registers, memory end-state, and stack slots, 11/11 on the FIRST
capture (the emulator's zero-budget ILLEGAL/TRAP delivery, fast-vector
pipeline fix, DO/ENDDO budgets, and the inline-DO fault bailout were
implemented against the isolated probes before this bank was
authored).

# Bank b22 goldens provenance

Captured 2026-08-27 from real MCPX APU GP silicon (xbtest oracle,
fresh boot, `--dump-mem --dump-stack`, dirty-state fill per the meta
header).

Closes the two blind spots identified after the loop-path
differential found three defects the 539-case corpus did not exercise:

- **Non-linear addressing inside a REP or inline DO body.** The
  address is recomputed from Rn each iteration and Rn ends correct
  either way, so a pointer that never advanced is invisible in the
  register snapshot - only the stored words tell them apart, and no
  prior case wrote memory from inside such a loop under a non-linear
  M. `rep_mod_store` walks modulo-4 down Y:$0380-$0383 and wraps;
  `rep_bitrev_store` is the reverse-carry (M=0, N=8) butterfly walk
  over Y:$03a0; `do_mod_store` is the same shape through the inline-DO
  arm rather than REP. Silicon writes three and four distinct cells
  respectively, confirming the fixed `emit_update_rn`.

- **A nested DO sharing the enclosing loop's LA.** Hardware ends only
  the innermost loop at that address; the outer one has its LA
  restored behind a PC that has already passed it, so it never
  terminates - LF stays set, its frame stays on the stack, and
  execution runs straight on. Measured here for the first time:
  `do_nested_shared_la` leaves SP=2 with LF set (one frame),
  `do_nested_shared_la3` leaves SP=4 (two of three frames), and
  `do_nested_shared_la_lc1` shows the same with the inner count at 1.
  Until this capture the reference for this shape was our own
  `execute_one`, not silicon.

Silicon matches the JIT in both `--mode=step` and `--mode=block` -
registers, memory end-state and stack slots - 6/6 on the first
capture.

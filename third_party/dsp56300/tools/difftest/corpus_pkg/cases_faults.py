"""Stack-error exception delivery (VBA-redirect fault cases).

Each case redirects VBA to the bank's in-image vector page (see the
bank's `aux` blob in banks.py: `jsr >handler` at VBA:$02, handler
records SP -> n6 / SR -> n7 and returns with RTI), triggers a stack
error, rides out the delivery shadow on NOPs, and restores clean
machine state (SP/SC/VBA/SR) for the next case.

Delivery model (silicon-pinned, see ARCHITECTURE-NOTES
"Stack-Error Exception Delivery"): after the faulting instruction, a
per-class STREAM-WORD budget of shadow words executes - branches
inside the window run and the stream continues at their target - and
the first instruction that would exceed the budget is annulled and
becomes the frame's saved PC. Budgets: register-destination pops and
in-place SSH writes 6, memory-destination pops 7, push overflow
9 - len (JSR-family included: the branch completes and target words
count), SE-bit SP writes 6 - len, RTS/RTI underflow 3 - len (the
branch to slot-0 SSH storage executes). The annulled instruction is
re-executed after RTI, so every window instruction runs exactly once.

Authoring rules for fault cases: no SSH/SP writes inside the shadow
window, branch-class shapes must guard their fault re-entry paths (a
resumed RTS/RTI target re-executes - see the flag-guarded two-phase
cases below), and end every case with the cleanup tail (SP, SC, VBA,
and SR if touched).
"""

THEME = "faults"

_VBA_SET = ["movec #>$000600,vba"]
_CLEANUP = [
    "movec #$0,sp",
    "nop",
    "movec #$0,sc",
    "movec #>$ff0000,vba",
]

# (name, [body lines], max_steps)
CASES = [
    # Underflow pop via bit-test read at sp=0: fault anchors at start+1,
    # delivers at start+7; frame lands in slot 0 (real storage), handler
    # sees SP=$30, RTI pops the slot-0 frame back (SP -> $3F).
    ("fault_btst_ssh_sp0",
     _VBA_SET + ["btst #3,ssh"] + ["nop"] * 7 + _CLEANUP, 200),

    # Underflow pop via movec read at sp=0 (register destination).
    ("fault_read_ssh_sp0",
     _VBA_SET + ["movec ssh,r0"] + ["nop"] * 7 + _CLEANUP, 200),

    # In-place SSH write at sp=0: SP $00 -> $30 (UF|SE, no push), the
    # dispatch frame lands in slot 1 (handler sees SP=$31).
    ("fault_bset_ssh_sp0",
     _VBA_SET + ["bset #3,ssh"] + ["nop"] * 7 + _CLEANUP, 200),

    # SR entry mask: scaling bit S0 set pre-fault is cleared at handler
    # entry (n7) and restored by RTI from the frame; case restores SR.
    ("fault_sr_mask",
     _VBA_SET + ["movec #>$c00700,sr", "bset #3,ssh"] + ["nop"] * 7
     + ["movec #>$c00300,sr"] + _CLEANUP, 200),

    # Boundary annulment: five 1-word markers fill the shadow, then a
    # 2-word move straddles the boundary - annulled pre-vector, executed
    # exactly once after RTI (y0 = $666666 either way, saved PC differs
    # from the marker-only shape and is pinned via the frame slot).
    ("fault_straddle",
     _VBA_SET + ["bset #3,ssh", "move #$11,n0", "move #$22,n1",
                 "move #$33,n2", "move #$44,n3", "move #$55,n4",
                 "move #>$666666,y0", "nop", "nop"] + _CLEANUP, 200),

    # Push overflow at SP=15: anchors at start+3 (length-independent),
    # delivers at start+9; frame lands in slot 1 past the wrapped SP.
    ("fault_push_overflow",
     _VBA_SET + ["move #>$abcdef,x0", "movec #$f,sp", "nop",
                 "movec x0,ssh"] + ["nop"] * 9 + _CLEANUP, 200),

    # Underflow pop with a memory destination (2-word absolute form):
    # anchors one word later than the register form (start+len+1).
    ("fault_pop_mem",
     _VBA_SET + ["movec ssh,x:$0327"] + ["nop"] * 9 + _CLEANUP, 200),

    # --- authored in bank b19 (branch-class + SP-write classes) ---

    # JSR push overflow at SP=15: the branch COMPLETES and the shadow
    # rides at the target (stream budget 9 - len = 7 target words);
    # the frame lands in slot 1 (handler sees SP=$11, saved PC =
    # target+7, dump-visible as sh01).
    ("fault_jsr_overflow",
     _VBA_SET + ["movec #$f,sp", "nop", "jsr lbl_fjo_t", "nop",
                 "lbl_fjo_t: nop", "nop", "nop", "nop", "nop", "nop",
                 "nop", "nop", "nop"] + _CLEANUP, 400),

    # Writing the SE bit into SP is itself a stack error (UF-only writes
    # are not): boundary = write start + 6 words flat. Frame in slot 1.
    ("fault_sp_write_se",
     _VBA_SET + ["movec #$10,sp"] + ["nop"] * 7 + _CLEANUP, 200),
    ("fault_sp_write_se_2w",
     _VBA_SET + ["movec #>$000010,sp"] + ["nop"] * 7 + _CLEANUP, 200),

    # RTS underflow at sp=0, frame-seeded: phase 1 (btst underflow)
    # plants a known saved PC (the annulled jmp J) in slot-0 SSH
    # storage; phase 2's RTS then branches THERE (slot-0 storage is the
    # branch target), executes the 2-word jmp (budget 3 - len = 2), and
    # is annulled at the jmp's target. The x:$3d flag guards the
    # post-fault re-entry of phase 2 (the resumed target re-executes).
    # Evidence: n6/n7 = SP/SR at handler ($30 = underflow class), r4 =
    # slot-0 SSL = the fault frame's saved SR.
    ("fault_rts_underflow",
     _VBA_SET + ["move #>$000000,x1", "move x1,x:$3d",
                 "btst #3,ssh", "nop", "nop", "nop", "nop", "nop", "nop",
                 "jmp lbl_fru_p2", "nop",
                 "lbl_fru_p2: jset #0,x:$3d,lbl_fru_fin",
                 "bset #0,x:$3d",
                 "movec #$0,sp", "nop", "rts",
                 "nop", "nop", "nop",
                 "lbl_fru_fin: movec #$0,sp", "nop", "movec ssl,r4"]
     + _CLEANUP, 400),

    # RTI underflow at sp=0: same two-phase shape; slot-0 SSL is seeded
    # with a sane SR (in-place write at sp=0 is legal) so the RTI pops
    # SR=$c00300 and PC=J, then faults exactly like the RTS case.
    ("fault_rti_underflow",
     _VBA_SET + ["move #>$000000,x1", "move x1,x:$3e",
                 "btst #3,ssh", "nop", "nop", "nop", "nop", "nop", "nop",
                 "jmp lbl_fri_p2", "nop",
                 "lbl_fri_p2: jset #0,x:$3e,lbl_fri_fin",
                 "bset #0,x:$3e",
                 "movec #$0,sp", "nop", "movec #>$c00300,ssl", "rti",
                 "nop", "nop", "nop",
                 "lbl_fri_fin: movec #$0,sp", "nop", "movec ssl,r4"]
     + _CLEANUP, 400),

    # --- authored in bank b21 (ILLEGAL/TRAP map + fault corners,
    #     silicon-pinned probe rounds; see ARCHITECTURE-NOTES
    #     "Core-Fault Map") ---

    # ILLEGAL: vector VBA:$04, ZERO stream-word budget - the next
    # instruction is annulled and becomes the saved PC, so the handler's
    # RTI resumes AFTER the illegal (it is skipped, not re-executed).
    ("fault_illegal",
     _VBA_SET + ["illegal", "nop", "nop", "nop", "nop"] + _CLEANUP, 400),

    # TRAP: vector VBA:$08, same zero-budget delivery.
    ("fault_trap",
     _VBA_SET + ["trap", "nop", "nop", "nop", "nop"] + _CLEANUP, 400),

    # Taken TRAPcc behaves exactly like TRAP (VBA:$08, budget 0).
    ("fault_trapcc_taken",
     _VBA_SET + ["andi #$00,ccr", "trapne", "nop", "nop", "nop", "nop"]
     + _CLEANUP, 400),

    # TRAPcc with a false condition is a plain nop (no vectoring).
    ("fault_trapcc_nottaken",
     _VBA_SET + ["andi #$00,ccr", "trapeq", "move #$77,n0", "nop"]
     + _CLEANUP, 400),

    # ILLEGAL through a fast vector (two plain words at VBA:$04, aux
    # page $0680): both vector words execute, NO frame is pushed, and
    # execution resumes at F+len (the illegal is skipped).
    ("fault_ill_fastvec",
     ["movec #>$000680,vba", "illegal", "move #$11,n0", "move #$22,n1",
      "nop"] + _CLEANUP, 400),

    # A fast-vectored STACK ERROR still annuls at its budget boundary,
    # executes the two vector words, and resumes at the ANNULLED
    # address (which re-executes); no frame, so SP keeps UF|SE - r0
    # records $3f before the cleanup clears it.
    ("fault_se_fastvec",
     ["movec #>$000680,vba", "btst #3,ssh", "nop", "nop", "nop", "nop",
      "nop", "nop", "movec sp,r0"] + _CLEANUP, 400),

    # DO push overflow at SP=15: budget 3 stream words after the 2-word
    # DO (start+5, NOT the JSR push class), LA/LC stay loaded, push2
    # frame in slot 1, dispatch frame in slot 2, and the overflowing
    # push1 wraps into slot 0 (old LA/LC) - which the loop exit then
    # pops back into LA/LC after the RTI resume rides out the two
    # remaining iterations over the nop body.
    ("fault_do_overflow",
     _VBA_SET + ["movec #$f,sp", "nop", "do #2,lbl_fdov_e",
                 "nop", "nop", "nop", "nop", "lbl_fdov_e: nop"]
     + _CLEANUP, 400),

    # ENDDO pop underflow at sp=0: budget 5 (start+6 flat), the double
    # pop takes SP $00->$3F->$3E, LA/LC restore from slot-14 contents
    # (fill ramp), and the dispatch frame lands in slot 15.
    ("fault_enddo_underflow",
     _VBA_SET + ["enddo", "nop", "nop", "nop", "nop", "nop", "nop",
                 "nop"] + _CLEANUP, 400),

    # Bit-op SP write that sets SE faults exactly like the movec form
    # (SpWrite class, start+6 flat, frame slot 1).
    ("fault_sp_bset_se",
     _VBA_SET + ["bset #4,sp", "nop", "nop", "nop", "nop", "nop", "nop",
                 "nop"] + _CLEANUP, 400),

    # Bit-op SP write that only sets UF does NOT fault at the write
    # (r0 records SP=$20; the poisoned SP would fault the next stack
    # op, which this case never performs before its cleanup).
    ("fault_sp_bset_uf",
     _VBA_SET + ["bset #5,sp", "move #$11,n0", "movec sp,r0"]
     + _CLEANUP, 400),

    # JSR overflow at SP=15: the overflowing push itself wraps and
    # LANDS in slot 0 (the seeded $abcd SSL is overwritten with the
    # pushed SR, read back as r4 after the resume clears SP).
    ("fault_jsr_ovf_slot0",
     _VBA_SET + ["movec #>$00abcd,ssl", "movec #$f,sp", "nop",
                 "jsr lbl_fjs_t", "nop",
                 "lbl_fjs_t: nop", "nop", "nop", "nop", "nop", "nop",
                 "nop", "movec #$0,sp", "nop", "movec ssl,r4"]
     + _CLEANUP, 400),
]

# --- authored in bank b23 (armed-budget x REP interaction).
# Silicon model, pinned by this bank on real MCPX GP silicon: REP locks
# the fetch stream, so an armed core-fault window never delivers inside
# a REP complex and the iterations consume no stream words. The complex
# itself (the REP plus its one-word target, fetched once whatever the
# count - zero included) charges TWO words, is annulled at the REP when
# it does not fit the remaining window, and its retirement truncates
# the window to at most one more word (annul at rep+3, unless the
# original boundary comes sooner). A movec SSH pop delivers at start+8
# regardless of destination (register forms included; the bit-op reads
# stay at start+7). Observables: each case's armer (movec ssh,r0 at
# sp=0) reads slot-0 storage = the PREVIOUS case's fault-frame saved
# PC, chaining the annul positions through r0; inc a counts executed
# iterations; the b23 handler records LC -> n5 at delivery.
_REP_PRE = _VBA_SET + ["movec #$0,lc", "clr a", "movec ssh,r0"]

def _rep_shadow(nops_before, count):
    return (_REP_PRE + ["nop"] * nops_before
            + ["rep #%d" % count, "inc a"] + ["nop"] * 10 + _CLEANUP, 300)

CASES += [
    # The five original shapes: control past the window, boundary lands
    # mid-iterations (all iterations still run), on the first iteration,
    # a count far exceeding the window, and a zero count.
    ("fault_rep_after_budget",
     _REP_PRE + ["nop"] * 7 + ["rep #3", "inc a"] + ["nop"] * 2
     + _CLEANUP, 200),
    ("fault_rep_shadow_mid",
     _REP_PRE + ["nop", "rep #6", "inc a"] + ["nop"] * 8 + _CLEANUP, 200),
    ("fault_rep_shadow_edge",
     _REP_PRE + ["nop"] * 5 + ["rep #3", "inc a"] + ["nop"] * 6
     + _CLEANUP, 200),
    ("fault_rep_shadow_big",
     _REP_PRE + ["nop", "rep #20", "inc a"] + ["nop"] * 6 + _CLEANUP, 200),
    ("fault_rep_shadow_zero",
     _REP_PRE + ["nop", "rep #0", "inc a"] + ["nop"] * 8 + _CLEANUP, 200),
    # The saved-PC offset sweep that pinned the model: the base position
    # with no REP, the REP at every offset through the boundary, and the
    # count-1/count-0 variants. The terminal reader exposes the last
    # sweep case's saved PC through its own armer read.
    ("fault_rep_probe_base", (_REP_PRE + ["nop"] * 14 + _CLEANUP, 300)[0], 300),
    ("fault_rep_probe_d0", *_rep_shadow(0, 3)),
    ("fault_rep_probe_d1", *_rep_shadow(1, 3)),
    ("fault_rep_probe_d2", *_rep_shadow(2, 3)),
    ("fault_rep_probe_d3", *_rep_shadow(3, 3)),
    ("fault_rep_probe_d4", *_rep_shadow(4, 3)),
    ("fault_rep_probe_d5", *_rep_shadow(5, 3)),
    ("fault_rep_probe_d6", *_rep_shadow(6, 3)),
    ("fault_rep_probe_d7", *_rep_shadow(7, 3)),
    ("fault_rep_probe_d1c1", *_rep_shadow(1, 1)),
    ("fault_rep_probe_d1c0", *_rep_shadow(1, 0)),
    ("fault_rep_reader", (_REP_PRE + ["nop"] * 14 + _CLEANUP, 300)[0], 300),
]

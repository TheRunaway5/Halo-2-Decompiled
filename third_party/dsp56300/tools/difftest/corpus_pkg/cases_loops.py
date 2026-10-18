"""DO/DOR/REP forms and loop boundary behavior.

Case bodies are verbatim from the original bank lists; bank membership
and order live in banks.py. See registry.py for the authoring rules.
"""

THEME = "loops"

# (name, [body lines], max_steps)
CASES = [
    # --- authored in bank b0 ---

    # --- Loops ---
    ("do_imm", ["clr a", "move #>$000001,x0", "do #5,lbl_do1", "add x0,a", "lbl_do1: nop"], 200),
    ("do_reg", ["move #$3,r0", "clr b", "move #>$000002,x1", "do r0,lbl_do2", "add x1,b", "lbl_do2: nop"], 200),
    ("rep_imm", ["clr a", "move #>$000100,x0", "move x0,a1", "rep #4", "asr a", "nop"], 200),
    ("rep_reg", ["move #$3,n2", "clr a", "move #>$000001,x0", "rep n2", "add x0,a", "nop"], 200),
    ("do_nested", ["clr a", "move #>$000001,x0", "do #3,lbl_dn_o", "do #2,lbl_dn_i", "add x0,a", "lbl_dn_i: nop", "lbl_dn_o: nop"], 400),
    # --- authored in bank b2 ---
    # Loops: boundary conditions
    ("do_lc_one", ["clr a", "move #>$000001,x0", "do #1,lbl_d1", "add x0,a", "lbl_d1: nop"], 100),
    ("do_from_memory", ["move #>$000004,x0", "move x0,x:$35", "clr a", "move #>$000001,x1", "do x:$35,lbl_d3", "add x1,a", "lbl_d3: nop"], 200),
    ("do_from_ea_postinc", ["move #>$36,r1", "move #>$000003,x0", "move x0,x:$36", "clr b", "move #>$000001,x1", "do x:(r1)+,lbl_d4", "add x1,b", "lbl_d4: nop", "move r1,n6"], 200),
    ("do_enddo_early", ["clr a", "move #>$000001,x0", "do #5,lbl_d5", "add x0,a", "enddo", "add x0,a", "nop", "nop", "lbl_d5: nop", "movec lc,y0", "movec #>$0,lc"], 200),
    # BRKcc spelling rules (silicon-verified): no arithmetic instruction
    # immediately before the brk (manual A.3.4) and keep it out of the
    # LA-2..LA zone - violations skip the loop-state restore, corrupt the
    # loop, or wedge the core (see ARCHITECTURE-NOTES.md).
    ("do_brkcc", ["clr a", "move #>$000001,x0", "do #6,lbl_d6", "add x0,a", "nop", "brkne", "nop", "nop", "nop", "lbl_d6: nop", "movec lc,y1", "movec sp,y0"], 200),
    ("do_forever_brk", ["clr a", "move #>$000001,x0", "do forever,lbl_d7", "add x0,a", "cmp #$4,a", "nop", "brkeq", "nop", "nop", "nop", "lbl_d7: nop", "movec sp,y1", "movec lc,y0"], 400),
    ("dor_relative", ["clr b", "move #>$000002,x0", "dor #3,lbl_d8", "add x0,b", "lbl_d8: nop"], 200),
    ("do_nested_deep", ["clr a", "move #>$000001,x0", "do #2,lbl_n1", "do #2,lbl_n2", "do #2,lbl_n3", "do #2,lbl_n4", "add x0,a", "lbl_n4: nop", "lbl_n3: nop", "lbl_n2: nop", "lbl_n1: nop"], 800),
    ("rep_from_memory", ["move #>$000005,x0", "move x0,y:$37", "clr b", "move #>$000001,x1", "rep y:$37", "add x1,b", "nop"], 200),
    ("loop_regs_after", ["movec #>$00dead,lc", "movec #>$00beef,la", "clr a", "move #>$000001,x0", "do #2,lbl_lr1", "add x0,a", "lbl_lr1: nop", "movec lc,x1", "movec la,y1", "movec #>$ffffff,la", "movec #>$000000,lc"], 200),
    ("rep_zero", ["clr a", "move #>$000001,x0", "rep #0", "add x0,a", "move #>$0,y0"], 70000),
    ("do_zero_iterations", ["clr a", "move #>$000001,x0", "do #0,lbl_d2", "add x0,a", "lbl_d2: nop"], 70000),
    # --- authored in bank b4 ---
    # DOR remaining source forms
    ("dor_reg_form", ["move #$2,n1", "clr a", "move #>$000001,x0", "dor n1,lbl_dr1", "add x0,a", "lbl_dr1: nop"], 200),
    ("dor_aa_form", ["move #>$000003,x0", "move x0,x:$22", "clr b", "move #>$000001,x1", "dor x:$22,lbl_dr2", "add x1,b", "lbl_dr2: nop"], 200),
    ("dor_ea_form", ["move #>$23,r2", "move #>$000002,x0", "move x0,x:$23", "clr a", "move #>$000001,x1", "dor x:(r2)+,lbl_dr3", "add x1,a", "lbl_dr3: nop", "move r2,n5"], 200),
    ("dor_forever_form", ["clr a", "move #>$000001,x0", "dor forever,lbl_dr4", "add x0,a", "cmp #$3,a", "nop", "brkeq", "nop", "nop", "nop", "lbl_dr4: nop", "movec sp,y0", "movec #>$c00300,sr", "movec #>$ffffff,la", "movec #>$0,lc"], 400),
    # REP ea form + REP LC=0 via register/aa/ea sources (silicon verdict:
    # LC=0 executes the target zero times; these pin the non-immediate forms)
    ("rep_ea_form", ["move #>$24,r3", "move #>$000003,x0", "move x0,x:$24", "clr a", "move #>$000001,x1", "rep x:(r3)", "add x1,a", "move r3,n6"], 200),
    ("rep_reg_zero", ["move #>$0,x1", "clr a", "move #>$000001,x0", "rep x1", "add x0,a", "move #$1,y0"], 200),
    ("rep_aa_zero", ["move #>$000000,x0", "move x0,x:$25", "clr b", "move #>$000001,x1", "rep x:$25", "add x1,b", "move #$2,y0"], 200),
    ("rep_ea_zero", ["move #>$26,r4", "move #>$000000,x0", "move x0,x:$26", "clr a", "move #>$000001,x1", "rep x:(r4)-", "add x1,a", "move r4,n7", "move #$3,y0"], 200),
    # --- authored in bank b6 ---
    # REP inside a DO body (inline-body rep handling).
    ("do_rep_inside", ["clr a", "move #>$000001,x0", "do #3,lbl_dri",
                       "rep #2", "add x0,a", "nop", "lbl_dri: nop"], 400),
    # DO body containing a subroutine call (block-terminator body: the
    # non-inline loop path).
    ("do_body_bsr", ["clr b", "move #>$000001,x1", "do #2,lbl_db6",
                     "bsr lbl_sub6", "nop", "lbl_db6: nop",
                     "bra lbl_ov6", "lbl_sub6: add x1,b", "rts",
                     "lbl_ov6: nop"], 400),
    # --- authored in bank b8 ---
    # DO with SP as the count source (the documented LC=SP+1 special
    # case). One balancing pre-push; pop it after the loop. Probe-first.
    ("do_sp_source", ["movec #>$00aa01,ssh", "clr a", "move #>$000001,x0", "do sp,lbl_dsp", "add x0,a", "lbl_dsp: nop", "movec ssh,r5", "movec sp,y0"], 200),
    # DO with accumulator count sources (limited 24-bit read path).
    ("do_acc_source", ["clr a", "move #>$000005,x0", "move x0,a1", "clr b", "move #>$000001,x1", "do a,lbl_dac1", "add x1,b", "lbl_dac1: nop", "clr a", "do b,lbl_dac2", "add x1,a", "lbl_dac2: nop"], 400),
    # DO/DOR count sources from Y space (aa and ea forms).
    ("do_y_sources", ["move #>$000004,x0", "move x0,y:$21", "clr a", "move #>$000001,x1", "do y:$21,lbl_dys1", "add x1,a", "lbl_dys1: nop", "move #>$000022,r6", "move #>$000003,x0", "move x0,y:$22", "clr b", "do y:(r6)+,lbl_dys2", "add x1,b", "lbl_dys2: nop", "move r6,n7", "dor y:$21,lbl_dys3", "add x1,a", "lbl_dys3: nop"], 400),
    # DO bodies hitting the inline-analyzer reject guards; each runs via
    # the non-inline path (block-mode codegen differs from step mode).
    ("doinline_jcc", ["clr a", "move #>$000001,x0", "do #3,lbl_dij_e", "add x0,a", "cmp #$3,a", "jeq lbl_dij_s", "add x0,a", "lbl_dij_s: nop", "lbl_dij_e: nop"], 400),
    ("doinline_jmp_jclr", ["clr b", "move #>$000001,x1", "move #>$000000,x0", "move x0,x:$25", "do #2,lbl_djj_e", "add x1,b", "jclr #2,x:$25,lbl_djj_s", "add x1,b", "lbl_djj_s: jmp lbl_djj_t", "add x1,b", "lbl_djj_t: nop", "lbl_djj_e: nop"], 400),
    ("doinline_movem", ["move #>$00c0de,x0", "move #>$000000,x1", "clr a", "do #2,lbl_dmv_e", "movem x0,p:>$07c4", "movem p:>$07c4,x1", "add x1,a", "lbl_dmv_e: nop"], 400),
    # 65-instruction body: one past the analyzer's MAX_INLINE_LEN.
    ("doinline_len65", ["clr a", "move #>$000001,x0", "do #2,lbl_dln"]
     + ["add x0,a"] * 65 + ["lbl_dln: nop"], 400),
    # --- authored in bank b12 ---
    # DO/DOR/REP count-from-memory with EA update side effects beyond
    # (Rn)+: pre-decrement, +Nn, and Y-space forms.
    ("do_rep_ea_modes", ["move #>$000002,x0", "move x0,x:$0210", "move #>$0211,r5", "clr a", "move #>$000001,x1", "do x:-(r5),lbl_de1", "add x1,a", "lbl_de1: nop", "move r5,n4", "move #>$000003,x0", "move x0,y:$0213", "move #>$0213,r6", "move #>$000002,n6", "clr b", "do y:(r6)+n6,lbl_de2", "add x1,b", "lbl_de2: nop", "move r6,n5", "move #>$000002,x0", "move x0,y:$0216", "move #>$0216,r7", "move #>$000002,n7", "rep y:(r7)+n7", "add x1,a"], 400),
    ("dor_rep_more", ["move #>$000002,x0", "move x0,y:$0219", "move #>$0219,r3", "clr b", "move #>$000001,x1", "dor y:(r3)-,lbl_dm1", "add x1,b", "lbl_dm1: nop", "move r3,n4", "clr a", "move #>$000003,x0", "move x0,a1", "clr b", "rep a", "add x1,b"], 400),
    # DO FOREVER nested inside a finite DO body: the one bankable
    # inline-analyzer reject arm left (forever-in-body). The inner
    # frame's SR push carries FV=1... exercised via ENDDO in the sister
    # case below.
    ("do_forever_in_body", ["clr a", "move #>$000001,x0", "do #2,lbl_df_o", "do forever,lbl_df_i", "add x0,a", "cmp #$2,a", "nop", "brkge", "nop", "nop", "nop", "lbl_df_i: nop", "lbl_df_o: nop", "movec sp,y0", "movec #>$ffffff,la", "movec #>$0,lc"], 800),
    # ENDDO popping a frame while the OUTER loop is a FOREVER: the
    # restored SR half carries FV=1 (the manual-errata-adjacent
    # LF+FV restore, never exercised with FV set).
    ("enddo_fv_restore", ["clr a", "move #>$000001,x0", "do forever,lbl_eo", "do #3,lbl_ei", "add x0,a", "enddo", "movec sr,r5", "jmp <lbl_ebk", "nop", "nop", "lbl_ei: nop", "lbl_ebk: cmp #$2,a", "nop", "brkge", "nop", "nop", "nop", "lbl_eo: nop", "movec sr,r4", "movec sp,y1", "movec #>$ffffff,la", "movec #>$0,lc"], 400),
    # --- authored in bank b14 ---
    # Fuzzer-found inline-loop codegen regression shapes. Annulled DO
    # (runtime LC=0 from zeroed memory) with a pre-DO register write and
    # a memory-EA body: block mode lost the pre-DO write.
    ("do_annul_regs", ["move #>$000142,r2", "move #>$0230,r4", "do x:(r4)+,lbl_da1", "move y:-(r2),x1", "lbl_da1: nop", "move r2,n4", "move r4,n5"], 70000),
    # Register written ONLY inside the annulled body, with the block
    # forced to start at the DO: block mode zero-clobbered it.
    ("do_annul_zero_clobber", ["move #>$123456,x1", "move x1,a1", "clr b", "addl a,b", "move #>$000000,n3", "jmp lbl_zc0", "lbl_zc0: do n3,lbl_zc1", "mpy x1,#3,b", "lbl_zc1: nop"], 70000),
    # Zero-count REP whose body invalidates/reloads SR (limited pm read):
    # block mode zero-clobbered whole registers at block end.
    ("rep_zero_pm_body", ["clr b", "move #>$345678,y0", "move y0,b1", "rep y:$23", "rnd b b,y1", "movec sr,r4"], 70000),
    # Conditional transfer + accumulate as a loop-carried pair: block
    # mode resurrected a stale accumulator from memory on arm-skipping
    # iterations.
    ("tcc_loop_carried", ["move #>$9c1aad,x0", "move #>$010000,y0", "clr a", "mac x0,y0,a", "clr b", "move #>$111111,y1", "dor #4,lbl_tl1", "tcs y1,b", "addr a,b", "lbl_tl1: nop", "movec sr,r5"], 400),
    # --- authored in bank b13 ---
    # Limiting accumulator store INSIDE a DO body, SR read after the
    # loop: block mode must carry the limiter's L/S updates out of the
    # loop.
    ("do_limit_store_sr", ["clr a", "move #>$000012,x0", "move x0,a2", "do #2,lbl_dls", "move a,x:$0220", "lbl_dls: nop", "movec sr,r6", "move x:$0220,y0"], 200),
    # The canonical DSP copy-loop idiom: REP + limiting store, then a
    # conditional on L that must see the limiter's flag.
    ("rep_limit_store_jcc", ["clr a", "move #>$000034,x0", "move x0,a2", "move #>$0224,r0", "rep #4", "move a,x:(r0)+", "move #>$0,y1", "jclr #6,sr,lbl_rlj", "move #>$600d99,y1", "lbl_rlj: move r0,n4"], 200),
    # --- authored in bank b18 ---
    # Limiting L-move (move ab,l:) inside a DO body whose NEXT iteration
    # starts with a flag-writing ALU op (asr): the body-top SR load rides
    # the loop backedge, so a lazily-invalidated promoted SR variable is
    # stale on iterations >= 2 and the flag flush wrote it back over the
    # limiter's sticky L (silicon-arbitrated fuzz finding flow_seed92015:
    # step mode's SR c00354 is correct, block mode lost L).
    ("do_body_asr_limit_l", ["move #>$808080,x1", "move x1,a1", "clr b", "do #5,lbl_dal", "asr b", "move ab,l:$028b", "move l:$028b,a10", "lbl_dal: nop"], 200),
    # --- authored in bank b22 ---
    # A nested DO whose LA is the enclosing loop's LA. Only the innermost
    # loop ends at that address, so the outer one is left with its LA
    # restored behind a PC that has already passed it: it never
    # terminates, LF stays set and its frame stays on the stack while
    # execution runs straight on. SP and SR before the ENDDO are what
    # say whether one frame was popped or two. The corpus had no shared-LA
    # nesting, and this is the shape whose hardware behaviour was
    # inferred rather than measured.
    ("do_nested_shared_la", ["clr a", "move #>$000001,x0", "do #3,lbl_sla", "do #2,lbl_sla", "add x0,a", "lbl_sla: nop", "movec sp,n2", "movec sr,n3", "movec la,n4", "movec lc,n5", "enddo", "movec sp,n6", "movec sr,n7"], 400),
    # Three loops sharing one LA: two frames should survive.
    ("do_nested_shared_la3", ["clr b", "move #>$000001,x1", "do #4,lbl_sl3", "do #3,lbl_sl3", "do #2,lbl_sl3", "add x1,b", "lbl_sl3: nop", "movec sp,r4", "movec sr,r5", "enddo", "movec sp,r6", "enddo", "movec sp,r7", "movec sr,n0"], 800),
    # Shared LA where the inner count is 1, so the inner loop ends on its
    # first pass: the outer frame is still the one left behind.
    ("do_nested_shared_la_lc1", ["clr a", "move #>$000002,x0", "do #3,lbl_s1", "do #1,lbl_s1", "add x0,a", "lbl_s1: nop", "movec sp,n1", "movec la,y0", "movec lc,y1", "enddo", "movec sp,x1"], 400),
]

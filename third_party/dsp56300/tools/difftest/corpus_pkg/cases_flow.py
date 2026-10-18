"""Jumps, branches, subroutines, Tcc, bit-test branches, RTI.

Case bodies are verbatim from the original bank lists; bank membership
and order live in banks.py. See registry.py for the authoring rules.
"""

THEME = "flow"

# (name, [body lines], max_steps)
CASES = [
    # --- authored in bank b0 ---

    # --- Control flow ---
    ("jmp_abs", ["move #>$111111,x0", "jmp <lbl_jmp1", "move #>$222222,x0", "lbl_jmp1: nop"], 100),
    ("jcc_taken", ["clr a", "tst a", "jeq <lbl_jc1", "move #>$bad001,x1", "lbl_jc1: nop"], 100),
    ("jcc_not_taken", ["clr a", "inc a", "tst a", "jeq <lbl_jc2", "move #>$600d02,x1", "lbl_jc2: nop"], 100),
    ("bra_fwd", ["move #>$333333,y0", "bra <lbl_br1", "move #>$444444,y0", "lbl_br1: nop"], 100),
    ("jsr_rts", ["move #>$0,x1", "jsr <lbl_sub1", "move #>$999999,y1", "jmp <lbl_done1", "lbl_sub1: move #>$777777,x1", "rts", "lbl_done1: nop"], 100),
    ("bsr_rts", ["bsr <lbl_sub2", "jmp <lbl_done2", "lbl_sub2: move #>$888888,y0", "rts", "lbl_done2: nop"], 100),
    ("jclr_aa_taken", ["move #>$000000,x0", "move x0,x:$30", "jclr #3,x:$30,lbl_jb1", "move #>$bad003,x1", "lbl_jb1: nop"], 100),
    ("jset_reg_taken", ["move #>$000004,x1", "jset #2,x1,lbl_jb2", "move #>$bad004,y1", "lbl_jb2: nop"], 100),
    ("brclr_aa", ["move #>$000000,x0", "move x0,x:$31", "brclr #7,x:$31,lbl_bb1", "move #>$bad005,x1", "lbl_bb1: nop"], 100),
    ("tcc_transfer", ["clr a", "tst a", "move #>$123456,x0", "clr b", "teq x0,b"], 100),
    # --- authored in bank b1 ---
    # Tcc matrix (flags set up per condition)
    ("tcc_cs_lt", ["clr a", "move #>$800000,x0", "move x0,a1", "add x0,a", "move #>$111111,x1", "move #>$100,r0", "clr b", "tcs x1,b", "tlt x1,b r0,r1"], 100),
    ("tcc_gt_le", ["clr a", "inc a", "move #>$222222,x1", "clr b", "tgt x1,b", "clr a", "dec a", "move #>$333333,y1", "tle y1,a"], 100),
    ("tcc_es_ec_ls", ["clr a", "move #>$400000,x0", "move x0,a2", "tst a", "move #>$444444,x1", "clr b", "tes x1,b", "andi #$00,ccr", "tst a", "tec x1,b"], 100),
    ("tcc_with_rpair", ["move #>$200,r2", "move #>$300,r3", "move #>$0,r1", "clr a", "tst a", "move #>$555555,x1", "clr b", "teq x1,b r2,r3"], 100),
    # --- authored in bank b2 ---
    # Control flow: EA jumps, register branches, PC-relative
    ("jmp_indirect_ea", ["move #>$0,x0", "move #>lbl_ji1,r3", "jmp (r3)", "move #>$bad100,x0", "lbl_ji1: nop"], 100),
    ("jcc_ea_not_taken_side_effect", ["move #>lbl_je1,r4", "clr a", "inc a", "tst a", "jeq (r4)+", "move #>$600d10,x0", "lbl_je1: move r4,n4"], 100),
    ("jsr_indirect", ["move #>lbl_js1,r5", "move #>$0,y0", "jsr (r5)", "move #>$600d11,y1", "jmp <lbl_js2", "lbl_js1: move #>$c0de01,y0", "rts", "lbl_js2: nop"], 100),
    ("bra_register", ["move #$0,x0", "move #$0,y0", "move #$0,y1", "move #$2,r6", "bra r6", "move #$21,x0", "move #$22,y0", "move #$23,y1"], 100),
    ("bsr_register", ["move #$0,x0", "move #$0,y0", "move #$0,y1", "move #$3,r7", "bsr r7", "move #$41,x0", "jmp <lbl_bs2", "move #$42,y0", "move #$43,y1", "rts", "lbl_bs2: nop"], 100),
    ("bcc_backward", ["clr a", "move #>$000001,x0", "lbl_bb2: add x0,a", "cmp #$3,a", "jne <lbl_bb2"], 200),
    # Condition-code sweep (taken/not-taken pairs per flag recipe)
    ("jcc_cs_cc", ["clr a", "move #>$800000,x0", "move x0,a1", "add x0,a", "jcs <lbl_cs1", "move #>$bad110,x1", "lbl_cs1: jcc <lbl_cs2", "move #>$600d14,y1", "lbl_cs2: nop"], 100),
    ("jcc_mi_pl", ["clr a", "dec a", "jmi <lbl_mp1", "move #>$bad111,x1", "lbl_mp1: jpl <lbl_mp2", "move #>$600d15,y1", "lbl_mp2: nop"], 100),
    ("jcc_ge_lt", ["clr a", "dec a", "jge <lbl_gl1", "move #>$600d16,x1", "lbl_gl1: jlt <lbl_gl2", "move #>$bad112,y1", "lbl_gl2: nop"], 100),
    ("jcc_gt_le", ["clr a", "inc a", "jgt <lbl_le1", "move #>$bad113,x1", "lbl_le1: jle <lbl_le2", "move #>$600d17,y1", "lbl_le2: nop"], 100),
    ("jcc_nn_nr", ["clr a", "move #>$400000,x0", "move x0,a1", "tst a", "jnr <lbl_nr1", "move #>$600d18,x1", "lbl_nr1: jnn <lbl_nr2", "move #>$bad114,y1", "lbl_nr2: nop"], 100),
    ("jcc_ec_es", ["clr a", "move #>$100000,x0", "move x0,a2", "tst a", "jes <lbl_es1", "move #>$bad115,x1", "lbl_es1: jec <lbl_es2", "move #>$600d19,y1", "lbl_es2: nop"], 100),
    # Bit-test branch matrix
    ("jsclr_jsset", ["move #>$000010,x0", "move x0,x:$32", "move #>$0,y0", "jsset #$4,x:$32,lbl_jb3", "move #>$bad120,y1", "jmp <lbl_jb4", "lbl_jb3: move #>$c0de03,y0", "rts", "lbl_jb4: nop"], 100),
    ("brset_reg_form", ["move #>$000100,x1", "brset #$8,x1,lbl_bt1", "move #>$bad121,y1", "lbl_bt1: nop"], 100),
    ("bsclr_taken", ["move #>$000000,x0", "move x0,x:$33", "move #>$0,y0", "bsclr #$6,x:$33,lbl_bt2", "jmp <lbl_bt3", "lbl_bt2: move #>$c0de04,y0", "rts", "lbl_bt3: nop"], 100),
    ("jclr_ea_form", ["move #>$34,r0", "move #>$000000,x0", "move x0,x:$34", "jclr #$5,x:(r0),lbl_bt4", "move #>$bad122,y1", "lbl_bt4: nop"], 100),
    # RTI with hand-built frame
    ("rti_hand_frame", ["move #>$0,x1", "movec #>lbl_rt1,ssh", "movec #>$c00300,ssl", "rti", "move #>$bad130,x1", "lbl_rt1: move #>$600d20,y1"], 100),
    # --- authored in bank b3 ---
    ("bra_short_fwd", ["move #$0,x0", "move #$0,y0", "move #$0,y1", "bra @{7}", "move #$11,x0", "move #$12,y0", "move #$13,y1"], 100),
    ("bcc_short_taken_pair", ["clr a", "move #$0,x0", "move #$0,y0", "bcc @{7}", "move #$21,x0", "move #$22,y0", "move #$23,y1", "bcs @{10}", "move #$24,x0", "move #$25,y0", "nop"], 100),
    ("bcc_short_backward", ["move #$0,x0", "move #$0,y0", "clr a", "bra @{8}", "move #$31,x0", "jmp @{10}", "move #$32,y0", "nop", "bcc @{6}", "nop", "nop"], 100),
    ("bsr_short_lit", ["move #$0,x0", "move #$0,y0", "bsr @{6}", "move #$41,x0", "jmp @{9}", "nop", "move #$42,y0", "rts", "nop"], 100),
    ("bscc_short_forms", ["clr a", "move #$0,x0", "move #$0,y0", "bscc @{7}", "move #$51,x0", "jmp @{10}", "nop", "move #$52,y0", "rts", "nop", "bscs @{7}", "nop"], 100),
    ("bscc_long_lit", ["clr a", "move #$0,y1", "bscc >@{7}", "move #$61,y1", "jmp @{9}", "move #$62,y1", "rts", "nop", "nop"], 100),
    ("jcc_short_lit", ["clr a", "move #$0,x0", "move #$0,y0", "jne @{7}", "jeq @{7}", "move #$71,x0", "nop", "move #$72,y0"], 100),
    ("jscc_short_lit", ["clr a", "move #$0,x0", "move #$0,y0", "jseq @{8}", "move #$81,x0", "jmp @{10}", "nop", "move #$82,y0", "rts", "nop", "nop"], 100),
    ("bcc_long_label", ["clr a", "move #$0,x0", "move #$0,y0", "bne lbl_bll1", "beq lbl_bll1", "move #$b1,x0", "lbl_bll1: move #$b2,y0"], 100),
    ("bcc_rn_forms", ["clr a", "move #$3,r7", "move #$0,x0", "move #$0,y0", "bcc r7", "move #$91,x0", "move #$92,y0", "move #$93,y1", "nop"], 100),
    ("bscc_rn_form", ["clr a", "move #$4,r6", "move #$0,x0", "move #$0,y0", "bscc r6", "move #$a1,x0", "jmp @{11}", "nop", "move #$a2,y0", "rts", "nop"], 100),
    ("jscc_ea_form", ["clr a", "move #>@{8},r2", "move #$0,y1", "jseq (r2)", "move #$b1,y1", "jmp @{10}", "move #$b2,y1", "rts", "nop", "nop"], 100),
    ("bra_rn_backward", ["move #>$fffffe,r5", "move #$0,x0", "move #$0,y0", "bra @{8}", "move #$c1,x0", "jmp @{10}", "nop", "bra r5", "nop", "nop"], 100),
    # --- authored in bank b4 ---
    # Tcc register-only form (no accumulator pair)
    ("tcc_regs_only", ["clr a", "tst a", "move #>$0333,r0", "move #>$0,r1", "teq r0,r1", "tne r0,r1"], 100),
    # Bit-test branches: register/aa/ea targets with high bit numbers
    # (silicon-validates the 5-bit bbbbb decode-template fix)
    ("jclr_reg_high_bit", ["move #>$000000,x1", "move #$0,y1", "jclr #$17,x1,lbl_jh1", "move #>$bad200,y1", "lbl_jh1: nop"], 100),
    ("jset_forms_high_bit", ["move #>$800000,x0", "move x0,x:$29", "move #$0,y1", "jset #$17,x:$29,lbl_jh2", "move #>$bad201,y1", "lbl_jh2: move #>$2a,r0", "move #>$c00000,x0", "move x0,x:$2a", "jset #$16,x:(r0),lbl_jh3", "move #>$bad202,y1", "lbl_jh3: nop"], 100),
    ("jsclr_forms", ["move #>$000000,x0", "move x0,x:$2b", "move #$0,y0", "jsclr #$15,x:$2b,lbl_js1", "move #>$bad203,y0", "jmp <lbl_js2", "lbl_js1: move #$11,y0", "rts", "lbl_js2: move #>$2c,r1", "move x0,x:$2c", "jsclr #$14,x:(r1),lbl_js3", "move #>$bad204,y0", "jmp <lbl_js4", "lbl_js3: move #$12,y1", "rts", "lbl_js4: nop"], 100),
    ("jsclr_reg_form", ["move #>$000000,n2", "move #$0,y0", "jsclr #$17,n2,lbl_jr1", "move #>$bad205,y0", "jmp <lbl_jr2", "lbl_jr1: move #$13,y0", "rts", "lbl_jr2: nop"], 100),
    ("jsset_forms", ["move #>$ffffff,x0", "move x0,x:$2d", "move #$0,y0", "jsset #$17,x:$2d,lbl_ju1", "move #>$bad206,y0", "jmp <lbl_ju2", "lbl_ju1: move #$14,y0", "rts", "lbl_ju2: move #>$2e,r2", "move x0,x:$2e", "jsset #$10,x:(r2),lbl_ju3", "move #>$bad207,y0", "jmp <lbl_ju4", "lbl_ju3: move #$15,y1", "rts", "lbl_ju4: nop"], 100),
    ("jsset_reg_form", ["move #>$ffffff,n3", "move #$0,y0", "jsset #$17,n3,lbl_jv1", "move #>$bad208,y0", "jmp <lbl_jv2", "lbl_jv1: move #$16,y0", "rts", "lbl_jv2: nop"], 100),
    ("brclr_forms", ["move #>$000000,x0", "move x0,x:$2f", "move #$0,y1", "brclr #$17,x:$2f,lbl_bc1", "move #>$bad209,y1", "lbl_bc1: move #>$30,r3", "move x0,x:$30", "brclr #$12,x:(r3),lbl_bc2", "move #>$bad20a,y1", "lbl_bc2: move #>$000000,n4", "brclr #$16,n4,lbl_bc3", "move #>$bad20b,y1", "lbl_bc3: nop"], 100),
    ("brset_forms", ["move #>$ffffff,x0", "move x0,x:$31", "move #$0,y1", "brset #$17,x:$31,lbl_bs1", "move #>$bad20c,y1", "lbl_bs1: move #>$32,r4", "move x0,x:$32", "brset #$11,x:(r4),lbl_bs2", "move #>$bad20d,y1", "lbl_bs2: nop"], 100),
    ("bsclr_forms", ["move #>$000000,x0", "move x0,x:$33", "move #$0,y0", "bsclr #$17,x:$33,lbl_bl1", "jmp <lbl_bl2", "lbl_bl1: move #$21,y0", "rts", "lbl_bl2: move #>$34,r5", "move x0,x:$34", "bsclr #$13,x:(r5),lbl_bl3", "jmp <lbl_bl4", "lbl_bl3: move #$22,y1", "rts", "lbl_bl4: move #>$000000,n5", "bsclr #$16,n5,lbl_bl5", "jmp <lbl_bl6", "lbl_bl5: move #$23,y0", "rts", "lbl_bl6: nop"], 100),
    ("bsset_forms", ["move #>$ffffff,x0", "move x0,x:$35", "move #$0,y0", "bsset #$17,x:$35,lbl_bu1", "jmp <lbl_bu2", "lbl_bu1: move #$24,y0", "rts", "lbl_bu2: move #>$36,r6", "move x0,x:$36", "bsset #$0f,x:(r6),lbl_bu3", "jmp <lbl_bu4", "lbl_bu3: move #$25,y1", "rts", "lbl_bu4: move #>$ffffff,n6", "bsset #$17,n6,lbl_bu5", "jmp <lbl_bu6", "lbl_bu5: move #$26,y0", "rts", "lbl_bu6: nop"], 100),
    # debugcc with a false condition is a NOP (taken DEBUG wedges: excluded)
    ("debugcc_not_taken", ["clr a", "move #$0,x0", "debugcs", "move #$31,x0"], 100),
    # --- authored in bank b8 ---
    # Tcc acc-to-acc transfers (56-bit, no limiting) incl. the rpair form.
    ("tcc_acc_xfer", ["clr b", "move #>$123456,x0", "move x0,b2", "move #>$654321,x1", "move x1,b1", "move #>$abcdef,y1", "move y1,b0", "clr a", "move #>$ffffff,x0", "move x0,a2", "move x0,a1", "move x0,a0", "move #>$000001,y0", "add y0,a", "tcs b,a", "tcc x1,b", "move #>$000333,r0", "move #>$000000,r1", "tcs a,b r0,r1"], 100),
    # Tcc condition codes never exercised before (cc, ge, pl, nn, mi, nr).
    ("tcc_cc_matrix1", ["move #>$111001,x0", "move #>$222002,x1", "move #>$333003,y0", "move #>$444004,y1", "clr a", "clr b", "tcc x0,a", "tge x1,b", "move a1,r4", "move b1,r5", "ori #$09,ccr", "tcc y0,a", "tge y1,b", "move a1,r6", "move b1,r7", "andi #$00,ccr", "tpl x0,b", "tnn x1,a", "ori #$18,ccr", "tpl y0,b", "tnn y1,a"], 100),
    ("tcc_cc_matrix2", ["move #>$555005,x0", "move #>$666006,x1", "move #>$777007,y0", "move #>$888008,y1", "move #>$000100,r2", "move #>$000300,r3", "clr a", "clr b", "tmi x0,a", "tnr y0,a", "move a1,r4", "ori #$08,ccr", "tmi x1,b r2,r3", "ori #$10,ccr", "tnr y1,b"], 100),
    # Bit-test-branch EA-update commit semantics: does (rn)+ / -(rn)
    # commit on taken AND not-taken paths? Observe the r registers.
    ("btb_ea_commit_jclr", ["move #>$000000,x0", "move x0,x:$0340", "move #>$ffffff,x1", "move x1,x:$0341", "move #>$0340,r0", "move #>$0341,r1", "jclr #3,x:(r0)+,lbl_bec1", "nop", "lbl_bec1: move r0,n0", "jclr #3,x:(r1)+,lbl_bec2", "bra lbl_bec3", "lbl_bec2: nop", "lbl_bec3: move r1,n1"], 100),
    ("btb_ea_commit_predec", ["move #>$ffffff,x0", "move x0,y:$0344", "move #>$0344,r2", "move #>$000002,n2", "jset #5,y:(r2)+n2,lbl_bej1", "nop", "lbl_bej1: move r2,n4", "move #>$000000,x1", "move x1,x:$0347", "move #>$0348,r3", "brclr #4,x:-(r3),lbl_bej2", "nop", "lbl_bej2: move r3,n5", "brset #4,x:-(r3),lbl_bej3", "nop", "lbl_bej3: move r3,n6"], 100),
    # --- authored in bank b12 ---
    # Y-space aa-form bit-test branches (all eight families were X-only).
    ("btb_y_aa_forms", ["move #>$800000,x0", "move x0,y:$30", "move #>$0,x1", "jclr #4,y:$30,lbl_ya1", "move #>$bad301,x1", "lbl_ya1: jset #23,y:$30,lbl_ya2", "move #>$bad302,x1", "lbl_ya2: brclr #4,y:$30,lbl_ya3", "move #>$bad303,x1", "lbl_ya3: brset #23,y:$30,lbl_ya4", "move #>$bad304,x1", "lbl_ya4: nop"], 100),
    ("btb_y_aa_sub", ["move #>$000000,x0", "move x0,y:$31", "move #>$0,y0", "jsclr #6,y:$31,lbl_ys1", "move #>$bad305,y0", "jmp <lbl_ys2", "lbl_ys1: move #$11,y0", "rts", "lbl_ys2: bsclr #7,y:$31,lbl_ys3", "jmp <lbl_ys4", "lbl_ys3: move #$12,y1", "rts", "lbl_ys4: move #>$ffffff,x1", "move x1,y:$32", "jsset #5,y:$32,lbl_ys5", "move #>$bad306,y0", "jmp <lbl_ys6", "lbl_ys5: move #$13,y0", "rts", "lbl_ys6: bsset #4,y:$32,lbl_ys7", "jmp <lbl_ys8", "lbl_ys7: move #$14,y1", "rts", "lbl_ys8: nop"], 100),
    # Jump-family EA update modes: does the Rn update commit around the
    # control transfer (taken and not-taken)?
    ("jump_ea_updates", ["move #>lbl_ju1,r2", "move #$2,n2", "jmp (r2)+", "lbl_ju1: move r2,n4", "move #>lbl_ju2,r3", "jcs (r3)-", "move r3,n5", "lbl_ju2: move #>lbl_ju3,r1", "move #$3,n1", "move #>$0,y0", "jsr (r1)+n1", "move #>$c0de10,y1", "jmp <lbl_ju4", "lbl_ju3: move #>$c0de11,y0", "rts", "lbl_ju4: move r1,n6"], 100),
    ("jscc_ea_update_taken", ["clr a", "tst a", "move #>lbl_jv1,r5", "move #$4,n5", "move #>$0,y0", "jseq (r5)+n5", "move #>$bad310,y0", "jmp <lbl_jv2", "lbl_jv1: move #>$c0de12,y0", "rts", "lbl_jv2: move r5,n7"], 100),
    # Bit-test-branch EA modes with update side effects on the
    # not-taken/taken paths ((Rn)+Nn, (Rn)-, -(Rn)).
    ("btb_ea_mode_fanout", ["move #>$ffffff,x0", "move x0,x:$0334", "move #>$0334,r0", "move #>$000002,n0", "brset #23,x:(r0)+n0,lbl_bf1", "move #>$bad311,y1", "lbl_bf1: move r0,n4", "move #>$0339,r1", "move #>$0,y0", "jsclr #3,y:(r1)-,lbl_bf2", "move #>$bad312,y0", "jmp <lbl_bf3", "lbl_bf2: move #$21,y0", "rts", "lbl_bf3: move r1,n5", "move #>$033c,r3", "bsset #2,x:-(r3),lbl_bf4", "jmp <lbl_bf5", "lbl_bf4: move #$22,y1", "rts", "lbl_bf5: move r3,n6"], 100),
]

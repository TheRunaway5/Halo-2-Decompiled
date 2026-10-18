"""Parallel-move classes (PM0/1/2/4/8), XY dual, VSL, IFcc.

Case bodies are verbatim from the original bank lists; bank membership
and order live in banks.py. See registry.py for the authoring rules.
"""

THEME = "pmove"

# (name, [body lines], max_steps)
CASES = [
    # --- authored in bank b0 ---
    ("move_xy_dual", ["move #$120,r0", "move #$400,r4", "move #>$aaa111,x0", "move #>$bbb222,y0", "move x0,x:(r0)+ y0,y:(r4)+", "move #$120,r1", "move #$400,r5", "move x:(r1)+,x1 y:(r5)+,y1"], 100),
    # --- authored in bank b1 ---
    # IFcc conditional ALU
    ("ifcc_taken_flags", ["clr a", "move #>$000001,x0", "tst a", "add x0,a ifeq", "add x0,a ifne"], 100),
    ("ifcc_u_updates_ccr", ["clr a", "move #>$800000,x1", "move x1,a1", "tst a", "clr b", "move #>$000001,y0", "add y0,b ifmi.u", "add y0,b ifpl.u"], 100),
    # --- authored in bank b2 ---
    # PM0/PM1 parallel move classes
    ("pm0_class2_xr", ["move #>$0220,r0", "move #>$abc001,x0", "clr a", "move #>$def002,x1", "move x1,a1", "move a,x:(r0)+ x0,a"], 100),
    ("pm1_x_and_reg", ["move #>$0224,r1", "move #>$111222,x0", "move x0,x:$0224", "clr a", "move #>$333444,y1", "move y1,a1", "move x:(r1)+,x0 a,y0"], 100),
    ("pm1_reg_and_y", ["move #>$0228,r5", "clr a", "move #>$555666,x1", "move x1,a1", "clr b", "move #>$777888,y0", "move y0,b1", "move a,x0 b,y:(r5)+"], 100),
    ("pm1_x_imm_with_reg", ["clr a", "move #>$999aaa,y1", "move y1,a1", "move #>$123abc,x1 a,y1"], 100),
    # PM8 dual XY combinations
    ("pm8_acc_both", ["move #>$0230,r1", "move #>$0230,r5", "move #>$aaa000,x0", "move x0,x:$0230", "move #>$bbb000,y0", "move y0,y:$0230", "move x:(r1)+,a y:(r5)+,b"], 100),
    ("pm8_write_accs", ["move #>$0234,r2", "move #>$0234,r6", "clr a", "move #>$ccc111,x0", "move x0,a1", "clr b", "move #>$ddd222,y1", "move y1,b1", "move a,x:(r2)+ b,y:(r6)+"], 100),
    ("pm8_mixed_dir", ["move #>$0238,r3", "move #>$0238,r7", "move #>$eee333,y0", "move y0,y:$0238", "clr a", "move #>$fff444,x1", "move x1,a1", "move a,x:(r3)+ y:(r7)+,y0"], 100),
    # VSL
    ("vsl_insert_bits", ["move #>$0240,r0", "clr a", "move #>$400000,x0", "move x0,a1", "move #>$123456,x1", "move x1,a0", "vsl a,0,(r0)+", "vsl a,1,(r0)+", "move x:$0240,y0", "move y:$0241,y1"], 100),
    # --- authored in bank b4 ---
    # Parallel-move class gaps
    ("pm0_class2_ry", ["move #>$39,r5", "clr b", "move #>$abc123,y0", "move y0,b1", "move b,y:(r5)+ y0,b"], 100),
    ("pm1_x_write_reg", ["move #>$3a,r0", "clr a", "move #>$654321,x1", "move x1,a1", "move a,x:(r0)+ a,y0"], 100),
    ("pm1_y_read_reg", ["move #>$3b,r5", "move #>$999888,x0", "move x0,y:$3b", "clr a", "move #>$777666,x1", "move x1,a1", "move a,x0 y:(r5)+,y1"], 100),
    ("pm1_y_imm_reg", ["clr a", "move #>$555444,x1", "move x1,a1", "move a,x1 #>$123321,y0"], 100),
    ("pm2_ea_update", ["move #>$100,r3", "move #$4,n3", "move (r3)+", "move (r3)+n3", "move (r3)-", "move r3,n0"], 100),
    ("pm4_read_aa_forms", ["move #>$886644,x0", "move x0,x:$3c", "move #>$775533,x1", "move x1,y:$3c", "move x:$3c,y0", "move y:$3c,y1"], 100),
    ("pm4_y_read_ea", ["move #>$3d,r6", "move #>$664422,x0", "move x0,y:$3d", "move y:(r6)-,x1"], 100),
    ("pm8_dual_read_write", ["move #>$3e,r0", "move #>$3e,r4", "move #>$aa11bb,x0", "move x0,x:$3e", "move #>$cc22dd,y0", "move y0,y:$3e", "move x:(r0)+,x1 y:(r4)+,y1", "move #>$3f,r1", "move #>$3f,r5", "move x1,x:(r1)+ y1,y:(r5)+"], 100),
    # --- authored in bank b7 ---
    ("pm_y_store_ea", ["move #>$0f0e0d,y0", "move #>$1f1e1d,y1",
                       "move #>$160,r1", "move #>$2,n1", "move #>$163,r2",
                       "move y0,y:(r1)+n1", "move y1,y:(r2)-",
                       "move y:$160,x0", "move y:$163,x1",
                       "move r1,n4", "move r2,n5"], 100),
    ("pm_xy_dual_read", ["move #>$2a2b2c,x0", "move x0,x:$164",
                         "move #>$3a3b3c,x0", "move x0,y:$168",
                         "move #>$164,r0", "move #>$168,r4",
                         "clr a", "move x:(r0)+,x0  y:(r4)+,y0",
                         "mpy x0,y0,a  x:(r0)-,x1  y:(r4)-,y1",
                         "move r0,n0", "move r4,n6"], 100),
    ("pm_xy_ab_write", ["clr a", "move #>$4a4b4c,x0", "move x0,a1",
                        "clr b", "move #>$5a5b5c,x1", "move x1,b1",
                        "move #>$165,r0", "move #>$169,r4",
                        "move a,x:(r0)+  b,y:(r4)+",
                        "move x:$165,y0", "move y:$169,y1"], 100),
    ("pm1_ab_transfer", ["clr a", "move #>$6a6b6c,x0", "move x0,a1",
                         "clr b", "move #>$7a7b7c,x1", "move x1,b1",
                         "move #>$166,r2", "move #>$16a,r5",
                         "add x0,b  a,x:(r2)+",
                         "add x1,a  b,y:(r5)+",
                         "move x:$166,y0", "move y:$16a,y1"], 100),
    # X:R-class (pm1) with A/B as the memory-transfer register, both
    # directions, plus the register-transfer slot; pins the class's
    # pre/post-ALU value ordering on silicon.
    ("pm1_ab_store", ["clr a", "move #>$8b8c8d,x0", "move x0,a1",
                      "clr b", "move #>$9b9c9d,x1", "move x1,b1",
                      "move #>$16c,r1", "move #>$16d,r2",
                      "rnd a  a,x:(r1)+  b,y0",
                      "rnd b  b,x:(r2)-  a,y1",
                      "move x:$16c,x0", "move x:$16d,x1"], 100),
    ("pm1_ab_load", ["move #>$aba1a2,x0", "move x0,x:$16e",
                     "move #>$bcb1b2,x1", "move x1,x:$16f",
                     "clr a", "clr b",
                     "move #>$000001,x0", "move #>$000002,y0",
                     "move #>$16e,r3", "move #>$16f,r0",
                     "add x0,b  x:(r3)+,a  b,y0",
                     "sub y0,a  x:(r0)+,b  a,y1",
                     "move r3,n1", "move r0,n2"], 100),
    # pm8 cross combinations: B in the X slot with A in the Y slot, and
    # the X-read + Y-write mix (the one class the corpus lacked).
    ("pm8_cross_pairs", ["clr a", "move #>$cac1c2,x0", "move x0,a1",
                         "clr b", "move #>$dbd1d2,x1", "move x1,b1",
                         "move #>$170,r0", "move #>$174,r4",
                         "move b,x:(r0)+  a,y:(r4)+",
                         "move #>$e1e2e3,x0", "move x0,x:$171",
                         "move x:(r0)+,x1  b,y:(r4)+",
                         "move x:$170,y0", "move y:$174,y1"], 100),
    # --- authored in bank b8 ---
    # Parallel move with an absolute address (mode 6 RRR=0), both
    # directions, alongside a live ALU op.
    ("pm0_abs_parallel", ["clr a", "move #>$123321,x0", "move x0,a1", "move #>$000111,x1", "add x1,a  a,x:$0350", "move #>$445566,y0", "move y0,y:$0351", "sub x1,a  y:$0351,b", "move x:$0350,y1"], 100),
    # pm4 register selects: A transferred through Y space, B through X.
    ("pm4_ab_selects", ["move #>$abc111,x0", "move x0,y:$0354", "move #>$def222,x1", "move x1,x:$0358", "move #>$0354,r4", "move #>$0358,r0", "clr a", "clr b", "move y:(r4)+,a", "move x:(r0)+,b", "move a,y:(r4)+", "move b,x:(r0)+", "move y:$0355,y1", "move x:$0359,y0"], 100),
    # pm8 MM=0: dual reads with NO address update, register and acc dests.
    ("pm8_noupd", ["move #>$aa00cc,x0", "move x0,x:$035c", "move #>$bb00dd,x1", "move x1,y:$035c", "move #>$035c,r0", "move #>$035c,r4", "move x:(r0),x0  y:(r4),y0", "clr a", "clr b", "move x:(r0),a  y:(r4),b", "move r0,n0", "move r4,n5"], 100),
    # --- authored in bank b12 ---
    # pm8 with +Nn on BOTH sides (the MM code never used on either).
    ("pm8_pin_modes", ["move #>$0340,r0", "move #>$000002,n0", "move #>$0344,r4", "move #>$000001,n4", "move #>$aa1122,x0", "move x0,x:$0340", "move #>$bb3344,x1", "move x1,y:$0344", "move x:(r0)+n0,x0 y:(r4)+n4,y0", "move r0,n5", "move r4,n6"], 100),
    # pm0 (Class II) with (Rn)+Nn and (Rn)- update modes.
    ("pm0_mode_fanout", ["move #>$0348,r0", "clr a", "move #>$cc5566,x0", "move x0,a1", "move #>$000002,n0", "move a,x:(r0)+n0 x0,a", "move r0,n4", "move #>$034c,r5", "clr b", "move #>$dd7788,y0", "move y0,b1", "move b,y:(r5)- y0,b", "move r5,n7", "move x:$0348,y1"], 100),
    # pm2 (Rn)-Nn update + pm1 -(Rn) read and imm-with-register-write.
    ("pm2_pm1_modes", ["move #>$0350,r3", "move #>$000002,n3", "move (r3)-n3", "move r3,n4", "clr a", "move #>$123123,x1", "move x1,a1", "move #>$ee99aa,x0", "move x0,x:$0351", "move #>$0352,r2", "move x:-(r2),x0 a,y0", "move r2,n5", "clr b", "move #>$456456,y1", "move y1,b1", "move #>$abcabc,x1 b,y1"], 100),
    # pm4 indexed (Rn+Nn) - never appears in ANY pm4 form - plus L: with
    # +Nn update.
    ("pm4_indexed_l_modes", ["move #>$0354,r1", "move #>$000003,n1", "move #>$ff11bb,y1", "move y1,y:(r1+n1)", "move y:$0357,y0", "move #>$0358,r5", "move #>$000002,n5", "move #>$cc22dd,x0", "move x0,x:$035a", "move x:(r5+n5),b", "move r5,n6", "clr a", "move #>$334455,x1", "move x1,a1", "move #>$667788,y0", "move y0,a0", "move #>$035c,r2", "move #>$000001,n2", "move a10,l:(r2)+n2", "move l:$035c,x", "move r2,n7"], 100),
    # pm3 short-immediate destinations never used (b, x1, acc parts).
    ("pm3_more_dests", ["clr a", "move #$7f,a1", "move #$44,a0", "move #$33,a2", "move #$55,b", "move #$66,x1"], 100),
    # VSL: B source, (Rn)-, (Rn)+Nn, and the two-word absolute form.
    ("vsl_more", ["move #>$0360,r2", "move #>$0362,r0", "move #>$000002,n0", "clr b", "move #>$400000,x0", "move x0,b1", "move #>$987654,x1", "move x1,b0", "vsl b,0,(r2)-", "vsl b,1,(r0)+n0", "move x:$0360,y0", "move x:$0362,y1", "move r2,n4", "move r0,n5", "vsl b,1,>$0364", "move x:$0364,x1"], 100),
    # IFcc breadth: conditions beyond eq/ne/mi/pl, including .u forms.
    ("ifcc_breadth", ["ori #$01,ccr", "move #>$000001,x1", "clr b", "add x1,b ifcs", "movec sr,r4", "add x1,b ifge", "movec sr,r5", "andi #$00,ccr", "ori #$08,ccr", "add x1,b iflt.u", "movec sr,r6", "cmp x1,b ifgt.u", "movec sr,r7"], 100),
]

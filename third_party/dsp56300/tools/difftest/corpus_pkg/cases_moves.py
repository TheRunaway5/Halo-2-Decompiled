"""Plain/MOVEC/MOVEM moves, L-space, limiting, displacements.

Case bodies are verbatim from the original bank lists; bank membership
and order live in banks.py. See registry.py for the authoring rules.
"""

THEME = "moves"

# (name, [body lines], max_steps)
CASES = [
    # --- authored in bank b0 ---
    # --- ALU basics ---
    ("move_imm", ["move #>$123456,x0", "move #>$654321,x1"], 100),
    ("move_long_disp_r1", ["move #$100,r1", "move #>$abc123,x0", "move x0,y:(r1+$208)", "move y:(r1+$208),y1"], 100),
    ("move_long_disp_r0", ["move #$5,r0", "move #>$def456,x0", "move x0,y:(r0+$300)", "move y:(r0+$300),y1"], 100),
    ("move_short_disp", ["move #$110,r2", "move #>$654abc,x0", "move x0,x:(r2+2)", "move x:(r2+2),y0"], 100),
    ("move_l_space", ["clr a", "move #>$123456,x0", "move x0,a1", "move #>$654321,x1", "move x1,a0", "move a,l:$130", "move l:$130,b"], 100),
    ("move_limiting", ["move #>$7fffff,x0", "clr a", "move x0,a1", "asl a", "move a,y0"], 100),
    ("move_short_imm", ["move #$12,r3", "move #$34,n3", "move #$56,x0", "move #$78,a"], 100),
    ("movec_regs", ["move #$3,r0", "movec r0,lc", "movec lc,r1", "move #>$001234,r2", "movec r2,la"], 100),
    ("movem_pread", ["move #>$0200,r0", "movem p:(r0),x0"], 100),
    # --- authored in bank b2 ---
    # Accumulator limiting through move reads (scaling modes)
    ("limit_scale_up_read", ["movec #>$c00b00,sr", "clr a", "move #>$800000,x0", "move x0,a1", "move a,y0", "movec #>$c00300,sr"], 100),
    ("limit_scale_up_negative", ["movec #>$c00b00,sr", "clr b", "move #>$ffffff,x0", "move x0,b2", "move #>$7fffff,x1", "move x1,b1", "move b,y1", "movec #>$c00300,sr"], 100),
    ("limit_scale_down_read", ["movec #>$c00700,sr", "clr a", "move #>$000001,x0", "move x0,a2", "move a,x1", "movec #>$c00300,sr"], 100),
    ("limit_scale_up_ok", ["movec #>$c00b00,sr", "clr a", "move #>$200000,x0", "move x0,a1", "move a,y0", "movec #>$c00300,sr"], 100),
    ("limit_write_through", ["clr a", "move #>$7fffff,x0", "move x0,a1", "move #>$ffffff,x1", "move x1,a0", "asl a", "move a,x:$0210", "move x:$0210,y0"], 100),
    # L-space composite registers
    ("lmove_a10_b10", ["clr a", "clr b", "move #>$111111,x0", "move x0,a1", "move #>$222222,x1", "move x1,a0", "move a10,l:$0214", "move l:$0214,b10"], 100),
    ("lmove_x_y_pairs", ["move #>$333333,x0", "move #>$444444,x1", "move x,l:$0216", "move l:$0216,y"], 100),
    ("lmove_ab_ba", ["clr a", "move #>$0000aa,x0", "move x0,a1", "clr b", "move #>$0000bb,x1", "move x1,b1", "move ab,l:$0218", "move l:$0218,ba"], 100),
    ("lmove_ea_forms", ["move #>$021a,r3", "clr a", "move #>$556677,x0", "move x0,a1", "move #>$8899aa,x1", "move x1,a0", "move a10,l:(r3)+", "move l:-(r3),b10"], 100),
    # Displacement move matrix
    ("disp_long_negative", ["move #>$02a4,r1", "move #>$99aabb,x0", "move x0,x:$02a0", "move x:(r1-$4),x1"], 100),
    ("disp_short_y", ["move #>$02a8,r2", "move #>$ccddee,x0", "move x0,y:$02a9", "move y:(r2+1),y0", "move x0,y:(r2+2)", "move y:$02aa,y1"], 100),
    ("movem_read_own_code", ["move #>$0100,r0", "movem p:(r0),x0", "movem p:>$0101,x1"], 100),
    # MOVEC matrix beyond lc/la
    ("movec_sz_vba_read", ["movec #>$00cafe,sz", "movec sz,x0", "movec vba,x1", "movec #>$000000,sz"], 100),
    ("movec_sr_roundtrip", ["movec sr,y0", "movec #>$c00301,sr", "movec sr,y1", "movec #>$c00300,sr"], 100),
    ("movec_mem_forms", ["movec #>$00beef,lc", "movec lc,x:$02b0", "move #>$02b1,r1", "movec #>$00f00d,la", "movec la,x:(r1)+", "movec x:$02b0,ep", "movec #>$000000,ep", "movec #>$ffffff,la", "movec #>$000000,lc"], 100),
    # --- authored in bank b4 ---
    # movec aa short form (address <= $3F) both directions
    ("movec_aa_short", ["movec #>$00abcd,lc", "movec lc,x:$27", "movec x:$27,la", "movec la,y:$28", "movec y:$28,lc", "movec lc,x1", "movec #>$ffffff,la", "movec #>$0,lc"], 100),
    # movem P-space writes to the never-executed scratch past the snapshot
    ("movem_p_write", ["move #>$abc001,x0", "move #>$0,x1", "movem x0,p:>$07c0", "movem p:>$07c0,x1", "move #>$07c1,r1", "move #>$def002,y0", "move #>$0,y1", "movem y0,p:(r1)", "movem p:(r1),y1"], 100),
    # a2/b2 store convention: silicon sign-extends the 8-bit register on read
    ("acc_ext_store", ["clr a", "move #>$0000ff,x0", "move x0,a2", "move a2,x:$37", "move x:$37,y0", "clr b", "move #>$00007f,x1", "move x1,b2", "move b2,x:$38", "move x:$38,y1"], 100),
    # L flag conditions (jls/jlc + tls/tlc) via limiting
    ("l_flag_conditions", ["clr a", "move #>$7fffff,x0", "move x0,a1", "asl a", "move a,y0", "move #>$111111,x1", "clr b", "tls x1,b", "jls <lbl_lf1", "move #>$bad210,y1", "lbl_lf1: andi #$00,ccr", "clr a", "move a,y1", "tlc x1,b", "jlc <lbl_lf2", "move #>$bad211,y1", "lbl_lf2: nop"], 100),
    # --- authored in bank b6 ---
    # movem aa-form roundtrips (write-then-read per the rules note).
    ("movem_aa_roundtrip", ["move #>$00abc1,x0", "movem x0,p:$20",
                            "movem p:$20,y0", "move #>$00def2,y1",
                            "movem y1,p:$3f", "movem p:$3f,x1"], 100),
    # L-register stores not yet hardware-validated: b10, y, b (limited), ba.
    ("l_store_gap", ["clr b", "move #>$616263,x0", "move x0,b1",
                     "move #>$717273,x1", "move x1,b0",
                     "move #>$818283,y0", "move #>$919293,y1",
                     "clr a", "move #>$0a0a0a,r0", "move r0,a1",
                     "move b10,l:$140", "move y,l:$141", "move b,l:$142",
                     "move ba,l:$144",
                     "move l:$140,x", "move l:$141,a10"], 100),
    # Limited B store with a value the limiter must saturate.
    ("l_store_b_limited_sat", ["clr b", "move #>$7fffff,x0", "move x0,b1",
                               "asl b", "move b,l:$146",
                               "move l:$146,y"], 100),
    # L loads into the pair registers (write_l_reg paths).
    ("l_load_pairs", ["move #>$0a0b0c,x0", "move x0,x:$150",
                      "move #>$1a1b1c,x1", "move x1,y:$150",
                      "clr a", "clr b",
                      "move l:$150,a10", "move l:$150,b10"], 100),
    ("l_load_ab_ba", ["move #>$2a2b2c,x0", "move x0,x:$151",
                      "move #>$3a3b3c,x1", "move x1,y:$151",
                      "move #>$4a4b4c,y0", "move y0,x:$152",
                      "move #>$5a5b5c,y1", "move y1,y:$152",
                      "clr a", "clr b",
                      "move l:$151,ab", "move a1,x0", "move b1,y0",
                      "move l:$152,ba"], 100),
    # Short-aa L forms and parallel-ALU L moves.
    ("l_short_parallel", ["move #>$0c0d0e,x0", "move x0,x:$31",
                          "move #>$1c1d1e,y1", "move y1,y:$31",
                          "clr a", "move #>$000001,x1",
                          "move l:$31,y",
                          "move #>$32,r5",
                          "add x1,a  a,l:(r5)",
                          "sub x1,a  l:(r5),b",
                          "move x,l:$30"], 100),
    # --- authored in bank b8 ---
    # SP write path (never emitted by the JIT before) + 0/15 boundary
    # values. Wedge-suspect: probe-first, keep at the front of the bank.
    ("movec_sp_bounds", ["move #>$000002,x0", "movec x0,sp", "movec sp,y0", "movec sc,y1", "movec #$f,sp", "movec sp,x1", "movec #$0,sp", "movec sc,r4"], 100),
    # VBA write + readback; restore the silicon boot value ($FF0000).
    ("movec_vba_rw", ["move #>$123400,x0", "movec x0,vba", "movec vba,y0", "movec #>$ff0000,vba"], 100),
    # MOVEC read direction for registers only ever written before.
    ("movec_read_dir", ["movec m0,x0", "movec #>$001234,ep", "movec ep,x1", "movec #>$000000,ep", "movec omr,y0"], 100),
    # Negative-side limiting unscaled + exact-rail reads on both sides.
    ("limit_rails", ["clr a", "move #>$7fffff,x0", "move x0,a1", "move #>$ffffff,x1", "move x1,a0", "move a,x1", "movec sr,r4", "clr b", "move #>$ffffff,y0", "move y0,b2", "move #>$800000,y1", "move y1,b1", "move b,y1", "movec sr,r5", "clr b", "move #>$ffffff,y0", "move y0,b2", "move #>$123456,y0", "move y0,b1", "move b,y0", "movec sr,r6"], 100),
    # --- authored in bank b12 ---
    # MOVEM register-class fan-out: control regs, M regs, and
    # accumulators through P space (only x0/x1/y0/y1 ever moved before).
    ("movem_class_fanout", ["movec #>$00abc9,la", "movem la,p:>$7c8", "movem p:>$7c8,lc", "movec lc,x1", "movem sr,p:>$7ca", "movem p:>$7ca,y1", "movem m2,p:>$7cb", "movem p:>$7cb,n4", "clr a", "move #>$123456,x0", "move x0,a1", "movem a,p:>$7cc", "movem p:>$7cc,b", "movec #>$ffffff,la", "movec #>$0,lc"], 100),
    # MOVEM P-space EA update modes (only abs/(Rn) before).
    ("movem_ea_updates", ["move #>$07c9,r2", "move #>$000002,n2", "move #>$abc123,x0", "movem x0,p:(r2)+", "movem p:-(r2),x1", "movem x0,p:(r2)+n2", "move r2,n5"], 100),
    # MOVEC through Y space (entirely untested) with update modes.
    ("movec_y_ea_forms", ["movec #>$00feed,lc", "move #>$0212,r0", "movec lc,y:(r0)-", "move r0,n4", "move #>$0212,r2", "movec y:(r2)+,la", "movec la,x1", "move r2,n6", "movec #>$ffffff,la", "movec #>$0,lc"], 100),
    # MOVEC X-space pre-decrement write + indexed read + sc store.
    ("movec_x_update_forms", ["movec #>$00beef,la", "move #>$0216,r1", "movec la,x:-(r1)", "move r1,n5", "move #>$0215,r3", "move #>$000000,n3", "movec x:(r3+n3),lc", "movec lc,y1", "movec sc,x:$2a", "move x:$2a,y0", "movec #>$ffffff,la", "movec #>$0,lc"], 100),
    # MOVEC register-form SR write (only the immediate form ever wrote
    # SR) + m2-m7 reads.
    ("movec_reg_sr_write", ["movec sr,x1", "ori #$0f,ccr", "movec x1,sr", "movec sr,r4", "movec m3,y1", "movec m6,n4"], 100),
    # MOVEC short-immediate destinations beyond ssh/m/sp (zero-extend
    # per destination class).
    ("movec_short_imm_dests", ["movec #$3f,lc", "movec lc,x1", "movec #$12,sz", "movec sz,y1", "movec #$21,ep", "movec ep,y0", "movec #$05,la", "movec la,n4", "movec #>$ffffff,la", "movec #>$0,lc", "movec #>$0,sz", "movec #>$0,ep"], 100),
    # Long-displacement move in X space (the reclaimed jclr-collision
    # code points - only the Y flavor is silicon-validated).
    ("mld_x_space", ["move #$100,r1", "move #>$abc321,x0", "move x0,x:(r1+$208)", "move x:(r1+$208),y1", "move x:(r1-$4),y0"], 100),
    # Short-displacement move with accumulator operands (limiting on
    # write, sign path on read).
    ("msd_acc_numreg", ["move #>$0219,r2", "clr a", "move #>$7fffff,x0", "move x0,a1", "move #>$ffffff,x1", "move x1,a0", "move a,x:(r2+3)", "move x:(r2+3),y0", "clr b", "move x:(r2+3),b", "move b1,n4"], 100),
]

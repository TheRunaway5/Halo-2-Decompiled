"""Address generation: update modes, modulo, bit-reverse, LUA/LRA.

Case bodies are verbatim from the original bank lists; bank membership
and order live in banks.py. See registry.py for the authoring rules.
"""

THEME = "agu"

# (name, [body lines], max_steps)
CASES = [
    # --- authored in bank b0 ---

    # --- AGU / moves ---
    ("agu_postinc", ["move #$10,r0", "move #>$aaaaaa,x0", "move x0,x:(r0)+", "move #>$bbbbbb,x0", "move x0,x:(r0)+", "move #$10,r1", "move x:(r1)+,y0", "move x:(r1)+,y1"], 100),
    ("agu_postdec_predec", ["move #$20,r2", "move #>$111111,x0", "move x0,x:(r2)-", "move #$22,r3", "move x:-(r3),x1"], 100),
    ("agu_offset_n", ["move #$30,r4", "move #$3,n4", "move #>$777777,x0", "move x0,x:(r4)+n4", "move #$33,r5", "move #$3,n5", "move x:(r5)-n5,y0"], 100),
    ("agu_indexed", ["move #$40,r6", "move #$5,n6", "move #>$123123,x0", "move x0,x:(r6+n6)"], 100),
    # M-register writes staged through a data register to sidestep the
    # assembler's broken plain `move #imm,mN` spelling (see README findings);
    # can be simplified to `movec #imm,mN` once the assembler is fixed.
    ("agu_modulo", ["move #>$000004,x1", "movec x1,m0", "move #$50,r0", "move #>$111111,x0", "move x0,x:(r0)+", "move x0,x:(r0)+", "move x0,x:(r0)+", "move x0,x:(r0)+", "move x0,x:(r0)+", "move #>$ffffff,x1", "movec x1,m0"], 100),
    ("agu_bitrev", ["move #>$000000,y1", "movec y1,m1", "move #$60,r1", "move #$4,n1", "move x0,x:(r1)+n1", "move x0,x:(r1)+n1", "move #>$ffffff,y1", "movec y1,m1"], 100),
    ("lua_basic", ["move #$70,r7", "move #$2,n7", "lua (r7)+n7,n0", "lua (r7)-,r2"], 100),
    # --- authored in bank b1 ---
    ("addr_negative_shift", ["clr a", "move #>$800000,x0", "move x0,a2", "clr b", "move #>$100000,x1", "move x1,b1", "addr b,a"], 100),
    # --- authored in bank b2 ---
    # LUA data-register destinations (JIT paths fixed, no hw goldens)
    ("lua_data_reg_dest", ["move #>$0203,r0", "lua (r0)+,x0", "move #>$030f,r1", "move #>$4,n1", "lua (r1)+n1,y1"], 100),
    ("lua_accumulator_dest", ["move #>$876542,r2", "clr b", "lua (r2)+,a", "move #>$001234,r3", "lua (r3)-,b"], 100),
    ("lua_rel_negative", ["move #>$100,r1", "lua (r1-2),n0", "lua (r1+$35),r4"], 100),
    ("lra_forms", ["move #>$50,r0", "lra r0,n2", "lra $0155,r5"], 100),
    # Modulo addressing edges
    ("agu_mod_negative_step", ["movec #$7,m0", "move #>$0248,r0", "move #>$fffffd,n0", "move #>$111aaa,x0", "move x0,x:$024e", "move x:(r0)+n0,x1", "movec #>$ffffff,m0"], 100),
    ("agu_mod_transient", ["movec #$7,m1", "move #>$0255,r1", "move #>$fffffd,n1", "move #>$222bbb,x0", "move x0,x:$0252", "move x:(r1+n1),y0", "movec #>$ffffff,m1"], 100),
    ("agu_mod_large_step", ["movec #$7,m2", "move #>$0263,r2", "move #>$8,n2", "move #>$333ccc,x0", "move x0,x:$0263", "move x:(r2)+n2,x1", "movec #>$ffffff,m2"], 100),
    ("agu_mod_nonpow2", ["movec #$5,m3", "move #>$0268,r3", "move #>$444ddd,x0", "move x0,x:$0268", "move x:(r3)+,y1", "move x:(r3)+,y1", "move x:(r3)+,y1", "move x:(r3)+,y1", "move x:(r3)+,y1", "move x:(r3)+,y1", "movec #>$ffffff,m3"], 100),
    ("agu_mod_min_m1", ["movec #$1,m4", "move #>$0270,r4", "move #>$555eee,x0", "move x0,x:$0270", "move x:(r4)+,x1", "move x:(r4)+,x1", "move x:(r4)+,x1", "movec #>$ffffff,m4"], 100),
    ("agu_mod_predec_wrap", ["movec #$7,m5", "move #>$0278,r5", "move #>$666fff,x0", "move x0,x:$027f", "move x:-(r5),y0", "movec #>$ffffff,m5"], 100),
    ("agu_multiwrap_mod2", ["movec #>$008001,m6", "move #>$0281,r6", "move #>$777abc,x0", "move x0,x:$0281", "move x:(r6)+,x1", "movec #>$ffffff,m6"], 100),
    ("agu_multiwrap_mod4", ["movec #>$008003,m7", "move #>$0286,r7", "move #>$3,n7", "move #>$888def,x0", "move x0,x:$0286", "move x:(r7)+n7,y1", "movec #>$ffffff,m7"], 100),
    # Bit-reverse addressing
    ("agu_bitrev_walk", ["movec #$0,m0", "move #>$0,r0", "move #>$8,n0", "move #>$0290,r1", "lua (r0)+n0,r2", "move r0,x:$0290", "move x:(r0)+n0,x0", "move r0,x:$0291", "move x:(r0)+n0,x0", "move r0,n1", "movec #>$ffffff,m0"], 100),
    ("agu_bitrev_highbits", ["movec #$0,m2", "move #>$000ff0,r2", "move #>$8,n2", "lua (r2)+n2,r3", "movec #>$ffffff,m2"], 100),
    # Linear wraparound
    ("agu_linear_wrap24", ["move #>$fffffe,r4", "move #>$3,n4", "lua (r4)+n4,n0", "move #>$000001,r5", "move #>$fffffe,n5", "lua (r5)+n5,n1"], 100),
    # --- authored in bank b8 ---
    # M=0 (bit-reverse mode) with plain +/- updates: semantics unprobed,
    # LUA-only so no memory access rides on the result. Probe-first.
    ("lua_m0_plain", ["movec #$0,m0", "move #>$001234,r0", "lua (r0)+,r1", "lua (r0)-,r2", "movec #>$ffffff,m0"], 100),
    # Bit-reverse with N=0 (revbits=24 corner).
    ("agu_bitrev_n0", ["movec #$0,m1", "move #>$000f0f,r1", "move #>$000000,n1", "lua (r1)+n1,r3", "movec #>$ffffff,m1"], 100),
    # Multi-wrap modulo where the step forces a pre-adjustment wrap.
    ("agu_multiwrap_preadj", ["movec #>$008003,m3", "move #>$000102,r3", "move #>$00000e,n3", "lua (r3)+n3,r4", "movec #>$ffffff,m3"], 100),
    # LUA (rn)-n: the one plain-update LUA form with no prior coverage.
    ("lua_minus_n", ["move #>$001280,r4", "move #>$000005,n4", "lua (r4)-n4,r5"], 100),
    # --- authored in bank b11 ---
    # Standard modulo with N > modulo (non-multiple): the emulator's
    # bufsize-stepping loops never executed. LUA-only; probe-first.
    ("agu_mod_n_gt_m", ["movec #>$000005,m0", "move #>$000242,r0", "move #>$000014,n0", "lua (r0)+n0,r1", "lua (r0)-n0,r2", "movec #>$ffffff,m0"], 100),
    # Standard modulo with the pointer initially OUTSIDE the buffer
    # (offset >= modulus for a non-power-of-2 modulo). Probe-first.
    ("agu_mod_ptr_outside", ["movec #>$000005,m1", "move #>$000247,r1", "lua (r1)+,r3", "lua (r1)-,r6", "movec #>$ffffff,m1"], 100),
    # Bit-reverse mode (Rn)-Nn with |N| > 1: the emulator discards the
    # update's sign (walks like +Nn). LUA-only; probe-first.
    ("lua_m0_minus_n", ["movec #$0,m2", "move #>$001234,r2", "move #>$000008,n2", "lua (r2)-n2,r5", "move #>$000003,n2", "lua (r2)-n2,r6", "lua (r2)+n2,r7", "movec #>$ffffff,m2"], 100),
    # M=0 post-decrement through a real memory access (address = old Rn,
    # safe; the update lands in the snapshot via r3).
    ("m0_postdec_move", ["movec #$0,m3", "move #>$000250,r3", "move #>$99aabb,x0", "move x0,x:$0250", "move x:(r3)-,x1", "move r3,n4", "movec #>$ffffff,m3"], 100),
    # Reserved M-register values: $4000-$7FFF band, $C000-$FFFE band,
    # and multi-wrap-shaped values with bits above 15 set. The emulator
    # treats them as modulo / freeze / bits-23:16-don't-care - all
    # conjecture. LUA-only; probe-first.
    ("agu_reserved_m", ["movec #>$004000,m5", "move #>$001234,r5", "move #>$000003,n5", "lua (r5)+n5,r6", "movec #>$007fff,m5", "lua (r5)+n5,r7", "movec #>$00c000,m5", "lua (r5)+n5,n4", "movec #>$00fffe,m5", "lua (r5)+n5,n6", "movec #>$018000,m5", "lua (r5)+n5,n7", "movec #>$010003,m5", "lua (r5)+n5,r4", "movec #>$ffffff,m5"], 100),
    # --- authored in bank b22 ---
    # A REP'd memory write whose pointer walks under non-linear
    # addressing. The address is recomputed per iteration from Rn, so
    # only the stored words distinguish a pointer that advanced from
    # one that stayed put - Rn itself ends correct either way, and the
    # corpus had no case that wrote memory from inside a REP under a
    # non-linear M. Needs --dump-mem to see anything.
    # Modulo 4 over Y:$0380-$0383, walking down from $0382 and wrapping.
    ("rep_mod_store", ["movec #>$000003,m5", "move #>$000382,r5", "clr a", "move #>$5a5a5a,x0", "move x0,a1", "rep #3", "move a,y:(r5)-", "move r5,n0", "move y:$0380,x1", "move y:$0381,y0", "move y:$0382,y1", "move y:$0383,n1", "movec #>$ffffff,m5"], 200),
    # Same shape under reverse-carry (M=0) with N=8: offsets 0, 8, 4, 12
    # from Y:$03a0, which is the FFT butterfly walk.
    ("rep_bitrev_store", ["movec #>$000000,m6", "move #>$0003a0,r6", "move #>$000008,n6", "clr b", "move #>$3c3c3c,x1", "move x1,b1", "rep #4", "move b,y:(r6)+n6", "move r6,n2", "move y:$03a0,x0", "move y:$03a4,y0", "move y:$03a8,y1", "move y:$03ac,n3", "movec #>$ffffff,m6"], 200),
    # The same walk driven by an inline-able DO body rather than REP,
    # since the two take different arms of the loop emitter.
    ("do_mod_store", ["movec #>$000003,m7", "move #>$0003c1,r7", "clr a", "move #>$0d0d0d,x0", "move x0,a1", "do #3,lbl_dms", "move a,x:(r7)+", "lbl_dms: nop", "move r7,n4", "move x:$03c0,x1", "move x:$03c1,y0", "move x:$03c2,y1", "move x:$03c3,n5", "movec #>$ffffff,m7"], 400),
]

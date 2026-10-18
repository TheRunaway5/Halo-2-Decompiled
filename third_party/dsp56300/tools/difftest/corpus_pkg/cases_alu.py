"""Arithmetic: add/sub families, abs/neg, cmp, rounding, div, tfr.

Case bodies are verbatim from the original bank lists; bank membership
and order live in banks.py. See registry.py for the authoring rules.
"""

THEME = "alu"

# (name, [body lines], max_steps)
CASES = [
    # --- authored in bank b0 ---
    ("add_reg", ["move #>$000001,x0", "clr a", "add x0,a", "add x0,a"], 100),
    ("add_carry", ["move #>$ffffff,x0", "clr a", "move x0,a1", "add x0,a"], 100),
    ("add_overflow", ["move #>$7fffff,x0", "clr a", "move x0,a1", "add x0,a"], 100),
    ("sub_borrow", ["clr a", "move #>$000001,x0", "sub x0,a"], 100),
    ("addl_subl", ["move #>$111111,x0", "clr a", "move x0,a1", "clr b", "move #>$222222,x1", "move x1,b1", "addl b,a", "subl a,b"], 100),
    ("addr_subr", ["move #>$400000,x0", "clr a", "move x0,a1", "clr b", "move #>$100000,x1", "move x1,b1", "addr b,a", "subr a,b"], 100),
    ("adc_sbc", ["move #>$ffffff,x0", "clr a", "move x0,a1", "move #>$000001,x1", "add x1,a", "clr b", "move x1,x0", "adc x,b"], 100),
    ("abs_neg", ["move #>$800001,x0", "clr a", "move x0,a1", "abs a", "neg a"], 100),
    ("cmp_flags", ["move #>$000005,x0", "clr a", "move #>$000003,x1", "move x1,a1", "cmp x0,a"], 100),
    ("cmpm_cmpu", ["move #>$800000,x0", "clr a", "move #>$400000,x1", "move x1,a1", "cmpm x0,a", "cmpu x0,a"], 100),
    ("tst_zero", ["clr a", "tst a"], 100),
    ("max_maxm", ["clr a", "move #>$100000,x0", "move x0,a1", "clr b", "move #>$200000,x1", "move x1,b1", "max a,b"], 100),
    ("rnd_basic", ["clr a", "move #>$123456,x0", "move x0,a1", "move #>$800000,x1", "move x1,a0", "rnd a"], 100),
    ("inc_dec_acc", ["clr a", "inc a", "inc a", "dec a"], 100),
    ("clb_norm", ["clr a", "move #>$000123,x0", "move x0,a1", "clb a,b"], 100),
    ("extract_insert", ["move #>$00050c,x0", "clr a", "move #>$abcdef,x1", "move x1,a1", "extractu x0,a,b"], 100),
    # --- authored in bank b1 ---
    # ALU carry/borrow/overflow edges
    ("add_carry_out48", ["move #>$7fffff,x0", "clr a", "move x0,a1", "move #>$000001,x1", "add x1,a", "add x0,a"], 100),
    ("sub_borrow_deep", ["clr a", "move #>$000001,x0", "move x0,a0", "move #>$000002,x1", "sub x1,a"], 100),
    ("adc_carry_chain", ["move #>$ffffff,x0", "clr a", "move x0,a1", "move x0,a0", "move #>$000001,x1", "move #>$000000,y0", "add x1,a", "clr b", "move #>$100000,x0", "move #>$080000,x1", "adc x,b"], 100),
    ("sbc_borrow_in", ["move #>$ffffff,x0", "clr a", "move x0,a1", "move #>$000001,x1", "add x1,a", "clr b", "move #>$500000,y1", "move y1,b1", "move #>$100000,x1", "move #>$080000,x0", "sbc x,b"], 100),
    ("abs_min_negative", ["clr a", "move #>$800000,x0", "move x0,a2", "abs a"], 100),
    ("neg_min_negative", ["clr a", "move #>$800000,x0", "move x0,a2", "neg a"], 100),
    ("addl_carry_xor", ["clr a", "move #>$c00000,x0", "move x0,a1", "clr b", "move #>$100000,x1", "move x1,b1", "addl b,a"], 100),
    ("subl_edge", ["clr a", "move #>$300000,x0", "move x0,a1", "clr b", "move #>$100000,x1", "move x1,b1", "subl b,a"], 100),
    ("tfr_acc_and_reg", ["move #>$123456,x0", "clr b", "tfr x0,a", "tfr a,b"], 100),
    # Rounding
    ("rnd_tie_even", ["clr a", "move #>$400000,x0", "move x0,a1", "move #>$800000,x1", "move x1,a0", "rnd a", "clr b", "move #>$400001,x0", "move x0,b1", "move x1,b0", "rnd b"], 100),
    ("rnd_tie_above", ["clr a", "move #>$400000,x0", "move x0,a1", "move #>$800001,x1", "move x1,a0", "rnd a"], 100),
    ("rnd_twos_complement", ["movec #>$e00300,sr", "clr a", "move #>$400000,x0", "move x0,a1", "move #>$800000,x1", "move x1,a0", "rnd a", "movec #>$c00300,sr"], 100),
    # Max / compares
    ("max_no_transfer", ["clr a", "move #>$300000,x0", "move x0,a1", "clr b", "move #>$100000,x1", "move x1,b1", "max a,b"], 100),
    ("maxm_magnitude", ["clr a", "move #>$900000,x0", "move x0,a1", "clr b", "move #>$200000,x1", "move x1,b1", "maxm a,b"], 100),
    ("cmp_imm_forms", ["clr a", "move #>$000030,x0", "move x0,a1", "cmp #$30,a", "cmp #>$000031,a"], 100),
    ("add_imm_short", ["clr a", "add #$3f,a", "sub #$01,a"], 100),
    # CLB / NORM / NORMF
    ("clb_positive_small", ["clr a", "move #>$010000,x0", "move x0,a1", "clb a,b"], 100),
    ("clb_negative", ["clr a", "move #>$ffffff,x0", "move x0,a2", "move #>$f00000,x1", "move x1,a1", "clb a,b"], 100),
    ("clb_zero_acc", ["clr a", "clb a,b"], 100),
    ("norm_iterations", ["move #>$000000,r1", "clr a", "move #>$000123,x0", "move x0,a1", "norm r1,a", "norm r1,a", "norm r1,a"], 100),
    ("normf_shift", ["clr a", "move #>$001000,x0", "move x0,a1", "clb a,b", "normf b1,a"], 100),
    # EXTRACT / INSERT / MERGE
    ("extract_signed_field", ["move #>$00082f,x0", "clr a", "move #>$abcdef,x1", "move x1,a1", "extract x0,a,b"], 100),
    ("extract_one_bit", ["move #>$00012f,x0", "clr a", "move #>$800000,x1", "move x1,a1", "extract x0,a,b"], 100),
    ("extractu_imm_control", ["clr a", "move #>$fedcba,x1", "move x1,a1", "extractu #$00080c,a,b"], 100),
    ("insert_preserve", ["move #>$000008,y0", "clr a", "move #>$ffff00,x1", "move x1,a1", "move #>$0000ab,x0", "insert #$000800,x0,a"], 100),
    ("insert_reg_control", ["move #>$00080c,y1", "clr b", "move #>$111111,x1", "move x1,b1", "move #>$0000ff,x0", "insert y1,x0,b"], 100),
    ("merge_fields", ["move #>$000abc,x0", "clr a", "move #>$123456,x1", "move x1,a1", "merge x0,a", "move #>$000800,y0", "clr b", "merge y0,b"], 100),
    # DIV
    ("div_iterations", ["clr a", "move #>$100000,x1", "move x1,a1", "move #>$600000,x0", "div x0,a", "div x0,a", "div x0,a", "div x0,a"], 100),
    ("div_negative", ["clr a", "move #>$e00000,x0", "move x0,a1", "move #>$400000,y0", "div y0,a", "div y0,a"], 100),
    # --- authored in bank b4 ---
    # ALU immediate entry forms not yet covered
    ("alu_imm_entries", ["clr a", "add #>$123456,a", "sub #>$000456,a", "and #$3c,a", "or #$05,a", "eor #$3f,a"], 100),
    ("extract_imm_control", ["clr a", "move #>$abcdef,x1", "move x1,a1", "extract #$00080c,a,b"], 100),
    # ADDL/SUBL carry-edge matrix: C comes from the add/sub stage only;
    # the destination shift's carry-out is ignored (silicon-verified;
    # diverges from the XOR rule sim56300 exhibits). Each edge
    # dumps C via a rol into a distinct register bit trail.
    ("addl_carry_edges", ["clr a", "clr b", "move #>$000001,x0", "move x0,a0", "move #>$800000,x1", "move x1,b2", "addl a,b", "movec sr,y0", "clr a", "clr b", "move #>$ffffff,x0", "move x0,a2", "move x0,a1", "move x0,a0", "move #>$200000,x1", "move x1,b2", "addl a,b", "movec sr,y1"], 100),
    ("subl_carry_edges", ["clr a", "clr b", "move #>$800000,x0", "move x0,b2", "subl a,b", "movec sr,x1", "clr a", "clr b", "move #>$000001,x0", "move x0,a0", "subl a,b", "movec sr,y1"], 100),
    # --- authored in bank b8 ---
    # ADC/SBC with carry-in = 0 (every prior case forced C=1) and a
    # negative 48-bit X operand.
    ("adc_sbc_c0", ["move #>$ffffff,x1", "move #>$fffffe,x0", "clr b", "move #>$000005,y0", "move y0,b1", "andi #$00,ccr", "adc x,b", "movec sr,r4", "clr a", "move #>$000007,y1", "move y1,a1", "andi #$00,ccr", "sbc x,a", "movec sr,r5"], 100),
    # CMPU accumulator sources and the remaining ggg register codes.
    ("cmpu_forms", ["clr a", "move #>$800000,x0", "move x0,a1", "clr b", "move #>$400000,x1", "move x1,b1", "cmpu a,b", "movec sr,r4", "cmpu b,a", "movec sr,r5", "cmpu x1,a", "movec sr,r6", "move #>$c00000,y0", "cmpu y0,b", "movec sr,r7", "move #>$000001,y1", "cmpu y1,b", "movec sr,n4"], 100),
    # --- authored in bank b11 ---
    # Bit-field ops with REAL widths: every earlier extract/insert case
    # mis-packed the width into control bits 11:8 (hw+emu read 17:12),
    # so the whole non-degenerate datapath was silicon-unpinned.
    # Control word = width<<12 | offset.
    ("bf_extractu_wide", ["clr a", "move #>$abcdef,x0", "move x0,a1", "move #>$654321,x1", "move x1,a0", "extractu #$00800c,a,b", "movec sr,r4", "extractu #$018000,a,b", "movec sr,r5"], 100),
    ("bf_extract_signed", ["clr a", "move #>$8bcdef,x0", "move x0,a1", "move #>$654321,x1", "move x1,a0", "extract #$00802f,a,b", "movec sr,r4", "extract #$001000,a,b", "movec sr,r5"], 100),
    ("bf_extract_reg_ctl", ["move #>$00800c,x0", "clr a", "move #>$fedcba,x1", "move x1,a1", "extract x0,a,b", "movec sr,r4", "move #>$00c018,y0", "extractu y0,a,b", "movec sr,r5"], 100),
    ("bf_insert_real", ["move #>$0000ab,x0", "clr a", "move #>$ffff00,x1", "move x1,a1", "move #>$111111,y0", "move y0,a0", "insert #$00800c,x0,a", "movec sr,r4", "move #>$005014,y1", "move #>$00000f,x1", "insert y1,x1,a", "movec sr,r5"], 100),
    # Out-of-range field composition (offset+width > 56): undefined per
    # manual; probe-first.
    ("bf_extract_overrange", ["clr a", "move #>$abcdef,x0", "move x0,a1", "extractu #$00803c,a,b", "movec sr,r4", "extract #$008038,a,b", "movec sr,r5"], 100),
    # Immediate-ALU family with B as destination (never encoded before)
    # plus inc b. Short forms kept <= $3F so the short rows stay pinned.
    ("imm_alu_b_dest", ["clr b", "move #>$7ffff0,x0", "move x0,b1", "add #$3f,b", "movec sr,r4", "cmp #$5,b", "sub #$3,b", "inc b", "and #>$f0f00f,b", "or #$12,b", "eor #>$123456,b", "add #>$123456,b", "movec sr,r5"], 100),
    # DIV with a negative divisor (both dividend signs) - the
    # D[55]^S[23] decision with S[23]=1 was never exercised - plus a
    # forced carry seed before the first step.
    ("div_sign_matrix", ["clr a", "move #>$100000,x1", "move x1,a1", "move #>$a00000,x0", "ori #$01,ccr", "div x0,a", "movec sr,r4", "div x0,a", "movec sr,r5", "clr b", "move #>$ffffff,y0", "move y0,b2", "move #>$e00000,y1", "move y1,b1", "div x0,b", "movec sr,r6", "div x0,b", "movec sr,r7"], 100),
    ("div_src_dest", ["clr b", "move #>$200000,x0", "move x0,b1", "move #>$600000,x1", "div x1,b", "movec sr,r4", "clr a", "move #>$180000,y0", "move y0,a1", "move #>$500000,y1", "div y1,a", "div y1,a", "movec sr,r5"], 100),
    # NORM E=1 right-shift arm + already-normalized terminal state.
    ("norm_e_path", ["move #>$000000,r1", "clr a", "move #>$c00000,x0", "move x0,a1", "tst a", "norm r1,a", "movec sr,r4", "move r1,n4", "clr b", "move #>$400000,x1", "move x1,b1", "move #>$000010,r2", "tst b", "norm r2,b", "movec sr,r5", "move r2,n5"], 200),
    # NORMF: ASR arm (positive count), ASL arm with sign change (V),
    # and count 0.
    ("normf_arms", ["clr a", "move #>$400000,x0", "move x0,a1", "move #>$000005,x1", "normf x1,a", "movec sr,r4", "clr b", "move #>$300000,y0", "move y0,b1", "move #>$fffffe,y1", "normf y1,b", "movec sr,r5", "move #>$000000,y1", "normf y1,b", "movec sr,r6"], 100),
    # MAX/MAXM: exactly-equal operands (the <= vs < discriminator) and
    # an opposite-sign magnitude tie.
    ("max_equal_ties", ["clr a", "move #>$123456,x0", "move x0,a1", "clr b", "move x0,b1", "max a,b", "movec sr,r4", "clr a", "move #>$200000,x1", "move x1,a1", "tfr a,b", "neg b", "maxm a,b", "movec sr,r5"], 100),
    # CMPU with equal low 48 bits but different extensions (proves the
    # extension bytes are ignored).
    ("cmpu_ext_ignored", ["clr a", "move #>$345678,x0", "move x0,a1", "clr b", "move x0,b1", "move #>$0000ff,x1", "move x1,b2", "cmpu a,b", "movec sr,r4", "cmpu b,a", "movec sr,r5"], 100),
    # CLB edge values: all-ones accumulator and +1 in a0 only.
    ("clb_edges", ["clr a", "move #>$ffffff,x0", "move x0,a2", "move x0,a1", "move x0,a0", "clb a,b", "move b1,r4", "clr a", "move #>$000001,x1", "move x1,a0", "clb a,b", "move b1,r5"], 100),
    # RND just below the tie (a0=$7FFFFF must not round up).
    ("rnd_tie_below", ["clr a", "move #>$123456,x0", "move x0,a1", "move #>$7fffff,x1", "move x1,a0", "rnd a", "movec sr,r4"], 100),
    # INC crossing zero from -1 (Z and C together) and DEC from 0.
    ("inc_dec_cross_zero", ["clr a", "move #>$ffffff,x0", "move x0,a2", "move x0,a1", "move x0,a0", "inc a", "movec sr,r4", "clr b", "dec b", "movec sr,r5"], 100),
    # ADC edges: X=-1 + C=1 into zero acc (result 0, Z with carry);
    # carry generated by the +C increment alone; X negative with C=1.
    ("adc_sbc_edges2", ["move #>$ffffff,x1", "move #>$ffffff,x0", "clr b", "ori #$01,ccr", "adc x,b", "movec sr,r4", "clr a", "move #>$ffffff,y0", "move y0,a2", "move y0,a1", "move y0,a0", "move #>$000000,x1", "move #>$000000,x0", "ori #$01,ccr", "adc x,a", "movec sr,r5", "move #>$ffffff,x1", "move #>$fffffe,x0", "clr b", "move #>$000005,y1", "move y1,b1", "ori #$01,ccr", "adc x,b", "movec sr,r6"], 100),
    # -1.0 squared through the 2-word immediate multiply path.
    ("mpyi_neg1_sq", ["move #>$800000,y1", "mpyi #>$800000,y1,a", "movec sr,r4", "maci #>$800000,y1,a", "movec sr,r5"], 100),
    # MPYI/MACI x-side sources and B destination.
    ("mpyi_src_dest", ["move #>$345678,x0", "move #>$456789,x1", "move #>$222222,y0", "mpyi #>$120000,x0,b", "maci #>$003456,x1,b", "movec sr,r4", "mpyi #>$654321,y0,b", "movec sr,r5"], 100),
    # MERGE width discriminator: the manual's operation line says
    # S[7:0], the description/example use 12 bits; garbage above bit 11
    # separates the readings (and pins upper-bit masking).
    ("merge_width_edges", ["move #>$ffffff,x0", "clr a", "move #>$123456,x1", "move x1,a1", "merge x0,a", "movec sr,r4", "move #>$000fab,y0", "clr b", "merge y0,b", "movec sr,r5"], 100),
]

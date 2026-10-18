"""MPY/MAC families, immediate and shifted forms, DMAC.

Case bodies are verbatim from the original bank lists; bank membership
and order live in banks.py. See registry.py for the authoring rules.
"""

THEME = "multiply"

# (name, [body lines], max_steps)
CASES = [
    # --- authored in bank b0 ---

    # --- Multiply ---
    ("mpy_basic", ["move #>$400000,x0", "move #>$400000,y0", "mpy x0,y0,a"], 100),
    ("mpy_neg", ["move #>$c00000,x0", "move #>$400000,y0", "mpy x0,y0,a", "mpy -x0,y0,b"], 100),
    ("mac_macr", ["move #>$200000,x0", "move #>$400000,y0", "clr a", "mac x0,y0,a", "mac x0,y0,a", "macr x0,y0,a"], 100),
    ("mpyi_maci", ["move #>$100000,y1", "mpyi #>$300000,y1,a", "maci #>$100000,y1,a"], 100),
    ("mpy_su_uu", ["move #>$800001,x0", "move #>$800001,y0", "mpyuu x0,y0,a", "mpysu x0,y0,b"], 100),
    ("dmac_ss", ["move #>$400000,x0", "move #>$200000,y0", "mpy x0,y0,a", "dmacss x0,y0,a"], 100),
    # --- authored in bank b1 ---
    # Multiply family
    ("mpy_all_qq_pairs", ["move #>$400000,x0", "move #>$200000,x1", "move #>$100000,y0", "move #>$080000,y1", "mpy x1,y0,a", "mpy y1,y0,b", "mpy x0,y1,a", "mpy y0,x0,b"], 100),
    ("mpy_square_forms", ["move #>$c00000,x0", "move #>$300000,y1", "mpy x0,x0,a", "mpy y1,x1,b"], 100),
    ("mpyr_rounding", ["move #>$400001,x0", "move #>$400000,y0", "mpyr x0,y0,a", "mpyr -x0,y0,b"], 100),
    ("mac_negative_products", ["move #>$c00000,x0", "move #>$400000,y0", "clr a", "mac -x0,y0,a", "mac -x0,y0,a", "macr -x0,y0,a"], 100),
    ("mul_shift_imm", ["move #>$400000,y1", "mpy y1,#2,a", "mac y1,#3,a", "mpyr y1,#1,b", "macr y1,#2,b"], 100),
    ("macsu_macuu", ["move #>$800001,x0", "move #>$800001,y0", "clr a", "macsu x0,y0,a", "clr b", "macuu x0,y0,b"], 100),
    ("dmac_su_uu", ["move #>$800001,x0", "move #>$400000,y0", "mpy x0,y0,a", "dmacsu x0,y0,a", "mpy x0,y0,b", "dmacuu x0,y0,b"], 100),
    ("dmac_negate", ["move #>$400000,x1", "move #>$200000,y1", "mpy x1,y1,a", "dmacss -x1,y1,a"], 100),
    ("mpyri_macri", ["move #>$300000,y1", "mpyri #>$400000,y1,a", "macri #>$200000,y1,a"], 100),
    # --- authored in bank b4 ---
    # Remaining DMAC operand pairs beyond bank 1
    ("dmac_more_pairs", ["move #>$400000,x0", "move #>$300000,x1", "move #>$200000,y0", "move #>$100000,y1", "mpy x0,y0,a", "dmacss x1,y0,a", "dmacss y1,x1,a", "mpy x0,y0,b", "dmacsu y0,y1,b", "dmacuu x0,x1,b"], 100),
    # --- authored in bank b8 ---
    # qq_reg_mulshift table rows beyond y1 (a different register mapping
    # from qq_reg), plus shift count #0.
    ("qq_mulshift_rows", ["move #>$400000,x0", "move #>$200000,y0", "move #>$300000,x1", "move #>$100000,y1", "mpy x0,#2,a", "mpy y1,#0,b", "mac y0,#1,b", "macr x1,#3,a"], 100),
    # Minus-form (k=1) multiplies: never executed by any prior case.
    ("mul_minus_shift", ["move #>$400001,y1", "move #>$300001,x0", "move #>$200001,y0", "move #>$100001,x1", "mpy -y1,#2,a", "mpyr -x0,#1,b", "mac -y0,#2,a", "macr -x1,#0,b"], 100),
    ("mul_minus_imm", ["move #>$123456,y1", "move #>$654321,y0", "mpyi -#>$345678,y1,a", "maci -#>$234567,y1,a", "mpyri -#>$456789,y0,b", "macri -#>$56789a,y0,b"], 100),
    ("mul_minus_wide", ["move #>$800001,x0", "move #>$800001,y0", "move #>$789abc,x1", "move #>$abcdef,y1", "mpysu -x0,y0,a", "mpyuu -x1,y1,b", "macsu -x0,y0,a", "macuu -x1,y1,b", "dmacsu -x0,y0,b"], 100),
    # $800000 x $800000 (-1.0 squared): the canonical fractional corner.
    ("neg1_sq_mpy", ["move #>$800000,x0", "move #>$800000,y0", "mpy x0,y0,a", "mpyr x0,y0,b", "movec sr,r6"], 100),
    ("neg1_sq_mac", ["move #>$800000,x0", "move #>$800000,y0", "clr a", "move #>$7fffff,x1", "move x1,a1", "move #>$ffffff,y1", "move y1,a0", "mac x0,y0,a", "movec sr,r4", "clr b", "move #>$7f0000,x1", "move x1,b1", "macr x0,y0,b", "movec sr,r5"], 100),
    ("neg1_sq_wide", ["move #>$800000,x0", "move #>$800000,y0", "mpysu x0,y0,a", "mpyuu x0,y0,b", "dmacss x0,y0,a", "movec sr,r7"], 100),
    # --- authored in bank b11 ---
    # Multiply-with-shift counts > 23: the 5-bit field's negative-shift
    # arm (S * 2^-n with n=24/25/31) never executed; undocumented range,
    # probe-first.
    ("mulshift_hi_counts", ["move #>$400000,y1", "mpy y1,#25,a", "movec sr,r4", "clr b", "mac y1,#31,b", "movec sr,r5", "mpyr y1,#24,a", "movec sr,r6", "mpy y1,#24,b", "movec sr,r7"], 100),
    # --- authored in bank b12 ---
    # MPYR/MAC #0 (zero-product rule under rounding) banked - probed in
    # isolation - plus mpyr #3 with d=A.
    ("mpyr_shift_variants", ["move #>$400001,x0", "mpyr x0,#0,a", "movec sr,r4", "mpyr x0,#3,a", "movec sr,r5", "move #>$500001,y0", "mac y0,#0,a", "movec sr,r6"], 100),
]

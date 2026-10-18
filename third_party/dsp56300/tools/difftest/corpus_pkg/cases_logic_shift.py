"""Logical ops, ANDI/ORI fields, and the shift/rotate matrix.

Case bodies are verbatim from the original bank lists; bank membership
and order live in banks.py. See registry.py for the authoring rules.
"""

THEME = "logic_shift"

# (name, [body lines], max_steps)
CASES = [
    # --- authored in bank b0 ---

    # --- Logic and shifts ---
    ("and_or_eor", ["move #>$f0f0f0,x0", "clr a", "move #>$0ff00f,x1", "move x1,a1", "and x0,a", "or x0,a", "eor x0,a"], 100),
    ("not_lsl_lsr", ["clr a", "move #>$800001,x0", "move x0,a1", "not a", "lsl a", "lsr a"], 100),
    ("asl_asr", ["clr a", "move #>$c00001,x0", "move x0,a1", "asl a", "asr a", "asr a"], 100),
    ("asl_imm_multi", ["clr a", "move #>$000123,x0", "move x0,a1", "asl #4,a,a", "asr #2,a,a"], 100),
    ("rol_ror", ["clr a", "move #>$800001,x0", "move x0,a1", "rol a", "ror a", "ror a"], 100),
    ("andi_ori_ccr", ["move #>$ffffff,x0", "clr a", "move x0,a1", "add x0,a", "ori #$0f,ccr", "andi #$fe,ccr"], 100),
    ("and_or_imm_long", ["clr a", "move #>$123456,x0", "move x0,a1", "and #>$0f0f0f,a", "or #>$300030,a", "eor #>$111111,a"], 100),
    # --- authored in bank b1 ---
    # Logic on accumulator halves
    ("and_only_a1", ["clr a", "move #>$abcdef,x0", "move x0,a0", "move #>$123456,x1", "move x1,a1", "move #>$0f0f0f,y0", "and y0,a"], 100),
    ("logic_x1_y1_sources", ["clr a", "move #>$f0f0f0,x1", "move #>$123456,y0", "move y0,a1", "and x1,a", "move #>$0000ff,y1", "or y1,a", "eor x1,a"], 100),
    ("not_edges", ["clr a", "not a", "clr b", "move #>$ffffff,x0", "move x0,b1", "not b"], 100),
    ("andi_mr_preserve", ["move #>$654321,x0", "clr a", "ori #$0f,ccr", "andi #$f3,mr", "ori #$04,ccr"], 100),
    # Shift edges
    ("lsl_zero_count", ["clr a", "move #>$800000,x0", "move x0,a1", "asl #0,a,a", "lsr #0,a"], 100),
    ("lsl_imm_max", ["clr a", "move #>$ffffff,x0", "move x0,a1", "lsl #$17,a"], 100),
    ("lsr_imm_max", ["clr a", "move #>$ffffff,x0", "move x0,a1", "lsr #$17,a"], 100),
    ("asl_multi_to_other", ["clr a", "move #>$c00001,x0", "move x0,a1", "asl #4,a,b", "asr #3,b,a"], 100),
    ("shift_by_register", ["move #>$000004,x0", "clr a", "move #>$0000f0,x1", "move x1,a1", "asl x0,a,a", "asr x0,a,a"], 100),
    ("lsl_lsr_by_register", ["move #>$000008,y0", "clr b", "move #>$ff00ff,x1", "move x1,b1", "lsl y0,b", "lsr y0,b"], 100),
    ("rol_ror_carry_chain", ["clr a", "move #>$800001,x0", "move x0,a1", "lsl a", "rol a", "rol a", "ror a"], 100),
    # --- authored in bank b6 ---
    # LSL #0: flag-only path (clears C, logical N/Z from unchanged a1).
    ("lsl_zero_pair", ["clr a", "move #>$800001,x0", "move x0,a1",
                       "ori #$01,ccr", "lsl #0,a",
                       "clr b", "move #>$400002,x1", "move x1,b1",
                       "lsl #0,b"], 100),
    # ORI into MR: set the scaling bits, observe, restore.
    ("ori_mr_scale", ["move #>$123456,x0", "clr a", "ori #$0c,mr",
                      "movec sr,y0", "andi #$f3,mr", "movec sr,y1",
                      "movec #>$c00300,sr"], 100),
    # --- authored in bank b8 ---
    # Out-of-range register shift counts: {31,32}, {40,55}, {56,63}
    # (counts 0/24/25 already probed and matching; the manual leaves
    # these undefined, so isolated probes precede bank capture).
    ("shift_cnt_3132", ["clr a", "move #>$c00001,x0", "move x0,a1", "move #>$00001f,y0", "lsl y0,a", "movec sr,r4", "clr b", "move x0,b1", "move #>$000020,y1", "lsl y1,b", "movec sr,r5", "clr a", "move x0,a1", "lsr y0,a", "movec sr,r6", "clr b", "move x0,b1", "lsr y1,b", "movec sr,r7"], 100),
    ("shift_cnt_4055", ["clr a", "move #>$c00001,x0", "move x0,a1", "move #>$000028,y0", "lsl y0,a", "movec sr,r4", "clr b", "move x0,b1", "move #>$000037,y1", "lsl y1,b", "movec sr,r5", "clr a", "move x0,a1", "lsr y0,a", "movec sr,r6", "clr b", "move x0,b1", "lsr y1,b", "movec sr,r7"], 100),
    ("shift_cnt_5663", ["clr a", "move #>$c00001,x0", "move x0,a1", "move #>$000038,y0", "lsl y0,a", "movec sr,r4", "clr b", "move x0,b1", "move #>$00003f,y1", "lsl y1,b", "movec sr,r5", "clr a", "move x0,a1", "lsr y0,a", "movec sr,r6", "clr b", "move x0,b1", "lsr y1,b", "movec sr,r7"], 100),
    # ASR #0 (the carry-flag branch of the zero-count path) + ASL #23
    # into the other accumulator.
    ("asr0_asl23", ["clr a", "move #>$800001,x0", "move x0,a1", "ori #$01,ccr", "asr #0,a,a", "movec sr,r4", "clr a", "move #>$654321,x1", "move x1,a1", "asl #23,a,b", "movec sr,r5"], 100),
    # --- authored in bank b12 ---
    # Shift count sources beyond x0/y0/y1: b1, x1 (counts kept < 24).
    ("shift_src_fanout", ["clr b", "move #>$000003,y0", "move y0,b1", "clr a", "move #>$c00001,x0", "move x0,a1", "asl b1,a,a", "movec sr,r4", "move #>$000004,x1", "asr x1,a,b", "movec sr,r5", "lsl b1,a", "movec sr,r6", "move #>$000005,x1", "lsr x1,b", "movec sr,r7"], 100),
    # a1 as shift-count source + ASR immediate counts 9-23 (never used).
    ("shift_a1_count_asr_imm", ["clr a", "move #>$000002,x0", "move x0,a1", "clr b", "move #>$c00001,x1", "move x1,b1", "asl a1,b,b", "movec sr,r4", "lsl a1,b", "movec sr,r5", "asr #20,b,a", "movec sr,r6", "clr b", "move x1,b1", "asr #23,b,a", "movec sr,r7"], 100),
    # 70 straight-line ops with no terminator: the emit_block max_len
    # fallthrough path (and multi-word dirty-range checks) never fired.
    ("straightline_70", ["clr a", "move #>$000001,x0"] + ["add x0,a"] * 70, 200),
    # ROR carry-source discriminator: the manual's CCR table says C=bit
    # 47 (copy-paste erratum); the description (and the emulator) use
    # old bit 24. Operands make the two readings differ.
    ("ror_carry_source", ["clr a", "move #>$800000,x0", "move x0,a1", "ror a", "movec sr,r4", "clr b", "move #>$000001,y0", "move y0,b1", "ror b", "movec sr,r5"], 100),
]

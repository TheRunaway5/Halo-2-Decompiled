"""Bit set/clear/change/test on registers and memory.

Case bodies are verbatim from the original bank lists; bank membership
and order live in banks.py. See registry.py for the authoring rules.
"""

THEME = "bits"

# (name, [body lines], max_steps)
CASES = [
    # --- authored in bank b0 ---

    # --- Bit ops ---
    ("bset_bclr_mem", ["move #>$000000,x0", "move x0,x:$28", "bset #5,x:$28", "bset #0,x:$28", "bchg #5,x:$28", "bclr #0,x:$28", "btst #5,x:$28"], 100),
    ("bset_reg", ["move #>$000000,x1", "bset #17,x1", "btst #17,x1", "bchg #17,x1", "btst #17,x1"], 100),
    # --- authored in bank b1 ---
    # Bit ops: register targets incl accumulator parts and control regs
    ("bitops_acc_parts", ["clr a", "bset #$17,a1", "bchg #$0,a0", "bset #$5,a2", "btst #$17,a1"], 100),
    ("bitops_b_parts", ["clr b", "bset #$0,b2", "bclr #$0,b2", "bset #$16,b1", "bchg #$16,b1"], 100),
    ("bitops_agu_regs", ["move #>$000000,r5", "bset #$14,r5", "move #>$ffffff,n5", "bclr #$00,n5", "btst #$17,n5"], 100),
    ("bitops_sr_ccr", ["bset #$0,sr", "bchg #$1,sr", "btst #$0,sr", "bclr #$0,sr", "movec #>$c00300,sr"], 100),
    ("bitops_ssh_no_pop", ["movec #>$001234,ssh", "bset #$3,ssh", "bchg #$0,ssh", "movec ssh,r3"], 100),
    ("bitops_la_lc", ["movec #>$000f0f,lc", "bset #$4,lc", "bclr #$0,lc", "movec #>$0000ff,la", "bchg #$7,la", "movec #>$ffffff,la", "movec #>$000000,lc"], 100),
    # Bit ops: memory EA forms
    ("bitops_y_mem", ["move #>$000000,x0", "move x0,y:$40", "bset #$5,y:$40", "bchg #$17,y:$40", "bclr #$5,y:$40", "btst #$17,y:$40"], 100),
    ("bitops_x_ea_update", ["move #>$44,r6", "move #>$000000,x0", "move x0,x:$44", "move x0,x:$43", "bset #$b,x:(r6)-", "bset #$2,x:(r6)", "btst #$b,x:$44"], 100),
    ("bitops_predec_nn", ["move #>$48,r7", "move #>$2,n7", "move #>$000000,x0", "move x0,x:$47", "move x0,x:$48", "move x0,x:$4a", "bset #$0,x:-(r7)", "bset #$1,x:(r7)+n7", "bchg #$17,x:(r7)"], 100),
    ("bitops_abs_long", ["move #>$000000,x0", "move x0,x:$0500", "bset #$10,x:>$0500", "bchg #$00,x:>$0500", "btst #$10,x:>$0500"], 100),
    ("btst_bit23_carry", ["move #>$800000,x0", "move x0,x:$4c", "btst #$17,x:$4c", "clr a", "rol a"], 100),
    # --- authored in bank b12 ---
    # Y-space aa-form bit ops (all four were X-only in aa form; the b1
    # y:$40 case used the absolute encoding).
    ("bitops_y_aa", ["move #>$000000,x0", "move x0,y:$2c", "bset #5,y:$2c", "bchg #23,y:$2c", "btst #0,y:$2c", "movec sr,r4", "bclr #5,y:$2c", "move y:$2c,y1"], 100),
    # Bit ops on control-register classes never targeted (ep/m/sz/sc/
    # vba), with restores.
    ("bitops_ctrl_regs", ["movec #>$001234,ep", "bchg #0,ep", "movec ep,x1", "bset #2,m1", "btst #15,m1", "movec m1,y1", "movec #>$ffffff,m1", "bset #4,sz", "bclr #1,sz", "movec sz,y0", "movec #>$0,sz", "movec #>$0,ep", "btst #3,sc", "movec sr,r4", "bclr #16,vba", "movec vba,n4", "movec #>$ff0000,vba"], 100),
    # Bit ops with the FULL accumulator as target (limited RMW path).
    ("bitops_full_acc", ["clr a", "move #>$7fffff,x0", "move x0,a1", "move #>$ffffff,x1", "move x1,a0", "bset #23,a", "movec sr,r4", "clr b", "move #>$800000,y0", "move y0,b1", "bchg #0,b", "btst #23,b", "movec sr,r5"], 100),
]

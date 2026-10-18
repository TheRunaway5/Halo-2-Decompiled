"""SR arithmetic modes: SM saturation, S1:S0 scaling, cc disjuncts.

Every case restores SR to the boot value ($C00300) before ending.
SM cases are wedge-suspect class (first hardware exposure of SR bit 20)
and sit at the front of their bank for isolated --cases probing.
See registry.py for the authoring rules.
"""

THEME = "srmodes"

# (name, [body lines], max_steps)
CASES = [
    # --- authored in bank b9 ---
    # SM saturation mode (SR bit 20): clamp on 48-bit crossings. First
    # case doubles as the isolated silicon probe for the mode itself.
    ("srm_sm_add_up", ["movec #>$d00300,sr", "clr a", "move #>$7fffff,x0", "move x0,a1", "move #>$ffffff,x1", "move x1,a0", "move #>$000001,y0", "add y0,a", "movec sr,r4", "movec #>$c00300,sr"], 100),
    ("srm_sm_sub_down", ["movec #>$d00300,sr", "clr a", "move #>$ffffff,x0", "move x0,a2", "move #>$800000,x1", "move x1,a1", "move #>$000001,y0", "sub y0,a", "movec sr,r5", "movec #>$c00300,sr"], 100),
    ("srm_sm_abs_neg", ["movec #>$d00300,sr", "clr a", "move #>$000080,x0", "move x0,a2", "abs a", "movec sr,r6", "tfr a,b", "clr a", "move x0,a2", "neg a", "movec sr,r7", "movec #>$c00300,sr"], 100),
    ("srm_sm_asl", ["movec #>$d00300,sr", "clr a", "move #>$400000,x0", "move x0,a1", "asl a", "movec sr,r4", "clr b", "move #>$ffffff,x1", "move x1,b2", "move #>$bf0000,y0", "move y0,b1", "asl b", "movec sr,r5", "movec #>$c00300,sr"], 100),
    ("srm_sm_mac", ["movec #>$d00300,sr", "move #>$800000,x0", "move #>$800000,y0", "clr a", "move #>$7f0000,x1", "move x1,a1", "mac x0,y0,a", "movec sr,r6", "clr b", "move #>$810000,y1", "move y1,b1", "macr x0,y0,b", "movec sr,r7", "movec #>$c00300,sr"], 100),
    ("srm_sm_rnd", ["movec #>$d00300,sr", "clr a", "move #>$7fffff,x0", "move x0,a1", "move #>$800000,x1", "move x1,a0", "rnd a", "movec sr,r4", "movec #>$c00300,sr"], 100),
    # Control: results well inside the 48-bit range must NOT clamp.
    ("srm_sm_noclamp", ["movec #>$d00300,sr", "clr a", "move #>$123456,x0", "move x0,a1", "move #>$000111,y0", "add y0,a", "movec sr,r5", "clr b", "move #>$400000,x1", "move x1,b1", "rnd b", "movec sr,r6", "movec #>$c00300,sr"], 100),
    # SM combined with the scaling modes (saturation position moves).
    ("srm_sm_scale_dn", ["movec #>$d00700,sr", "clr a", "move #>$3fffff,x0", "move x0,a1", "move #>$ffffff,x1", "move x1,a0", "move #>$000001,y0", "add y0,a", "movec sr,r4", "clr b", "move #>$400000,y1", "move y1,b1", "rnd b", "movec sr,r5", "movec #>$c00300,sr"], 100),
    ("srm_sm_scale_up", ["movec #>$d00b00,sr", "clr a", "move #>$3fffff,x0", "move x0,a1", "move #>$ffffff,x1", "move x1,a0", "move #>$000001,y0", "add y0,a", "movec sr,r6", "clr b", "move #>$200000,y1", "move y1,b1", "asl b", "movec sr,r7", "movec #>$c00300,sr"], 100),
    # Arithmetic CCR (E/U) under S1:S0 scaling: tst accs straddling each
    # mode's E/U bit windows.
    ("srm_scl_tst_dn", ["movec #>$c00700,sr", "clr a", "move #>$200000,x0", "move x0,a1", "tst a", "movec sr,r4", "clr a", "move #>$400000,x1", "move x1,a1", "tst a", "movec sr,r5", "clr a", "move #>$800000,y0", "move y0,a1", "tst a", "movec sr,r6", "movec #>$c00300,sr"], 100),
    ("srm_scl_tst_up", ["movec #>$c00b00,sr", "clr a", "move #>$200000,x0", "move x0,a1", "tst a", "movec sr,r4", "clr a", "move #>$400000,x1", "move x1,a1", "tst a", "movec sr,r5", "clr a", "move #>$800000,y0", "move y0,a1", "tst a", "movec sr,r6", "movec #>$c00300,sr"], 100),
    # Rounding ties under scaling (the round position moves per mode).
    ("srm_scl_rnd_dn", ["movec #>$c00700,sr", "clr a", "move #>$123456,x0", "move x0,a1", "move #>$800000,x1", "move x1,a0", "rnd a", "movec sr,r4", "clr b", "move x0,b1", "move #>$400000,y0", "move y0,b0", "rnd b", "movec sr,r5", "mpyr x0,x0,b", "movec sr,r6", "movec #>$c00300,sr"], 100),
    ("srm_scl_rnd_up", ["movec #>$c00b00,sr", "clr a", "move #>$123457,x0", "move x0,a1", "move #>$800000,x1", "move x1,a0", "rnd a", "movec sr,r4", "clr b", "move #>$345677,y0", "move y0,b1", "move #>$400000,y1", "move y1,b0", "rnd b", "movec sr,r5", "macr y1,x1,b", "movec sr,r6", "movec #>$c00300,sr"], 100),
    # Jcc on E under scaling.
    ("srm_scl_jcc", ["movec #>$c00700,sr", "clr a", "move #>$400000,x0", "move x0,a1", "tst a", "move #>$0,y0", "move #>$0,y1", "jes lbl_sje1", "move #>$bad301,y0", "lbl_sje1: jnn lbl_sje2", "move #>$600d31,y1", "lbl_sje2: movec #>$c00300,sr"], 100),
    # cc disjunct matrix: manufacture N/V/Z/U/E directly so the V=1 arm of
    # GE/LT/GT/LE (never reached by arithmetic-only corpus flags) and the
    # U/E arms of NN/NR are pinned. Distinct payload per transfer.
    ("srm_cc_ge_lt", ["move #>$0111aa,x0", "move #>$0222bb,x1", "move #>$0333cc,y0", "move #>$0444dd,y1", "clr a", "clr b", "ori #$02,ccr", "tge x0,a", "tlt x1,b", "move a1,r4", "move b1,r5", "andi #$00,ccr", "ori #$0a,ccr", "tge y0,a", "tlt y1,b"], 100),
    ("srm_cc_gt_le", ["move #>$0555aa,x0", "move #>$0666bb,x1", "move #>$0777cc,y0", "move #>$0888dd,y1", "clr a", "clr b", "ori #$02,ccr", "tgt x0,a", "tle x1,b", "move a1,r4", "move b1,r5", "andi #$00,ccr", "ori #$0a,ccr", "tgt y0,a", "tle y1,b", "move a1,r6", "move b1,r7", "andi #$00,ccr", "ori #$06,ccr", "tgt x0,b", "tle y1,a"], 100),
    ("srm_cc_nn_nr", ["move #>$0999aa,x0", "move #>$0aaabb,x1", "move #>$0bbbcc,y0", "move #>$0cccdd,y1", "clr a", "clr b", "tnn x0,a", "tnr x1,b", "move a1,r4", "move b1,r5", "andi #$00,ccr", "ori #$10,ccr", "tnn y0,a", "tnr y1,b", "move a1,r6", "move b1,r7", "andi #$00,ccr", "ori #$24,ccr", "tnn x1,a", "tnr y0,b"], 100),
    # --- authored in bank b11 ---
    # RM+SM combined: a saturating RND in truncation mode - does the
    # rounded-grid clamp still apply? Probe-first (first RM+SM silicon).
    ("srm_rm_sm_sat", ["movec #>$f00300,sr", "clr a", "move #>$7fffff,x0", "move x0,a1", "move #>$800000,x1", "move x1,a0", "rnd a", "movec sr,r4", "movec #>$c00300,sr"], 100),
    # SM x unsigned-mode multiplies: the emulator never clamps
    # mpysu/mpyuu/macsu/macuu and clamps dmac only for ss=0. Probe-first.
    ("srm_sm_unsigned", ["movec #>$d00300,sr", "move #>$800000,x0", "move #>$800000,y0", "clr a", "move #>$7f0000,x1", "move x1,a1", "macuu x0,y0,a", "movec sr,r4", "clr b", "move x1,b1", "macsu x0,y0,b", "movec sr,r5", "mpyuu x0,y0,b", "movec sr,r6", "clr a", "move #>$7f0000,y1", "move y1,a1", "dmacsu x0,y0,a", "movec sr,r7", "movec #>$c00300,sr"], 100),
    # Reserved scaling mode S1:S0 = 11: flag windows + limited read.
    # Probe-first (writable SR state the corpus never entered).
    ("srm_scl_mode3", ["movec #>$c00f00,sr", "clr a", "move #>$400000,x0", "move x0,a1", "tst a", "movec sr,r4", "move a,y0", "movec sr,r5", "movec #>$c00300,sr"], 100),
    # RM (two's-complement rounding, SR bit 21): only one case ever set
    # it before. Rounding multiplies and a non-tie RND under truncation.
    ("srm_rm_rounds", ["movec #>$e00300,sr", "move #>$400001,x0", "move #>$400000,y0", "mpyr x0,y0,a", "movec sr,r4", "clr b", "move #>$333333,x1", "move x1,b1", "move #>$789abc,y1", "move y1,b0", "rnd b", "movec sr,r5", "macr x0,y0,b", "movec sr,r6", "movec #>$c00300,sr"], 100),
    ("srm_rm_mulshift", ["movec #>$e00300,sr", "move #>$400001,y1", "mpyr y1,#1,a", "movec sr,r4", "clr b", "move #>$123457,x0", "move x0,b1", "macr y1,#2,b", "movec sr,r5", "mpyri #>$400001,y1,a", "movec sr,r6", "movec #>$c00300,sr"], 100),
    # RM x scaling: exact ties under S0 and S1 must truncate, not
    # converge.
    ("srm_rm_scaled", ["movec #>$e00700,sr", "clr a", "move #>$123456,x0", "move x0,a1", "move #>$800000,x1", "move x1,a0", "rnd a", "movec sr,r4", "movec #>$e00b00,sr", "clr b", "move x0,b1", "move #>$400000,y0", "move y0,b0", "rnd b", "movec sr,r5", "movec #>$c00300,sr"], 100),
    # SM x ADC/SBC: carry-in pushing the sum across the rail.
    ("srm_sm_adc", ["movec #>$d00300,sr", "move #>$7fffff,x1", "move #>$ffffff,x0", "clr a", "ori #$01,ccr", "adc x,a", "movec sr,r4", "clr b", "move #>$ffffff,y0", "move y0,b2", "move #>$800000,y1", "move y1,b1", "andi #$fe,ccr", "sbc x,b", "movec sr,r5", "movec #>$c00300,sr"], 100),
    # SM x ADDL/SUBL: shift+add crossing the rail; post-clamp C.
    ("srm_sm_addl", ["movec #>$d00300,sr", "clr a", "move #>$400000,x0", "move x0,a1", "clr b", "move #>$300000,x1", "move x1,b1", "addl a,b", "movec sr,r4", "clr a", "clr b", "move #>$ffffff,y0", "move y0,a2", "move #>$900000,y1", "move y1,a1", "subl b,a", "movec sr,r5", "movec #>$c00300,sr"], 100),
    # SM clamp grid position when a saturating RND runs under scaling.
    ("srm_sm_scaled_rndclamp", ["movec #>$d00700,sr", "clr a", "move #>$7fffff,x0", "move x0,a1", "move #>$ffffff,x1", "move x1,a0", "rnd a", "movec sr,r4", "movec #>$d00b00,sr", "clr b", "move x0,b1", "move x1,b0", "rnd b", "movec sr,r5", "movec #>$c00300,sr"], 100),
    # S and L flags from limited reads under scaling, stashed BEFORE the
    # SR restore (the b2 limit cases restored first, losing them).
    ("srm_scl_s_flag", ["movec #>$c00700,sr", "clr a", "move #>$200000,x0", "move x0,a1", "move a,y0", "movec sr,r4", "movec #>$c00b00,sr", "clr b", "move #>$400000,x1", "move x1,b1", "move b,y1", "movec sr,r5", "movec #>$c00300,sr"], 100),
    # Scaled E windows with a2 participation (every earlier scaled tst
    # left a2 = 0, so the scale-down E arm never fired).
    ("srm_scl_e_windows", ["movec #>$c00700,sr", "clr a", "move #>$000001,x0", "move x0,a2", "tst a", "movec sr,r4", "clr a", "move #>$ffffff,x1", "move x1,a2", "move #>$800000,y0", "move y0,a1", "tst a", "movec sr,r5", "movec #>$c00b00,sr", "clr a", "move #>$000080,x0", "move x0,a2", "tst a", "movec sr,r6", "movec #>$c00300,sr"], 100),
    # Exact convergent-rounding ties under S0 (a0=0, a1 even) and S1
    # (low 23 bits zero) - the tie-clear arms never fired scaled.
    ("srm_scl_ties", ["movec #>$c00700,sr", "clr a", "move #>$123456,x0", "move x0,a1", "move #>$000000,x1", "move x1,a0", "rnd a", "movec sr,r4", "movec #>$c00b00,sr", "clr b", "move #>$345678,y0", "move y0,b1", "move #>$400000,y1", "move y1,b0", "rnd b", "movec sr,r5", "movec #>$c00300,sr"], 100),
    # --- authored in bank b13 ---
    # Back-to-back SM ops where only the FIRST saturates, with the SR
    # stash only after both: regression shape for the deferred-flag
    # sm_needs_sat_var clobber (block mode lost the sticky L).
    ("sm_chain_deferred_l", ["movec #>$d00300,sr", "clr a", "move #>$7fffff,x0", "move x0,a1", "move #>$ffffff,x1", "move x1,a0", "move #>$000001,y0", "add y0,a", "clr b", "add y0,b", "movec sr,r4", "movec #>$c00300,sr"], 100),
    # --- authored in bank b16 ---
    # Stale SM saturation marker: a plain IFcc'd ASL saturates (its CCR
    # update is architecturally suppressed), then a NON-saturating op
    # whose pending-flag kind includes the SM term (cmp) flushes - the
    # leftover needs_sat must not OR into V/L (silicon: no V/L). The SR
    # stash right after the cmp captures the would-be leak; the saturated
    # (clamped) B value pins the acc-select path of the suppressed op.
    ("ifcc_sm_stale_sat", ["movec #>$d00300,sr", "clr a", "clr b", "move #>$200001,x1", "move x1,b", "move #>$123456,y0", "subl a,b", "asl b ifec", "cmp y0,a", "movec sr,r4", "movec #>$c00300,sr"], 100),
    # --- authored in bank b20 ---
    # Direct SR write over a PENDING flag computation orphans the
    # deferred SM saturation marker: an SM-mode asl saturates (marker
    # set, flags still pending), a movec-to-SR discards the pending
    # computation, and the next flush (cmp) must not OR the stale
    # needs_sat into V/L (silicon: clean V/L). The sibling of
    # ifcc_sm_stale_sat - there the pending computation flushed, here it
    # is discarded.
    ("sm_marker_sr_overwrite", ["move #>$0002c4,r2", "move #$0,n2", "move #>$0002d4,r5", "move #>$123456,x1", "clr a", "clr b", "movec #>$d00300,sr", "not b x1,x:(r2)+ y:(r5)+,y1", "asl b y:(r2)-n2,x0", "movec #>$c00b00,sr", "cmp #>$808080,a", "movec sr,r4", "movec #>$c00300,sr"], 100),
]

"""Bank packing manifest.

Banks exist because the hardware runner boots each corpus as ONE image
loaded by the bootstrap's single 0x800-word DMA: a bank is what fits
between the cases' base (P:$0100) and the runner's snapshot code
(P:$0720). Case ORDER is load-bearing (bank images replay
sequentially on hardware, carrying register state across cases).

sealed=True + a fingerprint means the bank's image is golden-bound:
the generator refuses to emit a different image (recapture on the
console, then reseal, to change it). sealed=False marks a bank as
authored but awaiting hardware capture. The fingerprint is a sha256
over the canonical content both engines consume - sorted
(space, addr, word) memory triples plus the meta lines - NOT raw
bytes, because assembler symbol-record order is not deterministic.
"""

from dataclasses import dataclass

from .cases_sweeps import SWEEP_NAMES
from .cases_valsweeps import VALSWEEP_NAMES


@dataclass(frozen=True)
class Bank:
    cases: tuple
    fixed_slots: bool = False
    sealed: bool = True
    fingerprint: str = None
    # Deterministic dirty-state init, or None for the historical
    # zero-fill. (kx, cx, ky, cy, sshb, sslb): both engines pre-fill
    # X:$0000-$0BFF with (kx*addr+cx)&$FFFFFF and Y:$0000-$07FF with
    # (ky*addr+cy)&$FFFFFF, and seed hardware stack slot j (1..15)
    # with SSH=sshb+j-1 / SSL=sslb+j-1 (values must stay <= $FF so the
    # hardware preamble can use short movec immediates). K values must
    # be odd (bijective mod 2^24) so every address gets a unique word.
    # The tuple is emitted as a `fill` header line in the bank's meta,
    # so it is covered by the sealed fingerprint and both engines plus
    # the hardware runner configure themselves from the meta alone.
    fill: tuple = None
    # Bank-level auxiliary asm lines (verbatim, may contain org
    # directives) appended to the combined image after the cases -
    # e.g. a fault bank's interrupt vector page and handler. Covered by
    # the sealed fingerprint via the combined lod; must live above the
    # last case's end and below the runner's snapshot code.
    aux: tuple = None


BANKS = {
    "b0": Bank(
        cases=(
            "move_imm",
            "add_reg",
            "add_carry",
            "add_overflow",
            "sub_borrow",
            "addl_subl",
            "addr_subr",
            "adc_sbc",
            "abs_neg",
            "cmp_flags",
            "cmpm_cmpu",
            "tst_zero",
            "max_maxm",
            "rnd_basic",
            "inc_dec_acc",
            "and_or_eor",
            "not_lsl_lsr",
            "asl_asr",
            "asl_imm_multi",
            "rol_ror",
            "andi_ori_ccr",
            "and_or_imm_long",
            "clb_norm",
            "extract_insert",
            "mpy_basic",
            "mpy_neg",
            "mac_macr",
            "mpyi_maci",
            "mpy_su_uu",
            "dmac_ss",
            "agu_postinc",
            "agu_postdec_predec",
            "agu_offset_n",
            "agu_indexed",
            "agu_modulo",
            "agu_bitrev",
            "lua_basic",
            "move_long_disp_r1",
            "move_long_disp_r0",
            "move_short_disp",
            "move_xy_dual",
            "move_l_space",
            "move_limiting",
            "move_short_imm",
            "movec_regs",
            "movem_pread",
            "bset_bclr_mem",
            "bset_reg",
            "jmp_abs",
            "jcc_taken",
            "jcc_not_taken",
            "bra_fwd",
            "jsr_rts",
            "bsr_rts",
            "jclr_aa_taken",
            "jset_reg_taken",
            "brclr_aa",
            "tcc_transfer",
            "do_imm",
            "do_reg",
            "rep_imm",
            "rep_reg",
            "do_nested",
            "stack_push_pop",
            "sc_unbalanced_push",
        ),
        fixed_slots=True,
    ),
    "b1": Bank(
        cases=(
            "add_carry_out48",
            "sub_borrow_deep",
            "adc_carry_chain",
            "sbc_borrow_in",
            "abs_min_negative",
            "neg_min_negative",
            "addr_negative_shift",
            "addl_carry_xor",
            "subl_edge",
            "tfr_acc_and_reg",
            "rnd_tie_even",
            "rnd_tie_above",
            "rnd_twos_complement",
            "max_no_transfer",
            "maxm_magnitude",
            "cmp_imm_forms",
            "add_imm_short",
            "and_only_a1",
            "logic_x1_y1_sources",
            "not_edges",
            "andi_mr_preserve",
            "lsl_zero_count",
            "lsl_imm_max",
            "lsr_imm_max",
            "asl_multi_to_other",
            "shift_by_register",
            "lsl_lsr_by_register",
            "rol_ror_carry_chain",
            "clb_positive_small",
            "clb_negative",
            "clb_zero_acc",
            "norm_iterations",
            "normf_shift",
            "extract_signed_field",
            "extract_one_bit",
            "extractu_imm_control",
            "insert_preserve",
            "insert_reg_control",
            "merge_fields",
            "mpy_all_qq_pairs",
            "mpy_square_forms",
            "mpyr_rounding",
            "mac_negative_products",
            "mul_shift_imm",
            "macsu_macuu",
            "dmac_su_uu",
            "dmac_negate",
            "mpyri_macri",
            "div_iterations",
            "div_negative",
            "tcc_cs_lt",
            "tcc_gt_le",
            "tcc_es_ec_ls",
            "tcc_with_rpair",
            "ifcc_taken_flags",
            "ifcc_u_updates_ccr",
            "bitops_acc_parts",
            "bitops_b_parts",
            "bitops_agu_regs",
            "bitops_sr_ccr",
            "bitops_ssh_no_pop",
            "bitops_la_lc",
            "bitops_y_mem",
            "bitops_x_ea_update",
            "bitops_predec_nn",
            "bitops_abs_long",
            "btst_bit23_carry",
        ),
        fixed_slots=False,
    ),
    "b2": Bank(
        cases=(
            "lua_data_reg_dest",
            "lua_accumulator_dest",
            "lua_rel_negative",
            "lra_forms",
            "limit_scale_up_read",
            "limit_scale_up_negative",
            "limit_scale_down_read",
            "limit_scale_up_ok",
            "limit_write_through",
            "lmove_a10_b10",
            "lmove_x_y_pairs",
            "lmove_ab_ba",
            "lmove_ea_forms",
            "pm0_class2_xr",
            "pm1_x_and_reg",
            "pm1_reg_and_y",
            "pm1_x_imm_with_reg",
            "pm8_acc_both",
            "pm8_write_accs",
            "pm8_mixed_dir",
            "vsl_insert_bits",
            "agu_mod_negative_step",
            "agu_mod_transient",
            "agu_mod_large_step",
            "agu_mod_nonpow2",
            "agu_mod_min_m1",
            "agu_mod_predec_wrap",
            "agu_multiwrap_mod2",
            "agu_multiwrap_mod4",
            "agu_bitrev_walk",
            "agu_bitrev_highbits",
            "agu_linear_wrap24",
            "disp_long_negative",
            "disp_short_y",
            "movem_read_own_code",
            "movec_sz_vba_read",
            "movec_sr_roundtrip",
            "movec_mem_forms",
            "stack_two_deep",
            "stack_ssl_rewrite",
            "stack_sc_write",
            "jmp_indirect_ea",
            "jcc_ea_not_taken_side_effect",
            "jsr_indirect",
            "bra_register",
            "bsr_register",
            "bcc_backward",
            "jcc_cs_cc",
            "jcc_mi_pl",
            "jcc_ge_lt",
            "jcc_gt_le",
            "jcc_nn_nr",
            "jcc_ec_es",
            "jsclr_jsset",
            "brset_reg_form",
            "bsclr_taken",
            "jclr_ea_form",
            "do_lc_one",
            "do_from_memory",
            "do_from_ea_postinc",
            "do_enddo_early",
            "do_brkcc",
            "do_forever_brk",
            "dor_relative",
            "do_nested_deep",
            "rep_from_memory",
            "rep_ssh_pops",
            "loop_regs_after",
            "rti_hand_frame",
            "rep_zero",
            "do_zero_iterations",
        ),
        fixed_slots=False,
    ),
    "b3": Bank(
        cases=(
            "bra_short_fwd",
            "bcc_short_taken_pair",
            "bcc_short_backward",
            "bsr_short_lit",
            "bscc_short_forms",
            "bscc_long_lit",
            "jcc_short_lit",
            "jscc_short_lit",
            "bcc_long_label",
            "bcc_rn_forms",
            "bscc_rn_form",
            "jscc_ea_form",
            "bra_rn_backward",
        ),
        fixed_slots=True,
    ),
    "b4": Bank(
        cases=(
            "alu_imm_entries",
            "extract_imm_control",
            "tcc_regs_only",
            "dor_reg_form",
            "dor_aa_form",
            "dor_ea_form",
            "dor_forever_form",
            "rep_ea_form",
            "rep_reg_zero",
            "rep_aa_zero",
            "rep_ea_zero",
            "movec_aa_short",
            "jclr_reg_high_bit",
            "jset_forms_high_bit",
            "jsclr_forms",
            "jsclr_reg_form",
            "jsset_forms",
            "jsset_reg_form",
            "brclr_forms",
            "brset_forms",
            "bsclr_forms",
            "bsset_forms",
            "debugcc_not_taken",
            "movem_p_write",
            "acc_ext_store",
            "l_flag_conditions",
            "pm0_class2_ry",
            "pm1_x_write_reg",
            "pm1_y_read_reg",
            "pm1_y_imm_reg",
            "pm2_ea_update",
            "pm4_read_aa_forms",
            "pm4_y_read_ea",
            "pm8_dual_read_write",
            "stack_fifteen_deep",
            "addl_carry_edges",
            "subl_carry_edges",
            "dmac_more_pairs",
        ),
        fixed_slots=False,
    ),
    "b5": Bank(cases=tuple(SWEEP_NAMES)),
    "b6": Bank(
        cases=(
            "btst_ssh_pop",
            "movem_aa_roundtrip",
            "l_store_gap",
            "l_store_b_limited_sat",
            "l_load_pairs",
            "l_load_ab_ba",
            "l_short_parallel",
            "lsl_zero_pair",
            "ori_mr_scale",
            "do_rep_inside",
            "do_body_bsr",
            "jclr_ssh_pops",
            "jsset_ssh_pops",
            "ssh_rmw_inplace",
        ),
    ),
    "b7": Bank(
        cases=(
            "pm_y_store_ea",
            "pm_xy_dual_read",
            "pm_xy_ab_write",
            "pm1_ab_transfer",
            "pm1_ab_store",
            "pm1_ab_load",
            "pm8_cross_pairs",
        ),
    ),
    # Gap fill from the four-agent coverage analysis. Wedge-suspect /
    # probe-first cases lead the bank so their boot images are
    # self-contained under --cases.
    "b8": Bank(
        cases=(
            "movec_sp_bounds",
            "movec_vba_rw",
            "shift_cnt_3132",
            "shift_cnt_4055",
            "shift_cnt_5663",
            "lua_m0_plain",
            "do_sp_source",
            "agu_bitrev_n0",
            "agu_multiwrap_preadj",
            "lua_minus_n",
            "qq_mulshift_rows",
            "mul_minus_shift",
            "mul_minus_imm",
            "mul_minus_wide",
            "adc_sbc_c0",
            "neg1_sq_mpy",
            "neg1_sq_mac",
            "neg1_sq_wide",
            "tcc_acc_xfer",
            "tcc_cc_matrix1",
            "tcc_cc_matrix2",
            "btb_ea_commit_jclr",
            "btb_ea_commit_predec",
            "movec_read_dir",
            "cmpu_forms",
            "do_acc_source",
            "do_y_sources",
            "asr0_asl23",
            "pm0_abs_parallel",
            "pm4_ab_selects",
            "pm8_noupd",
            "limit_rails",
            "doinline_jcc",
            "doinline_jmp_jclr",
            "doinline_movem",
            "doinline_len65",
        ),
    ),
    # SR-modes bank: SM saturation, S1:S0 scaling, cc disjunct matrix.
    # SM cases (first silicon exposure of SR bit 20) lead the bank.
    "b9": Bank(
        cases=(
            "srm_sm_add_up",
            "srm_sm_sub_down",
            "srm_sm_abs_neg",
            "srm_sm_asl",
            "srm_sm_mac",
            "srm_sm_rnd",
            "srm_sm_noclamp",
            "srm_sm_scale_dn",
            "srm_sm_scale_up",
            "srm_scl_tst_dn",
            "srm_scl_tst_up",
            "srm_scl_rnd_dn",
            "srm_scl_rnd_up",
            "srm_scl_jcc",
            "srm_cc_ge_lt",
            "srm_cc_gt_le",
            "srm_cc_nn_nr",
        ),
    ),
    # Seeded value sweeps (deterministic generator, see cases_valsweeps).
    "b10": Bank(cases=tuple(VALSWEEP_NAMES)),
    # Gap-analysis round 2 (see dsp-corpus-gap-analysis.md): bit-field
    # datapath (the width=0 authoring bug), RM/SM/scaling interactions,
    # imm-ALU B destinations, DIV sign matrix, AGU reserved-M and
    # modulo-abuse probes. Wedge-suspect / probe-first cases lead.
    "b11": Bank(
        cases=(
            "sp_bitop_probe",
            "ssl_rmw_semantics",
            "srm_rm_sm_sat",
            "srm_sm_unsigned",
            "srm_scl_mode3",
            "bf_extract_overrange",
            "mulshift_hi_counts",
            "agu_mod_n_gt_m",
            "agu_mod_ptr_outside",
            "lua_m0_minus_n",
            "m0_postdec_move",
            "agu_reserved_m",
            "srm_rm_rounds",
            "srm_rm_mulshift",
            "srm_rm_scaled",
            "srm_sm_adc",
            "srm_sm_addl",
            "srm_sm_scaled_rndclamp",
            "srm_scl_s_flag",
            "srm_scl_e_windows",
            "srm_scl_ties",
            "bf_extractu_wide",
            "bf_extract_signed",
            "bf_extract_reg_ctl",
            "bf_insert_real",
            "imm_alu_b_dest",
            "div_sign_matrix",
            "div_src_dest",
            "norm_e_path",
            "normf_arms",
            "max_equal_ties",
            "cmpu_ext_ignored",
            "clb_edges",
            "rnd_tie_below",
            "inc_dec_cross_zero",
            "adc_sbc_edges2",
            "mpyi_neg1_sq",
            "mpyi_src_dest",
            "merge_width_edges",
        ),
        sealed=False,
    ),
    # Gap-analysis round 2, part 2: move/movem/movec class+mode fan-out,
    # Y-space aa forms, EA-update side effects, pmove residue.
    "b12": Bank(
        cases=(
            "movem_class_fanout",
            "movem_ea_updates",
            "movec_y_ea_forms",
            "movec_x_update_forms",
            "movec_reg_sr_write",
            "movec_short_imm_dests",
            "mld_x_space",
            "msd_acc_numreg",
            "bitops_y_aa",
            "bitops_ctrl_regs",
            "bitops_full_acc",
            "btb_y_aa_forms",
            "btb_y_aa_sub",
            "jump_ea_updates",
            "jscc_ea_update_taken",
            "btb_ea_mode_fanout",
            "do_rep_ea_modes",
            "dor_rep_more",
            "do_forever_in_body",
            "enddo_fv_restore",
            "pm8_pin_modes",
            "pm0_mode_fanout",
            "pm2_pm1_modes",
            "pm4_indexed_l_modes",
            "pm3_more_dests",
            "vsl_more",
            "ifcc_breadth",
            "shift_src_fanout",
            "shift_a1_count_asr_imm",
            "ror_carry_source",
            "straightline_70",
            "mpyr_shift_variants",
        ),
        sealed=False,
    ),
    # JIT sequence-state shapes where step and block mode can diverge
    # (stale promoted registers after helper calls inside inline loops;
    # deferred-SM sticky-L clobber), pinned against silicon.
    "b13": Bank(
        cases=(
            "sm_chain_deferred_l",
            "rep_ssh_body_pops",
            "do_limit_store_sr",
            "rep_limit_store_jcc",
        ),
        sealed=False,
    ),
    # Inline-loop codegen regression shapes from step-vs-block fuzzing
    # (annul-path state loss, zero-iteration clobbers, loop-carried
    # conditional writes), pinned against silicon.
    "b14": Bank(
        cases=(
            "do_annul_regs",
            "do_annul_zero_clobber",
            "rep_zero_pm_body",
            "tcc_loop_carried",
        ),
        sealed=False,
    ),
    # Dirty-state bank: first bank with a deterministic affine memory
    # fill (see Bank.fill) instead of the zero-fill. Cases read cells
    # they never wrote - deterministic and address-unique under the
    # fill, so wrong-address/wrong-space reads and writes diverge.
    "b15": Bank(
        cases=(
            "dirty_probe_x",
            "dirty_probe_y",
            "dirty_stack_ramp",
            "dirty_rmw",
            "dirty_mod_wrap",
            "dirty_bitrev_walk",
            "dirty_pm8_read",
            "dirty_lmove_read",
            "dirty_write_zero",
            "dirty_partial_overwrite",
            "dirty_do_dyn",
            "dirty_movec_lc",
            "dirty_indexed_n",
        ),
        sealed=False,
        fill=(0x00A50D, 0x03157E, 0x00C3A9, 0x2B0C99, 0x0000A0, 0x0000C0),
    ),
    # Stale SM needs_sat marker leaking V/L through a later
    # non-saturating op's flag flush inside one block, plus the plain-IFcc
    # pending-flag discard, pinned against silicon.
    "b16": Bank(
        cases=("ifcc_sm_stale_sat",),
        sealed=False,
        fill=(0x00A50D, 0x03157E, 0x00C3A9, 0x2B0C99, 0x0000A0, 0x0000C0),
    ),
    # Stack-error exception delivery (VBA-redirect fault cases, see
    # cases_faults.py). The aux blob is the bank's vector page at
    # P:$0600 (VBA=$000600 -> stack-error vector at $0602) plus the
    # shared handler: record SP/SR into n6/n7, RTI back into the case.
    "b17": Bank(
        cases=(
            "fault_btst_ssh_sp0",
            "fault_read_ssh_sp0",
            "fault_bset_ssh_sp0",
            "fault_sr_mask",
            "fault_straddle",
            "fault_push_overflow",
            "fault_pop_mem",
        ),
        sealed=False,
        fill=(0x00A50D, 0x03157E, 0x00C3A9, 0x2B0C99, 0x0000A0, 0x0000C0),
        aux=(
            "        org     p:$0602",
            "        jsr     >$000640",
            "        org     p:$0640",
            "        movec sp,n6",
            "        movec sr,n7",
            "        rti",
        ),
    ),
    # Open bank: new cases land here until captured and sealed.
    "b18": Bank(cases=("do_body_asr_limit_l",), sealed=False),
    # Branch-class + SP-write fault delivery (stream-word budget model,
    # silicon-pinned): JSR overflow rides its target, SE-bit
    # SP writes fault at start+6 flat, RTS/RTI underflow branch to
    # slot-0 storage with a 2-word window. Same vector page / handler
    # shape as b17.
    "b19": Bank(
        cases=(
            "fault_jsr_overflow",
            "fault_sp_write_se",
            "fault_sp_write_se_2w",
            "fault_rts_underflow",
            "fault_rti_underflow",
        ),
        sealed=False,
        fill=(0x00A50D, 0x03157E, 0x00C3A9, 0x2B0C99, 0x0000A0, 0x0000C0),
        aux=(
            "        org     p:$0602",
            "        jsr     >$000640",
            "        org     p:$0640",
            "        movec sp,n6",
            "        movec sr,n7",
            "        rti",
        ),
    ),
    # Stale-SM-marker discard path (see cases_srmodes.py).
    "b20": Bank(cases=("sm_marker_sr_overwrite",), sealed=False),
    # ILLEGAL/TRAP vector map + fault corners (probe rounds:
    # ILLEGAL VBA:$04 / TRAP VBA:$08 with ZERO shadow budget, fast-vector
    # shapes, DO-overflow budget 3, ENDDO-underflow budget 5, SP bit-op
    # SE/UF classes, JSR-overflow slot-0 wrap-write). Aux carries the
    # b17-style long-vector page at $0600 (stack-error $0602, ILLEGAL
    # $0604, TRAP $0608 -> shared recording handler at $0640) plus a
    # fast-vector page at $0680 (two plain marker words per slot).
    "b21": Bank(
        cases=(
            "fault_illegal",
            "fault_trap",
            "fault_trapcc_taken",
            "fault_trapcc_nottaken",
            "fault_ill_fastvec",
            "fault_se_fastvec",
            "fault_do_overflow",
            "fault_enddo_underflow",
            "fault_sp_bset_se",
            "fault_sp_bset_uf",
            "fault_jsr_ovf_slot0",
        ),
        sealed=False,
        fill=(0x00A50D, 0x03157E, 0x00C3A9, 0x2B0C99, 0x0000A0, 0x0000C0),
        aux=(
            "        org     p:$0602",
            "        jsr     >$000640",
            "        org     p:$0604",
            "        jsr     >$000640",
            "        org     p:$0608",
            "        jsr     >$000640",
            "        org     p:$0640",
            "        movec sp,n6",
            "        movec sr,n7",
            "        rti",
            "        org     p:$0682",
            "        move #$61,n6",
            "        move #$62,n7",
            "        org     p:$0684",
            "        move #$63,n4",
            "        move #$64,n5",
        ),
    ),
    "b22": Bank(
        cases=(
            "rep_mod_store",
            "rep_bitrev_store",
            "do_mod_store",
            "do_nested_shared_la",
            "do_nested_shared_la3",
            "do_nested_shared_la_lc1",
        ),
        sealed=False,
        fill=(0x00B71D, 0x0517A3, 0x00D9E5, 0x1F4B27, 0x000050, 0x000080),
    ),
    # Open bank: new cases land here until captured and sealed.
    "b18": Bank(cases=("do_body_asr_limit_l",), sealed=False),
    # Branch-class + SP-write fault delivery (stream-word budget model,
    # silicon-pinned): JSR overflow rides its target, SE-bit
    # SP writes fault at start+6 flat, RTS/RTI underflow branch to
    # slot-0 storage with a 2-word window. Same vector page / handler
    # shape as b17.
    "b19": Bank(
        cases=(
            "fault_jsr_overflow",
            "fault_sp_write_se",
            "fault_sp_write_se_2w",
            "fault_rts_underflow",
            "fault_rti_underflow",
        ),
        sealed=False,
        fill=(0x00A50D, 0x03157E, 0x00C3A9, 0x2B0C99, 0x0000A0, 0x0000C0),
        aux=(
            "        org     p:$0602",
            "        jsr     >$000640",
            "        org     p:$0640",
            "        movec sp,n6",
            "        movec sr,n7",
            "        rti",
        ),
    ),
    # Stale-SM-marker discard path (see cases_srmodes.py).
    "b20": Bank(cases=("sm_marker_sr_overwrite",), sealed=False),
    # ILLEGAL/TRAP vector map + fault corners (probe rounds:
    # ILLEGAL VBA:$04 / TRAP VBA:$08 with ZERO shadow budget, fast-vector
    # shapes, DO-overflow budget 3, ENDDO-underflow budget 5, SP bit-op
    # SE/UF classes, JSR-overflow slot-0 wrap-write). Aux carries the
    # b17-style long-vector page at $0600 (stack-error $0602, ILLEGAL
    # $0604, TRAP $0608 -> shared recording handler at $0640) plus a
    # fast-vector page at $0680 (two plain marker words per slot).
    "b21": Bank(
        cases=(
            "fault_illegal",
            "fault_trap",
            "fault_trapcc_taken",
            "fault_trapcc_nottaken",
            "fault_ill_fastvec",
            "fault_se_fastvec",
            "fault_do_overflow",
            "fault_enddo_underflow",
            "fault_sp_bset_se",
            "fault_sp_bset_uf",
            "fault_jsr_ovf_slot0",
        ),
        sealed=False,
        fill=(0x00A50D, 0x03157E, 0x00C3A9, 0x2B0C99, 0x0000A0, 0x0000C0),
        aux=(
            "        org     p:$0602",
            "        jsr     >$000640",
            "        org     p:$0604",
            "        jsr     >$000640",
            "        org     p:$0608",
            "        jsr     >$000640",
            "        org     p:$0640",
            "        movec sp,n6",
            "        movec sr,n7",
            "        rti",
            "        org     p:$0682",
            "        move #$61,n6",
            "        move #$62,n7",
            "        org     p:$0684",
            "        move #$63,n4",
            "        move #$64,n5",
        ),
    ),
    "b22": Bank(
        cases=(
            "rep_mod_store",
            "rep_bitrev_store",
            "do_mod_store",
            "do_nested_shared_la",
            "do_nested_shared_la3",
            "do_nested_shared_la_lc1",
        ),
        sealed=False,
        fill=(0x00B71D, 0x0517A3, 0x00D9E5, 0x1F4B27, 0x000050, 0x000080),
    ),
    # Open bank: new cases land here until captured and sealed.
    "b18": Bank(cases=("do_body_asr_limit_l",), sealed=False),
    # Branch-class + SP-write fault delivery (stream-word budget model,
    # silicon-pinned): JSR overflow rides its target, SE-bit
    # SP writes fault at start+6 flat, RTS/RTI underflow branch to
    # slot-0 storage with a 2-word window. Same vector page / handler
    # shape as b17.
    "b19": Bank(
        cases=(
            "fault_jsr_overflow",
            "fault_sp_write_se",
            "fault_sp_write_se_2w",
            "fault_rts_underflow",
            "fault_rti_underflow",
        ),
        sealed=False,
        fill=(0x00A50D, 0x03157E, 0x00C3A9, 0x2B0C99, 0x0000A0, 0x0000C0),
        aux=(
            "        org     p:$0602",
            "        jsr     >$000640",
            "        org     p:$0640",
            "        movec sp,n6",
            "        movec sr,n7",
            "        rti",
        ),
    ),
    # Stale-SM-marker discard path (see cases_srmodes.py).
    "b20": Bank(cases=("sm_marker_sr_overwrite",), sealed=False),
    # ILLEGAL/TRAP vector map + fault corners (probe rounds:
    # ILLEGAL VBA:$04 / TRAP VBA:$08 with ZERO shadow budget, fast-vector
    # shapes, DO-overflow budget 3, ENDDO-underflow budget 5, SP bit-op
    # SE/UF classes, JSR-overflow slot-0 wrap-write). Aux carries the
    # b17-style long-vector page at $0600 (stack-error $0602, ILLEGAL
    # $0604, TRAP $0608 -> shared recording handler at $0640) plus a
    # fast-vector page at $0680 (two plain marker words per slot).
    "b21": Bank(
        cases=(
            "fault_illegal",
            "fault_trap",
            "fault_trapcc_taken",
            "fault_trapcc_nottaken",
            "fault_ill_fastvec",
            "fault_se_fastvec",
            "fault_do_overflow",
            "fault_enddo_underflow",
            "fault_sp_bset_se",
            "fault_sp_bset_uf",
            "fault_jsr_ovf_slot0",
        ),
        sealed=False,
        fill=(0x00A50D, 0x03157E, 0x00C3A9, 0x2B0C99, 0x0000A0, 0x0000C0),
        aux=(
            "        org     p:$0602",
            "        jsr     >$000640",
            "        org     p:$0604",
            "        jsr     >$000640",
            "        org     p:$0608",
            "        jsr     >$000640",
            "        org     p:$0640",
            "        movec sp,n6",
            "        movec sr,n7",
            "        rti",
            "        org     p:$0682",
            "        move #$61,n6",
            "        move #$62,n7",
            "        org     p:$0684",
            "        move #$63,n4",
            "        move #$64,n5",
        ),
    ),
    "b22": Bank(
        cases=(
            "rep_mod_store",
            "rep_bitrev_store",
            "do_mod_store",
            "do_nested_shared_la",
            "do_nested_shared_la3",
            "do_nested_shared_la_lc1",
        ),
        sealed=False,
        fill=(0x00B71D, 0x0517A3, 0x00D9E5, 0x1F4B27, 0x000050, 0x000080),
    ),
    # Open bank: new cases land here until captured and sealed.
    "b18": Bank(cases=("do_body_asr_limit_l",), sealed=False),
    # Branch-class + SP-write fault delivery (stream-word budget model,
    # silicon-pinned): JSR overflow rides its target, SE-bit
    # SP writes fault at start+6 flat, RTS/RTI underflow branch to
    # slot-0 storage with a 2-word window. Same vector page / handler
    # shape as b17.
    "b19": Bank(
        cases=(
            "fault_jsr_overflow",
            "fault_sp_write_se",
            "fault_sp_write_se_2w",
            "fault_rts_underflow",
            "fault_rti_underflow",
        ),
        sealed=False,
        fill=(0x00A50D, 0x03157E, 0x00C3A9, 0x2B0C99, 0x0000A0, 0x0000C0),
        aux=(
            "        org     p:$0602",
            "        jsr     >$000640",
            "        org     p:$0640",
            "        movec sp,n6",
            "        movec sr,n7",
            "        rti",
        ),
    ),
    # Stale-SM-marker discard path (see cases_srmodes.py).
    "b20": Bank(cases=("sm_marker_sr_overwrite",), sealed=False),
    # ILLEGAL/TRAP vector map + fault corners (probe rounds:
    # ILLEGAL VBA:$04 / TRAP VBA:$08 with ZERO shadow budget, fast-vector
    # shapes, DO-overflow budget 3, ENDDO-underflow budget 5, SP bit-op
    # SE/UF classes, JSR-overflow slot-0 wrap-write). Aux carries the
    # b17-style long-vector page at $0600 (stack-error $0602, ILLEGAL
    # $0604, TRAP $0608 -> shared recording handler at $0640) plus a
    # fast-vector page at $0680 (two plain marker words per slot).
    "b21": Bank(
        cases=(
            "fault_illegal",
            "fault_trap",
            "fault_trapcc_taken",
            "fault_trapcc_nottaken",
            "fault_ill_fastvec",
            "fault_se_fastvec",
            "fault_do_overflow",
            "fault_enddo_underflow",
            "fault_sp_bset_se",
            "fault_sp_bset_uf",
            "fault_jsr_ovf_slot0",
        ),
        sealed=False,
        fill=(0x00A50D, 0x03157E, 0x00C3A9, 0x2B0C99, 0x0000A0, 0x0000C0),
        aux=(
            "        org     p:$0602",
            "        jsr     >$000640",
            "        org     p:$0604",
            "        jsr     >$000640",
            "        org     p:$0608",
            "        jsr     >$000640",
            "        org     p:$0640",
            "        movec sp,n6",
            "        movec sr,n7",
            "        rti",
            "        org     p:$0682",
            "        move #$61,n6",
            "        move #$62,n7",
            "        org     p:$0684",
            "        move #$63,n4",
            "        move #$64,n5",
        ),
    ),
    "b22": Bank(
        cases=(
            "rep_mod_store",
            "rep_bitrev_store",
            "do_mod_store",
            "do_nested_shared_la",
            "do_nested_shared_la3",
            "do_nested_shared_la_lc1",
        ),
        sealed=False,
        fill=(0x00B71D, 0x0517A3, 0x00D9E5, 0x1F4B27, 0x000050, 0x000080),
    ),
    # Open bank: new cases land here until captured and sealed.
    # Armed-budget x REP interaction; the vector page's handler also
    # records LC so a mid-REP delivery is distinguishable from a
    # completed REP (see cases_faults.py, b23 block).
    "b23": Bank(
        cases=(
            "fault_rep_after_budget",
            "fault_rep_shadow_mid",
            "fault_rep_shadow_edge",
            "fault_rep_shadow_big",
            "fault_rep_shadow_zero",
            "fault_rep_probe_base",
            "fault_rep_probe_d0",
            "fault_rep_probe_d1",
            "fault_rep_probe_d2",
            "fault_rep_probe_d3",
            "fault_rep_probe_d4",
            "fault_rep_probe_d5",
            "fault_rep_probe_d6",
            "fault_rep_probe_d7",
            "fault_rep_probe_d1c1",
            "fault_rep_probe_d1c0",
            "fault_rep_reader",
        ),
        sealed=False,
        fill=(0x00C97B, 0x0713E9, 0x00E1A7, 0x35A4D1, 0x000070, 0x0000B0),
        aux=(
            "        org     p:$0602",
            "        jsr     >$000640",
            "        org     p:$0640",
            "        movec sp,n6",
            "        movec sr,n7",
            "        movec lc,n5",
            "        rti",
        ),
    ),
}

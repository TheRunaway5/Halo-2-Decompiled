use super::{inst_len_for_ea, *};

impl<'a> Emitter<'a> {
    pub(super) fn emit_nop(&mut self) {
        self.set_inst_len(1);
        self.set_cycles(1);
    }

    pub(super) fn emit_illegal(&mut self) {
        self.set_inst_len(1);
        self.set_cycles(5);
        self.emit_add_interrupt(interrupt::ILLEGAL);
        // ILLEGAL delivers with a ZERO stream-word budget: the next
        // instruction is annulled and becomes the frame's saved PC, so
        // an RTI resume SKIPS the ILLEGAL (silicon, probe_ill_vector/
        // probe_ill_rti: vector VBA:$04 per Table 2-2 - the
        // page 13-76 "P:$3E" note is a DSP56000 holdover - saved PC =
        // F+len, zero shadow words, fast-vector shape honored).
        self.emit_arm_fault_budget(0);
    }

    pub(super) fn emit_trapcc(&mut self, cc: CondCode) {
        let taken = self.eval_cc_bool(cc);

        let trap_blk = self.builder.create_block();
        let merge_blk = self.builder.create_block();

        let mut cond_state = self.begin_conditional();
        self.builder
            .ins()
            .brif(taken, trap_blk, &[], merge_blk, &[]);

        self.builder.switch_to_block(trap_blk);
        self.builder.seal_block(trap_blk);
        self.emit_add_interrupt(interrupt::TRAP);
        // TRAP/TRAPcc deliver with a ZERO stream-word budget: saved PC
        // = F+len, zero shadow words (silicon, probe_trap_vector/
        // probe_trapcc_taken: vector VBA:$08 per Table 2-2).
        self.emit_arm_fault_budget(0);
        self.end_conditional_arm(&mut cond_state);
        self.builder.ins().jump(merge_blk, &[]);

        self.builder.switch_to_block(merge_blk);
        self.builder.seal_block(merge_blk);
        // merge_conditional, not end_conditional_arm: the trap arm touches
        // no promoted registers today, but the merge is what invalidates
        // arm-modified registers if that changes.
        self.merge_conditional(&cond_state);
    }

    /// Emit IR to post an interrupt (equivalent to `add_interrupt(inter)`).
    /// Sets bit `inter` in `pending_bits[word]`.
    pub(super) fn emit_add_interrupt(&mut self, inter: usize) {
        let word = inter / 64;
        let bit = inter % 64;
        let offset = OFF_INTERRUPT_PENDING_BITS + (word as i32) * 8;
        let pending = self
            .builder
            .ins()
            .load(types::I64, Self::flags(), self.state_ptr, offset);
        let mask = self.builder.ins().iconst(types::I64, 1i64 << bit);
        let new_pending = self.builder.ins().bor(pending, mask);
        self.builder
            .ins()
            .store(Self::flags(), new_pending, self.state_ptr, offset);
    }

    pub(super) fn emit_jmp(&mut self, addr: u32) {
        self.set_inst_len(0);
        self.set_cycles(3);
        let v = self.builder.ins().iconst(types::I32, addr as i64);
        self.store_pc(v);
    }

    pub(super) fn emit_jcc(&mut self, cc: CondCode, addr: u32) {
        self.set_cycles(4);
        let taken = self.eval_cc_bool(cc);

        let taken_blk = self.builder.create_block();
        let not_taken_blk = self.builder.create_block();
        let merge_blk = self.builder.create_block();

        let mut cond_state = self.begin_conditional();
        self.builder
            .ins()
            .brif(taken, taken_blk, &[], not_taken_blk, &[]);

        self.builder.switch_to_block(taken_blk);
        self.builder.seal_block(taken_blk);
        let a = self.builder.ins().iconst(types::I32, addr as i64);
        self.store_pc(a);
        self.set_inst_len(0);
        self.end_conditional_arm(&mut cond_state);
        self.builder.ins().jump(merge_blk, &[]);

        self.builder.switch_to_block(not_taken_blk);
        self.builder.seal_block(not_taken_blk);
        self.set_inst_len(1);
        self.builder.ins().jump(merge_blk, &[]);

        self.builder.switch_to_block(merge_blk);
        self.builder.seal_block(merge_blk);
        self.merge_conditional(&cond_state);
    }

    pub(super) fn emit_jsr(&mut self, addr: u32, pc: u32) {
        self.set_inst_len(0);
        self.set_cycles(3);
        let ret = self
            .builder
            .ins()
            .iconst(types::I32, mask_pc(pc + 1) as i64);
        self.emit_interrupt_aware_stack_push(ret);
        let a = self.builder.ins().iconst(types::I32, addr as i64);
        self.store_pc(a);
    }

    pub(super) fn emit_rts(&mut self) {
        self.set_inst_len(0);
        self.set_cycles(3);
        // RTS/RTI underflow budget: 3 - len (silicon: the branch to
        // slot-0 storage executes, then 2 more stream words).
        let budget = 3u32.saturating_sub(self.cur_decode_len.max(1));
        let (ssh, _ssl) = self.stack_pop(budget);
        self.store_pc(ssh);
    }

    pub(super) fn emit_rti(&mut self) {
        self.set_inst_len(0);
        self.set_cycles(3);
        let budget = 3u32.saturating_sub(self.cur_decode_len.max(1));
        let (ssh, ssl) = self.stack_pop(budget);
        self.store_pc(ssh);
        self.store_reg(reg::SR, ssl);
    }

    pub(super) fn emit_bra(&mut self, offset: i32, pc: u32) {
        self.set_inst_len(0);
        self.set_cycles(4);
        let target = mask_pc((pc as i32 + offset) as u32);
        let v = self.builder.ins().iconst(types::I32, target as i64);
        self.store_pc(v);
    }

    pub(super) fn emit_bcc(&mut self, cc: CondCode, offset: i32, pc: u32) {
        self.set_cycles(4);
        let taken = self.eval_cc_bool(cc);

        let taken_blk = self.builder.create_block();
        let not_taken_blk = self.builder.create_block();
        let merge_blk = self.builder.create_block();

        let mut cond_state = self.begin_conditional();
        self.builder
            .ins()
            .brif(taken, taken_blk, &[], not_taken_blk, &[]);

        self.builder.switch_to_block(taken_blk);
        self.builder.seal_block(taken_blk);
        let target = mask_pc((pc as i32 + offset) as u32);
        let a = self.builder.ins().iconst(types::I32, target as i64);
        self.store_pc(a);
        self.set_inst_len(0);
        self.end_conditional_arm(&mut cond_state);
        self.builder.ins().jump(merge_blk, &[]);

        self.builder.switch_to_block(not_taken_blk);
        self.builder.seal_block(not_taken_blk);
        self.set_inst_len(1);
        self.builder.ins().jump(merge_blk, &[]);

        self.builder.switch_to_block(merge_blk);
        self.builder.seal_block(merge_blk);
        self.merge_conditional(&cond_state);
    }

    pub(super) fn emit_bra_long(&mut self, pc: u32, next_word: u32) {
        self.set_inst_len(0);
        self.set_cycles(4);
        let target = mask_pc(pc.wrapping_add(next_word));
        let v = self.builder.ins().iconst(types::I32, target as i64);
        self.store_pc(v);
    }

    pub(super) fn emit_bcc_long(&mut self, cc: CondCode, pc: u32, next_word: u32) {
        self.set_cycles(5);
        let taken = self.eval_cc_bool(cc);

        let taken_blk = self.builder.create_block();
        let not_taken_blk = self.builder.create_block();
        let merge_blk = self.builder.create_block();

        let mut cond_state = self.begin_conditional();
        self.builder
            .ins()
            .brif(taken, taken_blk, &[], not_taken_blk, &[]);

        self.builder.switch_to_block(taken_blk);
        self.builder.seal_block(taken_blk);
        let target = mask_pc(pc.wrapping_add(next_word));
        let a = self.builder.ins().iconst(types::I32, target as i64);
        self.store_pc(a);
        self.set_inst_len(0);
        self.end_conditional_arm(&mut cond_state);
        self.builder.ins().jump(merge_blk, &[]);

        self.builder.switch_to_block(not_taken_blk);
        self.builder.seal_block(not_taken_blk);
        self.set_inst_len(2);
        self.end_conditional_arm(&mut cond_state);
        self.builder.ins().jump(merge_blk, &[]);

        self.builder.switch_to_block(merge_blk);
        self.builder.seal_block(merge_blk);
        self.merge_conditional(&cond_state);
    }

    pub(super) fn emit_bsr(&mut self, offset: i32, pc: u32) {
        self.set_inst_len(0);
        self.set_cycles(4);
        let ret = self
            .builder
            .ins()
            .iconst(types::I32, mask_pc(pc + 1) as i64);
        self.emit_interrupt_aware_stack_push(ret);
        let target = mask_pc((pc as i32 + offset) as u32);
        let v = self.builder.ins().iconst(types::I32, target as i64);
        self.store_pc(v);
    }

    pub(super) fn emit_bsr_long(&mut self, pc: u32, next_word: u32) {
        self.set_inst_len(0);
        self.set_cycles(5);
        let ret = self
            .builder
            .ins()
            .iconst(types::I32, mask_pc(pc + 2) as i64);
        self.emit_interrupt_aware_stack_push(ret);
        let target = mask_pc(pc.wrapping_add(next_word));
        let v = self.builder.ins().iconst(types::I32, target as i64);
        self.store_pc(v);
    }

    pub(super) fn emit_bscc(&mut self, cc: CondCode, offset: i32, pc: u32) {
        self.set_cycles(4);
        let taken = self.eval_cc_bool(cc);

        let taken_blk = self.builder.create_block();
        let not_taken_blk = self.builder.create_block();
        let merge_blk = self.builder.create_block();

        let mut cond_state = self.begin_conditional();
        self.builder
            .ins()
            .brif(taken, taken_blk, &[], not_taken_blk, &[]);

        self.builder.switch_to_block(taken_blk);
        self.builder.seal_block(taken_blk);
        let ret = self
            .builder
            .ins()
            .iconst(types::I32, mask_pc(pc + 1) as i64);
        self.emit_interrupt_aware_stack_push(ret);
        let target = mask_pc((pc as i32 + offset) as u32);
        let a = self.builder.ins().iconst(types::I32, target as i64);
        self.store_pc(a);
        self.set_inst_len(0);
        self.end_conditional_arm(&mut cond_state);
        self.builder.ins().jump(merge_blk, &[]);

        self.builder.switch_to_block(not_taken_blk);
        self.builder.seal_block(not_taken_blk);
        self.set_inst_len(1);
        self.builder.ins().jump(merge_blk, &[]);

        self.builder.switch_to_block(merge_blk);
        self.builder.seal_block(merge_blk);
        self.merge_conditional(&cond_state);
    }

    pub(super) fn emit_bscc_long(&mut self, cc: CondCode, pc: u32, next_word: u32) {
        self.set_cycles(5);
        let taken = self.eval_cc_bool(cc);

        let taken_blk = self.builder.create_block();
        let not_taken_blk = self.builder.create_block();
        let merge_blk = self.builder.create_block();

        let mut cond_state = self.begin_conditional();
        self.builder
            .ins()
            .brif(taken, taken_blk, &[], not_taken_blk, &[]);

        self.builder.switch_to_block(taken_blk);
        self.builder.seal_block(taken_blk);
        let ret = self
            .builder
            .ins()
            .iconst(types::I32, mask_pc(pc + 2) as i64);
        self.emit_interrupt_aware_stack_push(ret);
        let target = mask_pc(pc.wrapping_add(next_word));
        let a = self.builder.ins().iconst(types::I32, target as i64);
        self.store_pc(a);
        self.set_inst_len(0);
        self.end_conditional_arm(&mut cond_state);
        self.builder.ins().jump(merge_blk, &[]);

        self.builder.switch_to_block(not_taken_blk);
        self.builder.seal_block(not_taken_blk);
        self.set_inst_len(2);
        self.end_conditional_arm(&mut cond_state);
        self.builder.ins().jump(merge_blk, &[]);

        self.builder.switch_to_block(merge_blk);
        self.builder.seal_block(merge_blk);
        self.merge_conditional(&cond_state);
    }

    pub(super) fn emit_bscc_rn(&mut self, cc: CondCode, rn: u8, pc: u32) {
        self.set_cycles(4);
        let taken = self.eval_cc_bool(cc);

        let taken_blk = self.builder.create_block();
        let not_taken_blk = self.builder.create_block();
        let merge_blk = self.builder.create_block();

        let mut cond_state = self.begin_conditional();
        self.builder
            .ins()
            .brif(taken, taken_blk, &[], not_taken_blk, &[]);

        self.builder.switch_to_block(taken_blk);
        self.builder.seal_block(taken_blk);
        let ret = self
            .builder
            .ins()
            .iconst(types::I32, mask_pc(pc + 1) as i64);
        self.emit_interrupt_aware_stack_push(ret);
        let rn_val = self.load_reg(reg::R0 + rn as usize);
        let pc_val = self.builder.ins().iconst(types::I32, pc as i64);
        let target = self.builder.ins().iadd(pc_val, rn_val);
        let target = self.mask24(target);
        self.store_pc(target);
        self.set_inst_len(0);
        self.end_conditional_arm(&mut cond_state);
        self.builder.ins().jump(merge_blk, &[]);

        self.builder.switch_to_block(not_taken_blk);
        self.builder.seal_block(not_taken_blk);
        self.set_inst_len(1);
        self.builder.ins().jump(merge_blk, &[]);

        self.builder.switch_to_block(merge_blk);
        self.builder.seal_block(merge_blk);
        self.merge_conditional(&cond_state);
    }

    pub(super) fn emit_bra_rn(&mut self, rn: u8, pc: u32) {
        self.set_inst_len(0);
        self.set_cycles(4);
        let rn_val = self.load_reg(reg::R0 + rn as usize);
        let pc_val = self.builder.ins().iconst(types::I32, pc as i64);
        let target = self.builder.ins().iadd(pc_val, rn_val);
        let target = self.mask24(target);
        self.store_pc(target);
    }

    pub(super) fn emit_bcc_rn(&mut self, cc: CondCode, rn: u8, pc: u32) {
        self.set_cycles(4);
        let taken = self.eval_cc_bool(cc);

        let taken_blk = self.builder.create_block();
        let not_taken_blk = self.builder.create_block();
        let merge_blk = self.builder.create_block();

        let mut cond_state = self.begin_conditional();
        self.builder
            .ins()
            .brif(taken, taken_blk, &[], not_taken_blk, &[]);

        self.builder.switch_to_block(taken_blk);
        self.builder.seal_block(taken_blk);
        let rn_val = self.load_reg(reg::R0 + rn as usize);
        let pc_val = self.builder.ins().iconst(types::I32, pc as i64);
        let target = self.builder.ins().iadd(pc_val, rn_val);
        let target = self.mask24(target);
        self.store_pc(target);
        self.set_inst_len(0);
        self.end_conditional_arm(&mut cond_state);
        self.builder.ins().jump(merge_blk, &[]);

        self.builder.switch_to_block(not_taken_blk);
        self.builder.seal_block(not_taken_blk);
        self.set_inst_len(1);
        self.builder.ins().jump(merge_blk, &[]);

        self.builder.switch_to_block(merge_blk);
        self.builder.seal_block(merge_blk);
        self.merge_conditional(&cond_state);
    }

    pub(super) fn emit_bsr_rn(&mut self, rn: u8, pc: u32) {
        self.set_inst_len(0);
        self.set_cycles(4);
        let ret = self
            .builder
            .ins()
            .iconst(types::I32, mask_pc(pc + 1) as i64);
        self.emit_interrupt_aware_stack_push(ret);
        let rn_val = self.load_reg(reg::R0 + rn as usize);
        let pc_val = self.builder.ins().iconst(types::I32, pc as i64);
        let target = self.builder.ins().iadd(pc_val, rn_val);
        let target = self.mask24(target);
        self.store_pc(target);
    }

    pub(super) fn emit_jmp_ea(&mut self, ea_mode: u8, next_word: u32) {
        self.set_inst_len(0);
        self.set_cycles(3);
        let (addr, _) = self.emit_calc_ea_ext(ea_mode as u32, next_word);
        self.store_pc(addr);
    }

    pub(super) fn emit_jsr_ea(&mut self, ea_mode: u8, pc: u32, next_word: u32) {
        self.set_inst_len(0);
        self.set_cycles(3);
        let (addr, _) = self.emit_calc_ea_ext(ea_mode as u32, next_word);
        // Return address: for mode 6 (2-word), ret = pc+2; otherwise pc+1
        let mode = (ea_mode >> 3) & 0x7;
        let ret_offset = if mode == 6 { 2 } else { 1 };
        let ret = self
            .builder
            .ins()
            .iconst(types::I32, mask_pc(pc + ret_offset) as i64);
        self.emit_interrupt_aware_stack_push(ret);
        self.store_pc(addr);
    }

    pub(super) fn emit_jcc_ea(&mut self, cc: CondCode, ea_mode: u8, next_word: u32) {
        self.set_cycles(4);
        let (addr, _) = self.emit_calc_ea_ext(ea_mode as u32, next_word);
        let taken = self.eval_cc_bool(cc);

        let taken_blk = self.builder.create_block();
        let not_taken_blk = self.builder.create_block();
        let merge_blk = self.builder.create_block();

        let mut cond_state = self.begin_conditional();
        self.builder
            .ins()
            .brif(taken, taken_blk, &[], not_taken_blk, &[]);

        self.builder.switch_to_block(taken_blk);
        self.builder.seal_block(taken_blk);
        self.store_pc(addr);
        self.set_inst_len(0);
        self.end_conditional_arm(&mut cond_state);
        self.builder.ins().jump(merge_blk, &[]);

        self.builder.switch_to_block(not_taken_blk);
        self.builder.seal_block(not_taken_blk);
        self.set_inst_len(inst_len_for_ea(ea_mode));
        self.end_conditional_arm(&mut cond_state);
        self.builder.ins().jump(merge_blk, &[]);

        self.builder.switch_to_block(merge_blk);
        self.builder.seal_block(merge_blk);
        self.merge_conditional(&cond_state);
    }

    pub(super) fn emit_jscc(&mut self, cc: CondCode, addr: u32, pc: u32) {
        self.set_cycles(4);
        let taken = self.eval_cc_bool(cc);

        let taken_blk = self.builder.create_block();
        let not_taken_blk = self.builder.create_block();
        let merge_blk = self.builder.create_block();

        let mut cond_state = self.begin_conditional();
        self.builder
            .ins()
            .brif(taken, taken_blk, &[], not_taken_blk, &[]);

        self.builder.switch_to_block(taken_blk);
        self.builder.seal_block(taken_blk);
        let ret_pc = self
            .builder
            .ins()
            .iconst(types::I32, mask_pc(pc + 1) as i64);
        self.emit_interrupt_aware_stack_push(ret_pc);
        let target = self.builder.ins().iconst(types::I32, addr as i64);
        self.store_pc(target);
        self.set_inst_len(0);
        self.end_conditional_arm(&mut cond_state);
        self.builder.ins().jump(merge_blk, &[]);

        self.builder.switch_to_block(not_taken_blk);
        self.builder.seal_block(not_taken_blk);
        self.set_inst_len(1);
        self.end_conditional_arm(&mut cond_state);
        self.builder.ins().jump(merge_blk, &[]);

        self.builder.switch_to_block(merge_blk);
        self.builder.seal_block(merge_blk);
        self.merge_conditional(&cond_state);
    }

    pub(super) fn emit_jscc_ea(&mut self, cc: CondCode, ea_mode: u8, pc: u32, next_word: u32) {
        self.set_cycles(4);
        let (ea_addr, _) = self.emit_calc_ea_ext(ea_mode as u32, next_word);
        let taken = self.eval_cc_bool(cc);

        let taken_blk = self.builder.create_block();
        let not_taken_blk = self.builder.create_block();
        let merge_blk = self.builder.create_block();

        let mode = (ea_mode >> 3) & 0x7;
        let ret_offset = if mode == 6 { 2 } else { 1 };

        let mut cond_state = self.begin_conditional();
        self.builder
            .ins()
            .brif(taken, taken_blk, &[], not_taken_blk, &[]);

        self.builder.switch_to_block(taken_blk);
        self.builder.seal_block(taken_blk);
        let ret_pc = self
            .builder
            .ins()
            .iconst(types::I32, mask_pc(pc + ret_offset) as i64);
        self.emit_interrupt_aware_stack_push(ret_pc);
        self.store_pc(ea_addr);
        self.set_inst_len(0);
        self.end_conditional_arm(&mut cond_state);
        self.builder.ins().jump(merge_blk, &[]);

        self.builder.switch_to_block(not_taken_blk);
        self.builder.seal_block(not_taken_blk);
        self.set_inst_len(if mode == 6 { 2 } else { 1 });
        self.end_conditional_arm(&mut cond_state);
        self.builder.ins().jump(merge_blk, &[]);

        self.builder.switch_to_block(merge_blk);
        self.builder.seal_block(merge_blk);
        self.merge_conditional(&cond_state);
    }

    pub(super) fn emit_wait(&mut self) {
        self.set_inst_len(1);
        self.set_cycles(10);
        let val = self
            .builder
            .ins()
            .iconst(types::I8, PowerState::Wait as i64);
        let off = offset_of!(DspState, power_state) as i32;
        self.builder
            .ins()
            .store(Self::flags(), val, self.state_ptr, off);
    }

    pub(super) fn emit_reset(&mut self) {
        self.set_inst_len(1);
        self.set_cycles(7);
    }

    pub(super) fn emit_stop(&mut self) {
        self.set_inst_len(1);
        self.set_cycles(10);
        let val = self
            .builder
            .ins()
            .iconst(types::I8, PowerState::Stop as i64);
        let off = offset_of!(DspState, power_state) as i32;
        self.builder
            .ins()
            .store(Self::flags(), val, self.state_ptr, off);
    }

    /// Emit a NOP-like instruction (just sets length and cycles, no side effects).
    pub(super) fn emit_nop_like(&mut self, len: u32) {
        self.set_inst_len(len);
        self.set_cycles(1);
    }

    // IFcc / IFcc.U: conditional ALU execution
    // IFcc:   0010 0000 0010 CCCC alu -- CCR never updated
    // IFcc.U: 0010 0000 0011 CCCC alu -- CCR updated if cc true
    pub(super) fn emit_ifcc(&mut self, opcode: u32, alu: &ParallelAlu, update_ccr: bool) {
        let cc_bits = (opcode >> 8) & 0xF;
        let cc = CondCode::from_bits(cc_bits);

        // Evaluate condition BEFORE ALU (uses current CCR)
        let cond = self.eval_cc(cc);
        // SR ends as a runtime select between post-ALU and the pre-ALU
        // snapshot (or is restored outright), so the quarter is current
        // after this instruction only if it already was before it - the
        // materialization forced by post_sr's load must not upgrade it.
        let pre_eunz_current = self.eunz_current;

        // Save accumulators and SR before ALU op
        let save_a = self.load_acc(Accumulator::A);
        let save_b = self.load_acc(Accumulator::B);
        // Snapshot for the splice below - never observed, only written
        // back into SR - so deferral-transparent.
        let save_sr = self.load_sr_transparent();

        // Execute ALU op (unconditionally -- modifies accumulators and flags)
        self.emit_parallel_alu(alu);

        // For IFcc: always restore SR (CCR never updated)
        // For IFcc.U: restore SR only if condition is false
        if update_ccr {
            // IFcc.U: conditionally update CCR. Loading SR materializes
            // the ALU op's pending flags (and consumes its SM marker)
            // before the select.
            let post_sr = self.load_sr_transparent();
            let sr_result = self.builder.ins().select(cond, post_sr, save_sr);
            self.store_reg(reg::SR, sr_result);
        } else {
            // IFcc: never update CCR. Discard the ALU op's still-pending
            // flag computation outright (materializing it would land on
            // top of the restored SR at the next flush) and neutralize
            // its SM saturation marker, which would otherwise OR V/L
            // into the next flag-writing op's flush.
            self.pending_flags = None;
            self.pending_sm_marker = None;
            let zero = self.builder.ins().iconst(types::I32, 0);
            self.builder.def_var(self.sm_needs_sat_var, zero);
            self.store_reg(reg::SR, save_sr);
        }

        self.eunz_current = pre_eunz_current;

        // Conditionally restore accumulators (if cc false, undo ALU writes)
        let post_a = self.load_acc(Accumulator::A);
        let a_result = self.builder.ins().select(cond, post_a, save_a);
        self.store_acc(Accumulator::A, a_result);

        let post_b = self.load_acc(Accumulator::B);
        let b_result = self.builder.ins().select(cond, post_b, save_b);
        self.store_acc(Accumulator::B, b_result);
    }

    /// Read the operand value for a bit-test instruction given the addressing mode.
    ///
    /// An SSH register operand always pops the stack (hardware-verified
    /// for every bit-test-branch variant; the manual documents it only
    /// for BRCLR/BRSET/BSCLR/BSSET, p.13-26,29,32,35).
    pub(super) fn read_bit_test_operand(&mut self, addr: &BitTestAddr) -> Value {
        match *addr {
            BitTestAddr::Pp { space, pp_offset } => {
                self.read_mem(space, 0xFFFFC0u32 + pp_offset as u32)
            }
            BitTestAddr::Qq { space, qq_offset } => {
                self.read_mem(space, PERIPH_BASE + qq_offset as u32)
            }
            BitTestAddr::Aa { space, addr } => self.read_mem(space, addr as u32),
            BitTestAddr::Reg { reg_idx } => {
                if reg_idx as usize == reg::SSH {
                    // Every bit-test read of SSH pops the stack
                    // (hardware-verified for the jump, jump-subroutine,
                    // branch, and branch-subroutine variants alike; the
                    // manual documents only the BR*/BS* forms).
                    self.emit_spill_fault_budget(FaultClass::Pop);
                    self.emit_call_extern_ret(jit_read_ssh as *const () as usize)
                } else {
                    // Plain register access - no move side effects.
                    self.load_reg(reg_idx as usize)
                }
            }
            BitTestAddr::Ea { space, ea_mode } => {
                // Mode-6 (absolute/immediate) never reaches here: the
                // decoder resolves those code points to MOVE (Rn+xxxx)
                // or Unknown (see no_mode6 in the opcode table), so the
                // wrapper's next_word=0 is fine.
                let (ea_addr, _) = self.emit_calc_ea(ea_mode as u32);
                self.read_mem_dyn(space, ea_addr)
            }
        }
    }

    /// The branch-taken predicate of a bit-test branch: read the operand
    /// (with its side effects - EA post-update, SSH pop), test the bit.
    /// Shared by the block-terminator emitter and the inline skip form.
    pub(super) fn emit_bit_test_predicate(
        &mut self,
        addr: &BitTestAddr,
        bit_num: u8,
        test_set: bool,
    ) -> Value {
        let val = self.read_bit_test_operand(addr);
        let mask = self.builder.ins().iconst(types::I32, 1i64 << bit_num);
        let masked = self.builder.ins().band(val, mask);
        let zero = self.builder.ins().iconst(types::I32, 0);
        let cc = if test_set {
            IntCC::NotEqual
        } else {
            IntCC::Equal
        };
        self.builder.ins().icmp(cc, masked, zero)
    }

    /// Branch-taken predicate (i8, brif-ready) for an instruction
    /// `forward_skip_target` accepted, charging its cycles and length.
    /// The predicate runs unconditionally, exactly as the instruction
    /// itself would evaluate it before branching.
    pub(super) fn emit_skip_predicate(&mut self, inst: &Instruction) -> Value {
        let bit = |e: &mut Self, addr: &BitTestAddr, bit_num: u8, test_set: bool, cycles: i64| {
            e.set_cycles(cycles);
            e.set_inst_len(2);
            e.emit_bit_test_predicate(addr, bit_num, test_set)
        };
        match inst {
            Instruction::BrclrEa {
                space,
                ea_mode,
                bit_num,
            } => bit(self, &BitTestAddr::ea(*space, *ea_mode), *bit_num, false, 5),
            Instruction::BrclrAa {
                space,
                addr,
                bit_num,
            } => bit(self, &BitTestAddr::aa(*space, *addr), *bit_num, false, 5),
            Instruction::BrclrPp {
                space,
                pp_offset,
                bit_num,
            } => bit(
                self,
                &BitTestAddr::pp(*space, *pp_offset),
                *bit_num,
                false,
                5,
            ),
            Instruction::BrclrQq {
                space,
                qq_offset,
                bit_num,
            } => bit(
                self,
                &BitTestAddr::qq(*space, *qq_offset),
                *bit_num,
                false,
                5,
            ),
            Instruction::BrclrReg { reg_idx, bit_num } => {
                bit(self, &BitTestAddr::reg(*reg_idx), *bit_num, false, 5)
            }
            Instruction::BrsetEa {
                space,
                ea_mode,
                bit_num,
            } => bit(self, &BitTestAddr::ea(*space, *ea_mode), *bit_num, true, 5),
            Instruction::BrsetAa {
                space,
                addr,
                bit_num,
            } => bit(self, &BitTestAddr::aa(*space, *addr), *bit_num, true, 5),
            Instruction::BrsetPp {
                space,
                pp_offset,
                bit_num,
            } => bit(
                self,
                &BitTestAddr::pp(*space, *pp_offset),
                *bit_num,
                true,
                5,
            ),
            Instruction::BrsetQq {
                space,
                qq_offset,
                bit_num,
            } => bit(
                self,
                &BitTestAddr::qq(*space, *qq_offset),
                *bit_num,
                true,
                5,
            ),
            Instruction::BrsetReg { reg_idx, bit_num } => {
                bit(self, &BitTestAddr::reg(*reg_idx), *bit_num, true, 5)
            }
            Instruction::JclrEa {
                space,
                ea_mode,
                bit_num,
            } => bit(self, &BitTestAddr::ea(*space, *ea_mode), *bit_num, false, 4),
            Instruction::JclrAa {
                space,
                addr,
                bit_num,
            } => bit(self, &BitTestAddr::aa(*space, *addr), *bit_num, false, 4),
            Instruction::JclrPp {
                space,
                pp_offset,
                bit_num,
            } => bit(
                self,
                &BitTestAddr::pp(*space, *pp_offset),
                *bit_num,
                false,
                4,
            ),
            Instruction::JclrQq {
                space,
                qq_offset,
                bit_num,
            } => bit(
                self,
                &BitTestAddr::qq(*space, *qq_offset),
                *bit_num,
                false,
                4,
            ),
            Instruction::JclrReg { reg_idx, bit_num } => {
                bit(self, &BitTestAddr::reg(*reg_idx), *bit_num, false, 4)
            }
            Instruction::JsetEa {
                space,
                ea_mode,
                bit_num,
            } => bit(self, &BitTestAddr::ea(*space, *ea_mode), *bit_num, true, 4),
            Instruction::JsetAa {
                space,
                addr,
                bit_num,
            } => bit(self, &BitTestAddr::aa(*space, *addr), *bit_num, true, 4),
            Instruction::JsetPp {
                space,
                pp_offset,
                bit_num,
            } => bit(
                self,
                &BitTestAddr::pp(*space, *pp_offset),
                *bit_num,
                true,
                4,
            ),
            Instruction::JsetQq {
                space,
                qq_offset,
                bit_num,
            } => bit(
                self,
                &BitTestAddr::qq(*space, *qq_offset),
                *bit_num,
                true,
                4,
            ),
            Instruction::JsetReg { reg_idx, bit_num } => {
                bit(self, &BitTestAddr::reg(*reg_idx), *bit_num, true, 4)
            }
            Instruction::Bcc { cc, .. } => {
                self.set_cycles(4);
                self.set_inst_len(1);
                self.eval_cc_bool(*cc)
            }
            Instruction::BccLong { cc } => {
                self.set_cycles(5);
                self.set_inst_len(2);
                self.eval_cc_bool(*cc)
            }
            _ => unreachable!("emit_skip_predicate on non-skip instruction"),
        }
    }

    /// Unified bit-test-and-branch emitter for jclr/jset, jsclr/jsset,
    /// brclr/brset, bsclr/bsset.
    pub(super) fn emit_bit_test_branch(
        &mut self,
        addr: &BitTestAddr,
        bit_num: u8,
        next_word: u32,
        test_set: bool,
        branch: BitTestBranch,
    ) {
        let cycles = match &branch {
            BitTestBranch::Jump | BitTestBranch::JumpSub { .. } => 4,
            BitTestBranch::Branch { .. } | BitTestBranch::BranchSub { .. } => 5,
        };
        self.set_cycles(cycles);
        let cond = self.emit_bit_test_predicate(addr, bit_num, test_set);

        let taken_blk = self.builder.create_block();
        let not_taken_blk = self.builder.create_block();
        let merge_blk = self.builder.create_block();

        let mut cond_state = self.begin_conditional();
        self.builder
            .ins()
            .brif(cond, taken_blk, &[], not_taken_blk, &[]);

        self.builder.switch_to_block(taken_blk);
        self.builder.seal_block(taken_blk);

        // Push return address if subroutine variant
        match &branch {
            BitTestBranch::JumpSub { pc } | BitTestBranch::BranchSub { pc } => {
                let return_addr = self
                    .builder
                    .ins()
                    .iconst(types::I32, mask_pc(*pc + 2) as i64);
                self.emit_interrupt_aware_stack_push(return_addr);
            }
            _ => {}
        }

        // Compute target address
        let target = match &branch {
            BitTestBranch::Jump | BitTestBranch::JumpSub { .. } => next_word,
            BitTestBranch::Branch { pc } | BitTestBranch::BranchSub { pc } => {
                mask_pc(pc.wrapping_add(next_word))
            }
        };
        let target_val = self.builder.ins().iconst(types::I32, target as i64);
        self.store_pc(target_val);
        self.set_inst_len(0);
        self.end_conditional_arm(&mut cond_state);
        self.builder.ins().jump(merge_blk, &[]);

        self.builder.switch_to_block(not_taken_blk);
        self.builder.seal_block(not_taken_blk);
        self.set_inst_len(2);
        self.end_conditional_arm(&mut cond_state);
        self.builder.ins().jump(merge_blk, &[]);

        self.builder.switch_to_block(merge_blk);
        self.builder.seal_block(merge_blk);
        self.merge_conditional(&cond_state);
    }
}

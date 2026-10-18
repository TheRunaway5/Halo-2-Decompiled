use super::{inst_len_for_ea, *};

impl<'a> Emitter<'a> {
    /// Apply a bit modification to `val` at `bit_num`. Returns the modified
    /// value, or None for Test (btst).
    pub(super) fn apply_bit_op(&mut self, val: Value, bit_num: u32, op: BitOp) -> Option<Value> {
        match op {
            BitOp::Clear => {
                let mask = self.builder.ins().iconst(types::I32, !(1i64 << bit_num));
                Some(self.builder.ins().band(val, mask))
            }
            BitOp::Set => {
                let mask = self.builder.ins().iconst(types::I32, 1i64 << bit_num);
                Some(self.builder.ins().bor(val, mask))
            }
            BitOp::Toggle => {
                let mask = self.builder.ins().iconst(types::I32, 1i64 << bit_num);
                Some(self.builder.ins().bxor(val, mask))
            }
            BitOp::Test => None,
        }
    }

    /// Update the carry bit in SR from a specific bit of a value.
    pub(super) fn update_carry_from_bit(&mut self, val: Value, bit_num: u32) {
        let sr_c = self.clear_sr_flags(1u32 << sr::C);
        let c_shift = self.builder.ins().iconst(types::I32, bit_num as i64);
        let bit = self.builder.ins().ushr(val, c_shift);
        let one = self.builder.ins().iconst(types::I32, 1);
        let bit = self.builder.ins().band(bit, one);
        // Carry is bit 0 of SR, so no shift needed
        let sr_new = self.builder.ins().bor(sr_c, bit);
        self.store_reg(reg::SR, sr_new);
    }

    pub(super) fn emit_bit_op_pp(
        &mut self,
        space: MemSpace,
        pp_offset: u8,
        bit_num: u8,
        op: BitOp,
    ) {
        self.set_inst_len(1);
        self.set_cycles(2);
        let pp_addr = 0xFFFFC0u32 + pp_offset as u32;
        let val = self.read_mem(space, pp_addr);
        self.update_carry_from_bit(val, bit_num as u32);
        if let Some(result) = self.apply_bit_op(val, bit_num as u32, op) {
            self.write_mem(space, pp_addr, result);
        }
    }

    pub(super) fn emit_bit_op_qq(
        &mut self,
        space: MemSpace,
        qq_offset: u8,
        bit_num: u8,
        op: BitOp,
    ) {
        self.set_inst_len(1);
        self.set_cycles(2);
        let qq_addr = PERIPH_BASE + qq_offset as u32;
        let val = self.read_mem(space, qq_addr);
        self.update_carry_from_bit(val, bit_num as u32);
        if let Some(result) = self.apply_bit_op(val, bit_num as u32, op) {
            self.write_mem(space, qq_addr, result);
        }
    }

    pub(super) fn emit_bit_op_aa(&mut self, space: MemSpace, addr: u8, bit_num: u8, op: BitOp) {
        self.set_inst_len(1);
        self.set_cycles(2);
        let val = self.read_mem(space, addr as u32);
        self.update_carry_from_bit(val, bit_num as u32);
        if let Some(result) = self.apply_bit_op(val, bit_num as u32, op) {
            self.write_mem(space, addr as u32, result);
        }
    }

    pub(super) fn emit_bit_op_reg(&mut self, reg_idx: u8, bit_num: u8, op: BitOp) {
        self.set_inst_len(1);
        self.set_cycles(2);
        // SSH is special (hardware-verified): BTST reads it like a move
        // source and pops the stack; the modifying ops rewrite the top
        // stack slot in place without touching SP. SSL modifying ops
        // likewise rewrite the top slot in place (hardware-verified
        // via the stack-walk slot dump: silicon's bchg #0,ssl
        // residue persists in the slot across pops - a mirror-only store
        // vanished at the next reload). SP goes through the same helper
        // as its move path so the SSH/SSL views are recomputed.
        // FULL-accumulator targets (a/b) go through the move path on
        // silicon (hardware-verified): limited (scaled) 24-bit
        // read, RMW on the limited value, sign-extended write-back that
        // clears a0, with L/S updated by the read. Other registers use
        // load_reg/store_reg - no limiting.
        let is_ssh = reg_idx as usize == reg::SSH;
        let is_ssl = reg_idx as usize == reg::SSL;
        let is_sp = reg_idx as usize == reg::SP;
        let is_full_acc = reg_idx as usize == reg::A || reg_idx as usize == reg::B;
        let val = if is_ssh && op == BitOp::Test {
            self.emit_spill_fault_budget(FaultClass::Pop);
            self.emit_call_extern_ret(jit_read_ssh as *const () as usize)
        } else if is_full_acc {
            self.read_reg_for_move(reg_idx as usize)
        } else {
            // For SSH/SSL this reads the top-of-stack mirror, kept
            // coherent by every push/pop.
            self.load_reg(reg_idx as usize)
        };
        if let Some(result) = self.apply_bit_op(val, bit_num as u32, op) {
            if is_ssh {
                self.emit_spill_fault_budget(FaultClass::Pop);
                self.emit_call_extern_val(jit_write_ssh_tos as *const () as usize, result);
            } else if is_ssl {
                self.emit_call_extern_val(jit_write_ssl as *const () as usize, result);
            } else if is_sp {
                self.emit_spill_fault_budget(FaultClass::SpWrite);
                self.emit_call_extern_val(jit_write_sp as *const () as usize, result);
            } else if is_full_acc {
                self.write_reg_for_move(reg_idx as usize, result);
            } else {
                self.store_reg(reg_idx as usize, result);
            }
        }
        // Per DSP56300FM: for SR target with modifying ops (BCLR/BSET/BCHG),
        // the bit operation itself modifies CCR bits directly, so we must not
        // separately update C (which would corrupt the modified SR). But for
        // BTST (read-only), C should still be updated per the standard rule.
        if reg_idx as usize != reg::SR || op == BitOp::Test {
            self.update_carry_from_bit(val, bit_num as u32);
        }
    }

    pub(super) fn emit_bit_op_ea(
        &mut self,
        space: MemSpace,
        ea_mode: u8,
        bit_num: u8,
        op: BitOp,
        next_word: u32,
    ) {
        self.set_inst_len(inst_len_for_ea(ea_mode));
        self.set_cycles(2);
        let (ea_addr, _) = self.emit_calc_ea_ext(ea_mode as u32, next_word);
        let val = self.read_mem_dyn(space, ea_addr);
        self.update_carry_from_bit(val, bit_num as u32);
        if let Some(result) = self.apply_bit_op(val, bit_num as u32, op) {
            self.write_mem_dyn(space, ea_addr, result);
        }
    }
}

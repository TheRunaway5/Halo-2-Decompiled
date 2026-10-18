use super::*;

impl<'a> Emitter<'a> {
    /// Flush any pending lazy flag computation into SR. Called automatically
    /// when SR is read via `load_reg(SR)`. The `take()` ensures re-entrant
    /// calls (from the flag computation code itself loading SR) are safe.
    fn set_pending(&mut self, flags: PendingFlags) {
        // Materialize any previously deferred computation first. Pending
        // kinds cover different CCR subsets (and L is sticky), so replacing
        // without materializing loses the earlier op's contributions — e.g.
        // add;or would drop the add's E/U/C/L, and even add;add drops the
        // first add's sticky L.
        //
        // The E/U/N/Z quarter is the exception. Reaching here means nothing
        // read SR since the outgoing computation was recorded (`load_reg`
        // flushes), so if the incoming kind rewrites all four — every kind
        // that goes through `update_nz_now`, which clears E|U|N|Z and then
        // sets them from the new result — the outgoing helper call is dead
        // and is never emitted. Its V/C/L half still is.
        let eunz_dead = flags.rewrites_eunz();
        self.flush_pending_flags_eliding_eunz(eunz_dead);
        // Snapshot the SM saturation marker for the kinds that consume it,
        // and reset the variable. The marker belongs to THIS instruction —
        // `emit_saturate_sm` runs before the flag kind is declared — so
        // capturing it here decouples it from the variable, and the next
        // instruction's `emit_saturate_sm` is then free to redefine the
        // variable without first forcing this computation to materialize.
        if flags.consumes_sm_marker() {
            self.pending_sm_marker = Some(self.builder.use_var(self.sm_needs_sat_var));
            let zero = self.builder.ins().iconst(types::I32, 0);
            self.builder.def_var(self.sm_needs_sat_var, zero);
        }
        self.pending_flags = Some(flags);
    }

    /// Materialize the pending flags.
    pub(super) fn flush_pending_flags(&mut self) {
        self.flush_pending_flags_eliding_eunz(false);
    }

    /// Flush at an inline DO's back edge, deferring the E/U/N/Z helper call
    /// to the loop exit when the pending kind allows it.
    ///
    /// Only the quarter defers: it is a pure overwrite of the result, so
    /// the last iteration's call subsumes the others. The V/C/L/SM half
    /// still materializes here, once per iteration - V and C because that
    /// IS last-write-wins per iteration, L and the SM saturation V/L
    /// because they are sticky ORs whose intermediate contributions are
    /// architectural: L set by a non-final iteration must survive.
    ///
    /// Returns the deferred call's argument; the caller must emit
    /// `emit_deferred_nz` with it on every exit from the loop.
    pub(super) fn flush_pending_flags_backedge_deferring_eunz(&mut self) -> Option<Value> {
        let result56 = self.pending_flags.as_ref().and_then(|f| f.nz_result56());
        self.flush_pending_flags_eliding_eunz(result56.is_some());
        result56
    }

    /// Materialize a deferred backedge E/U/N/Z computation
    /// (`flush_pending_flags_backedge_deferring_eunz`): the helper call the
    /// per-iteration flush elided, on the final iteration's result.
    pub(super) fn emit_deferred_nz(&mut self, result56: Value) {
        // The helper's SR access is the flush machinery's own RMW, not a
        // guest observation - keep it out of `defer_hazard_sites`.
        self.in_flag_flush = true;
        self.update_nz_maybe(result56, false);
        self.in_flag_flush = false;
    }

    fn flush_pending_flags_eliding_eunz(&mut self, eunz_dead: bool) {
        let Some(flags) = self.pending_flags.take() else {
            return;
        };
        assert!(
            self.cond_keep_flags == 0,
            "flag flush inside a keep-flags conditional arm would strand \
             the pending computation on one path (pc=${:06x})",
            self.cur_inst_pc,
        );
        // The flush's own SR loads are internal RMWs, not backedge-deferral
        // hazards - see `in_flag_flush`. Re-entrant flushes (load_reg(SR)
        // from the flag code) take() None and return above, so a plain
        // set/clear pair brackets exactly this materialization.
        self.in_flag_flush = true;
        match flags {
            PendingFlags::AluAddSub {
                result56,
                source,
                dest,
                result_raw,
                is_sub,
            } => {
                self.update_nz_maybe(result56, eunz_dead);
                self.update_vcl(source, dest, result_raw, is_sub);
                self.emit_sm_vl_deferred();
            }
            PendingFlags::NzClearV { result56 } => {
                self.update_nz_maybe(result56, eunz_dead);
                self.clear_v_flag();
            }
            PendingFlags::NzOnly { result56 } => {
                self.update_nz_maybe(result56, eunz_dead);
            }
            PendingFlags::NzClearVSm { result56 } => {
                self.update_nz_maybe(result56, eunz_dead);
                self.clear_v_flag();
                self.emit_sm_vl_deferred();
            }
            PendingFlags::MacVlSm {
                result56,
                product,
                acc,
            } => {
                self.update_nz_maybe(result56, eunz_dead);
                self.mac_set_vl(product, acc, result56);
                self.emit_sm_vl_deferred();
            }
            PendingFlags::NzVlSm { result56, overflow } => {
                self.update_nz_maybe(result56, eunz_dead);
                self.set_vl_overflow(overflow);
                self.emit_sm_vl_deferred();
            }
            PendingFlags::NzSm { result56 } => {
                self.update_nz_maybe(result56, eunz_dead);
                self.emit_sm_vl_deferred();
            }
            PendingFlags::NzVclSub {
                result56,
                source,
                dest,
                result_raw,
            } => {
                self.update_nz_maybe(result56, eunz_dead);
                self.update_vcl_sub(source, dest, result_raw);
            }
            PendingFlags::AddlSubl {
                result56,
                source,
                dest_shifted,
                result_raw,
                is_sub,
                asl_v,
            } => {
                self.update_nz_maybe(result56, eunz_dead);
                // C comes from the add/sub stage alone; the destination
                // shift's carry-out does NOT fold into C (hardware
                // carry-edge probes: (asl,add) 10->0, 01->1,
                // 11->1). The shift still contributes V/L via asl_v.
                self.update_vcl(source, dest_shifted, result_raw, is_sub);
                self.or_vl_sr(asl_v);
                self.emit_sm_vl_deferred();
            }
            PendingFlags::DmacVl {
                result56,
                product,
                acc,
            } => {
                self.update_nz_maybe(result56, eunz_dead);
                self.mac_set_vl(product, acc, result56);
            }
            PendingFlags::Shift24 {
                carry,
                n_val,
                result,
            } => {
                self.update_shift24_flags(carry, n_val, result);
            }
            PendingFlags::Logical { result24 } => {
                self.update_nzv_logical(result24);
            }
        }
        self.in_flag_flush = false;
    }

    /// Set pending EUNZ + VCL flags for a standard add/sub ALU operation.
    /// Replaces any previously pending flags.
    pub(super) fn set_flags_addsub(
        &mut self,
        result56: Value,
        source: Value,
        dest: Value,
        result_raw: Value,
        is_sub: bool,
    ) {
        self.set_pending(PendingFlags::AluAddSub {
            result56,
            source,
            dest,
            result_raw,
            is_sub,
        });
    }

    /// Set pending EUNZ flags with V cleared (TST pattern).
    pub(super) fn set_flags_nz_clear_v(&mut self, result56: Value) {
        self.set_pending(PendingFlags::NzClearV { result56 });
    }

    pub(super) fn set_flags_nz(&mut self, result56: Value) {
        self.set_pending(PendingFlags::NzOnly { result56 });
    }

    pub(super) fn set_flags_nz_clear_v_sm(&mut self, result56: Value) {
        self.set_pending(PendingFlags::NzClearVSm { result56 });
    }

    pub(super) fn set_flags_mac_vl_sm(&mut self, result56: Value, product: Value, acc: Value) {
        self.set_pending(PendingFlags::MacVlSm {
            result56,
            product,
            acc,
        });
    }

    pub(super) fn set_flags_nz_vl_sm(&mut self, result56: Value, overflow: Value) {
        self.set_pending(PendingFlags::NzVlSm { result56, overflow });
    }

    pub(super) fn set_flags_nz_sm(&mut self, result56: Value) {
        self.set_pending(PendingFlags::NzSm { result56 });
    }

    pub(super) fn set_flags_nz_vcl_sub(
        &mut self,
        result56: Value,
        source: Value,
        dest: Value,
        result_raw: Value,
    ) {
        self.set_pending(PendingFlags::NzVclSub {
            result56,
            source,
            dest,
            result_raw,
        });
    }

    #[allow(clippy::too_many_arguments)]
    pub(super) fn set_flags_addl_subl(
        &mut self,
        result56: Value,
        source: Value,
        dest_shifted: Value,
        result_raw: Value,
        is_sub: bool,
        asl_v: Value,
    ) {
        self.set_pending(PendingFlags::AddlSubl {
            result56,
            source,
            dest_shifted,
            result_raw,
            is_sub,
            asl_v,
        });
    }

    pub(super) fn set_flags_dmac_vl(&mut self, result56: Value, product: Value, acc: Value) {
        self.set_pending(PendingFlags::DmacVl {
            result56,
            product,
            acc,
        });
    }

    pub(super) fn set_flags_shift24(&mut self, carry: Value, n_val: Option<Value>, result: Value) {
        self.set_pending(PendingFlags::Shift24 {
            carry,
            n_val,
            result,
        });
    }

    pub(super) fn set_flags_logical(&mut self, result24: Value) {
        self.set_pending(PendingFlags::Logical { result24 });
    }

    /// Load SR and clear the given flag bits. Returns the cleared SR value.
    /// `flags` is a bitmask, e.g. `(1u32 << sr::N) | (1u32 << sr::Z)`.
    pub(super) fn clear_sr_flags(&mut self, flags: u32) -> Value {
        // Every caller is a pure RMW chain (clear, OR in computed bits,
        // store back to SR) - deferral-transparent.
        let sr = self.load_sr_transparent();
        let mask = self.builder.ins().iconst(types::I32, !flags as i64);
        self.builder.ins().band(sr, mask)
    }

    /// Lazy EUNZ update: stores result for deferred computation.
    pub(super) fn update_nz(&mut self, acc_val: Value) {
        self.set_flags_nz(acc_val);
    }

    /// Immediate EUNZ update via extern helper. A single native call rather
    /// than inline IR: a program that pages overlays rebuilds tens of
    /// thousands of blocks a second, so what a site costs to emit is not a
    /// rounding error.
    fn update_nz_now(&mut self, acc_val: Value) {
        use crate::core::jit_update_nz;
        let real = jit_update_nz as *const () as usize;
        self.emit_call_sr_helper_pure(real, acc_val);
        // The helper rewrites all four E/U/N/Z bits from acc_val, so from
        // here on this straight-line path SR's quarter is architecturally
        // current - but only if this point dominates what follows (not
        // inside a conditional arm). See `eunz_current`.
        if self.cond_depth == 0 {
            self.eunz_current = true;
        }
    }

    /// `update_nz_now` unless the caller has established that E/U/N/Z are
    /// dead — rewritten in full before anything can read them.
    fn update_nz_maybe(&mut self, acc_val: Value, dead: bool) {
        if !dead {
            self.update_nz_now(acc_val);
        }
    }

    /// Update CCR flags for logical operations (AND, OR, EOR, NOT).
    ///
    /// Logical ops only clear N/Z/V and set N from bit 23 of the 24-bit
    /// result, Z if result == 0. E, U, C, L are left untouched.
    pub(super) fn update_nzv_logical(&mut self, result24: Value) {
        // Clear N, Z, V
        let sr_new = self.clear_sr_flags((1u32 << sr::N) | (1u32 << sr::Z) | (1u32 << sr::V));

        // N = (result >> 23) & 1
        let c23 = self.builder.ins().iconst(types::I32, 23);
        let n_raw = self.builder.ins().ushr(result24, c23);
        let one = self.builder.ins().iconst(types::I32, 1);
        let n_raw = self.builder.ins().band(n_raw, one);
        let n_bit = self.shift_to_bit(n_raw, sr::N);
        let sr_new = self.builder.ins().bor(sr_new, n_bit);

        // Z = (result == 0)
        let zero = self.builder.ins().iconst(types::I32, 0);
        let is_zero = self.builder.ins().icmp(IntCC::Equal, result24, zero);
        let z_val = self.builder.ins().uextend(types::I32, is_zero);
        let z_bit = self.shift_to_bit(z_val, sr::Z);
        let sr_new = self.builder.ins().bor(sr_new, z_bit);

        self.store_reg(reg::SR, sr_new);
    }

    /// Shift `overflow` (0/1 i32) to V and L bit positions and OR into `sr`.
    pub(super) fn or_vl(&mut self, sr: Value, overflow: Value) -> Value {
        let v_bit = self.shift_to_bit(overflow, sr::V);
        let l_bit = self.shift_to_bit(overflow, sr::L);
        let sr = self.builder.ins().bor(sr, v_bit);
        self.builder.ins().bor(sr, l_bit)
    }

    /// Update V and L from an overflow bit. Clears V, sets V and L (sticky).
    /// Does NOT modify C.
    pub(super) fn set_vl_overflow(&mut self, overflow: Value) {
        let sr_new = self.clear_sr_flags(1u32 << sr::V);
        let sr_new = self.or_vl(sr_new, overflow);
        self.store_reg(reg::SR, sr_new);
    }

    /// Arithmetic Saturation Mode via extern helper. Returns bits 55:0 as the
    /// (possibly clamped) value. Bit 56 of the helper's return encodes needs_sat.
    pub(super) fn emit_saturate_sm(&mut self, result: Value) -> Value {
        use crate::core::jit_saturate_sm;

        // A pending CCR computation is deliberately NOT materialized here.
        // `set_pending` snapshots the SM marker when it records an *Sm
        // kind, so redefining sm_needs_sat_var below cannot strand one:
        // without the snapshot a pending *Sm kind would flush later with
        // THIS instruction's needs_sat and lose the earlier saturation's
        // sticky L/V (two back-to-back SM adds where only the first
        // saturates, with no SR read in between). Nothing else here needs
        // the CCR: the helper reads SR for SM alone, and no pending
        // computation writes SM.
        // Flush SR so the helper can read SM. Don't invalidate — SM doesn't change.
        self.flush_reg(reg::SR);
        self.promoted.dirty[reg::SR] = false;

        let fn_ptr = self
            .builder
            .ins()
            .iconst(self.ptr_ty, jit_saturate_sm as *const () as usize as i64);
        let mut sig = Signature::new(HOST_CALL_CONV);
        sig.params.push(AbiParam::new(self.ptr_ty));
        sig.params.push(AbiParam::new(types::I64));
        sig.returns.push(AbiParam::new(types::I64));
        let sig_ref = self.builder.import_signature(sig);
        let call = self
            .builder
            .ins()
            .call_indirect(sig_ref, fn_ptr, &[self.state_ptr, result]);
        let packed = self.builder.inst_results(call)[0];

        // Extract needs_sat (bit 56) for deferred V/L update
        let needs_sat = self.extract_bit_i64(packed, 56);
        self.builder.def_var(self.sm_needs_sat_var, needs_sat);

        // Mask to 56 bits
        self.mask56(packed)
    }

    /// SM saturation for ROUNDING instructions (RND/MPYR/MACR families).
    /// Silicon clamps these to the rounded grid: the low 24 bits of the
    /// saturation constant are zero ($7FFFFF:000000 positive rail), unlike
    /// the plain-op clamp ($7FFFFF:FFFFFF). Verified on MCPX hardware via
    /// SM-mode RND and MACR probes (see ARCHITECTURE-NOTES.md).
    pub(super) fn emit_saturate_sm_rnd(&mut self, result: Value) -> Value {
        let sat = self.emit_saturate_sm(result);
        let needs_sat = self.builder.use_var(self.sm_needs_sat_var);
        // The marker variable is I32; an I64 zero here is a type error the
        // Cranelift verifier rejects in debug builds.
        let zero = self.builder.ins().iconst(types::I32, 0);
        let did_sat = self.builder.ins().icmp(IntCC::NotEqual, needs_sat, zero);
        let low_mask = self.builder.ins().iconst(types::I64, !0xFFFFFFi64);
        let low_mask = self.mask56(low_mask);
        let zeroed = self.builder.ins().band(sat, low_mask);
        self.builder.ins().select(did_sat, zeroed, sat)
    }

    /// Apply deferred V/L flags from a previous `emit_saturate_sm` call.
    /// Call AFTER the instruction's normal flag computation (update_nz, update_vcl, etc.).
    pub(super) fn emit_sm_vl_deferred(&mut self) {
        // The marker was snapshotted (and the variable re-zeroed) when the
        // pending kind was recorded. Several kinds carry the SM term but
        // belong to ops that never saturate (cmp is AluAddSub), and inside
        // a multi-instruction block the variable is not re-zeroed per
        // instruction otherwise - a stale needs_sat would OR V/L into the
        // next flag flush.
        //
        // `set_pending` is the only place a pending kind is recorded, and
        // it snapshots for exactly the kinds that reach here, so the
        // fallback is unreachable; it reads the variable rather than
        // panicking.
        let needs_sat = match self.pending_sm_marker.take() {
            Some(v) => v,
            None => self.builder.use_var(self.sm_needs_sat_var),
        };
        let sr = self.load_reg(reg::SR);
        let sr = self.or_vl(sr, needs_sat);
        self.store_reg(reg::SR, sr);
    }

    /// Compute V from MAC accumulation overflow (C unchanged).
    /// Uses standard signed overflow formula: both input signs agree but differ from result.
    pub(super) fn mac_set_vl(&mut self, product: Value, acc: Value, result: Value) {
        let sign_s = self.extract_bit_i64(product, 55);
        let sign_d = self.extract_bit_i64(acc, 55);
        let sign_r = self.extract_bit_i64(result, 55);
        let sr_xr = self.builder.ins().bxor(sign_s, sign_r);
        let dr_xr = self.builder.ins().bxor(sign_d, sign_r);
        let overflow = self.builder.ins().band(sr_xr, dr_xr);
        self.set_vl_overflow(overflow);
    }

    /// Update SR flags for 24-bit shift/rotate: clear C|N|Z|V, then set C from
    /// `carry` (bit-0 value), optionally set N from `n_val` (bit-0 value), set Z
    /// from `result == 0`. V is always cleared.
    pub(super) fn update_shift24_flags(
        &mut self,
        carry: Value,
        n_val: Option<Value>,
        result: Value,
    ) {
        let sr_new = self
            .clear_sr_flags((1u32 << sr::C) | (1u32 << sr::N) | (1u32 << sr::Z) | (1u32 << sr::V));
        // C = carry (already in bit 0 position, same as sr::C)
        let sr_new = self.builder.ins().bor(sr_new, carry);
        // N (if provided)
        let sr_new = if let Some(n) = n_val {
            let n_bit = self.shift_to_bit(n, sr::N);
            self.builder.ins().bor(sr_new, n_bit)
        } else {
            sr_new
        };
        // Z = (result == 0)
        let zero = self.builder.ins().iconst(types::I32, 0);
        let is_zero = self.builder.ins().icmp(IntCC::Equal, result, zero);
        let z_val = self.builder.ins().uextend(types::I32, is_zero);
        let z_bit = self.shift_to_bit(z_val, sr::Z);
        let sr_new = self.builder.ins().bor(sr_new, z_bit);
        self.store_reg(reg::SR, sr_new);
    }

    /// Update V, C, L flags for a 56-bit add or subtract.
    ///
    /// `source` and `dest` are the original 56-bit values.
    /// `result` is the unmasked i64 result. `is_sub` selects the overflow formula.
    pub(super) fn update_vcl(&mut self, source: Value, dest: Value, result: Value, is_sub: bool) {
        // Carry = bit 56 of the unmasked result
        let carry = self.extract_bit_i64(result, 56);

        // Extract sign bits (bit 55)
        let sign_s = self.extract_bit_i64(source, 55);
        let sign_d = self.extract_bit_i64(dest, 55);
        let result56 = self.mask56(result);
        let sign_r = self.extract_bit_i64(result56, 55);

        // Overflow formula differs for add vs sub
        let overflow = if is_sub {
            // Sub: (sign_s ^ sign_d) & (sign_r ^ sign_d)
            let sd = self.builder.ins().bxor(sign_s, sign_d);
            let rd = self.builder.ins().bxor(sign_r, sign_d);
            self.builder.ins().band(sd, rd)
        } else {
            // Add: (sign_s ^ sign_r) & (sign_d ^ sign_r)
            let sr = self.builder.ins().bxor(sign_s, sign_r);
            let dr = self.builder.ins().bxor(sign_d, sign_r);
            self.builder.ins().band(sr, dr)
        };

        // Update SR: clear V|C, set C (bit 0), V and L (sticky)
        let sr_new = self.clear_sr_flags((1u32 << sr::V) | (1u32 << sr::C));
        let sr_new = self.builder.ins().bor(sr_new, carry); // C is bit 0
        let sr_new = self.or_vl(sr_new, overflow);
        self.store_reg(reg::SR, sr_new);
    }

    /// Set the carry flag in SR.
    pub(super) fn set_carry(&mut self, c_bit: Value) {
        let sr_c = self.clear_sr_flags(1u32 << sr::C);
        // c_bit should be 0 or 1; C is bit 0 so mask only
        let one = self.builder.ins().iconst(types::I32, 1);
        let c_masked = self.builder.ins().band(c_bit, one);
        let sr_new = self.builder.ins().bor(sr_c, c_masked); // C is bit 0
        self.store_reg(reg::SR, sr_new);
    }

    /// Clear the V (overflow) flag in SR.
    pub(super) fn clear_v_flag(&mut self) {
        let sr = self.clear_sr_flags(1u32 << sr::V);
        self.store_reg(reg::SR, sr);
    }

    pub(super) fn update_vcl_sub(&mut self, source: Value, dest: Value, result: Value) {
        self.update_vcl(source, dest, result, true);
    }
}

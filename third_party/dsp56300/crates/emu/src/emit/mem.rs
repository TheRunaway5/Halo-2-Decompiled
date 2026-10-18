use super::*;

impl<'a> Emitter<'a> {
    /// Emit an inline load from a Buffer region at a constant address.
    /// The base pointer is baked as an immediate -- zero overhead.
    pub(super) fn emit_buffer_load(&mut self, base: *mut u32, idx: u32) -> Value {
        let base_val = self.builder.ins().iconst(self.ptr_ty, base as i64);
        let byte_off = (idx as i64) * 4;
        self.builder.ins().load(
            types::I32,
            MemFlagsData::trusted(),
            base_val,
            byte_off as i32,
        )
    }

    /// Emit an inline store to a Buffer region at a constant address.
    pub(super) fn emit_buffer_store(&mut self, base: *mut u32, idx: u32, val: Value) {
        let base_val = self.builder.ins().iconst(self.ptr_ty, base as i64);
        let byte_off = (idx as i64) * 4;
        self.builder
            .ins()
            .store(MemFlagsData::trusted(), val, base_val, byte_off as i32);
    }

    /// Compute the native address of `base[(addr - adj) as usize]` where
    /// `adj = start.wrapping_sub(offset)`.
    fn emit_buffer_elem_addr(
        &mut self,
        base: *mut u32,
        start: u32,
        offset: u32,
        addr: Value,
    ) -> Value {
        let base_val = self.builder.ins().iconst(self.ptr_ty, base as i64);
        let adj = start.wrapping_sub(offset);
        let index = if adj == 0 {
            addr
        } else {
            self.builder.ins().iadd_imm_s(addr, -(adj as i64))
        };
        let byte_off = self.builder.ins().ishl_imm_u(index, 2);
        let byte_off_ext = if self.ptr_ty == types::I64 {
            self.builder.ins().uextend(types::I64, byte_off)
        } else {
            byte_off
        };
        self.builder.ins().iadd(base_val, byte_off_ext)
    }

    /// Emit an inline load from a Buffer region at a dynamic address.
    pub(super) fn emit_buffer_load_dyn(
        &mut self,
        base: *mut u32,
        start: u32,
        offset: u32,
        addr: Value,
    ) -> Value {
        let elem_addr = self.emit_buffer_elem_addr(base, start, offset, addr);
        self.builder
            .ins()
            .load(types::I32, MemFlagsData::trusted(), elem_addr, 0)
    }

    /// Emit an indirect call to a Callback region's read function.
    /// Does NOT flush/reload promoted registers -- caller is responsible.
    pub(super) fn emit_callback_read_dyn(
        &mut self,
        opaque: *mut std::ffi::c_void,
        read_fn: unsafe extern "C" fn(*mut std::ffi::c_void, u32) -> u32,
        addr: Value,
    ) -> Value {
        // Deliberately NOT a `defer_hazard_sites` bump: pending flags
        // already ride across callback calls unmaterialized (see
        // emit_call_read_accu24), so "callbacks do not read SR" is a
        // standing contract, and a map with any callback region would
        // otherwise gate every dynamic access off the backedge deferral.
        // Peripheral-WRITING bodies still gate via the exit check's
        // flush_all_to_memory.
        let fn_val = self
            .builder
            .ins()
            .iconst(self.ptr_ty, read_fn as usize as i64);
        let opaque_val = self
            .builder
            .ins()
            .iconst(self.ptr_ty, opaque as usize as i64);
        let mut sig = Signature::new(HOST_CALL_CONV);
        sig.params.push(AbiParam::new(self.ptr_ty)); // opaque
        sig.params.push(AbiParam::new(types::I32)); // address
        sig.returns.push(AbiParam::new(types::I32)); // return
        let sig_ref = self.builder.import_signature(sig);
        let call = self
            .builder
            .ins()
            .call_indirect(sig_ref, fn_val, &[opaque_val, addr]);
        self.builder.inst_results(call)[0]
    }

    /// Emit an indirect call to a Callback region's write function.
    /// Does NOT flush/reload promoted registers -- caller is responsible.
    pub(super) fn emit_callback_write_dyn(
        &mut self,
        opaque: *mut std::ffi::c_void,
        write_fn: unsafe extern "C" fn(*mut std::ffi::c_void, u32, u32),
        addr: Value,
        val: Value,
    ) {
        // No `defer_hazard_sites` bump - see emit_callback_read_dyn.
        let fn_val = self
            .builder
            .ins()
            .iconst(self.ptr_ty, write_fn as usize as i64);
        let opaque_val = self
            .builder
            .ins()
            .iconst(self.ptr_ty, opaque as usize as i64);
        let mut sig = Signature::new(HOST_CALL_CONV);
        sig.params.push(AbiParam::new(self.ptr_ty)); // opaque
        sig.params.push(AbiParam::new(types::I32)); // address
        sig.params.push(AbiParam::new(types::I32)); // value
        let sig_ref = self.builder.import_signature(sig);
        self.builder
            .ins()
            .call_indirect(sig_ref, fn_val, &[opaque_val, addr, val]);
    }

    /// Read from a memory space at a constant address (compile-time map lookup).
    pub(super) fn read_mem(&mut self, space: MemSpace, addr: u32) -> Value {
        let raw = if let Some(region) = self.map.lookup(space, addr) {
            match region.kind {
                RegionKind::Buffer { base, offset } => {
                    self.emit_buffer_load(base, addr - region.start + offset)
                }
                RegionKind::Callback {
                    opaque, read_fn, ..
                } => {
                    let addr_val = self.builder.ins().iconst(types::I32, addr as i64);
                    self.emit_callback_read_dyn(opaque, read_fn, addr_val)
                }
            }
        } else {
            self.builder.ins().iconst(types::I32, 0)
        };
        self.mask24(raw)
    }

    /// Write to a memory space at a constant address (compile-time map lookup).
    pub(super) fn write_mem(&mut self, space: MemSpace, addr: u32, val: Value) {
        if let Some(region) = self.map.lookup(space, addr) {
            let masked = self.mask24(val);
            match region.kind {
                RegionKind::Buffer { base, offset } => {
                    self.emit_buffer_store(base, addr - region.start + offset, masked);
                }
                RegionKind::Callback {
                    opaque, write_fn, ..
                } => {
                    let addr_val = self.builder.ins().iconst(types::I32, addr as i64);
                    self.emit_callback_write_dyn(opaque, write_fn, addr_val, masked);
                }
            }
        }
    }

    /// The space's RAM: its first region, when that is a buffer starting
    /// at address 0 - every embedder's layout. Returns (base, offset, end).
    fn ram_region(regions: &[crate::core::MemoryRegion]) -> Option<(*mut u32, u32, u32)> {
        match regions.first() {
            Some(crate::core::MemoryRegion {
                start: 0,
                end,
                kind: RegionKind::Buffer { base, offset },
            }) => Some((*base, *offset, *end)),
            _ => None,
        }
    }

    /// Bounds test against `end` plus an index that is in range whether
    /// or not the address is: a power-of-two size masks, anything else
    /// selects. Returns (in_range, clamped index).
    fn emit_clamped_index(&mut self, addr: Value, end: u32) -> (Value, Value) {
        let ok = self
            .builder
            .ins()
            .icmp_imm_u(IntCC::UnsignedLessThan, addr, end as i64);
        let safe = if end.is_power_of_two() {
            let mask = self.builder.ins().iconst(types::I32, (end - 1) as i64);
            self.builder.ins().band(addr, mask)
        } else {
            let zero = self.builder.ins().iconst(types::I32, 0);
            self.builder.ins().select(ok, addr, zero)
        };
        (ok, safe)
    }

    /// `jit_read_mem(state, space, addr)`: the run-time region walk, for
    /// addresses outside the space's RAM.
    fn emit_read_mem_helper(&mut self, space: MemSpace, addr: Value) -> Value {
        let fn_ptr = self.builder.ins().iconst(
            self.ptr_ty,
            crate::core::jit_read_mem as *const () as usize as i64,
        );
        let space_val = self.builder.ins().iconst(types::I32, space as u32 as i64);
        let mut sig = Signature::new(HOST_CALL_CONV);
        sig.params.push(AbiParam::new(self.ptr_ty));
        sig.params.push(AbiParam::new(types::I32));
        sig.params.push(AbiParam::new(types::I32));
        sig.returns.push(AbiParam::new(types::I32));
        let sig_ref = self.builder.import_signature(sig);
        let call =
            self.builder
                .ins()
                .call_indirect(sig_ref, fn_ptr, &[self.state_ptr, space_val, addr]);
        self.builder.inst_results(call)[0]
    }

    /// `jit_write_mem(state, space, addr, val)` for X/Y addresses outside
    /// the space's RAM.
    fn emit_write_mem_helper(&mut self, space: MemSpace, addr: Value, val: Value) {
        let fn_ptr = self
            .builder
            .ins()
            .iconst(self.ptr_ty, jit_write_mem as *const () as usize as i64);
        let space_val = self.builder.ins().iconst(types::I32, space as u32 as i64);
        let mut sig = Signature::new(HOST_CALL_CONV);
        sig.params.push(AbiParam::new(self.ptr_ty));
        sig.params.push(AbiParam::new(types::I32));
        sig.params.push(AbiParam::new(types::I32));
        sig.params.push(AbiParam::new(types::I32));
        let sig_ref = self.builder.import_signature(sig);
        self.builder
            .ins()
            .call_indirect(sig_ref, fn_ptr, &[self.state_ptr, space_val, addr, val]);
    }

    /// Read memory at a dynamic address.
    ///
    /// The space's RAM is the fast path. Its bounds check clamps the index
    /// instead of guarding the load, so the load is unconditional and the
    /// common case is straight-line code that falls through; in a space
    /// with no other region the miss value is a `select` as well and the
    /// access has no control flow at all. Every other region - a
    /// peripheral, a ROM, an alias - is reached from one cold block
    /// through `jit_read_mem`, whose run-time region walk is cheaper than
    /// a per-region branch tree is to compile: under a map with a
    /// peripheral region a tree turns a 32-instruction block into hundreds
    /// of Cranelift blocks, and register allocation, priced per block, is
    /// most of a compile.
    ///
    /// Nothing is spilled around the call: callbacks may not touch the
    /// register file (`RegionKind::Callback`), and the helper does not.
    /// Spilling every dirty promoted register before the dispatch and
    /// invalidating all of them after it - on the buffer path too, since
    /// the branch is resolved at run time - would add ~100 loads and ~44
    /// stores to that same block.
    pub(super) fn read_mem_dyn(&mut self, space: MemSpace, addr: Value) -> Value {
        let regions = self.map.regions(space);
        let Some((base, offset, end)) = Self::ram_region(regions) else {
            let raw = self.emit_read_mem_helper(space, addr);
            return self.mask24(raw);
        };
        let single = regions.len() == 1;
        let (ok, safe) = self.emit_clamped_index(addr, end);
        let hit = self.emit_buffer_load_dyn(base, 0, offset, safe);
        let raw = if single {
            let zero = self.builder.ins().iconst(types::I32, 0);
            self.builder.ins().select(ok, hit, zero)
        } else {
            let merge_blk = self.builder.create_block();
            self.builder.append_block_param(merge_blk, types::I32);
            let slow_blk = self.builder.create_block();
            self.builder.set_cold_block(slow_blk);
            self.builder
                .ins()
                .brif(ok, merge_blk, &[BlockArg::Value(hit)], slow_blk, &[]);
            self.builder.switch_to_block(slow_blk);
            self.builder.seal_block(slow_blk);
            let miss = self.emit_read_mem_helper(space, addr);
            self.builder.ins().jump(merge_blk, &[BlockArg::Value(miss)]);
            self.builder.switch_to_block(merge_blk);
            self.builder.seal_block(merge_blk);
            self.builder.block_params(merge_blk)[0]
        };
        self.mask24(raw)
    }

    /// Write memory at a dynamic address; the mirror of `read_mem_dyn`.
    /// The store is unconditional too: its address selects between the
    /// RAM element and `DspState::mem_write_sink`, so a miss lands in a
    /// word nothing reads and only then takes the cold path. P-space
    /// writes keep the helper form for their dirty tracking.
    pub(super) fn write_mem_dyn(&mut self, space: MemSpace, addr: Value, val: Value) {
        if space == MemSpace::P {
            return self.write_mem_dyn_p(addr, val);
        }
        let regions = self.map.regions(space);
        let masked = self.mask24(val);
        let Some((base, offset, end)) = Self::ram_region(regions) else {
            self.emit_write_mem_helper(space, addr, masked);
            return;
        };
        let (ok, safe) = self.emit_clamped_index(addr, end);
        let elem = self.emit_buffer_elem_addr(base, 0, offset, safe);
        let sink = self
            .builder
            .ins()
            .iadd_imm_s(self.state_ptr, OFF_MEM_WRITE_SINK as i64);
        let ptr = self.builder.ins().select(ok, elem, sink);
        self.builder
            .ins()
            .store(MemFlagsData::trusted(), masked, ptr, 0);
        if regions.len() > 1 {
            let merge_blk = self.builder.create_block();
            let slow_blk = self.builder.create_block();
            self.builder.set_cold_block(slow_blk);
            self.builder.ins().brif(ok, merge_blk, &[], slow_blk, &[]);
            self.builder.switch_to_block(slow_blk);
            self.builder.seal_block(slow_blk);
            self.emit_write_mem_helper(space, addr, masked);
            self.builder.ins().jump(merge_blk, &[]);
            self.builder.switch_to_block(merge_blk);
            self.builder.seal_block(merge_blk);
        }
    }

    /// P-space form of `write_mem_dyn`: the runtime helper, for its dirty
    /// tracking.
    fn write_mem_dyn_p(&mut self, addr: Value, val: Value) {
        self.flush_promoted();
        let fn_ptr = self
            .builder
            .ins()
            .iconst(self.ptr_ty, jit_write_mem as *const () as usize as i64);
        let space_val = self
            .builder
            .ins()
            .iconst(types::I32, MemSpace::P as u32 as i64);
        let mut sig = Signature::new(HOST_CALL_CONV);
        sig.params.push(AbiParam::new(self.ptr_ty));
        sig.params.push(AbiParam::new(types::I32));
        sig.params.push(AbiParam::new(types::I32));
        sig.params.push(AbiParam::new(types::I32));
        let sig_ref = self.builder.import_signature(sig);
        self.builder
            .ins()
            .call_indirect(sig_ref, fn_ptr, &[self.state_ptr, space_val, addr, val]);
        self.invalidate_promoted();
    }
}

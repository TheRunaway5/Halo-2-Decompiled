use super::*;

/// Cycles an inline REP/DO loop may run inside one block dispatch before it
/// returns to the run loop. Large enough that ordinary loop nests finish
/// inline, small enough that a runaway loop cannot hold the block for a
/// perceptible time: 4096 cycles is ~40 us of DSP time.
///
/// Swept on a production workload, 4096 is the knee: the smallest value
/// at which the check never truncates one of that program's inline loops,
/// so above it the quantum is no longer what ends a block. Throughput
/// cannot tell the values apart.
pub(crate) const INLINE_LOOP_QUANTUM: i32 = 4096;

impl<'a> Emitter<'a> {
    /// Emit a preemption check for an inline-loop backedge. Once this block
    /// invocation has run `loop_quantum` cycles, spill all state and
    /// return from the block with pc = `resume_pc` (the top of the loop
    /// body). Loop state (LF/LA/LC and the loop stack) is architectural at
    /// iteration boundaries, so the run loop resumes the remaining
    /// iterations through the non-inline block-boundary path. Without a
    /// check here, an inline DO with a large runtime LC (register/memory
    /// forms reach 65535) - and nested inline DOs, multiplicatively -
    /// executes for arbitrarily long inside one block.
    ///
    /// The bound is a fixed quantum, not the run loop's remaining budget.
    /// Against the budget, the caller's slice size would decide how far into
    /// a loop the switch to the block-boundary path happens, so the same
    /// program would take a different path through the translator depending
    /// on how finely its host schedules it. A quantum keeps the loop
    /// preemptible and bounded while making where it breaks a property of
    /// the code, not of the schedule.
    ///
    /// Leaves the builder positioned in the continue block.
    ///
    /// `deferred_nz`: an E/U/N/Z computation deferred across the back edge
    /// (`flush_pending_flags_backedge_deferring_eunz`) that must reach SR
    /// before the bail's exit - the run loop resumes through a block that
    /// reads SR as materialized. Emitted in the bail arm only; the continue
    /// path leaves it deferred for the loop exit.
    fn emit_loop_preemption_check(&mut self, resume_pc: u32, deferred_nz: Option<Value>) {
        self.flush_pending_cycles();
        let total = self.builder.use_var(self.total_cycles);
        let quantum = self
            .builder
            .ins()
            .iconst(types::I32, self.loop_quantum as i64);
        let exceeded = self
            .builder
            .ins()
            .icmp(IntCC::SignedGreaterThanOrEqual, total, quantum);

        let bail = self.builder.create_block();
        let cont = self.builder.create_block();
        self.builder.ins().brif(exceeded, bail, &[], cont, &[]);

        self.builder.switch_to_block(bail);
        self.builder.seal_block(bail);
        // The bail arm's materialization runs only on the bail path; the
        // continue block below must not inherit its `eunz_current`.
        let saved_eunz_current = self.eunz_current;
        if let Some(result56) = deferred_nz {
            self.emit_deferred_nz(result56);
        }
        self.flush_all_to_memory();
        let pc_val = self.builder.ins().iconst(types::I32, resume_pc as i64);
        self.store_pc(pc_val);
        let ret = self.builder.use_var(self.total_cycles);
        self.builder.ins().return_(&[ret]);

        self.builder.switch_to_block(cont);
        self.builder.seal_block(cont);
        self.eunz_current = saved_eunz_current;
    }

    /// Mask a value to 16-bit LC width using REG_MASKS[reg::LC].
    fn mask_lc(&mut self, val: Value) -> Value {
        let lc_mask = self
            .builder
            .ins()
            .iconst(types::I32, REG_MASKS[reg::LC] as i64);
        self.builder.ins().band(val, lc_mask)
    }

    /// Returns true if the instruction is a REP variant (handled inline
    /// by `emit_block` as a Cranelift loop rather than as a block terminator).
    pub(super) fn is_rep_instruction(inst: &Instruction) -> bool {
        matches!(
            inst,
            Instruction::RepImm { .. }
                | Instruction::RepAa { .. }
                | Instruction::RepEa { .. }
                | Instruction::RepReg { .. }
        )
    }

    /// Returns true if the instruction is a DO/DOR variant that may be
    /// inlined as a Cranelift loop when the body is safe.
    pub(super) fn is_do_instruction(inst: &Instruction) -> bool {
        matches!(
            inst,
            Instruction::DoImm { .. }
                | Instruction::DoReg { .. }
                | Instruction::DoForever
                | Instruction::DoAa { .. }
                | Instruction::DoEa { .. }
                | Instruction::DorImm { .. }
                | Instruction::DorReg { .. }
                | Instruction::DorForever
                | Instruction::DorAa { .. }
                | Instruction::DorEa { .. }
        )
    }

    /// Extract the absolute loop-end address (LA) from a DO/DOR instruction.
    pub(super) fn compute_do_la(inst: &Instruction, pc: u32, next_word: u32) -> u32 {
        match inst {
            // DO variants use absolute LA from next_word.
            Instruction::DoImm { .. }
            | Instruction::DoReg { .. }
            | Instruction::DoForever
            | Instruction::DoAa { .. }
            | Instruction::DoEa { .. } => next_word & REG_MASKS[reg::LA],
            // DOR variants use relative LA: pc + next_word.
            Instruction::DorImm { .. }
            | Instruction::DorReg { .. }
            | Instruction::DorForever
            | Instruction::DorAa { .. }
            | Instruction::DorEa { .. } => mask_pc(pc.wrapping_add(next_word)),
            _ => unreachable!("not a DO instruction"),
        }
    }

    /// Check whether a DO loop body [body_start, la] can be safely inlined.
    ///
    /// Returns true only if the body contains no block terminators, no
    /// peripheral writes that may set idle, no P-memory writes, fits
    /// within the block size limit, and doesn't cross an outer DO loop
    /// boundary.
    pub(super) fn is_do_body_inlineable(
        map: &MemoryMap,
        body_start: u32,
        la: u32,
        outer_stop_pc: u32,
    ) -> bool {
        Self::is_do_body_inlineable_inner(map, body_start, la, outer_stop_pc, 0)
    }

    pub(super) fn is_do_body_inlineable_inner(
        map: &MemoryMap,
        body_start: u32,
        la: u32,
        outer_stop_pc: u32,
        depth: u32,
    ) -> bool {
        const MAX_NESTING_DEPTH: u32 = 8;
        if depth > MAX_NESTING_DEPTH {
            return false;
        }
        let p_end = map.p_space_end();
        // Body must fit within PRAM bounds.
        if la >= p_end {
            return false;
        }
        // Body must not reach an enclosing DO loop's boundary. Ending at
        // the same address as the enclosing loop is not a nest the inlined
        // shape can express: hardware ends only the innermost loop there, so
        // the enclosing one restores its LA behind a PC that has already
        // passed it and never terminates - which is what the
        // instruction-at-a-time and block-boundary paths both do. The inlined
        // form ends both, because the block's own end-of-loop handling fires
        // at the same instruction as the inline loop's exit. `outer_stop_pc`
        // is the enclosing loop's LA+1 (or u32::MAX outside a loop), so
        // rejecting equality here covers both a lexically nested DO and a
        // block compiled inside a loop the translator cannot see.
        if la + 1 >= outer_stop_pc {
            return false;
        }
        // Body must start at or before LA (non-empty body).
        if body_start > la {
            return false;
        }

        let mut body_pc = body_start;
        let mut count = 0u32;
        const MAX_INLINE_LEN: u32 = 64;

        // Open forward-skip merge targets, innermost last (see
        // `forward_skip_target`). The emitter closes a skip when the walk
        // reaches its target, so targets must land on top-level
        // instruction boundaries: anything else leaves the stack non-empty
        // at la+1 and rejects the body.
        let mut open_skips: Vec<u32> = Vec::new();

        while body_pc <= la {
            while open_skips.last() == Some(&body_pc) {
                open_skips.pop();
            }
            if body_pc >= p_end {
                return false;
            }
            let opcode = map.read_pram(body_pc);
            let inst = decode::decode(opcode);
            let inst_len = decode::instruction_length(&inst);

            let nw = map.read_pram(mask_pc(body_pc + 1));
            if let Some(target) = Self::forward_skip_target(&inst, body_pc, nw) {
                // Forward only, staying strictly inside the body (a
                // branch to LA+1 would leave the loop frame active
                // without running the loop-end machinery), and
                // well-nested inside any enclosing open skip.
                if target < body_pc + inst_len || target > la {
                    return false;
                }
                if let Some(&enclosing) = open_skips.last()
                    && target > enclosing
                {
                    return false;
                }
                open_skips.push(target);
                body_pc += inst_len;
                count += 1;
                continue;
            }

            if Self::is_do_instruction(&inst) {
                // A DO/REP under conditional control needs loop-scope and
                // flush mechanics inside a conditional arm the emitter
                // does not support - keep skip regions loop-free.
                if !open_skips.is_empty() {
                    return false;
                }
                // DO FOREVER cannot be inlined (infinite native loop)
                if matches!(inst, Instruction::DoForever | Instruction::DorForever) {
                    return false;
                }
                // Nested DO/DOR: recursively check if the inner body is safe.
                let nw = map.read_pram(mask_pc(body_pc + 1));
                let inner_la = Self::compute_do_la(&inst, body_pc, nw);
                // The inner loop must end strictly before the outer's LA;
                // the recursive call's `outer_stop_pc` check says the same
                // thing, this is just the early-out.
                if inner_la >= la {
                    return false;
                }
                let inner_body_start = body_pc + 2;
                if !Self::is_do_body_inlineable_inner(
                    map,
                    inner_body_start,
                    inner_la,
                    la + 1,
                    depth + 1,
                ) {
                    return false;
                }
                let inner_count = Self::count_body_instructions(map, inner_body_start, inner_la);
                count += 1 + inner_count;
                body_pc = inner_la + 1;
                continue;
            }

            if Self::is_block_terminator(&inst) {
                return false;
            }
            if Self::needs_exit_check(&inst) {
                return false;
            }
            if Self::writes_p_memory(&inst) {
                return false;
            }
            // A fault-arming instruction inside an inline body would post
            // its armed stream-word budget mid-loop and iterate uncharged;
            // the block-boundary path bails at the armer instead (see
            // emit_fault_arm_bail).
            if Self::may_arm_stack_fault(&inst) {
                return false;
            }

            if Self::is_rep_instruction(&inst) {
                if !open_skips.is_empty() {
                    return false;
                }
                // REP consumes itself (1 word) + the repeated instruction.
                let rep_next = body_pc + 1;
                if rep_next >= p_end {
                    return false;
                }
                let rep_opcode = map.read_pram(rep_next);
                let rep_inst = decode::decode(rep_opcode);
                let rep_len = decode::instruction_length(&rep_inst);
                // The repeated instruction must also be safe - including
                // not arming faults mid-loop, and not being the
                // manual-restricted REP/DO target the legacy fallback in
                // emit_block handles.
                if Self::is_block_terminator(&rep_inst)
                    || Self::needs_exit_check(&rep_inst)
                    || Self::writes_p_memory(&rep_inst)
                    || Self::may_arm_stack_fault(&rep_inst)
                    || Self::is_rep_instruction(&rep_inst)
                    || Self::is_do_instruction(&rep_inst)
                {
                    return false;
                }
                // REP + repeated instruction must fit within the loop body.
                if rep_next + rep_len > la + 1 {
                    return false;
                }
                body_pc = rep_next + rep_len;
                count += 2;
            } else {
                body_pc += inst_len;
                count += 1;
            }
        }

        // Instructions must tile [body_start, la+1) exactly.
        if body_pc != la + 1 {
            return false;
        }
        // Every skip target must have landed on a top-level instruction
        // boundary at or before LA.
        if !open_skips.is_empty() {
            return false;
        }
        // Practical size limit.
        if count > MAX_INLINE_LEN {
            return false;
        }
        true
    }

    /// Check whether the skip region of a straight-line forward branch at
    /// `branch_pc` (taken target `target`, from `forward_skip_target`) can
    /// compile as a structured conditional skip inside `emit_block`
    /// instead of terminating the block. Same rules as the in-loop scan
    /// minus the loop-frame ones, with one addition: exit-check
    /// instructions (peripheral writes) are allowed in the region - their
    /// early-return spill (`flush_all_to_memory`) is side-path safe, and
    /// the skip opener flushed the promotion cache, so memory is current
    /// on the return path too. The merge target must stay strictly inside
    /// any enclosing DO frame (`stop_pc` = LA+1): a taken branch to LA+1
    /// would otherwise become a sequential arrival there and trigger the
    /// run loop's loop-back where hardware does not loop.
    ///
    /// Returns the region's instruction count (nested skip openers
    /// included) for the caller's block budget, or None to fall back to
    /// the block-terminator form.
    pub(super) fn straightline_skip_region_len(
        map: &MemoryMap,
        branch_pc: u32,
        branch_len: u32,
        target: u32,
        stop_pc: u32,
    ) -> Option<u32> {
        let p_end = map.p_space_end();
        if target < branch_pc + branch_len || target >= stop_pc || target > p_end {
            return None;
        }
        // Open nested skip targets, innermost last (see the in-loop scan).
        let mut open_skips: Vec<u32> = Vec::new();
        let mut pc = branch_pc + branch_len;
        let mut count = 0u32;
        while pc < target {
            while open_skips.last() == Some(&pc) {
                open_skips.pop();
            }
            let inst = decode::decode(map.read_pram(pc));
            let inst_len = decode::instruction_length(&inst);
            let nw = map.read_pram(mask_pc(pc + 1));

            if let Some(inner) = Self::forward_skip_target(&inst, pc, nw) {
                // Forward and well-nested inside the innermost open region.
                if inner < pc + inst_len || inner > *open_skips.last().unwrap_or(&target) {
                    return None;
                }
                open_skips.push(inner);
                pc += inst_len;
                count += 1;
                continue;
            }
            if Self::is_do_instruction(&inst)
                || Self::is_rep_instruction(&inst)
                || Self::is_block_terminator(&inst)
                || Self::writes_p_memory(&inst)
            {
                return None;
            }
            pc += inst_len;
            count += 1;
        }
        // A two-word instruction straddling a merge point misaligns the
        // region: the outer target via the overshoot check, a nested one
        // by never popping (only nested targets equal to the outer merge
        // may remain; emission closes those LIFO at the merge itself).
        if pc != target || open_skips.iter().any(|&t| t != target) {
            return None;
        }
        Some(count)
    }

    /// Count instructions in a DO loop body [body_start, la] for the
    /// emit_block instruction budget. Only called after is_do_body_inlineable
    /// returned true, so we know the body is well-formed.
    pub(super) fn count_body_instructions(map: &MemoryMap, body_start: u32, la: u32) -> u32 {
        let mut body_pc = body_start;
        let mut count = 0u32;
        while body_pc <= la {
            let opcode = map.read_pram(body_pc);
            let inst = decode::decode(opcode);
            if Self::is_do_instruction(&inst) {
                let nw = map.read_pram(mask_pc(body_pc + 1));
                let inner_la = Self::compute_do_la(&inst, body_pc, nw);
                let inner_body_start = body_pc + 2;
                let inner_count = Self::count_body_instructions(map, inner_body_start, inner_la);
                count += 1 + inner_count;
                body_pc = inner_la + 1;
            } else if Self::is_rep_instruction(&inst) {
                let rep_opcode = map.read_pram(mask_pc(body_pc + 1));
                let rep_inst = decode::decode(rep_opcode);
                body_pc += 1 + decode::instruction_length(&rep_inst);
                count += 2;
            } else {
                body_pc += decode::instruction_length(&inst);
                count += 1;
            }
        }
        count
    }

    /// Emit a REP instruction as an inline Cranelift loop. The repeated
    /// instruction (next in pram) executes LC times inside a native loop.
    ///
    /// This is only used by `emit_block()`. `compile_instruction()` still
    /// uses the original `emit_rep_*` functions which set `loop_rep` for
    /// step-by-step debugging via `postexecute_update_pc`.
    pub(super) fn emit_rep_inline(&mut self, rep_inst: &Instruction, rep_pc: u32) {
        // 1. Emit REP setup: save TEMP, compute LC
        let old_lc = self.load_reg(reg::LC);
        self.store_reg(reg::TEMP, old_lc);
        self.set_cycles(5); // REP overhead

        let lc_val = self.emit_rep_lc_value(rep_inst);
        self.store_reg(reg::LC, lc_val);

        // 2. Decode the next instruction (the one to repeat)
        let next_pc = rep_pc + 1;
        let next_opcode = self.map.read_pram(next_pc);
        let next_next_word = self.map.read_pram(mask_pc(next_pc + 1));
        let next_inst = decode::decode(next_opcode);

        // 3. Create Cranelift loop with deferred pre-loop block. The loop
        // is do-while-style with an entry annul guard: REP with LC=0
        // executes the target zero times (hardware-verified; diverges from
        // the 56300FM's "65,536 repeats") by branching straight to the
        // merge, and the loop's exit edge leaves from the LC-decrement at
        // the body's bottom - so a deferred E/U/N/Z result, defined in the
        // body, dominates the exit where it materializes. The pre-loop
        // block still sits between the guard and the body, so no control
        // path into the loop bypasses its deferred register loads.
        let pre_loop = self.builder.create_block();
        let loop_top = self.builder.create_block();
        let loop_exit = self.builder.create_block();
        let after_loop = self.builder.create_block();

        self.flush_pending_cycles(); // flush pre-REP cycles before entering loop
        // Pre-REP CCR state must materialize before the loop: the body may
        // execute zero times, and a set_pending inside it would strand the
        // pre-REP computation on the skip path.
        self.flush_pending_flags();
        // Flush dirty registers before the loop: the body may execute zero
        // times, and a flush/invalidate inside it (e.g. a P-memory-writing
        // target) clears compile-time dirty flags globally - without this,
        // pre-REP register state would never reach memory on the skip path.
        self.flush_promoted();
        let zero = self.builder.ins().iconst(types::I32, 0);
        let annul = self.builder.ins().icmp(IntCC::Equal, lc_val, zero);
        self.builder
            .ins()
            .brif(annul, after_loop, &[], pre_loop, &[]);
        self.builder.switch_to_block(loop_top);
        // Don't seal loop_top yet - back-edge pending

        // 4. Push loop scope; the body IS the loop head: run the repeated
        // instruction, then decrement and test at the bottom.
        self.push_loop_scope(pre_loop);
        // Snapshot the hazard counter (see emit_do_inline): the body may
        // defer its final flag computation across the back edge only if it
        // emits no site that could observe SR mid-iteration.
        let hazard_mark = self.defer_hazard_sites;
        // Each iteration re-enters here with the quarter possibly elided.
        self.eunz_current = false;
        let lc_cur = self.load_reg(reg::LC);
        self.emit_instruction(&next_inst, next_pc, next_next_word);

        // 5. Decrement LC and branch: back to the top, or out.
        self.flush_pending_cycles(); // flush body cycles once per iteration
        // The body's final CCR update materializes once per iteration. When
        // nothing in the body can observe SR mid-iteration, the E/U/N/Z
        // helper call - a pure overwrite, dead on every iteration but the
        // last - defers to the loop exit and runs once, on the final
        // iteration's result; the sticky/overwrite V/C/L/SM half still
        // lands per iteration. REP is not preemptible (silicon locks out
        // interrupts for its duration, and no quantum check is emitted),
        // so the exit is the only materialization site.
        let deferred_nz = if self.defer_hazard_sites == hazard_mark {
            self.flush_pending_flags_backedge_deferring_eunz()
        } else {
            self.flush_pending_flags();
            None
        };
        // Flush the body's register writes to memory each iteration:
        // conditional-arm merges invalidate their destinations, and the
        // resulting inline memory reloads re-execute every iteration -
        // memory must be current at each iteration boundary for those
        // reloads to be sound (mirrors emit_do_inline).
        self.flush_promoted();
        // Decrement the top-of-body SSA value (lc_cur), not a load_reg: a
        // flush/invalidate inside the body (callback reads, P writes)
        // would otherwise turn this into a stale memory reload and the
        // loop would never terminate.
        let one = self.builder.ins().iconst(types::I32, 1);
        let new_lc = self.builder.ins().isub(lc_cur, one);
        let new_lc = self.mask_lc(new_lc);
        self.store_reg(reg::LC, new_lc);
        // Keep LC's memory image current across the backedge (see
        // emit_lc_decrement_and_branch): the store lands after the
        // body-bottom flush.
        self.flush_reg(reg::LC);
        let done = self.builder.ins().icmp(IntCC::Equal, new_lc, zero);
        self.builder.ins().brif(done, loop_exit, &[], loop_top, &[]);

        // 6. Pop loop scope and emit pre-loop block with targeted loads
        self.pop_loop_scope(loop_top);

        // 7. Loop exit: materialize the deferred backedge E/U/N/Z call on
        // the last iteration's result (this edge leaves the body bottom,
        // so the body-defined value dominates it), then merge.
        self.builder.switch_to_block(loop_exit);
        self.builder.seal_block(loop_exit);
        if let Some(result56) = deferred_nz {
            self.emit_deferred_nz(result56);
        }
        self.flush_promoted();
        self.builder.ins().jump(after_loop, &[]);

        // 8. Merge point (loop exit or annul skip). Both incoming paths
        // left memory authoritative; invalidate the promotion cache so
        // downstream code reloads from memory instead of using
        // body-defined variables that are undefined on the annul path.
        self.builder.switch_to_block(after_loop);
        self.builder.seal_block(after_loop);
        // The annul path skips the loop (and any exit materialization),
        // so the quarter is not provably current past the merge.
        self.eunz_current = false;
        self.invalidate_promoted();

        // 9. Restore LC from TEMP - and flush it: the invalidate above
        // makes downstream loads (e.g. an enclosing inline-DO's LC
        // decrement) reload from memory, which still holds the REP's
        // exhausted count until this store reaches it.
        let saved_lc = self.load_reg(reg::TEMP);
        self.store_reg(reg::LC, saved_lc);
        self.flush_reg(reg::LC);
    }

    /// Emit a DO/DOR instruction as an inline Cranelift loop. The loop body
    /// [do_pc+2, la] executes LC times inside a native loop with no per-
    /// iteration run-loop overhead.
    ///
    /// Only called from `emit_block()` after `is_do_body_inlineable` returns
    /// true. The caller must advance `pc` past the entire loop (to `la + 1`).
    pub(super) fn emit_do_inline(&mut self, do_inst: &Instruction, do_pc: u32, la: u32) {
        // 1. DO setup: push stack, set LA/LC/LF.
        let overhead_cycles: i64 = 5; // all DO/DOR variants: Table A-1
        self.set_inst_len(2);
        self.set_cycles(overhead_cycles);

        let lc_val = self.emit_do_lc_value(do_inst);
        let la_val = self.builder.ins().iconst(types::I32, la as i64);
        let forever = matches!(do_inst, Instruction::DoForever | Instruction::DorForever);
        self.emit_do_setup(la_val, lc_val, do_pc + 2, forever);

        // The DO header pushes can post a stack-error core fault
        // (overflow at SP=15) with an armed stream-word budget. An
        // inline loop cannot honor the Armed model's word-granular
        // annulment mid-block, so bail out to the run loop before the
        // body executes, resuming at the body's first instruction -
        // step-mode delivery then counts the silicon budget from there
        // (probe_do_overflow block-mode divergence).
        // Mirrors emit_loop_budget_check's bail mechanics.
        self.flush_pending_cycles();
        let fb = self.builder.ins().load(
            types::I32,
            Self::flags(),
            self.state_ptr,
            OFF_INTERRUPT_FAULT_BUDGET,
        );
        // INVALID_FAULT_BUDGET == 0xFFFF_FFFF == -1 as i32.
        let invalid = self.builder.ins().iconst(types::I32, -1);
        let faulted = self.builder.ins().icmp(IntCC::NotEqual, fb, invalid);
        let fault_bail = self.builder.create_block();
        let no_fault = self.builder.create_block();
        self.builder
            .ins()
            .brif(faulted, fault_bail, &[], no_fault, &[]);
        self.builder.switch_to_block(fault_bail);
        self.builder.seal_block(fault_bail);
        // Loop machinery's own spill - see emit_loop_preemption_check.
        self.flush_all_to_memory();
        let resume = self.builder.ins().iconst(types::I32, (do_pc + 2) as i64);
        self.store_pc(resume);
        let ret = self.builder.use_var(self.total_cycles);
        self.builder.ins().return_(&[ret]);
        self.builder.switch_to_block(no_fault);
        self.builder.seal_block(no_fault);

        // 2. Create Cranelift loop with deferred pre-loop block.
        let pre_loop = self.builder.create_block();
        let loop_header = self.builder.create_block();
        let loop_exit = self.builder.create_block();
        let after_loop = self.builder.create_block();

        // Check for DO annul (LC=0): skip body entirely
        self.flush_pending_cycles(); // flush pre-DO cycles before entering loop
        // Pre-DO CCR state must materialize before the annul branch: a
        // set_pending inside the body would strand it on the skip path.
        self.flush_pending_flags();
        // Flush dirty registers before the annul branch (mirroring
        // emit_rep_inline): the body may be annulled, and a flush inside
        // it clears compile-time dirty flags globally - without this,
        // pre-DO register state never reaches memory on the annul path.
        self.flush_promoted();
        self.emit_do_annul_check(lc_val, forever, do_pc, la, after_loop, true);
        self.builder.ins().jump(pre_loop, &[]);
        self.builder.switch_to_block(loop_header);
        // Don't seal loop_header yet -- back-edge coming.

        // 3. Push loop scope and emit all body instructions [do_pc+2, la].
        self.push_loop_scope(pre_loop);
        // Snapshot the hazard counter: the body may defer its final flag
        // computation across the back edge only if it emits no site that
        // could observe SR (or leave compiled code) mid-iteration.
        let hazard_mark = self.defer_hazard_sites;
        // Each iteration re-enters here with the quarter possibly elided.
        self.eunz_current = false;
        let body_start = do_pc + 2;
        let mut body_pc = body_start;
        // Open forward skips: (merge target, merge block, conditional
        // state), innermost last. `is_do_body_inlineable` validated the
        // nesting, so every target lands on a top-level boundary at or
        // before LA and skip regions contain no DO/REP.
        let mut open_skips: Vec<(u32, Block, ConditionalState)> = Vec::new();
        while body_pc <= la {
            while open_skips.last().is_some_and(|(t, _, _)| *t == body_pc) {
                let (_, merge_blk, mut cond_state) = open_skips.pop().unwrap();
                // Path-accurate cycles: the fall-through arm's cycle
                // charge lands inside the arm, matching step mode where
                // skipped instructions never execute.
                self.flush_pending_cycles();
                self.end_conditional_arm(&mut cond_state);
                self.builder.ins().jump(merge_blk, &[]);
                self.builder.switch_to_block(merge_blk);
                self.builder.seal_block(merge_blk);
                self.merge_conditional(&cond_state);
            }
            let opcode = self.map.read_pram(body_pc);
            let nw = self.map.read_pram(mask_pc(body_pc + 1));
            let inst = decode::decode(opcode);

            if let Some(target) = Self::forward_skip_target(&inst, body_pc, nw) {
                let inst_len = decode::instruction_length(&inst);
                debug_assert!(target >= body_pc + inst_len && target <= la);
                // The predicate runs unconditionally (operand read
                // side effects included), exactly as the branch
                // instruction would evaluate it.
                self.cur_inst_pc = body_pc;
                self.cur_decode_len = inst_len;
                let taken = self.emit_skip_predicate(&inst);
                let merge_blk = self.builder.create_block();
                let fall_blk = self.builder.create_block();
                let cond_state = self.begin_conditional_skip();
                self.builder
                    .ins()
                    .brif(taken, merge_blk, &[], fall_blk, &[]);
                self.builder.switch_to_block(fall_blk);
                self.builder.seal_block(fall_blk);
                open_skips.push((target, merge_blk, cond_state));
                body_pc += inst_len;
                continue;
            }

            if Self::is_do_instruction(&inst) {
                let inner_la = Self::compute_do_la(&inst, body_pc, nw);
                self.emit_do_inline(&inst, body_pc, inner_la);
                body_pc = inner_la + 1;
            } else if Self::is_rep_instruction(&inst) {
                self.emit_rep_inline(&inst, body_pc);
                let rep_next = body_pc + 1;
                let rep_opcode = self.map.read_pram(rep_next);
                let rep_inst = decode::decode(rep_opcode);
                let rep_len = decode::instruction_length(&rep_inst);
                body_pc = rep_next + rep_len;
            } else {
                self.emit_instruction(&inst, body_pc, nw);
                body_pc += decode::instruction_length(&inst);
            }
        }

        debug_assert!(open_skips.is_empty());

        // 4. Decrement LC, check loop continuation.
        self.flush_pending_cycles(); // flush body cycles once per iteration
        // The body's final CCR update materializes once per iteration here.
        // When nothing in the body can observe SR mid-iteration, the
        // E/U/N/Z helper call - a pure overwrite, so dead on every
        // iteration but the last - defers to the loop exit (and the
        // preemption bail) and runs once, on the final iteration's result.
        // The V/C/L/SM half still lands per iteration: L and the SM V/L
        // are sticky ORs. The exit edge leaves from the LC-decrement block
        // at the body's bottom, so the body-defined result dominates every
        // materialization site. (REP inlines defer the same way; see
        // emit_rep_inline.)
        let deferred_nz = if self.defer_hazard_sites == hazard_mark {
            self.flush_pending_flags_backedge_deferring_eunz()
        } else {
            self.flush_pending_flags();
            None
        };
        // Flush the body's register writes to memory once per iteration:
        // conditional-arm merges invalidate their destinations, and the
        // resulting inline memory reloads re-execute EVERY iteration - a
        // loop-carried value living only in a variable would be resurrected
        // stale from memory on paths that skip the arm. Iteration-boundary
        // memory currency makes every in-body reload sound.
        self.flush_promoted();
        // Route the backedge through a budget check so inline loops stay
        // preemptible (see emit_loop_preemption_check). DO FOREVER never
        // reaches here: emit_block excludes it from inlining and nested
        // FOREVER fails is_do_body_inlineable.
        let backedge = self.builder.create_block();
        self.emit_lc_decrement_and_branch(backedge, loop_exit);
        self.builder.switch_to_block(backedge);
        self.builder.seal_block(backedge);
        self.emit_loop_preemption_check(body_start, deferred_nz);
        self.builder.ins().jump(loop_header, &[]);

        // 5. Pop loop scope and emit pre-loop block with targeted loads.
        self.pop_loop_scope(loop_header);

        // 6. Switch to loop exit.
        self.builder.switch_to_block(loop_exit);
        self.builder.seal_block(loop_exit);

        // 7. Loop exit cleanup: pop stack, restore LA/LC/LF. A deferred
        // backedge E/U/N/Z call materializes first, on the last iteration's
        // result (this edge leaves the LC-decrement block, so it dominates)
        // - and before the LF restore reads SR.
        if let Some(result56) = deferred_nz {
            self.emit_deferred_nz(result56);
        }
        self.emit_enddo_cleanup();
        // Flush the body's final register state to memory on the loop path
        // (the annul path flushed inside the annul block). Registers whose
        // Cranelift variables are defined only inside the body have no
        // definition on the annul path - the merged block below must treat
        // memory as authoritative.
        self.flush_promoted();
        self.builder.ins().jump(after_loop, &[]);

        // 8. Merge point (reached from loop exit or annul skip). Both
        // incoming edges flushed their state; invalidate the promotion
        // cache so downstream code reloads from memory instead of using
        // variables that are undefined (annul path) or stale on one edge -
        // the block-end flush would otherwise store a zero-initialized
        // variable over valid state.
        self.builder.switch_to_block(after_loop);
        self.builder.seal_block(after_loop);
        // The annul path skips the loop (and any exit materialization),
        // so the quarter is not provably current past the merge.
        self.eunz_current = false;
        self.invalidate_promoted();
    }

    /// Compute the LC value for a REP instruction (from immediate, register,
    /// or memory). Returns a Cranelift Value with the 16-bit loop count.
    pub(super) fn emit_rep_lc_value(&mut self, inst: &Instruction) -> Value {
        match inst {
            Instruction::RepImm { count } => self.builder.ins().iconst(types::I32, *count as i64),
            Instruction::RepReg { reg_idx } => {
                let val = self.read_reg_for_move(*reg_idx as usize);
                self.mask_lc(val)
            }
            Instruction::RepAa { space, addr } => self.read_mem(*space, *addr as u32),
            Instruction::RepEa { space, ea_mode } => {
                let (ea_addr, _) = self.emit_calc_ea(*ea_mode as u32);
                self.read_mem_dyn(*space, ea_addr)
            }
            _ => unreachable!("not a REP instruction"),
        }
    }

    /// Compute the LC value for a DO/DOR instruction (from immediate, register,
    /// or memory). Returns a Cranelift Value with the 16-bit loop count.
    pub(super) fn emit_do_lc_value(&mut self, inst: &Instruction) -> Value {
        match inst {
            Instruction::DoImm { count } | Instruction::DorImm { count } => {
                self.builder.ins().iconst(types::I32, *count as i64)
            }
            Instruction::DoForever | Instruction::DorForever => {
                // emit_do_lc_value is only called from emit_do_inline, and
                // emit_block never inlines FOREVER loops (is_do_body_inlineable
                // rejects them) - the non-inline path goes through
                // emit_do_or_dor_forever instead.
                unreachable!("DO/DOR FOREVER is never inlined")
            }
            Instruction::DoReg { reg_idx } | Instruction::DorReg { reg_idx } => {
                // Manual page 13-56 claims DO SP loads "SP before DO,
                // incremented by one"; MCPX silicon loads SP unmodified
                // (probed at SP=1 and SP=3: the body runs exactly SP times).
                let val = self.read_reg_for_move(*reg_idx as usize);
                self.mask_lc(val)
            }
            Instruction::DoAa { space, addr } => {
                let mem_val = self.read_mem(*space, *addr as u32);
                self.mask_lc(mem_val)
            }
            Instruction::DoEa { space, ea_mode } | Instruction::DorEa { space, ea_mode } => {
                let (ea_addr, _) = self.emit_calc_ea(*ea_mode as u32);
                let mem_val = self.read_mem_dyn(*space, ea_addr);
                self.mask_lc(mem_val)
            }
            Instruction::DorAa { space, addr } => {
                let mem_val = self.read_mem(*space, *addr as u32);
                self.mask_lc(mem_val)
            }
            _ => unreachable!("not a DO instruction"),
        }
    }

    /// The step path's first call after a legacy REP, compiled: REP with
    /// LC=0 does not execute the target (hardware-verified; diverges from
    /// the 56300FM's "65,536 repeats") - annul it, restore LC from TEMP
    /// and drop the REP context; otherwise clear pc_on_rep and land on
    /// the (necessarily one-word) target. Mirrors `advance_pc`'s
    /// pc_on_rep arm exactly. Used when `emit_block` emits a REP through
    /// the legacy `loop_rep` machinery instead of inlining (a target that
    /// can arm a core fault, or a manual-restricted REP/DO target): the
    /// block ends here and the run loop steps while `loop_rep` is live,
    /// so the remaining iterations run the step leg's exact semantics.
    pub(super) fn emit_rep_first_call_arm(&mut self, rep_pc: u32) {
        self.flush_pending_cycles();
        let lc = self.load_reg(reg::LC);
        let zero = self.builder.ins().iconst(types::I32, 0);
        let annul = self.builder.ins().icmp(IntCC::Equal, lc, zero);

        let zero_blk = self.builder.create_block();
        let run_blk = self.builder.create_block();
        let merge = self.builder.create_block();
        self.builder.ins().brif(annul, zero_blk, &[], run_blk, &[]);

        self.builder.switch_to_block(zero_blk);
        self.builder.seal_block(zero_blk);
        let off = self.builder.ins().iconst(types::I8, 0);
        self.store_bool(OFF_LOOP_REP, off);
        let saved = self.load_reg(reg::TEMP);
        self.store_reg(reg::LC, saved);
        let skip_pc = self.builder.ins().iconst(types::I32, (rep_pc + 2) as i64);
        self.store_pc(skip_pc);
        self.builder.ins().jump(merge, &[]);

        self.builder.switch_to_block(run_blk);
        self.builder.seal_block(run_blk);
        let off2 = self.builder.ins().iconst(types::I8, 0);
        self.store_bool(OFF_PC_ON_REP, off2);
        let tgt_pc = self.builder.ins().iconst(types::I32, (rep_pc + 1) as i64);
        self.store_pc(tgt_pc);
        self.builder.ins().jump(merge, &[]);

        self.builder.switch_to_block(merge);
        self.builder.seal_block(merge);
        // PC was stored explicitly on both arms - branch-style block end.
        self.set_inst_len(0);
    }

    /// Common REP setup: save LC to TEMP, set loop_rep and pc_on_rep.
    pub(super) fn emit_rep_setup(&mut self) {
        let old_lc = self.load_reg(reg::LC);
        self.store_reg(reg::TEMP, old_lc);
        let one = self.builder.ins().iconst(types::I8, 1);
        self.store_bool(OFF_LOOP_REP, one);
        self.store_bool(OFF_PC_ON_REP, one);
    }

    pub(super) fn emit_rep_imm(&mut self, count: u16) {
        self.set_inst_len(1);
        self.set_cycles(5);
        self.emit_rep_setup();
        let cv = self.builder.ins().iconst(types::I32, count as i64);
        self.store_reg(reg::LC, cv);
    }

    /// ENDDO cleanup: pop (PC, SR), restore LF+FV from saved SR, pop (LA, LC).
    pub(super) fn emit_enddo_cleanup(&mut self) {
        // ENDDO pop-underflow budget: 5 stream words after the 1-word
        // ENDDO (silicon delivers at start+6 flat; the double pop takes
        // SP $00->$3F->$3E and the dispatch frame lands in slot 15,
        // probe_enddo_underflow). DO-annul pops share this
        // path; their budget is extrapolated from ENDDO (unprobed).
        let (_saved_pc, saved_sr) = self.stack_pop(5);
        // The LF/FV restore's SR read-modify-write: E/U/N/Z pass through
        // unobserved. Charge it to its own hazard kind.
        let sr_val = self.load_reg(reg::SR);
        let lf_fv_mask = (1u32 << sr::LF) | (1u32 << sr::FV);
        let mask = self.builder.ins().iconst(types::I32, lf_fv_mask as i64);
        let inv_mask = self.builder.ins().iconst(types::I32, !lf_fv_mask as i64);
        let sr_without = self.builder.ins().band(sr_val, inv_mask);
        let saved_flags = self.builder.ins().band(saved_sr, mask);
        let sr_new = self.builder.ins().bor(sr_without, saved_flags);
        self.store_reg(reg::SR, sr_new);
        let (la, lc) = self.stack_pop(5);
        self.store_reg(reg::LA, la);
        self.store_reg(reg::LC, lc);
    }

    pub(super) fn emit_enddo(&mut self) {
        self.set_inst_len(1);
        self.set_cycles(1);
        self.emit_enddo_cleanup();
    }

    /// Common DO loop setup: push LA/LC, set LA, push ret_pc/SR, set LF (+FV for forever), set LC.
    ///
    /// For non-forever DO with LC=0 and SC=0, the loop should be annulled
    /// (DOR p.13-61, Table 5-1 bit 13). Callers handle this via
    /// `emit_do_annul_check`.
    pub(super) fn emit_do_setup(
        &mut self,
        la_val: Value,
        lc_val: Value,
        ret_pc: u32,
        forever: bool,
    ) {
        let old_la = self.load_reg(reg::LA);
        let old_lc = self.load_reg(reg::LC);
        // DO push-overflow budget: 3 stream words after the 2-word DO
        // (silicon delivers at start+5, probe_do_overflow -
        // NOT the JSR push class's 9 - len). The second push normally
        // faults under the first's SE latch; if it faults alone the
        // same class budget is assumed.
        self.stack_push(old_la, old_lc, 3);
        self.store_reg(reg::LA, la_val);
        let ret = self
            .builder
            .ins()
            .iconst(types::I32, mask_pc(ret_pc) as i64);
        // DO setup's own SR read (stacked to SSL) - charge the hazard to
        // its own kind so an enclosing loop's gate can name it.
        let sr_val = self.load_reg(reg::SR);
        self.stack_push(ret, sr_val, 3);
        let sr_new = if forever {
            let flags = (1u32 << sr::LF) | (1u32 << sr::FV);
            let flag_bits = self.builder.ins().iconst(types::I32, flags as i64);
            self.builder.ins().bor(sr_val, flag_bits)
        } else {
            // Set LF, clear FV (a regular DO nested inside DO FOREVER must not
            // inherit the FV=1 from the outer loop).
            let set_lf = self
                .builder
                .ins()
                .iconst(types::I32, (1u32 << sr::LF) as i64);
            let sr_with_lf = self.builder.ins().bor(sr_val, set_lf);
            let clear_fv = self
                .builder
                .ins()
                .iconst(types::I32, !(1u32 << sr::FV) as i64);
            self.builder.ins().band(sr_with_lf, clear_fv)
        };
        self.store_reg(reg::SR, sr_new);
        self.store_reg(reg::LC, lc_val);
    }

    /// Emit a conditional annul check for DO with LC=0.
    ///
    /// If `lc_val == 0`, undoes the DO setup (pops stack, restores LF/FV),
    /// sets `inst_len` to skip past the loop body (to LA+1), and jumps to
    /// `annul_target`. Otherwise falls through.
    /// No-op for DO FOREVER (forever flag set).
    ///
    /// Per DOR page 13-61: "If the LC initial value is zero [...] the DO
    /// loop is not executed." (SC=0 behavior; SC=1 not implemented.)
    pub(super) fn emit_do_annul_check(
        &mut self,
        lc_val: Value,
        forever: bool,
        _do_pc: u32,
        la: u32,
        annul_target: Block,
        flush_on_annul: bool,
    ) {
        if forever {
            return;
        }
        let zero = self.builder.ins().iconst(types::I32, 0);
        let lc_is_zero = self.builder.ins().icmp(IntCC::Equal, lc_val, zero);

        let continue_block = self.builder.create_block();
        let annul_block = self.builder.create_block();
        self.builder
            .ins()
            .brif(lc_is_zero, annul_block, &[], continue_block, &[]);

        // Annul block: undo DO setup, jump to LA+1 using the branch pattern
        // (store_pc + inst_len=0) so the block JIT path picks up the correct
        // target address instead of falling through to the loop body.
        self.builder.switch_to_block(annul_block);
        self.builder.seal_block(annul_block);
        self.emit_enddo_cleanup();
        if flush_on_annul {
            // Inline-loop caller: the merge block invalidates the promotion
            // cache, so the annul path's cleanup stores (SR/LA/LC/SP) must
            // reach memory here. The non-inline caller terminates the block
            // right after; its unconditional block-end flush covers both
            // paths via merged variables, so it passes false.
            self.flush_promoted();
        }
        let target = self
            .builder
            .ins()
            .iconst(types::I32, mask_pc(la + 1) as i64);
        self.store_pc(target);
        self.set_inst_len(0);
        self.builder.ins().jump(annul_target, &[]);

        self.builder.switch_to_block(continue_block);
        self.builder.seal_block(continue_block);
    }

    /// Decrement LC, mask it, store it, and branch back to loop_header or to loop_exit.
    fn emit_lc_decrement_and_branch(&mut self, loop_header: Block, loop_exit: Block) {
        let lc = self.load_reg(reg::LC);
        let one = self.builder.ins().iconst(types::I32, 1);
        let new_lc = self.builder.ins().isub(lc, one);
        let new_lc = self.mask_lc(new_lc);
        self.store_reg(reg::LC, new_lc);
        // Flush the decremented LC: this store happens AFTER the body-end
        // flush, so it would otherwise cross the backedge only in the
        // variable - any in-body consumer that reads LC from memory (a
        // mid-body flush/invalidate point emitted earlier than this store,
        // whose flush set was fixed at emission time) would see a stale
        // count forever and never terminate.
        self.flush_reg(reg::LC);
        let zero = self.builder.ins().iconst(types::I32, 0);
        let done = self.builder.ins().icmp(IntCC::Equal, new_lc, zero);
        self.builder
            .ins()
            .brif(done, loop_exit, &[], loop_header, &[]);
    }

    /// Shared tail for non-forever DO/DOR: setup, annul check, merge block.
    fn emit_do_tail(&mut self, la_val: Value, lc_val: Value, pc: u32, la: u32) {
        let merge = self.builder.create_block();
        self.emit_do_setup(la_val, lc_val, pc + 2, false);
        self.emit_do_annul_check(lc_val, false, pc, la, merge, false);
        self.builder.ins().jump(merge, &[]);
        self.builder.switch_to_block(merge);
        self.builder.seal_block(merge);
    }

    /// Compute LA for DO (absolute) or DOR (relative).
    fn compute_la(pc: u32, next_word: u32, relative: bool) -> u32 {
        if relative {
            mask_pc(pc.wrapping_add(next_word))
        } else {
            next_word & REG_MASKS[reg::LA]
        }
    }

    fn emit_do_or_dor_imm(&mut self, count: u16, pc: u32, next_word: u32, relative: bool) {
        self.set_inst_len(2);
        self.set_cycles(5);
        let la = Self::compute_la(pc, next_word, relative);
        let la_val = self.builder.ins().iconst(types::I32, la as i64);
        let lc_val = self.builder.ins().iconst(types::I32, count as i64);
        self.emit_do_tail(la_val, lc_val, pc, la);
    }

    pub(super) fn emit_do_imm(&mut self, count: u16, pc: u32, next_word: u32) {
        self.emit_do_or_dor_imm(count, pc, next_word, false);
    }

    pub(super) fn emit_dor_imm(&mut self, count: u16, pc: u32, next_word: u32) {
        self.emit_do_or_dor_imm(count, pc, next_word, true);
    }

    fn emit_do_or_dor_forever(&mut self, pc: u32, next_word: u32, relative: bool) {
        self.set_inst_len(2);
        // DO FOREVER is 4 cycles, DOR FOREVER is 5 cycles.
        self.set_cycles(if relative { 5 } else { 4 });
        let la = Self::compute_la(pc, next_word, relative);
        let la_val = self.builder.ins().iconst(types::I32, la as i64);
        let lc_val = self.load_reg(reg::LC);
        self.emit_do_setup(la_val, lc_val, pc + 2, true);
    }

    pub(super) fn emit_do_forever(&mut self, pc: u32, next_word: u32) {
        self.emit_do_or_dor_forever(pc, next_word, false);
    }

    pub(super) fn emit_dor_forever(&mut self, pc: u32, next_word: u32) {
        self.emit_do_or_dor_forever(pc, next_word, true);
    }

    fn emit_do_or_dor_reg(&mut self, reg_idx: u8, pc: u32, next_word: u32, relative: bool) {
        self.set_inst_len(2);
        self.set_cycles(5);
        let la = Self::compute_la(pc, next_word, relative);
        let la_val = self.builder.ins().iconst(types::I32, la as i64);
        let numreg = reg_idx as usize;
        // Manual page 13-56 claims DO SP loads "SP before DO, incremented
        // by one"; MCPX silicon loads SP unmodified (probed at SP=1 and
        // SP=3 - see ARCHITECTURE-NOTES.md).
        let val = self.read_reg_for_move(numreg);
        let lc_val = self.mask_lc(val);
        self.emit_do_tail(la_val, lc_val, pc, la);
    }

    pub(super) fn emit_do_reg(&mut self, reg_idx: u8, pc: u32, next_word: u32) {
        self.emit_do_or_dor_reg(reg_idx, pc, next_word, false);
    }

    pub(super) fn emit_dor_reg(&mut self, reg_idx: u8, pc: u32, next_word: u32) {
        self.emit_do_or_dor_reg(reg_idx, pc, next_word, true);
    }

    pub(super) fn emit_rep_reg(&mut self, reg_idx: u8) {
        self.set_inst_len(1);
        self.set_cycles(5);
        self.emit_rep_setup();
        let numreg = reg_idx as usize;
        let lc_val = self.read_reg_for_move(numreg);
        let lc_masked = self.mask_lc(lc_val);
        self.store_reg(reg::LC, lc_masked);
    }

    fn emit_do_or_dor_aa(
        &mut self,
        space: MemSpace,
        addr: u8,
        pc: u32,
        next_word: u32,
        relative: bool,
    ) {
        self.set_inst_len(2);
        self.set_cycles(5);
        let la = Self::compute_la(pc, next_word, relative);
        let la_val = self.builder.ins().iconst(types::I32, la as i64);
        let addr = addr as u32;
        let mem_val = self.read_mem(space, addr);
        let lc_val = self.mask_lc(mem_val);
        self.emit_do_tail(la_val, lc_val, pc, la);
    }

    pub(super) fn emit_do_aa(&mut self, space: MemSpace, addr: u8, pc: u32, next_word: u32) {
        self.emit_do_or_dor_aa(space, addr, pc, next_word, false);
    }

    pub(super) fn emit_dor_aa(&mut self, space: MemSpace, addr: u8, pc: u32, next_word: u32) {
        self.emit_do_or_dor_aa(space, addr, pc, next_word, true);
    }

    fn emit_do_or_dor_ea(
        &mut self,
        space: MemSpace,
        ea_mode: u8,
        pc: u32,
        next_word: u32,
        relative: bool,
    ) {
        self.set_inst_len(2);
        self.set_cycles(5);
        let la = Self::compute_la(pc, next_word, relative);
        let la_val = self.builder.ins().iconst(types::I32, la as i64);
        let (ea_addr, _) = self.emit_calc_ea(ea_mode as u32);
        let mem_val = self.read_mem_dyn(space, ea_addr);
        let lc_val = self.mask_lc(mem_val);
        self.emit_do_tail(la_val, lc_val, pc, la);
    }

    pub(super) fn emit_do_ea(&mut self, space: MemSpace, ea_mode: u8, pc: u32, next_word: u32) {
        self.emit_do_or_dor_ea(space, ea_mode, pc, next_word, false);
    }

    pub(super) fn emit_dor_ea(&mut self, space: MemSpace, ea_mode: u8, pc: u32, next_word: u32) {
        self.emit_do_or_dor_ea(space, ea_mode, pc, next_word, true);
    }

    pub(super) fn emit_rep_aa(&mut self, space: MemSpace, addr: u8) {
        self.set_inst_len(1);
        self.set_cycles(5);
        self.emit_rep_setup();
        let addr = addr as u32;
        let mem_val = self.read_mem(space, addr);
        self.store_reg(reg::LC, mem_val);
    }

    pub(super) fn emit_rep_ea(&mut self, space: MemSpace, ea_mode: u8) {
        self.set_inst_len(1);
        self.set_cycles(5);
        self.emit_rep_setup();
        let (ea_addr, _) = self.emit_calc_ea(ea_mode as u32);
        let mem_val = self.read_mem_dyn(space, ea_addr);
        self.store_reg(reg::LC, mem_val);
    }

    pub(super) fn emit_brkcc(&mut self, cc: CondCode) {
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
        // Pop the loop frame and restore LF/FV/LA/LC, then jump to LA+1
        // (manual p.13-28; hardware-verified with a LEGAL
        // spelling - no arithmetic immediately before the brk, per
        // restriction A.3.4, and outside the LA-2..LA zone). Violating
        // A.3.4 makes hardware skip the restore entirely; see
        // docs/ARCHITECTURE-NOTES.md "Restriction-violation behaviors".
        // Read current LA before cleanup pops it.
        let la = self.load_reg(reg::LA);
        let one = self.builder.ins().iconst(types::I32, 1);
        let target = self.builder.ins().iadd(la, one);
        let target = self.mask24(target);
        self.emit_enddo_cleanup();
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
}

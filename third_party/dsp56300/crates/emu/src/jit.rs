//! DSP56300 emulator with a JIT execution engine.
//!
//! Manages the Cranelift JIT module, block cache, and run loop. Provides
//! compilation and execution of individual instructions and basic blocks.

use std::collections::HashMap;
use std::io::Write;

use cranelift_codegen::ir::{AbiParam, types};
use cranelift_codegen::settings::{self, Configurable};
use cranelift_frontend::{FunctionBuilder, FunctionBuilderContext};
use cranelift_jit::{JITBuilder, JITModule};
use cranelift_module::Module;

use crate::core::{DspState, InterruptState, MemoryMap, PowerState, REG_MASKS, interrupt, reg, sr};
use crate::emit::Emitter;
use dsp56300_core::{Instruction, decode, mask_pc};

/// Longest block the translator will build.
///
/// Sits on the flat part of the dispatch-overhead curve
/// (`bench_block_dispatch_overhead`): host time per DSP cycle falls
/// steeply up to a cap of about 24 and is flat from there to 120. A larger
/// cap merges loop bodies this one splits, but removing a dispatch does not
/// remove the work it dispatched, so throughput does not move.
const MAX_BLOCK_LEN: u32 = 32;

/// Compiled function signature: takes a pointer to DspState, returns cycles.
type CompiledFn = unsafe fn(*mut DspState) -> i32;

/// A compiled basic block.
#[derive(Clone, Copy)]
struct CompiledBlock {
    func: CompiledFn,
    /// PC after the last instruction in this block (exclusive end of code range).
    end_pc: u32,
    /// PRAM generation at compilation time. When this matches the current
    /// pram_dirty.generation, the dirty bitmap scan is skipped (the block is known clean).
    generation: u32,
    /// The block ended on the instruction cap, not on a terminator or a DO
    /// boundary. Its successor dispatch exists only because of the cap.
    ends_open: bool,
}

/// Code cache: flat array indexed by start_pc.
///
/// A flat array eliminates all HashMap hashing overhead that was the dominant
/// cost in the run loop (SipHash on every lookup).
///
/// When a DO loop boundary (stop_pc < cached block's end_pc) would truncate a
/// block, the entry is evicted and recompiled with the tighter boundary.
struct CodeCache {
    blocks: Vec<Option<CompiledBlock>>,
    /// Widest extent (end_pc - start_pc) ever inserted. A block overlapping
    /// [lo, hi] can start no earlier than lo - (max_span - 1), so range
    /// invalidation scans that window instead of all of PRAM; a program
    /// that pages overlays invalidates tens of thousands of times a second
    /// from the run loop.
    max_span: u32,
}

impl CodeCache {
    fn new(pram_size: usize) -> Self {
        Self {
            blocks: vec![None; pram_size],
            max_span: 0,
        }
    }

    fn insert(&mut self, pc: u32, block: CompiledBlock) {
        self.max_span = self.max_span.max(block.end_pc.saturating_sub(pc));
        self.blocks[pc as usize] = Some(block);
    }

    /// Invalidate all cached blocks (e.g. when P-memory changes).
    fn invalidate_all(&mut self) {
        self.blocks.fill(None);
    }

    /// Invalidate only blocks whose code range [start_pc, end_pc) overlaps [lo, hi].
    fn invalidate_range(&mut self, lo: u32, hi: u32) {
        if self.blocks.is_empty() {
            return;
        }
        let first = (lo as usize).saturating_sub(self.max_span.saturating_sub(1) as usize);
        let hi = (hi as usize).min(self.blocks.len() - 1);
        for pc in first..=hi {
            if let Some(block) = &self.blocks[pc]
                && (block.end_pc as usize) > lo as usize
            {
                self.blocks[pc] = None;
            }
        }
    }
}

/// What one start PC cost, accumulated as it runs.
///
/// `words` sums each dispatch's `end_pc - pc`: the cache entry at dump time
/// is a different question (the PC may have been evicted, or recompiled over
/// an overlay load with a different extent), and reading the extent from
/// there reports every such PC as a one-word block.
///
/// `ticks` is the reason this exists. Hits and cycles say how much guest
/// work a PC did, not what it cost the host, and the two disagree: a hot
/// loop can be a third of the cycles while hundreds of other blocks share
/// the rest at some average nobody has measured. Guest cycles are a
/// constant per instruction; host time is not.
#[derive(Clone, Copy, Default, PartialEq, Eq, Debug)]
struct BlockStat {
    hits: u64,
    cycles: u64,
    words: u64,
    ticks: u64,
}

/// Host time the run loop spends around compiled code, split by phase.
///
/// Global counters, not per-PC: the loop is the same code for every block,
/// and the question it answers is what a dispatch pays outside the block -
/// the residual the per-block profile can only report as a gap against the
/// caller's wall clock. Phases cover the whole loop iteration, so their sum
/// plus block time is the loop's cost and nothing is left to inference.
#[derive(Clone, Copy, Default)]
struct DispatchStat {
    /// Iterations that dispatched a compiled block.
    iters: u64,
    /// Loop top to the block call: power checks, stop_pc, the dirty/evict
    /// check, the cache lookup. Compile time is carved out into
    /// `compile_ticks`, so this is the price every dispatch pays.
    pre_ticks: u64,
    /// Block return to iteration end: mode check, DO loop-back, pending
    /// interrupts.
    post_ticks: u64,
    /// get_or_compile_block plus the invalidation that may follow, and how
    /// many dispatches paid it. Covers both fresh compiles and rebuilds
    /// from retained translations.
    compile_ticks: u64,
    compile_count: u64,
    /// Single-step fallback iterations (interrupt pipeline, PC outside
    /// PRAM), timed loop top to their continue.
    step_ticks: u64,
    step_count: u64,
}

/// Per-PC block statistics plus what it takes to read the clock they use.
struct BlockProfile {
    stats: Vec<BlockStat>,
    /// Where the loop's time goes when it is not inside a block.
    dispatch: DispatchStat,
    /// Nanoseconds per `read_ticks()` unit, 0 when this target has no
    /// cheap counter and every `ticks` is 0.
    ns_per_tick: f64,
    /// What an empty timed region reads, in ticks. Both reads of the pair
    /// land partly inside the interval they bracket, so every dispatch is
    /// biased up by roughly this much; the dump subtracts `hits * probe`
    /// and reports the correction so it can be checked rather than trusted.
    probe_ticks: u64,
}

/// A cheap monotonic counter, read twice around each block dispatch.
///
/// Not serialising: the CPU may move a read past neighbouring work. The
/// indirect call to compiled code sits between the pair and does not get
/// reordered around, and the calibration probe uses the same instruction
/// pair, so the bias it leaves is the one `probe_ticks` measures.
#[inline(always)]
fn read_ticks() -> u64 {
    #[cfg(target_arch = "x86_64")]
    {
        // SAFETY: rdtsc is unprivileged and reads no memory.
        unsafe { core::arch::x86_64::_rdtsc() }
    }
    #[cfg(target_arch = "aarch64")]
    {
        let v: u64;
        // SAFETY: CNTVCT_EL0 is readable from EL0 and has no side effects.
        unsafe { core::arch::asm!("mrs {}, cntvct_el0", out(reg) v) };
        v
    }
    #[cfg(not(any(target_arch = "x86_64", target_arch = "aarch64")))]
    {
        0
    }
}

impl BlockProfile {
    fn new(pram_size: usize) -> Self {
        let (ns_per_tick, probe_ticks) = if read_ticks() == 0 {
            (0.0, 0)
        } else {
            (Self::calibrate_rate(), Self::calibrate_probe())
        };
        BlockProfile {
            stats: vec![BlockStat::default(); pram_size],
            dispatch: DispatchStat::default(),
            ns_per_tick,
            probe_ticks,
        }
    }

    /// Nanoseconds per tick, from a millisecond of wall clock. Paid once,
    /// when profiling is switched on.
    fn calibrate_rate() -> f64 {
        let t0 = read_ticks();
        let w0 = std::time::Instant::now();
        while w0.elapsed() < std::time::Duration::from_millis(1) {
            std::hint::spin_loop();
        }
        let ns = w0.elapsed().as_nanos() as f64;
        let ticks = read_ticks().wrapping_sub(t0) as f64;
        if ticks > 0.0 { ns / ticks } else { 0.0 }
    }

    /// What a timed region costs when it contains nothing. The minimum of
    /// many, not the mean: an interrupt landing inside the probe inflates
    /// it, and an over-estimate here would subtract real time from every
    /// block in the dump.
    fn calibrate_probe() -> u64 {
        let mut best = u64::MAX;
        for _ in 0..1000 {
            let t0 = read_ticks();
            let t1 = read_ticks();
            best = best.min(t1.wrapping_sub(t0));
        }
        best
    }
}

/// JIT compilation engine.
pub struct JitEngine {
    module: Option<JITModule>,
    ctx: cranelift_codegen::Context,
    func_ctx: FunctionBuilderContext,
    ptr_ty: cranelift_codegen::ir::Type,
    cache: CodeCache,
    /// Cache for single-instruction compilation (used by execute_one).
    /// Key: (pc, opcode, next_word) -> (compiled function, instruction length).
    instr_cache: HashMap<(u32, u32, u32), (CompiledFn, u32)>,
    /// Perf map file for profiling JIT blocks with `perf record` (Linux only).
    #[cfg(target_os = "linux")]
    perf_map: Option<std::fs::File>,
    /// Number of PRAM words (determines cache and profile array sizes).
    pram_size: usize,
    /// Block execution profiler, one entry per PC. See `BlockProfile`.
    block_profile: Option<BlockProfile>,
    /// Translations kept past invalidation, keyed by the code itself.
    /// Key: (start_pc, stop_pc) -> candidates (the words translated, function).
    translations: HashMap<(u32, u32), Vec<Translation>>,
    /// Total candidates held, so the cache can be bounded.
    translation_count: usize,
    /// Inline-loop preemption quantum baked into the blocks this engine
    /// compiles. A property of the engine, never of a run() call - see
    /// `emit_loop_preemption_check`.
    loop_quantum: i32,
    /// Instruction cap for the blocks this engine compiles. `MAX_BLOCK_LEN`
    /// except in the bench that measures what the cap costs.
    max_block_len: u32,
    /// Translation accounting. Cheap enough to keep unconditional, and the
    /// only thing that distinguishes a slow program from one whose code is
    /// being rebuilt faster than it runs.
    pub stats: JitStats,
}

/// Translation counters.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct JitStats {
    /// Blocks handed to Cranelift.
    pub compiles: u64,
    /// Nanoseconds spent translating.
    pub compile_ns: u64,
    /// Longest single translation, in nanoseconds. Cumulative maximum: a
    /// window in which it grows is a window that contained the worst
    /// compile seen so far, which is what a latency burst looks like from
    /// the counters.
    pub compile_ns_worst: u64,
    /// Cached blocks dropped because the words under them changed.
    pub invalidations: u64,
    /// Translations reused from the content cache instead of rebuilt.
    pub cache_hits: u64,
    /// Translations currently retained, against `MAX_TRANSLATIONS`.
    pub retained: u64,
    /// Times the run loop dispatched a compiled block. With the cycles
    /// retired, this gives the average block length and so how much of the
    /// per-cycle cost is dispatch rather than generated code.
    pub block_entries: u64,
    /// Dispatches whose block was cut short by the enclosing DO loop's LA+1.
    /// The run loop's loop-back is what these pay for.
    pub block_ends_do_boundary: u64,
    /// Dispatches whose block hit the instruction cap. Every one of these
    /// forces a further dispatch that a larger cap would have absorbed.
    pub block_ends_open: u64,
    /// Machine-code bytes emitted, live and evicted alike: how much code the
    /// host's instruction cache has been asked to hold. Measured, it is what
    /// sets the cost of a block dispatch - see
    /// `bench_block_dispatch_overhead`.
    pub code_bytes: u64,
    /// Where a block compile's time goes: building the CLIF (`emit_ns`),
    /// Cranelift's lowering, register allocation and encoding
    /// (`codegen_ns`), and placing the bytes in executable memory
    /// (`finalize_ns`). Investigation counters for the burst-latency work;
    /// three clock reads per compile, nothing on the dispatch path.
    pub emit_ns: u64,
    pub codegen_ns: u64,
    pub finalize_ns: u64,
}

/// A retained translation: the words it was compiled from, the function,
/// and whether the block ended at the instruction cap rather than at a
/// terminator.
type Translation = (Box<[u32]>, CompiledFn, bool);

/// Whether PRAM still holds `words` starting at `start_pc`. Two translations
/// of a range are interchangeable exactly when its words are unchanged, so
/// this is compared in full rather than by a digest.
fn pram_matches(map: &MemoryMap, start_pc: u32, words: &[u32]) -> bool {
    words
        .iter()
        .enumerate()
        .all(|(i, &w)| map.read_pram(start_pc + i as u32) == w)
}

/// Snapshot the PRAM words a block covers.
fn block_words(map: &MemoryMap, start_pc: u32, end_pc: u32) -> Box<[u32]> {
    (start_pc..end_pc).map(|pc| map.read_pram(pc)).collect()
}

/// Cap on retained translations. Real programs cycle a bounded set of
/// overlays; a runaway generator of distinct code should not grow the cache
/// without limit, so past this it is dropped wholesale and refilled.
const MAX_TRANSLATIONS: usize = 16384;

impl JitEngine {
    pub fn new(pram_size: usize) -> Self {
        let module = Self::new_module();
        let ptr_ty = module.isa().pointer_type();
        let ctx = module.make_context();
        let func_ctx = FunctionBuilderContext::new();

        Self {
            module: Some(module),
            ctx,
            func_ctx,
            ptr_ty,
            cache: CodeCache::new(pram_size),
            instr_cache: HashMap::new(),
            #[cfg(target_os = "linux")]
            perf_map: None,
            pram_size,
            block_profile: None,
            translations: HashMap::new(),
            translation_count: 0,
            loop_quantum: crate::emit::INLINE_LOOP_QUANTUM,
            max_block_len: MAX_BLOCK_LEN,
            stats: JitStats::default(),
        }
    }

    /// Create a fresh Cranelift JIT module.
    ///
    /// `regalloc_algorithm` is the single largest performance setting in the
    /// emulator. `single_pass` never splits a live range, so anything that
    /// outlives a few instructions spills, and this emitter promotes 36 DSP
    /// registers plus two 56-bit accumulators to Cranelift variables that
    /// live the whole block. Backtracking keeps them in machine registers:
    /// 15-24% less host time per DSP cycle for the same cycles per dispatch.
    ///
    /// It roughly doubles translation time, which the content-keyed
    /// translation cache absorbs: steady state compiles nothing, so the
    /// bill is paid during warm-up. A program that reloads overlays
    /// continuously would see the trade go the other way.
    ///
    /// `opt_level` stays at `none`: `speed` measures inside the noise while
    /// costing 2.5x the translation time and a worst-case compile over a
    /// millisecond.
    fn new_module() -> JITModule {
        let mut flag_builder = settings::builder();
        let _ = flag_builder.set("opt_level", "none");
        // Cranelift's verifier is what proves a promoted-register or
        // deferred-flag SSA value still dominates its uses after an emitter
        // change. It roughly doubles translation time, so debug builds pay
        // it and release builds do not.
        let verify = if cfg!(debug_assertions) {
            "true"
        } else {
            "false"
        };
        let _ = flag_builder.set("enable_verifier", verify);
        let _ = flag_builder.set("unwind_info", "false");
        let _ = flag_builder.set("regalloc_algorithm", "backtracking");
        let isa_builder = cranelift_native::builder().unwrap();
        let isa = isa_builder
            .finish(settings::Flags::new(flag_builder))
            .unwrap();
        let builder = JITBuilder::with_isa(isa, cranelift_module::default_libcall_names());
        JITModule::new(builder)
    }

    /// Instruction cap for the blocks this engine compiles; the bench
    /// that prices translation against block length sets it.
    #[doc(hidden)]
    pub fn set_max_block_len(&mut self, cap: u32) {
        self.max_block_len = cap.max(1);
    }

    /// Enable perf map output for `perf record` profiling (Linux only).
    /// Creates `/tmp/perf-<pid>.map`.
    #[cfg(target_os = "linux")]
    pub fn enable_perf_map(&mut self) {
        if self.perf_map.is_none() {
            self.perf_map =
                std::fs::File::create(format!("/tmp/perf-{}.map", std::process::id())).ok();
        }
    }

    /// No-op on non-Linux platforms.
    #[cfg(not(target_os = "linux"))]
    pub fn enable_perf_map(&mut self) {}

    /// Enable block execution profiling (hits, cycles, extent and host time
    /// per PC). Calibrates the clock, so it costs a millisecond.
    pub fn enable_profiling(&mut self) {
        if self.block_profile.is_none() {
            self.block_profile = Some(BlockProfile::new(self.pram_size));
        }
    }

    pub fn is_profiling(&self) -> bool {
        self.block_profile.is_some()
    }

    /// Number of compiled blocks in the cache.
    pub fn block_count(&self) -> usize {
        self.cache.blocks.iter().filter(|b| b.is_some()).count()
    }

    /// Number of cached single-instruction compilations.
    pub fn instr_cache_count(&self) -> usize {
        self.instr_cache.len()
    }

    /// Iterate over compiled blocks: yields (start_pc, end_pc, num_words).
    pub fn block_sizes(&self) -> Vec<(u32, u32, u32)> {
        self.cache
            .blocks
            .iter()
            .enumerate()
            .filter_map(|(pc, b)| {
                b.as_ref()
                    .map(|b| (pc as u32, b.end_pc, b.end_pc - pc as u32))
            })
            .collect()
    }

    /// Set the inline-loop preemption quantum for blocks compiled from here
    /// on, dropping everything already compiled. Test-only, behind the
    /// `tunable-loop-quantum` feature: preemption must be a property of the
    /// guest program, so an embedder never gets to move it. The differential
    /// tests need two engines that disagree about it in one process.
    #[cfg(feature = "tunable-loop-quantum")]
    #[doc(hidden)]
    pub fn set_inline_loop_quantum(&mut self, cycles: i32) {
        self.loop_quantum = cycles;
        self.invalidate_cache();
    }

    /// Invalidate all cached blocks and release compiled code memory.
    pub fn invalidate_cache(&mut self) {
        self.cache.invalidate_all();
        self.instr_cache.clear();
        // Retained translations point into the module's code memory, which
        // is about to be freed.
        self.translations.clear();
        self.translation_count = 0;
        self.stats.retained = 0;
        if let Some(old) = self.module.replace(Self::new_module()) {
            unsafe { old.free_memory() };
        }
    }

    /// Invalidate block cache only, keeping the instruction cache and
    /// Cranelift module intact. Use when PRAM layout changes but the
    /// same opcodes are likely to recur (e.g. fuzzing).
    pub fn invalidate_blocks(&mut self) {
        self.cache.invalidate_all();
    }

    /// Invalidate only blocks whose code overlaps P-memory range [lo, hi].
    pub fn invalidate_range(&mut self, lo: u32, hi: u32) {
        self.cache.invalidate_range(lo, hi);
        self.instr_cache.retain(|&(pc, _, _), _| pc < lo || pc > hi);
    }

    /// Dump the block execution profile to a file, ordered by host time
    /// where the target has a counter to read and by guest cycles where it
    /// does not. Each line: pc and mean extent, hits, cycles, host ns, and
    /// the two rates that separate a block that runs often from one that
    /// runs slowly.
    pub fn dump_profile(&self, map: &MemoryMap, path: &str) {
        let Some(ref profile) = self.block_profile else {
            return;
        };
        let mut entries: Vec<(u32, BlockStat)> = profile
            .stats
            .iter()
            .enumerate()
            .filter(|(_, s)| s.hits > 0)
            .map(|(pc, s)| (pc as u32, *s))
            .collect();
        // By host time where there is a clock: the whole question this
        // answers is which blocks carry the nanoseconds, and cycles are
        // exactly the answer that does not distinguish them.
        let timed = profile.ns_per_tick > 0.0;
        if timed {
            entries.sort_by_key(|(_, s)| std::cmp::Reverse(s.ticks));
        } else {
            entries.sort_by_key(|(_, s)| std::cmp::Reverse(s.cycles));
        }

        let total_cycles: u64 = entries.iter().map(|(_, s)| s.cycles).sum();
        let total_hits: u64 = entries.iter().map(|(_, s)| s.hits).sum();
        let total_ticks: u64 = entries.iter().map(|(_, s)| s.ticks).sum();
        // The clock reads are the profiler's own cost, not the guest's.
        let probe = profile.probe_ticks;
        let net = |s: &BlockStat| s.ticks.saturating_sub(s.hits * probe);
        let total_net: u64 = entries.iter().map(|(_, s)| net(s)).sum();
        let ns = |t: u64| t as f64 * profile.ns_per_tick;

        let mut f = match std::fs::File::create(path) {
            Ok(f) => f,
            Err(_) => return,
        };
        let _ = writeln!(
            f,
            "{:<24} {:>10} {:>14} {:>8} {:>6} {:>12} {:>8} {:>8}",
            "block", "hits", "cycles", "avg", "pct", "ns", "ns/disp", "ns/cyc"
        );
        let _ = writeln!(f, "{}", "-".repeat(100));
        for (pc, s) in &entries {
            // Words per dispatch is the mean of what actually ran, not the
            // extent of whatever block happens to sit at this PC now.
            let avg_words = s.words as f64 / s.hits as f64;
            let pct = if timed && total_net > 0 {
                (net(s) as f64 / total_net as f64) * 100.0
            } else {
                (s.cycles as f64 / total_cycles.max(1) as f64) * 100.0
            };
            let block_ns = ns(net(s));
            let _ = writeln!(
                f,
                "{:04x}+{:<6.2}w            {:>10} {:>14} {:>8} {:>5.1}% {:>12.0} {:>8.1} {:>8.2}",
                pc,
                avg_words,
                s.hits,
                s.cycles,
                s.cycles / s.hits.max(1),
                pct,
                block_ns,
                block_ns / s.hits as f64,
                block_ns / s.cycles.max(1) as f64,
            );
        }
        let _ = writeln!(f, "\ntotal_cycles: {}", total_cycles);
        if timed {
            // The correction is reported, not just applied: it is a
            // per-dispatch constant, so it lands hardest on exactly the
            // short blocks this dump exists to weigh, and a reader has to
            // be able to see how much of the answer it is.
            let _ = writeln!(
                f,
                "total_ns: {:.0}\nns_per_cycle: {:.3}\n\
                 profiler_probe_ns: {:.0} ({:.1}% of {:.0} raw, {:.2} ns x {} dispatches)",
                ns(total_net),
                ns(total_net) / total_cycles.max(1) as f64,
                ns(total_ticks - total_net),
                if total_ticks > 0 {
                    ((total_ticks - total_net) as f64 / total_ticks as f64) * 100.0
                } else {
                    0.0
                },
                ns(total_ticks),
                ns(probe),
                total_hits,
            );
            // Block time is the time inside compiled code. Everything the
            // run loop does between dispatches is timed on its own, split
            // by phase, so the dispatch overhead reads as numbers rather
            // than a residual against the caller's wall clock. Totals are
            // cumulative like the block table; difference two dumps for a
            // window. Same probe correction, same reporting of it.
            let d = &profile.dispatch;
            let dnet = |t: u64, n: u64| t.saturating_sub(n * probe);
            let per = |t: u64, n: u64| ns(dnet(t, n)) / n.max(1) as f64;
            let _ = writeln!(
                f,
                "dispatch_pre_ns: {:.0} ({} dispatches, {:.2} ns/disp)\n\
                 dispatch_post_ns: {:.0} ({:.2} ns/disp)\n\
                 dispatch_compile_ns: {:.0} ({} compiles, {:.0} ns/compile)\n\
                 dispatch_step_ns: {:.0} ({} steps)",
                ns(dnet(d.pre_ticks, d.iters)),
                d.iters,
                per(d.pre_ticks, d.iters),
                ns(dnet(d.post_ticks, d.iters)),
                per(d.post_ticks, d.iters),
                ns(dnet(d.compile_ticks, d.compile_count)),
                d.compile_count,
                per(d.compile_ticks, d.compile_count),
                ns(dnet(d.step_ticks, d.step_count)),
                d.step_count,
            );
        } else {
            let _ = writeln!(f, "total_ns: unavailable (no cycle counter on this target)");
        }
        // Why blocks end, cumulative over the engine's life. Differenced
        // between two dumps these say how many dispatches the instruction
        // cap and the DO-loop boundary each create.
        let _ = writeln!(
            f,
            "block_entries: {}\nblock_ends_open: {}\nblock_ends_do_boundary: {}",
            self.stats.block_entries, self.stats.block_ends_open, self.stats.block_ends_do_boundary
        );

        // Dump raw P-space words for offline disassembly
        let _ = writeln!(f, "\n\n{}", "=".repeat(80));
        let _ = writeln!(f, "P-SPACE DUMP OF TOP 20 BLOCKS");
        let _ = writeln!(f, "{}", "=".repeat(80));
        let p_end = map.p_space_end();
        for (pc, st) in entries.iter().take(20) {
            // Dump the widest extent this PC is known to have run with: the
            // mean rounded up, or the live cache entry if it reaches further
            // (a block recompiled since, or one preempted short of its end).
            let avg_words = (st.words as f64 / st.hits as f64).ceil() as u32;
            let cached_end = self.cache.blocks[*pc as usize]
                .as_ref()
                .map(|b| b.end_pc)
                .unwrap_or(0);
            let end_pc = cached_end.max(pc + avg_words.max(1));
            let pct = (st.cycles as f64 / total_cycles.max(1) as f64) * 100.0;
            let _ = writeln!(
                f,
                "\n=== Block {:04x}..{:04x} ({:.2} words/dispatch, {:.1}% of cycles, \
                 {} cycles, {:.0} ns) ===",
                pc,
                end_pc,
                st.words as f64 / st.hits as f64,
                pct,
                st.cycles,
                ns(net(st)),
            );
            for addr in *pc..end_pc.min(p_end) {
                let _ = writeln!(f, "P {:04X} {:06X}", addr, map.read_pram(addr));
            }
        }

        // Full P-space, so the histogram above can be disassembled in
        // context: the hot PCs sit inside DO loops whose header is outside
        // any one block's extent, and on a program that swaps overlays the
        // code at a given address is whichever overlay is resident right
        // now - captured here, at the same moment as the counters, or not
        // at all.
        let _ = writeln!(f, "\n\n{}", "=".repeat(80));
        let _ = writeln!(f, "FULL P-SPACE DUMP ({} words)", p_end);
        let _ = writeln!(f, "{}", "=".repeat(80));
        for addr in 0..p_end {
            let _ = writeln!(f, "P {:04X} {:06X}", addr, map.read_pram(addr));
        }
    }

    /// Get a cached compiled instruction or compile and cache it.
    ///
    /// Cache key optimizations to maximize sharing:
    /// - PC-independent instructions use `pc_key=0` (one compilation serves all addresses)
    /// - Single-word instructions use `nw_key=0` (next_word is irrelevant)
    /// - PC-dependent instructions (branches, DO loops, LRA) include the actual PC
    pub fn get_or_compile_instruction(
        &mut self,
        pc: u32,
        opcode: u32,
        next_word: u32,
        map: &MemoryMap,
    ) -> (CompiledFn, u32) {
        let inst = decode::decode(opcode);
        let inst_len = decode::instruction_length(&inst);
        let pc_key = if Self::instruction_uses_pc(&inst) {
            pc
        } else {
            0
        };
        let nw_key = if inst_len > 1 { next_word } else { 0 };
        let key = (pc_key, opcode, nw_key);
        if let Some(&entry) = self.instr_cache.get(&key) {
            return entry;
        }
        let func = self.compile_instruction(&inst, pc, next_word, map);
        self.instr_cache.insert(key, (func, inst_len));
        (func, inst_len)
    }

    /// Returns true if the instruction's compiled code depends on PC.
    fn instruction_uses_pc(inst: &Instruction) -> bool {
        matches!(
            inst,
            Instruction::Bra { .. }
                | Instruction::BraLong
                | Instruction::BraRn { .. }
                | Instruction::Bcc { .. }
                | Instruction::BccLong { .. }
                | Instruction::BccRn { .. }
                | Instruction::Bsr { .. }
                | Instruction::BsrLong
                | Instruction::BsrRn { .. }
                | Instruction::Bscc { .. }
                | Instruction::BsccLong { .. }
                | Instruction::BsccRn { .. }
                | Instruction::Jsr { .. }
                | Instruction::JsrEa { .. }
                | Instruction::Jscc { .. }
                | Instruction::JsccEa { .. }
                | Instruction::DoImm { .. }
                | Instruction::DoReg { .. }
                | Instruction::DoAa { .. }
                | Instruction::DoEa { .. }
                | Instruction::DoForever
                | Instruction::DorImm { .. }
                | Instruction::DorReg { .. }
                | Instruction::DorAa { .. }
                | Instruction::DorEa { .. }
                | Instruction::DorForever
                | Instruction::LraRn { .. }
                | Instruction::LraDisp { .. }
                | Instruction::BrclrEa { .. }
                | Instruction::BrclrAa { .. }
                | Instruction::BrclrPp { .. }
                | Instruction::BrclrQq { .. }
                | Instruction::BrclrReg { .. }
                | Instruction::BrsetEa { .. }
                | Instruction::BrsetAa { .. }
                | Instruction::BrsetPp { .. }
                | Instruction::BrsetQq { .. }
                | Instruction::BrsetReg { .. }
                | Instruction::BsclrEa { .. }
                | Instruction::BsclrAa { .. }
                | Instruction::BsclrPp { .. }
                | Instruction::BsclrQq { .. }
                | Instruction::BsclrReg { .. }
                | Instruction::BssetEa { .. }
                | Instruction::BssetAa { .. }
                | Instruction::BssetPp { .. }
                | Instruction::BssetQq { .. }
                | Instruction::BssetReg { .. }
                | Instruction::JsclrEa { .. }
                | Instruction::JsclrAa { .. }
                | Instruction::JsclrPp { .. }
                | Instruction::JsclrQq { .. }
                | Instruction::JsclrReg { .. }
                | Instruction::JssetEa { .. }
                | Instruction::JssetAa { .. }
                | Instruction::JssetPp { .. }
                | Instruction::JssetQq { .. }
                | Instruction::JssetReg { .. }
        )
    }

    /// Compile a single decoded instruction into a callable function.
    pub fn compile_instruction(
        &mut self,
        inst: &Instruction,
        pc: u32,
        next_word: u32,
        map: &MemoryMap,
    ) -> CompiledFn {
        self.ctx
            .func
            .signature
            .params
            .push(AbiParam::new(self.ptr_ty));
        self.ctx
            .func
            .signature
            .returns
            .push(AbiParam::new(types::I32));

        {
            let builder = FunctionBuilder::new(&mut self.ctx.func, &mut self.func_ctx);
            let mut emitter = Emitter::new(builder, self.ptr_ty, map);
            emitter.set_loop_quantum(self.loop_quantum);
            // Single-instruction functions are cached by opcode and reused
            // at other addresses; fault shadow budgets are opcode
            // properties (no PC involved), so this is cache-safe.
            emitter.emit_instruction(inst, pc, next_word);
            let frontend_config = self.module.as_ref().unwrap().isa().frontend_config();
            emitter.finalize_and_return(frontend_config);
        }

        self.finalize_function(&format!("dsp_inst_{:04x}", pc))
    }

    /// Compile a basic block starting at `start_pc`.
    /// `stop_pc` is the address at which the block must end (LA+1 for DO
    /// loops, `u32::MAX` when no loop is active).
    /// Return a translation for `start_pc`, reusing one whose PRAM words
    /// still match rather than rebuilding it.
    ///
    /// A program that DMAs code overlays over its own P memory and re-enters
    /// them at the same addresses gets back the words already translated -
    /// the dirty bits say "changed" because a write happened, not because
    /// the code differs. Keyed by content, those reloads cost a comparison
    /// of the block instead of a Cranelift compile, which for such a program
    /// is the difference between running the code and rebuilding it.
    ///
    /// Translations bake in the memory map's region pointers, so entries are
    /// only interchangeable within one engine's map - `invalidate_cache`,
    /// which is what a caller uses when the map changes underneath, drops them
    /// along with the code they point at.
    fn get_or_compile_block(
        &mut self,
        start_pc: u32,
        stop_pc: u32,
        generation: u32,
        map: &MemoryMap,
    ) -> CompiledBlock {
        if let Some(candidates) = self.translations.get(&(start_pc, stop_pc)) {
            for (words, func, ends_open) in candidates {
                if pram_matches(map, start_pc, words) {
                    self.stats.cache_hits += 1;
                    return CompiledBlock {
                        func: *func,
                        end_pc: start_pc + words.len() as u32,
                        generation,
                        ends_open: *ends_open,
                    };
                }
            }
        }

        let t0 = std::time::Instant::now();
        let block = self.compile_block(start_pc, stop_pc, generation, map);
        let dt = t0.elapsed().as_nanos() as u64;
        self.stats.compiles += 1;
        self.stats.compile_ns += dt;
        self.stats.compile_ns_worst = self.stats.compile_ns_worst.max(dt);

        if self.translation_count >= MAX_TRANSLATIONS {
            self.translations.clear();
            self.translation_count = 0;
        }
        self.translations
            .entry((start_pc, stop_pc))
            .or_default()
            .push((
                block_words(map, start_pc, block.end_pc),
                block.func,
                block.ends_open,
            ));
        self.translation_count += 1;
        self.stats.retained = self.translation_count as u64;
        block
    }

    fn compile_block(
        &mut self,
        start_pc: u32,
        stop_pc: u32,
        generation: u32,
        map: &MemoryMap,
    ) -> CompiledBlock {
        self.ctx
            .func
            .signature
            .params
            .push(AbiParam::new(self.ptr_ty));
        self.ctx
            .func
            .signature
            .returns
            .push(AbiParam::new(types::I32));

        let end_pc;
        let ends_open;
        let t_emit = std::time::Instant::now();
        {
            let builder = FunctionBuilder::new(&mut self.ctx.func, &mut self.func_ctx);
            let mut emitter = Emitter::new(builder, self.ptr_ty, map);
            emitter.set_loop_quantum(self.loop_quantum);
            (end_pc, ends_open) = emitter.emit_block(start_pc, self.max_block_len, stop_pc);
            let frontend_config = self.module.as_ref().unwrap().isa().frontend_config();
            emitter.finalize_and_return(frontend_config);
        }
        self.stats.emit_ns += t_emit.elapsed().as_nanos() as u64;

        let label = format!("dsp_block_{:04x}_{:04x}", start_pc, end_pc);
        let func = self.finalize_function(&label);
        CompiledBlock {
            func,
            end_pc,
            generation,
            ends_open,
        }
    }

    /// Finalize the current Cranelift function and return a callable pointer.
    fn finalize_function(&mut self, _label: &str) -> CompiledFn {
        let module = self.module.as_mut().unwrap();
        let func_id = module
            .declare_anonymous_function(&self.ctx.func.signature)
            .unwrap();
        let t_codegen = std::time::Instant::now();
        module.define_function(func_id, &mut self.ctx).unwrap();
        let code_size = self.ctx.compiled_code().unwrap().code_buffer().len();
        module.clear_context(&mut self.ctx);
        let t_finalize = std::time::Instant::now();
        module.finalize_definitions().unwrap();
        let t_end = std::time::Instant::now();
        self.stats.codegen_ns += (t_finalize - t_codegen).as_nanos() as u64;
        self.stats.finalize_ns += (t_end - t_finalize).as_nanos() as u64;
        self.stats.code_bytes += code_size as u64;

        let code_ptr = module.get_finalized_function(func_id);

        #[cfg(target_os = "linux")]
        if let Some(f) = &mut self.perf_map {
            let _ = writeln!(f, "{:x} {:x} {}", code_ptr as usize, code_size, _label);
        }

        // Safety: code_ptr points to JIT-compiled code with signature fn(*mut DspState) -> i32
        unsafe { std::mem::transmute::<*const u8, CompiledFn>(code_ptr) }
    }
}

impl Drop for JitEngine {
    fn drop(&mut self) {
        // JITModule leaks mmap'd code on drop; explicitly free it.
        if let Some(module) = self.module.take() {
            unsafe { module.free_memory() };
        }
    }
}

impl Default for JitEngine {
    fn default() -> Self {
        Self::new(0)
    }
}

impl DspState {
    /// Execute exactly one instruction at the current PC.
    /// Returns cycles consumed. Does not touch cycle_budget.
    pub fn execute_one(&mut self, jit: &mut JitEngine) -> i32 {
        self.step_one(jit)
    }

    /// Inner instruction execution: compile/lookup, execute, advance PC,
    /// service interrupts, tick cycle counter. Returns cycles consumed.
    fn step_one(&mut self, jit: &mut JitEngine) -> i32 {
        let opcode = self.map.read_pram(self.pc);
        let next_word = self.map.read_pram(mask_pc(self.pc + 1));

        let (func, inst_len) =
            jit.get_or_compile_instruction(self.pc, opcode, next_word, &self.map);

        // Armed core fault: instructions execute while the remaining
        // stream-word budget lasts (branches followed - a jmp in the
        // window executes and the stream continues at its target); the
        // first instruction that would exceed the budget is annulled and
        // becomes the frame's saved PC (silicon-probed).
        if self.interrupts.state == InterruptState::Armed && !self.loop_rep {
            // REP locks the fetch stream, so its iterations consume no
            // shadow words and delivery never lands inside the REP
            // complex. The complex itself charges two stream words - the
            // REP and its (necessarily one-word) target, fetched once
            // whatever the count, zero included - and a complex that does
            // not fit the remaining window is annulled at the REP. The
            // retire side (window truncated to at most one more word)
            // lives in `rep_truncate_armed_window`, with the silicon
            // observations behind both.
            let is_rep = matches!(
                decode::decode(opcode),
                Instruction::RepImm { .. }
                    | Instruction::RepReg { .. }
                    | Instruction::RepAa { .. }
                    | Instruction::RepEa { .. }
            );
            let cost = if is_rep { 2 } else { inst_len };
            let remaining = self.interrupts.fault_budget;
            if inst_len == 0 || cost > remaining {
                self.deliver_armed_fault();
                // Do NOT run process_pending_interrupts here: delivery
                // already did the stage-4 work (saved PC, vector fetch,
                // long detection) and set stage 3. An extra pipeline
                // tick before the first vector word executes desyncs the
                // fast-vector case: stage 2's saved-PC restore then
                // checks at vector+1 instead of vector+2 and never
                // fires, so execution falls off the end of the vector
                // (silicon: a fast-vectored fault executes exactly the
                // two vector words and resumes at the annulled address,
                // probe_se_fastvec).
                self.cycle_count += 2;
                return 2;
            }
            self.interrupts.fault_budget = remaining - cost;
        }

        self.pc_advance = inst_len;

        let consumed = unsafe { func(self as *mut DspState) };

        self.advance_pc();
        self.process_pending_interrupts();
        self.cycle_count += consumed as u32;

        consumed
    }

    /// Run the DSP for the given number of cycles using basic block JIT.
    ///
    /// Compiles and caches basic blocks, executing them until the cycle
    /// budget is exhausted or the DSP enters idle state.
    ///
    /// REP and inlineable DO/DOR loops are compiled as inline Cranelift loops
    /// inside blocks. Non-inlineable DO loops use block-boundary compilation:
    /// blocks end at LA+1 and the run loop handles loop-back/exit.
    pub fn run(&mut self, jit: &mut JitEngine, cycles: i32) {
        self.cycle_budget += cycles;

        while self.cycle_budget > 0 && !self.halt_requested {
            // One read at the top of every iteration when profiling: the
            // pre/step phases start here, so the loop's own checks are
            // inside a timed region rather than a residual.
            let profiling = jit.block_profile.is_some();
            let t_top = if profiling { read_ticks() } else { 0 };

            // STOP: all clocks halted, nothing happens until external RESET.
            if self.power_state == PowerState::Stop {
                self.cycle_budget = 0;
                break;
            }

            // WAIT: core halted, peripherals running. An unmasked interrupt
            // wakes the core. We check for pending interrupts: if one can
            // fire, transition back to Normal and resume execution.
            if self.power_state == PowerState::Wait {
                if self.interrupts.has_pending() {
                    let ipl_sr = ((self.registers[reg::SR] >> sr::I0) & 0x3) as i8;
                    let can_wake = (0..interrupt::COUNT).any(|i| {
                        self.interrupts.pending(i)
                            && (self.interrupts.ipl[i] == 3 || self.interrupts.ipl[i] >= ipl_sr)
                    });
                    if can_wake {
                        self.power_state = PowerState::Normal;
                    } else {
                        self.cycle_budget = 0;
                        break;
                    }
                } else {
                    self.cycle_budget = 0;
                    break;
                }
            }

            // During interrupt pipeline processing, fall back to single-step
            // so pipeline stages advance per-instruction.
            if self.interrupts.state != InterruptState::None {
                let consumed = self.step_one(jit);
                self.cycle_budget -= consumed;
                if let Some(ref mut profile) = jit.block_profile {
                    profile.dispatch.step_count += 1;
                    profile.dispatch.step_ticks += read_ticks().wrapping_sub(t_top);
                }
                continue;
            }

            // A live legacy REP context is step-path state: blocks neither
            // consult nor advance loop_rep, so dispatching one here would
            // execute the repeated instruction exactly once and leave the
            // context stuck. Reached when a fault delivered mid-REP (the
            // Armed stepping above ends with loop_rep still set) or when a
            // block ended on the legacy fallback for an uninlineable REP
            // target; step until the REP context retires.
            if self.loop_rep {
                let consumed = self.step_one(jit);
                self.cycle_budget -= consumed;
                if let Some(ref mut profile) = jit.block_profile {
                    profile.dispatch.step_count += 1;
                    profile.dispatch.step_ticks += read_ticks().wrapping_sub(t_top);
                }
                continue;
            }

            let pc = self.pc;

            // PC outside the configured PRAM: fall back to single-step.
            if pc as usize >= jit.pram_size {
                let consumed = self.step_one(jit);
                self.cycle_budget -= consumed;
                if let Some(ref mut profile) = jit.block_profile {
                    profile.dispatch.step_count += 1;
                    profile.dispatch.step_ticks += read_ticks().wrapping_sub(t_top);
                }
                continue;
            }

            let stop_pc = if (self.registers[reg::SR] & (1 << sr::LF)) != 0 {
                mask_pc(self.registers[reg::LA] + 1)
            } else {
                u32::MAX
            };

            // Evict if dirty or if a DO loop boundary falls within the block.
            // When stop_pc <= pc, the loop boundary is behind us (e.g. subroutine
            // called from inside a DO loop) and doesn't affect this block.
            if let Some(block) = &mut jit.cache.blocks[pc as usize] {
                let needs_evict = (stop_pc > pc && stop_pc < block.end_pc)
                    || (block.generation != self.pram_dirty.generation
                        && self.pram_dirty.is_range_dirty(pc, block.end_pc));
                if needs_evict {
                    jit.cache.blocks[pc as usize] = None;
                } else {
                    block.generation = self.pram_dirty.generation;
                }
            }

            // Time the compile path apart from the steady-state lookup: a
            // program that pages overlays rebuilds tens of thousands of
            // cache entries a second where a resident one rebuilds none,
            // and folding that into `pre` would blur the asymmetry this
            // split exists to weigh.
            let mut compile_ticks_here = 0u64;
            if jit.cache.blocks[pc as usize].is_none() {
                let tc0 = if profiling { read_ticks() } else { 0 };
                let block =
                    jit.get_or_compile_block(pc, stop_pc, self.pram_dirty.generation, &self.map);
                // The dirty bits are shared by every block covering these
                // words, so they can only be cleared once no cached block
                // still needs them. Blocks overlapping the range are exactly
                // the ones that may hold stale code, so drop them here;
                // otherwise a block nested inside this one would find the
                // bits already cleared, judge itself clean, and keep running
                // code the write replaced (an overlay load rewrites a region
                // under a dozen cached entry points at once).
                if self.pram_dirty.is_range_dirty(pc, block.end_pc) {
                    jit.stats.invalidations += 1;
                    jit.cache
                        .invalidate_range(pc, block.end_pc.saturating_sub(1));
                    self.pram_dirty.clear_dirty_range(pc, block.end_pc);
                }
                jit.cache.insert(pc, block);
                if let Some(ref mut profile) = jit.block_profile {
                    compile_ticks_here = read_ticks().wrapping_sub(tc0);
                    profile.dispatch.compile_count += 1;
                    profile.dispatch.compile_ticks += compile_ticks_here;
                }
            }

            let block = jit.cache.blocks[pc as usize].unwrap();
            jit.stats.block_entries += 1;
            // Order matters: the REP and inlined-DO paths break at stop_pc
            // without marking a terminator, so the boundary test has to run
            // first or those land in the cap's bucket.
            if block.end_pc == stop_pc {
                jit.stats.block_ends_do_boundary += 1;
            } else if block.ends_open {
                jit.stats.block_ends_open += 1;
            }
            // Bracket the call and nothing else: what the dispatch around
            // it costs is timed on its own in `dispatch`, and folding it
            // in here would hide it.
            let t0 = if profiling { read_ticks() } else { 0 };
            let consumed = unsafe { (block.func)(self as *mut DspState) };
            let t1 = if profiling { read_ticks() } else { 0 };
            self.exit_requested = false;

            if let Some(ref mut profile) = jit.block_profile {
                let stat = &mut profile.stats[pc as usize];
                stat.hits += 1;
                stat.cycles += consumed as u64;
                stat.words += (block.end_pc - pc) as u64;
                stat.ticks += t1.wrapping_sub(t0);
                profile.dispatch.iters += 1;
                profile.dispatch.pre_ticks +=
                    t0.wrapping_sub(t_top).saturating_sub(compile_ticks_here);
            }

            self.cycle_count += consumed as u32;
            self.cycle_budget -= consumed;

            // Sticky unimplemented-feature warnings: the single-step path
            // checks in advance_pc, but compiled blocks bypass it, so a
            // mode bit set inside a block would otherwise go unnoticed.
            self.check_unimplemented_modes();

            // Only sequential fall-through from LA triggers the loop-back
            // (pc_advance != 0); a branch landing on LA+1 does not. Matches
            // hardware BRKcc, which jumps to LA+1 leaving the loop state live.
            if (self.registers[reg::SR] & (1 << sr::LF)) != 0
                && self.pc_advance != 0
                && self.pc == mask_pc(self.registers[reg::LA] + 1)
            {
                self.registers[reg::LC] =
                    self.registers[reg::LC].wrapping_sub(1) & REG_MASKS[reg::LC];
                if self.registers[reg::LC] == 0 && (self.registers[reg::SR] & (1 << sr::FV)) == 0 {
                    let (_saved_pc, saved_sr) = self.stack_pop();
                    let lf_fv_mask = (1 << sr::LF) | (1 << sr::FV);
                    self.registers[reg::SR] =
                        (self.registers[reg::SR] & !lf_fv_mask) | (saved_sr & lf_fv_mask);
                    let (la, lc) = self.stack_pop();
                    self.registers[reg::LA] = la;
                    self.registers[reg::LC] = lc;
                } else {
                    self.pc = self.registers[reg::SSH];
                }
            }

            self.process_pending_interrupts();

            if let Some(ref mut profile) = jit.block_profile {
                profile.dispatch.post_ticks += read_ticks().wrapping_sub(t1);
            }
        }
    }
}

#[cfg(test)]
#[allow(unused_assignments)] // pram writes are read through raw pointers in DspState
mod tests {
    use super::*;
    use crate::core::{MemoryMap, reg};

    const PRAM_SIZE: usize = 4096;
    const XRAM_SIZE: usize = 4096;
    const YRAM_SIZE: usize = 2048;

    fn run_one(state: &mut DspState, jit: &mut JitEngine) -> i32 {
        state.execute_one(jit)
    }

    #[test]
    fn test_block_cache_hit() {
        // Run the same block twice to verify block cache works.
        let mut jit = JitEngine::new(PRAM_SIZE);
        let mut xram = [0u32; XRAM_SIZE];
        let mut yram = [0u32; YRAM_SIZE];
        let mut pram = [0u32; PRAM_SIZE];
        let mut s = DspState::new(MemoryMap::test(&mut xram, &mut yram, &mut pram));
        pram[0] = 0x000008; // inc A (1 cycle)
        pram[1] = 0x0C0000; // jmp $0 (3 cycles)
        // Block = [inc, jmp] = 4 cycles. Budget 9: 3 blocks.
        s.run(&mut jit, 9);
        assert!(jit.cache.blocks[0].is_some());
        assert_eq!(s.cycle_count, 12); // 3 blocks x 4 cycles
        assert_eq!(s.registers[reg::A0], 3);
    }

    #[test]
    fn test_block_path_checks_unimplemented_modes() {
        // The block run loop must notice unimplemented mode bits (sticky
        // warnings) even though it bypasses advance_pc.
        let mut jit = JitEngine::new(PRAM_SIZE);
        let mut xram = [0u32; XRAM_SIZE];
        let mut yram = [0u32; YRAM_SIZE];
        let mut pram = [0u32; PRAM_SIZE];
        let mut s = DspState::new(MemoryMap::test(&mut xram, &mut yram, &mut pram));
        pram[0] = 0x000008; // inc A
        pram[1] = 0x0C0000; // jmp $0
        s.registers[reg::SR] |= 1 << 13; // SC (16-bit compatibility mode)
        assert_eq!(s.warned_bits, 0);
        s.run(&mut jit, 4); // one block, no interrupt/single-step fallback
        assert_ne!(s.warned_bits, 0, "block path missed the mode check");
    }

    #[test]
    fn test_invalidate_cache() {
        let mut jit = JitEngine::new(PRAM_SIZE);
        let mut xram = [0u32; XRAM_SIZE];
        let mut yram = [0u32; YRAM_SIZE];
        let mut pram = [0u32; PRAM_SIZE];
        let mut s = DspState::new(MemoryMap::test(&mut xram, &mut yram, &mut pram));
        pram[0] = 0x000000; // nop
        pram[1] = 0x000000;
        run_one(&mut s, &mut jit);
        assert!(!jit.instr_cache.is_empty());
        jit.invalidate_cache();
        assert!(jit.cache.blocks.iter().all(|b| b.is_none()));
        assert!(jit.instr_cache.is_empty());
    }

    #[test]
    fn test_invalidate_range() {
        let mut jit = JitEngine::new(PRAM_SIZE);
        let mut xram = [0u32; XRAM_SIZE];
        let mut yram = [0u32; YRAM_SIZE];
        let mut pram = [0u32; PRAM_SIZE];
        let mut s = DspState::new(MemoryMap::test(&mut xram, &mut yram, &mut pram));
        // Use JSCLR (PC-dependent: embeds pc+2 as return address) at two PCs.
        // jsclr #0,X0,$100: opcode=0x0BC400, next_word=0x000100
        pram[0x00] = 0x0BC400;
        pram[0x01] = 0x000100;
        pram[0x10] = 0x0BC400;
        pram[0x11] = 0x000100;
        s.registers[reg::X0] = 0x000001; // bit 0 set -> not taken
        s.pc = 0;
        run_one(&mut s, &mut jit); // compiles jsclr at pc=0
        s.pc = 0x10;
        run_one(&mut s, &mut jit); // compiles jsclr at pc=0x10

        // Both should be cached with their actual PCs (PC-dependent)
        assert!(jit.instr_cache.keys().any(|&(pc, _, _)| pc == 0x00));
        assert!(jit.instr_cache.keys().any(|&(pc, _, _)| pc == 0x10));

        // Invalidate range [0, 1] - should only affect instruction at PC=0
        jit.invalidate_range(0, 1);
        assert!(!jit.instr_cache.keys().any(|&(pc, _, _)| pc == 0x00));
        assert!(jit.instr_cache.keys().any(|&(pc, _, _)| pc == 0x10));
    }

    #[test]
    fn test_invalidate_range_hits_block_starting_before_lo() {
        // The range scan starts max_span-1 entries before lo; a wide block
        // whose extent reaches into [lo, hi] must still be evicted even
        // though its start PC is below the invalidated range.
        let mut jit = JitEngine::new(PRAM_SIZE);
        let mut xram = [0u32; XRAM_SIZE];
        let mut yram = [0u32; YRAM_SIZE];
        let mut pram = [0u32; PRAM_SIZE];
        let mut s = DspState::new(MemoryMap::test(&mut xram, &mut yram, &mut pram));

        // Straight-line block $00..$08 ending in a self-loop at $08.
        for w in pram.iter_mut().take(8) {
            *w = 0x000008; // INC A
        }
        pram[8] = 0x0C0008; // JMP $8

        s.run(&mut jit, 12);
        let end = jit.cache.blocks[0].unwrap().end_pc;
        assert!(end > 4, "expected a multi-word block, got end_pc {end}");
        assert!(jit.cache.max_span >= end);

        // Invalidate a range the block only reaches into.
        jit.cache.invalidate_range(4, 4);
        assert!(jit.cache.blocks[0].is_none());
    }

    #[test]
    fn test_invalidate_range_block_cache() {
        // Compile blocks via run(), then invalidate a range and verify
        // only overlapping blocks are evicted.
        let mut jit = JitEngine::new(PRAM_SIZE);
        let mut xram = [0u32; XRAM_SIZE];
        let mut yram = [0u32; YRAM_SIZE];
        let mut pram = [0u32; PRAM_SIZE];
        let mut s = DspState::new(MemoryMap::test(&mut xram, &mut yram, &mut pram));

        // Chain: JMP $10 -> JMP $20 -> JMP $30 -> JMP-to-self
        // Each JMP creates a block at its PC.
        pram[0x00] = 0x0C0010; // JMP $10
        pram[0x10] = 0x0C0020; // JMP $20
        pram[0x20] = 0x0C0030; // JMP $30
        pram[0x30] = 0x0C0030; // JMP $30 (loop here, budget exhausts)

        s.run(&mut jit, 20);

        assert!(jit.cache.blocks[0x00].is_some());
        assert!(jit.cache.blocks[0x10].is_some());
        assert!(jit.cache.blocks[0x20].is_some());
        assert!(jit.cache.blocks[0x30].is_some());

        // Invalidate range [0x08, 0x15]: should evict blocks at 0x00 and 0x10.
        // Block at 0x00 (end_pc=0x01): pc=0 <= hi=0x15 and end_pc=1 > lo=0x08? No,
        // end_pc=1 is NOT > 0x08. So block at 0x00 should survive.
        // Block at 0x10 (end_pc=0x11): pc=0x10 <= 0x15 and end_pc=0x11 > 0x08. Evicted.
        // Block at 0x20: pc=0x20 > hi=0x15. Survives.
        // Block at 0x30: pc=0x30 > hi=0x15. Survives.
        jit.invalidate_range(0x08, 0x15);

        assert!(jit.cache.blocks[0x00].is_some());
        assert!(jit.cache.blocks[0x10].is_none());
        assert!(jit.cache.blocks[0x20].is_some());
        assert!(jit.cache.blocks[0x30].is_some());
    }

    #[test]
    fn test_dirty_bit_eviction() {
        // Compile a block, mark its PRAM dirty, then run again. The run loop
        // should detect the stale block and recompile it.
        let mut jit = JitEngine::new(PRAM_SIZE);
        let mut xram = [0u32; XRAM_SIZE];
        let mut yram = [0u32; YRAM_SIZE];
        let mut pram = [0u32; PRAM_SIZE];
        let mut s = DspState::new(MemoryMap::test(&mut xram, &mut yram, &mut pram));

        // First: INC A, JMP $0 -> A0 increments each iteration.
        pram[0] = 0x000008; // INC A
        pram[1] = 0x0C0000; // JMP $0

        // Run for a few cycles to compile and cache the block.
        s.run(&mut jit, 6);
        let a0_first = s.registers[reg::A0];
        assert!(a0_first > 0);
        assert!(jit.cache.blocks[0].is_some());
        let gen_before = jit.cache.blocks[0].unwrap().generation;

        // Overwrite PRAM and mark dirty. Replace INC with DEC.
        pram[0] = 0x00000A; // DEC A
        s.pram_dirty.mark_dirty(0);

        // Run again. The run loop should detect the dirty block and recompile.
        s.registers[reg::A0] = 10;
        s.registers[reg::A1] = 0;
        s.registers[reg::A2] = 0;
        s.pc = 0;
        s.cycle_count = 0;
        s.run(&mut jit, 6);

        // Block was recompiled with new generation.
        assert!(jit.cache.blocks[0].is_some());
        assert_ne!(jit.cache.blocks[0].unwrap().generation, gen_before);
        // DEC should have decremented A0 from 10.
        assert!(s.registers[reg::A0] < 10);
    }

    #[test]
    fn test_dirty_bit_eviction_overlapping_blocks() {
        // Two cached blocks can cover overlapping PRAM. Recompiling the
        // outer one must not launder the inner one: the dirty bits are the
        // only record that the words changed, and they are shared.
        let mut jit = JitEngine::new(PRAM_SIZE);
        let mut xram = [0u32; XRAM_SIZE];
        let mut yram = [0u32; YRAM_SIZE];
        let mut pram = [0u32; PRAM_SIZE];
        let mut s = DspState::new(MemoryMap::test(&mut xram, &mut yram, &mut pram));

        pram[0x00] = 0x000000; // NOP
        pram[0x01] = 0x000000; // NOP
        pram[0x02] = 0x000008; // INC A
        pram[0x03] = 0x0C0010; // JMP $10
        pram[0x10] = 0x0C0010; // JMP $10 (park)

        // Compile the outer block [$00,$04) and the inner one [$02,$04).
        s.pc = 0x00;
        s.run(&mut jit, 8);
        s.pc = 0x02;
        s.run(&mut jit, 8);
        assert!(jit.cache.blocks[0x00].is_some());
        assert!(jit.cache.blocks[0x02].is_some());

        // Rewrite the word both blocks contain (an overlay load does this).
        pram[0x02] = 0x00000A; // DEC A
        s.pram_dirty.mark_dirty(0x02);

        // Entering the outer block recompiles it and clears the dirty range.
        s.pc = 0x00;
        s.run(&mut jit, 8);

        // Entering the inner block must run DEC, not the cached INC.
        s.registers[reg::A0] = 10;
        s.registers[reg::A1] = 0;
        s.registers[reg::A2] = 0;
        s.pc = 0x02;
        s.run(&mut jit, 8);
        assert!(
            s.registers[reg::A0] < 10,
            "block at $02 executed stale code after $00 was recompiled"
        );
    }

    /// How much of a DSP cycle's cost is the run loop rather than the code it
    /// dispatches. Same instruction, same total cycles, two block lengths:
    /// the difference is the per-block overhead.
    ///
    /// `cap` is the emitter's instruction cap, swept independently of the
    /// program's shape. Sweeping only `block_len` cannot measure the cap:
    /// with the cap at 32 a 40- and a 120-instruction body both compile to
    /// 32-instruction blocks, and what varies between them is how much code
    /// the host is cycling through, not how long a block is.
    ///
    /// Run with: cargo test --release -p dsp56300-emu --lib bench_block -- --nocapture --ignored
    #[test]
    #[ignore]
    fn bench_block_dispatch_overhead() {
        fn time_cycles(block_len: usize, cap: u32, total_cycles: u32) -> (f64, f64, f64) {
            let mut jit = JitEngine::new(PRAM_SIZE);
            jit.max_block_len = cap;
            let mut xram = [0u32; XRAM_SIZE];
            let mut yram = [0u32; YRAM_SIZE];
            let mut pram = [0u32; PRAM_SIZE];
            // A ring of blocks, each `block_len` INC A instructions ended by a
            // JMP to the next; the last jumps back to the first.
            let nblocks = 8;
            let stride = block_len + 2; // body + 2-word JMP
            for b in 0..nblocks {
                let start = b * stride;
                for i in 0..block_len {
                    pram[start + i] = 0x000008; // INC A
                }
                let next = if b + 1 == nblocks {
                    0
                } else {
                    (b + 1) * stride
                };
                pram[start + block_len] = 0x0AF080; // JMP >
                pram[start + block_len + 1] = next as u32;
            }
            let mut s = DspState::new(MemoryMap::test(&mut xram, &mut yram, &mut pram));
            s.pc = 0;
            s.run(&mut jit, 20000); // warm the cache
            let before = s.cycle_count;
            let blocks_before = jit.stats.block_entries;
            let t0 = std::time::Instant::now();
            s.run(&mut jit, total_cycles as i32);
            let ns = t0.elapsed().as_nanos() as f64;
            let cycles = (s.cycle_count - before) as f64;
            let blocks = (jit.stats.block_entries - blocks_before) as f64;
            (ns / cycles, ns / blocks, cycles / blocks)
        }

        let total = 20_000_000;
        println!("-- program shape held at a 120-instruction body, cap swept --");
        for cap in [4u32, 8, 16, 24, 32, 40, 48, 64, 96, 120] {
            let (ns_cyc, ns_blk, cyc_blk) = time_cycles(120, cap, total);
            println!(
                "cap {:3}: {:5.2} ns/cyc, {:6.1} ns/block, {:5.1} cyc/block",
                cap, ns_cyc, ns_blk, cyc_blk
            );
        }
        println!("-- cap held at 128 (never binds), body swept --");
        for len in [4usize, 14, 40, 120] {
            let (ns_cyc, ns_blk, cyc_blk) = time_cycles(len, 128, total);
            println!(
                "body {:3} instr: {:5.2} ns/cyc, {:6.1} ns/block, {:5.1} cyc/block",
                len, ns_cyc, ns_blk, cyc_blk
            );
        }
    }

    #[test]
    fn test_translation_cache_reuses_reloaded_overlays() {
        // An overlay swapped out and back produces the same words at the same
        // PC, so the second load must reuse the translation rather than
        // rebuild it - and a load that brings back *different* words must not.
        let mut jit = JitEngine::new(PRAM_SIZE);
        let mut xram = [0u32; XRAM_SIZE];
        let mut yram = [0u32; YRAM_SIZE];
        let mut pram = [0u32; PRAM_SIZE];
        let mut s = DspState::new(MemoryMap::test(&mut xram, &mut yram, &mut pram));

        let overlay_a = [0x000008u32, 0x0C0010]; // INC A; JMP $10
        let overlay_b = [0x00000Au32, 0x0C0010]; // DEC A; JMP $10
        pram[0x10] = 0x0C0010; // park

        let load = |s: &mut DspState, words: &[u32]| {
            for (i, &w) in words.iter().enumerate() {
                s.write_memory(crate::core::MemSpace::P, i as u32, w);
                s.pram_dirty.mark_dirty(i as u32);
            }
        };

        load(&mut s, &overlay_a);
        s.pc = 0;
        s.run(&mut jit, 8);
        let first = jit.stats;
        assert!(first.compiles > 0);
        assert_eq!(first.cache_hits, 0);

        // A different overlay over the same words: a fresh translation.
        load(&mut s, &overlay_b);
        s.pc = 0;
        s.run(&mut jit, 8);
        assert_eq!(jit.stats.compiles, first.compiles + 1);
        assert_eq!(jit.stats.cache_hits, 0);
        let second = jit.stats;

        // The first one back again: reused, and it must still run INC A.
        load(&mut s, &overlay_a);
        s.registers[reg::A0] = 10;
        s.registers[reg::A1] = 0;
        s.registers[reg::A2] = 0;
        s.pc = 0;
        s.run(&mut jit, 8);
        assert_eq!(
            jit.stats.compiles, second.compiles,
            "reloaded overlay was recompiled"
        );
        assert_eq!(jit.stats.cache_hits, 1);
        assert_eq!(s.registers[reg::A0], 11, "cache served the wrong overlay");
    }

    #[test]
    fn test_movem_rewrite_seen_by_every_entry_point() {
        // The program rewrites its own P memory with MOVEM and then re-enters
        // the rewritten words, which is the shape an overlay load takes.
        // Unlike the tests above it never touches pram_dirty by hand:
        // the write goes through the compiled block's memory helper, so this
        // covers marking and eviction together.
        //
        //         move  #>$000000,x0
        //         move  #L2,r0
        //         jsr   L2          ; caches a block at L2
        //         movem x0,p:(r0)+  ; L2's INC A becomes NOP
        //         jsr   L1          ; L1's block spans L2 - recompiles
        //         jsr   L2          ; must not run the cached INC A
        //         jmp   END
        // L1:     nop
        // L2:     inc a
        //         rts
        // END:    jmp END
        let mut jit = JitEngine::new(PRAM_SIZE);
        let mut xram = [0u32; XRAM_SIZE];
        let mut yram = [0u32; YRAM_SIZE];
        let mut pram = [0u32; PRAM_SIZE];
        let prog: [(usize, u32); 18] = [
            (0x20, 0x44F400),
            (0x21, 0x000000),
            (0x22, 0x60F400),
            (0x23, 0x00002E),
            (0x24, 0x0BF080),
            (0x25, 0x00002E),
            (0x26, 0x075884),
            (0x27, 0x0BF080),
            (0x28, 0x00002D),
            (0x29, 0x0BF080),
            (0x2A, 0x00002E),
            (0x2B, 0x0AF080),
            (0x2C, 0x000030),
            (0x2D, 0x000000),
            (0x2E, 0x000008),
            (0x2F, 0x00000C),
            (0x30, 0x0AF080),
            (0x31, 0x000030),
        ];
        for (addr, word) in prog {
            pram[addr] = word;
        }
        let mut s = DspState::new(MemoryMap::test(&mut xram, &mut yram, &mut pram));

        s.pc = 0x20;
        for _ in 0..16 {
            if s.pc == 0x30 {
                break;
            }
            s.run(&mut jit, 8);
        }
        assert_eq!(s.pc, 0x30, "program did not reach the parking loop");
        // INC A ran once, before the rewrite. A second increment means the
        // block cached at L2 kept running the word the MOVEM replaced.
        assert_eq!(
            s.registers[reg::A0],
            1,
            "L2 executed stale code after the MOVEM rewrote it"
        );
    }

    #[test]
    fn test_profiling_enable_and_counters() {
        let mut jit = JitEngine::new(PRAM_SIZE);
        assert!(!jit.is_profiling());

        jit.enable_profiling();
        assert!(jit.is_profiling());

        // Run some instructions to accumulate profile data.
        let mut xram = [0u32; XRAM_SIZE];
        let mut yram = [0u32; YRAM_SIZE];
        let mut pram = [0u32; PRAM_SIZE];
        let mut s = DspState::new(MemoryMap::test(&mut xram, &mut yram, &mut pram));
        pram[0] = 0x000000; // NOP
        pram[1] = 0x0C0000; // JMP $0

        s.run(&mut jit, 10);

        // Profile should have recorded hits at PC 0.
        let profile = jit.block_profile.as_ref().unwrap();
        let s0 = profile.stats[0];
        assert!(s0.hits > 0, "expected hits > 0");
        assert!(s0.cycles > 0, "expected cycles > 0");
        // [NOP, JMP] is two words, every dispatch.
        assert_eq!(s0.words, s0.hits * 2, "expected 2 words/dispatch");
        // Host time is recorded where there is a counter to read.
        if profile.ns_per_tick > 0.0 {
            assert!(s0.ticks > 0, "expected host ticks > 0");
        }
    }

    #[test]
    fn test_profile_words_survive_recompilation() {
        // The extent is recorded per dispatch, so a PC that later hosts a
        // block of a different length still reports what actually ran.
        // Reading it back from the cache at dump time cannot do this:
        // overlay loads recompile hot PCs to different extents, and an
        // evicted PC has no entry to read at all.
        let mut jit = JitEngine::new(PRAM_SIZE);
        jit.enable_profiling();
        let mut xram = [0u32; XRAM_SIZE];
        let mut yram = [0u32; YRAM_SIZE];
        let mut pram = [0u32; PRAM_SIZE];
        let mut s = DspState::new(MemoryMap::test(&mut xram, &mut yram, &mut pram));
        pram[0] = 0x000008; // inc A  (1 cycle)
        pram[1] = 0x000008; // inc A
        pram[2] = 0x0C0000; // jmp $0 (3 cycles)
        s.run(&mut jit, 5); // one dispatch of the 3-word block
        let s0 = jit.block_profile.as_ref().unwrap().stats[0];
        assert_eq!((s0.hits, s0.cycles, s0.words), (1, 5, 3));

        // Shorten the block at PC 0 and run it again.
        s.pc = 0;
        pram[1] = 0x0C0000; // jmp $0 at word 1
        jit.invalidate_cache();
        s.run(&mut jit, 4); // one dispatch of the 2-word block
        let s0 = jit.block_profile.as_ref().unwrap().stats[0];
        assert_eq!(s0.hits, 2);
        assert_eq!(s0.words, 5, "3-word dispatch + 2-word dispatch");
    }

    #[test]
    #[cfg(target_os = "linux")]
    fn test_perf_map() {
        let mut jit = JitEngine::new(PRAM_SIZE);
        jit.enable_perf_map();
        assert!(jit.perf_map.is_some());

        // Compile a block to trigger a perf map write.
        let mut xram = [0u32; XRAM_SIZE];
        let mut yram = [0u32; YRAM_SIZE];
        let mut pram = [0u32; PRAM_SIZE];
        let mut s = DspState::new(MemoryMap::test(&mut xram, &mut yram, &mut pram));
        pram[0] = 0x0C0000; // JMP $0
        run_one(&mut s, &mut jit);

        // Verify the perf map file was created.
        let path = format!("/tmp/perf-{}.map", std::process::id());
        assert!(std::path::Path::new(&path).exists());
        let contents = std::fs::read_to_string(&path).unwrap();
        assert!(!contents.is_empty());
        std::fs::remove_file(&path).ok();
    }

    #[test]
    fn test_jit_engine_default() {
        let jit = JitEngine::default();
        assert!(!jit.is_profiling());
        #[cfg(target_os = "linux")]
        assert!(jit.perf_map.is_none());
    }
}

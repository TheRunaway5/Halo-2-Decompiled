//! DSP56300 core state: registers, memory, and constants.

use std::ffi::c_void;

// Re-export architectural constants from dsp56300-core
pub use dsp56300_core::{
    MemSpace, PC_MASK, PERIPH_BASE, PERIPH_SIZE, REG_MASKS, mask_pc, mask_reg, reg, sr,
};

/// What kind of access a memory region provides.
#[derive(Clone, Copy)]
pub enum RegionKind {
    /// Direct buffer access: `base[addr - start + offset]`.
    /// The caller owns the buffer; it must remain valid for the DspState lifetime.
    Buffer { base: *mut u32, offset: u32 },
    /// Callback-driven access (e.g. peripheral registers).
    ///
    /// Contract: a callback may read and write the embedder's own state
    /// and the memory buffers, but not the core's register file, stack
    /// or PC. Compiled code keeps those in host registers across the
    /// call and neither spills them before it nor reloads them after; a
    /// spill/reload pair around every dynamic access in a space with a
    /// callback region would be paid on the buffer path too. An embedder
    /// that needs the register file acts on it between `run` calls
    /// through the API instead.
    Callback {
        opaque: *mut c_void,
        read_fn: unsafe extern "C" fn(*mut c_void, u32) -> u32,
        write_fn: unsafe extern "C" fn(*mut c_void, u32, u32),
    },
}

// Safety: the raw pointers in RegionKind are only dereferenced during
// single-threaded DSP execution on the thread that owns the DspState.
unsafe impl Send for RegionKind {}
unsafe impl Sync for RegionKind {}

/// A contiguous region in DSP address space.
#[derive(Clone, Copy)]
pub struct MemoryRegion {
    /// First address in the region (inclusive).
    pub start: u32,
    /// One past the last address (exclusive).
    pub end: u32,
    /// How this region is accessed.
    pub kind: RegionKind,
}

/// Memory map describing the DSP address space.
///
/// Each of the three DSP address spaces (X, Y, P) has a list of regions
/// sorted by start address. The JIT emitter uses this at compile time to
/// generate inline loads for Buffer regions and indirect calls for Callback
/// regions.
#[derive(Clone, Default)]
pub struct MemoryMap {
    pub x_regions: Vec<MemoryRegion>,
    pub y_regions: Vec<MemoryRegion>,
    pub p_regions: Vec<MemoryRegion>,
}

impl MemoryMap {
    /// Return the region list for a given space.
    pub fn regions(&self, space: MemSpace) -> &[MemoryRegion] {
        match space {
            MemSpace::X => &self.x_regions,
            MemSpace::Y => &self.y_regions,
            MemSpace::P => &self.p_regions,
        }
    }

    /// Look up the region containing `addr` in the given space.
    pub fn lookup(&self, space: MemSpace, addr: u32) -> Option<&MemoryRegion> {
        self.regions(space)
            .iter()
            .find(|r| addr >= r.start && addr < r.end)
    }

    /// Read a 24-bit word from P-space at the given address.
    /// Handles both Buffer and Callback regions. Returns 0 if out of range.
    pub fn read_pram(&self, addr: u32) -> u32 {
        for region in &self.p_regions {
            if addr >= region.start && addr < region.end {
                let raw = match region.kind {
                    RegionKind::Buffer { base, offset } => {
                        let idx = (addr - region.start + offset) as usize;
                        unsafe { *base.add(idx) }
                    }
                    RegionKind::Callback {
                        opaque, read_fn, ..
                    } => unsafe { read_fn(opaque, addr) },
                };
                return raw & PC_MASK;
            }
        }
        0
    }

    /// Return the highest P-space address (exclusive) across all regions.
    pub fn p_space_end(&self) -> u32 {
        self.p_regions.iter().map(|r| r.end).max().unwrap_or(0)
    }

    /// Build a simple test map: X [0, xram.len()), Y [0, yram.len()), P [0, pram.len()).
    pub fn test(xram: &mut [u32], yram: &mut [u32], pram: &mut [u32]) -> Self {
        MemoryMap {
            x_regions: vec![MemoryRegion {
                start: 0,
                end: xram.len() as u32,
                kind: RegionKind::Buffer {
                    base: xram.as_mut_ptr(),
                    offset: 0,
                },
            }],
            y_regions: vec![MemoryRegion {
                start: 0,
                end: yram.len() as u32,
                kind: RegionKind::Buffer {
                    base: yram.as_mut_ptr(),
                    offset: 0,
                },
            }],
            p_regions: vec![MemoryRegion {
                start: 0,
                end: pram.len() as u32,
                kind: RegionKind::Buffer {
                    base: pram.as_mut_ptr(),
                    offset: 0,
                },
            }],
        }
    }
}

/// Initialization parameters for creating a new DSP state.
#[derive(Clone, Default)]
pub struct CreateInfo {
    pub memory_map: MemoryMap,
}

impl From<MemoryMap> for CreateInfo {
    fn from(memory_map: MemoryMap) -> Self {
        Self { memory_map }
    }
}

/// Power state (DSP56300FM section 8.4).
///
/// WAIT: halts the clock to the core but not peripherals. An unmasked
/// interrupt wakes the core. STOP: halts all clocks. Only an external
/// hardware RESET can restart.
#[repr(u8)]
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum PowerState {
    /// Normal operation.
    Normal = 0,
    /// WAIT instruction executed: core halted, peripherals running.
    /// Unmasked interrupt returns to Normal.
    Wait = 1,
    /// STOP instruction executed: all clocks halted.
    /// Only hardware RESET restarts.
    Stop = 2,
}

impl From<u8> for PowerState {
    fn from(v: u8) -> Self {
        match v {
            1 => Self::Wait,
            2 => Self::Stop,
            _ => Self::Normal,
        }
    }
}

/// Interrupt pipeline state (see DSP56300FM section 2.3.2).
#[repr(u8)]
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum InterruptState {
    /// No interrupt in progress (normal processing).
    None = 0,
    /// Fast interrupt: vector instructions being fetched/executed.
    Fast = 1,
    /// Long interrupt: JSR detected at vector, context stacked.
    Long = 2,
    /// Core fault armed with a known shadow word budget: instructions
    /// keep executing while the remaining budget lasts; the first
    /// instruction that would exceed it is annulled and becomes the
    /// exception frame's saved PC (silicon-probed).
    Armed = 3,
    /// A core fault that armed while another fault was in flight
    /// ("parked"): silicon defers its delivery on a completion
    /// countdown instead of dispatching immediately (probe_ill_in_shadow
    /// family). See `InterruptPipeline::parked_countdown`.
    Parked = 4,
}

/// Sentinel for "no fault shadow budget recorded".
///
/// Shadow budgets are counted in FETCH-STREAM WORDS remaining after the
/// faulting instruction, following branches: a branch inside the window
/// executes, consumes its word count, and the stream continues at its
/// target (silicon-probed, VBA-redirect probe rounds 1-5).
/// Per-class budgets, in words after the faulting instruction of length
/// `len` (equivalently: silicon delivers at start+len+6 for register
/// pops, start+len+7 for memory-destination pops, start+9 flat for
/// pushes, start+6 flat for SE-bit SP writes, start+3 for RTS/RTI):
/// - register-destination pops / in-place SSH writes: 6
/// - memory-destination pops: 7
/// - push overflow (incl. JSR-family; target words count): 9 - len
/// - SP writes that set the SE bit: 6 - len (UF-only writes don't fault;
///   bit-op SP writes behave exactly like movec forms - bset #4,sp
///   faults with this class, bset #5,sp (UF) doesn't, probe_sp_bset_se4)
/// - RTS/RTI underflow (the branch to slot-0 storage executes): 3 - len
/// - DO push overflow: 3 (silicon start+5 after the 2-word DO; the
///   overflowing push itself wraps and LANDS in slot 0, like JSR's)
/// - ENDDO pop underflow: 5 (silicon start+6 flat; the double pop takes
///   SP $00->$3F->$3E and the dispatch frame lands in slot 15)
/// - ILLEGAL / TRAP / TRAPcc: 0 (immediate: saved PC = F+len, so RTI
///   SKIPS the faulting instruction; vectors VBA:$04 / VBA:$08 per
///   Table 2-2; both honor the fast-vector shape - two plain vector
///   words execute with no frame push, resuming at F+len)
pub const INVALID_FAULT_BUDGET: u32 = 0xFFFF_FFFF;

impl From<u8> for InterruptState {
    fn from(v: u8) -> Self {
        match v {
            1 => Self::Fast,
            2 => Self::Long,
            3 => Self::Armed,
            4 => Self::Parked,
            _ => Self::None,
        }
    }
}

impl std::fmt::Display for InterruptState {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Self::None => f.write_str("none"),
            Self::Fast => f.write_str("fast"),
            Self::Long => f.write_str("long"),
            Self::Armed => f.write_str("armed"),
            Self::Parked => f.write_str("parked"),
        }
    }
}

/// Interrupt pipeline state machine (DSP56300FM section 2.3.2).
///
/// The DSP56300 supports 128 interrupt vectors in a 256-word IVT.
/// Each vector occupies 2 words at address `index * 2`.
#[derive(Clone)]
pub struct InterruptPipeline {
    pub state: InterruptState,
    pub pending_bits: [u64; 2],
    pub pipeline_stage: u8,
    pub vector_addr: u32,
    pub saved_pc: u32,
    pub ipl: [i8; interrupt::COUNT],
    pub ipl_to_raise: u8,
    /// Remaining shadow word budget of the pending core fault (stream
    /// words that may still execute before delivery; see
    /// `INVALID_FAULT_BUDGET` docs for per-class values). Enables the
    /// Armed shadow model; `INVALID_FAULT_BUDGET` when unknown.
    pub fault_budget: u32,
    /// Completion countdown for a Parked core fault (one that armed
    /// while another fault was in flight). Silicon delivers a parked
    /// fault 6 instruction completions after the prior fault's
    /// dispatch; a guest SP WRITE during the countdown defers
    /// delivery to at least 3 completions after the write, SP reads
    /// do not defer (probe_ill_in_shadow / _pad / _noclean /
    /// _noaccess / _earlywrite). The pipeline's stage-0
    /// completion arm runs on the 4th completion after delivery, so
    /// the hook there seeds this countdown with 3.
    pub parked_countdown: u32,
    /// Set by a guest SP write while state == Parked; consumed by the
    /// next countdown tick (the deferral counts from after the
    /// writing instruction's own completion).
    pub parked_sp_write: bool,
}

impl Default for InterruptPipeline {
    fn default() -> Self {
        Self::new()
    }
}

impl InterruptPipeline {
    pub fn new() -> Self {
        let mut ipl = [-1i8; interrupt::COUNT];
        // Core interrupts are IPL 3 (non-maskable) by default
        ipl[interrupt::RESET] = 3;
        ipl[interrupt::ILLEGAL] = 3;
        ipl[interrupt::STACK_ERROR] = 3;
        ipl[interrupt::TRAP] = 3;
        Self {
            state: InterruptState::None,
            pending_bits: [0; 2],
            pipeline_stage: 0,
            vector_addr: 0xFFFF,
            saved_pc: 0xFFFF,
            ipl,
            ipl_to_raise: 0,
            fault_budget: INVALID_FAULT_BUDGET,
            parked_countdown: 0,
            parked_sp_write: false,
        }
    }

    pub fn has_pending(&self) -> bool {
        self.pending_bits[0] != 0 || self.pending_bits[1] != 0
    }

    pub fn pending(&self, i: usize) -> bool {
        self.pending_bits[i / 64] & (1u64 << (i % 64)) != 0
    }

    pub fn set_pending(&mut self, i: usize) {
        self.pending_bits[i / 64] |= 1u64 << (i % 64);
    }

    pub fn clear_pending(&mut self, i: usize) {
        self.pending_bits[i / 64] &= !(1u64 << (i % 64));
    }

    /// Post a pending interrupt.
    pub fn add(&mut self, inter: usize) {
        if inter >= interrupt::COUNT || self.ipl[inter] == -1 {
            return;
        }
        self.set_pending(inter);
    }
}

/// Interrupt definitions (DSP56300FM Table 2-2).
///
/// The DSP56300 has 128 interrupt vectors. Each vector's address is `index * 2`.
/// The first 8 are core-defined; the rest are peripheral interrupt requests.
pub mod interrupt {
    /// Total number of interrupt sources.
    pub const COUNT: usize = 128;

    // Core interrupt indices = architectural IVT slot number.
    // Vector address = VBA + (index * 2), per Table 2-2.
    //
    // Note: The ILLEGAL instruction page (13-76) says "P:$3E" - this is a
    // DSP56000 holdover. The DSP56300 IVT is relocatable via VBA and Table 2-2
    // places ILLEGAL at VBA:$04.
    pub const RESET: usize = 0; // VBA:$00
    pub const STACK_ERROR: usize = 1; // VBA:$02
    pub const ILLEGAL: usize = 2; // VBA:$04
    pub const DEBUG: usize = 3; // VBA:$06
    pub const TRAP: usize = 4; // VBA:$08
    pub const NMI: usize = 5; // VBA:$0A

    /// Return the slot offset (low 8 bits of vector address) for interrupt index `i`.
    pub const fn vector_addr(i: usize) -> u16 {
        (i as u16) * 2
    }
}

/// Dirty bitmap for tracking PRAM writes that invalidate JIT blocks.
pub struct PramDirtyBitmap {
    /// Total number of tracked PRAM words.
    pram_size: usize,
    /// Dirty bitmap: one bit per PRAM word.
    /// Set when JIT code or DMA writes a DIFFERENT value to P-space.
    /// The run loop checks the block's range before execution.
    pub dirty: Vec<u64>,
    /// Generation counter. Bumped whenever a dirty bit is set.
    /// Cached blocks store the generation they were compiled at; if it matches,
    /// the dirty bitmap scan is skipped entirely.
    pub generation: u32,
}

impl PramDirtyBitmap {
    pub fn new(pram_size: usize) -> Self {
        Self {
            pram_size,
            dirty: vec![0u64; pram_size.div_ceil(64)],
            generation: 0,
        }
    }

    /// Mark a PRAM address as dirty and bump the generation counter.
    pub fn mark_dirty(&mut self, addr: u32) {
        let addr = addr as usize;
        if addr < self.pram_size {
            self.dirty[addr / 64] |= 1u64 << (addr % 64);
            self.generation = self.generation.wrapping_add(1);
        }
    }

    /// Check whether any dirty bit is set in the range [start_pc, end_pc).
    pub fn is_range_dirty(&self, start_pc: u32, end_pc: u32) -> bool {
        let lo = start_pc as usize;
        let hi = (end_pc as usize).min(self.pram_size);
        if lo >= hi {
            return false;
        }
        let first_word = lo / 64;
        let last_word = (hi - 1) / 64;
        if first_word == last_word {
            let mask = Self::range_mask(lo % 64, hi.wrapping_sub(first_word * 64).min(64));
            return self.dirty[first_word] & mask != 0;
        }
        // First partial word
        let first_mask = !0u64 << (lo % 64);
        if self.dirty[first_word] & first_mask != 0 {
            return true;
        }
        // Full words in the middle
        if self.dirty[(first_word + 1)..last_word]
            .iter()
            .any(|&w| w != 0)
        {
            return true;
        }
        // Last partial word
        let last_bits = hi - last_word * 64;
        let last_mask = if last_bits >= 64 {
            !0u64
        } else {
            (1u64 << last_bits) - 1
        };
        self.dirty[last_word] & last_mask != 0
    }

    /// Clear dirty bits for PRAM range [start_pc, end_pc).
    pub fn clear_dirty_range(&mut self, start_pc: u32, end_pc: u32) {
        let lo = start_pc as usize;
        let hi = (end_pc as usize).min(self.pram_size);
        if lo >= hi {
            return;
        }
        let first_word = lo / 64;
        let last_word = (hi - 1) / 64;
        if first_word == last_word {
            let mask = Self::range_mask(lo % 64, hi.wrapping_sub(first_word * 64).min(64));
            self.dirty[first_word] &= !mask;
            return;
        }
        self.dirty[first_word] &= !(!0u64 << (lo % 64));
        self.dirty[(first_word + 1)..last_word].fill(0);
        let last_bits = hi - last_word * 64;
        let last_mask = if last_bits >= 64 {
            !0u64
        } else {
            (1u64 << last_bits) - 1
        };
        self.dirty[last_word] &= !last_mask;
    }

    /// Bitmask for bits [lo_bit, hi_bit) within a single u64 word.
    pub fn range_mask(lo_bit: usize, hi_bit: usize) -> u64 {
        debug_assert!(lo_bit < 64 && hi_bit <= 64 && lo_bit < hi_bit);
        let top = if hi_bit >= 64 {
            !0u64
        } else {
            (1u64 << hi_bit) - 1
        };
        top & !((1u64 << lo_bit) - 1)
    }
}

/// DSP56300 state.
pub struct DspState {
    /// Program counter
    pub pc: u32,
    /// PC advance after instruction execution (word count added to PC)
    pub pc_advance: u32,
    /// Where a dynamic-address store lands when its address misses the
    /// space's RAM: the emitter selects between the RAM element and this
    /// word so the store itself is unconditional (`write_mem_dyn`). Never
    /// read.
    pub mem_write_sink: u32,
    /// Total cycle count
    pub cycle_count: u32,
    /// General registers (indexed by reg::* constants)
    pub registers: [u32; reg::COUNT],
    /// Hardware stack: stack\[0\] = SSH, stack\[1\] = SSL
    pub stack: [[u32; 16]; 2],

    /// True while inside a REP loop
    pub loop_rep: bool,
    /// True on the first iteration after REP (skip LC decrement)
    pub pc_on_rep: bool,

    pub interrupts: InterruptPipeline,

    /// Remaining cycle budget for `run()`
    pub cycle_budget: i32,
    /// External halt request (set by peripheral callback, checked by run loop)
    pub halt_requested: bool,
    /// Block-level exit request. Set when a condition requires the currently
    /// executing block to return to the run loop (e.g. halt_requested was
    /// set by a peripheral callback). Cleared by the run loop after each block.
    pub exit_requested: bool,
    /// Shadow word budget of the instruction currently executing, spilled
    /// by JIT code immediately before calls that can post a core fault
    /// (stack errors). Consumed as `InterruptPipeline::fault_budget` when
    /// a fault is posted. `INVALID_FAULT_BUDGET` when unknown (e.g.
    /// direct helper calls in tests).
    pub fault_budget_hint: u32,
    /// Power state (WAIT/STOP). Set by WAIT/STOP instructions, checked by
    /// the run loop. WAIT is cleared on unmasked interrupt; STOP requires
    /// external RESET.
    pub power_state: PowerState,

    /// Tracks PRAM modifications for JIT block invalidation.
    pub pram_dirty: PramDirtyBitmap,

    /// Configurable memory map (set via CreateInfo at init time)
    pub map: MemoryMap,

    /// Bitmask of unimplemented-feature warnings already printed (warn once per bit).
    pub(crate) warned_bits: u32,
    /// Last unimplemented-mode bit combination already reported, so the cold
    /// path is entered once per distinct combination rather than per block.
    pub(crate) unimpl_reported: u32,
}

impl DspState {
    /// Create a new DSP state with the given configuration.
    /// The caller owns all buffers referenced by the memory map; they must
    /// remain valid for the lifetime of this DspState.
    pub fn new(info: impl Into<CreateInfo>) -> Self {
        let map = info.into().memory_map;
        let mut registers = [0u32; reg::COUNT];
        // M registers default to $FFFFFF (linear addressing mode) per Section 4.3.4
        for i in 0..8 {
            registers[reg::M0 + i] = REG_MASKS[reg::M0];
        }
        // SR reset: CP[1-0]=1 (bits 23-22), I[1-0]=1 (bits 9-8) per Fig 5-4, Section 2.3.3
        registers[reg::SR] = 0xC0_0300;
        // OMR reset: CDP[1-0]=1 (bits 8-9) per Table 5-2; mode pins (bits 0-3)
        // are loaded from external pins on hardware - caller sets as needed.
        registers[reg::OMR] = 0x0300;
        Self {
            pc: 0,
            pc_advance: 0,
            mem_write_sink: 0,
            cycle_count: 0,
            registers,
            stack: [[0; 16]; 2],
            loop_rep: false,
            pc_on_rep: false,
            cycle_budget: 0,
            halt_requested: false,
            exit_requested: false,
            fault_budget_hint: INVALID_FAULT_BUDGET,
            power_state: PowerState::Normal,
            interrupts: InterruptPipeline::new(),
            pram_dirty: PramDirtyBitmap::new(map.p_space_end() as usize),
            map,
            warned_bits: 0,
            unimpl_reported: 0,
        }
    }

    // Stack operations

    /// Push SSH and SSL to the hardware stack.
    pub fn stack_push(&mut self, ssh_val: u32, ssl_val: u32) {
        let stack_error = self.registers[reg::SP] & (1 << 4); // SE bit
        let underflow = self.registers[reg::SP] & (1 << 5); // UF bit
        let stack = (self.registers[reg::SP] & 0xF) + 1;

        // Detect overflow: stack pointer bit 4 becomes set, no prior error
        if stack_error == 0 && (stack & (1 << 4)) != 0 {
            self.interrupts.add(interrupt::STACK_ERROR);
            self.interrupts.fault_budget = self.fault_budget_hint;
            self.fault_budget_hint = INVALID_FAULT_BUDGET;
        }

        self.registers[reg::SP] = (underflow | stack_error | stack) & 0x3F;
        // SC monitors how many hardware-stack entries are in use; it is
        // updated implicitly by every push/pop (manual 5.4.3.2). Verified
        // against sim56300 (DO mid-loop shows sc=2 for its two pushes).
        self.registers[reg::SC] = (self.registers[reg::SC] + 1) & REG_MASKS[reg::SC];
        let idx = (stack & 0xF) as usize;
        if stack != 0 {
            self.stack[0][idx] = ssh_val & REG_MASKS[reg::SSH];
            self.stack[1][idx] = ssl_val & REG_MASKS[reg::SSL];
        }
        self.registers[reg::SSH] = self.stack[0][idx];
        self.registers[reg::SSL] = self.stack[1][idx];
    }

    /// Pop SSH and SSL from the hardware stack.
    pub fn stack_pop(&mut self) -> (u32, u32) {
        let stack_error = self.registers[reg::SP] & (1 << 4);
        let underflow = self.registers[reg::SP] & (1 << 5);
        let stack = (self.registers[reg::SP] & 0xF).wrapping_sub(1);

        // Detect underflow: stack pointer bit 4 becomes set, no prior error
        if stack_error == 0 && (stack & (1 << 4)) != 0 {
            self.interrupts.add(interrupt::STACK_ERROR);
            self.interrupts.fault_budget = self.fault_budget_hint;
            self.fault_budget_hint = INVALID_FAULT_BUDGET;
        }

        self.registers[reg::SP] = (underflow | stack_error | stack) & 0x3F;
        // SC counts stack entries in use; decrement on pop (manual 5.4.3.2).
        self.registers[reg::SC] = self.registers[reg::SC].wrapping_sub(1) & REG_MASKS[reg::SC];
        let ssh = self.registers[reg::SSH];
        let ssl = self.registers[reg::SSL];
        let idx = (stack & 0xF) as usize;
        self.registers[reg::SSH] = self.stack[0][idx];
        self.registers[reg::SSL] = self.stack[1][idx];
        (ssh, ssl)
    }

    /// Warn once per unimplemented feature bit when guest code enables it.
    #[inline]
    pub(crate) fn check_unimplemented_modes(&mut self) {
        // Fast path: combined mask of all unimplemented SR bits we care about.
        const SR_UNIMPL: u32 = (1 << sr::SC) | (1 << sr::SA) | (1 << sr::DM);
        const OMR_UNIMPL: u32 = (1 << 20) | (1 << 7); // SEN, MS
        let sr = self.registers[reg::SR];
        let omr = self.registers[reg::OMR];
        let bits = (sr & SR_UNIMPL) | (omr & OMR_UNIMPL);
        if bits == 0 || bits == self.unimpl_reported {
            // Nothing set, or exactly the combination already warned about.
            // A program that sets one of these bits permanently would
            // otherwise run the cold path on every block dispatch for its
            // whole life.
            return;
        }
        self.unimpl_reported = bits;
        self.check_unimplemented_modes_slow();
    }

    #[cold]
    fn check_unimplemented_modes_slow(&mut self) {
        let sr = self.registers[reg::SR];
        let omr = self.registers[reg::OMR];
        // Each warning bit corresponds to one unimplemented feature.
        // We only print once per feature (sticky in warned_bits).
        const W_SC: u32 = 1 << 0;
        const W_SA: u32 = 1 << 1;
        const W_DM: u32 = 1 << 2;
        const W_SEN: u32 = 1 << 3;
        const W_MS: u32 = 1 << 4;

        let mut new_warnings = 0u32;

        if sr & (1 << sr::SC) != 0 && self.warned_bits & W_SC == 0 {
            eprintln!(
                "WARNING: DSP56300 16-bit compatibility mode (SC, SR bit 13) enabled at PC=${:06X} - not implemented",
                self.pc
            );
            new_warnings |= W_SC;
        }
        if sr & (1 << sr::SA) != 0 && self.warned_bits & W_SA == 0 {
            eprintln!(
                "WARNING: DSP56300 16-bit arithmetic mode (SA, SR bit 17) enabled at PC=${:06X} - not implemented",
                self.pc
            );
            new_warnings |= W_SA;
        }
        if sr & (1 << sr::DM) != 0 && self.warned_bits & W_DM == 0 {
            eprintln!(
                "WARNING: DSP56300 double-precision multiply mode (DM, SR bit 14) enabled at PC=${:06X} - not implemented",
                self.pc
            );
            new_warnings |= W_DM;
        }
        if omr & (1 << 20) != 0 && self.warned_bits & W_SEN == 0 {
            eprintln!(
                "WARNING: DSP56300 stack extension (SEN, OMR bit 20) enabled at PC=${:06X} - not implemented",
                self.pc
            );
            new_warnings |= W_SEN;
        }
        if omr & (1 << 7) != 0 && self.warned_bits & W_MS == 0 {
            eprintln!(
                "WARNING: DSP56300 memory switch mode (MS, OMR bit 7) enabled at PC=${:06X} - not implemented",
                self.pc
            );
            new_warnings |= W_MS;
        }
        self.warned_bits |= new_warnings;
    }

    /// A REP retiring inside an armed core-fault window truncates the
    /// remaining shadow to at most one more stream word. Silicon (p23
    /// sweep): with a movec-pop's 7-word window, a REP at
    /// offset d=0..4 moves the annul to rep+3 (target + one post word
    /// execute), d=5 keeps the original armer+8 boundary (min wins), and
    /// a REP standing at the boundary is annulled unexecuted. Iterations
    /// themselves are free (the fetch stream is locked; see the armed
    /// gate in step_one) and the count - zero included - does not matter.
    fn rep_truncate_armed_window(&mut self) {
        if self.interrupts.state == InterruptState::Armed {
            self.interrupts.fault_budget = self.interrupts.fault_budget.min(1);
        }
    }

    /// Post-execution PC update: handles REP iteration, PC advancement,
    /// and DO loop end-of-loop checks.
    pub fn advance_pc(&mut self) {
        self.check_unimplemented_modes();
        // REP handling
        if self.loop_rep {
            if !self.pc_on_rep {
                self.registers[reg::LC] =
                    self.registers[reg::LC].wrapping_sub(1) & REG_MASKS[reg::LC];
                if self.registers[reg::LC] > 0 {
                    self.pc_advance = 0; // stay on instruction
                } else {
                    self.loop_rep = false;
                    self.registers[reg::LC] = self.registers[reg::TEMP];
                    self.rep_truncate_armed_window();
                }
            } else {
                // First call after REP instruction. REP with LC=0 does not
                // execute the target at all on real hardware - the 56300FM's
                // "repeats 65,536 times" (page 13-160) does not hold on
                // this core. Skip the (necessarily
                // one-word) repeated instruction.
                if self.registers[reg::LC] == 0 {
                    self.loop_rep = false;
                    self.registers[reg::LC] = self.registers[reg::TEMP];
                    self.pc_advance += 1;
                    self.rep_truncate_armed_window();
                }
                self.pc_on_rep = false;
            }
        }

        self.pc = mask_pc(self.pc + self.pc_advance);

        // DO loop end-of-loop check. Only sequential fall-through from the
        // instruction at LA triggers the loop-back (pc_advance != 0); a
        // branch that lands on LA+1 does not. Hardware-verified: BRKcc jumps
        // to LA+1 with the loop state left live, and the loop neither
        // re-enters nor decrements LC.
        if (self.registers[reg::SR] & (1 << sr::LF)) != 0
            && self.pc_advance != 0
            && self.pc == mask_pc(self.registers[reg::LA] + 1)
        {
            self.registers[reg::LC] = self.registers[reg::LC].wrapping_sub(1) & REG_MASKS[reg::LC];
            if self.registers[reg::LC] == 0 && (self.registers[reg::SR] & (1 << sr::FV)) == 0 {
                // End of loop: pop saved PC+SR, restore LF+FV, pop saved LA+LC
                let (_saved_pc, saved_sr) = self.stack_pop();
                let lf_fv_mask = (1 << sr::LF) | (1 << sr::FV);
                self.registers[reg::SR] =
                    (self.registers[reg::SR] & !lf_fv_mask) | (saved_sr & lf_fv_mask);
                let (la, lc) = self.stack_pop();
                self.registers[reg::LA] = la;
                self.registers[reg::LC] = lc;
            } else {
                // Loop again: jump to loop start address (SSH)
                self.pc = self.registers[reg::SSH];
            }
        }
    }

    pub fn process_pending_interrupts(&mut self) {
        // REP is not interruptible
        if self.loop_rep {
            return;
        }

        // Handle interrupt pipeline if an interrupt is in flight
        if self.interrupts.state == InterruptState::Fast {
            match self.interrupts.pipeline_stage {
                5 => {
                    self.interrupts.pipeline_stage -= 1;
                    return;
                }
                4 => {
                    // Save PC, jump to interrupt vector
                    self.interrupts.saved_pc = self.pc;
                    self.pc = self.interrupts.vector_addr;

                    // Read instruction at vector to detect fast vs long
                    let instr = self.read_memory(MemSpace::P, self.pc);
                    self.detect_long_interrupt(instr);

                    self.interrupts.pipeline_stage -= 1;
                    return;
                }
                3 => {
                    // Second instruction prefetch (if 2-word instruction)
                    if self.pc == mask_pc(self.interrupts.vector_addr + 1) {
                        let instr = self.read_memory(MemSpace::P, self.pc);
                        self.detect_long_interrupt(instr);
                    }
                    self.interrupts.pipeline_stage -= 1;
                    return;
                }
                2 => {
                    // Fast interrupt: restore saved PC after 2-word vector
                    if self.interrupts.state != InterruptState::Long
                        && self.pc == mask_pc(self.interrupts.vector_addr + 2)
                    {
                        self.pc = self.interrupts.saved_pc;
                    }
                    self.interrupts.pipeline_stage -= 1;
                    return;
                }
                1 => {
                    self.interrupts.pipeline_stage -= 1;
                    return;
                }
                0 => {
                    // Pipeline complete, re-enable interrupts
                    self.interrupts.saved_pc = 0xFFFF;
                    self.interrupts.vector_addr = 0xFFFF;
                    // A core fault that armed while this fault was in
                    // flight parks: silicon delivers it 6 completions
                    // after this fault's dispatch. This arm runs on the
                    // 4th completion after delivery (stage hits 0 on
                    // the 3rd; the completion arm fires the tick
                    // after), so 3 completions remain
                    // (probe_ill_in_shadow family).
                    if self.interrupts.pending(interrupt::ILLEGAL)
                        || self.interrupts.pending(interrupt::STACK_ERROR)
                        || self.interrupts.pending(interrupt::TRAP)
                    {
                        self.interrupts.state = InterruptState::Parked;
                        self.interrupts.parked_countdown = 3;
                        self.interrupts.parked_sp_write = false;
                        return;
                    }
                    self.interrupts.state = InterruptState::None;
                }
                _ => return,
            }
        }

        // A parked core fault counts down completions to its delivery;
        // a guest SP write during the countdown defers it to at least
        // 3 completions after the writing instruction (silicon-probed:
        // max(dispatch+6, last_sp_write+3) fits all five
        // probe_ill_shadow* variants; SP reads do not defer).
        if self.interrupts.state == InterruptState::Parked {
            if self.interrupts.parked_countdown > 0 {
                self.interrupts.parked_countdown -= 1;
            }
            if self.interrupts.parked_sp_write {
                self.interrupts.parked_sp_write = false;
                self.interrupts.parked_countdown = self.interrupts.parked_countdown.max(3);
            }
            if self.interrupts.parked_countdown == 0 {
                self.deliver_parked_fault();
            }
            return;
        }

        // An armed core fault owns the window until delivery; don't
        // re-arbitrate over it.
        if self.interrupts.state == InterruptState::Armed {
            return;
        }

        if !self.interrupts.has_pending() {
            return;
        }

        // Arbitrate: find highest-priority unmasked interrupt
        let ipl_sr = ((self.registers[reg::SR] >> sr::I0) & 0x3) as i8;
        let mut index: Option<usize> = None;
        let mut ipl_to_raise: i8 = -1;

        for i in 0..interrupt::COUNT {
            if !self.interrupts.pending(i) {
                continue;
            }
            // Level 3 always wins (non-maskable)
            if self.interrupts.ipl[i] == 3 {
                index = Some(i);
                break;
            }
            // Skip masked interrupts
            if self.interrupts.ipl[i] < ipl_sr {
                continue;
            }
            // Pick highest IPL
            if self.interrupts.ipl[i] > ipl_to_raise {
                index = Some(i);
                ipl_to_raise = self.interrupts.ipl[i];
            }
        }

        let Some(idx) = index else { return };

        // Dispatch: clear pending, start pipeline
        self.interrupts.clear_pending(idx);

        let new_ipl = (self.interrupts.ipl[idx] + 1).min(3);
        // Vector address = VBA[23:8] | slot_offset[7:0] (per Section 5.4.4.4)
        let vba = self.registers[reg::VBA] & 0xFFFF00;
        self.interrupts.vector_addr = vba | interrupt::vector_addr(idx) as u32;
        self.interrupts.ipl_to_raise = new_ipl as u8;
        if (idx == interrupt::STACK_ERROR || idx == interrupt::ILLEGAL || idx == interrupt::TRAP)
            && self.interrupts.fault_budget != INVALID_FAULT_BUDGET
        {
            // Silicon-probed shadow model: keep executing while the
            // remaining stream-word budget lasts, then annul and vector
            // (deliver_armed_fault, called from the step loop where the
            // next instruction's length is known).
            self.interrupts.state = InterruptState::Armed;
        } else {
            self.interrupts.pipeline_stage = 5;
            self.interrupts.state = InterruptState::Fast;
        }
    }

    /// Deliver an Armed core fault: annul the instruction at the current PC
    /// (it becomes the exception frame's saved PC) and start vector fetch.
    /// Mirrors the Fast pipeline's stage-4 body; stages 3..0 then run as
    /// usual via `process_pending_interrupts`.
    pub fn deliver_armed_fault(&mut self) {
        self.interrupts.saved_pc = self.pc;
        self.pc = self.interrupts.vector_addr;
        self.interrupts.state = InterruptState::Fast;
        self.interrupts.fault_budget = INVALID_FAULT_BUDGET;

        let instr = self.read_memory(MemSpace::P, self.pc);
        self.detect_long_interrupt(instr);
        self.interrupts.pipeline_stage = 3;
    }

    /// Deliver a parked core fault (see the Parked countdown in
    /// `process_pending_interrupts`). Vector selection mirrors the
    /// arbitration loop (first pending IPL-3 source in index order);
    /// dispatch mirrors `deliver_armed_fault`. The parked fault's own
    /// stream budget does not apply: silicon delivers with zero
    /// additional shadow words - the saved PC is the next unexecuted
    /// instruction (probe_ill_in_shadow family).
    fn deliver_parked_fault(&mut self) {
        let idx = (0..interrupt::COUNT)
            .find(|&i| self.interrupts.pending(i) && self.interrupts.ipl[i] == 3);
        let Some(idx) = idx else {
            self.interrupts.state = InterruptState::None;
            return;
        };
        self.interrupts.clear_pending(idx);
        let vba = self.registers[reg::VBA] & 0xFFFF00;
        self.interrupts.vector_addr = vba | interrupt::vector_addr(idx) as u32;
        self.interrupts.ipl_to_raise = 3;
        self.deliver_armed_fault();
    }

    /// Detect whether the instruction at the interrupt vector is a long
    /// interrupt handler (contains a JSR). If so, push context to stack
    /// and update SR. Called during pipeline stages 4 and 3.
    ///
    /// Per Section 2.3.2.5: "Any Jump To Subroutine (JSR) instruction makes
    /// the interrupt long (for example, JScc, BSSET, and so on.)"
    fn detect_long_interrupt(&mut self, instr: u32) {
        use dsp56300_core::{Instruction, decode};
        let is_long = matches!(
            decode::decode(instr),
            Instruction::Jsr { .. }
                | Instruction::JsrEa { .. }
                | Instruction::Jscc { .. }
                | Instruction::JsccEa { .. }
                | Instruction::Bsr { .. }
                | Instruction::BsrLong
                | Instruction::BsrRn { .. }
                | Instruction::Bscc { .. }
                | Instruction::BsccLong { .. }
                | Instruction::BsccRn { .. }
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
        );

        if is_long && self.interrupts.state != InterruptState::Long {
            self.interrupts.state = InterruptState::Long;
            self.stack_push(self.interrupts.saved_pc, self.registers[reg::SR]);
            // Manual Section 2.3.2.5: clear LF, S1, S0, SA, and set IPL.
            // FV is NOT cleared (not listed in the manual).
            let clear_mask = (1 << sr::LF)
                | (1 << sr::S1)
                | (1 << sr::S0)
                | (1 << sr::I0)
                | (1 << sr::I1)
                | (1 << sr::SA);
            self.registers[reg::SR] &= !clear_mask;
            self.registers[reg::SR] |= (self.interrupts.ipl_to_raise as u32) << sr::I0;
        }
    }

    // Memory access

    /// Read a 24-bit word from the specified memory space.
    pub fn read_memory(&self, space: MemSpace, addr: u32) -> u32 {
        let regions = match space {
            MemSpace::X => &self.map.x_regions,
            MemSpace::Y => &self.map.y_regions,
            MemSpace::P => &self.map.p_regions,
        };
        for region in regions {
            if addr >= region.start && addr < region.end {
                let raw = match region.kind {
                    RegionKind::Buffer { base, offset } => {
                        let idx = (addr - region.start + offset) as usize;
                        unsafe { *base.add(idx) }
                    }
                    RegionKind::Callback {
                        opaque, read_fn, ..
                    } => unsafe { read_fn(opaque, addr) },
                };
                return raw & 0x00FF_FFFF;
            }
        }
        0
    }

    // Address register update

    /// Reverse the low 24 bits of a value (bit-reverse addressing domain).
    fn bitrev24(v: u32) -> u32 {
        v.reverse_bits() >> 8
    }

    /// Update address register Rn based on M register mode.
    pub fn update_rn(&mut self, numreg: usize, modifier: i32) {
        let r_mask = REG_MASKS[reg::R0];
        let m_reg = self.registers[reg::M0 + numreg] & REG_MASKS[reg::M0];
        if m_reg == REG_MASKS[reg::M0] {
            // Linear addressing (M = $FFFFFF)
            let value = (self.registers[reg::R0 + numreg] as i32).wrapping_add(modifier);
            self.registers[reg::R0 + numreg] = (value as u32) & r_mask;
        } else if m_reg == 0 {
            // Bit-reverse mode: every update operates in the bit-reversed
            // domain - r' = rev24(rev24(r) +- rev24(|modifier|)). One rule
            // reproduces all silicon observations (probes):
            // the +-1 plain updates (rev(1) = $800000 -> bit-0 toggle),
            // N = 0 (no-op), power-of-2 +Nn walks, non-power-of-2 N, and
            // the subtract direction (true reversed borrow, NOT the +Nn
            // walk: -N=8 from $001234 gives $001238, -N=3 gives $001236).
            let r_mask = REG_MASKS[reg::R0];
            let r_val = self.registers[reg::R0 + numreg] & r_mask;
            let rev_r = Self::bitrev24(r_val);
            let rev_n = Self::bitrev24(modifier.unsigned_abs() & r_mask);
            let rev_new = if modifier >= 0 {
                rev_r.wrapping_add(rev_n)
            } else {
                rev_r.wrapping_sub(rev_n)
            } & r_mask;
            self.registers[reg::R0 + numreg] = Self::bitrev24(rev_new);
        } else if (m_reg & 0xC000) == 0x8000 {
            // Multiple wrap-around modulo: bit 15=1, bit 14=0.
            // Modulo M (power of 2) stored as M-1 in bits 13:0.
            // Unlike standard modulo, supports |Nn| > M (multiple wraps).
            let modulo = (m_reg & 0x3FFF) + 1; // M, power of 2
            let r_val = self.registers[reg::R0 + numreg];
            // Base address = Rn with modulo-sized block bits cleared
            let base = r_val & !(modulo - 1);
            // Offset within block
            let offset = r_val & (modulo - 1);
            // New offset = (offset + modifier) mod M, using wrapping arithmetic
            let new_offset = (offset as i32).wrapping_add(modifier) as u32 & (modulo - 1);
            self.registers[reg::R0 + numreg] = (base | new_offset) & r_mask;
        } else if m_reg <= 0x7FFFFF {
            self.update_rn_modulo(numreg, modifier);
        }
        // else: reserved M register values, do nothing
    }

    /// Modulo address update.
    fn update_rn_modulo(&mut self, numreg: usize, mut modifier: i32) {
        let r_mask = REG_MASKS[reg::R0];
        let modulo = (self.registers[reg::M0 + numreg] & REG_MASKS[reg::M0]).wrapping_add(1);
        let orig_modifier = modifier;
        let mut bufsize: u32 = 1;
        let mut bufmask: u32 = r_mask;
        while bufsize < modulo {
            bufsize <<= 1;
            bufmask <<= 1;
        }
        bufmask &= r_mask;

        let lobound = self.registers[reg::R0 + numreg] & bufmask;
        let hibound = lobound.wrapping_add(modulo).wrapping_sub(1) & r_mask;

        let mut r_reg = self.registers[reg::R0 + numreg] as i32;

        if orig_modifier > (modulo as i32) {
            let bs = bufsize as i32;
            while modifier > bs {
                r_reg = r_reg.wrapping_add(bufsize as i32);
                modifier = modifier.wrapping_sub(bufsize as i32);
            }
            while modifier < -bs {
                r_reg = r_reg.wrapping_sub(bufsize as i32);
                modifier = modifier.wrapping_add(bufsize as i32);
            }
        }

        r_reg = r_reg.wrapping_add(modifier);

        // Wrap correction is direction-gated on silicon: an ADD corrects
        // only past the high bound, a SUBTRACT only below the low bound.
        // With the pointer starting OUTSIDE the buffer (offset >= modulo,
        // possible for non-power-of-2 moduli), a decrement that stays
        // >= lobound is plain arithmetic - silicon does NOT re-normalize
        // it into the buffer (probed: M=5, r=$0247: (r)+ -> $0242 but
        // (r)- -> $0246).
        if orig_modifier != (modulo as i32) {
            if modifier > 0 && r_reg > (hibound as i32) {
                r_reg = r_reg.wrapping_sub(modulo as i32);
            } else if modifier < 0 && r_reg < (lobound as i32) {
                r_reg = r_reg.wrapping_add(modulo as i32);
            }
        }

        self.registers[reg::R0 + numreg] = (r_reg as u32) & r_mask;
    }

    /// Write a 24-bit word to the specified memory space.
    pub fn write_memory(&mut self, space: MemSpace, addr: u32, value: u32) {
        let value = value & 0x00FF_FFFF;
        let regions = match space {
            MemSpace::X => &self.map.x_regions,
            MemSpace::Y => &self.map.y_regions,
            MemSpace::P => &self.map.p_regions,
        };
        for region in regions {
            if addr >= region.start && addr < region.end {
                match region.kind {
                    RegionKind::Buffer { base, offset } => {
                        let idx = (addr - region.start + offset) as usize;
                        unsafe {
                            *base.add(idx) = value;
                        }
                    }
                    RegionKind::Callback {
                        opaque, write_fn, ..
                    } => unsafe {
                        write_fn(opaque, addr, value);
                    },
                }
                return;
            }
        }
    }
}

// extern "C" helpers callable from JIT-compiled code

/// Update address register Rn with modulo/bit-reverse support.
/// Called from JIT-compiled code when M\[numreg\] != $FFFFFF (non-linear mode).
///
/// # Safety
/// `state` must be a valid pointer to a `DspState`.
pub unsafe extern "C" fn jit_update_rn(state: *mut DspState, numreg: u32, modifier: i32) {
    let state = unsafe { &mut *state };
    state.update_rn(numreg as usize, modifier);
}

/// Write to SSH register: increments SP and writes SSH, but leaves SSL untouched.
///
/// # Safety
/// `state` must be a valid pointer to a `DspState`.
pub unsafe extern "C" fn jit_write_ssh(state: *mut DspState, value: u32) {
    let state = unsafe { &mut *state };
    let stack_error = state.registers[reg::SP] & (1 << 4);
    let underflow = state.registers[reg::SP] & (1 << 5);
    let stack = (state.registers[reg::SP] & 0xF) + 1;

    if stack_error == 0 && (stack & (1 << 4)) != 0 {
        state.interrupts.add(interrupt::STACK_ERROR);
        state.interrupts.fault_budget = state.fault_budget_hint;
        state.fault_budget_hint = INVALID_FAULT_BUDGET;
    }

    state.registers[reg::SP] = (underflow | stack_error | stack) & 0x3F;
    // SSH write is a push: SC counts the new entry (manual 5.4.3.2).
    state.registers[reg::SC] = (state.registers[reg::SC] + 1) & REG_MASKS[reg::SC];
    let idx = (stack & 0xF) as usize;
    if stack != 0 {
        state.stack[0][idx] = value & REG_MASKS[reg::SSH];
    }
    state.registers[reg::SSH] = state.stack[0][idx];
    state.registers[reg::SSL] = state.stack[1][idx];
}

/// Read SSH register with stack pop semantics.
/// Returns the popped SSH value.
///
/// # Safety
/// `state` must be a valid pointer to a `DspState`.
pub unsafe extern "C" fn jit_read_ssh(state: *mut DspState) -> u32 {
    let state = unsafe { &mut *state };
    let (ssh, _ssl) = state.stack_pop();
    ssh
}

/// Write SSH in place: modify the top stack slot without touching SP.
/// Used by the bit-modifying ops (BSET/BCLR/BCHG on SSH), which
/// hardware-verifiably rewrite the top entry rather than pushing.
///
/// At sp=0 the operation is a STACK ERROR on silicon (VBA-redirect
/// probe): SP goes $00 -> $30 (UF|SE set, nibble untouched,
/// no push), a stack-error exception is raised, and the exception
/// frame lands in slot 1. Whether the slot-0 write itself lands is
/// not yet observable; we keep the skip until a probe pins it.
///
/// # Safety
/// `state` must be a valid pointer to a `DspState`.
pub unsafe extern "C" fn jit_write_ssh_tos(state: *mut DspState, value: u32) {
    let state = unsafe { &mut *state };
    let sp = state.registers[reg::SP];
    let idx = (sp & 0xF) as usize;
    if idx != 0 {
        state.stack[0][idx] = value & REG_MASKS[reg::SSH];
    } else {
        if sp & (1 << 4) == 0 {
            state.interrupts.add(interrupt::STACK_ERROR);
            state.interrupts.fault_budget = state.fault_budget_hint;
        }
        state.registers[reg::SP] = (sp | (1 << 5) | (1 << 4)) & 0x3F;
    }
    state.registers[reg::SSH] = state.stack[0][idx];
}

/// Write to SSL register: update stack\[1\]\[SP\].
///
/// # Safety
/// `state` must be a valid pointer to a `DspState`.
pub unsafe extern "C" fn jit_write_ssl(state: *mut DspState, value: u32) {
    let state = unsafe { &mut *state };
    let idx = (state.registers[reg::SP] & 0xF) as usize;
    // Slot 0 is real storage on silicon: an SSL write at sp=0 (movec or
    // bit op) persists in the slot and reads back after SP round-trips
    // (hardware-verified, probe_ssl_movec_write_sp0 /
    // probe_ssl_bset_sp0). No zero-forcing.
    let value = value & REG_MASKS[reg::SSL];
    state.stack[1][idx] = value;
    state.registers[reg::SSL] = value;
}

/// Write to SP register: update SP and recompute SSH/SSL from stack.
///
/// # Safety
/// `state` must be a valid pointer to a `DspState`.
pub unsafe extern "C" fn jit_write_sp(state: *mut DspState, value: u32) {
    let state = unsafe { &mut *state };
    let mask = REG_MASKS[reg::SP];
    // A guest SP write while a parked fault is counting down defers
    // its delivery to at least 3 completions after this instruction
    // (silicon, probe_ill_in_shadow/_pad; SP reads do not
    // defer).
    if state.interrupts.state == InterruptState::Parked {
        state.interrupts.parked_sp_write = true;
    }
    // Writing the SE bit into SP is itself a stack error (silicon,
    // probe_irq_sp_write_se/_2w: boundary = write start + 6
    // words flat). A UF-only write does NOT fault at the write; the
    // poisoned SP faults on the next stack operation instead
    // (probe_irq_sp_write_uf).
    let stack_error = state.registers[reg::SP] & (3 << 4);
    if stack_error == 0 && (value & (1 << 4)) != 0 {
        state.interrupts.add(interrupt::STACK_ERROR);
        state.interrupts.fault_budget = state.fault_budget_hint;
        state.fault_budget_hint = INVALID_FAULT_BUDGET;
    }
    state.registers[reg::SP] = value & mask;
    // Recompute SSH/SSL from the current stack[SP] position.
    let idx = (state.registers[reg::SP] & 0xF) as usize;
    state.registers[reg::SSH] = state.stack[0][idx];
    state.registers[reg::SSL] = state.stack[1][idx];
}

/// Read memory. Called from JIT-compiled code for runtime-computed addresses.
///
/// # Safety
/// `state` must be a valid pointer to a `DspState`.
pub unsafe extern "C" fn jit_read_mem(state: *mut DspState, space: MemSpace, address: u32) -> u32 {
    unsafe { &*state }.read_memory(space, address)
}

/// Write memory. Called from JIT-compiled code for runtime-computed addresses.
/// P-space writes include dirty bitmap tracking for JIT cache invalidation.
///
/// # Safety
/// `state` must be a valid pointer to a `DspState`.
pub unsafe extern "C" fn jit_write_mem(
    state: *mut DspState,
    space: MemSpace,
    address: u32,
    value: u32,
) {
    let state = unsafe { &mut *state };
    if space == MemSpace::P {
        let masked = value & 0x00FF_FFFF;
        let old = state.read_memory(MemSpace::P, address);
        if old != masked {
            state.write_memory(MemSpace::P, address, masked);
            state.pram_dirty.mark_dirty(address);
        }
    } else {
        state.write_memory(space, address, value)
    }
}

/// Round a 56-bit accumulator value.
/// Implements convergent rounding (round-to-even) with three scaling modes.
/// The value is stored as a 56-bit signed integer in an i64:
///   bits 55:48 = A2 (sign extension), bits 47:24 = A1, bits 23:0 = A0.
///
/// # Safety
/// `state` must be a valid pointer to a `DspState`.
pub unsafe extern "C" fn jit_rnd56(state: *mut DspState, val: i64) -> i64 {
    let state = unsafe { &*state };
    let sr = state.registers[reg::SR];
    let convergent = sr & (1 << sr::RM) == 0; // RM=0: convergent, RM=1: two's complement

    let (r2, r1, r0) = if sr & (1 << sr::S0) != 0 {
        // Scaling mode S0: round at bit 24 (A1 boundary)
        let sum = val.wrapping_add(1 << 24); // add rnd_const = {0, 1, 0}
        let s0 = (sum & 0xFF_FFFF) as u32;
        let mut s1 = ((sum >> 24) & 0xFF_FFFF) as u32;
        let s2 = ((sum >> 48) & 0xFF) as u32;
        if convergent && s0 == 0 && (s1 & 1) == 0 {
            s1 &= 0xFF_FFFF - 0x3;
        }
        s1 &= 0xFF_FFFE;
        (s2, s1, 0u32)
    } else if sr & (1 << sr::S1) != 0 {
        // Scaling mode S1: round at bit 22
        let sum = val.wrapping_add(1 << 22);
        let mut s0 = (sum & 0xFF_FFFF) as u32;
        let s1 = ((sum >> 24) & 0xFF_FFFF) as u32;
        let s2 = ((sum >> 48) & 0xFF) as u32;
        if convergent && (s0 & 0x7F_FFFF) == 0 {
            s0 = 0;
        }
        s0 &= 0x80_0000;
        (s2, s1, s0)
    } else {
        // No scaling: round at bit 23 (A0/A1 boundary)
        let sum = val.wrapping_add(1 << 23);
        let s0 = (sum & 0xFF_FFFF) as u32;
        let mut s1 = ((sum >> 24) & 0xFF_FFFF) as u32;
        let s2 = ((sum >> 48) & 0xFF) as u32;
        if convergent && s0 == 0 {
            s1 &= 0xFF_FFFE;
        }
        (s2, s1, 0u32)
    };

    ((r2 as i64) << 48) | ((r1 as i64) << 24) | (r0 as i64)
}

/// Update E, U, N, Z in SR from a 56-bit accumulator value, in all three
/// scaling modes. Called about once per DSP cycle, so its shape matters:
///
/// * SR goes in as a value and the new SR comes back, so the emitted code
///   keeps SR in a register across the call. A `*mut DspState` form would
///   cost a store and a reload on each side.
/// * The common no-scaling mode is tested inline and the other three are
///   outlined. Scaling is a mode that holds for long stretches, and a
///   `match` on it compiles to an indirect jump.
///
/// Emitting the body inline as IR is not worth it: the call is a small
/// fraction of the body's cost, and a program that pages overlays rebuilds
/// tens of thousands of blocks a second, so the extra IR per site would be
/// paid back in compile time.
pub extern "C" fn jit_update_nz(sr: u32, acc_val: i64) -> u32 {
    let reg1 = ((acc_val >> 24) & 0xFF_FFFF) as u32; // MSP

    let (e, u) = if sr & ((1 << sr::S0) | (1 << sr::S1)) == 0 {
        // No scaling.
        let reg0 = ((acc_val >> 48) & 0xFF) as u32; // extension byte
        let val_e = ((reg0 << 1) | (reg1 >> 23)) & 0x1FF;
        let bits = reg1 & 0xC0_0000;
        (val_e != 0 && val_e != 0x1FF, bits == 0 || bits == 0xC0_0000)
    } else {
        eu_scaled(sr, acc_val)
    };

    let n = (acc_val >> 55) & 1 != 0;
    let z = (acc_val & 0x00FF_FFFF_FFFF_FFFF) == 0;

    let clear_mask = !((1u32 << sr::E) | (1u32 << sr::U) | (1u32 << sr::N) | (1u32 << sr::Z));
    let mut new_sr = sr & clear_mask;
    if e {
        new_sr |= 1 << sr::E;
    }
    if u {
        new_sr |= 1 << sr::U;
    }
    if n {
        new_sr |= 1 << sr::N;
    }
    if z {
        new_sr |= 1 << sr::Z;
    }
    new_sr
}

/// E and U for the three scaling modes that are not "no scaling". Outlined
/// so the common mode's path stays straight-line; see `jit_update_nz`.
#[cold]
#[inline(never)]
fn eu_scaled(sr: u32, acc_val: i64) -> (bool, bool) {
    let reg0 = ((acc_val >> 48) & 0xFF) as u32;
    let reg1 = ((acc_val >> 24) & 0xFF_FFFF) as u32;
    match (sr >> sr::S0) & 3 {
        1 => {
            // Scale down (S1:S0=01)
            let val = ((reg0 << 1) | (reg1 >> 23)) & 3;
            (reg0 != 0 && reg0 != 0xFF, val == 0 || val == 3)
        }
        2 => {
            // Scale up (S1:S0=10)
            let val_e = ((reg0 << 2) | (reg1 >> 22)) & 0x3FF;
            let bits = reg1 & 0x60_0000;
            (val_e != 0 && val_e != 0x3FF, bits == 0 || bits == 0x60_0000)
        }
        _ => (false, false), // scaling=3: no change
    }
}

/// Arithmetic Saturation Mode: clamp a 56-bit accumulator if SM=1.
/// Returns the result with bit 56 set if saturation was needed (needs_sat flag).
/// Bits 55:0 contain the (possibly clamped) value.
///
/// # Safety
/// `state` must be a valid pointer to a `DspState`.
pub unsafe extern "C" fn jit_saturate_sm(state: *mut DspState, val: i64) -> i64 {
    let state = unsafe { &*state };
    let sr = state.registers[reg::SR];
    if sr & (1 << sr::SM) == 0 {
        return val; // SM=0: no saturation, needs_sat=0 (bit 56 clear)
    }
    let b55 = (val >> 55) & 1;
    let b48 = (val >> 48) & 1;
    let b47 = (val >> 47) & 1;
    let mismatch = (b55 ^ b48) | (b48 ^ b47);
    if mismatch == 0 {
        return val; // No saturation needed
    }
    // Saturate: bit 55 = 0 -> max positive, 1 -> max negative
    let saturated = if b55 != 0 {
        0x00FF_8000_0000_0000_u64 as i64
    } else {
        0x0000_7FFF_FFFF_FFFF_i64
    };
    saturated | (1i64 << 56) // Set needs_sat flag in bit 56
}

/// Read accumulator as 24-bit value with scaling, limiting, and S flag update.
///
/// Returns the 24-bit result in bits \[23:0\]. Bit 24 is the `no_limit` flag
/// (1 = value was not clamped). Updates SR.L and SR.S in-place.
///
/// `acc_idx`: 0 = A, 1 = B (index into sub-register triples)
///
/// # Safety
/// `state` must be a valid pointer to a `DspState`.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn jit_read_accu24(state: *mut DspState, acc_idx: u32) -> u32 {
    let state = unsafe { &mut *state };
    let (a2, a1, a0) = if acc_idx == 0 {
        (
            state.registers[reg::A2],
            state.registers[reg::A1],
            state.registers[reg::A0],
        )
    } else {
        (
            state.registers[reg::B2],
            state.registers[reg::B1],
            state.registers[reg::B0],
        )
    };

    let sr = state.registers[reg::SR];
    let scaling = (sr >> sr::S0) & 3;

    // Apply data shifter
    let combined = (a2 << 24) | a1;
    let value = match scaling {
        1 => combined >> 1,                      // scale down
        2 => (combined << 1) | ((a0 >> 23) & 1), // scale up
        _ => combined,                           // no scaling
    } & 0x00FF_FFFF;

    // Limiting check
    let ok_pos = a2 == 0 && value <= 0x7F_FFFF;
    let ok_neg = a2 == 0xFF && value >= 0x80_0000;
    let mut no_limit = ok_pos || ok_neg;

    // Scale-up fix: bit 47 (A1[23]) must match A2 sign at extension boundary
    if scaling == 2 {
        let a1_bit23 = (a1 >> 23) & 1;
        let ok_pos_s2 = a2 == 0 && value <= 0x7F_FFFF && a1_bit23 == 0;
        let ok_neg_s2 = a2 == 0xFF && value >= 0x80_0000 && a1_bit23 != 0;
        no_limit = ok_pos_s2 || ok_neg_s2;
    }

    let result = if no_limit {
        value
    } else {
        // Clamp: negative (A2 bit 7 set) -> 0x800000, positive -> 0x7FFFFF
        if a2 & 0x80 != 0 { 0x80_0000 } else { 0x7F_FFFF }
    };

    // Update SR: set L if limited, compute S flag (sticky data growth)
    let mut new_sr = sr;
    if !no_limit {
        new_sr |= 1 << sr::L;
    }

    // S flag: adjacent bits differ in the unscaled accumulator. The
    // examined pair moves WITH the scaling mode: scale down watches
    // 47^46, scale up watches 45^44 (silicon-verified).
    let acc_packed =
        ((a2 as u64 & 0xFF) << 48) | ((a1 as u64 & 0xFF_FFFF) << 24) | (a0 as u64 & 0xFF_FFFF);
    let s_bit = match scaling {
        1 => ((acc_packed >> 47) ^ (acc_packed >> 46)) & 1, // scale down
        2 => ((acc_packed >> 45) ^ (acc_packed >> 44)) & 1, // scale up
        _ => ((acc_packed >> 46) ^ (acc_packed >> 45)) & 1, // no scaling
    };
    if s_bit != 0 {
        new_sr |= 1 << sr::S;
    }
    state.registers[reg::SR] = new_sr;

    result | (if no_limit { 1 << 24 } else { 0 })
}

impl Default for DspState {
    fn default() -> Self {
        Self::new(MemoryMap::default())
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    pub const PRAM_SIZE: usize = 4096;
    pub const XRAM_SIZE: usize = 4096;
    pub const YRAM_SIZE: usize = 2048;

    #[test]
    fn test_new_state_is_zeroed() {
        let state = DspState::new(MemoryMap::default());
        assert_eq!(state.pc, 0);
        assert_eq!(state.cycle_count, 0);
        assert!(!state.halt_requested);
        assert_eq!(state.registers[reg::A1], 0);
    }

    #[test]
    fn test_memory_read_write() {
        let mut xram = [0u32; XRAM_SIZE];
        let mut yram = [0u32; YRAM_SIZE];
        let mut pram = [0u32; PRAM_SIZE];
        let mut state = DspState::new(MemoryMap::test(&mut xram, &mut yram, &mut pram));
        state.write_memory(MemSpace::X, 3, 0x123456);
        assert_eq!(state.read_memory(MemSpace::X, 3), 0x123456);

        state.write_memory(MemSpace::Y, 100, 0xABCDEF);
        assert_eq!(state.read_memory(MemSpace::Y, 100), 0xABCDEF);

        state.write_memory(MemSpace::P, 0x40, 0x0AF080);
        assert_eq!(state.read_memory(MemSpace::P, 0x40), 0x0AF080);
    }

    #[test]
    fn test_memory_24bit_mask() {
        let mut xram = [0u32; XRAM_SIZE];
        let mut yram = [0u32; YRAM_SIZE];
        let mut pram = [0u32; PRAM_SIZE];
        let mut state = DspState::new(MemoryMap::test(&mut xram, &mut yram, &mut pram));
        state.write_memory(MemSpace::X, 0, 0xFF123456);
        assert_eq!(state.read_memory(MemSpace::X, 0), 0x123456);
    }

    #[test]
    fn test_out_of_bounds_read_returns_zero() {
        let mut xram = [0u32; XRAM_SIZE];
        let mut yram = [0u32; YRAM_SIZE];
        let mut pram = [0u32; PRAM_SIZE];
        let state = DspState::new(MemoryMap::test(&mut xram, &mut yram, &mut pram));
        assert_eq!(state.read_memory(MemSpace::X, XRAM_SIZE as u32 + 100), 0);
        assert_eq!(state.read_memory(MemSpace::Y, YRAM_SIZE as u32 + 100), 0);
        assert_eq!(state.read_memory(MemSpace::P, PRAM_SIZE as u32 + 100), 0);
    }

    // Bit-reverse addressing

    #[test]
    fn test_bitreverse_basic() {
        // M0=0 means bit-reverse mode. N0 determines reversal width.
        // N0=8 -> 4 bits (trailing zeros in 8=0b1000 -> revbits=3+1=4).
        // R0=0 -> reversed increment: 0->8->4->12->2->...
        let mut state = DspState::new(MemoryMap::default());
        state.registers[reg::M0] = 0;
        state.registers[reg::N0] = 8;
        state.registers[reg::R0] = 0;

        // Step through the bit-reverse sequence ((Rn)+Nn form: the
        // modifier carries Nn; magnitude beyond +-1 selects the
        // N-derived reverse-carry walk)
        let expected = [8, 4, 12, 2, 10, 6, 14, 1];
        for &exp in &expected {
            state.update_rn(0, 8);
            assert_eq!(
                state.registers[reg::R0],
                exp,
                "expected R0={exp} after bit-reverse step"
            );
        }
    }

    #[test]
    fn test_bitreverse_plain_toggles_bit0() {
        // Silicon: plain (Rn)+ / (Rn)- in bit-reverse mode toggle bit 0,
        // independent of N and direction (see ARCHITECTURE-NOTES.md).
        let mut state = DspState::new(MemoryMap::default());
        state.registers[reg::M0] = 0;
        for n in [0u32, 1, 3, 8] {
            state.registers[reg::N0] = n;
            state.registers[reg::R0] = 0x001234;
            state.update_rn(0, 1);
            assert_eq!(state.registers[reg::R0], 0x001235, "inc, N={n}");
            state.registers[reg::R0] = 0x000F0F;
            state.update_rn(0, -1);
            assert_eq!(state.registers[reg::R0], 0x000F0E, "dec, N={n}");
        }
    }

    #[test]
    fn test_bitreverse_n0_4() {
        // N0=4=0b100 -> revbits=2+1=3, reversing 3 bits
        // Sequence from 0: 4,2,6,1,5,3,7,0
        let mut state = DspState::new(MemoryMap::default());
        state.registers[reg::M0] = 0;
        state.registers[reg::N0] = 4;
        state.registers[reg::R0] = 0;

        let expected = [4, 2, 6, 1, 5, 3, 7, 0];
        for &exp in &expected {
            state.update_rn(0, 4);
            assert_eq!(state.registers[reg::R0], exp);
        }
    }

    // Interrupt priority selection

    #[test]
    fn test_interrupt_priority_level3_wins() {
        let mut xram = [0u32; XRAM_SIZE];
        let mut yram = [0u32; YRAM_SIZE];
        let mut pram = [0u32; PRAM_SIZE];
        // Place NOP at interrupt vector for ILLEGAL (0x3E, 0x3F)
        pram[0x3E] = 0x000000; // nop
        pram[0x3F] = 0x000000; // nop
        // Level 3 (non-maskable) should always be dispatched regardless of IPL mask
        let mut state = DspState::new(MemoryMap::test(&mut xram, &mut yram, &mut pram));
        state.interrupts.vector_addr = 0xFFFF;
        state.interrupts.saved_pc = 0xFFFF;
        // Set IPL mask to max (I0=1, I1=1 -> IPL=3)
        state.registers[reg::SR] = 3 << sr::I0;
        // Set up a level-3 interrupt (ILLEGAL, ipl=3)
        state.interrupts.ipl[interrupt::ILLEGAL] = 3;
        state.interrupts.add(interrupt::ILLEGAL);

        assert!(state.interrupts.has_pending());
        state.process_pending_interrupts();
        // Interrupt should be dispatched (pipeline started)
        assert!(!state.interrupts.has_pending());
        assert_eq!(state.interrupts.state, InterruptState::Fast);
    }

    #[test]
    fn test_interrupt_masked_by_ipl() {
        // TRAP at IPL 1 should be masked when SR IPL mask is 2
        let mut state = DspState::new(MemoryMap::default());
        state.interrupts.vector_addr = 0xFFFF;
        state.interrupts.saved_pc = 0xFFFF;
        state.registers[reg::SR] = 2 << sr::I0;
        state.interrupts.ipl[interrupt::TRAP] = 1;
        state.interrupts.add(interrupt::TRAP);

        state.process_pending_interrupts();
        assert!(state.interrupts.has_pending());
        assert!(state.interrupts.pending(interrupt::TRAP));
    }

    #[test]
    fn test_interrupt_highest_priority_wins() {
        let mut xram = [0u32; XRAM_SIZE];
        let mut yram = [0u32; YRAM_SIZE];
        let mut pram = [0u32; PRAM_SIZE];
        // Place NOPs at ILLEGAL vector so dispatch succeeds
        let vec_addr = interrupt::vector_addr(interrupt::ILLEGAL) as usize;
        pram[vec_addr] = 0x000000;
        pram[vec_addr + 1] = 0x000000;
        let mut state = DspState::new(MemoryMap::test(&mut xram, &mut yram, &mut pram));
        state.interrupts.vector_addr = 0xFFFF;
        state.interrupts.saved_pc = 0xFFFF;
        state.registers[reg::SR] = 0;
        // TRAP at IPL 1, ILLEGAL at IPL 2
        state.interrupts.ipl[interrupt::TRAP] = 1;
        state.interrupts.ipl[interrupt::ILLEGAL] = 2;
        state.interrupts.add(interrupt::TRAP);
        state.interrupts.add(interrupt::ILLEGAL);

        state.process_pending_interrupts();
        // ILLEGAL (IPL 2) should be dispatched first
        assert!(!state.interrupts.pending(interrupt::ILLEGAL));
        assert!(state.interrupts.pending(interrupt::TRAP));
        assert!(state.interrupts.has_pending());
    }

    // jit_rnd56 convergent rounding

    fn pack_acc56(a2: u32, a1: u32, a0: u32) -> i64 {
        ((a2 as i64 & 0xFF) << 48) | ((a1 as i64 & 0xFF_FFFF) << 24) | (a0 as i64 & 0xFF_FFFF)
    }

    #[test]
    fn test_rnd56_default_round_up() {
        let mut state = DspState::new(MemoryMap::default());
        // A0 > 0x800000 -> round up
        let val = pack_acc56(0x00, 0x400000, 0x800001);
        let result = unsafe { jit_rnd56(&mut state as *mut DspState, val) };
        let r1 = ((result >> 24) & 0xFF_FFFF) as u32;
        let r0 = (result & 0xFF_FFFF) as u32;
        assert_eq!(r1, 0x400001, "A1 should round up");
        assert_eq!(r0, 0, "A0 should be cleared");
    }

    #[test]
    fn test_rnd56_default_round_down() {
        let mut state = DspState::new(MemoryMap::default());
        // A0 < 0x800000 -> round down (truncate)
        let val = pack_acc56(0x00, 0x400000, 0x7FFFFF);
        let result = unsafe { jit_rnd56(&mut state as *mut DspState, val) };
        let r1 = ((result >> 24) & 0xFF_FFFF) as u32;
        assert_eq!(r1, 0x400000, "A1 should stay");
    }

    #[test]
    fn test_rnd56_convergent_half_even() {
        let mut state = DspState::new(MemoryMap::default());
        // Exactly half (A0=0x800000), A1 bit 0 = 0 -> round down (convergent)
        let val = pack_acc56(0x00, 0x400000, 0x800000);
        let result = unsafe { jit_rnd56(&mut state as *mut DspState, val) };
        let r1 = ((result >> 24) & 0xFF_FFFF) as u32;
        assert_eq!(r1, 0x400000, "even A1 -> round down");
    }

    #[test]
    fn test_rnd56_convergent_half_odd() {
        let mut state = DspState::new(MemoryMap::default());
        // Exactly half (A0=0x800000), A1 bit 0 = 1 -> round up (convergent)
        let val = pack_acc56(0x00, 0x400001, 0x800000);
        let result = unsafe { jit_rnd56(&mut state as *mut DspState, val) };
        let r1 = ((result >> 24) & 0xFF_FFFF) as u32;
        assert_eq!(r1, 0x400002, "odd A1 -> round up");
    }

    #[test]
    fn test_rnd56_s0_scaling() {
        let mut state = DspState::new(MemoryMap::default());
        state.registers[reg::SR] = 1 << sr::S0;
        let val = pack_acc56(0x00, 0x400000, 0x800000);
        let result = unsafe { jit_rnd56(&mut state as *mut DspState, val) };
        // S0 mode rounds at bit 24 (A1 boundary) - result should clear A0
        let r0 = (result & 0xFF_FFFF) as u32;
        assert_eq!(r0, 0);
    }

    #[test]
    fn test_rnd56_s1_scaling() {
        let mut state = DspState::new(MemoryMap::default());
        state.registers[reg::SR] = 1 << sr::S1;
        let val = pack_acc56(0x00, 0x400001, 0xC00000);
        let result = unsafe { jit_rnd56(&mut state as *mut DspState, val) };
        // S1 mode rounds at bit 22
        let r0 = (result & 0xFF_FFFF) as u32;
        // A0 should be masked to just bit 23
        assert_eq!(
            r0 & 0x7F_FFFF,
            0,
            "lower 23 bits of A0 should be cleared in S1 mode"
        );
    }

    // ---- read_pram / p_space_end ----

    #[test]
    fn test_read_pram_buffer() {
        let mut pram = [0u32; 4];
        pram[0] = 0xABCDEF;
        pram[1] = 0xFFFFFF;
        pram[2] = 0x123456;
        let map = MemoryMap {
            p_regions: vec![MemoryRegion {
                start: 0,
                end: 4,
                kind: RegionKind::Buffer {
                    base: pram.as_mut_ptr(),
                    offset: 0,
                },
            }],
            ..Default::default()
        };
        assert_eq!(map.read_pram(0), 0xABCDEF);
        assert_eq!(map.read_pram(1), 0xFFFFFF);
        assert_eq!(map.read_pram(2), 0x123456);
        assert_eq!(map.read_pram(4), 0); // out of range
        assert_eq!(map.p_space_end(), 4);
    }

    #[test]
    fn test_read_pram_callback() {
        unsafe extern "C" fn cb_read(_opaque: *mut std::ffi::c_void, addr: u32) -> u32 {
            // Return addr * 0x111 as a recognizable pattern
            addr * 0x111
        }
        unsafe extern "C" fn cb_write(_opaque: *mut std::ffi::c_void, _addr: u32, _val: u32) {}
        let map = MemoryMap {
            p_regions: vec![MemoryRegion {
                start: 0,
                end: 8,
                kind: RegionKind::Callback {
                    opaque: std::ptr::null_mut(),
                    read_fn: cb_read,
                    write_fn: cb_write,
                },
            }],
            ..Default::default()
        };
        assert_eq!(map.read_pram(0), 0);
        assert_eq!(map.read_pram(3), 0x333);
        assert_eq!(map.read_pram(7), 0x777);
        assert_eq!(map.read_pram(8), 0); // out of range
        assert_eq!(map.p_space_end(), 8);
    }

    #[test]
    fn test_read_pram_masks_to_24_bits() {
        unsafe extern "C" fn cb_read(_opaque: *mut std::ffi::c_void, _addr: u32) -> u32 {
            0xFF123456 // upper byte should be masked off
        }
        unsafe extern "C" fn cb_write(_opaque: *mut std::ffi::c_void, _addr: u32, _val: u32) {}
        let map = MemoryMap {
            p_regions: vec![MemoryRegion {
                start: 0,
                end: 1,
                kind: RegionKind::Callback {
                    opaque: std::ptr::null_mut(),
                    read_fn: cb_read,
                    write_fn: cb_write,
                },
            }],
            ..Default::default()
        };
        assert_eq!(map.read_pram(0), 0x123456);
    }

    #[test]
    fn test_read_pram_empty_map() {
        let map = MemoryMap::default();
        assert_eq!(map.read_pram(0), 0);
        assert_eq!(map.p_space_end(), 0);
    }

    #[test]
    fn test_interrupt_state_from() {
        assert!(matches!(InterruptState::from(0), InterruptState::None));
        assert!(matches!(InterruptState::from(1), InterruptState::Fast));
        assert!(matches!(InterruptState::from(2), InterruptState::Long));
        assert!(matches!(InterruptState::from(255), InterruptState::None));
    }

    #[test]
    fn test_interrupt_state_display() {
        assert_eq!(format!("{}", InterruptState::None), "none");
        assert_eq!(format!("{}", InterruptState::Fast), "fast");
        assert_eq!(format!("{}", InterruptState::Long), "long");
    }

    #[test]
    fn test_dirty_range_middle_words() {
        let mut cache = PramDirtyBitmap::new(4096);
        // Mark a single bit in the middle word (word index 1, bit 64+10=74)
        cache.mark_dirty(74);
        assert!(cache.is_range_dirty(0, 192));
        assert!(!cache.is_range_dirty(0, 64));
        assert!(!cache.is_range_dirty(128, 192));
    }

    #[test]
    fn test_clear_dirty_range_multi_word() {
        let mut cache = PramDirtyBitmap::new(4096);
        // Mark dirty bits across 3 words
        cache.mark_dirty(10); // word 0
        cache.mark_dirty(74); // word 1
        cache.mark_dirty(140); // word 2
        assert!(cache.is_range_dirty(0, 192));
        // Clear the whole range
        cache.clear_dirty_range(0, 192);
        assert!(!cache.is_range_dirty(0, 192));
    }

    #[test]
    fn test_modulo_negative_wrap() {
        // M0 = 3 means modulo 4. Large negative modifier triggers the
        // while (modifier < -bufsize) loop.
        let mut state = DspState::new(MemoryMap::default());
        state.registers[reg::M0] = 3; // modulo = M+1 = 4
        state.registers[reg::R0] = 6; // start address
        state.registers[reg::N0] = 0;
        state.update_rn(0, -10);
        let r = state.registers[reg::R0];
        assert!(r <= 7, "R0 should be within modulo buffer bounds, got {r}");
    }
}

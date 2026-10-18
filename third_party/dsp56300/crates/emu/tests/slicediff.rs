//! Slice-differential search: one big run() must equal many small run()s.
//!
//! run() preempts an inline DO loop at an iteration boundary and resumes the
//! rest of it through the block-boundary path, so a loop can execute by two
//! different routes through the translator and the two are supposed to be the
//! same machine. The preemption point is a fixed quantum rather than the run
//! loop's remaining budget, so the caller's slice size must not pick the
//! route (see emit_loop_preemption_check); this asserts that whatever the
//! schedule, the machine ends up in the same state.
//!
//!   cargo test --release --test slicediff -- --ignored --nocapture
//!   SLICEDIFF_MINIMIZE=<seed> cargo test --release --test slicediff -- --ignored
use dsp56300_emu::core::{DspState, MemoryMap, PowerState, REG_MASKS, mask_reg, reg};
use dsp56300_emu::jit::JitEngine;

const PRAM: usize = 64;
const XRAM: usize = 256;
const YRAM: usize = 256;
const DEFAULT_TOTAL: i32 = 300;
/// Programs per run, overridable with SLICEDIFF_ITERS.
const DEFAULT_ITERS: u64 = 20_000;

struct Rng(u64);
impl Rng {
    fn next(&mut self) -> u64 {
        let mut x = self.0;
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        self.0 = x;
        x
    }
    fn n(&mut self, m: u32) -> u32 {
        (self.next() % m as u64) as u32
    }
}

/// Words that decode to something interesting more often than chance.
const POOL: &[u32] = &[
    0x000000, 0x000008, 0x200013, 0x200040, 0x200050, 0x20001B, 0x218000, 0x5C5800, 0x4C5800,
    0x565800, 0x445800, 0x08F4A0, 0x0603A0, 0x060280, 0x2E0000, 0x44F400, 0x200003, 0x20002B,
    0x0000B9, 0x0000BB, 0x00008C, 0x21C400, 0x21CC00, 0x21DE00, 0x4EA000, 0x5EA000, 0x46D800,
    0x56D800,
];

const REG_NAME: [&str; 8] = ["X0", "X1", "Y0", "Y1", "A0", "B0", "A2", "B2"];

fn reg_name(i: usize) -> String {
    match i {
        0x04..=0x0b => REG_NAME[i - 4].into(),
        0x0c => "A1".into(),
        0x0d => "B1".into(),
        0x10..=0x17 => format!("R{}", i - 0x10),
        0x18..=0x1f => format!("N{}", i - 0x18),
        0x20..=0x27 => format!("M{}", i - 0x20),
        reg::SR => "SR".into(),
        reg::OMR => "OMR".into(),
        reg::SP => "SP".into(),
        reg::SSH => "SSH".into(),
        reg::SSL => "SSL".into(),
        reg::LA => "LA".into(),
        reg::LC => "LC".into(),
        reg::TEMP => "TEMP".into(),
        _ => format!("reg[{:#04x}]", i),
    }
}

/// A program plus the register file it starts from.
#[derive(Clone)]
struct Case {
    p: Vec<u32>,
    x: Vec<u32>,
    y: Vec<u32>,
    regs: [u32; reg::COUNT],
}

fn snapshot(s: &DspState, x: &[u32], y: &[u32], p: &[u32]) -> Vec<u32> {
    let mut v = Vec::new();
    v.push(s.pc);
    v.push(s.cycle_count);
    v.extend_from_slice(&s.registers);
    for h in 0..2 {
        v.extend_from_slice(&s.stack[h]);
    }
    v.push(s.loop_rep as u32);
    v.push(s.pc_on_rep as u32);
    v.extend_from_slice(x);
    v.extend_from_slice(y);
    v.extend_from_slice(p);
    v
}

fn label(i: usize) -> String {
    if i == 0 {
        return "pc".into();
    }
    if i == 1 {
        return "cycle_count".into();
    }
    let i = i - 2;
    if i < reg::COUNT {
        return reg_name(i);
    }
    let i = i - reg::COUNT;
    if i < 32 {
        return format!("stack[{}][{}]", i / 16, i % 16);
    }
    let i = i - 32;
    if i < 2 {
        return ["loop_rep", "pc_on_rep"][i].into();
    }
    let i = i - 2;
    if i < XRAM {
        return format!("X:{:03x}", i);
    }
    let i = i - XRAM;
    if i < YRAM {
        return format!("Y:{:03x}", i);
    }
    format!("P:{:03x}", i - YRAM)
}

fn gen_case(rng: &mut Rng) -> Case {
    let mut p = vec![0u32; PRAM];
    let body = 1 + rng.n(5) as usize;
    let la = 1 + body; // the body is [2, la]
    let count = 1 + rng.n(6);
    p[0] = 0x060080 | (count << 8); // do #count,la
    p[1] = la as u32;
    for w in p.iter_mut().take(PRAM).skip(2) {
        *w = if rng.n(2) == 0 {
            POOL[rng.n(POOL.len() as u32) as usize]
        } else {
            rng.n(0x1000000)
        };
    }
    // Straight-line forward-skip clusters (the emit_block if-conversion):
    // the same op set as gen_body's in-loop skips, planted in the tail
    // the loop falls into. The region words stay whatever the random
    // fill put there, so misaligned merges and nested/rejected shapes
    // all occur.
    for _ in 0..1 + rng.n(3) {
        let at = la + 2 + rng.n((PRAM - la - 8) as u32) as usize;
        let op = match rng.n(3) {
            0 => 0x0CC581, // brclr #1,x1,<fwd>
            1 => 0x0CC6A0, // brset #0,y0,<fwd>
            _ => 0x0D1049, // blt <fwd>
        };
        p[at] = op;
        p[at + 1] = 2 + rng.n(3);
    }
    let x: Vec<u32> = (0..XRAM).map(|_| rng.n(0x1000000)).collect();
    let y: Vec<u32> = (0..YRAM).map(|_| rng.n(0x1000000)).collect();

    let mut regs = [0u32; reg::COUNT];
    for i in 0..8 {
        regs[reg::M0 + i] = REG_MASKS[reg::M0];
    }
    regs[reg::SR] = 0xC0_0300;
    regs[reg::OMR] = 0x0300;
    // Seed only architecturally reachable values: A2/B2 are 8 bits, and an
    // out-of-range extension byte diverges the promoted-accumulator path
    // from the single-instruction path over a state no program can produce.
    for r in [
        reg::A0,
        reg::A1,
        reg::A2,
        reg::B0,
        reg::B1,
        reg::B2,
        reg::X0,
        reg::X1,
        reg::Y0,
        reg::Y1,
    ] {
        regs[r] = mask_reg(r, rng.n(0x1000000));
    }
    for i in 0..8 {
        regs[reg::R0 + i] = rng.n(XRAM as u32);
        regs[reg::N0 + i] = rng.n(8);
    }
    Case { p, x, y, regs }
}

struct Buffers {
    jit: JitEngine,
    x: Vec<u32>,
    y: Vec<u32>,
    p: Vec<u32>,
}

impl Buffers {
    fn new() -> Self {
        Self {
            jit: JitEngine::new(PRAM),
            x: vec![0u32; XRAM],
            y: vec![0u32; YRAM],
            p: vec![0u32; PRAM],
        }
    }

    /// An engine whose inline loops bail after `quantum` cycles in a block.
    #[cfg(feature = "tunable-loop-quantum")]
    fn with_quantum(quantum: i32) -> Self {
        let mut b = Self::new();
        b.jit.set_inline_loop_quantum(quantum);
        b
    }

    /// The same program one instruction at a time, through the
    /// single-instruction compilation path the silicon goldens are captured
    /// against. Only the three-way differential calls it, and that needs the
    /// tunable quantum.
    #[cfg_attr(not(feature = "tunable-loop-quantum"), allow(dead_code))]
    fn run_stepped(&mut self, c: &Case, total: i32) -> Option<Vec<u32>> {
        self.p.copy_from_slice(&c.p);
        self.x.copy_from_slice(&c.x);
        self.y.copy_from_slice(&c.y);
        let mut s = DspState::new(MemoryMap::test(&mut self.x, &mut self.y, &mut self.p));
        s.registers = c.regs;
        while (s.cycle_count as i32) < total {
            s.execute_one(&mut self.jit);
            if s.power_state != PowerState::Normal || s.halt_requested {
                return None;
            }
        }
        Some(snapshot(&s, &self.x, &self.y, &self.p))
    }

    fn run(&mut self, c: &Case, slice: i32, total: i32) -> Option<Vec<u32>> {
        self.p.copy_from_slice(&c.p);
        self.x.copy_from_slice(&c.x);
        self.y.copy_from_slice(&c.y);
        let mut s = DspState::new(MemoryMap::test(&mut self.x, &mut self.y, &mut self.p));
        s.registers = c.regs;
        let mut given = 0;
        while given < total {
            let step = slice.min(total - given);
            given += step;
            s.run(&mut self.jit, step);
            if s.power_state != PowerState::Normal || s.halt_requested {
                return None;
            }
        }
        Some(snapshot(&s, &self.x, &self.y, &self.p))
    }
}

fn env_u64(k: &str) -> Option<u64> {
    std::env::var(k).ok().and_then(|v| {
        let v = v.trim().to_string();
        match v.strip_prefix("0x") {
            Some(h) => u64::from_str_radix(h, 16).ok(),
            None => v.parse().ok().or_else(|| u64::from_str_radix(&v, 16).ok()),
        }
    })
}

fn hard() -> bool {
    std::env::var("SLICEDIFF_HARD").is_ok()
}

/// Returns the differing (whole, sliced) snapshots, or None when they agree
/// or the case is not comparable.
fn diff(a: &mut Buffers, b: &mut Buffers, c: &Case, total: i32) -> Option<(Vec<u32>, Vec<u32>)> {
    // Retained translations bake in the memory map's region pointers, so an
    // engine may only ever be re-used against the buffers it compiled for.
    // SLICEDIFF_HARD=1 also drops the content-keyed translation cache, which
    // separates "the two run paths differ" from "a retained translation was
    // reused for code it no longer matches".
    if hard() {
        a.jit.invalidate_cache();
        b.jit.invalidate_cache();
    } else {
        a.jit.invalidate_blocks();
        b.jit.invalidate_blocks();
    }
    let whole = a.run(c, total, total)?;
    let sliced = b.run(c, 1, total)?;
    // A block runs to completion, so a big budget legitimately overshoots
    // further than a small one and the two can simply stop in different
    // places. Only an identical pc and cycle_count means both stopped at the
    // same architectural point - then any other difference is a divergence.
    if whole[0] != sliced[0] || whole[1] != sliced[1] {
        return None;
    }
    if whole == sliced {
        None
    } else {
        Some((whole, sliced))
    }
}

fn report(c: &Case, whole: &[u32], sliced: &[u32]) {
    println!("  program:");
    for (i, w) in c.p.iter().enumerate() {
        if *w != 0 || i < 4 {
            println!("    P:{:02x} {:06x}", i, w);
        }
    }
    for (i, (va, vb)) in whole.iter().zip(sliced.iter()).enumerate() {
        if va != vb {
            println!("    {:<14} whole={:06x} sliced={:06x}", label(i), va, vb);
        }
    }
}

/// Shrink a failing case: shorten the program, blank body words, and zero
/// registers, keeping any change that preserves the mismatch.
fn minimize(a: &mut Buffers, b: &mut Buffers, c: &Case, total: i32) -> Case {
    let mut best = c.clone();
    let mut progress = true;
    while progress {
        progress = false;
        // Replacements must be monotone or the search oscillates: a word
        // only ever becomes a nop, never anything else.
        for i in 2..PRAM {
            if best.p[i] == 0 {
                continue;
            }
            let mut t = best.clone();
            t.p[i] = 0;
            if diff(a, b, &t, total).is_some() {
                best = t;
                progress = true;
            }
        }
        for i in 0..reg::COUNT {
            if best.regs[i] == 0
                || i == reg::SR
                || i == reg::OMR
                || (reg::M0..=reg::M7).contains(&i)
            {
                continue;
            }
            let mut t = best.clone();
            t.regs[i] = 0;
            if diff(a, b, &t, total).is_some() {
                best = t;
                progress = true;
            }
        }
        for i in 0..XRAM {
            if best.x[i] != 0 {
                let mut t = best.clone();
                t.x[i] = 0;
                if diff(a, b, &t, total).is_some() {
                    best = t;
                    progress = true;
                }
            }
        }
        for i in 0..YRAM {
            if best.y[i] != 0 {
                let mut t = best.clone();
                t.y[i] = 0;
                if diff(a, b, &t, total).is_some() {
                    best = t;
                    progress = true;
                }
            }
        }
    }
    best
}

/// Smallest budget at which the two runs disagree, so the divergence can be
/// read off a short trace instead of 300 cycles of aftermath.
fn first_diverging_total(a: &mut Buffers, b: &mut Buffers, c: &Case, max: i32) -> Option<i32> {
    (1..=max).find(|&t| diff(a, b, c, t).is_some())
}

#[test]
#[ignore = "randomized search; run explicitly with --ignored"]
fn slice_differential() {
    let iters = env_u64("SLICEDIFF_ITERS").unwrap_or(DEFAULT_ITERS);
    let seed0 = env_u64("SLICEDIFF_SEED").unwrap_or(0x2026_0827);
    let total = env_u64("SLICEDIFF_TOTAL").unwrap_or(DEFAULT_TOTAL as u64) as i32;
    let mut a = Buffers::new();
    let mut b = Buffers::new();

    if let Some(seed) = env_u64("SLICEDIFF_CASE") {
        let c = gen_case(&mut Rng(seed));
        let Some(t) = first_diverging_total(&mut a, &mut b, &c, total) else {
            panic!("seed {:#x} does not diverge within {} cycles", seed, total);
        };
        println!(
            "seed {:#x} first diverges at a budget of {} cycles",
            seed, t
        );
        let (w, s) = diff(&mut a, &mut b, &c, t).unwrap();
        report(&c, &w, &s);
        let m = minimize(&mut a, &mut b, &c, t);
        let (w, s) = diff(&mut a, &mut b, &m, t).unwrap();
        println!("minimized:");
        report(&m, &w, &s);
        let nz: Vec<String> = (0..reg::COUNT)
            .filter(|&i| m.regs[i] != 0)
            .map(|i| format!("{}={:06x}", reg_name(i), m.regs[i]))
            .collect();
        println!("  regs: {}", nz.join(" "));
        for (i, v) in m.x.iter().enumerate() {
            if *v != 0 {
                println!("  X:{:03x} = {:06x}", i, v);
            }
        }
        for (i, v) in m.y.iter().enumerate() {
            if *v != 0 {
                println!("  Y:{:03x} = {:06x}", i, v);
            }
        }
        return;
    }

    let mut rng = Rng(seed0);
    let (mut checked, mut fails, mut aligned) = (0u64, 0u64, 0u64);
    let mut seeds = Vec::new();
    for _ in 0..iters {
        let seed = rng.next() | 1;
        let c = gen_case(&mut Rng(seed));
        checked += 1;
        if let Some((w, s)) = diff(&mut a, &mut b, &c, total) {
            fails += 1;
            // Equal pc and cycle_count means both runs stopped at the same
            // architectural point, so every other difference is a divergence
            // rather than an artifact of where the budget ran out.
            aligned += 1;
            seeds.push(seed);
            if aligned <= 3 {
                println!("MISMATCH seed={:#x}", seed);
                report(&c, &w, &s);
            }
        }
    }
    println!(
        "checked {}, mismatches {} ({} at an identical pc and cycle_count)",
        checked, fails, aligned
    );
    println!("seeds: {:x?}", &seeds[..seeds.len().min(20)]);
    assert_eq!(fails, 0, "sliced run() diverged from a single run()");
}

/// Loop-path differential: the same program, run three ways, must land in the
/// same state.
///
/// A DO loop can execute by three routes through the translator - inlined
/// into a block, preempted at an iteration boundary and resumed through the
/// block-boundary path, or one instruction at a time - and they are supposed
/// to be the same machine. The two block routes are compared directly, by
/// two engines that disagree about the quantum, since with a fixed quantum
/// the schedule cannot pick the route. `execute_one` is the third leg and
/// the reference: it is
/// what the silicon goldens are captured against, so when it disagrees with
/// either block path, the block path is the one that is wrong.
///
///   cargo test --release --features tunable-loop-quantum \
///     --test slicediff -- --ignored --nocapture loop_paths
#[cfg(feature = "tunable-loop-quantum")]
mod loop_paths {
    use super::*;

    /// Bounds a runaway loop without ever firing for these programs.
    /// Large enough that no loop a 300-cycle budget can care about ever
    /// preempts (the inline route is preserved for every comparable
    /// case), and small enough to bound one dispatch's overshoot: a
    /// case's own code can rewrite a register a nested DO reads its
    /// count from, and an unbounded dispatch would run such a nest's
    /// ~10^12 iterations to completion. The legs may then stop at
    /// different architectural points, which the comparability check
    /// already handles.
    const INLINE: i32 = 1 << 20;
    /// Preempts at every backedge.
    const EVERY: i32 = 1;

    const ENDDO: u32 = 0x00008C;

    /// The three ways to run a program. `Step` is the reference.
    #[derive(Clone, Copy, PartialEq, Eq)]
    enum Path {
        Step,
        Inline,
        Preempted,
    }

    impl Path {
        fn name(self) -> &'static str {
            match self {
                Path::Step => "step",
                Path::Inline => "inline",
                Path::Preempted => "preempted",
            }
        }
    }

    struct Engines {
        step: Buffers,
        inline: Buffers,
        preempted: Buffers,
    }

    impl Engines {
        fn new() -> Self {
            Self {
                step: Buffers::new(),
                inline: Buffers::with_quantum(INLINE),
                preempted: Buffers::with_quantum(EVERY),
            }
        }

        fn reset(&mut self) {
            for b in [&mut self.step, &mut self.inline, &mut self.preempted] {
                if hard() {
                    b.jit.invalidate_cache();
                } else {
                    b.jit.invalidate_blocks();
                }
            }
        }

        /// Run `c` every way. None when any path stopped for a reason that
        /// makes the comparison meaningless (STOP/WAIT, halt request).
        fn run_all(&mut self, c: &Case, total: i32) -> Option<[Vec<u32>; 3]> {
            self.reset();
            let step = self.step.run_stepped(c, total)?;
            let inline = self.inline.run(c, total, total)?;
            let preempted = self.preempted.run(c, total, total)?;
            Some([step, inline, preempted])
        }

        /// Did the loop take the inline path? Only then do the two block
        /// engines run different code, and only then does comparing them test
        /// anything. Read off the dispatch counts: an inlined loop is one
        /// dispatch, a preempted one is a dispatch per iteration.
        fn inlined(&self, before: (u64, u64)) -> bool {
            self.preempted.jit.stats.block_entries - before.1
                > self.inline.jit.stats.block_entries - before.0
        }

        fn entries(&self) -> (u64, u64) {
            (
                self.inline.jit.stats.block_entries,
                self.preempted.jit.stats.block_entries,
            )
        }
    }

    /// Compare two runs. A block runs to completion, so paths legitimately
    /// overshoot the budget by different amounts and can simply stop in
    /// different places; only an identical pc and cycle_count means both
    /// stopped at the same architectural point, and then any other difference
    /// is a divergence.
    fn disagree(a: &[u32], b: &[u32]) -> bool {
        a[0] == b[0] && a[1] == b[1] && a != b
    }

    /// The pairs that disagree, reference first.
    fn verdict(snaps: &[Vec<u32>; 3]) -> Vec<(Path, Path)> {
        let mut out = Vec::new();
        for (i, j) in [
            (Path::Step, Path::Inline),
            (Path::Step, Path::Preempted),
            (Path::Inline, Path::Preempted),
        ] {
            let (x, y) = (&snaps[i as usize], &snaps[j as usize]);
            if disagree(x, y) {
                out.push((i, j));
            }
        }
        out
    }

    /// Does the program REP an ENDDO anywhere? The manual does not allow
    /// ENDDO as a REP target (nor any other instruction that touches the loop
    /// stack), so its behaviour on an already-underflowed stack is not
    /// defined and no path is wrong; skip it rather than let an undefined
    /// shape hold the invariant hostage. Only the REP forms this
    /// generator and `POOL` can produce are recognised.
    fn reps_an_enddo(c: &Case) -> bool {
        c.p.windows(2).any(|w| {
            let rep_imm = (w[0] & 0xFF00FF) == 0x0600A0;
            let rep_reg = (w[0] & 0xFFC0FF) == 0x06C020;
            let rep_mem = (w[0] & 0xFFC0FF) == 0x060020; // rep x:aa
            (rep_imm || rep_reg || rep_mem) && w[1] == ENDDO
        })
    }

    /// Words the translator will inline as a DO body, found by asking it:
    /// wrap each candidate in a short loop and keep the ones where the
    /// preempting engine dispatches more blocks than the inlining one. Random
    /// words are overwhelmingly branches, block terminators or multi-word
    /// instructions that fail to tile the body, so an unbiased generator
    /// produces a search that silently tests nothing.
    fn body_pool(e: &mut Engines, want: usize) -> Vec<u32> {
        let mut pool: Vec<u32> = Vec::new();
        let mut rng = Rng(0x_b0d1_7001);
        let mut tries = 0u64;
        while pool.len() < want && tries < 4_000_000 {
            tries += 1;
            let w = if rng.n(3) == 0 {
                POOL[rng.n(POOL.len() as u32) as usize]
            } else {
                rng.n(0x1000000)
            };
            let mut c = gen_case(&mut Rng(rng.next() | 1));
            c.p = vec![0u32; PRAM];
            c.p[0] = 0x060680; // do #6,$0003
            c.p[1] = 0x000003;
            c.p[2] = w;
            let before = e.entries();
            e.reset();
            if e.inline.run(&c, 100, 100).is_none() || e.preempted.run(&c, 100, 100).is_none() {
                continue;
            }
            if e.inlined(before) && !pool.contains(&w) {
                pool.push(w);
            }
        }
        pool
    }

    /// Build a DO body out of items rather than words: a plain instruction, a
    /// REP of one, or a nested loop. Nesting recurses, so three levels deep
    /// happens on its own. Loop counts held in a register or in memory are
    /// seeded small - a 24-bit runtime count would run for millions of cycles
    /// inside one dispatch and never land the two engines on a comparable
    /// stopping point.
    fn gen_body(
        rng: &mut Rng,
        pool: &[u32],
        regs: &mut [u32; reg::COUNT],
        x: &mut [u32],
        at: usize,
        depth: u32,
    ) -> Vec<u32> {
        let word = |rng: &mut Rng| pool[rng.n(pool.len() as u32) as usize];
        let mut body: Vec<u32> = Vec::new();
        let want = 1 + rng.n(if depth == 0 { 6 } else { 3 }) as usize;
        while body.len() < want {
            match rng.n(if depth < 2 { 14 } else { 10 }) {
                0..=5 => body.push(word(rng)),
                6..=7 => {
                    // rep #n, <instruction>
                    body.push(0x0600A0 | ((1 + rng.n(4)) << 8));
                    body.push(word(rng));
                }
                8..=9 if depth < 2 => {
                    // enddo, only where the notes say it behaves: LA-3 or
                    // earlier. Padded so it cannot drift to the end.
                    body.push(ENDDO);
                    body.push(0);
                    body.push(0);
                    body.push(0);
                }
                10..=11 => {
                    // A nested loop, counted three ways.
                    let head_at = at + body.len();
                    let head: Vec<u32> = match rng.n(3) {
                        0 => vec![0x060080 | ((1 + rng.n(4)) << 8)],
                        1 => {
                            // do x0,LA
                            regs[reg::X0] = 1 + rng.n(4);
                            vec![0x06C400]
                        }
                        _ => {
                            // do x:$10,LA
                            x[0x10] = 1 + rng.n(4);
                            vec![0x061000 | 0x10]
                        }
                    };
                    let inner = gen_body(rng, pool, regs, x, head_at + 2, depth + 1);
                    // LA is the address of the inner body's last word, and
                    // must land strictly inside the enclosing body.
                    let la = head_at + 2 + inner.len() - 1;
                    body.extend(head);
                    body.push(la as u32);
                    body.extend(inner);
                    body.push(0);
                }
                _ => {
                    // A forward conditional skip over 1-3 body words
                    // (`forward_skip_target`): register-bit forms whose
                    // predicate never reads SR, and a cc form that does.
                    // A trailing pool word keeps the merge strictly
                    // inside the body most of the time (a skip whose
                    // target lands at LA+1 legitimately falls back to
                    // the non-inlined path).
                    let n = 1 + rng.n(3);
                    let op = match rng.n(3) {
                        0 => 0x0CC581, // brclr #1,x1,<fwd>
                        1 => 0x0CC6A0, // brset #0,y0,<fwd>
                        _ => 0x0D1049, // blt <fwd>
                    };
                    body.push(op);
                    body.push(2 + n);
                    for _ in 0..n {
                        body.push(word(rng));
                    }
                    body.push(word(rng));
                }
            }
        }
        body
    }

    /// A case whose DO body inlines, so the two block engines disagree about
    /// it. Everything after the loop is as random as in the
    /// slice-differential generator.
    fn gen_loop_case(rng: &mut Rng, pool: &[u32]) -> Case {
        let mut c = gen_case(rng);
        let body = gen_body(rng, pool, &mut c.regs, &mut c.x, 2, 0);
        c.p[0] = 0x060080 | ((1 + rng.n(6)) << 8);
        c.p[1] = (1 + body.len()) as u32;
        for (i, w) in body.iter().enumerate() {
            if 2 + i < PRAM {
                c.p[2 + i] = *w;
            }
        }
        c
    }

    fn report3(c: &Case, snaps: &[Vec<u32>; 3], pairs: &[(Path, Path)]) {
        println!("  program:");
        for (i, w) in c.p.iter().enumerate() {
            if *w != 0 || i < 4 {
                println!("    P:{:02x} {:06x}", i, w);
            }
        }
        for (a, b) in pairs {
            println!("  {} vs {}:", a.name(), b.name());
            let (x, y) = (&snaps[*a as usize], &snaps[*b as usize]);
            for (i, (va, vb)) in x.iter().zip(y.iter()).enumerate() {
                if va != vb {
                    println!(
                        "    {:<14} {}={:06x} {}={:06x}",
                        label(i),
                        a.name(),
                        va,
                        b.name(),
                        vb
                    );
                }
            }
        }
    }

    /// Shrink a failing case, keeping any change that preserves a
    /// disagreement. Replacements are monotone - a word only ever becomes a
    /// nop - or the search trades two candidates forever.
    fn minimize(e: &mut Engines, c: &Case, total: i32) -> Case {
        let fails = |e: &mut Engines, t: &Case| {
            e.run_all(t, total).is_some_and(|s| !verdict(&s).is_empty())
        };
        let mut best = c.clone();
        let mut progress = true;
        while progress {
            progress = false;
            for i in 2..PRAM {
                if best.p[i] == 0 {
                    continue;
                }
                let mut t = best.clone();
                t.p[i] = 0;
                if fails(e, &t) {
                    best = t;
                    progress = true;
                }
            }
            for i in 0..reg::COUNT {
                if best.regs[i] == 0
                    || i == reg::SR
                    || i == reg::OMR
                    || (reg::M0..=reg::M7).contains(&i)
                {
                    continue;
                }
                let mut t = best.clone();
                t.regs[i] = 0;
                if fails(e, &t) {
                    best = t;
                    progress = true;
                }
            }
            for i in 0..XRAM {
                if best.x[i] != 0 {
                    let mut t = best.clone();
                    t.x[i] = 0;
                    if fails(e, &t) {
                        best = t;
                        progress = true;
                    }
                }
            }
            for i in 0..YRAM {
                if best.y[i] != 0 {
                    let mut t = best.clone();
                    t.y[i] = 0;
                    if fails(e, &t) {
                        best = t;
                        progress = true;
                    }
                }
            }
        }
        best
    }

    fn dump(c: &Case) {
        let nz: Vec<String> = (0..reg::COUNT)
            .filter(|&i| c.regs[i] != 0)
            .map(|i| format!("{}={:06x}", reg_name(i), c.regs[i]))
            .collect();
        println!("  regs: {}", nz.join(" "));
        for (i, v) in c.x.iter().enumerate() {
            if *v != 0 {
                println!("  X:{:03x} = {:06x}", i, v);
            }
        }
        for (i, v) in c.y.iter().enumerate() {
            if *v != 0 {
                println!("  Y:{:03x} = {:06x}", i, v);
            }
        }
    }

    #[test]
    #[ignore = "randomized search; run explicitly with --ignored"]
    fn all_three_paths_agree() {
        let iters = env_u64("SLICEDIFF_ITERS").unwrap_or(DEFAULT_ITERS);
        let seed0 = env_u64("SLICEDIFF_SEED").unwrap_or(0x2026_0827);
        let total = env_u64("SLICEDIFF_TOTAL").unwrap_or(DEFAULT_TOTAL as u64) as i32;
        let mut e = Engines::new();
        // SLICEDIFF_POOL_FILE caches the (deterministic) body pool across
        // runs - body_pool costs ~a minute and dominates single-case
        // repro iteration.
        let pool = match std::env::var("SLICEDIFF_POOL_FILE").ok() {
            Some(path) => match std::fs::read_to_string(&path) {
                Ok(text) => text
                    .split_whitespace()
                    .filter_map(|w| u32::from_str_radix(w, 16).ok())
                    .collect(),
                Err(_) => {
                    let pool = body_pool(&mut e, 256);
                    let text: Vec<String> = pool.iter().map(|w| format!("{w:06x}")).collect();
                    let _ = std::fs::write(&path, text.join("\n"));
                    pool
                }
            },
            None => body_pool(&mut e, 256),
        };
        println!("body pool: {} words", pool.len());

        if let Some(seed) = env_u64("SLICEDIFF_CASE") {
            let c = gen_loop_case(&mut Rng(seed), &pool);
            if let Some(t1) = env_u64("SLICEDIFF_ONE") {
                let t0 = std::time::Instant::now();
                let r = e.run_all(&c, t1 as i32);
                eprintln!(
                    "ONE: t={} comparable={} elapsed={:?}",
                    t1,
                    r.is_some(),
                    t0.elapsed()
                );
                return;
            }
            if std::env::var_os("SLICEDIFF_DUMP").is_some() {
                for (i, w) in c.p.iter().enumerate() {
                    if *w != 0 {
                        eprintln!("P {:04x} {:06x}", i, w);
                    }
                }
                dump(&c);
                return;
            }
            let Some(t) =
                (1..=total).find(|&t| e.run_all(&c, t).is_some_and(|s| !verdict(&s).is_empty()))
            else {
                panic!("seed {:#x} does not diverge within {} cycles", seed, total);
            };
            println!(
                "seed {:#x} first diverges at a budget of {} cycles",
                seed, t
            );
            let snaps = e.run_all(&c, t).unwrap();
            report3(&c, &snaps, &verdict(&snaps));
            let m = minimize(&mut e, &c, t);
            let snaps = e.run_all(&m, t).unwrap();
            println!("minimized:");
            report3(&m, &snaps, &verdict(&snaps));
            dump(&m);
            return;
        }

        let mut rng = Rng(seed0);
        // SLICEDIFF_SKIP fast-forwards the seed sequence without running
        // the cases (engine state differs from a full run - a
        // state-dependent repro still needs the sequential prefix);
        // SLICEDIFF_PROGRESS traces each case's index and seed to stderr,
        // so a case that wedges an engine is the line after the last one
        // a run printed.
        let skip = env_u64("SLICEDIFF_SKIP").unwrap_or(0);
        for _ in 0..skip {
            rng.next();
        }
        let progress = std::env::var_os("SLICEDIFF_PROGRESS").is_some();
        let (mut checked, mut inlined, mut comparable) = (0u64, 0u64, 0u64);
        let (mut skipped, mut fails) = (0u64, 0u64);
        let mut by_pair = [0u64; 3];
        let mut seeds = Vec::new();
        for i in 0..iters {
            let seed = rng.next() | 1;
            if progress {
                eprintln!("case {} seed {:#x}", skip + i, seed);
            }
            let c = gen_loop_case(&mut Rng(seed), &pool);
            if reps_an_enddo(&c) {
                skipped += 1;
                continue;
            }
            checked += 1;
            let before = e.entries();
            let Some(snaps) = e.run_all(&c, total) else {
                continue;
            };
            if e.inlined(before) {
                inlined += 1;
            }
            if snaps[0][0] == snaps[1][0] && snaps[0][1] == snaps[1][1] {
                comparable += 1;
            }
            let pairs = verdict(&snaps);
            if pairs.is_empty() {
                continue;
            }
            fails += 1;
            seeds.push(seed);
            for (i, p) in [
                (Path::Step, Path::Inline),
                (Path::Step, Path::Preempted),
                (Path::Inline, Path::Preempted),
            ]
            .iter()
            .enumerate()
            {
                if pairs.contains(p) {
                    by_pair[i] += 1;
                }
            }
            if fails <= 3 {
                println!("MISMATCH seed={:#x}", seed);
                report3(&c, &snaps, &pairs);
            }
        }
        println!(
            "checked {}, inlined {}, comparable {}, mismatches {} \
             (step/inline {}, step/preempted {}, inline/preempted {}); \
             {} skipped for REP ENDDO",
            checked, inlined, comparable, fails, by_pair[0], by_pair[1], by_pair[2], skipped
        );
        println!("seeds: {:x?}", &seeds[..seeds.len().min(20)]);
        // A zero-mismatch result only means something if the cases reached
        // the inline path and stopped somewhere the paths could be compared.
        assert!(
            inlined > checked / 8,
            "too few cases took the inline path to be a search"
        );
        assert!(
            comparable > checked / 10,
            "too few cases stopped at the same architectural point to be a search"
        );
        assert_eq!(fails, 0, "the loop paths diverged");
    }
}

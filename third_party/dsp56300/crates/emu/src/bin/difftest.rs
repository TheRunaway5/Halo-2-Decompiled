//! Differential-test runner: executes corpus cases (or a whole-state
//! snapshot) on the JIT emulator and prints canonical state dumps for
//! comparison against an oracle (real hardware, or Motorola sim56300).
//!
//! Corpus mode:
//!   difftest corpus <corpus.lod> <corpus.meta>
//! `corpus.lod` is the repo's simplified LOD format (`<SPACE> <ADDR> <WORD>`
//! per line). `corpus.meta` lines: `case <name> <start_hex> <end_hex>
//! <max_steps>`. Each case runs from `start` until pc == `end` (or the step
//! cap), then a dump is printed.
//!
//! Snapshot mode:
//!   difftest run-to <snapshot.lod> <step|block> <max_cycles> <stop_pc>
//! Loads a whole-state snapshot in the LOD dialect an embedder writes and
//! runs it until the PC reaches `stop_pc`; see `run_to`.
//!
//! Dump format (one per case):
//! ```text
//!   case <name>
//!   steps <n>
//!   <reg> <hex>   (pc, sr, omr, ..., r0-r7, n0-n7, m0-m7, x0..b2)
//!   cyc <n>       (informational: emulated cycle count)
//!   end
//! ```

use std::fs;

use dsp56300_emu::core::{DspState, MemSpace, MemoryMap, reg};
use dsp56300_emu::jit::JitEngine;

const PRAM_SIZE: usize = 4096;
const XRAM_SIZE: usize = 4096;
const YRAM_SIZE: usize = 2048;

/// (label, register index) in canonical dump order, matching sim56300's
/// register display fields.
const DUMP_REGS: &[(&str, usize)] = &[
    ("sr", reg::SR),
    ("omr", reg::OMR),
    ("la", reg::LA),
    ("lc", reg::LC),
    ("sp", reg::SP),
    ("ssh", reg::SSH),
    ("ssl", reg::SSL),
    ("ep", reg::EP),
    ("sz", reg::SZ),
    ("sc", reg::SC),
    ("vba", reg::VBA),
    ("x0", reg::X0),
    ("x1", reg::X1),
    ("y0", reg::Y0),
    ("y1", reg::Y1),
    ("a0", reg::A0),
    ("a1", reg::A1),
    ("a2", reg::A2),
    ("b0", reg::B0),
    ("b1", reg::B1),
    ("b2", reg::B2),
    ("r0", reg::R0),
    ("r1", reg::R1),
    ("r2", reg::R2),
    ("r3", reg::R3),
    ("r4", reg::R4),
    ("r5", reg::R5),
    ("r6", reg::R6),
    ("r7", reg::R7),
    ("n0", reg::N0),
    ("n1", reg::N1),
    ("n2", reg::N2),
    ("n3", reg::N3),
    ("n4", reg::N4),
    ("n5", reg::N5),
    ("n6", reg::N6),
    ("n7", reg::N7),
    ("m0", reg::M0),
    ("m1", reg::M1),
    ("m2", reg::M2),
    ("m3", reg::M3),
    ("m4", reg::M4),
    ("m5", reg::M5),
    ("m6", reg::M6),
    ("m7", reg::M7),
];

/// Dirty-state init constants, parsed from the meta's `fill` header line
/// (`fill kx cx ky cy sshb sslb`, all 6-hex). When present, X:$0000-$0BFF
/// and Y:$0000-$07FF are pre-filled with `(k*addr + c) & $FFFFFF` and the
/// 15 hardware stack slots are seeded with an sshb/sslb ramp, matching the
/// hardware runner's preamble (which parses the same line). Memory dumps
/// then emit deviations from the fill instead of nonzero words, and print
/// the same `fill` header so diff.py knows each file's missing-key default.
struct Fill {
    kx: u32,
    cx: u32,
    ky: u32,
    cy: u32,
    sshb: u32,
    sslb: u32,
}

fn fill_word(k: u32, c: u32, addr: u32) -> u32 {
    ((k as u64 * addr as u64 + c as u64) & 0xFFFFFF) as u32
}

fn parse_fill(meta: &str) -> Option<Fill> {
    for line in meta.lines() {
        let f: Vec<&str> = line.split_whitespace().collect();
        if f.len() == 7 && f[0] == "fill" {
            let v: Vec<u32> = f[1..]
                .iter()
                .map(|s| u32::from_str_radix(s, 16).unwrap())
                .collect();
            assert!(v[0] & 1 == 1 && v[2] & 1 == 1, "fill K values must be odd");
            assert!(
                v[4] + 14 <= 0xFF && v[5] + 14 <= 0xFF,
                "stack ramp must fit 8-bit movec"
            );
            return Some(Fill {
                kx: v[0],
                cx: v[1],
                ky: v[2],
                cy: v[3],
                sshb: v[4],
                sslb: v[5],
            });
        }
    }
    None
}

/// A dump window: the key prefix, the space, and the address spans dumped.
type MemRange<'a> = (&'a str, MemSpace, &'a [(u32, u32)]);

fn dump(name: &str, steps: u64, s: &DspState, mem: bool, stack: bool, fill: Option<&Fill>) {
    println!("case {name}");
    println!("steps {steps}");
    println!("pc {:06x}", s.pc);
    for (label, idx) in DUMP_REGS {
        println!("{label} {:06x}", s.registers[*idx]);
    }
    println!("cyc {}", s.cycle_count);
    if mem {
        // Memory end-state, deviations only: a word is dumped when it
        // differs from the init value (the affine fill under a `fill`
        // header, zero otherwise), so diff.py's per-file missing-key
        // default makes sparse dumps compare exactly. Ranges mirror the
        // hardware runner's --dump-mem: the sanctioned case-operand
        // windows minus the harness snapshot window (X:$0400-$04FF) and
        // the SE mixbuffer (X:$0C00+); P covers only the movem scratch
        // zone past the snapshot code (P is never filled).
        let ranges: [MemRange; 3] = [
            ("xm", MemSpace::X, &[(0x0000, 0x0400), (0x0500, 0x0C00)]),
            ("ym", MemSpace::Y, &[(0x0000, 0x0800)]),
            ("pm", MemSpace::P, &[(0x07C0, 0x0800)]),
        ];
        for (tag, space, spans) in ranges {
            for &(lo, hi) in spans {
                for addr in lo..hi {
                    let w = s.read_memory(space, addr);
                    let init = match (tag, fill) {
                        ("xm", Some(f)) => fill_word(f.kx, f.cx, addr),
                        ("ym", Some(f)) => fill_word(f.ky, f.cy, addr),
                        _ => 0,
                    };
                    if w != init {
                        println!("{tag}{addr:04x} {w:06x}");
                    }
                }
            }
        }
    }
    if stack {
        // Hardware stack slots 1-15 (sh/sl = SSH/SSL halves), deviation-
        // encoded against the seed (the fill ramp, or zero): mirrors the
        // hardware runner's descending stack-walk epilogue. Slot 0 is not
        // real storage and is never dumped. Separate flag from --dump-mem
        // so dumps stay comparable against goldens captured without
        // slot data.
        for j in 1..16u32 {
            let (ih, il) = match fill {
                Some(f) => ((f.sshb + j - 1) & 0xFFFFFF, (f.sslb + j - 1) & 0xFFFFFF),
                None => (0, 0),
            };
            let h = s.stack[0][j as usize];
            let l = s.stack[1][j as usize];
            if h != ih {
                println!("sh{j:02x} {h:06x}");
            }
            if l != il {
                println!("sl{j:02x} {l:06x}");
            }
        }
    }
    println!("end");
}

fn load_lod(path: &str, xram: &mut [u32], yram: &mut [u32], pram: &mut [u32]) {
    let text = fs::read_to_string(path).unwrap_or_else(|e| panic!("read {path}: {e}"));
    for line in text.lines() {
        let f: Vec<&str> = line.split_whitespace().collect();
        if f.len() != 3 || !matches!(f[0], "P" | "X" | "Y") {
            continue; // skip symbol (I) records and anything else
        }
        let addr = usize::from_str_radix(f[1], 16).unwrap();
        let word = u32::from_str_radix(f[2], 16).unwrap();
        match f[0] {
            "P" => pram[addr] = word,
            "X" => xram[addr] = word,
            "Y" => yram[addr] = word,
            _ => unreachable!(),
        }
    }
}

/// Match sim56300 boot state where our defaults differ.
fn apply_sim_boot_state(s: &mut DspState) {
    s.registers[reg::LA] = 0xFFFFFF;
}

/// Match real MCPX GP silicon boot state (as measured via xbtest): the
/// hardware runner's init preamble forces every other register to our
/// defaults, but leaves OMR (mode pins read $30F) and VBA ($FF0000)
/// untouched because rewriting them on silicon risks remapping memory /
/// interrupt vectors.
fn apply_hw_boot_state(s: &mut DspState) {
    s.registers[reg::LA] = 0xFFFFFF;
    s.registers[reg::OMR] = 0x00030F;
    s.registers[reg::VBA] = 0xFF0000;
}

fn run_corpus(
    lod_path: &str,
    meta_path: &str,
    boot: &str,
    mode: &str,
    dump_mem: bool,
    dump_stack: bool,
) {
    let mut xram = vec![0u32; XRAM_SIZE];
    let mut yram = vec![0u32; YRAM_SIZE];
    let mut pram = vec![0u32; PRAM_SIZE];
    load_lod(lod_path, &mut xram, &mut yram, &mut pram);

    let meta = fs::read_to_string(meta_path).unwrap_or_else(|e| panic!("read {meta_path}: {e}"));
    let fill = parse_fill(&meta);
    if let Some(f) = &fill {
        // Dirty-state init: match the hardware preamble's affine fill of
        // the sanctioned operand windows (never the SE mixbuffer X:$0C00+,
        // which the hardware cannot deterministically fill).
        for a in 0..0xC00u32 {
            xram[a as usize] = fill_word(f.kx, f.cx, a);
        }
        for a in 0..0x800u32 {
            yram[a as usize] = fill_word(f.ky, f.cy, a);
        }
    }

    let mut jit = JitEngine::new(PRAM_SIZE);
    let mut s = DspState::new(MemoryMap::test(&mut xram, &mut yram, &mut pram));
    match boot {
        "sim" => apply_sim_boot_state(&mut s),
        "hw" => apply_hw_boot_state(&mut s),
        _ => panic!("unknown boot state '{boot}' (want sim or hw)"),
    }
    if let Some(f) = &fill {
        // Stack-slot ramp, matching the hardware preamble's 15 pushes:
        // push i (i=0..14) lands in slot i+1. Slot 0 and the mirrored
        // SSH/SSL registers (stack[..][0] at sp=0) stay zero.
        for i in 0..15u32 {
            s.stack[0][(i + 1) as usize] = (f.sshb + i) & 0xFFFFFF;
            s.stack[1][(i + 1) as usize] = (f.sslb + i) & 0xFFFFFF;
        }
        println!(
            "fill {:06x} {:06x} {:06x} {:06x} {:06x} {:06x}",
            f.kx, f.cx, f.ky, f.cy, f.sshb, f.sslb
        );
    }
    for line in meta.lines() {
        let f: Vec<&str> = line.split_whitespace().collect();
        if f.len() != 5 || f[0] != "case" {
            continue;
        }
        let name = f[1];
        let start = u32::from_str_radix(f[2], 16).unwrap();
        let end = u32::from_str_radix(f[3], 16).unwrap();
        let max_steps: u64 = f[4].parse().unwrap();

        s.pc = start;
        let mut steps = 0u64;
        match mode {
            "step" => {
                while s.pc != end && steps < max_steps {
                    s.execute_one(&mut jit);
                    steps += 1;
                }
            }
            "block" => {
                // Drive the basic-block run loop the way an embedder does
                // (dsp56300_run with a cycle budget) instead of stepping.
                // Park the case at its end pc with a self-jmp so the block
                // chain terminates there; jmp alters no compared state.
                // pram writes are read through raw pointers in DspState.
                let saved = pram[end as usize];
                pram[end as usize] = 0x0C0000 | end;
                s.pram_dirty.mark_dirty(end);
                // DIFFTEST_SLICE overrides the per-call cycle budget, so a
                // bank can be swept across slice sizes. An inline DO loop is
                // preempted mid-body and resumed through the block-boundary
                // path, so a sweep says whether both routes match silicon.
                let slice: i32 = std::env::var("DIFFTEST_SLICE")
                    .ok()
                    .and_then(|v| v.parse().ok())
                    .unwrap_or(256);
                while s.pc != end && steps < max_steps {
                    s.run(&mut jit, slice);
                    steps += 1;
                }
                pram[end as usize] = saved;
                s.pram_dirty.mark_dirty(end);
            }
            _ => panic!("unknown mode '{mode}' (want step or block)"),
        }
        dump(name, steps, &s, dump_mem, dump_stack, fill.as_ref());
    }
}

/// `difftest run-to <snapshot.lod> <step|block> <max_cycles> <stop_pc>`: load
/// a whole-state snapshot - the LOD dialect an embedder writes: `P|X|Y
/// <addr> <word>` memory records plus `R <idx>
/// <word>` registers, `SSH|SSL <slot> <word>` stack slots and `PC <word>` -
/// and run it until the PC reaches `stop_pc` (step mode checks after every
/// instruction; block mode, the embedder's path, between run() slices of
/// 1000 cycles, so it may overshoot into the spin loop the stop PC is in) or
/// the cycle cap. Then print the final PC, cycles, registers, X:$0000-$011F
/// and Y:$0000-$00FF as `name value` lines, the format the hardware runner
/// emits, so the two diff directly.
fn run_to(path: &str, mode: &str, max_cycles: u64, stop_pc: u32) {
    const SIZE: usize = 0x1000;
    let mut pram = vec![0u32; SIZE];
    let mut xram = vec![0u32; SIZE];
    let mut yram = vec![0u32; SIZE];
    let text = fs::read_to_string(path).unwrap_or_else(|e| panic!("read {path}: {e}"));
    let mut regs: [Option<u32>; 64] = [None; 64];
    let mut stack: [[Option<u32>; 16]; 2] = [[None; 16]; 2];
    let mut pc = None;
    for line in text.lines() {
        let f: Vec<&str> = line.split_whitespace().collect();
        if f.len() < 2 || f[0].starts_with(';') {
            continue;
        }
        if f[0] == "PC" {
            pc = u32::from_str_radix(f[1], 16).ok();
            continue;
        }
        if f.len() < 3 {
            continue;
        }
        let (Ok(a), Ok(v)) = (
            usize::from_str_radix(f[1], 16),
            u32::from_str_radix(f[2], 16),
        ) else {
            continue;
        };
        match f[0] {
            "P" if a < SIZE => pram[a] = v,
            "X" if a < SIZE => xram[a] = v,
            "Y" if a < SIZE => yram[a] = v,
            "R" if a < 64 => regs[a] = Some(v),
            "SSH" if a < 16 => stack[0][a] = Some(v),
            "SSL" if a < 16 => stack[1][a] = Some(v),
            _ => {}
        }
    }
    let mut jit = JitEngine::new(SIZE);
    let mut s = DspState::new(MemoryMap::test(&mut xram, &mut yram, &mut pram));
    for (i, r) in regs.iter().enumerate() {
        if let Some(v) = r {
            s.registers[i] = *v;
        }
    }
    for (h, half) in stack.iter().enumerate() {
        for (i, v) in half.iter().enumerate() {
            if let Some(v) = v {
                s.stack[h][i] = *v;
            }
        }
    }
    if let Some(p) = pc {
        s.pc = p;
    }
    let mut cycles: u64 = 0;
    let mut steps: u64 = 0;
    if mode == "step" {
        // DIFFTEST_TRACE_PCS=<path>: one line per executed instruction,
        // `pc opcode next sp`, for coverage/depth questions about a phase.
        let mut trace = std::env::var("DIFFTEST_TRACE_PCS")
            .ok()
            .map(|p| std::io::BufWriter::new(fs::File::create(p).unwrap()));
        while cycles < max_cycles && s.pc != stop_pc {
            if let Some(t) = trace.as_mut() {
                use std::io::Write;
                let op = s.read_memory(dsp56300_emu::core::MemSpace::P, s.pc);
                let nw = s.read_memory(dsp56300_emu::core::MemSpace::P, s.pc + 1);
                writeln!(
                    t,
                    "{:04x} {:06x} {:06x} {:02x}",
                    s.pc,
                    op,
                    nw,
                    s.registers[reg::SP]
                )
                .unwrap();
            }
            cycles += s.execute_one(&mut jit) as u64;
            steps += 1;
        }
    } else {
        while cycles < max_cycles && s.pc != stop_pc {
            s.cycle_count = 0;
            s.power_state = dsp56300_emu::core::PowerState::Normal;
            s.run(&mut jit, 1000);
            cycles += s.cycle_count as u64;
        }
    }
    println!("mode {mode}");
    println!("pc {:06x}", s.pc);
    println!("cycles {cycles}");
    println!("steps {steps}");
    for i in 0..64 {
        println!("reg[{i:02x}] {:06x}", s.registers[i]);
    }
    let n = if std::env::var("DIFFTEST_PHASE_ALL_X").is_ok() {
        0x1000
    } else {
        0x120
    };
    for a in 0..n {
        println!(
            "x[{a:04x}] {:06x}",
            s.read_memory(dsp56300_emu::core::MemSpace::X, a)
        );
    }
    for a in 0..0x100 {
        println!(
            "y[{a:04x}] {:06x}",
            s.read_memory(dsp56300_emu::core::MemSpace::Y, a)
        );
    }
}

/// Static coverage over corpus banks: walk every case's instruction range,
/// decode each word, and report which OPCODE_TABLE entries, parallel-ALU
/// opcode bytes, and parallel-move classes are exercised vs. the full sets.
fn run_coverage(pairs: &[(String, String)]) {
    use dsp56300_core::decode;
    use std::collections::BTreeSet;

    let mut pram = vec![0u32; PRAM_SIZE];
    let mut entries_hit: BTreeSet<&'static str> = BTreeSet::new();
    let mut alu_hit: BTreeSet<u8> = BTreeSet::new();
    let mut pmove_hit: BTreeSet<String> = BTreeSet::new();
    let mut unknown_words: BTreeSet<u32> = BTreeSet::new();

    for (lod, meta) in pairs {
        for w in pram.iter_mut() {
            *w = 0;
        }
        let mut xram = vec![0u32; XRAM_SIZE];
        let mut yram = vec![0u32; YRAM_SIZE];
        load_lod(lod, &mut xram, &mut yram, &mut pram);
        let meta_txt = fs::read_to_string(meta).unwrap_or_else(|e| panic!("read {meta}: {e}"));
        for line in meta_txt.lines() {
            let f: Vec<&str> = line.split_whitespace().collect();
            if f.len() != 5 || f[0] != "case" {
                continue;
            }
            let start = u32::from_str_radix(f[2], 16).unwrap();
            let end = u32::from_str_radix(f[3], 16).unwrap();
            let mut pc = start;
            while pc < end {
                let opcode = pram[pc as usize];
                let inst = decode::decode(opcode);
                if let dsp56300_core::Instruction::Parallel { alu, .. } = &inst {
                    let _ = alu;
                    alu_hit.insert((opcode & 0xFF) as u8);
                    pmove_hit.insert(classify_pmove(opcode));
                } else if let Some(name) = decode::decode_entry_name(opcode) {
                    entries_hit.insert(name);
                } else {
                    unknown_words.insert(opcode);
                }
                pc += dsp56300_core::decode::instruction_length(&inst);
            }
        }
    }

    // Full sets
    let all_entries: Vec<&'static str> = decode::opcode_table_names().collect();
    let mut valid_alu: Vec<(u8, String)> = Vec::new();
    for b in 0..=255u8 {
        let alu = dsp56300_core::decode_parallel_alu(b);
        let label = format!("{alu}");
        if label != "undefined" {
            valid_alu.push((b, label));
        }
    }

    // Entries deliberately not exercisable on real hardware. Peripheral
    // (pp/qq/movep) forms touch live APU registers and produce
    // nondeterministic dumps; the cache ops, DEBUG, and ENDDO-outside-loop
    // wedge MCPX silicon (probed); the interrupt/power group
    // needs vector/host infrastructure the corpus cannot provide.
    fn excluded(name: &str) -> Option<&'static str> {
        if name.contains(":pp")
            || name.contains(":qq")
            || name.contains("X:qq")
            || name.contains("Y:qq")
            || name.starts_with("movep")
        {
            return Some("peripheral space");
        }
        match name {
            "pflush" | "pflushun" | "pfree" | "plock ea" | "plockr xxxx" | "punlock ea"
            | "punlockr xxxx" => Some("cache op: wedges MCPX"),
            "debug" => Some("halts core (OnCE)"),
            "illegal" | "trap" | "trapcc" => Some("interrupt vector"),
            "reset" | "stop" | "wait" => Some("power/peripheral state"),
            _ => None,
        }
    }
    let mut missing = 0;
    let mut excluded_n = 0;
    for name in &all_entries {
        if !entries_hit.contains(name) {
            if let Some(why) = excluded(name) {
                excluded_n += 1;
                println!("excluded entry ({why}): {name}");
            } else {
                missing += 1;
                println!("MISSING entry: {name}");
            }
        }
    }
    println!(
        "== opcode table entries: {}/{} covered, {} excluded, {} MISSING ==",
        entries_hit.len(),
        all_entries.len(),
        excluded_n,
        missing
    );
    println!(
        "== parallel ALU bytes: {}/{} covered ==",
        valid_alu
            .iter()
            .filter(|(b, _)| alu_hit.contains(b))
            .count(),
        valid_alu.len()
    );
    for (b, label) in &valid_alu {
        if !alu_hit.contains(b) {
            println!("MISSING alu ${b:02x}: {label}");
        }
    }
    println!("== parallel move classes covered ==");
    for c in &pmove_hit {
        println!("pmove: {c}");
    }
    for w in &unknown_words {
        println!("WARNING: unknown/unnamed word ${w:06x} in corpus");
    }
}

/// Coarse parallel-move class fingerprint for coverage reporting.
fn classify_pmove(op: u32) -> String {
    if (op & 0xFE4000) == 0x080000 {
        let space = if op & (1 << 15) != 0 { "y" } else { "x" };
        return format!("pm0.{space}");
    }
    let move_bits = (op >> 20) & 0xF;
    match move_bits {
        1 => {
            let side = if op & (1 << 14) != 0 { "y" } else { "x" };
            let w = if op & (1 << 15) != 0 { "r" } else { "w" };
            let imm = if (op >> 8) & 0x3F == 0x34 && op & (1 << 15) != 0 {
                ".imm"
            } else {
                ""
            };
            format!("pm1.{side}.{w}{imm}")
        }
        2 | 3 => {
            let upper = (op >> 8) & 0xFFFF;
            if upper == 0x2000 {
                "pm2.nop".into()
            } else if upper & 0xFFF0 == 0x2020 {
                "pm2.ifcc".into()
            } else if upper & 0xFFF0 == 0x2030 {
                "pm2.ifcc_u".into()
            } else if upper & 0xFFE0 == 0x2040 {
                "pm2.update".into()
            } else if upper & 0xFC00 == 0x2000 {
                "pm2.r2r".into()
            } else {
                "pm3.imm_short".into()
            }
        }
        4..=7 => {
            let value = ((op >> 16) & 7) | ((op >> 17) & 0x18);
            let l_space = value >> 2 == 0;
            let sp = if l_space {
                "l"
            } else if op & (1 << 19) != 0 {
                "y"
            } else {
                "x"
            };
            let w = if op & (1 << 15) != 0 { "r" } else { "w" };
            if op & (1 << 14) != 0 {
                let mmm = (op >> 11) & 7;
                let rrr = (op >> 8) & 7;
                let form = if mmm == 6 && rrr == 4 {
                    "imm"
                } else if mmm == 6 {
                    "abs"
                } else {
                    "ea"
                };
                format!("pm4.{sp}.{w}.{form}")
            } else {
                format!("pm4.{sp}.{w}.aa")
            }
        }
        _ => {
            // Ground truth: $F09800 = `move x:(r0)+,x0 y:(r4)+,y0` (both
            // reads) has bit 15 and bit 22 set - set bit means read on
            // both sides.
            let wx = if op & (1 << 15) != 0 { "r" } else { "w" };
            let wy = if op & (1 << 22) != 0 { "r" } else { "w" };
            format!("pm8.x{wx}.y{wy}")
        }
    }
}

fn main() {
    let args: Vec<String> = std::env::args().collect();
    match args.get(1).map(String::as_str) {
        Some("corpus") if (4..=8).contains(&args.len()) => {
            let mut boot = "sim";
            let mut mode = "step";
            let mut dump_mem = false;
            let mut dump_stack = false;
            for a in &args[4..] {
                if let Some(v) = a.strip_prefix("--boot=") {
                    boot = v;
                } else if let Some(v) = a.strip_prefix("--mode=") {
                    mode = v;
                } else if a == "--dump-mem" {
                    dump_mem = true;
                } else if a == "--dump-stack" {
                    dump_stack = true;
                } else {
                    panic!(
                        "unknown flag '{a}' (want --boot=sim|hw --mode=step|block --dump-mem --dump-stack)"
                    );
                }
            }
            run_corpus(&args[2], &args[3], boot, mode, dump_mem, dump_stack)
        }
        Some("alu-dump") => {
            // byte<TAB>mnemonic for every defined parallel-ALU opcode byte
            for b in 0..=255u8 {
                let label = format!("{}", dsp56300_core::decode_parallel_alu(b));
                if label != "undefined" {
                    println!("{b:02x}\t{label}");
                }
            }
        }
        Some("coverage") if args.len() >= 4 && args.len().is_multiple_of(2) => {
            let pairs: Vec<(String, String)> = args[2..]
                .chunks(2)
                .map(|c| (c[0].clone(), c[1].clone()))
                .collect();
            run_coverage(&pairs)
        }
        Some("run-to") if args.len() == 6 => run_to(
            &args[2],
            &args[3],
            args[4].parse().unwrap(),
            u32::from_str_radix(args[5].trim_start_matches('$'), 16).unwrap(),
        ),
        _ => {
            eprintln!(
                "usage: difftest corpus <corpus.lod> <corpus.meta> [--boot=sim|hw] [--mode=step|block]"
            );
            eprintln!("       difftest coverage <corpus.lod> <corpus.meta> [...more pairs]");
            eprintln!("       difftest run-to <snapshot.lod> <step|block> <max_cycles> <stop_pc>");
            std::process::exit(2);
        }
    }
}

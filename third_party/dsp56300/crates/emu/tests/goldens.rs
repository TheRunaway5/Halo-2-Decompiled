//! Offline regression against the committed real-hardware goldens: run the
//! `difftest` binary over every sealed bank under tools/difftest/goldens in
//! both engine modes and compare the dumps the way tools/difftest/diff.py
//! does. The manifest and seal drift checks stay with the Python generator;
//! this test answers whether the engine still matches silicon.

use std::collections::BTreeMap;
use std::path::{Path, PathBuf};
use std::process::Command;

type Case = BTreeMap<String, String>;

struct Results {
    cases: Vec<(String, Case)>,
    /// Dirty-state header: memory dumps are deviations from
    /// `(k * addr + c) & $FFFFFF`, stack slots from a seed ramp.
    fill: Option<[u32; 6]>,
}

fn parse(text: &str) -> Results {
    let mut r = Results {
        cases: Vec::new(),
        fill: None,
    };
    let mut cur: Option<(String, Case)> = None;
    for line in text.lines() {
        let p: Vec<&str> = line.split_whitespace().collect();
        match p.as_slice() {
            [] => {}
            ["case", name] => cur = Some((name.to_string(), Case::new())),
            ["fill", v @ ..] if v.len() == 6 => {
                let mut f = [0u32; 6];
                for (i, s) in v.iter().enumerate() {
                    f[i] = u32::from_str_radix(s, 16).unwrap();
                }
                r.fill = Some(f);
            }
            ["end"] => {
                if let Some(c) = cur.take() {
                    r.cases.push(c);
                }
            }
            [k, v] => {
                if let Some((_, c)) = cur.as_mut() {
                    c.insert(k.to_string(), v.to_string());
                }
            }
            _ => {}
        }
    }
    r
}

fn is_mem_key(k: &str) -> bool {
    k.len() == 6 && matches!(&k[..2], "xm" | "ym" | "pm")
}

fn is_stack_key(k: &str) -> bool {
    k.len() == 4 && matches!(&k[..2], "sh" | "sl")
}

/// Value of a memory or stack-slot key a dump left out: its init value.
fn default_for(fill: Option<[u32; 6]>, k: &str) -> String {
    let Some(f) = fill else {
        return "000000".into();
    };
    if k.starts_with("pm") {
        return "000000".into();
    }
    if is_stack_key(k) {
        let base = if k.starts_with("sh") { f[4] } else { f[5] };
        let slot = u32::from_str_radix(&k[2..], 16).unwrap();
        return format!("{:06x}", (base + slot - 1) & 0xFF_FFFF);
    }
    let addr = u32::from_str_radix(&k[2..], 16).unwrap();
    let (kk, c) = if k.starts_with("xm") {
        (f[0], f[1])
    } else {
        (f[2], f[3])
    };
    format!(
        "{:06x}",
        (kk.wrapping_mul(addr).wrapping_add(c)) & 0xFF_FFFF
    )
}

const INFORMATIONAL: [&str; 3] = ["cyc", "ictr", "steps"];

/// Differences between an emulator dump and the goldens, one line each.
fn compare(emu: &Results, hw: &Results) -> Vec<String> {
    let hw_cases: BTreeMap<&str, &Case> = hw.cases.iter().map(|(n, c)| (n.as_str(), c)).collect();
    let mut out = Vec::new();
    for (name, a) in &emu.cases {
        let Some(b) = hw_cases.get(name.as_str()) else {
            out.push(format!("{name}: missing from the goldens"));
            continue;
        };
        let mut keys: Vec<&String> = a.keys().chain(b.keys()).collect();
        keys.sort();
        keys.dedup();
        for k in keys {
            if INFORMATIONAL.contains(&k.as_str()) {
                continue;
            }
            let sparse = is_mem_key(k) || is_stack_key(k);
            let va = match a.get(k) {
                Some(v) => v.clone(),
                None if sparse => default_for(emu.fill, k),
                None => continue,
            };
            let vb = match b.get(k) {
                Some(v) => v.clone(),
                None if sparse => default_for(hw.fill, k),
                None => continue,
            };
            if va == "-" || vb == "-" {
                continue; // explicitly unknown on one side
            }
            if va != vb {
                out.push(format!("{name} {k}: emu {va} | silicon {vb}"));
            }
        }
    }
    for (name, _) in &hw.cases {
        if !emu.cases.iter().any(|(n, _)| n == name) {
            out.push(format!("{name}: missing from the emulator dump"));
        }
    }
    out
}

fn goldens_dir() -> PathBuf {
    Path::new(env!("CARGO_MANIFEST_DIR")).join("../../tools/difftest/goldens")
}

fn run_bank(bank: &Path, mode: &str) -> Vec<String> {
    let hw_text = std::fs::read_to_string(bank.join("results_hw.txt")).unwrap();
    // Goldens captured with --dump-mem / --dump-stack carry sparse memory
    // and stack-slot keys; produce a matching dump so the same state is
    // compared. Goldens captured without either flag keep working as-is.
    let want_mem = hw_text
        .lines()
        .any(|l| l.split_whitespace().next().is_some_and(is_mem_key));
    let want_stack = hw_text
        .lines()
        .any(|l| l.split_whitespace().next().is_some_and(is_stack_key));
    let mut cmd = Command::new(env!("CARGO_BIN_EXE_difftest"));
    cmd.arg("corpus")
        .arg(bank.join("corpus.lod"))
        .arg(bank.join("corpus.meta"))
        .arg("--boot=hw")
        .arg(format!("--mode={mode}"));
    if want_mem {
        cmd.arg("--dump-mem");
    }
    if want_stack {
        cmd.arg("--dump-stack");
    }
    let out = cmd.output().expect("run difftest");
    assert!(
        out.status.success(),
        "difftest failed on {}: {}",
        bank.display(),
        String::from_utf8_lossy(&out.stderr)
    );
    let emu = parse(&String::from_utf8_lossy(&out.stdout));
    assert!(
        !emu.cases.is_empty(),
        "no cases dumped for {}",
        bank.display()
    );
    compare(&emu, &parse(&hw_text))
}

fn check(mode: &str) {
    let mut banks: Vec<PathBuf> = std::fs::read_dir(goldens_dir())
        .expect("goldens directory")
        .filter_map(|e| e.ok().map(|e| e.path()))
        .filter(|p| p.join("MANIFEST.sha256").exists())
        .collect();
    banks.sort();
    assert!(!banks.is_empty(), "no golden banks found");
    let mut failures = Vec::new();
    for bank in &banks {
        let name = bank.file_name().unwrap().to_string_lossy().into_owned();
        for d in run_bank(bank, mode) {
            failures.push(format!("{name}: {d}"));
        }
    }
    assert!(
        failures.is_empty(),
        "{} differences from silicon in {mode} mode:\n{}",
        failures.len(),
        failures.join("\n")
    );
}

#[test]
fn goldens_match_silicon_step() {
    check("step");
}

#[test]
fn goldens_match_silicon_block() {
    check("block");
}

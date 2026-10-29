#!/usr/bin/env python3
"""Measure actual large allocations on the real Rust snapshot receive path.

Run through tools/run_test.py. The counting allocator lives only in this owned
probe executable; the production protocol crate retains its unsafe-code ban.
"""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess

ROOT=Path(__file__).resolve().parents[1]
PROBE = r"""//! The real fragment receiver must reject tiny expansion bombs before allocation.
use halo2_online_protocol::netcode::stream::{MAX_BODY, Receiver};
use std::alloc::{GlobalAlloc, Layout, System};
use std::sync::atomic::{AtomicBool, AtomicUsize, Ordering};

struct AllocationProbe;
static WATCH: AtomicBool = AtomicBool::new(false);
static LARGE: AtomicUsize = AtomicUsize::new(0);
unsafe impl GlobalAlloc for AllocationProbe {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        if WATCH.load(Ordering::Relaxed) && layout.size() >= MAX_BODY {
            LARGE.fetch_add(1, Ordering::Relaxed);
        }
        unsafe { System.alloc(layout) }
    }
    unsafe fn dealloc(&self, pointer: *mut u8, layout: Layout) {
        unsafe { System.dealloc(pointer, layout) }
    }
}
#[global_allocator]
static ALLOCATOR: AllocationProbe = AllocationProbe;

fn main() {
    let mut packet = 1u32.to_le_bytes().to_vec(); // sequence
    packet.extend_from_slice(&[0, 0, 1, 0]); // fragment 0 of 1
    packet.push(1); // keyframe
    packet.extend_from_slice(&1u32.to_le_bytes());
    packet.push(0); // no baseline
    packet.extend_from_slice(&[0; 4]); // invalid digest, checked after decode
    let mut length = MAX_BODY as u64;
    while length >= 128 {
        packet.push((length as u8 & 127) | 128);
        length >>= 7;
    }
    packet.push(length as u8);
    packet.push(0); // no physical generic stream
    let mut receiver = Receiver::new(1400);
    WATCH.store(true, Ordering::Relaxed);
    let result = receiver.push(&packet, 0);
    WATCH.store(false, Ordering::Relaxed);
    assert!(result.is_err());
    assert_eq!(LARGE.load(Ordering::Relaxed), 0, "snapshot expanded before native validation");
}
"""

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--baseline',action='store_true')
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    library=ROOT/'rust/online_protocol'
    if args.baseline:
        old=scratch/'baseline'
        shutil.copytree(library,old,ignore=shutil.ignore_patterns('target'))
        for name in ('generic.rs','range.rs','stream.rs','pose.rs'):
            relative='rust/online_protocol/src/netcode/'+name
            (old/'src/netcode'/name).write_bytes(subprocess.check_output(['git','-C',str(ROOT),'show','HEAD:'+relative]))
        library=old
    probe=scratch/'probe';(probe/'src').mkdir(parents=True)
    (probe/'src/main.rs').write_text(PROBE)
    (probe/'Cargo.toml').write_text('[package]\nname="snapshot-allocation-probe"\nversion="0.1.0"\nedition="2024"\n[dependencies]\nhalo2-online-protocol={path='+json.dumps(str(library))+'}\n')
    lock=(library/'Cargo.lock').read_text()
    lock+='\n[[package]]\nname = "snapshot-allocation-probe"\nversion = "0.1.0"\ndependencies = ["halo2-online-protocol"]\n'
    (probe/'Cargo.lock').write_text(lock)
    command=['cargo','+1.96.0','run','--locked','--offline','--manifest-path',str(probe/'Cargo.toml')]
    result=subprocess.run(command,env=dict(os.environ,CARGO_BUILD_JOBS='2',CARGO_TARGET_DIR=str(scratch/'cargo')),
        stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
    text=result.stdout.decode(errors='replace')[-65536:]
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.with_suffix('.log').write_text(text)
    baseline_reproduced=args.baseline and result.returncode==101 and 'snapshot expanded before native validation' in text and 'left: 1' in text
    args.output.write_text(json.dumps({'passed':result.returncode==0,'baseline':args.baseline,'baseline_reproduced':baseline_reproduced,
        'command':command,'returncode':result.returncode,'scratch':str(scratch),'retained_large_artifacts':[]},indent=2)+'\n')
    print('Baseline 8 MiB allocation reproduced' if baseline_reproduced else ('No 8 MiB allocation before rejection' if result.returncode==0 else text[-3000:]))
    return result.returncode

if __name__=='__main__':raise SystemExit(main())

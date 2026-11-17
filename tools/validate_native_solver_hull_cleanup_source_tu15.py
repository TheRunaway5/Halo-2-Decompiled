#!/usr/bin/env python3
"""Private actual3139F0 retained cleanup with the genuine 149E00 null-data path."""
import argparse
import hashlib
import json
import os
import platform
import random
import re
import struct
import subprocess
from pathlib import Path

from research_mp_sandbox_update_functions import Xbe
from validate_native_tu15_prt_helpers import PIN

ROOT = Path(__file__).resolve().parents[1]
PRIMARY = ROOT/'src/physics/physics_live_support_tu15_generated.cpp'
WORLD = ROOT/'src/physics/runtime/physics_world_runtime_generated.cpp'
PARENT_PIN = 'c206d983eb1e95187a2b77908ecdc822fed57da3341ad38bbe8462f39846a15c'
CHILD_PIN = '915923a18cf561d7fe1392ed3ca9f135e3d628e8761115b76fbe8871fae8de85'
RESOLVER_PIN = '20de37b96fc56e19b3ec67f5a2e7aedebe84b001436654380330b4704430f0f2'


def digest(data):
    return hashlib.sha256(data).hexdigest()


def reference():
    primary, world = PRIMARY.read_text(), WORLD.read_text()
    body = re.search(r'static void run_3139f0\(WorldState &s\) \{.*?\n\}', primary, re.S)[0]
    child = re.search(r'static void run_148a70\(ObservedWorldState &s\) \{.*?\n\}', world, re.S)[0]
    resolver = re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{[^\n]*\}', world)[0]
    if tuple(digest(text.encode()) for text in (body, child, resolver)) != (PARENT_PIN, CHILD_PIN, RESOLVER_PIN):
        raise ValueError('Complete cleanup/allocator/reference resolver drift')
    sites = [int(re.search(r'services.instruction\(0x([0-9a-f]+)u\)', line)[1], 16)
             for line in body.splitlines() if re.search(r'\b(?:r|w)\(s.memory|s.push\(|s.pop\(', line)]
    child = re.sub(r'world_observation_pc\(0x[0-9a-f]+u,(0x[0-9a-f]+u),s\)', r'\1', child)
    if 'world_observation_pc(' in child:
        raise ValueError('Incomplete actual release-PC projection')
    child = child.replace('run_148a70(ObservedWorldState &s)', 'allocator_release(WorldState &s)', 1)
    body = body.replace('run_3139f0(', 'cleanup(', 1)
    io = 'static constexpr std::array<std::uint32_t,'+str(len(sites))+'> io_sites{'+','.join(hex(pc)+'u' for pc in sites)+'};'
    return '\n'.join((io, resolver, child, body))


def inputs(image):
    base, context, stack, fs = 0x61000000, 0x61001000, 0x61004000, 0x62000000
    vtable = image.read(0x45f000, 0x1000)
    allocator = image.read(0x486000, 0x1000)
    if struct.unpack_from('<I', allocator, 0x80)[0] != 0x45f4cc or struct.unpack_from('<I', vtable, 0x4e0)[0] != 0x149e00:
        raise ValueError('Genuine selected allocator object or vtable drift')
    global_bank = bytearray(0x1000)
    struct.pack_into('<I', global_bank, 0x938, 0x486080)
    rng = random.Random(0x3139f0)
    rows = []
    for i in range(32):
        bank = bytearray(rng.randbytes(0x6000)); gp = [rng.getrandbits(32) for _ in range(8)]
        gp[1], gp[4] = context, stack + 4*(i%4)
        struct.pack_into('<I', bank, gp[4]-base, 0x63000000)
        for n, (pointer, count) in enumerate(((0x2100, 0x2108), (0x1b0, 0x1b8), (8, 0x10))):
            capacity = (0, 3, 0x7ffffff0, 0x41617232)[i//8] | (0x80000000 if i & (1<<n) else 0)
            struct.pack_into('<I', bank, context-base+pointer, 0)
            struct.pack_into('<I', bank, context-base+count, capacity)
        banks = [(base, bank), (fs, rng.randbytes(0x1000)), (0x45f000, vtable),
                 (0x486000, allocator), (0x48c000, global_bank)]
        rows.append(dict(id=i,gp=gp,fs_base=fs,eflags=0x202|(rng.getrandbits(12)&0xcd5),
                         xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],
                         banks=[dict(address=address,before=value.hex())for address,value in banks]))
    return rows


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--report', type=Path, required=True)
    p.add_argument('--xbe', type=Path, required=True)
    p.add_argument('--compiler', default='c++')
    p.add_argument('--sanitizers', action='store_true')
    a = p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        p.error('Use tools/run_test.py')
    if platform.system()!='Linux' or platform.machine().lower()not in('x86_64','amd64','i386','i686') or not a.xbe.is_file():
        return 77
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    test = ROOT/'tests/native_tu15_solver_hull_cleanup_source_tests.cpp'
    files = [test, Path(__file__), ROOT/'src/physics/runtime/native_tu15_solver_hull_cleanup_source_frame.hpp',
             ROOT/'include/halo2/native_tu15_solver_hull_cleanup_kernel.hpp']
    pins = {str(path.relative_to(ROOT)):digest(path.read_bytes())for path in files}
    report = dict(passed=False, production_bound=False, owned_scratch=str(scratch), source_sha256=pins,
                  parent_body_sha256=PARENT_PIN, child_body_sha256=CHILD_PIN, retained_large_artifacts=[])
    try:
        if digest(a.xbe.read_bytes()) != PIN:
            raise ValueError('Wrong authenticated original image')
        original = reference()
        (scratch/'solver_hull_cleanup_actual.inc').write_text(original)
        data = scratch/'inputs.json'
        data.write_text(json.dumps(inputs(Xbe(a.xbe)), separators=(',', ':')))
        report['input_sha256'] = digest(data.read_bytes())
        command = [a.compiler,'-std=c++20','-O2','-Wno-ignored-attributes','-fno-fast-math','-ffp-contract=off',
                   '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'tests'),'-I'+str(scratch),str(test),'-o',str(scratch/'cleanup')]
        if a.sanitizers:
            command[1:1] = ['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        built = subprocess.run(command,capture_output=True,text=True,timeout=600)
        if built.returncode:
            raise ValueError('Cleanup source compile failed: '+built.stderr[-6000:])
        with data.open('rb') as inp:
            done = subprocess.run([command[-1]],stdin=inp,capture_output=True,text=True,timeout=600)
        if done.returncode:
            raise ValueError('Cleanup retained differential failed: '+done.stderr[-6000:])
        report.update(passed=True,measured=json.loads(done.stdout),scope='Retained source and actual149E00 null-data path; no physical FS/page-fault, non-null release, or default qualification.')
    except Exception as error:
        report['error'] = repr(error)
    try:
        reference()
        report['sources_unchanged'] = all(digest((ROOT/name).read_bytes())==value for name,value in pins.items())
    except (OSError,ValueError):
        report['sources_unchanged'] = False
    report['passed'] &= report['sources_unchanged']
    a.report.parent.mkdir(parents=True,exist_ok=True)
    a.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({key:report[key]for key in ('passed','error')if key in report}))
    return 0 if report['passed'] else 1


if __name__=='__main__':
    raise SystemExit(main())

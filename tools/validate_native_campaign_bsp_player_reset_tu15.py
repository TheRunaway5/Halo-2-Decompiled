#!/usr/bin/env python3
"""Compare native BSP resets with physical execution of pinned TU1.5 code.

The original 13A820 prefix stops at its callback loop. Original 14E1D0
and 154E70 run through return for the no-relocation path. A pending
relocation stops at 14E1F4. Compare ordered retained stores and every
retained heap byte. Source private-stack and return registers are outside
the native typed interface. Run under tools/run_test.py.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import random
import shutil
import signal
import struct
import subprocess
import sys

from generate_physics_live_support_tu15 import complete_body
from oracle_smg_firing_1_1 import Xbe, _Code
from original_audio_cpu import OriginalAudioCpu
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN

ROOT = Path(__file__).resolve().parents[1]
BASE = 0x1000000
GAME, USERS, POOL, FLAGS, PLAYERS = (BASE + n for n in (0, 0x2000, 0x2400, 0x2800, 0x3000))
STRIDE, STACK, STOP = 0x80, BASE + 0xff00, 0x3000000
RETAINED = 0x6000
KERNEL = 'src/campaign/native_campaign_bsp_player_reset_tu15.cpp'
TEST = 'tests/native_campaign_bsp_player_reset_tu15_tests.cpp'
HEADER = 'include/halo2/native_campaign_bsp_player_reset_tu15.hpp'


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def save(path, data):
    path.write_text(json.dumps(data, indent=2) + '\n')


def generate(scratch, image, report):
    code = _Code(image, '.text')
    entries = (0x13a820, 0x14e1d0, 0x154e70)
    allowed = set().union(*(complete_body(code, entry).code for entry in entries))
    vis_stores = {pc: ('rdi', 0, 4) for pc in (0x13a835, 0x13a842, 0x13a84f)}
    stores = {**vis_stores, 0x13a857: ('rdx', 2, 2), 0x14e5db: ('rcx', 0x2a, 2),
              0x154e80: ('rbp', 4, 1), 0x154e84: ('rbp', 5, 1), 0x154ed8: ('rbp', 4, 1),
              0x154ee4: ('rbp', 4, 1), 0x154eea: ('rbp', 5, 1)}
    rng = random.Random(0x14e1d0)
    cases, coverage = [], set()
    physical_steps = 0
    profiles = (
        ('empty', 0, ()), ('empty-high', 61, ()), ('gas-giant-stale-cluster', 1, (0,)),
        ('holes', 13, (1, 5, 12)), ('bitmap-boundary', 70, (0, 31, 32, 63, 64, 69)),
        ('eight-slots', 8, tuple(range(8))), ('signed-negative-high', 0xffffffff, ()),
    )
    with OriginalAudioCpu(image, scratch/'original.elf', 0x27f, 0x1f80) as cpu:
        cpu.trace(6, cpu.debug_offset+7*8, 0)
        for seed in range(4):
            for name, high, occupied in (*profiles, ('pending', 1, (0,)), ('visibility', 0, ())):
                bank = bytearray(rng.randbytes(RETAINED))
                def put(p, value, size=4):
                    bank[p-BASE:p-BASE+size] = value.to_bytes(size, 'little')
                masks = [0]*3
                for index in occupied:
                    masks[index//32] |= 1 << (index%32)
                for index, bits in enumerate(masks):
                    put(FLAGS + index*4, bits)
                for index in occupied:
                    player = PLAYERS + index*STRIDE
                    put(player+0x2a, 73 if name == 'gas-giant-stale-cluster' else rng.randrange(0x200), 2)
                    put(player+0x2c, 0x82d4b5c4 if (index+seed)%3 == 0 else 0xffffffff)
                    put(player+2, 8 if (index+seed)%3 == 1 else 0, 1)
                pending = name == 'pending'
                visibility = name == 'visibility'
                put(USERS+0xa4, seed if pending else 0xffff, 2)
                for off, value in ((0x24, STRIDE), (0x38, high), (0x44, PLAYERS), (0x48, FLAGS)):
                    put(POOL+off, value)
                global_cells = [(0x4f3218, GAME, 4), (0x4f54f0, USERS, 4), (0x4f54f4, POOL, 4),
                                (0x474c24, (0, 2, 4, 7)[seed], 2)]
                cpu.write(BASE, bank)
                cells = []
                for p, v, size in global_cells:
                    data = v.to_bytes(size, 'little')
                    cpu.write(p, data)
                    cells.append([p, data.hex()])
                cpu.write(STACK-256, rng.randbytes(272))
                cpu.write(STACK, struct.pack('<I', STOP))
                cpu.prepare(0x13a820 if visibility else 0x14e1d0, STACK, {})
                stop = 0x13a85b if visibility else 0x14e1f4 if pending else STOP
                regs = cpu.registers()
                events = []
                for _ in range(4000):
                    pc = regs.rip
                    if pc == stop:
                        break
                    assert pc in allowed, (name, hex(pc))
                    insn = code.at(pc)
                    assert cpu.read(pc, insn.size) == image.read(pc, insn.size)
                    coverage.add(pc)
                    if pc in stores:
                        reg, offset, width = stores[pc]
                        address = getattr(regs, reg) + offset
                        before = cpu.read(address, width)
                    cpu.trace(9)
                    _, status = os.waitpid(cpu.pid, 0)
                    assert os.WIFSTOPPED(status) and os.WSTOPSIG(status) == signal.SIGTRAP, (name, hex(pc), status)
                    if pc in stores:
                        events.append([pc, address, before.hex(), cpu.read(address, width).hex()])
                    regs = cpu.registers()
                    physical_steps += 1
                else:
                    raise AssertionError((name, 'original instruction budget'))
                if stop == STOP:
                    assert regs.rsp == STACK+4
                after = cpu.read(BASE, RETAINED)
                replay = bytearray(bank)
                for _, address, before, written in events:
                    off = address-BASE
                    data = bytes.fromhex(written)
                    assert 0<=off<=len(bank)-len(data)
                    assert replay[off:off+len(data)] == bytes.fromhex(before)
                    replay[off:off+len(data)] = data
                assert after == replay, (name, 'unexpected original retained write')
                cases.append(dict(name=f'{name}-{seed}', cells=cells, heap=bank.hex(), after=after.hex(),
                                  stores=events, refused=pending, visibility=visibility))
    report.update(original_sites=sorted(hex(pc) for pc in coverage), original_steps=physical_steps,
                  cases=len(cases), ordered_stores=sum(len(x['stores']) for x in cases))
    return cases


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    assert sha(NEW_DEFAULT) == NEW_PIN
    report = dict(passed=False, scope=__doc__, image_sha256=NEW_PIN, original_code_patches=0,
                  source_hashes={p: sha(ROOT/p) for p in (HEADER, KERNEL, TEST)}, builds=[],
                  retained_large_artifacts=[])
    try:
        fixture = scratch/'fixture.json'
        save(fixture, dict(image_sha256=NEW_PIN, cases=generate(scratch, Xbe(NEW_DEFAULT), report)))
        report['fixture_sha256'] = sha(fixture)
        project = scratch/'project'
        project.mkdir()
        shutil.copytree(ROOT/'include', project/'include')
        for source in (KERNEL, TEST):
            shutil.copyfile(ROOT/source, project/Path(source).name)
        kernel = project/Path(KERNEL).name
        clean = kernel.read_text()
        (project/'CMakeLists.txt').write_text(f'''cmake_minimum_required(VERSION 3.20)
project(bsp_reset LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
add_executable(bsp_reset {Path(KERNEL).name} {Path(TEST).name})
target_include_directories(bsp_reset PRIVATE include)
target_compile_features(bsp_reset PRIVATE cxx_std_20)
target_link_libraries(bsp_reset PRIVATE nlohmann_json::nlohmann_json)
add_test(NAME bsp_reset COMMAND bsp_reset "{fixture}")
''')
        mutants = (('stale-cluster', 'player+0x2au,0xffffu,2', 'player+0x2au,0u,2'),
                   ('wrong-status', 'status+5u,1,1', 'status+5u,0,1'),
                   ('short-mask', 'n<16;++n', 'n<15;++n'))
        for compiler in ('g++', 'clang++'):
            for name, old, new in (('native', '', ''), *mutants):
                expected = name == 'native'
                if not expected:
                    assert clean.count(old)==1
                    kernel.write_text(clean.replace(old, new))
                else:
                    kernel.write_text(clean)
                dest = output/(compiler+'-'+name)
                cmd = [sys.executable, '-B', str(ROOT/'tools/run_tests.py'), '--source', str(project),
                       '--scratch-root', str(scratch/'builds'), '--output', str(dest),
                       '--target', 'bsp_reset', '--filter', '^bsp_reset$', '--jobs', '2',
                       '--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler]
                with (output/(dest.name+'.log')).open('wb') as stream:
                    run = subprocess.run(cmd, stdout=stream, stderr=subprocess.STDOUT)
                result = json.loads((dest/'report.json').read_text())
                if not expected:
                    assert 'Original BSP reset mismatch:' in (dest/'tests.log').read_text()
                report['builds'].append(dict(compiler=compiler, case=name, native_exit=run.returncode,
                                             passed=result['passed'], expected_pass=expected))
                assert result['passed'] == expected and (run.returncode==0)==expected
        report['passed'] = True
    finally:
        save(output/'report.json', report)
    print(json.dumps({k: report[k] for k in ('passed', 'cases', 'original_steps', 'ordered_stores')}))


if __name__ == '__main__':
    main()

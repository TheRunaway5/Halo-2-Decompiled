#!/usr/bin/env python3
"""Compare whole423B0 against unchanged TU1.5 bytes at declared callees.

This tests CPU ordering and bank/ABI preservation. Scene copy352D0, collector
44C80, graphics setters, selector44360 and draw44540/445C0 are explicit fixture
boundaries, not recovered GPU services or evidence of hologram rendering.
"""
import argparse
import hashlib
import importlib.util
import json
import os
import platform
from pathlib import Path
import random
import struct
import subprocess
import sys

from tag_match_memory_budget import enter_memory_budget

PIN = '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
XBE = Path('/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe')
ROOT = Path(__file__).resolve().parents[1]
BASES = (0x473000, 0x492000, 0x4c6000, 0x4ce000, 0x2000000, 0x2100000)
STACK, PAYLOAD, STOP = 0x2000800, 0x2100100, 0x2200000
CELLS = (0x4ce268, 0x4ce26c, 0x4ce270, 0x4ce274, 0x4ce278, 0x473690, 0x473694, 0x473698, 0x4920d4)
ARGUMENTS = {0x352d0: 2, 0x44c80: 8, 0x14ba0: 2, 0x14b40: 3,
             0x15660: 0, 0x15350: 0, 0x44540: 0, 0x445c0: 0, 0x44360: 0}


def pack(*words):
    return struct.pack('<' + 'I' * len(words), *[v & 0xffffffff for v in words])


def sha(path):
    h = hashlib.sha256()
    with Path(path).open('rb') as f:
        while block := f.read(65536):
            h.update(block)
    return h.hexdigest()


def code_pages(path):
    # Read only needed code pages; the complete XBE is streamed for identity.
    with path.open('rb') as f:
        h = f.read(0x168)
        assert h[:4] == b'XBEH'
        base = struct.unpack_from('<I', h, 0x104)[0]
        count, table = struct.unpack_from('<II', h, 0x11c)
        f.seek(table - base)
        sections = [struct.unpack('<6I', f.read(56)[:24]) for _ in range(count)]
        pages = {}
        for address in (0x14000, 0x15000, 0x35000, 0x42000, 0x44000):
            section = next(s for s in sections if s[1] <= address and address + 4096 <= s[1] + s[4])
            f.seek(section[3] + address - section[1])
            pages[address] = f.read(4096)
        return pages


def cases():
    rng = random.Random(0x423b0)
    def one(mask, gates, frame, selectors):
        return [mask, gates, frame & 0xffffffff, selectors, rng.randrange(32),
                rng.getrandbits(32), rng.getrandbits(32), rng.getrandbits(32),
                rng.randrange(32), rng.getrandbits(32)]
    frames = (-0x80000000, -1, 0, 3, 4, 5, 7, 8, 0x7fffffff)
    bits = (1, 3, 8, 7, 2, 16, 21)
    for pattern in range(128):
        mask = sum(1 << bit for i, bit in enumerate(bits) if pattern & (1 << i))
        for gates in range(16):
            yield one(mask, gates, frames[(pattern + gates) % len(frames)], (pattern + gates) & 3)
    full = sum(1 << bit for bit in bits)
    for frame in frames:
        for gates in range(16):
            for selectors in range(4):
                yield one(full, gates, frame, selectors)
    for _ in range(128):
        yield one(rng.getrandbits(32), rng.randrange(16), rng.getrandbits(32), rng.randrange(4))


class Original:
    def __init__(self, pages, manifest):
        from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
        from unicorn import x86_const as x
        self.x = x
        self.g = Uc(UC_ARCH_X86, UC_MODE_32)
        for p, raw in pages.items():
            self.g.mem_map(p, 4096)
            self.g.mem_write(p, raw)
        for p in BASES:
            self.g.mem_map(p, 4096)
        self.g.mem_map(STOP, 4096)
        self.gp = [getattr(x, 'UC_X86_REG_' + r) for r in ('EAX', 'ECX', 'EDX', 'EBX', 'ESP', 'EBP', 'ESI', 'EDI')]
        self.xmm = [getattr(x, 'UC_X86_REG_XMM' + str(i)) for i in range(8)]
        self.instructions = {int(p, 16): bytes.fromhex(v['source_bytes']) for p, v in manifest['lowered_instructions'].items()}
        self.callsites = {int(p, 16): int(v['target'], 16) for p, v in manifest['external_owners'].items()}
        self.coverage, self.boundaries = set(), set()
        self.g.hook_add(UC_HOOK_CODE, self.hook)
        self.g.hook_add(UC_HOOK_MEM_WRITE, self.store)

    def word(self, p):
        return int.from_bytes(self.g.mem_read(p, 4), 'little')

    def hook(self, g, pc, size, _):
        x = self.x
        if pc in ARGUMENTS:
            count, sp = ARGUMENTS[pc], g.reg_read(x.UC_X86_REG_ESP)
            site = self.trace[-1]
            if self.callsites.get(site) != pc:
                raise ValueError('Undeclared original call boundary')
            self.boundaries.add((site, pc))
            gp = [g.reg_read(r) for r in self.gp]
            self.calls += pack(site, pc, 0, *gp, g.reg_read(x.UC_X86_REG_EFLAGS) & ~0x10)
            self.calls += pack(*[self.word(sp + i * 4) for i in range(count + 1)])
            self.calls += pack(*[self.word(p) for p in CELLS])
            returned = self.word(sp)
            if pc == 0x44c80:
                g.mem_write(0x4ce270, pack(self.input[0]))
            if pc == 0x44360:
                layer = gp[7]
                assert layer in (15, 21)
                for p, value in ((0x473690, layer), (0x473694, 0), (0x473698, 0x4ce268)):
                    g.mem_write(p, pack(value))
                g.reg_write(x.UC_X86_REG_EAX, (gp[0] & 0xffffff00) | ((self.input[3] >> (layer == 21)) & 1))
            g.reg_write(x.UC_X86_REG_ESP, sp + 4 + count * 4)
            g.reg_write(x.UC_X86_REG_EIP, returned)
            return
        raw = bytes(g.mem_read(pc, size))
        if self.instructions.get(pc) != raw:
            raise ValueError(f'Original instruction absent or changed at {pc:08x}')
        self.coverage.add(pc)
        self.trace.append(pc)

    def store(self, g, access, address, size, value, _):
        self.stores += pack(g.reg_read(self.x.UC_X86_REG_EIP), address, size)
        self.stores += bytes(g.mem_read(address, size))
        self.stores += (value & ((1 << (size * 8)) - 1)).to_bytes(size, 'little')

    def run(self, input):
        self.input = input
        self.trace, self.stores, self.calls = [], bytearray(), bytearray()
        mask, gates, frame, selectors, section, flags, oldmask, oldbank, target, seed = input
        g, x = self.g, self.x
        for r, p in enumerate(BASES):
            g.mem_write(p, bytes((i * 13 + r * 17 + seed) & 255 for i in range(4096)))
        values = ((STACK, STOP), (STACK + 4, PAYLOAD), (PAYLOAD, section), (PAYLOAD + 4, flags),
                  (0x4ce270, oldmask), (0x4ce26c, oldbank), (0x4920d8, target), (0x4920b8, frame))
        for p, value in values:
            g.mem_write(p, pack(value))
        for p, bit, value in ((0x4c6842, 1, 0x81), (0x4c6843, 2, 0x42), (0x492295, 4, 0xff), (0x492296, 8, 0x80)):
            g.mem_write(p, bytes([value if gates & bit else 0]))
        for i, reg in enumerate(self.gp):
            g.reg_write(reg, STACK if i == 4 else (0xa5000000 + i * 0x1020304 + seed) & 0xffffffff)
        for i, reg in enumerate(self.xmm):
            g.reg_write(reg, int.from_bytes(pack(*[0x87654321 + i * 101 + j * 0x10203 + seed for j in range(4)]), 'little'))
        g.reg_write(x.UC_X86_REG_EFLAGS, 0x202 | (seed & 0x8d5))
        g.reg_write(x.UC_X86_REG_FPCW, 0x27f)
        g.reg_write(x.UC_X86_REG_FPSW, 0)
        g.reg_write(x.UC_X86_REG_FPTAG, 0xffff)
        g.reg_write(x.UC_X86_REG_MXCSR, 0x1f80)
        g.emu_start(0x423b0, STOP, count=10000)
        assert g.reg_read(x.UC_X86_REG_EIP) == STOP and g.reg_read(x.UC_X86_REG_ESP) == STACK + 8
        assert self.word(0x4ce270) == oldmask and self.word(0x4ce274) == 0
        out = pack(len(self.trace), *self.trace) + pack(len(self.stores)) + self.stores
        out += pack(len(self.calls)) + self.calls
        out += pack(*[g.reg_read(r) for r in self.gp], g.reg_read(x.UC_X86_REG_EFLAGS) & ~0x10, 0x2300000)
        out += b''.join(g.reg_read(r).to_bytes(16, 'little') for r in self.xmm)
        out += pack(0, g.reg_read(x.UC_X86_REG_FPSW), g.reg_read(x.UC_X86_REG_FPCW), g.reg_read(x.UC_X86_REG_MXCSR))
        out += b''.join(bytes(g.mem_read(p, 4096)) for p in BASES)
        return out


def read_exact(stream, count):
    result = bytearray()
    while len(result) < count:
        data = stream.read(count - len(result))
        if not data:
            raise ValueError('Native group comparison exited before response')
        result += data
    return result


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--native', type=Path, required=True)
    p.add_argument('--manifest', type=Path, required=True)
    p.add_argument('--xbe', type=Path, default=XBE)
    p.add_argument('--output', type=Path, required=True)
    a = p.parse_args()
    if platform.system() != 'Linux' or platform.machine() not in ('x86_64', 'i386', 'i686'):
        return 77
    if importlib.util.find_spec('unicorn') is None:
        print('Independent Unicorn comparison dependency absent')
        return 77
    if not a.xbe.exists():
        print('Original TU1.5 XBE absent')
        return 77
    result = enter_memory_budget(a.output, 180)
    if result is not None:
        return result
    assert os.environ.get('HALO2_TEST_SCRATCH'), 'Run through tools/run_test.py'
    assert sha(a.xbe) == PIN
    manifest = json.loads(a.manifest.read_text())
    generated = ROOT / 'src/source_transparent_group_tu15_generated.cpp'
    assert manifest['generated_cpp_sha256'] == sha(generated)
    original = Original(code_pages(a.xbe), manifest)
    digest = hashlib.sha256()
    compared = instructions = stores = calls = count = 0
    peer = subprocess.Popen([str(a.native.resolve())], stdin=subprocess.PIPE, stdout=subprocess.PIPE)
    report = dict(passed=False, scope=__doc__, original_xbe_sha256=PIN,
                  generated_source_sha256=sha(generated), native_sha256=sha(a.native),
                  tool_sha256=sha(Path(__file__)), peer_source_sha256=sha(Path(__file__).with_suffix('.cpp')),
                  scratch=str(Path(os.environ['HALO2_TEST_SCRATCH'])), undefined_flags_excluded=['AF'],
                  supplied_callee_fixtures={f'{target:08x}': n for target, n in ARGUMENTS.items()})
    try:
        for index, input in enumerate(cases()):
            expected = original.run(input)
            peer.stdin.write(pack(*input))
            peer.stdin.flush()
            size = struct.unpack('<I', read_exact(peer.stdout, 4))[0]
            assert size < 65536
            actual = read_exact(peer.stdout, size)
            if expected != actual:
                mismatch = next((i for i, (a, b) in enumerate(zip(expected, actual)) if a != b), min(len(expected), len(actual)))
                report['failure'] = dict(case=index, input=input, offset=mismatch,
                                         expected=expected[max(0, mismatch-16):mismatch+32].hex(),
                                         actual=actual[max(0, mismatch-16):mismatch+32].hex(),
                                         expected_size=len(expected), actual_size=len(actual))
                raise ValueError(f'Whole group CPU mismatch in case{index} at byte{mismatch}')
            digest.update(expected)
            count += 1
            compared += len(expected)
            instructions += len(original.trace)
            stores += len(original.stores)
            calls += len(original.calls)
        peer.stdin.close()
        assert peer.wait(timeout=5) == 0
        control = subprocess.run([str(a.native.resolve()), '--reject-missing-owner'],
                                 input=pack(*next(cases())), stdout=subprocess.PIPE,
                                 stderr=subprocess.PIPE, timeout=5)
        assert control.returncode == 0, control.stderr.decode()
        assert original.coverage == set(original.instructions), 'Whole body instruction coverage incomplete'
        assert original.boundaries == set(original.callsites.items()), 'Declared call-site coverage incomplete'
        report.update(passed=True, cases=count, instructions=instructions,
                      distinct_instructions=len(original.coverage), call_sites=len(original.boundaries),
                      ordered_store_bytes=stores, boundary_abi_bytes=calls, compared_bytes=compared,
                      comparison_sha256=digest.hexdigest(),
                      missing_services_rendered=False, real_hologram_rendering_verified=False,
                      missing_scene_copy_owner_rejected=control.stdout.decode().strip())
    finally:
        if peer.poll() is None:
            peer.terminate()
            peer.wait(timeout=5)
        a.output.mkdir(parents=True, exist_ok=True)
        (a.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(f'PASS {count} whole423B0 CPU comparisons; {instructions} instructions, exact ordered stores, calls, exit ABI and 24KiB banks; GPU callees remain required')
    return 0


if __name__ == '__main__':
    sys.exit(main())

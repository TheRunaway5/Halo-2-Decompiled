#!/usr/bin/env python3
"""Qualify the native 2E79A0/2E7C40 projection queries against unchanged TU1.5.

Builds, in owned temporary storage:
  * a freestanding i386 runner with the authenticated XBE .text/.rdata/.data
    at their real virtual addresses (no libc, crash-isolated cases), and
  * a 64-bit harness holding the translated 2E79A0/2E6290/309600/3092A0 and
    2E7C40/2E6ED0 bodies extracted verbatim from the generated Shadow-frame
    unit (bridge lines removed) and the native bridges of
    src/rendering/shadows/native_tu15_projection_query.hpp.
Generated MOPP programs, planes, spheres, arrays (including genuine 2E3250
growth) and FP environments run through all three; native must equal the
physical original exactly. --mutants also runs the out-of-tree mutation
screen. Exit 77 when the XBE or a 32-bit toolchain is unavailable.
"""
import argparse
import hashlib
import json
import os
import re
import shutil
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
HERE = ROOT / 'tools/native_projection_query_physical'
PIN = '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
UNIT = ROOT / 'src/rendering/shadows/source_shadow_frame_tu15_generated.cpp'
HEADER = ROOT / 'src/rendering/shadows/native_tu15_projection_query.hpp'
BODIES = ('2e79a0', '2e6290', '309600', '3092a0', '2e7c40', '2e6ed0')
# Translated bodies (bridge lines stripped) these results were qualified on.
BODY_PINS = {
    '2e79a0': '031d98339dfba46a9fab5b3742ae3cc742be06899ccf8df646e6de3aba20637a',
    '2e6290': 'bd41fc547bf804dfaec3c9509c6c8f3e1bb75eab0c5308068fa3c0e534000ca9',
    '309600': 'c62e338a98cbda1c3ac37249707762623d13ca0c62796ea023e8be8ca3c65dec',
    '3092a0': '57270aac107ce9c9408f3002674e54fe3abcbf0a2359836114a3c093f2598ab7',
    '2e7c40': 'd9bd2801a235477383aab5f2c4c0e170ab52d453e898a603093151aeb644de84',
    '2e6ed0': 'ee31eddbbc19eb2a7f485612b4ae779229ceffd91f47b3cd235a76209b110870',
}
MUTATIONS = {
    'split-order': ('''                const auto right = volume_.classify(second, status_);
                if (right == inside) accept(second_pc, s);
                const auto left = volume_.classify(box, status_);
                if (left == inside) accept(first_pc, s);''', '''                const auto left = volume_.classify(box, status_);
                if (left == inside) accept(first_pc, s);
                const auto right = volume_.classify(second, status_);
                if (right == inside) accept(second_pc, s);'''),
    'single-split-plane': ('box.hi[axis] = quantized(s, axis, plane + 1);', 'box.hi[axis] = quantized(s, axis, plane);'),
    'rescale-shift': ('s.shift -= op; local = true; pc += 4; continue;', 's.shift -= op - 1; local = true; pc += 4; continue;'),
    'property-alias': ('else if (local) s.offset[slot - 1] = value;', 'else s.offset[slot - 1] = value;'),
    'plane-inside': ('return all == 15 ? inside : partial;', 'return all != 0 ? inside : partial;'),
    'plane-outside-sign': ('if (sse::signs(sse::sub(distance, radius)) != 15) return outside;',
                           'if (sse::signs(sse::sub(distance, radius)) == 0) return outside;'),
    'plane-radius-order': ('const auto radius = sse::add(sse::add(xx, yy), near_z);', 'const auto radius = sse::add(sse::add(xx, xx), near_z);'),
    'sphere-inside': ('return parity == 1u || parity == 4u ? inside : partial;', 'return parity == 1u ? partial : inside;'),
    'sphere-outside': ("if (!(first & 0x4100u)) { status = std::uint16_t(first & ~0x200u); return outside; }",
                       "if (!(first & 0x4000u)) { status = std::uint16_t(first & ~0x200u); return outside; }"),
    'endpoint-last-child': ('pc += last ? g_.u8(pc + 3) + 4 : 4; continue;', 'pc += 4; continue;'),
    'bound-offset': ('return x87::bound(std::bit_cast<std::int32_t>((value << (s.shift & 31u)) + s.offset[axis]), scale_, status_);',
                     'return x87::bound(std::bit_cast<std::int32_t>(value << (s.shift & 31u)), scale_, status_);'),
    'key-word': ('if (op == 0x0a) { s.key += g_.be(pc + 1, 2); local = true; pc += 3; continue; }',
                 'if (op == 0x0a) { s.key += g_.u8(pc + 1); local = true; pc += 3; continue; }'),
    'c1-dropped': ('    status = std::uint16_t((status & ~0x200u) | (word & 0x200u));\n}', '    (void)word; (void)status;\n}'),
    'terminal-width': ('events_.push_back({s.key + g_.be(pc + 1, op - 0x4f), Stream::direct}); return; }',
                       'events_.push_back({s.key + g_.be(pc + 1, 1), Stream::direct}); return; }'),
    'slab-absolute': ('box.lo[axis] = absolute(g_.be(pc + 1, 3));', 'box.lo[axis] = absolute(g_.be(pc + 1, 2));'),
    'origin-doubled': ('p[3] = sse::add(w, w);', 'p[3] = w;'),
    'sphere-lane3': ('lanes[3] = volume.radius;', 'lanes[3] = 0.f;'),
    'remainder-lane3': ('lanes[3] = x87::copy(p0[c], status);', 'lanes[3] = x87::copy(p2[c], status);'),
    'batch-count': ('const auto full = count & ~3u;', 'const auto full = count & ~7u;'),
}


def sha(data):
    return hashlib.sha256(data).hexdigest()


def sections(xbe):
    data = xbe.read_bytes()
    base = struct.unpack_from('<I', data, 0x104)[0]
    count = struct.unpack_from('<I', data, 0x11c)[0]
    header = struct.unpack_from('<I', data, 0x120)[0] - base
    out = {}
    for i in range(count):
        _, va, vsize, raw, rsize, name = struct.unpack_from('<IIIIII', data, header + i * 56)
        label = data[name - base:data.index(b'\0', name - base)].decode()
        out[label] = (va, vsize, data[raw:raw + rsize])
    return out


def read(secs, va, n):
    for sva, vsize, raw in secs.values():
        if sva <= va < sva + max(vsize, len(raw)):
            chunk = raw[va - sva:va - sva + n]
            return chunk + b'\0' * (n - len(chunk))
    raise KeyError(hex(va))


def extract(unit):
    text = unit.read_text()
    lines = text.split('\n')
    dispatch = next(i for i, l in enumerate(lines) if l.startswith('static void dispatch(WorldState &s,std::uint32_t target'))
    bodies, pins = [], {}
    for entry in BODIES:
        match = re.search(r'static void run_%s\(WorldState &s\) \{\n(.*?)\n\}\n' % entry, text, re.S)
        if not match:
            raise ValueError('missing translated body ' + entry)
        body = re.sub(r'\n    if\(halo2::native_tu15_projection_query::bridge_[0-9a-f]+\(s,.*?\)\)return;', '', match.group(0))
        if 'projection_query' in body:
            raise ValueError('bridge not removable from ' + entry)
        pins[entry] = sha(body.encode())
        bodies.append(body)
    child = ('static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail){\n'
             'if(target==0x2e6290u){run_2e6290(s);return;}if(target==0x2e6ed0u){run_2e6ed0(s);return;}\n'
             'if(target==0x309600u){run_309600(s);return;}if(target==0x3092a0u){run_3092a0(s);return;}\n'
             'if(target==0x2e79a0u){run_2e79a0(s);return;}if(target==0x2e7c40u){run_2e7c40(s);return;}\n'
             's.required(site,target,tail);}\n')
    return '\n'.join(lines[:dispatch]) + '\n' + child + '\n'.join(bodies) + '\n', pins


def run(command, **kw):
    result = subprocess.run(command, capture_output=True, text=True, **kw)
    if result.returncode:
        raise RuntimeError(' '.join(map(str, command[:3])) + ' failed: ' + (result.stdout + result.stderr)[-3000:])
    return result.stdout


def build_runner(secs, work):
    base, end = 0x12000, 0x573860
    image = bytearray(end - base)
    for name in ('.text', '.rdata', '.data'):
        va, _, raw = secs[name]
        image[va - base:va - base + len(raw)] = raw
    (work / 'image.bin').write_bytes(image)
    (work / 'image.s').write_text('.code32\n.section .image,"awx"\n.incbin "%s"\n' % (work / 'image.bin'))
    run(['gcc', '-m32', '-ffreestanding', '-fno-pic', '-fno-pie', '-O2', '-mgeneral-regs-only', '-fno-stack-protector',
         '-fno-asynchronous-unwind-tables', '-c', str(HERE / 'runner.c'), '-o', str(work / 'runner.o')])
    run(['as', '--32', str(HERE / 'glue.s'), '-o', str(work / 'glue.o')])
    run(['as', '--32', str(work / 'image.s'), '-o', str(work / 'image.o')])
    run(['ld', '-melf_i386', '--no-warn-rwx-segments', '-T', str(HERE / 'link.ld'), str(work / 'image.o'), str(work / 'glue.o'),
         str(work / 'runner.o'), '-o', str(work / 'runner')])
    return work / 'runner'


def build_harness(secs, work, compiler, header_root=None, extra=()):
    tables = bytearray()
    for va, n in ((0x2e6000, 0x2000), (0x309000, 0x1000), (0x41c000, 0x1000), (0x461000, 0x1000), (0x46a000, 0x1000)):
        tables += struct.pack('<II', va, n) + read(secs, va, n)
    (work / 'tables.bin').write_bytes(tables)
    output = work / ('harness' + ('-' + header_root.name if header_root else ''))
    command = [compiler, '-std=c++20', '-O2', '-fno-fast-math', '-ffp-contract=off', '-frounding-math', *extra,
               f'-DIMAGE_PATH="{work / "tables.bin"}"', f'-I{work}']
    if header_root:
        command.append(f'-I{header_root}')
    command += [f'-I{ROOT / "src"}', f'-I{ROOT / "include"}', '-isystem', '/usr/include/SDL2', str(HERE / 'harness.cpp'), '-o', str(output)]
    run(command)
    return output


def physical(harness, runner, seed, cases, work):
    directory = work / f'cases-{seed}'
    directory.mkdir(exist_ok=True)
    out = subprocess.run([str(harness), '--physical', str(cases), str(seed), str(runner), str(directory)],
                         capture_output=True, text=True, timeout=1800)
    summary = next((l for l in out.stdout.splitlines() if l.startswith('physical cases=')), None)
    if not summary:
        raise RuntimeError('physical run failed: ' + (out.stdout + out.stderr)[-2000:])
    fields = dict(kv.split('=') for kv in summary.split()[1:])
    classes = {l.strip().rsplit(':', 1)[0]: int(l.rsplit(':', 1)[1]) for l in out.stdout.splitlines() if l.startswith('  ')}
    shutil.rmtree(directory)
    return {k: int(v) for k, v in fields.items()} | {'classes': classes}


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--xbe', type=Path, required=True)
    parser.add_argument('--compiler', default='c++')
    parser.add_argument('--report', type=Path)
    parser.add_argument('--seeds', default='101-104')
    parser.add_argument('--cases', type=int, default=2000)
    parser.add_argument('--sanitizers', action='store_true')
    parser.add_argument('--mutants', action='store_true')
    args = parser.parse_args()
    if not args.xbe.is_file() or not all(shutil.which(t) for t in ('gcc', 'as', 'ld')):
        print('TU1.5 XBE or i386 toolchain unavailable'); return 77
    if sha(args.xbe.read_bytes()) != PIN:
        raise SystemExit('XBE does not match the authenticated TU1.5 image')
    first, _, last = args.seeds.partition('-')
    seeds = list(range(int(first), int(last or first) + 1))
    scratch = os.environ.get('HALO2_TEST_SCRATCH')
    with tempfile.TemporaryDirectory(dir=scratch) as temporary:
        work = Path(temporary)
        secs = sections(args.xbe)
        translated, pins = extract(UNIT)
        if pins != BODY_PINS:
            raise SystemExit('Translated projection-query bodies drifted: ' + json.dumps(pins))
        (work / 'translated.inc').write_text(translated)
        try:
            runner = build_runner(secs, work)
        except RuntimeError as error:
            print('i386 runner unavailable:', str(error)[-400:]); return 77
        extra = ('-fsanitize=address,undefined', '-fno-sanitize-recover=undefined', '-g') if args.sanitizers else ()
        harness = build_harness(secs, work, args.compiler, extra=extra)
        report = dict(schema='native-projection-query-physical-v1', xbe_sha256=PIN, header_sha256=sha(HEADER.read_bytes()),
                      translated_body_sha256=pins, sanitizers=args.sanitizers, seeds=seeds, cases_per_seed=args.cases, runs=[])
        failures = 0
        for seed in seeds:
            result = physical(harness, runner, seed, args.cases, work)
            report['runs'].append(dict(seed=seed, **result))
            failures += result['native_failures']
            print(f'seed {seed}: {result}')
        report['native_failures'] = failures
        if args.mutants:
            before = sha(HEADER.read_bytes())
            report['mutants'] = {}
            for name, (old, new) in MUTATIONS.items():
                root = work / 'mutant' / name
                target = root / 'rendering/shadows/native_tu15_projection_query.hpp'
                target.parent.mkdir(parents=True)
                text = HEADER.read_text()
                if text.count(old) != 1:
                    raise SystemExit('mutation anchor missing: ' + name)
                target.write_text(text.replace(old, new))
                binary = build_harness(secs, work, args.compiler, header_root=root)
                result = physical(binary, runner, seeds[0], 3000, work)
                report['mutants'][name] = result['native_failures']
                binary.unlink(); shutil.rmtree(root)
                print(f'mutant {name}: native_failures={result["native_failures"]}')
            report['header_unchanged'] = sha(HEADER.read_bytes()) == before
            failures += sum(1 for v in report['mutants'].values() if v == 0) + (not report['header_unchanged'])
        if args.report:
            args.report.write_text(json.dumps(report, indent=2) + '\n')
        return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())

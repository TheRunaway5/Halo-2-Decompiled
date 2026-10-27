#!/usr/bin/env python3
"""Physical TU1.5 initializer/hash with an explicit real native host clock.

Original.text and its encoded KeQuerySystemTime import cell stay unchanged.
The declared native i386 Linux import bridge queries the real wall clock and
publishes8B FILETIME. Original32BBAE computes seconds. Native initializer tests
consume only that genuine service return; they never execute the old root.
Private key banks, referenceELFs and builds remain owned disposable scratch.
"""
import argparse, copy, hashlib, json, os, random, struct, subprocess, sys, time
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_32, CS_AC_READ, CS_AC_WRITE
from capstone.x86_const import X86_OP_MEM
from oracle_smg_firing_1_1 import ROOT, Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import complete_body

BASE, KEY, STACK, SP, STOP = 0x1000000, 0x1000000, 0x2000000, 0x200f000, 0x3000000
BANKS = ((BASE, 0x200), (0x41b668, 4), (0x41f4f0, 0xc0), (0x450108, 4),
         (0x461900, 0x100), (0x47c1b0, 4), (0x48d480, 1))
OWNERS = (0x326640, 0x3268d0, 0x37c5c0, 0x32bbae, 0x32ae00, 0x33a4d3)
BACKEND = 'unchanged-tu15-init-clienthash-native-linux-time-import-v1'

# Actual native platform input, not a supplied time, date, solver result or
# ready value. NTAPI's one pointer is the genuine original output slot.
CLOCK_ASSEMBLY = r'''.code32
.section .native_clock,"ax"
 push %ebp
 mov %esp,%ebp
 push %ebx
 push %esi
 push %edi
 sub $8,%esp
 mov $78,%eax
 mov %esp,%ebx
 xor %ecx,%ecx
 int $0x80
 test %eax,%eax
 jns 1f
 mov $1,%eax
 mov $125,%ebx
 int $0x80
1:
 mov (%esp),%eax
 mov $10000000,%ecx
 mul %ecx
 add $0xd53e8000,%eax
 adc $0x019db1de,%edx
 mov %eax,%esi
 mov %edx,%edi
 mov 4(%esp),%eax
 mov $10,%ecx
 mul %ecx
 add %esi,%eax
 adc %edi,%edx
 mov 8(%ebp),%ecx
 mov %eax,(%ecx)
 mov %edx,4(%ecx)
 add $8,%esp
 pop %edi
 pop %esi
 pop %ebx
 leave
 ret $4
'''

def sha(path):
    return hashlib.file_digest(Path(path).open('rb'), 'sha256').hexdigest()

def descriptors(instructions):
    registers = {n: i for i, n in enumerate(('eax', 'ecx', 'edx', 'ebx', 'esp', 'ebp', 'esi', 'edi'))}
    result = []
    for instruction in instructions:
        operands = []
        for op in instruction.operands:
            if op.type != X86_OP_MEM or instruction.mnemonic == 'lea':
                continue
            access = op.access
            assert access & (CS_AC_READ | CS_AC_WRITE)
            base, index = instruction.reg_name(op.mem.base), instruction.reg_name(op.mem.index)
            assert not op.mem.segment
            operands.append(dict(base=registers[base] if base else -1,
                                 index=registers[index] if index else -1, scale=op.mem.scale,
                                 displacement=op.mem.disp & 0xffffffff, bytes=op.size,
                                 read=bool(access & CS_AC_READ), write=bool(access & CS_AC_WRITE)))
        result.append(dict(instruction=instruction.address, operands=operands))
    return result

def metadata(image, scratch):
    code = _Code(image, '.text')
    sites, counts, listings = [], [], {}
    for entry in OWNERS:
        body = complete_body(code, entry)
        counts.append(len(body.code))
        instructions = [code.at(p) for p in sorted(body.code)]
        sites += descriptors(instructions)
        listing = ''.join(f'{i.address:x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n' for i in instructions)
        listings[hex(entry)] = hashlib.sha256(listing.encode()).hexdigest()
    assert counts == [152, 125, 4, 22, 43, 1]
    assert image.read(0x41b668, 4) == struct.pack('<I', 0x80000080)
    sdk = ROOT / 'analysis/multiplayer/integration/spawn-source/kernel-sync-world/XboxDev-xboxkrnl.h'
    assert '100-nanosecond intervals since January 1, 1601, in GMT.' in sdk.read_text()
    assert 'NTAPI KeQuerySystemTime' in sdk.read_text()
    (scratch / 'clock.s').write_text(CLOCK_ASSEMBLY)
    subprocess.run(['as', '--32', str(scratch / 'clock.s'), '-o', str(scratch / 'clock.o')], check=True, capture_output=True)
    subprocess.run(['objcopy', '-O', 'binary', '-j', '.native_clock', str(scratch / 'clock.o'), str(scratch / 'clock.bin')], check=True, capture_output=True)
    decoder = Cs(CS_ARCH_X86, CS_MODE_32)
    decoder.detail = True
    native = list(decoder.disasm((scratch / 'clock.bin').read_bytes(), 0x80000080))
    assert sum(i.size for i in native) == (scratch / 'clock.bin').stat().st_size
    sites += descriptors(native)
    return dict(sites=sites, body_sites=counts, listing_sha256=listings,
                regions=[dict(address=p, bytes=n) for p, n in BANKS],
                clock_source_sha256=hashlib.sha256(CLOCK_ASSEMBLY.encode()).hexdigest(),
                clock_instruction_sha256=sha(scratch / 'clock.bin'), sdk_sha256=sha(sdk))

def make_case(image, name, entry=0x3268d0, key=None, client=None, ready=0, **extra):
    banks = {p: bytearray(image.read(p, n) if p != BASE else bytes(n)) for p, n in BANKS}
    def put(p, value, width=4):
        start = next(lo for lo, size in BANKS if lo <= p and p + width <= lo + size)
        banks[start][p-start:p-start+width] = (value & ((1 << (width*8))-1)).to_bytes(width, 'little')
    if key is not None:
        assert len(key) <= 0x200
        banks[BASE][:len(key)] = key
        put(0x47c1b0, KEY)
    pointer = struct.unpack('<I', banks[0x47c1b0])[0]
    if client is not None:
        put(0x450108, client)
    put(0x48d480, ready, 1)
    stop_pc = extra.pop('stop_pc', STOP)
    return dict(name=name, entry=entry, key_pointer=pointer,
                banks=[dict(address=p, before=bytes(banks[p]).hex()) for p, _ in BANKS],
                before_fp=[0x27f, 0, 0x1f80], stop_pc=stop_pc, **extra)

def input_cases(image):
    out = [make_case(image, 'actual_private_key_hash', 0x326640),
           make_case(image, 'actual_client_ready'),
           make_case(image, 'actual_client_logger', client=struct.unpack('<I', image.read(0x450108, 4))[0] ^ 1)]
    rng = random.Random(0x326640)
    for n in (0, 1, 2, 6, 31, 63, 64, 65, 100):
        for high in (False, True):
            key = bytes(rng.randrange(128, 256) if high else rng.randrange(1, 128) for _ in range(n)) + b'\0'
            out.append(make_case(image, f'hash_length_{n}_signed_{high}', 0x326640, key))
    for n, value in enumerate((0, 1, 0x7fc12345, 0x7f812345, 0xffc54321, 0xff854321, 0x80000000, 0xffffffff)):
        out.append(make_case(image, f'hash_integer_payload_{n}', 0x326640, struct.pack('<I', value) * 16 + b'\0'))
    mask = struct.unpack('<I', image.read(0x41f4f0, 4))[0]
    today = int(time.time()) // 86400
    def encoded(value):
        value = (value & 0xffffffff) ^ mask
        return bytes(0x41 + ((value >> (n*4)) & 15) for n in range(8))
    for day in (-1, 0, 1, 10, 179, 180, 181):
        for platform in (b'PC', b'P', b'', b'PX', b'PCX'):
            key = b'case-' + platform + b'-' + encoded((today + day) * 86400) + b'\0'
            out.append(make_case(image, f'date_day_{day}_platform_{platform.decode() or "empty"}', key=key, client=0))
    for n, value in enumerate((0, 1, 0x7fffffff, 0x80000000, 0xffffffff, 0x8001517f)):
        out.append(make_case(image, f'date_signed_edge_{n}', key=b'case-PC-' + encoded(value) + b'\0', client=0, ready=n & 1))
    for n, key in enumerate((b'no_delimiter\0', b'one-PC\0', b'case-PC-\0' + bytes(16), b'case--' + bytes([0xff]) * 8 + b'\0')):
        out.append(make_case(image, f'date_malformed_{n}', key=key, client=0, ready=n & 1))
    out.append(make_case(image, 'genuine_time_entry_boundary', key=b'case--AAAAAAAA\0', client=0, stop_pc=0x37c5c0))
    for n, case in enumerate(out):
        case['before_fp'] = [((0x27f, 0x37f, 0x7f)[n % 3] & ~0xc00) | ((n % 4) << 10),
                             ((n % 8) << 11) | (0x453f if n % 3 == 0 else 0),
                             (0x1f80, 0x5fa5, 0xffc0)[n % 3]]
    return out

def physical(image, metadata_value, cases, scratch):
    owned = scratch / 'physical-original'
    owned.mkdir(exist_ok=True)
    observer = owned / 'observer'
    if not observer.exists():
        subprocess.run(['c++', '-std=c++20', '-O2', str(ROOT / 'tools/observe_native_constraint_initializer_tu15.cpp'), '-o', str(observer)], check=True, capture_output=True)
    section = next(s for s in image.sections if s.name == '.text')
    (owned / 'text.bin').write_bytes(image.section_bytes(section))
    for number, case in enumerate(cases):
        actual_metadata = dict(metadata_value, stop_occurrence=case.get('stop_occurrence', 1),
                               stop_at_logger=case.get('discover_log_boundary', False) and 'fault' not in case)
        (owned / 'metadata.json').write_text(json.dumps(actual_metadata, separators=(',', ':')))
        assembly = [f'.section .original,"ax"\n.incbin "{owned / "text.bin"}"']
        segments = [('original', section.va, '5')]
        for n, bank in enumerate(case['banks']):
            path = owned / f'bank{n}.bin'
            path.write_bytes(bytes.fromhex(bank['before']))
            assembly.append(f'.section .bank{n},"aw"\n.incbin "{path}"')
            segments.append((f'bank{n}', bank['address'], '6'))
        stack = bytearray(0x10000)
        args = [STOP] + ([case['key_pointer']] if case['entry'] == 0x326640 else [])
        struct.pack_into('<' + 'I' * len(args), stack, SP - STACK, *args)
        (owned / 'stack.bin').write_bytes(stack)
        assembly.append(f'.section .stack,"aw"\n.incbin "{owned / "stack.bin"}"')
        segments.append(('stack', STACK, '6'))
        cw, sw, mxcsr = case['before_fp']
        environment = bytearray(28)
        struct.pack_into('<H', environment, 0, cw)
        struct.pack_into('<H', environment, 4, sw)
        struct.pack_into('<H', environment, 8, 0xffff)
        (owned / 'environment.bin').write_bytes(environment)
        assembly.append(f'''.code32
.section .entry,"ax"
.globl _start
_start:
 fninit
 fldenv environment
 ldmxcsr sse_control
 mov ${SP},%esp
 int3
 jmp *root
.balign 4
root:.long {case['entry']}
sse_control:.long {mxcsr}
environment:.incbin "{owned / 'environment.bin'}"
''')
        assembly.append(CLOCK_ASSEMBLY)
        segments += [('entry', 0x70000000, '5'), ('native_clock', 0x80000080, '5')]
        (owned / 'reference.s').write_text('\n'.join(assembly))
        phdrs = '\n'.join(f'{name} PT_LOAD FLAGS({flags});' for name, _, flags in segments)
        sections = '\n'.join(f'.{name} 0x{address:x} : {{ *(.{name}) }} :{name}' for name, address, _ in segments)
        (owned / 'link.ld').write_text('ENTRY(_start)\nPHDRS {\n' + phdrs + '\n}\nSECTIONS {\n' + sections + '\n}\n')
        subprocess.run(['as', '--32', str(owned / 'reference.s'), '-o', str(owned / 'reference.o')], check=True, capture_output=True)
        subprocess.run(['ld', '-melf_i386', '-T', str(owned / 'link.ld'), str(owned / 'reference.o'), '-o', str(owned / 'reference')], check=True, capture_output=True)
        result = subprocess.run([str(observer), str(owned / 'reference'), str(owned / 'metadata.json'), str(owned / 'observed.json'), hex(case['stop_pc'])], text=True, capture_output=True)
        assert result.returncode == 0, (case['name'], result.stdout, result.stderr)
        observed = json.loads((owned / 'observed.json').read_text())
        for before, after in zip(case['banks'], observed.pop('banks')):
            assert before['address'] == after['address']
            before['after'] = after['after']
        time_entry, time_return = observed.pop('time_entry'), observed.pop('time_return')
        case['time_called'] = time_entry is not None
        case['actual_time'] = None
        case['clock_dependency_reads'] = 0
        if time_return is not None:
            actual_filetime = int.from_bytes(bytes.fromhex(time_return['filetime']), 'little')
            assert time_return['seconds'] == ((actual_filetime - 116444736000000000) // 10000000) & 0xffffffff
            case['actual_time'] = dict(genuine_original_return=True, seconds=time_return['seconds'], fp=time_return['fp'], filetime=time_return['filetime'])
            assert time_entry['write_index'] == time_return['write_index']
            first, last = time_entry['read_index'], time_return['read_index']
            case['clock_dependency_reads'] = last - first
            observed['reads'] = observed['reads'][:first] + observed['reads'][last:]
        case.update(observed, physical_verified=True)
        case['stop_pc'] = case['boundary_instruction']
        if 'fault' in case:
            case['boundary'] = None
        elif case['stop_pc'] == 0x2e3850:
            case['boundary'] = dict(kind='log', message=case['boundary_stack'][1])
        elif case['stop_pc'] == 0x37c5c0:
            case['boundary'] = dict(kind='time')
        else:
            case['boundary'] = None
            assert case['stop_pc'] == STOP
        if case['entry'] == 0x326640 and 'fault' not in case:
            case['actual_hash'] = case['boundary_gp'][0]
        if number and number % 25 == 0:
            print('Physical initializer cases', number, flush=True)
    return cases

def fault_cases(cases):
    result = []
    seeds = [cases[0], next(c for c in cases if c['name'] == 'hash_length_64_signed_True'),
             next(c for c in cases if c['name'] == 'actual_client_ready'),
             next(c for c in cases if c['name'] == 'date_day_1_platform_PC')]
    for seed in seeds:
        seen, occurrences, pc_occurrences = set(), {}, {}
        for read in seed['reads']:
            key = read['address'], read['bytes']
            occurrences[key] = occurrences.get(key, 0) + 1
            pc = read['instruction']
            pc_occurrences[pc] = pc_occurrences.get(pc, 0) + 1
            nth = pc_occurrences[pc]
            if pc in seen and not (pc in (0x326800, 0x326812) and nth in (2, 10, 32, 64)):
                continue
            seen.add(pc)
            case = copy.deepcopy(seed)
            case.update(name=seed['name'] + f'_fault_{pc:x}_{nth}', stop_pc=pc,
                        stop_occurrence=nth, fault=dict(address=read['address'], bytes=read['bytes'], occurrence=occurrences[key]))
            result.append(case)
    return result

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    args.output.mkdir(parents=True, exist_ok=True)
    assert sha(NEW_DEFAULT) == NEW_PIN
    image = Xbe(NEW_DEFAULT)
    data = metadata(image, scratch)
    inputs = input_cases(image)
    # Each declared invalid client input ends at the genuine logger entry.
    inputs[2]['stop_pc'] = 0x2e3850
    for case in inputs:
        if case['name'].startswith('date_'):
            # Original may accept unusual encodings; determine its real stop
            # using a read-only observer with both genuine end boundaries.
            case['discover_log_boundary'] = True
    all_cases = physical(image, data, inputs, scratch)
    faults = fault_cases(all_cases)
    all_cases += physical(image, data, faults, scratch)
    fixture = scratch / 'initializer.json'
    fixture.write_text(json.dumps(dict(image_sha256=NEW_PIN, required_backend=BACKEND,
                                     body_sites=data['body_sites'], clock_import=0x80000080,
                                     listing_sha256=data['listing_sha256'], cases=all_cases), separators=(',', ':')))
    project = scratch / 'project'
    project.mkdir()
    source = ROOT / 'src/physics/native_tu15_constraint_solver_initializer.cpp'
    original = source.read_text()
    (project / 'original.cpp').write_text(original)
    mutations = [('hash', '0xc18f77a2u', '0xc18f77a3u'),
                 ('date', '!signed_less(time,date)', 'signed_less(time,date)')]
    for name, old, new in mutations:
        assert original.count(old) == 1
        (project / f'{name}.cpp').write_text(original.replace(old, new))
    (project / 'negative.py').write_text('import subprocess,sys\nr=subprocess.run([sys.argv[1],sys.argv[2]],text=True,capture_output=True)\nassert r.returncode!=0 and sys.argv[3] in r.stderr,(r.returncode,r.stdout,r.stderr)\nprint("Unchanged physical initializer rejects private mutation:",r.stderr.strip())\n')
    cmake = 'cmake_minimum_required(VERSION 3.20)\nproject(native_initializer_validation LANGUAGES CXX)\ninclude(CTest)\n'
    for name in ('original', 'hash', 'date'):
        target = 'native_initializer_' + name
        cmake += f'add_executable({target} "{ROOT}/tests/native_tu15_constraint_solver_initializer_tests.cpp" "{project}/{name}.cpp")\ntarget_include_directories({target} PRIVATE "{ROOT}/include")\ntarget_compile_features({target} PRIVATE cxx_std_20)\ntarget_compile_options({target} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n'
        expected = 'actual original client hash differs' if name == 'hash' else 'initializer fault/dependency completion differs'
        command = f'$<TARGET_FILE:{target}> "{fixture}"' if name == 'original' else f'"{sys.executable}" "{project}/negative.py" $<TARGET_FILE:{target}> "{fixture}" "{expected}"'
        cmake += f'add_test(NAME {target} COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- {command})\n'
    (project / 'CMakeLists.txt').write_text(cmake)
    results = []
    for compiler in ('g++', 'clang++'):
        command = [sys.executable, str(ROOT / 'tools/run_tests.py'), '--source', str(project), '--scratch-root', str(scratch / 'builds'), '--output', str(args.output / compiler)]
        for name in ('original', 'hash', 'date'):
            command += ['--target', 'native_initializer_' + name]
        command += ['--filter', '^native_initializer_', '--cmake-arg=-DCMAKE_CXX_COMPILER=' + compiler, '--jobs', '2', '--test-jobs', '1']
        result = subprocess.run(command)
        results.append(dict(compiler=compiler, exit_code=result.returncode, command=command))
    files = [source, ROOT / 'include/halo2/native_tu15_constraint_solver_initializer.hpp', ROOT / 'tests/native_tu15_constraint_solver_initializer_tests.cpp', ROOT / 'tools/observe_native_constraint_initializer_tu15.cpp', Path(__file__)]
    visited = set(p for case in all_cases for p in case['executed_sites'])
    code = _Code(image, '.text')
    coverage = {}
    for owner in OWNERS:
        sites = set(complete_body(code, owner).code)
        coverage[hex(owner)] = dict(total=len(sites), visited=len(sites & visited), missing=sorted(sites - visited))
    report = dict(passed=all(r['exit_code'] == 0 for r in results), required_backend=BACKEND,
                  compiler_results=results, original_cases=len(all_cases), read_fault_prefixes=len(faults),
                  complete_original_calls=sum(c['stop_pc'] == STOP and 'fault' not in c for c in all_cases),
                  genuine_logger_entry_stops=sum(c['stop_pc'] == 0x2e3850 and 'fault' not in c for c in all_cases),
                  genuine_time_entry_stops=sum(c['stop_pc'] == 0x37c5c0 and 'fault' not in c for c in all_cases),
                  genuine_clock_returns=sum(c['actual_time'] is not None for c in all_cases),
                  ordered_reads=sum(len(c['reads']) for c in all_cases), ordered_stores=sum(len(c['writes']) for c in all_cases),
                  original_instructions=sum(c['original_instructions'] for c in all_cases), image_sha256=NEW_PIN,
                  body_sites=data['body_sites'], listing_sha256=data['listing_sha256'], coverage=coverage,
                  clock_source_sha256=data['clock_source_sha256'], clock_instruction_sha256=data['clock_instruction_sha256'],
                  sdk_sha256=data['sdk_sha256'], source_hashes={str(p.relative_to(ROOT)): sha(p) for p in files},
                  fixture_sha256=sha(fixture), fixture_bytes=fixture.stat().st_size, owned_scratch=str(scratch),
                  key_text_printed=False, retained_large_artifacts=[],
                  clock_scope='Real native Linux gettimeofday8B FILETIME import; unchanged original32BBAE computes actual seconds; typed component consumes only captured genuine service output',
                  negative_controls=['Hash mixing seed changes C18F77A2->C18F77A3, must fail actual original hash comparison', 'Date signed predicate reversed, must fail actual dependency/ready completion'],
                  excluded=['original logger body execution', 'production initializer/clock binding', 'Xbox oscillator/time continuity', 'negative/overflowed platform-time values not produced by the actual current host clock', 'full live physics closure'])
    (args.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({k: report[k] for k in ('passed', 'original_cases', 'read_fault_prefixes', 'genuine_clock_returns')}))
    return 0 if report['passed'] else 1

if __name__ == '__main__':
    raise SystemExit(main())

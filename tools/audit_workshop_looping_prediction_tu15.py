"""Execute complete original looping-sound prediction into real cache touches.

The unchanged TU1.5 routines18D1D0 and128A20 select actual fixture-owned
track, pitch-range, permutation and chunk records. Their genuine21F260 callee
runs against resident or pending source-cache records, without intercepted
calls or supplied return values. Cold allocation/streaming, MCC projection,
native execution and mission acceptance remain separate requirements.
Run through run_test.py; retain only this bounded result.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import zlib

from generate_physics_live_support_tu15 import complete_body
from oracle_smg_firing_1_1 import Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn import x86_const as R


ROOT = 0x18d1d0
SOUND = 0x128a20
REQUEST = 0x21f260
DATA, SIZE = 0x2100000, 0x100000
DIRECTORY, LOOP, TRACKS, SOUND_ROOTS = [DATA + n * 0x10000 for n in range(4)]
GLOBALS, PITCHES, PERMUTATIONS, CHUNKS = [DATA + n * 0x10000 for n in range(4, 8)]
CACHE, CACHE_POOL, CACHE_STAMPS, CHUNK_POOL, CHUNK_ROWS = [DATA + n * 0x10000 for n in range(8, 13)]
STACK, STOP = DATA + SIZE - 0x1000, 0x3000000
LOOP_HANDLE = 0x12340001
CLOCK = 0x65432109


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def u32(value):
    return struct.pack('<I', value & 0xffffffff)


def patches(raw):
    """Losslessly describe a complete zero-initialized fixture bank."""
    result, first = [], None
    for offset, value in enumerate(raw + b'\0'):
        if value and first is None:
            first = offset
        elif not value and first is not None:
            result.append([first, raw[first:offset].hex()])
            first = None
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--fixture', type=Path)
    args = parser.parse_args()
    owner = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert not args.report.resolve().is_relative_to(owner)
    if args.fixture:
        assert args.fixture.resolve().is_relative_to(owner) and not args.fixture.exists()
    image = Xbe(NEW_DEFAULT)
    assert sha(image.data) == NEW_PIN
    code = _Code(image, '.text')
    bodies = {entry: complete_body(code, entry) for entry in (ROOT, SOUND, REQUEST)}
    assert {entry: len(body.code) for entry, body in bodies.items()} == {
        ROOT: 25, SOUND: 31, REQUEST: 92}
    assert not any(body.tables or body.tail_jumps for body in bodies.values())
    for body in bodies.values():
        for address, size in body.code.items():
            assert bytes(code.at(address).bytes) == image.read(address, size)
    assert image.read(0x18d1fe, 5).hex() == 'e81db8f9ff'
    assert image.read(0x128a78, 5).hex() == 'e8e3670f00'
    guest = Uc(UC_ARCH_X86, UC_MODE_32)
    guest.mem_map(0x10000, 0x800000)
    for section in image.sections:
        guest.mem_write(section.va, image.section_bytes(section))
    guest.mem_map(DATA, SIZE)
    guest.mem_map(STOP, 0x1000)
    visited, active, calls, sequence = set(), {}, [], []
    allowed = set().union(*(set(body.code) for body in bodies.values()))

    def read(at):
        return int.from_bytes(guest.mem_read(at, 4), 'little')

    def trace(_guest, address, _size, _data):
        assert address in allowed, f'Unexpected original dependency {address:#x}'
        visited.add(address)
        sequence.append(address)
        if address == SOUND:
            active['sounds'].append(guest.reg_read(R.UC_X86_REG_EDX))
        elif address == REQUEST:
            stack = guest.reg_read(R.UC_X86_REG_ESP)
            request = [read(stack + offset) for offset in (0, 4, 8, 12)]
            assert request[0] == 0x128a7d and request[3] == 2
            calls.append(request)

    guest.hook_add(UC_HOOK_CODE, trace)
    results, fixtures = [], []
    cases = [('NONE-loop', -1, []) , ('empty-loop', LOOP_HANDLE, [])]
    for count in (1, 2, 7, 32):
        cases.append((f'ready-{count}', LOOP_HANDLE, ['ready'] * count))
        cases.append((f'pending-{count}', LOOP_HANDLE, ['pending'] * count))
    cases += [('mixed-five', LOOP_HANDLE, ['NONE', 'no-pitches', 'no-permutations', 'ready', 'pending'])]
    aggregate = hashlib.sha256()
    for name, handle, tracks in cases:
        guest.mem_write(DATA, bytes(SIZE))
        guest.mem_write(0x4f0414, u32(DIRECTORY))
        guest.mem_write(0x52d664, u32(GLOBALS))
        guest.mem_write(0x50ebe4, u32(CACHE))
        guest.mem_write(0x50ebdc, u32(CHUNK_POOL))
        guest.mem_write(DIRECTORY + (LOOP_HANDLE & 65535) * 16 + 8, u32(LOOP))
        guest.mem_write(LOOP + 0x1c, u32(len(tracks)) + u32(TRACKS))
        guest.mem_write(GLOBALS + 0x24, u32(PITCHES))
        guest.mem_write(GLOBALS + 0x2c, u32(PERMUTATIONS))
        guest.mem_write(GLOBALS + 0x44, u32(CHUNKS))
        guest.mem_write(CACHE + 0x64, u32(CACHE_POOL))
        guest.mem_write(CACHE + 0x38, u32(CLOCK))
        guest.mem_write(CACHE_POOL + 0x44, u32(CACHE_STAMPS))
        guest.mem_write(CHUNK_POOL + 0x44, u32(CHUNK_ROWS))
        expected_sounds, expected_calls, expected_changes = [], [], {}
        for index, kind in enumerate(tracks):
            sound = 0x23450000 | (0x100 + index)
            root = SOUND_ROOTS + index * 0x40
            pitch, permutation, chunk = index * 3 + 1, index * 3 + 2, index * 3 + 3
            chunk_record = CHUNKS + chunk * 12
            cache_index = index * 2 + 1
            cache_row = CHUNK_ROWS + cache_index * 16
            stamp = CACHE_STAMPS + cache_index * 24 + 0x14
            if kind == 'NONE':
                sound = 0xffffffff
            else:
                guest.mem_write(DIRECTORY + (sound & 65535) * 16 + 8, u32(root))
                guest.mem_write(root + 8, struct.pack('<hB', pitch, 0 if kind == 'no-pitches' else 3))
                guest.mem_write(PITCHES + pitch * 12 + 8,
                                struct.pack('<hh', permutation, 0 if kind == 'no-permutations' else 3))
                guest.mem_write(PERMUTATIONS + permutation * 16 + 12, struct.pack('<h', chunk))
                guest.mem_write(chunk_record + 8, u32(0x34560000 | cache_index))
                guest.mem_write(cache_row + 2, bytes([kind == 'ready', 0, 0x5a]))
                if kind in ('ready', 'pending'):
                    expected_calls.append([0x128a7d, sound, chunk_record, 2])
                    for lane, byte in enumerate(u32(CLOCK)):
                        if byte:
                            expected_changes[stamp + lane] = byte
                    if kind == 'ready':
                        expected_changes[cache_row + 3] = 1
            # The original chooses only track+0x20, then the first selected
            # pitch-range/permutation chunk. Poison other sound/array slots;
            # the execution must never visit or modify those owners.
            guest.mem_write(TRACKS + index * 0x58, bytes([0xcd]) * 0x58)
            guest.mem_write(TRACKS + index * 0x58 + 0x20, u32(sound))
            expected_sounds.append(sound)
        before = bytes(guest.mem_read(DATA, SIZE - 0x2000))
        guest.mem_write(STACK, u32(STOP))
        guest.reg_write(R.UC_X86_REG_EAX, handle & 0xffffffff)
        guest.reg_write(R.UC_X86_REG_ESP, STACK)
        guest.reg_write(R.UC_X86_REG_EFLAGS, 0x202)
        guest.reg_write(R.UC_X86_REG_FPCW, 0x027f)
        guest.reg_write(R.UC_X86_REG_MXCSR, 0x1f80)
        guest.reg_write(R.UC_X86_REG_FPSW, 0x1800)
        for register, value in ((3, (0x8000000000000000, 0x3ffe)),
                                (4, (0x8000000000000000, 0xbfff)),
                                (5, (0x8000000000000000, 0x4000))):
            guest.reg_write(R.UC_X86_REG_FP0 + register, value)
        guest.reg_write(R.UC_X86_REG_FPTAG, 0xf03f)
        fp_registers = (R.UC_X86_REG_FPCW, R.UC_X86_REG_FPSW,
                        R.UC_X86_REG_FPTAG, R.UC_X86_REG_MXCSR)
        initial_fp = [guest.reg_read(register) for register in fp_registers]
        physical_fp = [guest.reg_read(R.UC_X86_REG_FP0 + register) for register in range(8)]
        preserved = [(R.UC_X86_REG_EBX, 0x76543210), (R.UC_X86_REG_EBP, 0xabcdef01),
                     (R.UC_X86_REG_ESI, 0x87654321), (R.UC_X86_REG_EDI, 0x10203040)]
        for register, value in preserved:
            guest.reg_write(register, value)
        gp = (R.UC_X86_REG_EAX, R.UC_X86_REG_ECX, R.UC_X86_REG_EDX, R.UC_X86_REG_EBX,
              R.UC_X86_REG_ESP, R.UC_X86_REG_EBP, R.UC_X86_REG_ESI, R.UC_X86_REG_EDI)
        # Explicitly reset scratch registers and all live SIMD lanes between
        # cases. The reference carries their actual initial/final ABI values.
        guest.reg_write(R.UC_X86_REG_ECX, 0xaabbccdd)
        guest.reg_write(R.UC_X86_REG_EDX, 0x99887766)
        xmm = [[0x12340000 + register * 256 + lane for lane in range(4)] for register in range(8)]
        for register, lanes in enumerate(xmm):
            guest.reg_write(R.UC_X86_REG_XMM0 + register,
                            int.from_bytes(struct.pack('<4I', *lanes), 'little'))
        initial_gp = [guest.reg_read(register) for register in gp]
        initial_data = bytes(guest.mem_read(DATA, SIZE))
        original_image = bytes(guest.mem_read(0x10000, 0x800000))
        calls.clear()
        sequence.clear()
        active.clear()
        active['sounds'] = []
        guest.emu_start(ROOT, STOP, count=100000)
        assert guest.reg_read(R.UC_X86_REG_EIP) == STOP
        assert guest.reg_read(R.UC_X86_REG_ESP) == STACK + 4
        assert all(guest.reg_read(register) == value for register, value in preserved)
        assert initial_fp == [guest.reg_read(register) for register in fp_registers]
        assert physical_fp == [guest.reg_read(R.UC_X86_REG_FP0 + register) for register in range(8)]
        assert original_image == bytes(guest.mem_read(0x10000, 0x800000))
        assert all(guest.reg_read(R.UC_X86_REG_XMM0 + register) ==
                   int.from_bytes(struct.pack('<4I', *lanes), 'little')
                   for register, lanes in enumerate(xmm))
        assert active['sounds'] == expected_sounds and calls == expected_calls
        after = bytes(guest.mem_read(DATA, SIZE - 0x2000))
        actual_changes = {DATA + index: new for index, (old, new) in enumerate(zip(before, after)) if old != new}
        assert actual_changes == expected_changes, (name, actual_changes, expected_changes)
        aggregate.update(before)
        aggregate.update(after)
        results.append(dict(name=name, tracks=len(tracks), original_sound_calls=len(active['sounds']),
            genuine_request_calls=len(calls), data_bytes_changed=len(actual_changes),
            exact_nonstack_bank_diff=True, saved_registers_and_stack_preserved=True))
        if args.fixture:
            fixtures.append(dict(name=name, initial_gp=initial_gp, initial_xmm=xmm,
                initial_flags=0x202, expected_gp=[guest.reg_read(register) for register in gp],
                expected_flags=guest.reg_read(R.UC_X86_REG_EFLAGS),
                defined_flags_mask=0x8c5 if name == 'empty-loop' else 0x8d5,
                initial_patches=patches(initial_data),
                expected_patches=patches(bytes(guest.mem_read(DATA, SIZE))),
                generated_effect_trace=[pc for pc in sequence if pc not in bodies[REQUEST].code],
                genuine_request_calls=len(calls)))
    for entry in (ROOT, SOUND):
        assert set(bodies[entry].code) <= visited
    report = dict(scope=__doc__, passed=True, xbe_sha256=NEW_PIN,
        helper_sha256=sha(Path(__file__).read_bytes()), cases=results,
        routines=[dict(entry=hex(entry), instruction_sites=len(body.code),
            observed_instruction_sites=len(set(body.code) & visited),
            all_sites_observed=set(body.code) <= visited,
            body_sha256=sha(b''.join(image.read(at, size) for at, size in sorted(body.code.items()))))
            for entry, body in bodies.items()],
        combined_bank_digest=aggregate.hexdigest(), original_code_patched=False,
        original_calls_intercepted=False, supplied_callee_results=False,
        cold_allocation_and_streaming_qualified=False, MCC_resource_projection_qualified=False,
        native_execution_qualified=False, production_admission_changed=False,
        complete_mod_support=False, owned_scratch=str(owner), retained_large_artifacts=[])
    if args.fixture:
        fixture = dict(xbe_sha256=NEW_PIN, original_code_patched=False,
            original_calls_intercepted=False, supplied_callee_results=False,
            entry=ROOT, data_base=DATA, data_bytes=SIZE,
            globals=[[at, bytes(guest.mem_read(at, 4)).hex()]
                     for at in (0x4f0414, 0x52d664, 0x50ebe4, 0x50ebdc)], cases=fixtures)
        decoded = (json.dumps(fixture, separators=(',', ':')) + '\n').encode()
        assert len(decoded) < 4 * 1024 * 1024
        packed = zlib.compress(decoded, 9)
        descriptor = dict(packed_bytes=len(packed), packed_sha256=sha(packed),
                          decoded_bytes=len(decoded), decoded_sha256=sha(decoded))
        args.fixture.parent.mkdir(parents=True, exist_ok=True)
        args.fixture.write_bytes(packed)
        args.fixture.with_suffix('').write_text(json.dumps(descriptor, indent=2) + '\n')
        report['fixture'] = descriptor
    raw = (json.dumps(report, indent=2) + '\n').encode()
    assert len(raw) < 1024 * 1024
    args.report.write_bytes(raw)
    print(json.dumps(dict(passed=True, cases=len(results),
        original_request_calls=sum(row['genuine_request_calls'] for row in results),
        complete_loop_sites=25, complete_sound_sites=31)))


if __name__ == '__main__':
    main()

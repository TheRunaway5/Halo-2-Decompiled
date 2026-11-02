"""Execute unchanged TU1.5 color packers against authored tiling inputs.

The integer-ratio proposal is compared with the actual original instructions,
including adjacent byte midpoints and out-of-domain controls. This establishes
the measured packing operation, not shader-compiler parameter roles or renderer
acceptance. Run through run_test.py; retain only the bounded JSON receipt.
"""
import argparse
import hashlib
import importlib.util
import json
import lzma
import os
from pathlib import Path
import struct
import sys

from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn import x86_const as R


STATE, STACK, STOP = 0x2100000, 0x22fef00, 0x3000000
ENTRIES = ((0x134960, 34), (0x1349e0, 27), (0x134a50, 32), (0x134ad0, 12))


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def words(values):
    return struct.pack('<' + 'I' * len(values), *(value & 0xffffffff for value in values))


def bits(value):
    return struct.unpack('<I', struct.pack('<f', value))[0]


def integer_channel(value):
    """Integer-ratio hypothesis for original FISTP under explicit FPCW027F."""
    if (value >> 23) & 255 == 255:
        return 0x80000000
    real = struct.unpack('<f', words([value]))[0]
    numerator, denominator = real.as_integer_ratio()
    quotient, remainder = divmod(abs(numerator) * 255, denominator)
    if 2 * remainder > denominator or (2 * remainder == denominator and quotient & 1):
        quotient += 1
    result = -quotient if numerator < 0 else quotient
    return result & 0xffffffff if -0x80000000 <= result <= 0x7fffffff else 0x80000000


def proposal(entry, argb, channel=integer_channel):
    a, r, g, b = map(channel, argb)
    if entry == 0x134960:
        return ((a << 24) | (r << 16) | (g << 8) | b) & 0xffffffff
    rgb = ((r & 255) << 16) | ((g & 255) << 8) | (b & 255)
    if entry == 0x1349e0:
        return rgb
    if entry == 0x134a50:
        return rgb | ((a << 24) & 0xffffffff)
    assert entry == 0x134ad0
    return (a << 24) & 0xffffffff


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--authority-directory', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--channel-proposal', type=Path)
    parser.add_argument('--fixture-output', type=Path)
    args = parser.parse_args()
    owner = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    channel_proposal = None
    if args.channel_proposal:
        spec = importlib.util.spec_from_file_location('owned_channel_proposal', args.channel_proposal)
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        channel_proposal = module.pack_channel
    image = Xbe(NEW_DEFAULT)
    assert sha(image.data) == NEW_PIN
    code = _Code(image, '.text')
    bodies = {}
    for entry, count in ENTRIES:
        body = _trace(code, entry, set())
        assert len(body.code) == count and not body.calls and not body.tail_jumps
        sites = sorted(body.code)
        assert sites[-1] + code.at(sites[-1]).size - entry == sum(body.code.values())
        assert code.at(sites[-1]).mnemonic == 'ret'
        bodies[entry] = dict(sites=set(sites), body_sha256=sha(image.read(entry, sum(body.code.values()))))
    assert image.read(0x46a148, 4) == words([bits(255)])
    guest = Uc(UC_ARCH_X86, UC_MODE_32)
    guest.mem_map(0x10000, 0x800000)
    for section in image.sections:
        guest.mem_write(section.va, image.section_bytes(section))
    guest.mem_map(STATE, 0x200000)
    guest.mem_map(STOP, 0x1000)
    visited = {entry: set() for entry, _ in ENTRIES}
    current = [None]

    def trace(_guest, address, _size, _data):
        assert address in bodies[current[0]]['sites'], hex(address)
        visited[current[0]].add(address)

    guest.hook_add(UC_HOOK_CODE, trace)

    def original(entry, argb):
        before = b'\xa5' * 16 + words(argb) + b'\x5a' * 16
        guest.mem_write(STATE, before)
        pointer = STATE + 16
        argv = {0x134960: [pointer], 0x1349e0: [pointer + 4],
                0x134a50: [argb[0], pointer + 4], 0x134ad0: [argb[0]]}[entry]
        guest.mem_write(STACK, words([STOP, *argv]))
        guest.reg_write(R.UC_X86_REG_ESP, STACK)
        guest.reg_write(R.UC_X86_REG_FPCW, 0x027f)
        guest.reg_write(R.UC_X86_REG_MXCSR, 0x1f80)
        guest.reg_write(R.UC_X86_REG_FPTAG, 0xffff)
        guest.reg_write(R.UC_X86_REG_FPSW, 0)
        preserved = ((R.UC_X86_REG_EBX, 0x8badf00d), (R.UC_X86_REG_EBP, 0x13579bdf),
                     (R.UC_X86_REG_ESI, 0x2468ace0), (R.UC_X86_REG_EDI, 0x5aa55aa5))
        for reg, value in preserved:
            guest.reg_write(reg, value)
        current[0] = entry
        guest.emu_start(entry, STOP, count=100)
        assert guest.reg_read(R.UC_X86_REG_EIP) == STOP
        assert guest.reg_read(R.UC_X86_REG_ESP) == STACK + 4
        assert guest.reg_read(R.UC_X86_REG_FPCW) == 0x027f
        assert all(guest.reg_read(reg) == value for reg, value in preserved)
        assert bytes(guest.mem_read(STATE, len(before))) == before
        return guest.reg_read(R.UC_X86_REG_EAX)

    cases, input_pins, paired = [], [], []
    authored_rows = 0
    for family in ('tex_bump_env_tiling_specular', 'tex_bump_env_illum_tiling_specular'):
        path = args.authority_directory / (family + '-pairs-1.json.xz')
        raw = path.read_bytes()
        data = json.loads(lzma.decompress(raw))
        assert not data['errors'] and data['template'].endswith('\\' + family)
        input_pins.append(dict(path=str(path), sha256=sha(raw)))
        for row in data['materials']:
            authored_rows += 1
            vectors = bytes.fromhex(row['source_banks']['20']['hex'])
            assert len(vectors) == row['source_banks']['20']['count'] * 16
            for index, vector in enumerate(struct.iter_unpack('<4I', vectors)):
                r, g, b, a = vector
                cases.append(('authored', [a, r, g, b]))
            if family == 'tex_bump_env_tiling_specular' and row['native_pair']:
                native = bytes.fromhex(row['native_banks']['12']['hex'])
                inputs = list(struct.iter_unpack('<4I', vectors))
                checks = []
                for native_slot, source_slot in ((1, 6), (2, 7), (3, 8), (4, 9)):
                    r, g, b, a = inputs[source_slot]
                    entry = 0x1349e0 if native_slot < 3 else 0x134ad0
                    actual = original(entry, [a, r, g, b])
                    expected = struct.unpack_from('<I', native, native_slot * 4)[0]
                    assert actual == expected, (row['shader'], native_slot, hex(actual), hex(expected))
                    checks.append(dict(native_pixel=native_slot, source_vector=source_slot,
                                       original_entry=hex(entry), original_result=hex(actual), exact=True))
                paired.append(dict(map=row['map'], shader=row['shader'], checks=checks))
    for channel in range(4):
        for byte in range(255):
            midpoint = bits((byte + .5) / 255)
            for value in (midpoint - 1, midpoint, midpoint + 1):
                argb = [bits(.25), bits(.5), bits(.75), bits(1)]
                argb[channel] = value
                cases.append(('adjacent byte midpoint', argb))
        controls = [0, 1, 0x007fffff, 0x00800000, 0x80000000, 0x80000001,
                    0x7f800000, 0xff800000, 0x7fc12345, 0x7f812345,
                    0x7f7fffff, 0xff7fffff]
        controls += [bits(value) for value in (-2, -1, -.5, 1.01, 2, 128, 256, 1e7, -1e7)]
        for value in controls:
            argb = [bits(.25), bits(.5), bits(.75), bits(1)]
            argb[channel] = value
            cases.append(('domain and invalid control', argb))
    rows_hash, result_hash = hashlib.sha256(), hashlib.sha256()
    comparisons, proposal_comparisons = 0, 0
    fixture_cases = []
    for label, argb in cases:
        rows_hash.update(words(argb))
        original_results = {}
        for entry, _ in ENTRIES:
            actual = original(entry, argb)
            expected = proposal(entry, argb)
            assert actual == expected, (label, hex(entry), argb, hex(actual), hex(expected))
            if channel_proposal and all((value >> 23) & 255 != 255 for value in argb):
                proposed = proposal(entry, argb, channel_proposal)
                assert actual == proposed, (label, hex(entry), argb, hex(actual), hex(proposed))
                proposal_comparisons += 1
            result_hash.update(words([actual]))
            original_results[entry] = actual
            comparisons += 1
        fixture_cases.append(dict(argb_words=argb, original_rgb=original_results[0x1349e0],
            original_alpha=original_results[0x134ad0], finite=all((value >> 23) & 255 != 255 for value in argb)))
    assert all(visited[entry] == body['sites'] for entry, body in bodies.items())
    report = dict(scope=__doc__, passed=True, TU15_sha256=NEW_PIN,
        helper_sha256=sha(Path(__file__).read_bytes()), authored_materials=authored_rows,
        cases=len(cases), original_comparisons=comparisons, input_sha256=rows_hash.hexdigest(),
        output_sha256=result_hash.hexdigest(), input_pins=input_pins, exact_paired_pixel_checks=paired,
        original_bodies={hex(entry): dict(sites=len(body['sites']), body_sha256=body['body_sha256'],
            all_sites_observed=visited[entry] == body['sites']) for entry, body in bodies.items()},
        FPCW='027F', MXCSR='1F80', clamp_applied=False, original_code_patched=False,
        finite_channel_proposal_comparisons=proposal_comparisons,
        channel_proposal=(dict(path=str(args.channel_proposal), sha256=sha(args.channel_proposal.read_bytes()))
                          if args.channel_proposal else None),
        native_material_projection_admitted=False, owned_scratch=str(owner), retained_large_artifacts=[])
    if args.fixture_output:
        assert args.fixture_output.resolve().is_relative_to(owner)
        fixture = dict(schema='H2_TU15_COLOR_PACKING_CASES_V1', TU15_sha256=NEW_PIN,
            FPCW='027F', original_code_patched=False, original_bodies=report['original_bodies'], cases=fixture_cases)
        fixture_raw = (json.dumps(fixture, separators=(',', ':')) + '\n').encode()
        assert len(fixture_raw) <= 1024 * 1024
        args.fixture_output.write_bytes(fixture_raw)
        report['owned_packing_fixture'] = dict(path=str(args.fixture_output), bytes=len(fixture_raw), sha256=sha(fixture_raw))
    raw = json.dumps(report, indent=2) + '\n'
    assert len(raw.encode()) < 1024 * 1024
    args.report.write_text(raw)
    print(json.dumps(dict(passed=True, materials=authored_rows, cases=len(cases),
                          original_comparisons=comparisons, paired_materials=len(paired))))


if __name__ == '__main__':
    main()

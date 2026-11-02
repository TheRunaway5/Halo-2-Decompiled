"""Execute unchanged TU1.5 static vertex remapping and observe its masks.

All defined component modes, both banks, every register and mixed ranges are
tested with literal independent source/initial banks. No instructions or calls
are substituted. This proves static lane copying; it does not qualify shader
animation, resource roles or GPU arithmetic. Run through run_test.py.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import random
import struct

from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn import x86_const as R


ENTRY = 0x16fe0
STATE, DEFINITION, IMPLEMENTATION, VECTORS, MAPPINGS, RANGE = [0x2100000 + i * 0x10000 for i in range(6)]
STACK, STOP = 0x22fef00, 0x3000000


def words(values):
    return struct.pack('<' + 'I' * len(values), *(value & 0xffffffff for value in values))


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    owner = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    image = Xbe(NEW_DEFAULT)
    assert sha(image.data) == NEW_PIN
    code = _Code(image, '.text')
    body = _trace(code, ENTRY, {ENTRY})
    assert len(body.code) == 101 and not body.calls and not body.tail_jumps
    returns = [code.at(address) for address in body.code if code.at(address).mnemonic == 'ret']
    assert len(returns) == 1 and returns[0].bytes.hex() == 'c20400'
    assert [(width, count) for _, width, count in body.tables] == [(4, 9), (1, 21)]
    guest = Uc(UC_ARCH_X86, UC_MODE_32)
    guest.mem_map(0x10000, 0x800000)
    for section in image.sections:
        guest.mem_write(section.va, image.section_bytes(section))
    guest.mem_map(STATE, 0x200000)
    guest.mem_map(STOP, 0x1000)
    seen = set()

    def trace(_guest, address, _size, _data):
        assert address in body.code, hex(address)
        seen.add(address)

    guest.hook_add(UC_HOOK_CODE, trace)
    rng = random.Random(0x16fe017000)
    masks, cases = {}, 0
    digest = hashlib.sha256()

    def original(initial, source, mappings, first=0):
        guest.mem_write(STATE, bytes(0x1800))
        guest.mem_write(STATE + 12, words([DEFINITION, IMPLEMENTATION]))
        guest.mem_write(DEFINITION + 0x18, words([VECTORS]))
        guest.mem_write(IMPLEMENTATION + 0x24, words([MAPPINGS]))
        guest.mem_write(STATE + 0x1530, words(initial))
        guest.mem_write(VECTORS, words(source))
        guest.mem_write(MAPPINGS, words(mappings))
        guest.mem_write(RANGE, struct.pack('<H', first | ((len(mappings) - first) << 9)))
        guest.mem_write(STACK, words([STOP, RANGE]))
        guest.reg_write(R.UC_X86_REG_ESP, STACK)
        guest.reg_write(R.UC_X86_REG_EAX, STATE)
        guest.reg_write(R.UC_X86_REG_FPCW, 0x027f)
        guest.reg_write(R.UC_X86_REG_MXCSR, 0x1f80)
        preserved = ((R.UC_X86_REG_EBX, 0x12345678), (R.UC_X86_REG_EBP, 0xabcdef01),
                     (R.UC_X86_REG_ESI, 0x98765432), (R.UC_X86_REG_EDI, 0x10203040))
        for reg, value in preserved:
            guest.reg_write(reg, value)
        guest.emu_start(ENTRY, STOP, count=20000)
        assert guest.reg_read(R.UC_X86_REG_EIP) == STOP
        assert guest.reg_read(R.UC_X86_REG_ESP) == STACK + 8
        assert all(guest.reg_read(reg) == value for reg, value in preserved)
        assert bytes(guest.mem_read(VECTORS, len(source) * 4)) == words(source)
        output = list(struct.unpack('<128I', guest.mem_read(STATE + 0x1530, 512)))
        dirty = list(struct.unpack('<2I', guest.mem_read(STATE + 0x1730, 8)))
        return output, dirty

    # Discover each original write mask using distinct input/sentinel words.
    for mode in range(21):
        initial = [0xa5a50000 + index for index in range(128)]
        source = [0x3f000000 + index for index in range(1024)]
        output, dirty = original(initial, source, [mode << 5])
        changed = [index for index, (old, new) in enumerate(zip(initial, output)) if old != new]
        assert all(index < 4 and output[index] == source[index] for index in changed)
        assert dirty == [1, 0]
        masks[mode] = sum(1 << index for index in changed)
    assert masks[9] == 3 and masks[10] == 12 and masks[13] == 15
    mappings_cases = [[]]
    for mode in range(21):
        for bank in range(2):
            for register in range(16):
                source = rng.randrange(256)
                mappings_cases.append([(source << 24) | (rng.randrange(64) << 10) | (mode << 5) | (bank << 4) | register])
    for _ in range(512):
        mappings_cases.append([(rng.randrange(256) << 24) | (rng.randrange(64) << 10) |
            (rng.randrange(21) << 5) | (rng.randrange(2) << 4) | rng.randrange(16)
            for _ in range(rng.randrange(1, 48))])
    for mappings in mappings_cases:
        initial = [rng.getrandbits(32) for _ in range(128)]
        source = [rng.getrandbits(32) for _ in range(1024)]
        first = rng.randrange(len(mappings) + 1)
        output, dirty = original(initial, source, mappings, first)
        expected, expected_dirty = list(initial), [0, 0]
        for mapping in mappings[first:]:
            mode, bank, register, source_slot = (mapping >> 5) & 31, (mapping >> 4) & 1, mapping & 15, mapping >> 24
            for lane in range(4):
                if masks[mode] & (1 << lane):
                    expected[(bank * 16 + register) * 4 + lane] = source[source_slot * 4 + lane]
            expected_dirty[bank] |= 1 << register
        assert output == expected and dirty == expected_dirty
        digest.update(words([*initial, *source, len(mappings), first, *mappings, *output, *dirty]))
        cases += 1
    assert seen == set(body.code), sorted(set(body.code) - seen)
    report = dict(scope=__doc__, passed=True, TU15_sha256=NEW_PIN, entry=hex(ENTRY),
        helper_sha256=sha(Path(__file__).read_bytes()), instruction_sites=len(body.code), all_sites_observed=True,
        body_sha256=sha(b''.join(image.read(address, size) for address, size in sorted(body.code.items()))),
        tables=[dict(address=hex(address), width=width, entries=count, sha256=sha(image.read(address, width * count)))
                for address, width, count in body.tables], masks=masks, discovery_cases=21,
        mixed_and_boundary_cases=cases, compared_register_words=cases * 128, combined_digest=digest.hexdigest(),
        original_code_patched=False, original_calls_intercepted=False, GPU_qualified=False,
        callee_stack_argument_bytes=4,
        owned_scratch=str(owner), retained_large_artifacts=[])
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(passed=True, masks=masks, cases=cases, all_sites_observed=True)))


if __name__ == '__main__':
    main()

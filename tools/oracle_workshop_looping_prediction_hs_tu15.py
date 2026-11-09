"""Record complete unchanged HS-to-looping-prediction/cache compositions.

Original211B10,212010,2B3E10 and211DA0 run together with real prediction and
cache routines. No call is intercepted and no result is supplied. Explicit
thread/syntax fixtures use the already independently checked sound-cache
inputs. This does not qualify MCC delivery, cold streaming or a mission.
Run through run_test.py; raw page references belong to HALO2_TEST_SCRATCH.
"""
import argparse
from collections import Counter
import hashlib
import json
import os
from pathlib import Path
import struct
import zlib

from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from audit_workshop_looping_prediction_tu15 import DATA, SIZE, STOP


GP = (R.UC_X86_REG_EAX, R.UC_X86_REG_ECX, R.UC_X86_REG_EDX, R.UC_X86_REG_EBX,
      R.UC_X86_REG_ESP, R.UC_X86_REG_EBP, R.UC_X86_REG_ESI, R.UC_X86_REG_EDI)
STACK, THREAD, FRAME = 0x60008000, 0x90003000, 0x90003040
POOL, SYNTAX_POOL, NODES, OUTPUT = 0x90002000, 0x90002100, 0x90004000, 0x90007000


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    owner = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert args.output.resolve().is_relative_to(owner) and not args.output.exists()
    assert not args.report.resolve().is_relative_to(owner)
    reference_path = root / 'tests/fixtures/workshop_looping_prediction.json.zlib'
    descriptor = json.loads(reference_path.with_suffix('').read_text())
    packed = reference_path.read_bytes()
    assert len(packed) == descriptor['packed_bytes'] and sha(packed) == descriptor['packed_sha256']
    raw = zlib.decompress(packed)
    assert len(raw) == descriptor['decoded_bytes'] and sha(raw) == descriptor['decoded_sha256']
    reference = json.loads(raw)
    assert reference['xbe_sha256'] == NEW_PIN and len(reference['cases']) == 11
    image = Xbe(NEW_DEFAULT)
    assert sha(image.data) == NEW_PIN
    assert int.from_bytes(image.read(0x480c00 + 605 * 4, 4), 'little') == 0x459998
    assert image.read(0x459998, 16).hex() == '04000000103e2b000000000001002300'
    guest = Uc(UC_ARCH_X86, UC_MODE_32)
    for at, size in ((0x10000, 0x800000), (DATA, SIZE), (0x90000000, 0x10000),
                     (0x60000000, 0x10000), (STOP, 0x1000)):
        guest.mem_map(at, size)
    initial_pages, trace, entries, parser_results = {}, [], Counter(), []
    watched = (0x211b10, 0x212010, 0x2b3e10, 0x211da0, 0x18d1d0, 0x128a20, 0x21f260)

    def instruction(_guest, pc, size, _data):
        assert bytes(guest.mem_read(pc, size)) == image.read(pc, size), f'Changed original code {pc:#x}'
        trace.append(pc)
        if pc in watched:
            entries[pc] += 1
        if pc == 0x2b3e35:
            parser_results.append(guest.reg_read(R.UC_X86_REG_EAX))

    def access(_guest, _kind, address, size, _value, _data):
        for page in range(address & ~4095, (address + size - 1 & ~4095) + 4096, 4096):
            if page not in initial_pages:
                initial_pages[page] = bytes(guest.mem_read(page, 4096))

    guest.hook_add(UC_HOOK_CODE, instruction)
    guest.hook_add(UC_HOOK_MEM_READ, access)
    guest.hook_add(UC_HOOK_MEM_WRITE, access)

    def put(at, value, size=4):
        guest.mem_write(at, (value & ((1 << (size * 8)) - 1)).to_bytes(size, 'little'))

    def node(index, opcode, kind, flags, next_, value):
        guest.mem_write(NODES + index * 20,
                        struct.pack('<4H3I', 0x9000 + index, opcode, kind, flags, next_, 0, value))

    def cpu():
        return dict(gp=[guest.reg_read(register) for register in GP],
            flags=guest.reg_read(R.UC_X86_REG_EFLAGS),
            xmm=[list(struct.unpack('<4I', guest.reg_read(R.UC_X86_REG_XMM0 + i).to_bytes(16, 'little')))
                 for i in range(8)])

    rows, cases, page_images, page_ids = [], [], [], {}
    all_sites, aggregate_entries = set(), Counter()
    for base in reference['cases']:
        guest.mem_write(0x10000, bytes(0x800000))
        for section in image.sections:
            guest.mem_write(section.va, image.section_bytes(section))
        guest.mem_write(DATA, bytes(SIZE))
        guest.mem_write(0x90000000, bytes(0x10000))
        guest.mem_write(0x60000000, bytes(0x10000))
        guest.mem_write(0x502248, bytes(0x3c10))
        for offset, values in base['initial_patches']:
            guest.mem_write(DATA + offset, bytes.fromhex(values))
        for at, values in reference['globals']:
            guest.mem_write(at, bytes.fromhex(values))
        put(0x505e5c, POOL)
        put(0x505e60, 1)
        put(0x505e6c, SYNTAX_POOL)
        put(0x4f3218, 0)
        put(POOL + 0x44, THREAD)
        put(SYNTAX_POOL + 0x20, 3)
        put(SYNTAX_POOL + 0x24, 20)
        put(SYNTAX_POOL + 0x44, NODES)
        node(0, 605, 4, 8, 0xffffffff, 0x90010001)
        node(1, 2, 2, 9, 0x90020002, 0)
        node(2, 35, 35, 9, 0xffffffff, base['initial_gp'][0])
        put(THREAD + 2, 1, 1)
        put(THREAD + 3, 1, 1)
        put(THREAD + 0x10, FRAME)
        put(FRAME, THREAD + 0x18)
        put(FRAME + 4, 0x90000000)
        put(THREAD + 0x20, OUTPUT)
        put(OUTPUT, 0xdeadbeef)
        put(THREAD + 0xc, 0x100, 2)
        put(FRAME + 0xc, 0, 2)
        put(STACK, STOP)
        gp = [0x12345678, 0x23456789, 0x3456789a, 0x456789ab, STACK,
              0x56789abc, 0x6789abcd, 0xa0010000]
        for register, value in zip(GP, gp):
            guest.reg_write(register, value)
        guest.reg_write(R.UC_X86_REG_EFLAGS, 0x202)
        guest.reg_write(R.UC_X86_REG_FPCW, 0x27f)
        guest.reg_write(R.UC_X86_REG_MXCSR, 0x1f80)
        guest.reg_write(R.UC_X86_REG_FPSW, 0x1800)
        for register, value in ((3, (0x8000000000000000, 0x3ffe)),
                                (4, (0x8000000000000000, 0xbfff)),
                                (5, (0x8000000000000000, 0x4000))):
            guest.reg_write(R.UC_X86_REG_FP0 + register, value)
        guest.reg_write(R.UC_X86_REG_FPTAG, 0xf03f)
        for index, lanes in enumerate(base['initial_xmm']):
            guest.reg_write(R.UC_X86_REG_XMM0 + index,
                            int.from_bytes(struct.pack('<4I', *lanes), 'little'))
        fp_regs = (R.UC_X86_REG_FPCW, R.UC_X86_REG_FPSW, R.UC_X86_REG_FPTAG, R.UC_X86_REG_MXCSR)
        before_fp = [guest.reg_read(r) for r in fp_regs]
        before_physical = [guest.reg_read(R.UC_X86_REG_FP0 + i) for i in range(8)]
        initial_cpu = cpu()
        initial_pages.clear()
        trace.clear()
        entries.clear()
        parser_results.clear()
        guest.emu_start(0x212470, STOP, count=10000)
        assert guest.reg_read(R.UC_X86_REG_EIP) == STOP
        initializer_cpu = cpu()
        # The dispatcher is a separate real CPU call, as in the production
        # component tests; preserve all initialized scalar storage in memory.
        for register, value in zip(GP, gp):
            guest.reg_write(register, value)
        guest.reg_write(R.UC_X86_REG_EFLAGS, 0x202)
        guest.emu_start(0x211b10, STOP, count=200000)
        assert guest.reg_read(R.UC_X86_REG_EIP) == STOP
        assert guest.reg_read(R.UC_X86_REG_ESP) == STACK + 4
        assert int.from_bytes(guest.mem_read(OUTPUT, 4), 'little') == 0
        assert int.from_bytes(guest.mem_read(THREAD + 0x10, 4), 'little') == THREAD + 0x18
        assert entries[0x21f260] == base['genuine_request_calls']
        assert entries[0x18d1d0] == 1 and entries[0x211da0] == 1
        assert entries[0x212010] >= 1 and entries[0x2b3e10] >= 1
        assert parser_results and any(parser_results)
        assert [guest.reg_read(r) for r in fp_regs] == before_fp
        assert [guest.reg_read(R.UC_X86_REG_FP0 + i) for i in range(8)] == before_physical
        assert cpu()['xmm'] == initial_cpu['xmm']
        # Independently recorded direct-effect banks contain exactly the
        # same cache touches. Their old private stack is a separate region.
        expected = bytearray(SIZE)
        for offset, values in base['expected_patches']:
            expected[offset:offset + len(bytes.fromhex(values))] = bytes.fromhex(values)
        assert bytes(guest.mem_read(DATA, SIZE - 0x2000)) == bytes(expected[:-0x2000])
        final_pages = {at: bytes(guest.mem_read(at, 4096)) for at in initial_pages}
        inputs, changes = [], []
        for address, before in sorted(initial_pages.items()):
            if before not in page_ids:
                page_ids[before] = len(page_images)
                page_images.append(before.hex())
            inputs.append([address, page_ids[before]])
            after = final_pages[address]
            first = None
            for offset in range(4097):
                changed = offset < 4096 and before[offset] != after[offset]
                if changed and first is None:
                    first = offset
                elif not changed and first is not None:
                    changes.append([address + first, after[first:offset].hex()])
                    first = None
        cases.append(dict(name=base['name'], initial_cpu=initial_cpu, initializer_cpu=initializer_cpu,
            expected_cpu=cpu(), expected_cache_requests=base['genuine_request_calls'],
            initial_pages=inputs, expected_page_changes=changes,
            trace=trace.copy(), parser_results=parser_results.copy()))
        rows.append(dict(name=base['name'], pages=len(initial_pages), instructions=len(trace),
                         calls={hex(at): count for at, count in sorted(entries.items())},
                         parsed_returns=len([v for v in parser_results if v]),
                         deferred_returns=parser_results.count(0)))
        all_sites.update(trace)
        aggregate_entries.update(entries)
    fixture = dict(scope=__doc__, xbe_sha256=NEW_PIN,
        source_fixture_sha256=descriptor['packed_sha256'], original_code_patched=False,
        original_calls_intercepted=False, supplied_callee_results=False, page_images=page_images, cases=cases)
    raw = (json.dumps(fixture, separators=(',', ':')) + '\n').encode()
    assert len(raw) < 4 * 1024 * 1024
    packed = zlib.compress(raw, 9)
    descriptor = dict(packed_bytes=len(packed), packed_sha256=sha(packed),
                      decoded_bytes=len(raw), decoded_sha256=sha(raw))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(packed)
    args.output.with_suffix('').write_text(json.dumps(descriptor, indent=2) + '\n')
    report = dict(scope=__doc__, passed=True, helper_sha256=sha(Path(__file__).read_bytes()),
        xbe_sha256=NEW_PIN, fixture=descriptor, cases=rows, observed_instruction_sites=len(all_sites),
        entry_calls={hex(at): count for at, count in sorted(aggregate_entries.items())},
        original_code_patched=False, original_calls_intercepted=False, supplied_callee_results=False,
        native_execution_qualified=False, MCC_projection_and_streaming_qualified=False,
        production_admission_changed=False, complete_mod_support=False,
        owned_scratch=str(owner), retained_large_artifacts=[])
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(passed=True, cases=len(cases), entry_calls=report['entry_calls'],
                         packed_reference_bytes=len(packed))))


if __name__ == '__main__':
    main()

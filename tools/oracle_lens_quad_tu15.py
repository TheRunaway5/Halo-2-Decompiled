#!/usr/bin/env python3
"""Execute untouched TU1.5 lens query quads with actual authored definitions.

The declared camera is projected by whole original2FD70. The callback2E520,
projection48AF0, rounding helpers and reached SDK begin/end execute unchanged.
A bounded clean SDK command bank records original output; no query result is
invented and no original instruction or service is replaced. GPU consumption
and presentation remain separate native owners.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_READ
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--requests', type=Path, default=Path('tests/fixtures/lens_request_tu15_original.json'))
    p.add_argument('--fixture', type=Path, required=True)
    p.add_argument('--report', type=Path, required=True)
    a = p.parse_args()
    assert not a.fixture.exists() and not a.report.exists(), 'Preserve earlier evidence'
    requests = json.loads(a.requests.read_text())
    assert requests['original_sha256'] == NEW_PIN
    x = Xbe(NEW_DEFAULT)
    assert hashlib.sha256(x.data).hexdigest() == NEW_PIN
    u = Uc(UC_ARCH_X86, UC_MODE_32)
    u.mem_map(0x10000, 0x800000)
    for section in x.sections:
        u.mem_write(section.va, x.section_bytes(section))
    original = bytes(u.mem_read(0x10000, 0x800000))
    u.mem_map(0x2000000, 0x100000)
    u.mem_map(0x2400000, 0x300000)
    u.mem_map(0x2800000, 0x1000)
    camera, projection, driver, fifo = 0x2000000, 0x2001000, 0x2002000, 0x2003000
    stack, stop, record = 0x2600000, 0x2800000, 0x52dfdc
    regs = [R.UC_X86_REG_EAX, R.UC_X86_REG_ECX, R.UC_X86_REG_EDX,
            R.UC_X86_REG_EBX, R.UC_X86_REG_ESP, R.UC_X86_REG_EBP,
            R.UC_X86_REG_ESI, R.UC_X86_REG_EDI]
    trace, reads = [], set()
    recording = False

    def instruction(cpu, pc, size, _):
        assert bytes(cpu.mem_read(pc, size)) == x.read(pc, size)
        if recording:
            trace.append(pc)

    def read(cpu, kind, address, size, value, _):
        if recording and address < 0x800000:
            reads.update(range(address & ~4095, (address + size + 4095) & ~4095, 4096))

    u.hook_add(UC_HOOK_CODE, instruction)
    u.hook_add(UC_HOOK_MEM_READ, read)

    def initialize(gp):
        for reg, value in zip(regs, gp):
            u.reg_write(reg, value)
        for n in range(8):
            u.reg_write(R.UC_X86_REG_XMM0 + n, 0)
        for reg, value in ((R.UC_X86_REG_EFLAGS, 0x202), (R.UC_X86_REG_FPCW, 0x27f),
                           (R.UC_X86_REG_FPSW, 0), (R.UC_X86_REG_FPTAG, 0xffff),
                           (R.UC_X86_REG_MXCSR, 0x1f80)):
            u.reg_write(reg, value)

    banks, bank_ids, cases = [], {}, []

    def reference(address, data):
        key = address, data.hex()
        if key not in bank_ids:
            bank_ids[key] = len(banks)
            banks.append(dict(address=address, hex=key[1]))
        return bank_ids[key]

    for request in requests['cases']:
        if request['variant'] != 'accepted':
            continue
        for name, position, rectangle in (
            ('near', (1, 0, 0), (0, 0, 480, 640)),
            ('far', (50, 0, 0), (0, 0, 480, 640)),
            ('behind', (-1, 0, 0), (0, 0, 480, 640)),
            ('off-axis', (5, 3, 1), (0, 0, 480, 640)),
            ('split', (5, 0, 0), (240, 320, 480, 640)),
        ):
            recording = False
            u.mem_write(0x10000, original)
            # These are caller camera fields, not a substituted projection.
            raw = bytearray(0x74)
            struct.pack_into('<9f', raw, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1)
            struct.pack_into('<f', raw, 0x28, 1.2217304763960306)
            struct.pack_into('<8h', raw, 0x30, *rectangle, *rectangle)
            struct.pack_into('<2f', raw, 0x40, .015625, 1024)
            u.mem_write(camera, bytes(raw))
            u.mem_write(stack + 0x4000, struct.pack('<I', stop))
            gp = [0] * 8
            gp[3], gp[4], gp[6] = projection, stack + 0x4000, camera
            initialize(gp)
            u.emu_start(0x2fd70, stop, count=100000)
            assert u.reg_read(R.UC_X86_REG_EIP) == stop
            projected = bytes(u.mem_read(projection, 0xc0))
            for index in request['initial_banks']:
                bank = requests['banks'][index]
                u.mem_write(bank['address'], bytes.fromhex(bank['hex']))
            for index in request['final_banks']:
                bank = requests['banks'][index]
                if bank['address'] == 0x52d000:
                    u.mem_write(bank['address'], bytes.fromhex(bank['hex']))
            u.mem_write(record + 12, struct.pack('<3f', *position))
            u.mem_write(0x491e38, bytes(raw))
            u.mem_write(0x491eac, projected)
            u.mem_write(0x411568, struct.pack('<I', driver))
            u.mem_write(0x411570, bytes(4))
            u.mem_write(driver, struct.pack('<3I', fifo, fifo + 0xf00, 0))
            u.mem_write(fifo, bytes(0x1000))
            scratch = bytearray([0xcd]) * 0x8000
            struct.pack_into('<3I', scratch, 0x4000, stop, record, record + 8)
            u.mem_write(stack, bytes(scratch))
            gp = [0x72000000 + n * 0x1000 for n in range(8)]
            gp[4] = stack + 0x4000
            initialize(gp)
            # Freeze the complete inputs before the original callback runs.
            high = [(bank['address'], bytes(u.mem_read(bank['address'], len(bytes.fromhex(bank['hex'])))))
                    for bank in (requests['banks'][i] for i in request['initial_banks'])
                    if bank['address'] >= 0x800000 and bank['address'] != stack]
            high += [(driver, bytes(u.mem_read(driver, 12))), (fifo, bytes(0x1000)), (stack, bytes(scratch))]
            before = bytes(u.mem_read(0x10000, 0x800000))
            trace.clear()
            reads.clear()
            recording = True
            u.emu_start(0x2e520, stop, count=100000)
            recording = False
            assert u.reg_read(R.UC_X86_REG_EIP) == stop
            low = [(page, before[page-0x10000:page-0x10000+4096]) for page in sorted(reads)]
            end = struct.unpack('<I', u.mem_read(driver, 4))[0]
            assert fifo <= end <= fifo + 0xf00
            final = [(driver, bytes(u.mem_read(driver, 12))),
                     (fifo, bytes(u.mem_read(fifo, 0x1000))),
                     (stack + 0x4000, bytes(u.mem_read(stack + 0x4000, 12)))]
            cases.append(dict(map=request['map'], tag=request['tag'], variant=name,
                entry=0x2e520, entry_gp=gp, exit_gp=[u.reg_read(reg) for reg in regs],
                eflags=u.reg_read(R.UC_X86_REG_EFLAGS), eflags_mask=0xcc5,
                exit_xmm=[f'{u.reg_read(R.UC_X86_REG_XMM0+n):032x}' for n in range(8)],
                x87_status=u.reg_read(R.UC_X86_REG_FPSW), mxcsr=u.reg_read(R.UC_X86_REG_MXCSR),
                instructions=trace.copy(), initial_banks=[reference(p, b) for p, b in low + high],
                final_banks=[reference(p, b) for p, b in final],
                fifo_bytes=end-fifo, sample_area=u.reg_read(R.UC_X86_REG_EAX)))
    assert len(cases) == 45 and any(c['fifo_bytes'] == 96 for c in cases)
    a.fixture.parent.mkdir(parents=True, exist_ok=True)
    a.report.parent.mkdir(parents=True, exist_ok=True)
    a.fixture.write_text(json.dumps(dict(entry=0x2e520, original_sha256=NEW_PIN,
        banks=banks, cases=cases), separators=(',', ':')) + '\n')
    a.report.write_text(json.dumps(dict(passed=True, original_sha256=NEW_PIN,
        cases=len(cases), quad_cases=sum(c['fifo_bytes'] == 96 for c in cases),
        instruction_patches=[], replaced_services=[], camera_producer='0002fd70',
        requests_sha256=hashlib.sha256(a.requests.read_bytes()).hexdigest(),
        fixture_sha256=hashlib.sha256(a.fixture.read_bytes()).hexdigest(),
        generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        full_goal_complete=False, boundary=__doc__), indent=2) + '\n')
    print('PASS original lens query callback:', len(cases), 'cases,',
          sum(c['fifo_bytes'] == 96 for c in cases), 'quads')


if __name__ == '__main__':
    main()

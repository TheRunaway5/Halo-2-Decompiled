#!/usr/bin/env python3
"""Original TU1.5 GP upload/allocation callers with declared resource contexts.

Existing decoder fixtures remain independent inputs. New cases execute whole
388309/38DE81 callers to natural returns or their first actual external call.
No allocation return, source acknowledgement, hardware register or ready flag
is supplied. Descriptor relocation and the later GP wait need composed tests.
"""
import argparse
import hashlib
import json
import shutil
import struct
from pathlib import Path
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE, UC_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--decoder-fixtures', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if args.output.exists():
        raise ValueError('Preserve prior upload evidence; use a fresh output')
    original = json.loads((args.decoder_fixtures / 'report.json').read_text())
    assert original['suite'] == 'audio_dsp' and original['xbe_sha256'] == NEW_PIN
    assert original['instruction_patches'] == original['supplied_external_results'] == []
    image = Xbe(NEW_DEFAULT)
    assert hashlib.sha256(image.data).hexdigest() == NEW_PIN
    resource = image.section_bytes(next(s for s in image.sections if s.name == 'DSPImage'))
    assert hashlib.sha256(resource).hexdigest() == original['dsp_section_sha256']
    args.output.mkdir(parents=True)
    for case in original['cases']:
        shutil.copyfile(args.decoder_fixtures / case['file'], args.output / case['file'])
    machine = Uc(UC_ARCH_X86, UC_MODE_32)
    machine.mem_map(0x10000, 0x600000)
    for section in image.sections:
        machine.mem_write(section.va, image.section_bytes(section))
    base, stack, stop = 0x1000000, 0x10f0000, 0x10fe000
    machine.mem_map(base, 0x100000)
    buffer, context, scratch, table = base + 0x1000, base + 0x9000, base + 0xa000, base + 0xa100
    gp_names = ('EAX', 'ECX', 'EDX', 'EBX', 'ESP', 'EBP', 'ESI', 'EDI')
    gp = lambda: [machine.reg_read(getattr(R, 'UC_X86_REG_' + name)) for name in gp_names]
    xmm = lambda: [list(struct.unpack('<4I', machine.reg_read(getattr(R, f'UC_X86_REG_XMM{i}')).to_bytes(16, 'little'))) for i in range(8)]
    initial, executed, cases, added = {}, set(), list(original['cases']), []
    digest, store_count, boundary = None, 0, None

    def word(address, value):
        machine.mem_write(address, struct.pack('<I', value))

    def instruction(m, pc, size, _):
        if pc == boundary:
            m.emu_stop()
            return
        assert bytes(m.mem_read(pc, size)) == image.read(pc, size)
        executed.add(pc)

    def access(m, kind, address, size, value, _):
        nonlocal store_count
        before = bytes(m.mem_read(address, size))
        for index, byte in enumerate(before):
            initial.setdefault(address + index, byte)
        if kind == UC_MEM_WRITE:
            written = (value & ((1 << (8 * size)) - 1)).to_bytes(size, 'little')
            digest.update(struct.pack('<3I', m.reg_read(R.UC_X86_REG_EIP), address, size))
            digest.update(before)
            digest.update(written)
            store_count += 1

    machine.hook_add(UC_HOOK_CODE, instruction)
    machine.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, access)

    def banks():
        out, start, last, data = {}, None, None, bytearray()
        for address, byte in sorted(initial.items()):
            if last is not None and address != last + 1:
                out[f'{start:08x}'] = data.hex()
                start, data = None, bytearray()
            if start is None:
                start = address
            data.append(byte)
            last = address
        if start is not None:
            out[f'{start:08x}'] = data.hex()
        return out

    def run(name, entry, requested, reserved, retained, external=None, prior_descriptor=0):
        nonlocal digest, store_count, boundary
        machine.mem_write(base, bytes(0x10000))
        machine.mem_write(buffer, resource)
        word(context + 8, scratch)
        word(context + 0x20, prior_descriptor)
        word(scratch, reserved + 8)
        word(scratch + 4, reserved)
        word(scratch + 0x10, table)
        word(table, buffer)
        word(scratch + 0x14, base + 0xb000 if retained else 0)
        word(scratch + 0x1c, retained)
        machine.mem_write(stack, struct.pack('<2I', stop, requested))
        for index, register in enumerate(gp_names):
            machine.reg_write(getattr(R, 'UC_X86_REG_' + register), 0x12340000 + index)
        machine.reg_write(R.UC_X86_REG_ECX, context if entry == 0x388309 else scratch)
        machine.reg_write(R.UC_X86_REG_ESP, stack)
        for register, value in ((R.UC_X86_REG_EFLAGS, 2), (R.UC_X86_REG_FPCW, 0x27f), (R.UC_X86_REG_FPSW, 0), (R.UC_X86_REG_FPTAG, 0xffff), (R.UC_X86_REG_MXCSR, 0x1f80)):
            machine.reg_write(register, value)
        for index in range(8):
            machine.reg_write(getattr(R, f'UC_X86_REG_XMM{index}'), 0)
        before_gp, before_xmm = gp(), xmm()
        initial.clear()
        digest, store_count, boundary = hashlib.sha256(), 0, external
        machine.emu_start(entry, stop, count=100000)
        assert machine.reg_read(R.UC_X86_REG_EIP) == (external or stop), name
        before = banks()
        after = {address: bytes(machine.mem_read(int(address, 16), len(raw) // 2)).hex() for address, raw in before.items()}
        row = dict(name=name, entry=entry, gp=before_gp, xmm=before_xmm, gp_after=gp(), xmm_after=xmm(), eflags_after=machine.reg_read(R.UC_X86_REG_EFLAGS), defined_flags_mask=0xcc5,
                   before=before, after=after, store_sha256=digest.hexdigest(), store_count=store_count,
                   store_hash_encoding='LE32 pc,address,extent followed by before and written bytes in execution order',
                   external_targets=[] if external is None else [external], external_boundary=[] if external is None else [external],
                   declared_context=dict(requested_bytes=requested, reserved_pages=reserved, retained_bytes=retained, prior_descriptor=prior_descriptor))
        filename = f'{len(cases):03d}-{name}.json'
        (args.output / filename).write_text(json.dumps(row, separators=(',', ':')) + '\n')
        cases.append(dict(file=filename))
        added.append(dict(file=filename, entry=entry, external=external, stores=store_count))

    for requested in (0, 1, 0x1000, 0x3fff, 0x4000):
        run(f'retained_{requested:04x}', 0x38de81, requested, 4, 0x4000)
    for requested in (0x5000, 0x10000):
        run(f'exceeds_reservation_{requested:x}', 0x38de81, requested, 4, 0x4000)
    for requested, reserved, retained in ((1, 0, 0), (0x3000, 4, 0x1000), (0x3fff, 4, 0x1000)):
        run(f'allocation_boundary_{requested:x}', 0x38de81, requested, reserved, retained, 0x38796c)
    run('descriptor_allocation_boundary', 0x388309, 0, 4, 0x4000, 0x387a32)
    run('descriptor_release_boundary', 0x388309, 0, 4, 0x4000, 0x387a9a, base + 0xc000)
    original.update(scope=__doc__, cases=cases, upload_cases=added, decoder_fixture_report_sha256=hashlib.sha256((args.decoder_fixtures / 'report.json').read_bytes()).hexdigest(),
                    executed_upload_instruction_addresses=sorted(executed), uploaded_descriptor_relocation_executed=False, gp_acknowledgement_supplied=False, full_engine_verified=False)
    (args.output / 'report.json').write_text(json.dumps(original, indent=2) + '\n')
    print(f'PASS {len(added)} unchanged original upload/allocation caller cases; {len(executed)} executed instruction addresses; no supplied results')


if __name__ == '__main__':
    main()

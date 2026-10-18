#!/usr/bin/env python3
"""Execute the unchanged original upload suffix on declared native caller input.

The input contains real native resources and the actual saved388383 caller
frame. It is not captured original RAM or an Xbox kernel-result fixture. The
original decoder/relocation/request store executes unchanged; execution stops
before the first GP poll. No callee result, request clear or readiness is given.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from unicorn import (Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE,
                     UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE, UC_MEM_WRITE)
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--native-before', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if args.output.exists():
        raise ValueError('Preserve prior suffix evidence; use a fresh output')
    image = Xbe(NEW_DEFAULT)
    assert hashlib.sha256(image.data).hexdigest() == NEW_PIN
    before = json.loads((args.native_before / 'before.json').read_text())
    assert before['pc'] == 0x388383 and not before['source_request_clear_supplied']
    machine = Uc(UC_ARCH_X86, UC_MODE_32)
    pages = set()
    regions = []
    for row in before['regions']:
        address, size = row['address'], row['bytes']
        raw = (args.native_before / row['file']).read_bytes()
        assert len(raw) == size
        pages.update(range(address & ~4095, (address + size + 4095) & ~4095, 4096))
        regions.append((address, raw))
    for page in sorted(pages):
        machine.mem_map(page, 4096)
    for address, raw in regions:
        machine.mem_write(address, raw)
    gp_names = ('EAX', 'ECX', 'EDX', 'EBX', 'ESP', 'EBP', 'ESI', 'EDI')
    for name, value in zip(gp_names, before['abi']['gp']):
        machine.reg_write(getattr(R, 'UC_X86_REG_' + name), value)
    for i, words in enumerate(before['abi']['xmm']):
        machine.reg_write(getattr(R, f'UC_X86_REG_XMM{i}'),
                          int.from_bytes(struct.pack('<4I', *words), 'little'))
    for register, value in ((R.UC_X86_REG_EFLAGS, before['abi']['eflags']),
                            (R.UC_X86_REG_FPCW, before['abi']['x87_control']),
                            (R.UC_X86_REG_FPSW, before['abi']['x87_status']),
                            (R.UC_X86_REG_FPTAG, 0xffff),
                            (R.UC_X86_REG_MXCSR, before['abi']['mxcsr'])):
        machine.reg_write(register, value)
    assert before['abi']['x87_depth'] == 0
    initial, sites = {}, set()
    digest, stores = hashlib.sha256(), 0
    boundary = 0x388470

    def instruction(m, pc, size, _):
        if pc == boundary:
            m.emu_stop()
            return
        assert bytes(m.mem_read(pc, size)) == image.read(pc, size)
        sites.add(pc)

    def access(m, kind, address, size, value, _):
        nonlocal stores
        assert address < 0xfe800000, 'No hardware read/result is admitted'
        previous = bytes(m.mem_read(address, size))
        for i, byte in enumerate(previous):
            initial.setdefault(address + i, byte)
        if kind == UC_MEM_WRITE:
            written = (value & ((1 << (8 * size)) - 1)).to_bytes(size, 'little')
            digest.update(struct.pack('<3I', m.reg_read(R.UC_X86_REG_EIP), address, size))
            digest.update(previous)
            digest.update(written)
            stores += 1

    machine.hook_add(UC_HOOK_CODE, instruction)
    machine.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, access)
    machine.emu_start(0x388383, boundary, count=5000000)
    assert machine.reg_read(R.UC_X86_REG_EIP) == boundary
    banks, start, last, data = {}, None, None, bytearray()
    for address, byte in sorted(initial.items()):
        if last is not None and address != last + 1:
            banks[f'{start:08x}'] = data.hex()
            start, data = None, bytearray()
        if start is None:
            start = address
        data.append(byte)
        last = address
    if start is not None:
        banks[f'{start:08x}'] = data.hex()
    after = {at: bytes(machine.mem_read(int(at, 16), len(raw) // 2)).hex()
             for at, raw in banks.items()}
    row = dict(scope=__doc__, xbe_sha256=NEW_PIN,
               native_before_manifest_sha256=hashlib.sha256((args.native_before / 'before.json').read_bytes()).hexdigest(),
               gp_after=[machine.reg_read(getattr(R, 'UC_X86_REG_' + name)) for name in gp_names],
               xmm_after=[list(struct.unpack('<4I', machine.reg_read(getattr(R, f'UC_X86_REG_XMM{i}')).to_bytes(16, 'little'))) for i in range(8)],
               eflags_after=machine.reg_read(R.UC_X86_REG_EFLAGS),
               before=banks, after=after, store_count=stores, store_sha256=digest.hexdigest(),
               entry=0x388383, boundary=boundary, executed_instructions=sorted(sites),
               instruction_patches=[], supplied_external_results=[], supplied_request_clear=False,
               full_engine_verified=False)
    args.output.mkdir()
    (args.output / 'case.json').write_text(json.dumps(row, indent=2) + '\n')
    print(f'PASS unchanged original upload suffix: {len(sites)} instruction addresses, {stores} ordered stores; first-poll boundary, no acknowledgement')


if __name__ == '__main__':
    main()

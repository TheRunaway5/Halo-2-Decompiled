#!/usr/bin/env python3
"""Unchanged TU1.5 scratch-table callers with declared resource inputs.

No Xbox kernel return, device read, processor acknowledgement or readiness is
supplied. MMIO is a declared write-only store sink. Native resource composition
and hardware execution remain separate gates. Preserve all previous evidence.
"""
import argparse
import hashlib
import json
import shutil
import struct
from pathlib import Path
from unicorn import (Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE,
                     UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE, UC_MEM_READ, UC_MEM_WRITE)
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--upload-fixtures', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if args.output.exists():
        raise ValueError('Preserve prior scratch evidence; use a fresh output')
    prior = json.loads((args.upload_fixtures / 'report.json').read_text())
    assert prior['suite'] == 'audio_dsp' and prior['xbe_sha256'] == NEW_PIN
    assert prior['instruction_patches'] == prior['supplied_external_results'] == []
    image = Xbe(NEW_DEFAULT)
    assert hashlib.sha256(image.data).hexdigest() == NEW_PIN
    resource = image.section_bytes(next(s for s in image.sections if s.name == 'DSPImage'))
    assert hashlib.sha256(resource).hexdigest() == prior['dsp_section_sha256']
    args.output.mkdir(parents=True)
    for case in prior['cases']:
        shutil.copyfile(args.upload_fixtures / case['file'], args.output / case['file'])
    machine = Uc(UC_ARCH_X86, UC_MODE_32)
    machine.mem_map(0x10000, 0x600000)
    for section in image.sections:
        machine.mem_write(section.va, image.section_bytes(section))
    base, stack, stop = 0x1000000, 0x10f0000, 0x10fe000
    context, table, data, source, descriptors, out = (
        base + 0x1000, base + 0x2000, base + 0x4000,
        base + 0x9000, base + 0xc000, base + 0xd000)
    machine.mem_map(base, 0x100000)
    machine.mem_map(0xfe802000, 0x1000)
    # The code hook stops before an imported target executes. Unicorn still
    # requires fetchable address space before invoking that hook.
    machine.mem_map(0x80000000, 0x1000)
    gp_names = ('EAX', 'ECX', 'EDX', 'EBX', 'ESP', 'EBP', 'ESI', 'EDI')
    gp = lambda: [machine.reg_read(getattr(R, 'UC_X86_REG_' + name)) for name in gp_names]
    xmm = lambda: [list(struct.unpack('<4I', machine.reg_read(
        getattr(R, f'UC_X86_REG_XMM{i}')).to_bytes(16, 'little'))) for i in range(8)]
    initial, executed, cases, added = {}, set(), list(prior['cases']), []
    digest, store_count, boundary = None, 0, None
    mmio_writes = []

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
        if address >= 0xfe800000:
            assert kind != UC_MEM_READ, 'No device read/result is admitted by this oracle'
            mmio_writes.append([m.reg_read(R.UC_X86_REG_EIP), address, size, value])
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
        result, start, last, raw = {}, None, None, bytearray()
        for address, byte in sorted(initial.items()):
            if last is not None and address != last + 1:
                result[f'{start:08x}'] = raw.hex()
                start, raw = None, bytearray()
            if start is None:
                start = address
            raw.append(byte)
            last = address
        if start is not None:
            result[f'{start:08x}'] = raw.hex()
        return result

    def run(name, entry, arguments, setup, external=None, declared=None):
        nonlocal digest, store_count, boundary
        machine.mem_write(base, bytes(0x10000))
        machine.mem_write(context, bytes([0xa5]) * 0x24)
        machine.mem_write(table, bytes([0xa5]) * 0x800)
        machine.mem_write(data, bytes([0xa5]) * 0x4000)
        machine.mem_write(source, bytes(i & 255 for i in range(0x2000)))
        machine.mem_write(0x391420, bytes(0x40))
        machine.mem_write(0xfe802000, bytes([0xa5]) * 0x1000)
        setup()
        machine.mem_write(stack, struct.pack('<' + 'I' * (1 + len(arguments)), stop, *arguments))
        for index, register in enumerate(gp_names):
            machine.reg_write(getattr(R, 'UC_X86_REG_' + register), 0x12340000 + index)
        machine.reg_write(R.UC_X86_REG_ECX, context)
        machine.reg_write(R.UC_X86_REG_ESP, stack)
        for register, value in ((R.UC_X86_REG_EFLAGS, 2), (R.UC_X86_REG_FPCW, 0x27f),
                                (R.UC_X86_REG_FPSW, 0), (R.UC_X86_REG_FPTAG, 0xffff),
                                (R.UC_X86_REG_MXCSR, 0x1f80)):
            machine.reg_write(register, value)
        for index in range(8):
            machine.reg_write(getattr(R, f'UC_X86_REG_XMM{index}'), 0)
        before_gp, before_xmm = gp(), xmm()
        initial.clear()
        # Retain unused bytes too: a wrong stride or over-copy must fail.
        for address, extent in ((context, 0x24), (table, 0x800), (data, 0x4000),
                                (source, 0x2000), (descriptors, 0x100), (out, 4),
                                (0x391420, 0x40)):
            for index, byte in enumerate(machine.mem_read(address, extent)):
                initial[address + index] = byte
        digest, store_count, boundary = hashlib.sha256(), 0, external
        mmio_writes.clear()
        machine.emu_start(entry, stop, count=100000)
        assert machine.reg_read(R.UC_X86_REG_EIP) == (external or stop), name
        before = banks()
        after = {address: bytes(machine.mem_read(int(address, 16), len(raw) // 2)).hex()
                 for address, raw in before.items()}
        row = dict(name=name, entry=entry, gp=before_gp, xmm=before_xmm,
                   gp_after=gp(), xmm_after=xmm(), eflags_after=machine.reg_read(R.UC_X86_REG_EFLAGS),
                   defined_flags_mask=0xcc5, before=before, after=after,
                   store_sha256=digest.hexdigest(), store_count=store_count,
                   store_hash_encoding='LE32 pc,address,extent followed by before and written bytes in execution order',
                   external_targets=[] if external is None else [external],
                   external_boundary=[] if external is None else [external],
                   mmio_writes=list(mmio_writes), declared_context=declared or {})
        filename = f'{len(cases):03d}-{name}.json'
        (args.output / filename).write_text(json.dumps(row, separators=(',', ':')) + '\n')
        cases.append(dict(file=filename))
        added.append(dict(file=filename, entry=entry, external=external, stores=store_count))

    for mode in (0, 1, 2):
        run(f'scratch_constructor_mode{mode}', 0x38dd0c, [mode], lambda: None)
    for pointer, extent in ((0, 0), (source, 0), (0, 4096), (source, 4096)):
        def setup_release(pointer=pointer, extent=extent):
            word(context + 0x14, pointer)
            word(context + 0x1c, extent)
        run(f'scratch_release_{pointer:x}_{extent:x}', 0x38dd26, [], setup_release,
            0x800000af if pointer and extent else None)
    for extent in (0, 1, 2, 3, 4, 7, 67, 4097):
        def setup_copy():
            word(context + 0x10, 0x391420)
            word(0x391420, data)
        run(f'scratch_copy_{extent}', 0x38de59, [3, source + 1, extent], setup_copy)
    for count in (0, 1, 3, 11):
        def setup_append():
            word(context, 196)
            word(context + 4, 189)
            word(0x391430, table)
            for i in range(count):
                word(descriptors + 16 * i, 0xaaaaaaaa)
                word(descriptors + 16 * i + 4, 0x6000000 + 0x3000 * i)
                word(descriptors + 16 * i + 8, 0xbbbbbbbb)
        run(f'scratch_append_{count}', 0x38de07, [descriptors, count, out], setup_append)
    for mode in (0, 1, 2):
        for prefix in (0, 1, 7):
            def setup_initialize(mode=mode, prefix=prefix):
                word(context + 8, mode)
                descriptor = 0x391420 if mode == 1 else 0x391440
                word(descriptor, data)
                word(descriptor + 4, 0x6000000)
                word(descriptor + 8, max(prefix, 1) * 4096)
                word(descriptor + 16, table)
                word(descriptor + 20, 0x6100000)
                word(descriptor + 24, (189 + prefix) * 8)
            run(f'scratch_initialize_mode{mode}_prefix{prefix}', 0x38dd54, [189],
                setup_initialize, 0x800000ad if prefix else None,
                dict(mode=mode, prefix_pages=prefix, reserved_pages=189,
                     descriptor_state='Declared actual-resource contract; not a recovered factory result'))
    run('scratch_page_walk_import_boundary', 0x38e6df,
        [descriptors, descriptors + 4, out],
        lambda: (word(descriptors, source + 17), word(descriptors + 4, 8192)), 0x800000ad)
    for contiguous in (0, out):
        run(f'scratch_translation_import_boundary_{contiguous:x}', 0x38e668,
            [source + 17, 8192, out + 4, contiguous], lambda: None, 0x800000ad)
    run('scratch_pool_release_capacity_boundary', 0x3879b3, [source],
        lambda: None, 0x80000017)
    run('gp_object_constructor', 0x388275, [], lambda: None)
    run('gp_object_initialize_pool_boundary', 0x388294, [], lambda: None, 0x387a32)
    child = base + 0x30000
    for count in (0, 4):
        def setup_object_append():
            word(context + 8, child)
            word(child, 524)
            word(child + 4, 516)
            word(0x391430, table)
            for i in range(count):
                word(descriptors + i * 16 + 4, 0x6000000 + i * 4096)
        run(f'gp_object_append_{count}', 0x38850a, [descriptors, count], setup_object_append)
    raw_resource, prefix_data = base + 0x20000, base + 0x40000
    def setup_preparation():
        machine.mem_write(raw_resource, resource)
        machine.mem_write(prefix_data, bytes(0xc000))
        machine.mem_write(child, bytes(0x28))
        word(context + 8, child)
        word(context + 0x20, 0)
        word(child + 0x10, 0x391420)
        word(0x391420, prefix_data)
    run('gp_resource_prepare_pool_boundary', 0x388576,
        [raw_resource, len(resource), out], setup_preparation, 0x387a32)
    gp_object, gp_context = base + 0x50000, base + 0x60000
    for offset in (0, 4096, 0x10000):
        def setup_ep_append():
            # Caller-supplied retained EP/GP software contexts. The actual
            # parent's last-three-page offset is10000; other offsets exercise
            # the original arithmetic without claiming a hardware producer.
            machine.mem_write(child, bytes(0x28))
            machine.mem_write(gp_object, bytes([0x5a]) * 0x3c)
            machine.mem_write(gp_context, bytes(0x28))
            word(context, child)
            word(context + 4, gp_object)
            word(child + 0x10, 0x391440)
            word(0x391440, data)
            word(0x391444, 0x6000000)
            word(0x391448, 0x13000)
            word(gp_object + 8, gp_context)
            word(gp_context, 524)
            word(gp_context + 4, 516)
            word(0x391430, table)
        run(f'ep_scratch_gp_append3_offset{offset:x}', 0x38f5f3, [offset], setup_ep_append,
            declared=dict(ep_scratch_descriptor=0x391440, gp_object=gp_object,
                          gp_context=gp_context, page_count=3, offset=offset,
                          scope='Declared native-resource caller input; no EP device/clock result'))
    # Pure original program selector: pointer/count stores only. Invalid
    # indices retain the original zero/zero result; no EP readiness is read.
    for program in (*range(8), 0xffffffff):
        run(f'ep_program_selector_{program:x}', 0x38f544,
            [program, out, out + 4], lambda: (word(out, 0xa5a5a5a5), word(out + 4, 0xa5a5a5a5)))
    prior.update(scope=__doc__, cases=cases, scratch_cases=added,
                 upload_fixture_report_sha256=hashlib.sha256((args.upload_fixtures / 'report.json').read_bytes()).hexdigest(),
                 executed_scratch_instruction_addresses=sorted(executed),
                 device_reads_supplied=False, kernel_returns_supplied=False,
                 full_engine_verified=False)
    (args.output / 'report.json').write_text(json.dumps(prior, indent=2) + '\n')
    print(f'PASS {len(added)} unchanged scratch caller cases; {len(executed)} instruction addresses; no supplied results')


if __name__ == '__main__':
    main()

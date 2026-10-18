#!/usr/bin/env python3
"""Follow an authenticated weapon replay into its original kernel wait code.

The initial kernel-boundary state is reconstructed from the original call entry
and every ordered replay write, with each overwritten byte checked. Execute only
unchanged, mapped original kernel instructions. Stop before interrupt, device,
clock, control-register or FP ownership. Guest IF-only execution is optional;
it uses the captured flags and supplies no asynchronous interrupt or IRQ state.
This is a diagnostic prefix, not a
kernel implementation, original live kernel-entry capture, or parity pass.
"""
import argparse
import ctypes
import hashlib
import json
import os
from pathlib import Path
import shutil
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_INVALID, UC_HOOK_MEM_WRITE, UC_PROT_ALL, UC_TLB_CPU
from unicorn import x86_const as R
from original_xbox_pose_case import physical_ram, PageTableMemory, cpu, sha


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for key in ('case', 'oracle', 'boundary', 'output'):
        parser.add_argument('--' + key, type=Path, required=True)
    parser.add_argument('--guest-interrupt-flags', action='store_true',
                        help='Execute guest CLI/STI flags only, still rejecting device/IRQ/scheduler ownership')
    parser.add_argument('--cpu-page-tables', action='store_true',
                        help='Use actual captured physical RAM and CR0/CR3/CR4; no substituted control-register reads')
    args = parser.parse_args()
    assert 'HALO2_TEST_SCRATCH' in os.environ
    output = args.output.absolute()
    output.mkdir(exist_ok=False)
    oracle = json.loads((args.oracle / 'report.json').read_text())
    boundary = json.loads((args.boundary / 'report.json').read_text())
    assert not oracle['passed'] and oracle['exit'] == boundary['entry']
    assert boundary['source_case_sha256'] == sha(args.case / 'call.json')
    assert boundary['oracle_report_sha256'] == sha(args.oracle / 'report.json')
    before = cpu(args.case, 'before')
    state = oracle['actual_CPU']
    prefix = json.loads((args.oracle / 'writes.json').read_text())
    assert len(prefix) == oracle['ordered_source_writes']
    trace, writes, pending, mapped, checked = [], [], [], {}, {}
    stop = None
    disassembler = Cs(CS_ARCH_X86, CS_MODE_32)
    disassembler.detail = True
    gp = [getattr(R, 'UC_X86_REG_' + n) for n in ('EAX', 'ECX', 'EDX', 'EBX', 'ESP', 'EBP', 'ESI', 'EDI')]
    with physical_ram(args.case, 'before') as ram, physical_ram(args.case, 'after') as after_ram:
        memory = PageTableMemory(ram, before)
        after = PageTableMemory(after_ram, cpu(args.case, 'after'))
        # Check all original executable pages before replay writes mutate RAM.
        code_pages = {}
        for section in boundary['kernel_executable_sections']:
            for page in section['pages']:
                if not page['available']:
                    continue
                address, size = page['address'], page['bytes']
                raw = memory.read(address, size)
                assert hashlib.sha256(raw).hexdigest() == page['sha256']
                assert raw == after.read(address, size)
                code_pages[address] = raw
        implicit = {}
        for update in oracle.get('implicit_CPU_RAM_updates', []):
            implicit.setdefault(update['before_source_write_index'], []).append(update)
        def apply_implicit(index):
            for update in implicit.get(index, []):
                address = update['address']
                old, new = bytes.fromhex(update['before']), bytes.fromhex(update['written'])
                assert len(old) == len(new) and ram[address:address + len(old)] == old
                ram[address:address + len(old)] = new
        for index, write in enumerate(prefix):
            apply_implicit(index)
            address, size = write['address'], write['bytes']
            physical_trace = oracle.get('write_and_read_trace_address_space', 'virtual') == 'physical'
            old = ram[address:address + size] if physical_trace else memory.read(address, size)
            assert old.hex() == write['before']
            raw = bytes.fromhex(write['written'])
            assert len(raw) == size
            offset = 0
            while offset < size:
                physical = address + offset if physical_trace else memory.translate(address + offset)[0]
                length = min(size - offset, 4096 - ((address + offset) & 4095))
                ram[physical:physical + length] = raw[offset:offset + length]
                offset += length
        apply_implicit(len(prefix))
        expected_reconstruction = oracle.get('complete_intermediate_RAM_reconstruction_sha256')
        if expected_reconstruction:
            assert hashlib.sha256(ram).hexdigest() == expected_reconstruction
        entry_stack = int.from_bytes(memory.read(state['gp'][4], 4), 'little')
        machine = Uc(UC_ARCH_X86, UC_MODE_32)
        pointer = ctypes.addressof(ctypes.c_char.from_buffer(ram))
        initial_control = oracle.get('final_control_registers') or before['control_registers']
        if args.cpu_page_tables:
            machine.ctl_set_tlb_mode(UC_TLB_CPU)
            machine.mem_map_ptr(0, len(ram), UC_PROT_ALL, pointer)
            for name in ('CR4', 'CR3', 'CR2', 'CR0'):
                machine.reg_write(getattr(R, 'UC_X86_REG_' + name), initial_control[name])

        def map_page(address):
            virtual = address & ~4095
            if virtual in mapped:
                return
            physical, writable = memory.translate(virtual)
            if not args.cpu_page_tables:
                machine.mem_map_ptr(virtual, 4096, UC_PROT_ALL, pointer + physical)
            mapped[virtual] = dict(physical=physical, writable=writable)

        def invalid(_machine, access, address, size, value, user):
            if args.cpu_page_tables:
                raise RuntimeError(f'Architectural paging reached unavailable physical/device bytes {address:08x}+{size:x}')
            for page in range(address & ~4095, (address + size + 4095) & ~4095, 4096):
                map_page(page)
            return True

        machine.hook_add(UC_HOOK_MEM_INVALID, invalid)
        gdt, limit = before['gdtr']
        map_page(gdt)
        machine.reg_write(R.UC_X86_REG_GDTR, (0, gdt, limit, 0))
        for name, selector in before['selectors'].items():
            machine.reg_write(getattr(R, 'UC_X86_REG_' + name.upper()), selector)
        assert machine.reg_read(R.UC_X86_REG_FS_BASE) == before['fs_base']
        for register, value in zip(gp, state['gp']):
            machine.reg_write(register, value)
        machine.reg_write(R.UC_X86_REG_EFLAGS, state['eflags'])

        def flush():
            for write in pending:
                address, size = write['address'], write['bytes']
                write['written'] = (ram[address:address + size] if args.cpu_page_tables else memory.read(address, size)).hex()
                writes.append(write)
            pending.clear()

        def instruction(_machine, pc, size, user):
            nonlocal stop
            flush()
            if args.cpu_page_tables:
                assert machine.reg_read(R.UC_X86_REG_CR3) == initial_control['CR3'], 'New address-space owner required'
            # PE section RVAs need not be page-aligned; the boundary inventory
            # hashes section-relative spans, including their partial edges.
            page = next((address for address, data in code_pages.items()
                         if address <= pc and pc + size <= address + len(data)), None)
            if page is None:
                stop = dict(address=pc, reason='outside authenticated mapped kernel executable pages')
                machine.emu_stop()
                return
            raw = memory.read(pc, size)
            original = code_pages[page][pc - page:pc - page + size]
            assert raw == original and len(raw) == size
            operation = next(disassembler.disasm(raw, pc, count=1))
            assert operation.size == size
            checked[page] = hashlib.sha256(code_pages[page]).hexdigest()
            # CPU/device ownership must be implemented explicitly before it can
            # be executed. Do not accept an emulator's default device results.
            register_names = {operation.reg_name(r) for group in operation.regs_access() for r in group}
            unsupported = operation.mnemonic in {
                'hlt', 'in', 'out', 'insb', 'insw', 'insd',
                'outsb', 'outsw', 'outsd', 'rdtsc', 'rdtscp', 'rdmsr', 'wrmsr',
                'cpuid', 'int', 'int3', 'into', 'iret', 'iretd', 'sysenter',
                'sysexit', 'invlpg', 'lgdt', 'lidt', 'lldt', 'ltr', 'wbinvd',
            } or (operation.mnemonic in ('cli', 'sti') and not args.guest_interrupt_flags) or operation.mnemonic.startswith('f') or any(
                name.startswith(('xmm', 'mm', 'st(', 'dr')) or (name.startswith('cr') and not args.cpu_page_tables)
                for name in register_names)
            row = dict(address=pc, bytes=raw.hex(), mnemonic=operation.mnemonic, operands=operation.op_str)
            if unsupported:
                stop = dict(**row, reason='CPU, interrupt, device, clock or FP owner requires separate validation')
                machine.emu_stop()
                return
            trace.append(row)

        def write(_machine, access, address, size, value, user):
            if args.cpu_page_tables:
                assert 0 <= address < address + size <= len(ram)
                old = ram[address:address + size]
            else:
                for page in range(address & ~4095, (address + size + 4095) & ~4095, 4096):
                    _, writable = memory.translate(page)
                    assert writable or not before['control_registers']['CR0'] & 0x10000
                old = memory.read(address, size)
            pending.append(dict(instruction=trace[-1]['address'], address=address, bytes=size,
                                before=old.hex()))

        machine.hook_add(UC_HOOK_CODE, instruction)
        machine.hook_add(UC_HOOK_MEM_WRITE, write)
        try:
            machine.emu_start(oracle['exit'], entry_stack, count=20000)
            flush()
        except Exception as error:
            stop = dict(address=machine.reg_read(R.UC_X86_REG_EIP), reason=repr(error))
        result = dict(eip=machine.reg_read(R.UC_X86_REG_EIP), gp=[machine.reg_read(r) for r in gp],
                      eflags=machine.reg_read(R.UC_X86_REG_EFLAGS))
        final_control = {name: machine.reg_read(getattr(R, 'UC_X86_REG_' + name))
                         for name in ('CR0', 'CR2', 'CR3', 'CR4')} if args.cpu_page_tables else None
    report = dict(diagnostic_completed=True, scope=__doc__, source_case_sha256=sha(args.case / 'call.json'),
        source_oracle_sha256=sha(args.oracle / 'report.json'), source_boundary_sha256=sha(args.boundary / 'report.json'),
        reconstructed_prefix_writes=len(prefix), kernel_instructions=len(trace), kernel_ordered_stores=len(writes),
        kernel_entry=oracle['exit'], kernel_return_address=entry_stack, stop=stop, final_integer_CPU=result,
        authenticated_executed_kernel_pages=checked, mapped_pages=mapped,
        kernel_return_reached=result['eip'] == entry_stack, actual_live_kernel_entry_observed=False,
        captured_initial_eflags=state['eflags'], guest_CLI_STI_flags_only=args.guest_interrupt_flags,
        architectural_CPU_paging=args.cpu_page_tables, initial_control_registers=initial_control,
        final_control_registers=final_control, write_and_read_trace_address_space='physical' if args.cpu_page_tables else 'virtual',
        asynchronous_interrupts_delivered=False, IRQ_PIC_or_scheduler_results_supplied=False,
        supplied_gameplay_or_kernel_returns=[], original_instruction_patches=[],
        native_kernel_implementation_verified=False, physical_xbox_verified=False, full_weapon_parity=False,
        runner_scratch=os.environ['HALO2_TEST_SCRATCH'], tool_sha256=sha(Path(__file__)))
    for name, data in (('report.json', report), ('instructions.json', trace), ('writes.json', writes)):
        (output / name).write_text(json.dumps(data, indent=2) + '\n')
    shutil.copyfile(Path(__file__), output / Path(__file__).name)
    print(json.dumps({k: report[k] for k in ('diagnostic_completed', 'kernel_instructions', 'kernel_ordered_stores', 'stop', 'kernel_return_reached', 'runner_scratch')}), flush=True)


if __name__ == '__main__':
    main()

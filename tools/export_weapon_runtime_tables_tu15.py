#!/usr/bin/env python3
"""Export whole1030F0's bounded immutable dispatch tables and original selectors.

Offline TU1.5 program-data authority only. No Xbox cache/runtime is started and
no gameplay branch result is supplied. Run through tools/run_test.py.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct

from capstone.x86 import X86_OP_IMM, X86_OP_MEM
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn import x86_const as registers
from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN

OWNER = 0x1030f0


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--manifest', type=Path, required=True)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert args.output.resolve().is_relative_to(scratch), 'Generated data belongs to owned scratch'
    image = Xbe(NEW_DEFAULT)
    assert hashlib.sha256(image.data).hexdigest() == NEW_PIN
    code = _Code(image, '.text')
    body = _trace(code, OWNER, {OWNER})
    instructions = [code.at(pc) for pc in sorted(body.code)]
    positions = {instruction.address: index for index, instruction in enumerate(instructions)}
    tables = []
    for address, width, count in sorted(body.tables):
        assert width == 4 and 0 < count <= 256
        section = next(s for s in image.sections if s.va <= address and
                       address + width * count <= s.va + min(s.vsize, s.rsize))
        assert not section.flags & 1, 'Original program table section is writable'
        jumps = [i for i in instructions if i.mnemonic == 'jmp' and
                 i.operands[0].type == X86_OP_MEM and i.operands[0].mem.disp == address]
        assert len(jumps) == 1
        jump = jumps[0]
        compare, bound = instructions[positions[jump.address] - 2:positions[jump.address]]
        assert compare.mnemonic == 'cmp' and compare.operands[1].type == X86_OP_IMM
        assert compare.operands[1].imm + 1 == count and bound.mnemonic == 'ja'
        assert bound.operands[0].type == X86_OP_IMM
        default = bound.operands[0].imm
        raw = image.read(address, width * count)
        entries = list(struct.unpack('<' + 'I' * count, raw))
        assert default in body.code and all(entry in body.code for entry in entries)
        index_register = jump.reg_name(jump.operands[0].mem.index)
        assert index_register == compare.reg_name(compare.operands[0].reg)
        cpu = Uc(UC_ARCH_X86, UC_MODE_32)
        start = section.va & ~4095
        size = (section.va - start + section.vsize + 4095) & ~4095
        cpu.mem_map(start, size)
        cpu.mem_write(section.va, image.section_bytes(section))
        index_id = getattr(registers, 'UC_X86_REG_' + index_register.upper())
        selectors = []
        path = []
        terminals = set(entries) | {default}
        def observe(machine, pc, size, _):
            if pc in terminals:
                machine.emu_stop()
            else:
                path.append(pc)
        cpu.hook_add(UC_HOOK_CODE, observe)
        for index in [-2147483648, -1, *range(count + 2), 2147483647]:
            path.clear()
            cpu.reg_write(index_id, index & 0xffffffff)
            cpu.reg_write(registers.UC_X86_REG_EFLAGS, 2)
            cpu.emu_start(compare.address, 0, count=8)
            target = cpu.reg_read(registers.UC_X86_REG_EIP)
            expected = entries[index] if 0 <= index < count else default
            assert target == expected
            assert path == [compare.address, bound.address] + ([] if index < 0 or index >= count else [jump.address])
            selectors.append(dict(index=index, target=target, instructions=list(path)))
        tables.append(dict(address=address, width=width, count=count, original_hex=raw.hex(),
            entries=entries, section=dict(name=section.name, flags=section.flags, read_only=True),
            guard=compare.address, guard_hex=bytes(compare.bytes).hex(),
            bound=bound.address, bound_hex=bytes(bound.bytes).hex(),
            dispatch=jump.address, dispatch_hex=bytes(jump.bytes).hex(),
            index_register=index_register, default_target=default, selectors=selectors))
    assert len(tables) == 5 and sum(row['count'] for row in tables) == 26
    # The actual MCC charge projectile reached2F8B27 -> [collector vtable+4].
    # Independent original installers bound these three adjacent interfaces;
    # the final one ends immediately before the next authored script string.
    # Retain complete tables, never a substituted target or method result.
    method_tables = []
    for address, count, installs, next_address in (
        (0x45f8b0, 4, (0x1848d3,0x1865d4,0x1866cc,0x1868ff,0x1869e5),0x45f8c0),
        (0x45f8c0, 2, (0x185111,),0x45f8c8),
        (0x45f8c8, 4, (0x1848a8,0x184902,0x186711,0x186a14,0x2f0de0),0x45f8d8)):
        assert address + count * 4 == next_address
        section = next(s for s in image.sections if s.va <= address and
                       next_address <= s.va + min(s.vsize,s.rsize))
        assert not section.flags & 1
        raw = image.read(address,count*4)
        entries = list(struct.unpack('<'+'I'*count,raw))
        installers=[]
        for pc in installs:
            instruction=code.at(pc)
            assert instruction.mnemonic=='mov' and instruction.operands[1].type==X86_OP_IMM
            assert instruction.operands[1].imm==address
            installers.append(dict(pc=pc,original_hex=bytes(instruction.bytes).hex()))
        methods=[]
        for entry in entries:
            method=_trace(code,entry,{entry})
            code_bytes=b''.join(bytes(code.at(pc).bytes) for pc in sorted(method.code))
            methods.append(dict(entry=entry,whole_original_sites=len(method.code),
                original_instruction_bytes_sha256=hashlib.sha256(code_bytes).hexdigest(),
                direct_calls=sorted(method.calls)))
        method_tables.append(dict(address=address,width=4,count=count,original_hex=raw.hex(),
            entries=entries,section=dict(name=section.name,flags=section.flags,read_only=True),
            original_installers=installers,methods=methods,next_boundary=next_address))
    assert image.read(0x45f8d8,len(b'scripted_dialog_player\0'))==b'scripted_dialog_player\0'
    lines = ['// Authenticated whole TU1.5 1030F0 program tables; unchanged source guards choose every index.',
             '// Offline authority SHA256 ' + NEW_PIN + '.']
    for table in tables:
        lines.append(f'// {table["count"]} entries, original unsigned guard {table["guard"]:08X}, dispatch {table["dispatch"]:08X}.')
        for index, target in enumerate(table['entries']):
            lines.append(f'share_immutable_word(memory,0x{table["address"] + index * 4:x}u,0x{target:x}u,'
                         '"native TU1.5 whole1030F0 bounded weapon dispatch target");')
    lines.append('// Complete authored contact collector interfaces reached by the real charge projectile.')
    for table in method_tables:
        for index,target in enumerate(table['entries']):
            lines.append(f'share_immutable_word(memory,0x{table["address"]+index*4:x}u,0x{target:x}u,'
                '"native TU1.5 authored charge projectile contact interface identity");')
    args.output.write_text('\n'.join(lines) + '\n')
    report = dict(schema='H2_TU15_WEAPON_RUNTIME_TABLES_V1', original_xbe_sha256=NEW_PIN,
        owner=OWNER, original_instruction_sites=len(instructions), tables=tables,
        total_words=26, total_bytes=104, selector_cases=sum(len(row['selectors']) for row in tables),
        method_tables=method_tables,method_words=10,method_bytes=40,
        source_instruction_changes=False, supplied_branch_indices_or_results=False,
        selector_boundary='Declared raw selector-register inputs; original cmp/ja/indirect-jmp execute unchanged until their first real destination instruction.',
        runtime_original_executable_dependency=False, tool_sha256=sha(__file__),
        data_inc_sha256=sha(args.output), scratch=str(scratch), retained_large_artifacts=[])
    args.manifest.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(tables=[dict(address=f'{row["address"]:08x}', count=row['count'],
        entries=[f'{entry:08x}' for entry in row['entries']], original_hex=row['original_hex']) for row in tables],
        total_bytes=104, original_selector_cases=report['selector_cases'], manifest=str(args.manifest)), indent=2))


if __name__ == '__main__':
    main()

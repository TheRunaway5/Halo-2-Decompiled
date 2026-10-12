#!/usr/bin/env python3
"""Single-step unchanged original frame bytes on the host i386 CPU backend.

Linux ELF/ptrace and an explicitly installed Linux LDT segment are test routing.
The selected original instructions, including an owned stop instruction, remain
byte-identical. A stopped boundary redirects to capture without callee results.
This is host CPU evidence on declared retained inputs, not Xbox hardware proof.
"""
import argparse
import copy
import hashlib
import json
import struct
import subprocess
from pathlib import Path
from capstone import CS_AC_READ, CS_AC_WRITE
from oracle_smg_firing_1_1 import ROOT, Xbe, _Code, PIN
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from original_i386_game_frame_bridge_20261011 import ASM


def sha(p):
    return hashlib.sha256(Path(p).read_bytes()).hexdigest()


def execute(fixture, paired, directory):
    latest = fixture['image'] == '1.5'
    path = NEW_DEFAULT if latest else ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'
    assert sha(path) == (NEW_PIN if latest else PIN)
    image, code = Xbe(path), _Code(Xbe(path), '.text')
    selected = {int(p, 16): q for key in ('address_map', 'instruction_map', 'entries')
                for p, q in paired[key].items()}
    cases = fixture['cases']
    assert cases and len({c['return_address'] for c in cases}) == 1
    caller = cases[0]['return_address']
    directory.mkdir(parents=True, exist_ok=False)
    bridge = ASM.replace('0x100f000', hex(caller))
    assert bridge.count('setup:\n') == 1
    # The source FS base is supplied retained ABI input. Only its host selector
    # and syscall adapter are harness policy; they do not construct a KPCR.
    bridge = bridge.replace('setup:\n', '''setup:
 mov phase+88,%eax
 mov %eax,fs_descriptor+4
 mov $123,%eax
 mov $1,%ebx
 mov $fs_descriptor,%ecx
 mov $16,%edx
 int $0x80
 test %eax,%eax
 js failure
 mov $7,%eax
 mov %ax,%fs
''')
    bridge += '\n.align 4\nfs_descriptor: .long 0,0,0xfffff,0x51\n'
    (directory/'bridge.s').write_text(bridge)
    flat = bytearray(0x7f0000)
    for section in image.sections:
        raw = image.section_bytes(section)
        assert section.va >= 0x10000 and section.va+len(raw) <= 0x800000
        flat[section.va-0x10000:section.va-0x10000+len(raw)] = raw
    (directory/'image.bin').write_bytes(flat)
    (directory/'image.s').write_text('.section .image,"awx"\n.incbin "'+str(directory/'image.bin')+'"\n')
    ranges = []
    for c in cases:
        for bank in c['initial_banks']:
            p, n = bank['address'], len(bytes.fromhex(bank['hex']))
            if 0x10000 <= p and p+n <= 0x800000:
                continue
            assert not (p < 0xa00000 and p+n > 0x800000), 'Input overlaps test bridge'
            ranges.append((p & ~4095, (p+n+4095) & ~4095))
    merged = []
    for left, right in sorted(ranges):
        if merged and left <= merged[-1][1]:
            merged[-1] = (merged[-1][0], max(right, merged[-1][1]))
        else:
            merged.append((left, right))
    script = 'ENTRY(_start)\nSECTIONS { . = 0x10000; .image : { *(.image) } . = 0x900000; .bridge : { *(.bridge) }\n'
    for n, (left, right) in enumerate(merged):
        assert left != 0, 'A zero-page hardware fixture requires a separate host owner'
        script += f'.bank{n} 0x{left:x} (NOLOAD) : {{ . += 0x{right-left:x}; }}\n'
    script += '}\n'
    (directory/'link.ld').write_text(script)
    for name in ('bridge', 'image'):
        subprocess.run(['as','--32',str(directory/(name+'.s')),'-o',str(directory/(name+'.o'))],check=True,capture_output=True)
    subprocess.run(['ld','-melf_i386','-T',str(directory/'link.ld'),
        str(directory/'bridge.o'),str(directory/'image.o'),'-o',str(directory/'original-hardware')],check=True,capture_output=True)
    names = subprocess.check_output(['nm',str(directory/'original-hardware')],text=True)
    symbols = {name:int(p,16) for p, kind, name in (row.split() for row in names.splitlines())}
    data, limits = bytearray(), {}
    for c in cases:
        a = c['entry_abi']
        env = bytearray(108)
        for offset, value in ((0,a['x87_control_word']),(4,a['x87_status']),(8,a['x87_tag'])):
            struct.pack_into('<H',env,offset,value)
        for n, value in enumerate(a['x87_values']):
            env[28+n*10:38+n*10] = bytes.fromhex(value)
        entry = selected[c['entry']] if latest else c['entry']
        data += struct.pack('<10I',c['id'],entry,*a['gp'])+struct.pack('<II',a['eflags'],a['mxcsr'])+env
        data += b''.join(int(x,16).to_bytes(16,'little') for x in a['xmm'])+struct.pack('<I',len(c['initial_banks']))
        phase = bytearray(92)
        struct.pack_into('<I',phase,88,a['fs_base'])
        data += phase
        assert len(c['initial_banks']) < 8192
        for bank in c['initial_banks']:
            raw = bytes.fromhex(bank['hex'])
            data += struct.pack('<II',bank['address'],len(raw))+raw
        limits[str(c['id'])] = dict(boundary=c['exit_entry'],return_address=c['return_address'],fs_base=a['fs_base'],
            retained_ranges=[[b['address'], len(bytes.fromhex(b['hex']))] for b in c['initial_banks']])
    (directory/'input.bin').write_bytes(data)
    gp = dict(eax=0, ecx=1, edx=2, ebx=3, esp=4, ebp=5, esi=6, edi=7)
    instructions = []
    for p, q in paired['instruction_map'].items():
        actual = q if latest else int(p,16)
        i = code.at(actual)
        assert flat[actual-0x10000:actual-0x10000+i.size] == i.bytes
        writes, reads = [], []
        def memory_operand(operand):
            segment = i.reg_name(operand.mem.segment) if operand.mem.segment else ''
            assert segment in ('','fs','es'),i.op_str
            return dict(base=gp[i.reg_name(operand.mem.base)] if operand.mem.base else -1,
                index=gp[i.reg_name(operand.mem.index)] if operand.mem.index else -1,
                scale=operand.mem.scale,displacement=operand.mem.disp & 0xffffffff,
                width=operand.size,fs=segment=='fs')
        for operand in i.operands:
            if operand.type == 3 and operand.access & CS_AC_READ and i.mnemonic != 'lea':
                reads.append(memory_operand(operand))
        if i.mnemonic in ('pop','ret'):
            reads.append(dict(base=4,index=-1,scale=1,displacement=0,width=4,fs=False))
        elif i.mnemonic == 'leave':
            reads.append(dict(base=5,index=-1,scale=1,displacement=0,width=4,fs=False))
        if i.mnemonic in ('push','call'):
            writes.append(dict(base=4,index=-1,scale=1,displacement=0xfffffffc,width=4,fs=False))
        else:
            for n, operand in enumerate(i.operands):
                store = i.mnemonic in ('fst','fstp','fistp','fnstcw','fnstsw','stmxcsr') or (n==0 and i.mnemonic in ('mov','movss','movaps','movq','rep movsd','rep movsb','rep stosd','rep stosb'))
                if operand.type != 3 or not (operand.access & CS_AC_WRITE or store):
                    continue
                writes.append(memory_operand(operand))
        estimate = None
        if i.mnemonic == 'rsqrtss':
            destination, source = i.operands
            assert destination.type == 1 and i.reg_name(destination.reg).startswith('xmm')
            estimate = dict(destination=int(i.reg_name(destination.reg)[3:]))
            if source.type == 1:
                assert i.reg_name(source.reg).startswith('xmm')
                estimate['source_xmm'] = int(i.reg_name(source.reg)[3:])
            else:
                assert source.type == 3 and source.size == 4
                estimate['source_memory'] = memory_operand(source)
        instructions.append(dict(actual=actual,canonical=int(p,16),repeated=i.mnemonic.startswith('rep '),
            writes=writes,reads=reads,estimate=estimate,timestamp=i.mnemonic=='rdtsc',packet=int(p,16)==0x300100))
    (directory/'metadata.json').write_text(json.dumps(dict(instructions=instructions,cases=limits,
        input_address=symbols['input'],capture_address=symbols['capture'],done_address=symbols['done_entry'])))
    tracer = ROOT/'tools/physics_game_frame_hardware_trace_20261011.cpp'
    subprocess.run(['c++','-std=c++20','-O2',str(tracer),'-o',str(directory/'tracer')],check=True,capture_output=True)
    subprocess.run([str(directory/'tracer'),str(directory/'original-hardware'),str(directory/'input.bin'),
        str(directory/'output.bin'),str(directory/'metadata.json'),str(directory/'trace.json')],check=True,timeout=600)
    raw = (directory/'output.bin').read_bytes()
    trace = json.loads((directory/'trace.json').read_text())
    traced = {c['id']:c for c in trace['cases']}
    offset, output = 0, []
    for c in cases:
        record = raw[offset:offset+280]
        assert len(record) == 280
        offset += 280
        e = copy.deepcopy(c['exit'])
        e['gp'] = list(struct.unpack_from('<8I',record))
        e['eflags'],e['mxcsr'] = struct.unpack_from('<II',record,32)
        env = record[40:148]
        for name, at in (('x87_control_word',0),('x87_status',4),('x87_tag',8)):
            e[name] = struct.unpack_from('<H',env,at)[0]
        top = (e['x87_status'] >> 11) & 7
        tags = [(e['x87_tag'] >> (((top+n)&7)*2)) & 3 for n in range(8)]
        depth = next((n for n,t in enumerate(tags) if t==3),8)
        assert all(t==3 for t in tags[depth:]), 'A hardware x87 stack with holes requires its own native owner'
        e['x87_values'] = [env[28+n*10:38+n*10].hex() for n in range(depth)]
        e['xmm'] = [f'{int.from_bytes(record[148+n*16:164+n*16],"little"):032x}' for n in range(8)]
        stop = struct.unpack_from('<I',record,276)[0]
        assert stop == c['exit_entry'] == traced[c['id']]['exit_entry']
        banks = []
        for bank in c['initial_banks']:
            n = len(bytes.fromhex(bank['hex']))
            assert offset+n <= len(raw)
            banks.append(dict(address=bank['address'],hex=raw[offset:offset+n].hex()))
            offset += n
        row = copy.deepcopy(c)
        observed = traced[c['id']]
        row.update(exit=e,final_banks=banks,instructions=observed['instructions'],writes=observed['writes'],
            estimates=observed['estimates'],timestamps=observed['timestamps'],packets=observed['packets'])
        output.append(row)
    assert offset == len(raw)
    return dict(fixture, cases=output, oracle_backend='original_i386_hardware_single_step',
        hardware_tool_sha256=sha(__file__),original_sha256=sha(path),
        bridge_input_tool_sha256=sha(ROOT/'tools/original_i386_game_frame_bridge_20261011.py'),
        trace_observer_sha256=sha(tracer), single_stepped_original_instructions=trace['single_stepped_original_instructions'],
        original_instruction_patches=[],original_hardware_binary_sha256=sha(directory/'original-hardware'),
        native_expected_data_used=False,xbox_hardware_proof=False,unretained_data_read_policy='reject',
        cpu_estimates='observed before and after unchanged RSQRTSS on the host CPU',scope=__doc__)


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--input',type=Path,required=True)
    ap.add_argument('--correspondence',type=Path,required=True)
    ap.add_argument('--output',type=Path,required=True)
    a = ap.parse_args()
    fixture = json.loads(a.input.read_text())
    pair = json.loads(a.correspondence.read_text())
    assert fixture['correspondence_sha256'] == sha(a.correspondence)
    input_pin, pair_pin, tool_pin = sha(a.input), sha(a.correspondence), sha(__file__)
    observer_pin = sha(ROOT/'tools/physics_game_frame_hardware_trace_20261011.cpp')
    result = execute(fixture,pair,a.output)
    assert (sha(a.input), sha(a.correspondence), sha(__file__)) == (input_pin, pair_pin, tool_pin)
    assert sha(ROOT/'tools/physics_game_frame_hardware_trace_20261011.cpp') == observer_pin
    result['input_fixture_sha256'] = sha(a.input)
    target = a.output/'fixture.json'
    target.write_text(json.dumps(result,sort_keys=True,separators=(',',':'))+'\n')
    print(json.dumps(dict(sha256=sha(target),cases=len(result['cases']),instructions=result['single_stepped_original_instructions'])))


if __name__ == '__main__':
    main()

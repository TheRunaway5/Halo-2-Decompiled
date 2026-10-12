#!/usr/bin/env python3
"""Run unchanged listener CPU bodies with retained prefix and explicit raw controls."""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from unicorn import Uc, UcError, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE, UC_HOOK_MEM_INVALID
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_MXCSR
from oracle_mp_title_update_display import ROOT, OLD, NEW, PINS, Xbe, _Code, sha, save, encoded

PAIR = ROOT/'analysis/multiplayer/integration/sound-driver-remaining-native/paired'
PARENT = ROOT/'analysis/multiplayer/integration/sound-driver-startup-native/measured/report.json'
PARENT_PIN = '12751ad5e29ef686a654884baa716d6b9446cf489ccf5194d91d7d3eb5845f24'
CALLER_OPERANDS = ROOT/'analysis/multiplayer/integration/sound-driver-startup-native/paired/providers.json'
CALLER_OPERANDS_PIN = 'ca5fb91813a9691ef4cf1907082cb3a1b6324c05364f2124ebe64ba6795851b3'
INPUT, DESCRIPTORS, PAYLOADS, STACK, STOP = 0x0b100000, 0x0b101000, 0x0b102000, 0x200d000, 0x3000000


def run(tag, path, output):
    assert sha(path) == PINS[str(path)] and sha(PARENT) == PARENT_PIN
    parent = json.loads(PARENT.read_text())
    prefix = parent['cases'][0 if tag == 'old' else 4]
    paired = json.loads((PAIR/'report.json').read_text())
    owner = next(q for q in paired['owners'] if q['old'] == '002228a0')
    lines = [(PAIR/'instructions'/f'002228a0-{side}.asm').read_text().splitlines() for side in ['old', 'u5']]
    pcs = {int(b.split()[0], 16): int(a.split()[0], 16) for a, b in zip(*lines)}
    row = next(i for i, q in enumerate(lines[0]) if q.startswith('00222b17 '))
    default_address = int(lines[0 if tag == 'old' else 1][row].split()[-1], 16)
    xbe = Xbe(path); code = _Code(xbe, '.text')
    default = xbe.read(default_address, 72)
    side = 'old' if tag == 'old' else 'new'
    driver_pointer = int(owner['memory'][0][side+'_va'], 16)
    epsilon_rows = [next(q for q in owner['memory'] if q['old_va'] == address) for address in ['0046508c', '0046504c']]
    epsilons = [int.from_bytes(xbe.read(int(q[side+'_va'], 16), 4), 'little') for q in epsilon_rows]
    constants = dict(orientation_epsilon=f'{epsilons[0]:08x}', environment_epsilon=f'{epsilons[1]:08x}',
                     default72_hex=default.hex(), default_address=f'{default_address:08x}')
    driver = int(prefix['after']['driver_address'], 16)
    # Explicit ABI input matches the caller's immutable basis/zero fields.
    # Full driver startup has not reached this input because its device is absent.
    assert sha(CALLER_OPERANDS) == CALLER_OPERANDS_PIN
    c = json.loads(CALLER_OPERANDS.read_text())['constants']
    authored = bytearray(56)
    for name, offset in [('basis_forward',12),('basis_up',24)]:
        value_address = c[name]['old_value_va' if tag=='old' else 'u5_value_va']
        actual = xbe.read(int(value_address,16),12)
        assert actual.hex() == c[name]['raw12_hex']
        authored[offset:offset+12] = actual
    descriptors = bytearray(48)
    for index in range(2):
        descriptors[index*24:index*24+4] = struct.pack('<I', PAYLOADS+index*72)
    regions = {0x0a000000: bytes.fromhex(prefix['output_arena_hex']), driver_pointer: struct.pack('<I', driver),
               INPUT: bytes(authored), DESCRIPTORS: bytes(descriptors), PAYLOADS: default*2}
    specs = [('prior_prefix_platform_stop', [])]
    active = [(driver, 1, 1)]
    specs.append(('raw_active_orientation_platform_stop', active))
    zero = active+[(INPUT+offset, 4, 0) for offset in range(0, 56, 4)]
    zero += [(INPUT+0x34, 4, DESCRIPTORS)]
    specs.append(('raw_active_zero_input', zero))
    specs.append(('raw_active_cached_defaults', zero+[(driver+0x2950+index*72+word*4, 4,
                   int.from_bytes(default[word*4:word*4+4], 'little')) for index in range(2) for word in range(18)]))
    for count in [1, 2, 3, 51, 0xffffffff, 0x80000000, 0x7fffffff]:
        specs.append((f'raw_count_{count:08x}', zero+[(INPUT+0x30, 4, count)]))
    values = [0, 0x80000000, epsilons[1]-1, epsilons[1], epsilons[1]+1,
              (epsilons[1]-1)|0x80000000, epsilons[1]|0x80000000, (epsilons[1]+1)|0x80000000,
              0x7f800000, 0xff800000, 0x7fc00000, 0xffc00000]
    for value in values:
        specs.append((f'raw_weight_{value:08x}', zero+[(INPUT+0x30,4,2), (DESCRIPTORS+0x14,4,value),
                                                     (DESCRIPTORS+0x18+0x14,4,value)]))
    for channel in range(2):
        specs.append((f'raw_parameters_{channel}', zero+[(INPUT+0x30,4,2)]+
                     [(DESCRIPTORS+channel*24+4+4*i,4,0x11223300+i) for i in range(4)]))
        specs.append((f'raw_payload_alias_{channel}', zero+[(INPUT+0x30,4,2),
                     (DESCRIPTORS+channel*24,4,driver+0x2950+channel*72+4)]))
    for axis in range(6):
        for value in [epsilons[0]-1,epsilons[0],epsilons[0]+1,epsilons[0]|0x80000000,0x7f800000,0x7fc00000]:
            specs.append((f'raw_orientation_{axis}_{value:08x}',zero+[(INPUT+0xc+axis*4,4,value)]))
    specs.append(('prior_position_axis_words',[(INPUT,4,0x3f800000),(INPUT+4,4,0x40000000),(INPUT+8,4,0x40400000)]))
    calls = {int(q[side+'_target'],16): ('position' if q['old_site']=='002228ca' else 'orientation') for q in owner['calls']}
    cases=[]; executed=set()
    for name, changes in specs:
        cpu=Uc(UC_ARCH_X86,UC_MODE_32);cpu.mem_map(0x10000,0x800000)
        for section in xbe.sections:cpu.mem_write(section.va,xbe.section_bytes(section))
        cpu.mem_map(0x0a000000,0x23000);cpu.mem_map(INPUT,0x10000)
        cpu.mem_map(0x2000000,0x10000);cpu.mem_map(STOP,0x1000)
        for address,raw in regions.items():cpu.mem_write(address,raw)
        controls=[]
        for address,extent,value in changes:
            before=int.from_bytes(cpu.mem_read(address,extent),'little')
            controls.append(dict(address=f'{address:08x}',bytes=extent,before=f'{before:0{extent*2}x}',after=f'{value:0{extent*2}x}'))
            cpu.mem_write(address,value.to_bytes(extent,'little'))
        stores=[];invalid=[];requests=[];local=set()
        def hook(g,address,extent,user):
            if address in calls:
                operation=calls[address];number=5 if operation=='position' else 8
                args=struct.unpack('<'+'I'*number,bytes(g.mem_read(g.reg_read(UC_X86_REG_ESP)+4,4*number)))
                requests.append(dict(operation=operation,device=f'{args[0]:08x}',words=[f'{v:08x}'for v in args[1:]]));g.emu_stop();return
            if code.at(address):local.add(address)
        def write(g,kind,address,extent,value,user):
            if 0x2000000<=address<0x2010000:return
            pc=g.reg_read(UC_X86_REG_EIP);canonical=pc if tag=='old'else pcs.get(pc,pc)
            stores.append(dict(instruction=f'{canonical:08x}',address=f'{address:08x}',bytes=extent,
                               before=bytes(g.mem_read(address,extent)).hex(),after=(value&((1<<(extent*8))-1)).to_bytes(extent,'little').hex()))
        def bad(g,kind,address,extent,value,user):
            invalid.append(dict(address=f'{address:08x}',bytes=extent));return False
        cpu.hook_add(UC_HOOK_CODE,hook);cpu.hook_add(UC_HOOK_MEM_WRITE,write);cpu.hook_add(UC_HOOK_MEM_INVALID,bad)
        cpu.mem_write(STACK,struct.pack('<II',STOP,INPUT));cpu.reg_write(UC_X86_REG_ESP,STACK)
        cpu.reg_write(UC_X86_REG_FPCW,0x027f);cpu.reg_write(UC_X86_REG_MXCSR,0x1f80)
        try:cpu.emu_start(int(owner['old'if tag=='old'else'u5'],16),STOP,count=1000000)
        except UcError:assert invalid,name
        outcome='platform_stop'if requests else'invalid_storage'if invalid else'returned'
        assert requests or invalid or cpu.reg_read(UC_X86_REG_EIP)==STOP
        for pc in local:
            instruction=code.at(pc);assert instruction.bytes==xbe.read(pc,len(instruction.bytes))==bytes(cpu.mem_read(pc,len(instruction.bytes)))
        executed.update(local)
        outputs=[dict(address=f'{address:08x}',bytes=len(raw),sha256=hashlib.sha256(bytes(cpu.mem_read(address,len(raw)))).hexdigest())for address,raw in sorted(regions.items())]
        cases.append(dict(name=tag+'_'+name,controls=controls,stores=stores,requests=requests,invalid=invalid,outcome=outcome,outputs=outputs))
    save(output/'instructions'/f'{tag}-executed.asm',''.join(f'{q.address:08x} {q.bytes.hex()} {q.mnemonic} {q.op_str}\n'for q in (code.at(pc)for pc in sorted(executed))).encode())
    return dict(image=tag,driver_pointer=f'{driver_pointer:08x}',input_address=f'{INPUT:08x}',constants=constants,
                regions=[dict(address=f'{a:08x}',raw_hex=b.hex())for a,b in sorted(regions.items())],cases=cases)


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
    images=[run(tag,path,args.output)for tag,path in [('old',OLD),('u5',NEW)]]
    report=dict(schema='original-sound-listener-controls-v1',oracle_sha256=sha(__file__),parent_report_sha256=PARENT_PIN,
                paired_report_sha256=sha(PAIR/'report.json'),images=images,instruction_patches=[],source_platform_returns=[],
                limits=['Actual allocation-prefix driver has no device and stops at its real platform request.',
                        'Active/count/float/alias cases are explicit raw controls, not source driver construction or live listener admission.',
                        'No original platform return or device identity is supplied.'])
    save(args.output/'report.json',encoded(report));print(json.dumps(dict(report_sha256=sha(args.output/'report.json'),
        cases=sum(len(q['cases'])for q in images),stores=sum(len(c['stores'])for q in images for c in q['cases']))))


if __name__=='__main__':main()

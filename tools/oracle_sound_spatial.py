#!/usr/bin/env python3
"""Declared raw-ABI spatial cases run through unchanged original instructions.

These are numerical/branch probes derived from one captured original call,
not additional genuine game events. Every edited input is listed. Retained
readiness, listener activation and ownership roots remain captured game state.
No source instruction or callee return is changed; execution stops before the
original driver method. The SDK and final audible result are outside scope.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn import x86_const as R


def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--capture',type=Path,required=True)
    parser.add_argument('--pair',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();args.output.mkdir(parents=True,exist_ok=False)
    replay=json.loads((args.capture/'replay.json').read_text())
    actual=json.loads((args.capture/'actual-boundary.json').read_text())
    assert actual['pc_equal']and actual['driver_equal']and all(r['equal']for r in actual['writes'])
    image_path=ROOT/'assets/disc/default.xbe';image=Xbe(image_path)
    assert sha(image_path)==replay['image_sha256']=='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
    pair=json.loads((args.pair/'report.json').read_text());pcs={}
    for row in pair['owners']:
        before=(args.pair/'instructions'/f'{row["base"]}-base.asm').read_text().splitlines()
        after=(args.pair/'instructions'/f'{row["base"]}-old.asm').read_text().splitlines()
        assert len(before)==len(after)==row['rows']
        pcs.update({int(a.split()[0],16):int(b.split()[0],16)for a,b in zip(before,after)})
    pages={}
    for row in replay['pages']:
        path=args.capture/f'page-{row["address"]:08x}.bin'
        assert sha(path)==row['sha256'];pages[row['address']]=path.read_bytes()
    def raw(a,n):return bytes(pages[(a+i)&~4095][(a+i)&4095]for i in range(n))
    def word(a):return int.from_bytes(raw(a,4),'little')
    roots=replay['state_roots'];sp=replay['registers']['esp'];position=sp-0x30
    voice=roots['voices']+replay['voice_index']*0x24
    source=word(roots['sources']+0x44)+(word(voice)&65535)*0xbc
    listener=roots['system']+0x88+raw(source+0xa1,1)[0]*0x48
    shared=word(roots['shared_voices']+0x44)+(word(voice+4)&65535)*0x6c
    definition=word(roots['tag_table']+(word(source+12)&65535)*16+8)
    globals=word(roots['current_matg']+0xc4)
    classes=word(roots['tag_table']+(word(globals+4)&65535)*16+8)
    cls=word(classes+4)+struct.unpack('<b',raw(definition+2,1))[0]*0x5c
    attenuation=replay['attenuation_address'];assert attenuation
    cases=[]
    def case(label,edits):cases.append((label,edits))
    def dword(address,value):return(address,4,value&0xffffffff)
    def byte(address,value):return(address,1,value&255)
    case('captured_control',[])
    case('null_attenuation',[dword(sp+4,0)])
    for audible in range(16):case(f'audible_nibble_{audible}',[byte(source+0x1b,(raw(source+0x1b,1)[0]&0xf0)|audible)])
    case('source_flag_disabled',[byte(source+6,raw(source+6,1)[0]&~2)])
    case('no_driver_index',[(voice+0xe,2,65535)])
    case('shared_voice_already_updated',[dword(shared+0x44,word(roots['system']+0x80))])
    case('shared_voice_previously_updated',[dword(shared+0x44,(word(roots['system']+0x80)-1)&0xffffffff)])
    case('no_shared_voice',[dword(voice+4,0xffffffff)])
    for location_type in (-128,-1,0,1,2,127):case(f'location_type_{location_type}',[byte(source+0x1a,location_type)])
    scalar_values=(0,0x80000000,0x3f800000,0xbf800000,0x3f000000,0x3eaaaaab,0x40000000,
                   0xc0400000,0x00800000,0x00000001,0x7f7fffff,0x7f800000,0xff800000,0x7fc12345)
    for value in scalar_values:
        case(f'transform_scale_{value:08x}',[dword(listener+8,value)])
        for component in range(3):case(f'attenuation_{component}_{value:08x}',[dword(attenuation+4*component,value)])
        case(f'class_spatial_scale_{value:08x}',[dword(cls+0x50,value)])
        case(f'elapsed_{value:08x}',[dword(roots['system']+0x1e0,value)])
        case(f'class_environment_db_{value:08x}',[dword(cls+0x10,value)])
    for audible in (0,1,2):
        for local in (False,True):
            edits=[byte(source+0x1b,audible),(source+0x18,2,int.from_bytes(raw(source+0x18,2),'little')|0x1000),dword(sp+4,0)]
            index=int.from_bytes(raw(roots['current_local_index_address'],2),'little')
            edits.append((source+0x46,2,index if local else(index+1)&65535))
            case(f'local_flag_audible_{audible}_matches_{local}',edits)
    lines=['DECLARED_SOUND_SPATIAL_ORIGINAL_V1','K\t'+'\t'.join(f'{int.from_bytes(image.read(a,4),"little"):08x}'for a in(0x45dbc0,0x44a0b4,0x45dc08))]
    names=('voices','sources','system','driver','shared_voices','tag_table','current_matg','current_local_index_address')
    reports=[];instructions=set()
    for label,edits in cases:
        cpu=Uc(UC_ARCH_X86,UC_MODE_32)
        for address,value in pages.items():cpu.mem_map(address,4096);cpu.mem_write(address,value)
        for address,size,value in edits:cpu.mem_write(address,value.to_bytes(size,'little'))
        initial={address:bytes(cpu.mem_read(address,4096))for address in pages}
        regs=replay['registers']
        for name in ('eax','ebx','ecx','edx','esi','edi','ebp','esp','eip','eflags'):cpu.reg_write(getattr(R,'UC_X86_REG_'+name.upper()),regs[name])
        for original,dest in (('fctrl','FPCW'),('fstat','FPSW'),('ftag','FPTAG'),('mxcsr','MXCSR')):cpu.reg_write(getattr(R,'UC_X86_REG_'+dest),regs[original])
        for i in range(8):
            mantissa,exponent=struct.unpack('<QH',bytes.fromhex(replay['floating'][f'st{i}']))
            cpu.reg_write(getattr(R,f'UC_X86_REG_FP{(i+((regs["fstat"]>>11)&7))&7}'),(mantissa,exponent))
            cpu.reg_write(getattr(R,f'UC_X86_REG_XMM{i}'),int.from_bytes(bytes.fromhex(replay['floating'][f'xmm{i}']),'little'))
        stores=[]
        def code(g,address,size,user):
            assert bytes(g.mem_read(address,size))==image.read(address,size)and address in pcs
            instructions.add(address)
            if address==0x220e00:g.emu_stop()
        def store(g,kind,address,size,value,user):
            if sp-0x10000<=address<sp+8:return
            pc=g.reg_read(R.UC_X86_REG_EIP)
            stores.append((pcs[pc],address,size,int.from_bytes(g.mem_read(address,size),'little'),value&((1<<(8*size))-1)))
        cpu.hook_add(UC_HOOK_CODE,code);cpu.hook_add(UC_HOOK_MEM_WRITE,store)
        cpu.emu_start(0x129790,replay['return_pc'],count=200000)
        boundary=cpu.reg_read(R.UC_X86_REG_EIP);assert boundary in (0x220e00,replay['return_pc'])
        driver_index=0xffffffff;driver_hex='-'
        if boundary==0x220e00:
            driver_index=int.from_bytes(cpu.mem_read(cpu.reg_read(R.UC_X86_REG_ESP)+4,4),'little')
            driver_hex=bytes(cpu.mem_read(cpu.reg_read(R.UC_X86_REG_EDI),36)).hex()
        lines.append('M\t'+'\t'.join(f'{roots[n]:08x}'for n in names))
        for address,value in sorted(initial.items()):lines.append(f'R\t{address:08x}\t{value.hex()}')
        initial_position=initial[position&~4095][position&4095:(position&4095)+12]
        assert len(initial_position)==12
        input_attenuation=int.from_bytes(initial[(sp+4)&~4095][(sp+4)&4095:((sp+4)&4095)+4],'little')
        lines.append(f'F\t{regs["fctrl"]:04x}\t{regs["mxcsr"]:08x}')
        lines.append(f'C\t{replay["voice_index"]&65535:04x}\t{input_attenuation:08x}\t{initial_position.hex()}\t{bytes(cpu.mem_read(position,12)).hex()}\t{driver_index:08x}\t{driver_hex}')
        for pc,address,size,before,after in stores:lines.append(f'S\t{pc:08x}\t{address:08x}\t{size}\t{before:08x}\t{after:08x}')
        lines.append('E')
        reports.append(dict(label=label,declared_input_edits=[dict(address=f'{a:08x}',bytes=n,value=f'{v:0{n*2}x}')for a,n,v in edits],
            driver_call=boundary==0x220e00,stores=len(stores)))
    fixture=args.output/'fixtures.tsv';fixture.write_text('\n'.join(lines)+'\n')
    report=dict(boundary=__doc__,tool_sha256=sha(__file__),image_sha256=sha(image_path),capture_sha256=sha(args.capture/'replay.json'),
        pair_sha256=sha(args.pair/'report.json'),fixture_sha256=sha(fixture),cases=reports,unchanged_original_instructions=len(instructions),
        intercepted_callees=[],instruction_patches=[],native_expected_values=False)
    (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(f'Original declared-ABI spatial cases: {len(cases)}, {len(instructions)} unchanged instructions')


if __name__=='__main__':main()

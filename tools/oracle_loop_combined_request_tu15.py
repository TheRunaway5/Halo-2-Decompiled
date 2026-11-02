#!/usr/bin/env python3
"""Observe the original shared-loop key and already-owned parent update.

The key slice220CB5..220CF3 and parent slice220D4E..220F37 run unchanged.
Listener selection, source-point selection, inverse transform and contribution
callees execute their actual original instructions. Upstream admission and
downstream track/playback are outside these explicitly declared components.
Run through tools/run_test.py; retain bounded JSON only.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
from oracle_smg_firing_1_1 import Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn import x86_const as R


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    if args.output.exists():parser.error('Use a fresh reference output path')
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    code=_Code(image,'.text');cpu=Uc(UC_ARCH_X86,UC_MODE_32);cpu.mem_map(0x10000,0x800000)
    for section in image.sections:cpu.mem_write(section.va,image.section_bytes(section))
    base=0x1000000;cpu.mem_map(base,0x40000)
    tags,definition,pool,parent,location,system,players=[base+0x1000*n for n in range(1,8)]
    stack=base+0x3e000
    def put(at,value):cpu.mem_write(at,struct.pack('<I',value&0xffffffff))
    def bits(value):return struct.unpack('<I',struct.pack('<f',value))[0]
    def real(at,value):put(at,bits(value))
    def read(at):return struct.unpack('<I',cpu.mem_read(at,4))[0]
    def word(raw,at,value):struct.pack_into('<I',raw,at,value&0xffffffff)
    put(0x4f0414,tags);put(tags+8,definition)
    put(0x50ebe8,pool);put(pool+0x24,0xd4);put(pool+0x38,1);put(pool+0x44,parent)
    put(0x4f2c50,system);put(0x4f54f0,players)
    cpu.mem_write(0x4f2c3d,b'\0')
    instructions={};context={}
    def observe(_,pc,size,__):
        if pc==context['stop']:context['complete']=True;cpu.emu_stop();return
        instruction=code.at(pc)
        assert instruction and bytes(cpu.mem_read(pc,size))==image.read(pc,size)
        instructions[f'{pc:08x}']=instruction.bytes.hex()
        if pc==0x220dfb:
            context['listener']=cpu.reg_read(R.UC_X86_REG_EAX)&0xffffffff
            context['distance']=read(stack+0x54)
            context['maximum']=read(stack+0x60)
        if pc==0x220e83:context['local_point']=list(struct.unpack('<3I',cpu.mem_read(stack+0x30,12)))
    cpu.hook_add(UC_HOOK_CODE,observe)
    def run(entry,stop,registers):
        context.clear();context['stop']=stop
        for reg,value in ((R.UC_X86_REG_ESP,stack),(R.UC_X86_REG_EFLAGS,2),
            (R.UC_X86_REG_FPCW,0x027f),(R.UC_X86_REG_FPSW,0),(R.UC_X86_REG_FPTAG,0xffff),
            (R.UC_X86_REG_MXCSR,0x1f80)):cpu.reg_write(reg,value)
        for reg,value in registers.items():cpu.reg_write(reg,value)
        cpu.emu_start(entry,0,count=12000);assert context.get('complete')
    keys=[]
    for n in range(256):
        datum=[0x12340000,0x80000000,0xffff0000,0x76540000][n%4]
        flags=[0,0x80,0x180,0x280,0x18,0xff,0x100,0x200][n%8]
        mode=(n//8)%4;requested=(n*0x7654321)&0xffffffff
        put(definition,flags);put(stack+0x60,location);put(stack+0x58,requested)
        cpu.mem_write(location+3,bytes([mode]))
        run(0x220cb5,0x220cf3,{R.UC_X86_REG_EBP:datum,R.UC_X86_REG_EBX:1})
        keys.append(dict(datum=datum,definition_flags=flags,mode=mode,requested=requested,
            key=cpu.reg_read(R.UC_X86_REG_EDI)&0xffffffff))
    rows=[]
    for n in range(256):
        frame=[0,1,255,0][n%4];previous=frame if n%3 else (frame-1)&255
        old=bytearray(68);incoming=bytearray(68)
        for raw,offset in ((old,0),(incoming,1)):
            flags=1|((n+offset)%4)*4
            if raw is incoming and n%5==0:flags|=2
            word(raw,0,flags|(((n+offset)%2)<<16)|(1<<24))
            word(raw,4,bits(1));word(raw,8,bits(-3))
            for k,value in enumerate((float((n+offset)%7-3),float(n%5-2),float(n%3))):word(raw,12+4*k,bits(value))
            word(raw,0x2c,0);word(raw,0x30,bits(4))
            word(raw,0x34,bits([0,.25,2][(n+offset)%3]));word(raw,0x38,bits([1,8,32][(n+offset)%3]))
        minimum=[0,.001,.5,3][n%4];maximum=[1,8,32,64][n%4]
        put(definition,0x80);real(definition+12,minimum);real(definition+16,maximum)
        cpu.mem_write(parent,bytes(0xd4));cpu.mem_write(parent,struct.pack('<H',0x1234))
        put(parent+4,0x12340000);put(parent+8,0x12346000)
        cpu.mem_write(parent+12,bytes(old));cpu.mem_write(location,bytes(incoming))
        prior=[bits(v) for v in ([2,.5,0,1] if n%11==0 else [.1,.2,.3,.4])]
        cpu.mem_write(parent+0xc4,struct.pack('<4I',*prior));cpu.mem_write(parent+0x53,bytes([previous]));cpu.mem_write(system+0x7f,bytes([frame]))
        for i in range(4):
            at=system+0x88+i*0x48;active=i<(n%5)
            cpu.mem_write(at,bytes(0x48));cpu.mem_write(at+6,bytes([active]))
            put(players+0xc+4*i,i if active else 0xffffffff)
            # Exact cardinal orthonormal transforms, including translation.
            axes=[(1,0,0),(0,1,0),(-1,0,0),(0,-1,0)]
            x,y,_=axes[(n+i)%4]
            matrix=(1,x,y,0,-y,x,0,0,0,1,float(i),float(-i),0)
            cpu.mem_write(at+8,struct.pack('<13f',*matrix))
        source_flags=(n*197)&65535
        put(stack+0x18,definition);put(stack+0x50,0x12340000);put(stack+0x54,source_flags)
        put(stack+0x60,location);put(stack+0x58,n*0x19771)
        # Derive the shared-branch byte from the unchanged key slice, rather
        # than supplying a combined/admission result to the parent component.
        run(0x220cb5,0x220cf3,{R.UC_X86_REG_EBP:0x12340000,R.UC_X86_REG_EBX:1})
        run(0x220d4e,0x220f37,{R.UC_X86_REG_EAX:0x12340000,R.UC_X86_REG_EBX:1})
        selected=context['listener']
        rows.append(dict(frame=frame,previous_frame=previous,old_location=old.hex(),request_location=incoming.hex(),
            source_flags=source_flags,prior=prior,minimum=bits(minimum),maximum=bits(maximum),
            selected_maximum=context['maximum'],
            selected_listener=selected,listener_input=None if selected==0xffffffff else dict(
                point=context['local_point'],distance=context['distance']),
            expected_location=bytes(cpu.mem_read(parent+12,68)).hex(),
            expected_source_flags=struct.unpack('<H',cpu.mem_read(parent+2,2))[0],
            expected_frame=cpu.mem_read(parent+0x53,1)[0],expected_amplitudes=list(struct.unpack('<4I',cpu.mem_read(parent+0xc4,16)))))
    result=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        keys=keys,cases=rows,executed_original_instructions=instructions,original_instruction_patches=[],substituted_calls=[],
        upstream_admission_verified=False,downstream_track_or_pcm_verified=False,retained_large_artifacts=[])
    raw=json.dumps(result,separators=(',',':'))+'\n';assert len(raw.encode())<1024*1024
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(raw)
    print(json.dumps(dict(keys=len(keys),requests=len(rows),instructions=len(instructions),bytes=len(raw.encode()))),flush=True)


if __name__=='__main__':main()

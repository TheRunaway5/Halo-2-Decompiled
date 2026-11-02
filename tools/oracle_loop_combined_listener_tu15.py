#!/usr/bin/env python3
"""Execute original listener matrix/selection/point components unchanged.

Camera position/forward/up and BSP PAS data are declared component inputs.
The matrix slice129F10:12A4B2..12A56F, complete12A810,12D6F0 and145200 run
their actual original instructions and callees. Vehicle-seat camera blending,
voice admission, source scheduling and DSP/device output remain outside scope.
Use tools/run_test.py; retain bounded JSON and no executable/memory images.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn import x86_const as R


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    if args.output.exists():parser.error('Use a fresh original reference path')
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    code=_Code(image,'.text');cpu=Uc(UC_ARCH_X86,UC_MODE_32);cpu.mem_map(0x10000,0x800000)
    for section in image.sections:cpu.mem_write(section.va,image.section_bytes(section))
    bank=0x1000000;cpu.mem_map(bank,0x40000)
    system,players,structure,scale,table,source,point,single,definition,parent,pool,tags=[bank+0x1000*n for n in range(1,13)]
    stack,stop=bank+0x3e000,bank+0x3f000
    def put(at,value):cpu.mem_write(at,struct.pack('<I',value&0xffffffff))
    def bits(value):return struct.unpack('<I',struct.pack('<f',value))[0]
    def words(at,count):return list(struct.unpack('<'+'I'*count,cpu.mem_read(at,4*count)))
    put(0x4f2c50,system);put(0x4f54f0,players);put(0x4ecc18,structure)
    put(0x4f0414,tags);put(tags+8,definition);put(0x50ebe8,pool)
    put(pool+0x24,0xd4);put(pool+0x38,1);put(pool+0x44,parent);cpu.mem_write(0x4f2c3d,b'\0')
    put(structure+0x9c,4);put(structure+0xe8,table);put(structure+0x228,scale);put(scale+0x28,table)
    instructions={};context={};allowed=set();routines={}
    roots={0x129f10,0x12a810,0x12d6f0,0x145200,0x220c60}
    def routine(entry):
        if entry not in routines:
            body=_trace(code,entry,roots);routines[entry]=set(body.code);allowed.update(body.code)
        return routines[entry]
    entries={0x12a4b2:0x129f10,0x12a810:0x12a810,0x12d6f0:0x12d6f0,
        0x145200:0x145200,0x220cb5:0x220c60,0x220d4e:0x220c60}
    for entry,owner in entries.items():assert entry in routine(owner)
    def observe(_,pc,size,__):
        if pc==context['stop']:context['complete']=True;cpu.emu_stop();return
        if pc not in allowed:
            caller=code.at(context['previous'])
            assert caller.mnemonic=='call' and caller.operands[0].type==2 and caller.operands[0].imm==pc
            assert pc in routine(pc)
        instruction=code.at(pc);assert instruction and bytes(cpu.mem_read(pc,size))==image.read(pc,size)
        instructions[f'{pc:08x}']=instruction.bytes.hex()
        context['previous']=pc
        if pc==0x220dfb:context['parent_listener']=cpu.reg_read(R.UC_X86_REG_EAX)&0xffffffff
        if pc==0x220e83:context['parent_point']=words(stack+0x30,3)
    cpu.hook_add(UC_HOOK_CODE,observe)
    def run(entry,end,registers,rounding):
        assert entry in routine(entries[entry])
        context.clear();context['stop']=end
        for reg,value in ((R.UC_X86_REG_ESP,stack),(R.UC_X86_REG_EFLAGS,2),
            (R.UC_X86_REG_FPCW,0x027f|(rounding<<10)),(R.UC_X86_REG_FPSW,0),
            (R.UC_X86_REG_FPTAG,0xffff),(R.UC_X86_REG_MXCSR,0x1f80|(rounding<<13))):cpu.reg_write(reg,value)
        for reg,value in registers.items():cpu.reg_write(reg,value)
        cpu.emu_start(entry,0,count=12000);assert context.get('complete'),hex(cpu.reg_read(R.UC_X86_REG_EIP))
    def matrix(at,position,forward,up,rounding):
        for offset,values in ((0x10,forward),(0x1c,up),(0x28,position),(0x54,[0,0,0])):
            cpu.mem_write(stack+offset,struct.pack('<3I',*values))
        # Volatile inputs carried by the original velocity/location prefix.
        run(0x12a4b2,0x12a56f,{R.UC_X86_REG_EBP:at+0x18,R.UC_X86_REG_EDX:0,
            R.UC_X86_REG_XMM0:0,R.UC_X86_REG_XMM3:forward[1],R.UC_X86_REG_XMM4:up[2],
            R.UC_X86_REG_XMM5:forward[2],R.UC_X86_REG_XMM6:up[1]},rounding)
        return words(at,13)
    rows=[];transforms=[]
    cardinal=[(1,0,0),(0,1,0),(-1,0,0),(0,-1,0)]
    for rounding in range(4):
      for n in range(128):
        mask=[0,1,4,10,15][n%5];views=[];matrices=[]
        for i in range(4):
            row=system+0x88+i*0x48;cpu.mem_write(row,bytes(0x48));active=bool(mask&(1<<i))
            cpu.mem_write(row+6,bytes([active]));put(players+0xc+i*4,i if active else 0xffffffff)
            if not active:views.append(None);matrices.append(None);continue
            position=[bits(v) for v in ((0,0,0) if n%7==0 else (i*1.125,-i*.75,((n+i)%3)*.25))]
            forward=list(cardinal[(n+i)%4]);up=[0,0,1]
            if n%3==0:forward=[.91,-.23,.37];up=[.17,.29,1.11]
            forward=list(map(bits,forward));up=list(map(bits,up));cluster=-1 if (n+i)%17==0 else i
            leaf=-1 if cluster<0 else i+10
            put(row,leaf);cpu.mem_write(row+4,struct.pack('<h',cluster))
            matrices.append(matrix(row+8,position,forward,up,rounding))
            views.append(dict(position=position,forward=forward,up=up,cluster=cluster,leaf=leaf))
        encoded=bool((n//6)%2);lower,upper=bits(8),bits(256)
        cells=bytes([0,1,127,128,254,255][(n+i)%6] for i in range(6))
        cpu.mem_write(table,cells);put(structure+0x224,int(encoded));put(scale+4,lower);put(scale+8,upper)
        raw=bytearray(68);flags=[0,1,0x100,0x800,0x101,0x801][n%6]
        struct.pack_into('<I',raw,0,flags|(((n//12)%2)<<16)|(1<<24))
        xyz=[bits(v) for v in ((0,0,0) if n%19==0 else ((n%9-4)*.37,(n%7-3)*.51,(n%5)*.25))]
        struct.pack_into('<3I',raw,12,*xyz)
        struct.pack_into('<I',raw,0x28,(10+n%4) if n%3==0 else 0xffffffff)
        struct.pack_into('<h',raw,0x2c,(n//5)%5-1);struct.pack_into('<I',raw,0x30,bits(1.5))
        cpu.mem_write(source,bytes(raw));maximum=bits([0,.001,.5,2,8,64][(n//6)%6])
        put(stack,stop);put(stack+4,source);put(stack+8,maximum);put(stack+12,single)
        run(0x12a810,stop,{},rounding);selected=cpu.reg_read(R.UC_X86_REG_EAX)&0xffffffff
        distance=words(single,1)[0];local=None
        if selected!=0xffffffff:
            assert views[selected] is not None
            put(stack,stop);run(0x12d6f0,stop,{R.UC_X86_REG_ECX:source,R.UC_X86_REG_EAX:point,R.UC_X86_REG_ESI:selected},rounding)
            put(stack,stop);run(0x145200,stop,{R.UC_X86_REG_ECX:system+0x90+selected*0x48,
                R.UC_X86_REG_EDX:point,R.UC_X86_REG_ESI:point},rounding);local=words(point,3)
        minimum=bits([0,.001,.5,2][n%4]);put(definition,0x80);put(definition+12,minimum);put(definition+16,maximum)
        cpu.mem_write(parent,bytes(0xd4));cpu.mem_write(parent,struct.pack('<H',0x1234))
        put(parent+4,0x12340000);put(parent+8,0x12346000)
        cpu.mem_write(parent+0x53,bytes([44]));cpu.mem_write(system+0x7f,bytes([45]))
        put(stack+0x50,0x12340000);put(stack+0x54,3);put(stack+0x58,n*0x19771);put(stack+0x60,source)
        run(0x220cb5,0x220cf3,{R.UC_X86_REG_EBP:0x12340000,R.UC_X86_REG_EBX:1},rounding)
        run(0x220d4e,0x220f37,{R.UC_X86_REG_EAX:0x12340000,R.UC_X86_REG_EBX:1},rounding)
        assert context['parent_listener']==selected
        if selected!=0xffffffff:assert context['parent_point']==local
        rows.append(dict(rounding=rounding,views=views,matrices=matrices,source=raw.hex(),maximum=maximum,
            encoded=encoded,lower=lower,upper=upper,cells=cells.hex(),listener=selected,distance=distance,local_point=local))
        rows[-1].update(minimum=minimum,amplitudes=words(parent+0xc4,4))
      for n in range(32):
        position=list(map(bits,(n*.125,-n*.25,n*.5)))
        forward=list(map(bits,(.91,-.23,.37)));up=list(map(bits,(.17,.29,1.11)))
        value=matrix(single,position,forward,up,rounding)
        value[0]=bits([0,1,-1,2,.5,1e-40][n%6]);cpu.mem_write(single,struct.pack('<13I',*value))
        p=list(map(bits,(n*.17-2,n*.31+1,n*.63-4)))
        cpu.mem_write(point,struct.pack('<3I',*p));put(stack,stop)
        run(0x145200,stop,{R.UC_X86_REG_ECX:single,R.UC_X86_REG_EDX:point,R.UC_X86_REG_ESI:point},rounding)
        transforms.append(dict(rounding=rounding,matrix=value,point=p,result=words(point,3)))
    result=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        cases=rows,inverse_points=transforms,executed_original_instructions=instructions,
        original_instruction_patches=[],substituted_calls=[],vehicle_seat_blend_verified=False,voice_or_device_verified=False,
        entry_boundaries_verified_against_complete_functions={f'{entry:08x}':f'{owner:08x}' for entry,owner in entries.items()},
        every_executed_site_has_rooted_function_provenance=True,
        retained_large_artifacts=[])
    raw=json.dumps(result,separators=(',',':'))+'\n';assert len(raw.encode())<1024*1024
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(raw)
    print(json.dumps(dict(queries=len(rows),inverse_points=len(transforms),instructions=len(instructions),bytes=len(raw.encode()))),flush=True)


if __name__=='__main__':main()

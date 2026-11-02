#!/usr/bin/env python3
"""Measure unchanged129F10 seat interpolation and its complete quaternion callees.

The observer vectors, real CD440 result, unit aim and authored seat+40 factor
are explicit component inputs. No CD440 result is supplied to a live game.
Location/velocity production, voice admission and device delivery are outside
this arithmetic component. Run through tools/run_test.py.
"""
import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import struct
from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn import x86_const as R


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--inputs',type=Path,help='Bounded actual native unit/seat/CD440 sample report to compare additionally')
    args=parser.parse_args();Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    if args.output.exists():parser.error('Preserve existing original references')
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    code=_Code(image,'.text');cpu=Uc(UC_ARCH_X86,UC_MODE_32);cpu.mem_map(0x10000,0x800000)
    for section in image.sections:cpu.mem_write(section.va,image.section_bytes(section))
    bank=0x1000000;cpu.mem_map(bank,0x10000)
    camera,unit,seat,identity,stack=[bank+0x1000*n for n in range(1,6)]
    def put(at,value):cpu.mem_write(at,struct.pack('<I',value&0xffffffff))
    def words(at,count):return list(struct.unpack('<'+'I'*count,cpu.mem_read(at,count*4)))
    def bits(value):return struct.unpack('<I',struct.pack('<f',value))[0]
    def vector(at,values):cpu.mem_write(at,struct.pack('<3I',*values))
    identity=struct.unpack('<I',image.read(0x474d2c,4))[0]
    assert image.read(identity,16)==struct.pack('<4f',0,0,0,1)
    roots={0x129f10,0x120060,0x120310,0x11f9c0,0x11ffd0};bodies={}
    for entry in roots:bodies[entry]=set(_trace(code,entry,roots).code)
    start,stop=0x12a05c,0x12a308
    assert start in bodies[0x129f10] and stop in bodies[0x129f10]
    allowed=set().union(*bodies.values());instructions={};context={}
    def observe(_,pc,size,__):
        if pc==stop:context['complete']=True;cpu.emu_stop();return
        assert pc in allowed,hex(pc)
        instruction=code.at(pc);assert bytes(cpu.mem_read(pc,size))==image.read(pc,size)
        instructions[f'{pc:08x}']=instruction.bytes.hex()
        if pc in roots:context['callees'].add(pc)
    cpu.hook_add(UC_HOOK_CODE,observe)
    directions=[(1,0,0),(-1,0,0),(0,1,0),(0,-1,0),(0,0,1),(0,0,-1),
        (.6,.8,0),(-.6,-.8,0),(.36,.48,.8),(0,0,0),(1e-20,0,0)]
    for n in range(21):
        v=(math.sin(n*.71),math.cos(n*.39),math.sin(n*.19+.3));length=math.sqrt(sum(x*x for x in v))
        directions.append(tuple(x/length for x in v))
    factors=[0,1,.25,.5,.75,-.5,1.5,1e-20]
    actual_inputs=json.loads(args.inputs.read_text())['samples'] if args.inputs else []
    assert len(actual_inputs)<=128
    rows=[]
    for rounding in range(4):
      for n in range(256+(len(actual_inputs) if rounding==0 else 0)):
        forward=directions[n%32];aim=directions[(n//8+n%8*7)%32]
        if n%16==0:aim=tuple(-x for x in forward)
        if n%16==1:aim=forward
        position=(n/31,-n/47,2.75);point=(position[0]+5,position[1]-3,position[2]+.625)
        up=(0,0,1) if n%3 else (.12,-.2,.91)
        factor=bits(factors[(n//32+n%8)%len(factors)])
        fields={name:list(map(bits,value)) for name,value in
            dict(position=position,forward=forward,up=up,unit_point=point,unit_aim=aim).items()}
        if n>=256:
            original_input=actual_inputs[n-256]
            fields={key:original_input[key] for key in fields};factor=original_input['factor']
        cpu.mem_write(stack,bytes(0x200));vector(camera,fields['position']);vector(camera+0x20,fields['forward'])
        vector(camera+0x2c,fields['up']);vector(unit+0x180,fields['unit_aim'])
        vector(stack+0x84,fields['unit_point']);put(stack+0x7c,unit);put(stack+0x34,0xcafe0000);put(seat+0x40,factor)
        for reg,value in ((R.UC_X86_REG_ESP,stack),(R.UC_X86_REG_EDI,camera),(R.UC_X86_REG_EBX,seat),
            (R.UC_X86_REG_EFLAGS,2),(R.UC_X86_REG_FPCW,0x027f|(rounding<<10)),
            (R.UC_X86_REG_FPSW,0),(R.UC_X86_REG_FPTAG,0xffff),(R.UC_X86_REG_MXCSR,0x1f80|(rounding<<13))):cpu.reg_write(reg,value)
        context.clear();context['callees']=set();cpu.emu_start(start,0,count=4000)
        assert context.get('complete'),hex(cpu.reg_read(R.UC_X86_REG_EIP))
        row=dict(rounding=rounding,**fields,factor=factor,rotation=words(stack+0x6c,4),
            result=words(stack+0x10,9),executed_callees=[f'{p:08x}' for p in sorted(context['callees'])])
        if n>=256:
            assert row['result']==original_input['native_forward']+original_input['native_up']+original_input['native_position'],original_input
            assert row['rotation']==original_input['native_rotation'],original_input
        rows.append(row)
    result=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,entry=f'{start:08x}',stop=f'{stop:08x}',
        rooted_owner='00129f10',every_executed_site_has_rooted_function_provenance=True,
        original_instruction_patches=[],substituted_calls=[],cases=rows,
        original_identity_pointer=identity,original_identity_words=words(identity,4),
        executed_original_instructions=instructions,actual_unit_point_producer_verified=False,
        actual_seat_admission_verified=False,location_velocity_or_device_verified=False)
    if args.inputs:result.update(actual_input_count=len(actual_inputs),actual_input_report_sha256=hashlib.sha256(args.inputs.read_bytes()).hexdigest(),
        actual_native_results_match_original=True)
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(result,separators=(',',':'))+'\n')
    print(f'PASS {len(rows)} original seat blends; {len(instructions)} rooted original instruction sites')


if __name__=='__main__':main()

#!/usr/bin/env python3
"""Verify unchanged original17BA50 preserves an unbound WORLD scale.

Use run_test.py. Explicit small location/stack inputs, no callees or supplied
results. This verifies the transform contract, not a whole effect lifecycle.
"""
import argparse
import json
import os
from pathlib import Path
import struct
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EBX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_ESP, UC_X86_REG_EAX, UC_X86_REG_EIP
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from original_xbox_pose_case import sha


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();assert sha(NEW_DEFAULT)==NEW_PIN
    image=Xbe(NEW_DEFAULT);cases=[]
    for scale in (0.,.5,1.,1.0000001192092896,2.,-1.):
        cpu=Uc(UC_ARCH_X86,UC_MODE_32);cpu.mem_map(0x10000,0x600000)
        for section in image.sections:
            raw=image.section_bytes(section)
            if section.va+len(raw)<=0x610000:cpu.mem_write(section.va,raw)
        cpu.mem_map(0x1000000,0x10000);location,effect,output,stack,stop=0x1001000,0x1002000,0x1003000,0x1008000,0x1100000
        cpu.mem_map(stop,0x1000)
        frame=struct.pack('<13f',scale,1,0,0,0,1,0,0,0,1,-2,11,-12)
        cpu.mem_write(location,b'\0\0\xff\xff'+b'\xff'*4+frame)
        cpu.mem_write(stack,struct.pack('<2I',stop,0))
        for reg,value in ((UC_X86_REG_EBX,location),(UC_X86_REG_ESI,effect),(UC_X86_REG_EDI,output),(UC_X86_REG_ESP,stack)):cpu.reg_write(reg,value)
        sites={}
        def instruction(machine,pc,size,_):
            raw=bytes(machine.mem_read(pc,size));assert raw==image.read(pc,size);sites[f'{pc:08x}']=raw.hex()
        cpu.hook_add(UC_HOOK_CODE,instruction);cpu.emu_start(0x17ba50,stop,count=100)
        assert cpu.reg_read(UC_X86_REG_EIP)==stop and cpu.reg_read(UC_X86_REG_ESP)==stack+8
        returned=cpu.reg_read(UC_X86_REG_EAX);actual=bytes(cpu.mem_read(returned,52))
        assert returned==location+8 and actual==frame
        cases.append(dict(scale=scale,scale_word=struct.unpack('<I',frame[:4])[0],returned_same_location=True,
            all_52_transform_bytes_preserved=True,executed_instruction_bytes=sites))
    report=dict(passed=True,original_xbe_sha256=NEW_PIN,entry='17BA50',branch='nodeFFFF returns location+8',
        cases=cases,original_instruction_patches=[],supplied_callee_results=[],full_effect_lifecycle_verified=False,
        runner_scratch=os.environ['HALO2_TEST_SCRATCH'],tool_sha256=sha(Path(__file__)),large_artifacts_retained=[])
    args.output.write_text(json.dumps(report,indent=2)+'\n');print('Verified six original unbound transforms; scale and all52 bytes are preserved.')


if __name__=='__main__':main()

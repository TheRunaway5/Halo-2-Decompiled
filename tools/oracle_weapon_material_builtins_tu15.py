#!/usr/bin/env python3
"""Execute unchanged TU1.5 object-function builtins used by weapon materials."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import struct
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from original_xbox_pose_case import ROOT,sha
from inspect_multiplayer import Cache
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN,containing,full_tokens


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--fixture',type=Path,required=True)
    args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    output=args.output.absolute();output.mkdir(exist_ok=False)
    (output/'attempt.json').write_text(json.dumps(dict(runner_scratch=str(scratch)))+'\n')
    retail_path=ROOT/'assets/disc/default.xbe'
    assert sha(retail_path)=='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
    assert sha(NEW_DEFAULT)==NEW_PIN
    old,new=Xbe(retail_path),Xbe(NEW_DEFAULT)
    oc,nc=_Code(old,'.text'),_Code(new,'.text')
    def recover(entry):
        body=_trace(oc,entry,{entry})
        instructions=[oc.at(pc) for pc in sorted(body.code)]
        pattern=[]
        for instruction in instructions[:12]:
            raw=bytearray(instruction.bytes);wild=set()
            for operand in instruction.operands:
                if operand.type==2 and ((instruction.mnemonic.startswith('j') or instruction.mnemonic=='call') or containing(old,operand.imm)):
                    wild.update(range(instruction.imm_offset,instruction.imm_offset+instruction.imm_size))
                if operand.type==3 and containing(old,operand.mem.disp):
                    wild.update(range(instruction.disp_offset,instruction.disp_offset+instruction.disp_size))
            pattern.extend(b'.' if i in wild else re.escape(bytes([value])) for i,value in enumerate(raw))
        section=new.section('.text');candidates=[]
        for match in re.finditer(b''.join(pattern),new.section_bytes(section),re.S):
            target=section.va+match.start()
            try:
                other=_trace(nc,target,{target})
                ni=[nc.at(pc) for pc in sorted(other.code)]
                if full_tokens(instructions,old)==full_tokens(ni,new):candidates.append(target)
            except (AssertionError,KeyError,ValueError):
                continue
        assert len(candidates)==1,candidates
        return candidates[0],instructions
    entry=0x108d90
    target,instructions=recover(entry)
    alias_target,alias_instructions=recover(0xbab40)
    machine=Uc(UC_ARCH_X86,UC_MODE_32)
    machine.mem_map(0x10000,0x800000)
    for s in new.sections:machine.mem_write(s.va,new.section_bytes(s))
    machine.mem_map(0x1000000,0x10000);machine.mem_map(0x2000000,0x10000);machine.mem_map(0x3000000,4096)
    def put(address,*words):machine.mem_write(address,struct.pack('<'+'I'*len(words),*words))
    put(0x4ecbd0,0x1000000);put(0x1000044,0x1001000);put(0x1001008,0x1002000)
    machine.mem_write(0x10020aa,b'\x02')
    put(0x1002014,0xffffffff);put(0x10020b4,0xffffffff)
    put(0x4f0414,0x1003000);put(0x1003008,0x1004000)
    trace=[]
    def observe(uc,pc,size,user):
        assert bytes(uc.mem_read(pc,size))==new.read(pc,size)
        trace.append(pc)
    machine.hook_add(UC_HOOK_CODE,observe)
    cases=[]
    for name in [0x030005a6,0x040005a7]:
        trace.clear();put(0x1008000,0xdeadbeef,0xdeadbeef)
        # The complete original handler takes object/name on the stack and
        # writes its scalar/active results through EBX/EDI, as its caller does.
        put(0x2008000,0x3000000,0x10000,name)
        machine.reg_write(R.UC_X86_REG_ESP,0x2008000)
        machine.reg_write(R.UC_X86_REG_EBX,0x1008000);machine.reg_write(R.UC_X86_REG_EDI,0x1008004)
        machine.reg_write(R.UC_X86_REG_FPCW,0x27f);machine.reg_write(R.UC_X86_REG_FPSW,0)
        machine.reg_write(R.UC_X86_REG_FPTAG,0xffff);machine.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
        machine.emu_start(target,0x3000000,count=10000)
        assert machine.reg_read(R.UC_X86_REG_EIP)==0x3000000
        scalar,enabled=struct.unpack('<II',machine.mem_read(0x1008000,8))
        cases.append(dict(name=name,recognized=machine.reg_read(R.UC_X86_REG_EAX)&255,
            value_bits=scalar,enabled=enabled&255,trace=trace.copy(),returned_sp=machine.reg_read(R.UC_X86_REG_ESP)))
    aliases=[]
    cache_path=ROOT/'assets/disc/maps/lockout.map'
    cache=Cache(cache_path)
    try:
        for weapon in cache.tags.values():
            if weapon['group']!='weap' or not any(weapon['name'].endswith(tail) for tail in
                    ['\\plasma_pistol','\\plasma_rifle','\\brute_plasma_rifle','\\needler','\\covenant_carbine']):
                continue
            root=weapon['offset']
            for index,at in enumerate(cache.block(root+0x64,32)):
                if cache.u(at+4) not in (0x04000567,0x1200056a):continue
                length,address=cache.u(at+20),cache.u(at+24)
                curve=bytes(cache.data[address-cache.magic:address-cache.magic+length])
                record=bytearray(cache.data[at:at+32])
                struct.pack_into('<II',record,20,length,0x1006000)
                put(0x1004064,1,0x1005000)
                machine.mem_write(0x1005000,bytes(record));machine.mem_write(0x1006000,curve)
                mags=cache.block(root+0x2c0,0x5c)
                assert len(mags)<=1
                put(0x10042c0,len(mags),0x1007000)
                capacity=cache.h(mags[0]+0xa) if mags else 0
                loaded=capacity//2
                if mags:machine.mem_write(0x1007000,bytes(cache.data[mags[0]:mags[0]+0x5c]))
                machine.mem_write(0x100222c,struct.pack('<h',loaded))
                for heat,age in [(0.,0.),(.5,0.),(.5,.25),(0.,.25),(1.25,.25)]:
                    trace.clear();machine.mem_write(0x1002180,struct.pack('<ff',heat,age))
                    put(0x1008000,0xdeadbeef);put(0x2008000,0x3000000,0x10000,cache.u(at+8),0x1008000)
                    machine.reg_write(R.UC_X86_REG_ESP,0x2008000)
                    machine.reg_write(R.UC_X86_REG_FPCW,0x27f);machine.reg_write(R.UC_X86_REG_FPSW,0)
                    machine.reg_write(R.UC_X86_REG_FPTAG,0xffff);machine.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
                    machine.emu_start(alias_target,0x3000000,count=10000)
                    assert machine.reg_read(R.UC_X86_REG_EIP)==0x3000000
                    aliases.append(dict(weapon=weapon['name'],function=index,name=cache.u(at+8),
                        heat_bits=struct.unpack('<I',struct.pack('<f',heat))[0],age_bits=struct.unpack('<I',struct.pack('<f',age))[0],
                        loaded=loaded,value_bits=struct.unpack('<I',machine.mem_read(0x1008000,4))[0],
                        enabled=machine.reg_read(R.UC_X86_REG_EAX)&255,trace=trace.copy()))
    finally:
        cache.data.close();cache.stream.close()
    fixture=dict(original_xbe_sha256=NEW_PIN,entry=target,alias_entry=alias_target,cases=cases,alias_cases=aliases)
    args.fixture.parent.mkdir(parents=True,exist_ok=True)
    args.fixture.write_text(json.dumps(fixture,indent=2)+'\n')
    report=dict(passed=True,original_xbe_sha256=NEW_PIN,retail_entry=entry,tu15_entry=target,
        normalized_complete_body_equal=True,complete_body_instructions=len(instructions),cases=cases,
        alias_entry=alias_target,complete_alias_body_instructions=len(alias_instructions),alias_cases=aliases,
        source_map_sha256=sha(cache_path),
        original_instruction_patches=[],supplied_callee_results=[],declared_object_and_output_banks=True,
        fixture_sha256=sha(args.fixture),tool_sha256=sha(Path(__file__)),runner_scratch=str(scratch),
        material_or_visual_parity_verified=False,large_artifacts_retained=[])
    (output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(entry=hex(target),body_instructions=len(instructions),builtin_cases=len(cases),alias_entry=hex(alias_target),authored_alias_cases=len(aliases))),flush=True)


if __name__=='__main__':
    main()

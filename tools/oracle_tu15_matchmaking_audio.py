#!/usr/bin/env python3
"""Unchanged TU1.5 matched-pane audio branch versus the native policy.

The first block stops before widget creation/callees. The second block starts
at the actual post-widget pane selector and stops before sound playback. No
instruction/callee is replaced; no original sound driver execution is claimed.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EIP
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True)
    a=p.parse_args();scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    x=Xbe(NEW_DEFAULT);assert hashlib.sha256(x.data).hexdigest()==NEW_PIN
    machine=Uc(UC_ARCH_X86,UC_MODE_32);machine.mem_map(0x10000,0x800000)
    for section in x.sections:machine.mem_write(section.va,x.section_bytes(section))
    owner,stack=0x1100000,0x110F000;machine.mem_map(owner,0x10000)
    from unicorn.x86_const import UC_X86_REG_ESP
    stops=set();visited=set()
    def observe(g,pc,size,unused):
        if pc in stops:g.emu_stop()
        else:visited.add((pc,bytes(g.mem_read(pc,size))))
    machine.hook_add(UC_HOOK_CODE,observe)
    def block(start,ends):
        stops.clear();stops.update(ends);machine.emu_start(start,0x3000000,count=100)
        result=machine.reg_read(UC_X86_REG_EIP);assert result in ends;return result
    rows=[]
    for previous in range(6):
        for pane in range(6):
            machine.mem_write(owner+0x618,struct.pack('<I',previous))
            machine.reg_write(UC_X86_REG_EBX,owner);machine.reg_write(UC_X86_REG_ESI,pane)
            changed=block(0x257E77,{0x257E7F,0x257ECD})==0x257E7F
            sound=None
            if changed:
                machine.reg_write(UC_X86_REG_ESP,stack)
                if block(0x257EB7,{0x257EBF,0x257EC4})==0x257EBF:sound=machine.reg_read(UC_X86_REG_EAX)
            rows.append(dict(previous=previous,pane=pane,sound=sound))
    cpp=scratch/'audio-policy.cpp';binary=scratch/'audio-policy'
    cpp.write_text('#include "halo2/frontend_matchmaking_progress.hpp"\n#include<iostream>\nint main(){unsigned a,b;while(std::cin>>a>>b){auto s=halo2::matchmaking_progress_pane_sound(a,b);std::cout<<(s?int(*s):-1)<<"\\n";}}\n')
    subprocess.run(['c++','-std=c++20','-Iinclude',str(cpp),'-o',str(binary)],check=True)
    actual=subprocess.check_output([str(binary)],input=''.join(f"{r['previous']} {r['pane']}\n"for r in rows).encode()).decode().splitlines()
    assert [int(v)for v in actual]==[r['sound']if r['sound']is not None else-1 for r in rows]
    assert all(x.read(pc,len(raw))==raw for pc,raw in visited)
    report=dict(passed=True,original_sha256=NEW_PIN,cases=rows,unchanged_original_instructions=len(visited),
        code_sha256={hex(entry):hashlib.sha256(x.read(entry,size)).hexdigest()for entry,size in [(0x257E77,8),(0x257EB7,13)]},
        native_policy_sha256=hashlib.sha256(Path('include/halo2/frontend_matchmaking_progress.hpp').read_bytes()).hexdigest(),
        scope=__doc__,no_instruction_patches=True,no_replaced_callees=True,actual_audio_driver_claimed=False)
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n')
    print('PASS36 unchanged TU1.5 matched-entry sound controls; repeated pane3 silent; no fabricated countdown ticks')


if __name__=='__main__':main()

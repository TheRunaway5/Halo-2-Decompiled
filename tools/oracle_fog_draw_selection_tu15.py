#!/usr/bin/env python3
"""Unchanged TU1.5 25D30 fog selection, stopped before external GPU owners.

Classification and two original control bytes are declared component inputs.
No instruction, GPU result, branch return or state-cleanup result is supplied.
"""
import argparse,hashlib,json,struct
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    assert hashlib.sha256(NEW_DEFAULT.read_bytes()).hexdigest()==NEW_PIN
    x=Xbe(NEW_DEFAULT);c=_Code(x,'.text');body=_trace(c,0x25d30,{0x25d30});assert len(body.code)==31
    assert bytes(c.at(0x25d78).bytes)==bytes.fromhex('e893e60000')
    rows=[]
    for classification in (0,1,2,3,4,5,6,255,0xffffffff):
      for suppressed in (0,1,128):
       for force in (0,1,128):
        g=Uc(UC_ARCH_X86,UC_MODE_32);end=max((s.va+s.vsize+4095)&~4095 for s in x.sections)
        g.mem_map(0x10000,end-0x10000)
        for s in x.sections:g.mem_write(s.va,x.section_bytes(s))
        g.mem_map(0x70000000,0x10000);stop=0x7000f000;sp=0x7000e000
        g.mem_write(sp,struct.pack('<I',stop));g.reg_write(UC_X86_REG_ESP,sp)
        g.mem_write(0x491f6c,struct.pack('<I',classification));g.mem_write(0x492295,bytes([suppressed]));g.mem_write(0x492080,bytes([force]))
        trace=[];choice=[]
        def hook(uc,pc,size,user):
            if pc in body.code:trace.append(pc)
            if pc==0x25d78:
                target=uc.reg_read(UC_X86_REG_EAX);assert target in (0x25e30,0x26500)
                choice.append('atmospheric' if target==0x25e30 else 'sky');uc.emu_stop()
            elif pc==0x25d7d:uc.emu_stop()
        g.hook_add(UC_HOOK_CODE,hook);g.emu_start(0x25d30,stop,count=1000)
        assert trace and trace[0]==0x25d30
        assert g.reg_read(UC_X86_REG_EIP) in (stop,0x25d78,0x25d7d)
        rows.append(dict(classification=classification,suppressed=suppressed,force_atmospheric=force,
            selected=choice[0] if choice else None,trace=trace,stopped=g.reg_read(UC_X86_REG_EIP)))
    report=dict(reference='OG Xbox TU1.5',xbe_sha256=NEW_PIN,tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        entry='00025d30',body_instructions=31,boundary=__doc__,cases=rows)
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2)+'\n');print('Original TU1.5 fog-selection cases:',len(rows))
if __name__=='__main__':main()

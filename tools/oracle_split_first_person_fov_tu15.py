#!/usr/bin/env python3
"""Execute unchanged TU1.5 observer conversion and split-camera lens setup.

This is a numeric lens reference, not original framebuffer acceptance.
Run through tools/run_test.py. No game services or math results are hooked.
"""
import argparse
import hashlib
import json
import os
import re
from pathlib import Path
import struct
import sys

from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
sys.path.insert(0,str(Path(__file__).resolve().parent/'reference/halo2-decompiled/tools'))
from xbe import Xbe
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn import x86_const as R
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from capstone.x86 import X86_OP_MEM,X86_OP_IMM

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--xbe',type=Path,default=NEW_DEFAULT)
    parser.add_argument('--fixture',type=Path,required=True)
    parser.add_argument('--report',type=Path,required=True)
    args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Use tools/run_test.py')
    x=Xbe(args.xbe)
    assert hashlib.sha256(x.data).hexdigest()==NEW_PIN
    original=Xbe(Path(__file__).resolve().parents[1]/'assets/disc/default.xbe')
    assert hashlib.sha256(original.data).hexdigest()=='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
    md=Cs(CS_ARCH_X86,CS_MODE_32);md.detail=True
    text=x.section('.text');code=x.section_bytes(text);entries={}
    for address in (0x171d90,0x2f800,0x377670):
        pattern=[]
        for index,i in enumerate(md.disasm(original.read(address,160),address)):
            raw=bytearray(i.bytes);mask=set()
            if any(o.type==X86_OP_MEM and not o.mem.base and not o.mem.index for o in i.operands):
                mask.update(range(i.disp_offset,i.disp_offset+i.disp_size))
            if (i.mnemonic.startswith('j') or i.mnemonic=='call') and i.operands[0].type==X86_OP_IMM:
                mask.update(range(i.imm_offset,i.imm_offset+i.imm_size))
            pattern.extend(b'.' if n in mask else re.escape(bytes([v])) for n,v in enumerate(raw))
            if i.mnemonic=='ret' or index==15:break
        matches=list(re.finditer(b''.join(pattern),code,re.DOTALL))
        if address==0x377670:
            observer=list(md.disasm(x.read(entries[0x171d90],48),entries[0x171d90]))
            baseline=next(i.operands[1].mem.disp for i in observer if i.mnemonic=='divss')
            matches=[m for m in matches if list(md.disasm(code[m.start():m.end()],text.va+m.start()))[1].operands[0].mem.disp==baseline]
        assert len(matches)==1,(hex(address),len(matches))
        entries[address]=text.va+matches[0].start()
    uc=Uc(UC_ARCH_X86,UC_MODE_32);uc.mem_map(0x10000,0x600000)
    for section in x.sections:uc.mem_write(section.va,x.section_bytes(section))
    uc.mem_map(0x1000000,0x10000);uc.mem_map(0x2000000,0x10000);uc.mem_map(0x3000000,0x1000)
    def call(address,registers=(),arguments=()):
        uc.mem_write(0x200ff00,struct.pack('<'+'I'*(len(arguments)+1),0x3000000,*arguments))
        uc.reg_write(R.UC_X86_REG_ESP,0x200ff00)
        uc.reg_write(R.UC_X86_REG_FPCW,0x027f);uc.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
        for register,value in registers:uc.reg_write(register,value)
        uc.emu_start(entries[address],0x3000000,count=10000)
        assert uc.reg_read(R.UC_X86_REG_EIP)==0x3000000
    call(0x377670)
    source,view,grid,span=0x1000000,0x1001000,0x1004000,0x1004010
    rows=[]
    for authored in (1.2217304706573486,1.5707963705062866):
        for columns,lines,sx,sy in ((1,1,1,1),(1,2,1,1),(2,1,1,1),(2,2,2,1),(2,2,1,2),(2,2,1,1)):
            uc.mem_write(source,bytes(0x54));uc.mem_write(source+0x20,struct.pack('<6f',1,0,0,0,0,1))
            uc.mem_write(source+0x38,struct.pack('<4f',authored,1.3333333730697632,0,0))
            call(0x171d90,[(R.UC_X86_REG_ESI,source)])
            uc.mem_write(grid,struct.pack('<2h',columns,lines));uc.mem_write(span,struct.pack('<2h',sx,sy))
            call(0x2f800,[(R.UC_X86_REG_EAX,source),(R.UC_X86_REG_EDX,span),(R.UC_X86_REG_ECX,view)],[grid])
            angle=struct.unpack('<f',uc.mem_read(view+0x28,4))[0]
            assert 0<angle<2
            rows.append((authored,columns,lines,sx,sy,angle))
    args.fixture.parent.mkdir(parents=True,exist_ok=True)
    args.fixture.write_text('# authored-horizontal grid-x grid-y span-x span-y original-vertical\n'+
        ''.join(f'{h:.9g}\t{x}\t{y}\t{sx}\t{sy}\t{a:.9g}\n'for h,x,y,sx,sy,a in rows))
    args.report.parent.mkdir(parents=True,exist_ok=True)
    args.report.write_text(json.dumps(dict(reference_sha256=NEW_PIN,entries={hex(a):hex(b) for a,b in entries.items()},
        declared_services={},cases=len(rows),fixture_sha256=hashlib.sha256(args.fixture.read_bytes()).hexdigest(),
        scope=__doc__),indent=2)+'\n')
    print(json.dumps({'cases':len(rows),'fixture':str(args.fixture)}))

if __name__=='__main__':main()

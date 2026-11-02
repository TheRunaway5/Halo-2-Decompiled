#!/usr/bin/env python3
"""Execute complete literal shape-type getters, including AD710 and AE690.

All register/FP poisons are declared inputs. Original instructions execute
without patches, intercepted dependencies or semantic memory reads/writes.
The typed native boundary consumes only the returned integer. Run through
run_test.py and keep generated reference data under HALO2_TEST_SCRATCH.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct

from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE, UC_HOOK_CODE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from oracle_workshop_titles_tu15 import GP

OWNERS=(0xad710,0xae690,0xa9520,0xab4b0,0xadb50,0xad1d0,0x2e8cf0,0x31dd70,0x31edf0,0xad740,0x31d520,
        0xa3850,0x31ede0,0xad390,0xaf940,0x1cb030)
STACK,STOP=0x2100100,0x3000000


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True)
    a=p.parse_args();scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert a.output.resolve().is_relative_to(scratch)
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    code=_Code(image,'.text');g=Uc(UC_ARCH_X86,UC_MODE_32)
    for at,size in ((0x10000,0x600000),(STACK&~4095,4096),(STOP,4096)):g.mem_map(at,size)
    for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
    allowed={};trace=[]
    def instruction(vm,pc,size,_):
        assert pc in allowed and bytes(vm.mem_read(pc,size))==allowed[pc],hex(pc)
        trace.append(pc)
    def read(vm,_kind,at,size,value,_):assert at==STACK and size==4,'Shape type getter read semantic memory'
    def write(*args):raise AssertionError('Shape type getter wrote memory')
    g.hook_add(UC_HOOK_CODE,instruction);g.hook_add(UC_HOOK_MEM_READ,read);g.hook_add(UC_HOOK_MEM_WRITE,write)
    rows=[];tsv=['# unchanged TU1.5 '+NEW_PIN,'# owner_hex returned_unsigned complete_instruction_hex']
    for entry in OWNERS:
        body=_trace(code,entry,set(OWNERS));instructions=[code.at(pc) for pc in sorted(body.code)]
        assert len(instructions)==2 and instructions[-1].mnemonic=='ret'
        allowed={i.address:bytes(i.bytes) for i in instructions};returns=set();outputs=[]
        for case in range(128):
            initial=[(0x10432567+n*0x1234567+case*0x10203)&0xffffffff for n in range(8)];initial[4]=STACK
            for reg,value in zip(GP,initial):g.reg_write(reg,value)
            for n in range(8):g.reg_write(R.UC_X86_REG_XMM0+n,int.from_bytes(bytes((case+n+j)&255 for j in range(16)),'little'))
            cw=(0x27f,0x67f,0xa7f,0xe7f)[case%4];sw=0x20 if case%2 else 0
            g.reg_write(R.UC_X86_REG_EFLAGS,0x247 if case%2 else 0x202)
            g.reg_write(R.UC_X86_REG_FPCW,cw);g.reg_write(R.UC_X86_REG_FPSW,sw);g.reg_write(R.UC_X86_REG_FPTAG,0xffff)
            g.reg_write(R.UC_X86_REG_MXCSR,0x1f80);g.mem_write(STACK,struct.pack('<I',STOP));trace.clear();g.emu_start(entry,STOP,count=10)
            result=[g.reg_read(reg) for reg in GP];returns.add(result[0])
            assert result[1:4]==initial[1:4] and result[5:]==initial[5:] and result[4]==STACK+4
            assert trace==sorted(body.code) and g.reg_read(R.UC_X86_REG_EIP)==STOP
            assert g.reg_read(R.UC_X86_REG_FPCW)==cw and g.reg_read(R.UC_X86_REG_FPSW)==sw and g.reg_read(R.UC_X86_REG_MXCSR)==0x1f80
            for n in range(8):assert g.reg_read(R.UC_X86_REG_XMM0+n).to_bytes(16,'little')==bytes((case+n+j)&255 for j in range(16))
            outputs.append([result,g.reg_read(R.UC_X86_REG_EFLAGS)])
        assert len(returns)==1;value=returns.pop();raw=b''.join(bytes(i.bytes)for i in instructions)
        tsv.append(f'{entry:x}\t{value}\t{raw.hex()}')
        rows.append(dict(entry=entry,value=value,cases=128,body=[[i.address,bytes(i.bytes).hex()]for i in instructions],
            outputs_sha256=hashlib.sha256(json.dumps(outputs,separators=(',',':')).encode()).hexdigest()))
    fixture=('\n'.join(tsv)+'\n').encode();a.output.with_suffix('.tsv').write_bytes(fixture)
    report=dict(scope=__doc__,xbe_sha256=NEW_PIN,cases=128*len(OWNERS),owners=rows,fixture_sha256=hashlib.sha256(fixture).hexdigest(),
        instruction_patches=[],intercepted_calls=[],typed_boundary='returned integer only; no guest register-file ABI claim',
        source_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
    a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(cases=128*len(OWNERS),owners=len(rows),fixture_bytes=len(fixture))))


if __name__=='__main__':main()

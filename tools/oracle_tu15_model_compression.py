#!/usr/bin/env python3
"""Retain TU1.5 1CDB0's c170..172 bank immediately before device submission.

The complete unchanged arithmetic prefix runs on literal authored model bounds
and flags. Cached-active=1 is an explicit control so identity cases also reach
the pre-device boundary. The original device call/lifetime is outside scope.
No instruction or consumed arithmetic result is replaced.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys
from audit_rendering import Cache
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn import x86_const as R
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
PIN='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--xbe',type=Path,required=True);p.add_argument('--map',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True);a=p.parse_args();assert not a.output.exists()
    x=Xbe(a.xbe);assert hashlib.sha256(x.data).hexdigest()==PIN
    vm=Uc(UC_ARCH_X86,UC_MODE_32);vm.mem_map(0x10000,0x600000)
    for section in x.sections:vm.mem_write(section.va,x.section_bytes(section))
    vm.mem_map(0x1000000,0x100000);executed={}
    def trace(g,pc,n,_):
        raw=bytes(g.mem_read(pc,n));assert raw==x.read(pc,n);executed[pc]=raw
    vm.hook_add(UC_HOOK_CODE,trace);cache=Cache(a.map);rows=[]
    for tag in cache.tags.values():
        if tag['group']!='mode' or not tag['address']:continue
        _,root=cache.tag(tag['datum'],'mode');bounds=cache.block(root+0x14,0x38)
        for section,at in enumerate(cache.block(root+0x24,0x5c)):
            if cache.u16(at)not in(1,2,3) or not cache.u16(at+4):continue
            flags=cache.u16(at+0x1a);pointer=0x1000000 if bounds else 0
            if bounds:vm.mem_write(pointer,bytes(cache.data[bounds[0]:bounds[0]+40]))
            stack=0x10fff00;vm.mem_write(stack,struct.pack('<II',0,flags&255))
            vm.mem_write(0x492311,b'\1');vm.reg_write(R.UC_X86_REG_ESP,stack)
            vm.reg_write(R.UC_X86_REG_EAX,pointer);vm.reg_write(R.UC_X86_REG_FPCW,0x23f);vm.reg_write(R.UC_X86_REG_MXCSR,0x1fa0)
            vm.emu_start(0x1cdb0,0x1cf12,count=10000);assert vm.reg_read(R.UC_X86_REG_EIP)==0x1cf12
            words=list(struct.unpack('<12I',bytes(vm.mem_read(vm.reg_read(R.UC_X86_REG_ESP)+4,48))))
            rows.append(dict(model=tag['datum'],section=section,flags=flags,constants=words))
    cache.close();a.output.parent.mkdir(parents=True,exist_ok=True)
    report=dict(scope=__doc__,original_xbe_sha256=PIN,cache_sha256=hashlib.sha256(a.map.read_bytes()).hexdigest(),
        entry=0x1cdb0,stop_before_device=0x1cf12,intercepted_instructions=0,executed_instructions=len(executed),
        executed_bank_sha256=hashlib.sha256(b''.join(struct.pack('<I',pc)+raw for pc,raw in sorted(executed.items()))).hexdigest(),
        tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),rows=rows)
    a.output.write_text(json.dumps(report,indent=2)+'\n');print('PASS',len(rows),'original compression banks;',len(executed),'unchanged instructions')


if __name__=='__main__':main()

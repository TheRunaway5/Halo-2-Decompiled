#!/usr/bin/env python3
"""Execute TU1.5 rank lookup, item visibility and bitmap page selection.

Use tools/run_test.py. UI globals use the literal disc wigl rank table; only
the globals accessor is supplied. The original rank helper, consumer branch
and bitmap frame setter execute unchanged. No original network state or GPU
framebuffer is supplied by this bounded oracle.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT / "tools/reference/halo2-decompiled/tools"))
from xbe import Xbe
from audit_rendering import Cache
from oracle_party_lobby_tu15 import DEFAULT, PIN
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--report-output",type=Path)
    args=parser.parse_args()
    assert os.environ.get("HALO2_TEST_SCRATCH"), "Use tools/run_test.py"
    xbe=Xbe(DEFAULT)
    assert hashlib.sha256(xbe.data).hexdigest()==PIN
    uc=Uc(UC_ARCH_X86,UC_MODE_32)
    uc.mem_map(0x10000,0x800000)
    for section in xbe.sections:uc.mem_write(section.va,xbe.section_bytes(section))
    uc.mem_map(0x1000000,0x100000)
    uc.mem_map(0x2000000,0x10000)
    stop,stack=0x2000100,0x200f000
    wigl,ranges,item,widget,definition,table,bitmap=[0x1000000+i*0x1000 for i in range(7)]
    def write(at,value):uc.mem_write(at,struct.pack("<I",value&0xffffffff))
    cache=Cache(ROOT/"assets/disc/maps/mainmenu.map")
    try:
        menu_sha=hashlib.sha256(cache.data).hexdigest()
        assert menu_sha=="1b5de78212ee7439b086ef3554885a4188884864ce1040b24ec6604af4a0143b"
        tag=next(t for t in cache.tags.values() if t["group"]=="wigl")
        records=cache.block(tag["address"]-cache.magic+0x158,4)
        data=bytes(cache.data[records[0]:records[0]+4*len(records)])
        uc.mem_write(ranges,data);write(wigl+0x158,len(records));write(wigl+0x15c,ranges)
        tag=next(t for t in cache.tags.values() if t["group"]=="bitm" and t["name"]=="ui\\global_bitmaps\\rank_icons_sm")
        pages=len(cache.block(tag["address"]-cache.magic+68,116))
        assert pages==50
    finally:cache.close()
    write(0x4f0414,table);write(table+24,bitmap);write(bitmap+0x44,pages)
    write(widget+0x70,definition);write(definition+0x1c,1)
    def hook(machine,address,size,user):
        if address in (stop,0x2b9e43):uc.emu_stop()
        elif address==0x14ae8d:
            sp=uc.reg_read(UC_X86_REG_ESP)
            uc.reg_write(UC_X86_REG_EAX,wigl)
            uc.reg_write(UC_X86_REG_EIP,struct.unpack("<I",uc.mem_read(sp,4))[0])
            uc.reg_write(UC_X86_REG_ESP,sp+4)
    uc.hook_add(UC_HOOK_CODE,hook)
    cases=[]
    for value in range(-128,128):
        write(item,0x103);write(stack,stop);write(stack+4,item)
        uc.reg_write(UC_X86_REG_EDI,value&0xffffffff);uc.reg_write(UC_X86_REG_ESP,stack)
        uc.emu_start(0x2b9eb2,stop+1,count=10000)
        flags=struct.unpack("<I",uc.mem_read(item,4))[0]
        rank=struct.unpack("<h",uc.mem_read(item+0x60,2))[0]
        expected=min(value,49) if value>=0 else -1
        assert rank==expected and flags==(0x143 if expected!=-1 else 0x103)
        bp=stack-0x1000;write(bp-0x10,rank)
        uc.mem_write(widget+0x6e,b"\1");uc.mem_write(widget+0x88,b"\0\0")
        uc.reg_write(UC_X86_REG_EBP,bp);uc.reg_write(UC_X86_REG_EDI,widget)
        uc.emu_start(0x2b9d59,0x2b9e44,count=10000)
        visible=bool(uc.mem_read(widget+0x6e,1)[0])
        page=struct.unpack("<H",uc.mem_read(widget+0x88,2))[0]
        assert visible==(expected!=-1) and page==max(expected,0)
        cases.append([value,rank,visible,page])
    entries=[(0x14c9ea,70),(0x2b9eb2,37),(0x2b9d59,25),(0x2ba733,52)]
    report=dict(passed=True,xbe_sha256=PIN,mainmenu_sha256=menu_sha,
        rank_ranges_sha256=hashlib.sha256(data).hexdigest(),pages=pages,cases=cases,
        original_leaves=[dict(entry=f"{a:08x}",bytes=n,sha256=hashlib.sha256(xbe.read(a,n)).hexdigest()) for a,n in entries])
    if args.report_output:
        args.report_output.parent.mkdir(parents=True,exist_ok=True)
        args.report_output.write_text(json.dumps(report,indent=2)+"\n")
    print("PASS TU1.5 rank lookup, visibility and actual bitmap page setter: all256 signed-byte values, all50 pages")


if __name__=="__main__":main()

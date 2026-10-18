#!/usr/bin/env python3
"""Identify a reached original kernel boundary from authenticated live RAM.

No kernel result or implementation is supplied. Discarded/unmapped PE pages
remain explicitly unavailable. This identifies the next owner, not its parity.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from original_xbox_pose_case import physical_ram,PageTableMemory,cpu,sha
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT

def main():
    p=argparse.ArgumentParser(description=__doc__)
    for key in ('case','oracle','output'):p.add_argument('--'+key,type=Path,required=True)
    p.add_argument('--entry',type=lambda x:int(x,0),required=True);args=p.parse_args()
    assert 'HALO2_TEST_SCRATCH'in os.environ
    out=args.output.absolute();out.mkdir(exist_ok=False);before=cpu(args.case,'before');after=cpu(args.case,'after')
    original=json.loads((args.oracle/'report.json').read_text());assert not original['passed']
    trace=json.loads((args.oracle/'instructions.json').read_text());code=_Code(Xbe(NEW_DEFAULT),'.text');pc=args.entry
    with physical_ram(args.case,'before')as a,physical_ram(args.case,'after')as z:
        m=PageTableMemory(a,before);n=PageTableMemory(z,after);kernel=0x80010000;header=m.read(kernel,0x1000)
        assert header[:2]==b'MZ';pe=struct.unpack_from('<I',header,0x3c)[0];assert header[pe:pe+4]==b'PE\0\0'
        optional=pe+24;assert struct.unpack_from('<H',header,optional)[0]==0x10b
        er,es=struct.unpack_from('<2I',header,optional+96);export=m.read(kernel+er,40)
        ordinal,count=struct.unpack_from('<II',export,16);functions=struct.unpack_from('<I',export,28)[0]
        rvas=struct.unpack('<'+'I'*count,m.read(kernel+functions,count*4));matches=[i+ordinal for i,rva in enumerate(rvas)if kernel+rva==pc]
        sections=[];section_count=struct.unpack_from('<H',header,pe+6)[0];table=optional+struct.unpack_from('<H',header,pe+20)[0]
        for i in range(section_count):
            row=header[table+i*40:table+(i+1)*40];name=row[:8].rstrip(b'\0').decode();size,rva=struct.unpack_from('<2I',row,8)
            if not struct.unpack_from('<I',row,36)[0]&0x20000000:continue
            pages=[]
            for address in range(kernel+rva,kernel+rva+size,4096):
                length=min(4096,kernel+rva+size-address)
                try:raw=m.read(address,length);other=n.read(address,length)
                except LookupError:pages.append(dict(address=address,available=False));continue
                pages.append(dict(address=address,bytes=length,available=True,sha256=hashlib.sha256(raw).hexdigest(),entry_exit_identical=raw==other))
            sections.append(dict(name=name,address=kernel+rva,bytes=size,pages=pages))
        raw=m.read(pc,256);assert raw==n.read(pc,len(raw));(out/'kernel-entry.bin').write_bytes(raw)
        dis=Cs(CS_ARCH_X86,CS_MODE_32);ops=[dict(address=i.address,mnemonic=i.mnemonic,operands=i.op_str)for i in dis.disasm(raw,pc)]
        observed={};tsc=[]
        for site in dict.fromkeys(trace):
            if site<kernel:operation=code.at(site)
            else:
                sample=m.read(site,15);operation=next(dis.disasm(sample,site,count=1))
                assert bytes(operation.bytes)==n.read(site,operation.size)
            observed[site]=operation
            if operation.mnemonic=='rdtsc':tsc.append(site)
        rows=[dict(address=site,mnemonic=observed[site].mnemonic,operands=observed[site].op_str)for site in trace[-28:]]
    report=dict(scope=__doc__,entry=pc,kernel_base=kernel,export_ordinals=matches,
        kernel_executable_sections=sections,entry_disassembly=ops,preceding_original_instructions=rows,
        executed_rdtsc_sites=tsc,original_kernel_executed_by_oracle=False,kernel_entry_bytes_equal_before_and_after=True,
        actual_original_full_call_completed=True,independent_original_full_call_verified=False,
        source_case_sha256=sha(args.case/'call.json'),oracle_report_sha256=sha(args.oracle/'report.json'),
        tool_sha256=sha(Path(__file__)),runner_scratch=os.environ['HALO2_TEST_SCRATCH'],full_weapon_parity=False)
    (out/'report.json').write_text(json.dumps(report,indent=2)+'\n');shutil.copyfile(Path(__file__),out/Path(__file__).name)
    print(json.dumps(dict(export_ordinals=matches,preceding=rows[-8:],entry=ops[:10],executed_rdtsc=tsc)),flush=True)

if __name__=='__main__':main()

#!/usr/bin/env python3
"""Publish original indexed reaction tables and their authored animation-name IDs.

CBC50 selects0..3/5..7; CBB90 can write only0/1/2 to its range output.
The consumer reads DWORDs at index*4, requiring eight rows of each table.
The class table then indexes four writable authored IDs. Original bytes alone
supply every value. No array extent is inferred from padding or live outcomes.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
from capstone import CS_AC_WRITE
from capstone.x86 import X86_OP_MEM,X86_OP_IMM
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--original-report',type=Path,required=True);ap.add_argument('--output',type=Path,required=True)
    ap.add_argument('--report',type=Path,required=True);args=ap.parse_args()
    assert args.output.resolve().is_relative_to(Path(os.environ['HALO2_TEST_SCRATCH']).resolve())
    proof=json.loads(args.original_report.read_bytes());assert proof['original_xbe_sha256']==NEW_PIN
    assert proof['natural_selector_results']==[0,1,2,3,5,6,7] and proof['maximum_selector_result']==7
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN;code=_Code(image,'.text')
    lower=_trace(code,0xcbb90,{0xcbb90});writes=[]
    for pc in sorted(lower.code):
        ins=code.at(pc)
        if len(ins.operands)!=2:continue
        destination,value=ins.operands
        if destination.type==X86_OP_MEM and destination.access&CS_AC_WRITE:
            assert ins.reg_name(destination.mem.base)=='edi' and not destination.mem.index and destination.mem.disp==0
            assert value.type==X86_OP_IMM and destination.size==4
            writes.append(dict(pc=pc,value=value.imm,instruction_hex=bytes(ins.bytes).hex()))
    assert {w['value']for w in writes}=={0,1,2}
    root=Path(__file__).resolve().parents[1];reference=root/'tools/reference/halo2-decompiled/src/unit_object_type.cpp'
    source=reference.read_bytes();assert 'long __stdcall function_c9770(' in source.decode()
    rows=[];lines=['// Original reaction table extents from selector bounds; no live state or results.']
    for pc,address in ((0x11266f,0x44aa00),(0x11267a,0x44aa20),(0x112685,0x44a9e0)):
        ins=code.at(pc);operand=ins.operands[1]
        assert operand.type==X86_OP_MEM and operand.size==4 and not operand.mem.base and ins.reg_name(operand.mem.index)=='ecx'
        assert operand.mem.scale==4 and operand.mem.disp==address
        raw=image.read(address,32);assert next(s for s in image.sections if s.va<=address<address+32<=s.va+s.vsize).name=='.rdata'
        rows.append(dict(address=address,bytes=32,rows=8,read_pc=pc,instruction_hex=bytes(ins.bytes).hex(),access='read-only',hex=raw.hex()))
        lines.append(f'literal(memory,0x{address:x}u,std::array<std::uint8_t,32>{{'+','.join(f'0x{v:02x}'for v in raw)+'});')
    classes=[int.from_bytes(image.read(0x44a9e0+i*4,4),'little',signed=True)for i in range(8)]
    assert set(classes)=={-1,0,1,2,3}
    ins=code.at(0x1126a6);operand=ins.operands[1];assert operand.mem.disp==0x47c1f0 and operand.mem.scale==4 and operand.size==4
    raw=image.read(0x47c1f0,16);assert next(s for s in image.sections if s.va<=0x47c1f0<0x47c200<=s.va+s.rsize).name=='.data'
    lines += ['{','constexpr std::array<std::uint8_t,16> names{'+','.join(f'0x{v:02x}'for v in raw)+'};',
        'const auto authored=std::as_bytes(std::span(names));',
        'auto owned=[&](std::uint32_t address) {',
        '    try {(void)memory.view(address,0);return true;}',
        '    catch(const std::out_of_range&){return false;}',
        '};',
        '// Keep existing readonly/writable words and their lifetimes. Validate',
        '// every prior byte before admitting any missing authored storage.',
        'for(std::size_t i=0;i<authored.size();++i)if(owned(0x47c1f0u+i) &&',
        '    memory.view(0x47c1f0u+i,1).front()!=authored[i])',
        '    throw std::invalid_argument("Original reaction name owner differs at"+std::to_string(0x47c1f0u+i));',
        'for(std::size_t i=0;i<authored.size();) {',
        '    if(owned(0x47c1f0u+i)){++i;continue;}',
        '    const auto first=i++;',
        '    while(i<authored.size() && !owned(0x47c1f0u+i))++i;',
        '    memory.mount(0x47c1f0u+first,authored.subspan(first,i-first),SourceRuntimeMemory::Access::ReadWrite,',
        '        "Original TU1.5 authored damage reaction animation-name IDs");','}','}']
    args.output.write_text('\n'.join(lines)+'\n')
    report=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,original_report_sha256=hashlib.sha256(args.original_report.read_bytes()).hexdigest(),
        selector_entry=0xcbc50,range_entry=0xcbb90,range_writes=writes,selector_results=proof['natural_selector_results'],
        readonly_bytes=96,readonly_tables=rows,authored_writable=dict(address=0x47c1f0,bytes=16,rows=4,hex=raw.hex(),read_pc=ins.address,instruction_hex=bytes(ins.bytes).hex()),
        typed_selector_reference=str(reference.relative_to(root)),typed_reference_sha256=hashlib.sha256(source).hexdigest(),
        data_sha256=hashlib.sha256(args.output.read_bytes()).hexdigest(),tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        existing_owners_validated_before_missing_ranges=True,existing_access_and_lifetimes_preserved=True,
        row4_natural_reachability_not_claimed=True,live_results_supplied=False,full_damage_animation_qualified=False)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k!='readonly_tables'}))


if __name__=='__main__':main()

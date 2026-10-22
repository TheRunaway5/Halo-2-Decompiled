#!/usr/bin/env python3
"""Authenticate the complete original HS registry and function definitions.

Use the original948 count word adjacent to the packed definition allocation,
validate every pointer/variable record and all admitted MCC profile signatures.
Preserve the .data pointer registry as authored read-write storage; definitions
and usage literals remain read-only. No evaluator, thread or return is replaced.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from export_final_startup_storage_data_tu15 import regions

ROOT=Path(__file__).resolve().parents[1]
TABLE,DEFINITIONS,COUNT=0x480c00,0x456b20,0x45b3d0


def registry(image):
    count=int.from_bytes(image.read(COUNT,4),'little');assert count==948
    code=_Code(image,'.text');next_record=DEFINITIONS;rows=[];points=set(range(COUNT,COUNT+4))
    for index in range(count):
        address=int.from_bytes(image.read(TABLE+index*4,4),'little');assert address==next_record
        result,flags,evaluator,usage,arity=struct.unpack('<HHIIH',image.read(address,14))
        assert result<62 and flags in (0,1,2) and arity<=8
        assert code.inside(evaluator) if evaluator else index==3
        arguments=struct.unpack('<'+'H'*arity,image.read(address+14,2*arity));assert all(t<62 for t in arguments)
        size=(16+arity*2+3)&~3;next_record=address+size;points.update(range(address,next_record))
        if usage:
            assert 0x41b600<=usage<0x46d6cc
            length=0
            while image.read(usage+length,1)!=b'\0':length+=1;assert length<4096
            points.update(range(usage,usage+length+1))
        rows.append(dict(index=index,address=address,bytes=size,return_type=result,flags=flags,
            evaluator=evaluator,usage=usage,parameters=arguments))
    assert next_record==COUNT
    profile_path=ROOT/'tools/mcc-script-layout.json';profile=json.loads(profile_path.read_bytes())
    assert profile['xbox_tu15_sha256']==NEW_PIN and profile['builtin_table_xbox']==hex(TABLE)
    checked=0
    for admitted in profile['builtins']:
        if admitted['xbox_opcode'] is None:continue
        row=rows[admitted['xbox_opcode']]
        assert row['address']==int(admitted['xbox_descriptor'],16)
        assert row['flags']==admitted['xbox_flags'] and row['return_type']==admitted['xbox_return_type']
        assert list(row['parameters'])==admitted['xbox_arguments'];checked+=1
    return rows,points,checked


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output',type=Path,required=True);ap.add_argument('--report',type=Path,required=True);args=ap.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();assert args.output.resolve().is_relative_to(scratch)
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    rows,points,checked=registry(image);table=image.read(TABLE,len(rows)*4)
    def values(raw):return ',\n    '.join(','.join(f'0x{b:02x}'for b in raw[i:i+32])for i in range(0,len(raw),32))
    lines=['// Original HS function registry and metadata; no executable or completed result.',
        '{',f'constexpr std::array<std::uint8_t,{len(table)}> registry{{'+values(table)+'};',
        'const auto authored=std::as_bytes(std::span(registry));',
        'auto owned=[&](std::uint32_t address) {',
        '    try {(void)memory.view(address,0);return true;}',
        '    catch(const std::out_of_range&){return false;}',
        '};',
        '// Validate prior owners first. Keep exact shared metadata and its access/lifetime.',
        f'for(std::size_t i=0;i<authored.size();++i)if(owned(0x{TABLE:x}u+i) &&',
        f'    memory.view(0x{TABLE:x}u+i,1).front()!=authored[i])',
        f'    throw std::invalid_argument("Original HS registry owner differs at"+std::to_string(0x{TABLE:x}u+i));',
        'for(std::size_t i=0;i<authored.size();) {',
        f'    if(owned(0x{TABLE:x}u+i)){{++i;continue;}}',
        '    const auto first=i++;',
        f'    while(i<authored.size() && !owned(0x{TABLE:x}u+i))++i;',
        f'    memory.mount(0x{TABLE:x}u+first,authored.subspan(first,i-first),SourceRuntimeMemory::Access::ReadWrite,',
        '        "Original TU1.5 authored HS function pointer registry");','}','}']
    operands=[]
    for address,size in regions(points):
        raw=image.read(address,size);operands.append([address,raw.hex()])
        lines.append(f'literal(memory,0x{address:x}u,std::array<std::uint8_t,{size}>{{'+values(raw)+'});')
    args.output.write_text('\n'.join(lines)+'\n')
    report=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        data_sha256=hashlib.sha256(args.output.read_bytes()).hexdigest(),table_address=TABLE,table_bytes=len(table),
        table_sha256=hashlib.sha256(table).hexdigest(),table_access='read-write',original_count_word=COUNT,
        functions=len(rows),definition_range=[DEFINITIONS,COUNT],readonly_bytes=len(points),readonly_operands=operands,
        admitted_profile_signatures_checked=checked,profile_sha256=hashlib.sha256((ROOT/'tools/mcc-script-layout.json').read_bytes()).hexdigest(),
        definitions=rows,evaluators_qualified=False,live_thread_state_supplied=False)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:v for k,v in report.items()if k not in ('definitions','readonly_operands')}))


if __name__=='__main__':main()

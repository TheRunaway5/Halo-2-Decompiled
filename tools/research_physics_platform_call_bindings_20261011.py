#!/usr/bin/env python3
"""Bind every supported lock import caller in the authenticated native world graph."""
import argparse
import hashlib
import json
import re
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT, Xbe, _Code, PIN
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN


def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--correspondence',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    pair=json.loads(args.correspondence.read_text())
    assert pair['extension_tool_sha256']==sha(ROOT/'tools/research_physics_world_platform_runtime_20261011.py')
    assert pair['disc_1_1_sha256']==PIN and pair['later_owned_xbe_sha256']==NEW_PIN
    old_path=ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'
    assert sha(old_path)==PIN and sha(NEW_DEFAULT)==NEW_PIN
    old,new=Xbe(old_path),Xbe(NEW_DEFAULT);oc,nc=_Code(old,'.text'),_Code(new,'.text')
    source=ROOT/'analysis/multiplayer/integration/spawn-source/kernel-sync-world'
    assert sha(source/'sources.json')=='0813f92e273e295bfc7fa146555afcd6e752f73cc5295f305681dfd5bf54505e'
    for row in json.loads((source/'sources.json').read_text()):assert sha(source/row['saved'])==row['sha256']
    declarations=(source/'XboxDev-xboxkrnl.h').read_text()
    assert '#define NTAPI __attribute__((__stdcall__))' in declarations
    for name in ('RtlEnterCriticalSection','RtlLeaveCriticalSection'):
        assert re.search(r'XBAPI VOID NTAPI '+name+r'\s*\(\s*IN PRTL_CRITICAL_SECTION CriticalSection\s*\);',declarations)
    bindings=[]
    for left,right in sorted(pair['instruction_map'].items()):
        p=int(left,16);i=oc.at(p)
        if i.mnemonic not in ('call','jmp') or i.operands[0].type!=3:continue
        m=i.operands[0].mem
        if m.base or m.index:continue
        target=int.from_bytes(old.read(m.disp,4),'little')
        if target not in (0x80000115,0x80000126):continue
        j=nc.at(right);assert j.mnemonic==i.mnemonic=='call' and j.operands[0].type==3
        n=j.operands[0].mem;assert not n.base and not n.index
        assert int.from_bytes(new.read(n.disp,4),'little')==target
        assert pair['instruction_map'][f'{p+i.size:08x}']==right+j.size
        bindings.append(dict(instruction=p,later_instruction=right,target=target,
            return11=p+i.size,return15=right+j.size,import11=m.disp,import15=n.disp,
            bytes11=i.bytes.hex(),bytes15=j.bytes.hex(),callee_stack_cleanup=8,void_result=True))
    assert {r['instruction'] for r in bindings}=={0x2dacad,0x2db25e,0x2db3d2}
    header='''// Generated from authenticated owned callers and retained public API declarations.
// Native void-call ABI preserves incoming registers; no original kernel clobber claim.
#pragma once
#include <array>
#include <cstdint>
namespace halo2::physics_platform_call_detail {
struct Binding { std::uint32_t instruction,target,return11,return15; };
inline constexpr std::array<Binding,3> bindings{{
'''+''.join('    {0x%xu,0x%xu,0x%xu,0x%xu},\n'%(r['instruction'],r['target'],r['return11'],r['return15']) for r in bindings)+'''}};
} // namespace halo2::physics_platform_call_detail
'''
    args.output.mkdir(parents=True,exist_ok=False)
    header_path=args.output/'bindings.hpp';header_path.write_text(header)
    report=dict(schema=1,tool_sha256=sha(__file__),correspondence_sha256=sha(args.correspondence),
        original_sha256=PIN,later_original_sha256=NEW_PIN,api_declaration_sha256=sha(source/'XboxDev-xboxkrnl.h'),
        bindings=bindings,header_sha256=sha(header_path),all_supported_import_callers_bound=True,
        original_kernel_clobbers_proven=False,live_gameplay_proof=False)
    path=args.output/'report.json';path.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
    print(json.dumps(dict(calls=len(bindings),report_sha256=sha(path),header_sha256=sha(header_path))))


if __name__=='__main__':main()

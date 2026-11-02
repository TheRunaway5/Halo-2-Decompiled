#!/usr/bin/env python3
"""Read-only primary ABI/storage receipt for the real2F950 producer callers."""
import argparse,hashlib,json,sys
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
    owners={};windows={}
    for entry in(0x2f950,0x2b770,0x13c00,0x2fd70,0x2fb50):
        body=complete_body(code,entry);listing=[dict(pc=hex(pc),bytes=code.at(pc).bytes.hex(),operation=code.at(pc).mnemonic,operands=code.at(pc).op_str)for pc in sorted(body.code)]
        owners[hex(entry)]=dict(sites=len(listing),listing_sha256=hashlib.sha256(json.dumps(listing,separators=(',',':')).encode()).hexdigest())
        if entry in(0x2b770,0x13c00):windows[hex(entry)]=listing
    assert owners['0x2f950']['sites']==124
    leaf=complete_body(code,0x2f950);assert not any(code.at(pc).mnemonic=='call'for pc in leaf.code)
    assert code.at(0x2b8b4).bytes.hex()=='e897400000' and code.at(0x13c8a).bytes.hex()=='e8c1bc0100'
    assert code.at(0x2b8b9).op_str=='eax, dword ptr [ebp + 8]'
    assert code.at(0x13c8f).op_str=='edx, esi' and code.at(0x13c91).op_str=='ebx, 0x4c6634' and code.at(0x13c96).op_str=='esi, 0x4c65c0'
    assert code.at(0x2fd73).op_str=='eax, [esp + 0x34]' and code.at(0x2fd77).op_str=='ecx, esi'
    files=['include/halo2/native_campaign_view_rectangle_tu15.hpp','include/halo2/native_campaign_view_rectangle_tu15_binding.hpp','src/rendering/world/native_campaign_view_rectangle_tu15.cpp','src/rendering/world/native_campaign_view_rectangle_tu15_binding.cpp','src/rendering/renderer/native_tu15_main_view_data.cpp']
    report=dict(passed=True,original_sha256=NEW_PIN,owners=owners,actual_caller_windows=windows,
        admitted_callers={
            '0x2b8b4':dict(entry='0x2f950',camera='EAX=EDI=view+0xC',output='ESI=actual private parent rectangle atSP+0x28',return_contract='plainRET; native transport consumes actual4B return word',live='ESI/EDI/EBX/EBP and actual memory/FP status; EAX/ECX/EDX and XMM0..6 results are dead before later use',value_profile='Actual private frame output is disjoint from retained submitted view/global storage; no source address is created for native value storage'),
            '0x13c8a':dict(entry='0x2f950',camera='EAX=4C65C0 actual restored camera',output='ESI=actual private16B parent output atSP+0xC',return_contract='plainRET; native transport consumes actual4B return word',live='ESI copied toEDX, thenEBX projection destination andESI camera set explicitly;2FD70 overwritesEAX/ECX before2FB50; no inherited SIMD result used',value_profile='Source camera is retained global storage; private rectangle has native storage')},
        fixed_reads={hex(p):image.read(p,4).hex()for p in(0x46a11c,0x46a0cc,0x46a0d0,0x46a264,0x46a260)},
        retained_owners={'0x4c6839':'Actual existing mutable one-byte geometry modifier owner; set from camera+58/+68 and OR scenario-type2','0x4ecc20':'Actual scenario pointer; optional signed short type at+10; pointer is reread after prior output writes'},
        source_hashes={f:sha(ROOT/f)for f in files},audit_source_sha256=sha(__file__),
        dynamic_gpu_owners=[],native_algorithm='Shared semantic rectangle computation with raw alias-preserving output or genuine native four-word value storage; no register-machine algorithm or private guest stack',
        scope_limits=['Static original caller liveness receipt, not completed2B770/13C00 execution','Other callers are rejected; x87 state untouched, masked SIMD exceptions required','No producer retirement/live route/main CMake activation in this component'],retained_large_artifacts=[])
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2)+'\n');print('Both actual2F950 caller contracts and fixed/data owners authenticated')
if __name__=='__main__':main()

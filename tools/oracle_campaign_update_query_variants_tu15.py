#!/usr/bin/env python3
"""Execute the four original update-query variants reached after Marine birth.

Reuse only declared component input controls from the independent birth-query
reference, never its outputs or dependency returns. The unchanged original
update routines produce every expected register, vector and ordered write.
Stop before real indirect callbacks. Coalesce Unicorn's split vector stores
at the actual instruction boundary, retaining all prior and produced bytes.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from oracle_campaign_ai_placement_settings_tu15 import GP
from oracle_campaign_birth_query_variants_tu15 import BASE,STACK,STOP,CONTEXT,CONTEXT_BYTES

ENTRIES={(1,1):0x290b50,(1,0):0x2912a0,(0,1):0x291740,(0,0):0x291e30}
ADDITIONAL={0x2843f0,0x2849f0,0x284ee0,0x285630,0x289240,0x289920,0x289d60,0x28a3f0,
    0x28cc70,0x28d310,0x28d6e0,0x28dd20}


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--controls',type=Path,required=True);ap.add_argument('--fixture',type=Path,required=True)
    ap.add_argument('--report',type=Path,required=True);ap.add_argument('--verify',action='store_true')
    ap.add_argument('--additional-families',action='store_true');args=ap.parse_args()
    assert 'HALO2_TEST_SCRATCH' in os.environ
    controls_bytes=args.controls.read_bytes();controls=json.loads(controls_bytes)
    assert controls['original_xbe_sha256']==NEW_PIN and not controls['instruction_patches'] and not controls['supplied_dependency_returns']
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    entries=ADDITIONAL if args.additional_families else set(ENTRIES.values())
    code=_Code(image,'.text');bodies={p:_trace(code,p,entries) for p in entries}
    sites=set().union(*(set(b.code) for b in bodies.values()));covered=set();cases=[]
    mask_controls={(0,0,0),(255,0,0),(0,255,0),(0,0,255),(85,128,170)}
    specs=[]
    for ordinal,control in enumerate(controls['cases']):
        if control['count'] not in (-1,0,1,8,9,32,33) or tuple(control['masks']) not in mask_controls or control['profile']=='remap_negative':continue
        if args.additional_families:
            if control['mapped']!=1 or control['sse']!=0 or control['poison']!=0xa5:continue
            for entry in sorted(ADDITIONAL):specs.append((ordinal,control,entry))
        else:specs.append((ordinal,control,ENTRIES[control['mapped'],control['sse']]))
    for ordinal,control,entry in specs:
        g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x600000)
        for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
        g.mem_map(BASE,0x10000);g.mem_write(BASE,bytes([control['poison']])*0x10000);g.mem_map(STOP,4096)
        patches=list(control['patches'])+[[0x510f50,struct.pack('<f',.5).hex()]]
        for address,bits in patches:g.mem_write(address,bytes.fromhex(bits))
        initial=control['gp'];xmm=control['xmm']
        for reg,value in zip(GP,initial):g.reg_write(reg,value)
        for i,lanes in enumerate(xmm):g.reg_write(R.UC_X86_REG_XMM0+i,int.from_bytes(struct.pack('<4I',*lanes),'little'))
        g.reg_write(R.UC_X86_REG_EFLAGS,0x202);g.reg_write(R.UC_X86_REG_FPCW,0x27f);g.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
        trace=[];stores=[];pending=[];stop={}
        def flush():
            if not pending:return
            pc=pending[0][0];assert all(v[0]==pc for v in pending)
            ins=code.at(pc);width=ins.operands[0].size if ins.operands and ins.operands[0].type==3 else 0
            if width==16 and sum(len(v[2]) for v in pending)==16:
                assert all(pending[i-1][1]+len(pending[i-1][2])==pending[i][1] for i in range(1,len(pending)))
                start=pending[0][1];stores.append([pc,start,b''.join(v[2] for v in pending).hex(),bytes(g.mem_read(start,16)).hex()])
            else:
                for at,address,before in pending:stores.append([at,address,before.hex(),bytes(g.mem_read(address,len(before))).hex()])
            pending.clear()
        def instruction(_g,pc,size,_u):
            flush();assert pc in sites and bytes(g.mem_read(pc,size))==image.read(pc,size),hex(pc)
            if code.at(pc).mnemonic in ('call','int3'):stop['prefix_end']=pc;g.emu_stop();return
            trace.append(pc);covered.add(pc)
        def store(_g,_kind,address,size,_value,_u):
            pending.append((g.reg_read(R.UC_X86_REG_EIP),address,bytes(g.mem_read(address,size))))
        g.hook_add(UC_HOOK_CODE,instruction);g.hook_add(UC_HOOK_MEM_WRITE,store)
        try:g.emu_start(entry,STOP,count=100000)
        except Exception as error:
            raise RuntimeError(f'Original update entry={entry:x}, control={ordinal},pc={g.reg_read(R.UC_X86_REG_EIP):x}') from error
        flush();assert stop or g.reg_read(R.UC_X86_REG_EIP)==STOP
        cases.append(dict(entry=entry,prefix_end=stop.get('prefix_end'),control_case=ordinal,
            mapped=control['mapped'],sse=control['sse'],count=control['count'],masks=control['masks'],profile=control['profile'],
            poison=control['poison'],patches=patches,gp=initial,xmm=xmm,trace=trace,stores=stores,
            final_gp=[g.reg_read(reg) for reg in GP],final_flags=g.reg_read(R.UC_X86_REG_EFLAGS),
            final_xmm=[list(struct.unpack('<4I',g.reg_read(R.UC_X86_REG_XMM0+i).to_bytes(16,'little'))) for i in range(8)],
            outputs=[[CONTEXT,bytes(g.mem_read(CONTEXT,CONTEXT_BYTES)).hex()],
                [STACK-0x100,bytes(g.mem_read(STACK-0x100,0x104)).hex()]]))
    scope=__doc__ if not args.additional_families else ('Execute all twelve remaining original query target bodies.\n'+__doc__)
    fixture=dict(scope=scope,original_xbe_sha256=NEW_PIN,
        component_controls_sha256=hashlib.sha256(controls_bytes).hexdigest(),instruction_patches=[],supplied_dependency_returns=[],
        native_component_banks=controls['native_component_banks'],immutable_operands=[[0x46a0d0,'0000803f']],cases=cases)
    data=(json.dumps(fixture,separators=(',',':'))+'\n').encode()
    if args.verify or args.fixture.exists():assert args.fixture.read_bytes()==data,'Preserve original update query reference'
    else:args.fixture.parent.mkdir(parents=True,exist_ok=True);args.fixture.write_bytes(data)
    report=dict(scope=scope,original_xbe_sha256=NEW_PIN,cases=len(cases),
        whole_cases=sum(c['prefix_end'] is None for c in cases),callback_prefixes=sum(c['prefix_end'] is not None for c in cases),
        component_controls_sha256=fixture['component_controls_sha256'],original_coverage={f'{p:08x}':dict(
            total=len(b.code),observed=len(set(b.code)&covered),unexecuted=sorted(set(b.code)-covered)) for p,b in bodies.items()},
        fixture_bytes=len(data),fixture_sha256=hashlib.sha256(data).hexdigest(),
        tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),runtime_reference_required=False)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))


if __name__=='__main__':main()

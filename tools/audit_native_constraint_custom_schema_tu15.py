#!/usr/bin/env python3
"""Authenticate actual TU1.5 callback registration, ABI and FP liveness."""
import argparse,hashlib,json
from pathlib import Path
from capstone.x86_const import X86_OP_REG,X86_OP_IMM
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
from audit_native_constraint_solver_entry_tu15 import ALIASES

def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args();args.output.mkdir(parents=True,exist_ok=True)
    assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);code=_Code(image,'.text');owners=[]
    for entry,count in((0x31f3a0,34),(0x31f410,26),(0x31f560,60)):
        body=complete_body(code,entry);assert len(body.code)==count;defined={'esp':0xffffffff};violations=[];private=0;maximum=0;listing=[]
        for pc in sorted(body.code):
            ins=code.at(pc);listing.append(f'{pc:x} {ins.bytes.hex()} {ins.mnemonic} {ins.op_str}\n');reads,writes=ins.regs_access()
            for reg in reads:
                name=ins.reg_name(reg)
                if name in ALIASES:
                    base,mask=ALIASES[name]
                    if pc==0x31f560 and base=='ebp':continue
                elif name.startswith('xmm'):base,mask=name,0xffffffff
                else:continue
                if mask&~defined.get(base,0):violations.append(dict(instruction=hex(pc),register=name))
            for reg in writes:
                name=ins.reg_name(reg)
                if name in ALIASES:base,mask=ALIASES[name]
                elif name.startswith('xmm'):base,mask=name,0xffffffff
                else:continue
                defined[base]=defined.get(base,0)|mask
            if ins.mnemonic=='fld':private+=1;maximum=max(maximum,private)
            elif ins.mnemonic=='fstp':assert private>=1;private-=1
            elif ins.mnemonic.startswith('f'):assert ins.mnemonic in('fmul','fadd')and private>=1
            assert not ins.mnemonic.startswith('j')and ins.mnemonic!='call',(hex(pc),ins.mnemonic)
        assert private==0 and maximum<=1 and not violations
        owners.append(dict(owner=hex(entry),sites=count,listing_sha256=hashlib.sha256(''.join(listing).encode()).hexdigest(),
                           cdecl_arguments=['row unused','payload read from actual ESP+8'],incoming_gp_or_xmm_uses=violations,
                           private_x87_maximum=maximum,private_x87_return_depth=private,incoming_x87_values_read=False))
    registration=[]
    for push,owner,call in((0x31f7be,0x31f560,0x31f7c3),(0x31f893,0x31f3a0,0x31f8a0),(0x31f959,0x31f410,0x31f95e)):
        a,b=code.at(push),code.at(call);assert a.mnemonic=='push'and a.operands[0].type==X86_OP_IMM and a.operands[0].imm==owner;assert b.mnemonic=='call'and b.operands[0].imm==0x32a390
        registration.append(dict(push=hex(push),callback_owner=hex(owner),emitter_call=hex(call),emitter_owner='0x32a390',push_bytes=a.bytes.hex(),call_bytes=b.bytes.hex()))
    integer_clock=[]
    for owner in(0x37c5c0,0x32bbae,0x32ae00,0x33a4d3):
        body=complete_body(code,owner);listing=[]
        for pc in sorted(body.code):
            ins=code.at(pc);assert not ins.mnemonic.startswith('f')and not any(ins.reg_name(reg).startswith(('st(','xmm','mm'))for reg in ins.regs_access()[0]+ins.regs_access()[1]);listing.append(f'{pc:x} {ins.bytes.hex()} {ins.mnemonic} {ins.op_str}\n')
        integer_clock.append(dict(owner=hex(owner),sites=len(body.code),listing_sha256=hashlib.sha256(''.join(listing).encode()).hexdigest(),x87_or_simd_operations=False))
    report=dict(passed=True,image_sha256=NEW_PIN,callback_owners=owners,actual_producer_registration=registration,
                clock_closure_to_platform_import=integer_clock,private_solver_depth_at_callback=0,
                source_hashes={'tools/audit_native_constraint_custom_schema_tu15.py':sha(__file__)},
                limits=['Static source ABI/liveness evidence; dynamic math qualification is separate',
                        'Only the three authenticated built-ins qualify; unknown registered callbacks require separate proof',
                        'Logger2E3850 and actual host clock import implementation require their own transport contract'])
    (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(passed=True,callback_owners=len(owners),integer_clock_owners=len(integer_clock))))
if __name__=='__main__':main()

#!/usr/bin/env python3
"""Actual TU1.5 schema emitter entry and internal-caller liveness audit."""
import argparse,hashlib,json,struct
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
from audit_native_constraint_solver_entry_tu15 import ALIASES
def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def listing(code,body):return ''.join(f'{p:x} {code.at(p).bytes.hex()} {code.at(p).mnemonic} {code.at(p).op_str}\n'for p in sorted(body.code))
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);args=p.parse_args();args.output.mkdir(parents=True,exist_ok=True);assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);code=_Code(image,'.text');body=complete_body(code,0x32a390);assert len(body.code)==15
    defined={'esp':0xffffffff};undefined=[]
    for pc in sorted(body.code):
        ins=code.at(pc);reads,writes=ins.regs_access();assert not ins.mnemonic.startswith('f')and ins.mnemonic!='call'
        for reg in reads:
            name=ins.reg_name(reg)
            if name not in ALIASES:continue
            base,mask=ALIASES[name]
            if pc==0x32a39b and base=='esi':continue
            if mask&~defined.get(base,0):undefined.append(dict(instruction=hex(pc),register=name))
        for reg in writes:
            name=ins.reg_name(reg)
            if name in ALIASES:base,mask=ALIASES[name];defined[base]=defined.get(base,0)|mask
    assert not undefined
    section=next(s for s in image.sections if s.name=='.text');raw=image.section_bytes(section);calls=[]
    for offset in range(len(raw)-4):
        if raw[offset]==0xe8 and(section.va+offset+5+struct.unpack_from('<i',raw,offset+1)[0])&0xffffffff==0x32a390:calls.append(section.va+offset)
    assert calls==[0x31f7c3,0x31f8a0,0x31f95e]
    callers=[]
    for call in calls:
        first={};pc=call+5;instructions=[]
        for n in range(100):
            ins=code.at(pc);instructions.append(f'{pc:x} {ins.bytes.hex()} {ins.mnemonic} {ins.op_str}\n');reads,writes=ins.regs_access();read_names={ins.reg_name(r)for r in reads};write_names={ins.reg_name(r)for r in writes}
            for name in('ecx','edx','eflags'):
                if name not in first and name in(read_names|write_names):first[name]=dict(instruction=hex(pc),access='read'if name in read_names else'write',text=ins.mnemonic+' '+ins.op_str)
            if len(first)==3:break
            assert not ins.mnemonic.startswith('j')and ins.mnemonic not in('call','ret'),(hex(call),hex(pc),'Unproved control transfer before overwrite')
            pc+=ins.size
        assert len(first)==3 and all(event['access']=='write'for event in first.values())
        callers.append(dict(callsite=hex(call),return_site=hex(call+5),required_native_result='EAX actual payload pointer',first_volatile_or_flags_access=first,continuation_listing_sha256=hashlib.sha256(''.join(instructions).encode()).hexdigest()))
    report=dict(passed=True,image_sha256=NEW_PIN,entry='0x32a390',body_sites=15,listing_sha256=hashlib.sha256(listing(code,body).encode()).hexdigest(),
                cdecl_arguments=['ESP+4 callback read after first header store','ESP+8 payload bytes read early','ESP+C unused','ESP+10 retained builder packet read early'],callee_pops=0,
                entry_physics_inputs=['Actual four argument cells/typed sources','Actual retained packet and schema memory'],undefined_gp_entry_uses=undefined,fp_or_xmm_operations=False,
                true_return='EAX actual payload pointer',other_defined_volatile_results=['ECX actual retained packet identity','EDX actual advanced schema cursor'],
                all_text_direct_calls=calls,internal_caller_contracts=callers,
                source_hashes={'tools/audit_native_constraint_schema_emitter_tu15.py':sha(__file__)},
                limits=['Source liveness proof is separate from physical store/numerical qualification','Only authenticated internal callers are covered by the EAX-only continuation contract','Unknown indirect callers require their own ABI proof'])
    (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(passed=True,internal_callers=len(callers))))
if __name__=='__main__':main()

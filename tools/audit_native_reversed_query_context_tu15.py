#!/usr/bin/env python3
"""Source local GP/XMM liveness for reversed-query parents and30E930."""
import argparse,hashlib,inspect,json,os,struct
from pathlib import Path
from capstone.x86_const import X86_OP_REG
import audit_native_contact_convex_query_context_tu15 as parent
OWNERS=(0x2f1a50,0x2f0f10,0x2f6f40,0x2f8410,0x30e930,0x30ba80,0x8eb50)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);a=p.parse_args();assert os.environ.get('HALO2_TEST_SCRATCH');assert parent.sha(parent.NEW_DEFAULT)==parent.NEW_PIN
 image=parent.Xbe(parent.NEW_DEFAULT);code=parent._Code(image,'.text')
 parent.SAVES={pc for owner in OWNERS for pc in parent.complete_body(code,owner).code if owner<=pc<owner+0x45 and code.at(pc).mnemonic=='push'and code.at(pc).operands[0].type==X86_OP_REG and code.at(pc).op_str in('ebp','ebx','esi','edi')}
 gp=[parent.gp_audit(image,code,o)for o in OWNERS]
 source=inspect.getsource(parent.xmm.audit)
 old="elif m=='jmp':assert ops[0].type==X86_OP_IMM;successors=[ops[0].imm]"
 new="elif m=='jmp':\n   if ops[0].type==X86_OP_IMM:successors=[ops[0].imm]\n   else:\n    assert pc==0x30ea37\n    successors=list(struct.unpack('<11I',image.read(0x30f6e0,44)))"
 assert source.count(old)==1;source=source.replace(old,new)
 source=source.replace("  if m.startswith('ret'):successors=[]","  if m=='call':defined=[0]*8\n  if m.startswith('ret'):successors=[]")
 environment=dict(parent.xmm.__dict__,struct=struct,image=image);exec(source,environment)
 lanes=[environment['audit'](code,o)for o in OWNERS]
 result=dict(passed=all(r['passed']for r in gp)and all(r['local_xmm_liveness_passed']for r in lanes),original_sha256=parent.NEW_PIN,GP=gp,XMM_invalidated_after_each_call=lanes,source_private_save_bytes_serialized=False,transitive_native_solver_closure_proven=False,native_kernel_validated=False,scope='Authenticated localCFG only. ECX typedreceiver/ESP formalarguments permitted; control-only prologue saves excluded. Calls remain explicit boundaries. No GP/XMM postimages supplied.',source_hashes={str(Path(__file__).relative_to(parent.ROOT)):parent.sha(__file__)},retained_large_artifacts=[])
 a.output.mkdir(parents=True,exist_ok=False);(a.output/'report.json').write_text(json.dumps(result,indent=2,sort_keys=True)+'\n');print(result['passed']);return 0 if result['passed']else 1
if __name__=='__main__':raise SystemExit(main())

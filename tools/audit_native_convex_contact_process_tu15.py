#!/usr/bin/env python3
"""Local ABI and post-call XMM audit for convex process/native service seams."""
import argparse,hashlib,inspect,json,os
from pathlib import Path
from capstone.x86_const import X86_OP_REG,X86_OP_IMM
import audit_native_contact_convex_query_context_tu15 as parent
OWNERS=(0x2f8b60,0x2e5320,0x30cd00,0x30cf40)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);a=p.parse_args();assert os.environ.get('HALO2_TEST_SCRATCH');assert parent.sha(parent.NEW_DEFAULT)==parent.NEW_PIN
 image=parent.Xbe(parent.NEW_DEFAULT);code=parent._Code(image,'.text');parent.SAVES={pc for owner in OWNERS for pc in parent.complete_body(code,owner).code if owner<=pc<owner+0x30 and code.at(pc).mnemonic=='push'and code.at(pc).operands[0].type==X86_OP_REG and code.at(pc).op_str in('ebp','ebx','esi','edi')}
 source=inspect.getsource(parent.gp_audit).replace('states={owner:(0,15,0,0,15,0,0,0)}','states={owner:(0,0 if owner==0x30cd00 else 15,0,0,15,0,0,0)}')
 source=source.replace('  for r in reads:\n','''  constant_destination=None
  if len(i.operands)>=2 and i.operands[0].type==X86_OP_REG:
   left,right=i.operands[:2]
   if i.mnemonic=='xor'and right.type==X86_OP_REG and left.reg==right.reg:constant_destination=left.reg
   if right.type==X86_OP_IMM and ((i.mnemonic=='or'and(right.imm&((1<<(left.size*8))-1))==(1<<(left.size*8))-1)or(i.mnemonic=='and'and right.imm==0)):constant_destination=left.reg
  for r in reads:
   if r==constant_destination:continue
''')
 environment=dict(parent.__dict__);exec(source,environment);gp=[environment['gp_audit'](image,code,o)for o in OWNERS]
 ordinary=[parent.xmm.audit(code,o)for o in OWNERS]
 xmm_source=inspect.getsource(parent.xmm.audit).replace('  if m.startswith(\'ret\'):successors=[]','  if m==\'call\':defined=[0]*8\n  if m.startswith(\'ret\'):successors=[]')
 environment=dict(parent.xmm.__dict__);exec(xmm_source,environment);postcall=[environment['audit'](code,o)for o in OWNERS]
 report=dict(passed=all(r['passed']for r in gp)and all(r['local_xmm_liveness_passed']for r in postcall),original_sha256=parent.NEW_PIN,GP=gp,XMM=ordinary,XMM_after_each_call_invalidated=postcall,constant_idiom_rules=['xor register,self','or register,allones','and register,zero'],CD00_receiver_is_cdecl_only=True,CF40_receiver_is_current_typed_context=True,source_private_save_bytes_serialized=False,transitive_callee_native_closure_proven=False,native_kernel_validated=False,scope='All local authenticatedCFG paths. Callee-saved prologue stores are control-only; result/data/service publication and transitive child behavior remain explicit. Postcall XMM cleared rather than supplying arbitrary callee postimages.',source_hashes={str(Path(__file__).relative_to(parent.ROOT)):parent.sha(__file__)},retained_large_artifacts=[])
 a.output.mkdir(parents=True,exist_ok=False);(a.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');print([(hex(r['entry']),r['incoming_GP_before_local_definition'])for r in gp]);print([(hex(r['entry']),r['incoming_xmm_reads_before_local_definition'])for r in postcall]);return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())

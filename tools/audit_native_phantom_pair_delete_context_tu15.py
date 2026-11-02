#!/usr/bin/env python3
"""Local typed-input liveness of authenticated320040/2E4D60 only."""
import argparse,hashlib,inspect,json,os
from pathlib import Path
import audit_native_pair_callbacks_context_tu15 as audit
from capstone.x86_const import X86_OP_IMM,X86_OP_REG
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN

def sha(path):return hashlib.file_digest(Path(path).open('rb'),'sha256').hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();assert os.environ.get('HALO2_TEST_SCRATCH')and sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);code=_Code(image,'.text');previous=audit.successors
 def successors(image,code,pc):
  i=code.at(pc)
  if pc==0x32009a:
   assert i.mnemonic=='jmp'and i.operands[0].type!=X86_OP_IMM
   return []
  return previous(image,code,pc)
 audit.successors=successors;audit.SAVES|={0x320040};source=inspect.getsource(audit.gp_audit);needle='  save=pc in SAVES';assert source.count(needle)==1;source=source.replace(needle,"  if pc==0x320083:\n   assert i.bytes.hex()=='33d2' and i.mnemonic=='xor' and i.operands[0].type==X86_OP_REG and i.operands[0].reg==i.operands[1].reg\n   reads=[r for r in reads if r!=i.operands[0].reg] # Authenticated full-word zero definition.\n"+needle);audit.__dict__.update(X86_OP_REG=X86_OP_REG);exec(source,audit.__dict__);owners=(0x320040,0x2e4d60);gp=[audit.gp_audit(image,code,o)for o in owners];xmm=[dict(entry=o,body_sites=len(audit.complete_body(code,o).code),XMM_operand_sites=[pc for pc in audit.complete_body(code,o).code if any(code.at(pc).reg_name(r).startswith('xmm')for r in sum(code.at(pc).regs_access(),[]))])for o in owners]
 j=dict(passed=all(x['passed']for x in gp)and all(not x['XMM_operand_sites']for x in xmm),original_sha256=NEW_PIN,owners_GP=gp,owners_XMM=xmm,scope='Authenticated all-local paths;32009A tailjmp is typedRET4 service boundary, no childpostimage inferred; source320083 XOR EDX,EDX is full zero definition, not incoming semantic input',source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__),str(Path(audit.__file__).relative_to(ROOT)):sha(audit.__file__)},audit_function_sha256=hashlib.sha256(source.encode()).hexdigest(),retained_large_artifacts=[],owned_scratch=os.environ['HALO2_TEST_SCRATCH']);a.output.mkdir(parents=True,exist_ok=False);(a.output/'report.json').write_text(json.dumps(j,indent=2,sort_keys=True)+'\n');print(j['passed']);return 0 if j['passed']else 1
if __name__=='__main__':raise SystemExit(main())

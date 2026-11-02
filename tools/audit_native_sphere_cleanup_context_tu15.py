#!/usr/bin/env python3
"""Read-only actual2F9960/2F6C80 semantic incoming-register audit."""
import argparse,hashlib,inspect,json,os
from pathlib import Path
import audit_native_pair_callbacks_context_tu15 as audit
from capstone.x86_const import X86_OP_REG
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN

def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();assert os.environ.get('HALO2_TEST_SCRATCH')and sha(NEW_DEFAULT)==NEW_PIN
 image=Xbe(NEW_DEFAULT);code=_Code(image,'.text');owners=(0x2f9960,0x2f6c80)
 audit.SAVES|={0x2f9960,0x2f6c80}
 source=inspect.getsource(audit.gp_audit);marker='  save=pc in SAVES';assert source.count(marker)==1
 # A32-bit XOR of a register with itself defines zero. Capstone's access
 # flags also list the destination as read; that is not an incoming operand.
 source=source.replace(marker,'''  if i.mnemonic=='xor' and len(i.operands)==2 and all(o.type==X86_OP_REG and o.size==4 for o in i.operands) and i.operands[0].reg==i.operands[1].reg:
   reads=[r for r in reads if r!=i.operands[0].reg]
'''+marker)
 audit.__dict__['X86_OP_REG']=X86_OP_REG;exec(source,audit.__dict__)
 gp=[audit.gp_audit(image,code,o)for o in owners]
 xmm=[dict(entry=o,body_sites=len(audit.complete_body(code,o).code),operand_sites=[pc for pc in audit.complete_body(code,o).code if any(code.at(pc).reg_name(r).startswith('xmm')for r in sum(code.at(pc).regs_access(),[]))])for o in owners]
 report=dict(passed=all(r['passed']for r in gp)and all(not r['operand_sites']for r in xmm),original_sha256=NEW_PIN,owners_GP=gp,owners_XMM=xmm,
     scope='All local source paths; ECX receiver only, no semantic incoming volatile GP/XMM value. Actual caller2F6C88 consumes no returned value before its next deleting destructor. Transitive manager/scalar/allocator owners require independent qualification; no full runtime claim.',
     source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__),str(Path(audit.__file__).relative_to(ROOT)):sha(audit.__file__)},audit_function_sha256=hashlib.sha256(source.encode()).hexdigest(),owned_scratch=os.environ['HALO2_TEST_SCRATCH'],retained_large_artifacts=[])
 a.output.mkdir(parents=True,exist_ok=False);(a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(report['passed']);return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())

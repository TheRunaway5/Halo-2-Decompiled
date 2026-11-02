#!/usr/bin/env python3
"""Authenticated typed-input liveness of native convex-agent factory parents."""
import argparse,hashlib,json,os
from pathlib import Path
import audit_native_pair_callbacks_context_tu15 as audit
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN

def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();assert os.environ.get('HALO2_TEST_SCRATCH')and sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);code=_Code(image,'.text');audit.SAVES|={0x2f8300,0x2f8321,0x2f8322,0x2f8325,0x2e4ece};owners=(0x2e4eb0,0x2f8300);gp=[audit.gp_audit(image,code,o)for o in owners];xmm=[dict(entry=o,body_sites=len(audit.complete_body(code,o).code),XMM_operand_sites=[pc for pc in audit.complete_body(code,o).code if any(code.at(pc).reg_name(r).startswith('xmm')for r in sum(code.at(pc).regs_access(),[]))])for o in owners];j=dict(passed=all(x['passed']for x in gp)and all(not x['XMM_operand_sites']for x in xmm),original_sha256=NEW_PIN,owners_GP=gp,owners_XMM=xmm,scope='All local sourcepaths;2E4EB0 formalcdecl payload/unusedcontext/userdata only, ECX saved as control then overwrittenfromallocatorglobal;2F8300 ECXagent and3formalargs. No volatileGP/XMM incoming dependency; frame alignment/SEH/controlsaves are absent from nativesemantic kernels. Shape/cache/geometry/allocator transitive owners retain separately qualifiedtyped DATA boundaries.',source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__),str(Path(audit.__file__).relative_to(ROOT)):sha(audit.__file__)},owned_scratch=os.environ['HALO2_TEST_SCRATCH'],retained_large_artifacts=[]);a.output.mkdir(parents=True,exist_ok=False);(a.output/'report.json').write_text(json.dumps(j,indent=2,sort_keys=True)+'\n');print(j['passed']);return 0 if j['passed']else 1
if __name__=='__main__':raise SystemExit(main())

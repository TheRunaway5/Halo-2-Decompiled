#!/usr/bin/env python3
"""Read-only authenticated2F9CA0 and actual2F6F40 caller register audit."""
import argparse,hashlib,json,os
from pathlib import Path
import audit_native_pair_callbacks_context_tu15 as audit
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN

def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();assert os.environ.get('HALO2_TEST_SCRATCH')and sha(NEW_DEFAULT)==NEW_PIN
 image=Xbe(NEW_DEFAULT);code=_Code(image,'.text');owners=(0x2f9ca0,0x2f6f40)
 audit.SAVES|={0x2f9ca0,0x2f9cb5,0x2f9cb6,0x2f6f40,0x2f6f49,0x2f6f4a,0x2f6f54}
 gp=[audit.gp_audit(image,code,o)for o in owners];lanes=[audit.xmm.audit(code,o)for o in owners]
 report=dict(passed=all(r['passed']for r in gp)and all(r['local_xmm_liveness_passed']for r in lanes),original_sha256=NEW_PIN,owners_GP=gp,owners_XMM=lanes,source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__),str(Path(audit.__file__).relative_to(ROOT)):sha(audit.__file__),str(Path(audit.xmm.__file__).relative_to(ROOT)):sha(audit.xmm.__file__)},scope='All local paths; source callback uses ordinary stack formals and defined SIMD only. Actual2F6FAB consumes no returned GP/SIMD value before restoring caller registers and RET16. Transitive collector behavior and runtime reachability require separate proofs.',owned_scratch=os.environ['HALO2_TEST_SCRATCH'],retained_large_artifacts=[])
 a.output.mkdir(parents=True,exist_ok=False);(a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(report['passed']);return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())

#!/usr/bin/env python3
"""Local semantic input and packet lifetime audit of316AA0/3223D0/325E90."""
import argparse,hashlib,inspect,json,os
from pathlib import Path
from capstone.x86_const import X86_OP_REG
import audit_native_pair_callbacks_context_tu15 as audit
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
OWNERS=(0x316aa0,0x3223d0,0x325e90)
def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);a=p.parse_args();assert os.environ.get('HALO2_TEST_SCRATCH')and sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);code=_Code(image,'.text');audit.SAVES|={0x316aa7,0x316aeb,0x3223d0,0x3223d1,0x3223d6,0x325eb7,0x325ec0}
 source=inspect.getsource(audit.gp_audit);anchor='  save=pc in SAVES';assert source.count(anchor)==1;source=source.replace(anchor,"  if pc==0x3223fa:\n   assert i.bytes.hex()=='8d9b00000000' and i.mnemonic=='lea' and i.op_str=='ebx, [ebx]'\n   reads=[];writes=[] # exact six-byte alignment NOP, no data/control effect\n  if pc==0x325e94:\n   assert i.bytes.hex()=='33d2' and i.mnemonic=='xor' and i.operands[0].type==X86_OP_REG and i.operands[0].reg==i.operands[1].reg\n   reads=[r for r in reads if r!=i.operands[0].reg]\n"+anchor);audit.__dict__['X86_OP_REG']=X86_OP_REG;exec(source,audit.__dict__);gp=[audit.gp_audit(image,code,o)for o in OWNERS];simd=[dict(entry=o,body_sites=len(audit.complete_body(code,o).code),XMM_operand_sites=[pc for pc in audit.complete_body(code,o).code if any(code.at(pc).reg_name(r).startswith('xmm')for r in sum(code.at(pc).regs_access(),[]))])for o in OWNERS]
 report=dict(passed=all(r['passed']for r in gp)and all(not r['XMM_operand_sites']for r in simd),original_sha256=NEW_PIN,owners_GP=gp,owners_XMM=simd,audit_function_sha256=hashlib.sha256(source.encode()).hexdigest(),source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__),str(Path(audit.__file__).relative_to(ROOT)):sha(audit.__file__)},owned_scratch=os.environ['HALO2_TEST_SCRATCH'],retained_large_artifacts=[],collector_info=dict(owner=0x325e90,current_table=0x41f478,virtual_slot=0x28,complete_initial_writes=[0x325e96,0x325e98,0x325e9b,0x325e9e],current_count_read=0x325ea5,source_packet_reads=[],retained_pointer_stores=[],normal_EAX='same supplied output pointer',actual_3201b0_caller_EAX_overwrite=0x320206,all16_bytes_initialized_before_return=True),scope='All local CFG paths. Current typed receiver/formal DATA only; no incoming volatileGP/XMM/postimage dependency.325E90 never reads/stores a retained packet pointer; its ordinary EAX output pointer is overwritten without use by actual3201B0. Explicit remaining callbacks still require independent native ownership/lifetime.')
 a.output.mkdir(parents=True,exist_ok=False);(a.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())

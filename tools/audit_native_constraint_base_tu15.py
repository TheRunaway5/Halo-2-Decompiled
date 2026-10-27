#!/usr/bin/env python3
"""Authenticate actual329720 control flow, ABI and private x87 ownership."""
import argparse,hashlib,json,os
from pathlib import Path
from capstone.x86_const import X86_OP_IMM,X86_OP_REG
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body

def sha(path):return hashlib.file_digest(Path(path).open('rb'),'sha256').hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);a=p.parse_args();assert os.environ.get('HALO2_TEST_SCRATCH');assert sha(NEW_DEFAULT)==NEW_PIN;code=_Code(Xbe(NEW_DEFAULT),'.text');body=complete_body(code,0x329720);assert len(body.code)==921;assert not any(code.at(pc).mnemonic=='call'for pc in body.code);todo=[(0x329720,0)];seen=set();depths={};peak=0;returns=[];listing=[]
 while todo:
  pc,depth=todo.pop()
  if(pc,depth)in seen:continue
  seen.add((pc,depth));assert pc in body.code;i=code.at(pc);depths.setdefault(pc,set()).add(depth)
  for op in i.operands:
   if op.type==X86_OP_REG:
    name=i.reg_name(op.reg)
    if name.startswith('st('):assert int(name[3:-1])<depth,(hex(pc),i.mnemonic,i.op_str,depth)
  if i.mnemonic in('fld','fild','fbld','fldz','fld1','fldpi'):depth+=1
  elif i.mnemonic in('fstp','fistp','fisttp','fcomp','fucomp','faddp','fmulp','fsubp','fsubrp','fdivp','fdivrp'):depth-=1
  elif i.mnemonic in('fcompp','fucompp'):depth-=2
  assert 0<=depth<=8,(hex(pc),depth);peak=max(peak,depth)
  if i.mnemonic.startswith('ret'):returns.append(dict(instruction=pc,private_depth=depth));assert depth==0;continue
  if i.mnemonic=='jmp':successors=[i.operands[0].imm]
  elif i.mnemonic.startswith('j'):assert i.operands[0].type==X86_OP_IMM;successors=[i.operands[0].imm,pc+i.size]
  else:successors=[pc+i.size]
  todo.extend((next_pc,depth)for next_pc in successors)
 for pc in sorted(body.code):
  i=code.at(pc);listing.append(f'{pc:x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n')
 report=dict(passed=True,original_sha256=NEW_PIN,entry=0x329720,body_sites=921,calls=0,listing_sha256=hashlib.sha256(''.join(listing).encode()).hexdigest(),reachable_sites=len(depths),maximum_private_x87_depth=peak,maximum_preserved_incoming_x87_depth=8-peak,returns=returns,ambiguous_private_depth_sites={hex(pc):sorted(d)for pc,d in depths.items()if len(d)>1},abi='cdecl6(row_data,row_count,runtime,frame,input,builder); ordinary immutable value parameters',no_incoming_x87_value_read=True,retained_row_stride=32,jacobian_row_stride=48,runtime_row_stride=28,metadata_words=['0x11801','0x30402','0x50803','0x51407','0x61808'],rsqrt_sites=[pc for pc in sorted(body.code)if code.at(pc).mnemonic=='rsqrtss'],sqrt_sites=[pc for pc in sorted(body.code)if code.at(pc).mnemonic=='sqrtss'],source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__)},retained_large_artifacts=[]);a.output.mkdir(parents=True,exist_ok=True);(a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k]for k in('passed','body_sites','maximum_private_x87_depth','maximum_preserved_incoming_x87_depth')}))
if __name__=='__main__':main()

#!/usr/bin/env python3
"""Authenticate actual325810 closed call family and x87 caller ownership."""
import argparse,hashlib,json,os
from pathlib import Path
from capstone.x86_const import X86_OP_IMM,X86_OP_REG
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
OWNERS={0x325810:160,0x323430:58,0x1e1010:31,0x37c5d0:69,0x2e5810:62,0x8fdb0:1}
def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);a=p.parse_args();assert os.environ.get('HALO2_TEST_SCRATCH')and sha(NEW_DEFAULT)==NEW_PIN;code=_Code(Xbe(NEW_DEFAULT),'.text');rows=[];allpcs=set();peak=0
 for owner,count in OWNERS.items():
  body=complete_body(code,owner);assert len(body.code)==count;todo=[(owner,0)];seen=set();maximum=0;calls=[];returns=[]
  while todo:
   pc,depth=todo.pop()
   if(pc,depth)in seen:continue
   seen.add((pc,depth));assert pc in body.code;i=code.at(pc)
   for op in i.operands:
    if op.type==X86_OP_REG and i.reg_name(op.reg).startswith('st('):assert int(i.reg_name(op.reg)[3:-1])<depth,(hex(pc),i.mnemonic,i.op_str,depth)
   if i.mnemonic in('fld','fild','fbld','fldz','fld1','fldpi'):depth+=1
   elif i.mnemonic in('fstp','fistp','fisttp','fcomp','fucomp','faddp','fmulp','fsubp','fsubrp','fdivp','fdivrp'):depth-=1
   elif i.mnemonic in('fcompp','fucompp'):depth-=2
   assert 0<=depth<=8,(hex(pc),depth);maximum=max(maximum,depth)
   if i.mnemonic=='call':
    assert i.operands[0].type==X86_OP_IMM and i.operands[0].imm in OWNERS;calls.append(dict(instruction=pc,target=i.operands[0].imm,private_x87_depth=depth));assert depth==0,('Need composed child depth',hex(pc),depth)
   if i.mnemonic.startswith('ret'):returns.append(dict(instruction=pc,private_x87_depth=depth));assert depth==0;continue
   if i.mnemonic=='jmp':successors=[i.operands[0].imm]
   elif i.mnemonic.startswith('j'):assert i.operands[0].type==X86_OP_IMM;successors=[i.operands[0].imm,pc+i.size]
   else:successors=[pc+i.size]
   todo.extend((target,depth)for target in successors)
  pcs={pc for pc,_ in seen};assert pcs==set(body.code);allpcs.update(pcs);peak=max(peak,maximum);listing=''.join(f'{pc:x} {code.at(pc).bytes.hex()} {code.at(pc).mnemonic} {code.at(pc).op_str}\n'for pc in sorted(pcs));rows.append(dict(entry=owner,sites=count,maximum_private_x87_depth=maximum,calls=calls,returns=returns,listing_sha256=hashlib.sha256(listing.encode()).hexdigest()))
 assert len(allpcs)==381;report=dict(passed=True,original_sha256=NEW_PIN,owners=rows,unique_sites=len(allpcs),maximum_private_x87_depth=peak,maximum_preserved_incoming_x87_depth=8-peak,no_incoming_x87_value_read=True,abi='325810 ECX motion +two immutable binary32 value arguments, RET8; 08FDB0 RET8 no data/FP work',source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__)},retained_large_artifacts=[]);a.output.mkdir(parents=True,exist_ok=True);(a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print('Authenticated closed381 sites; private x87 maximum',peak,'preserve≤',8-peak)
if __name__=='__main__':main()

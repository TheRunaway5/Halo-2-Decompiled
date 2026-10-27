#!/usr/bin/env python3
"""Conservative incoming-XMM lane audit of the actual closed325810 family."""
import argparse,hashlib,json,os
from pathlib import Path
from capstone.x86_const import X86_OP_REG,X86_OP_MEM,X86_OP_IMM
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
from audit_native_cached_motion_tu15 import OWNERS
def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);a=p.parse_args();assert os.environ.get('HALO2_TEST_SCRATCH')and sha(NEW_DEFAULT)==NEW_PIN;code=_Code(Xbe(NEW_DEFAULT),'.text');bodies={owner:complete_body(code,owner)for owner in OWNERS};defined=[0]*8;incoming=[];unknown=[];instructions=[]
 def reg(op):
  if op.type==X86_OP_REG:
   name=code.at(current[0]).reg_name(op.reg)
   if name.startswith('xmm'):return int(name[3:])
  return None
 def need(op,mask):
  n=reg(op)
  if n is not None and mask&~defined[n]:incoming.append(dict(instruction=current[0],assembly=current[1],register=n,read_lane_mask=mask,undefined_lane_mask=mask&~defined[n]))
 def put(op,mask):
  n=reg(op)
  if n is not None:defined[n]|=mask
 current=[0,'']
 def walk(owner):
  pc=owner
  while True:
   assert pc in bodies[owner].code;i=code.at(pc);current[:]=[pc,i.mnemonic+' '+i.op_str];instructions.append(pc)
   if i.mnemonic.startswith('ret'):return
   if i.mnemonic=='call':assert i.operands[0].type==X86_OP_IMM and i.operands[0].imm in bodies;walk(i.operands[0].imm)
   elif any(reg(o)is not None for o in i.operands):
    m=i.mnemonic;ops=i.operands;dest=reg(ops[0]);source=reg(ops[1])if len(ops)>1 else None
    if m in('movaps','movups','movdqa','movdqu'):
     need(ops[1],15);put(ops[0],15)
    elif m=='movss':
     need(ops[1],1)
     if dest is not None:put(ops[0],15 if ops[1].type==X86_OP_MEM else 1)
    elif m=='movsd':
     need(ops[1],3)
     if dest is not None:put(ops[0],15 if ops[1].type==X86_OP_MEM else 3)
    elif m=='movd':need(ops[1],1);put(ops[0],15)
    elif m in('xorps','pxor')and dest==source:put(ops[0],15)
    elif m in('addps','subps','mulps','divps','minps','maxps','andps','andnps','orps','xorps'):
     need(ops[0],15);need(ops[1],15);put(ops[0],15)
    elif m in('addss','subss','mulss','divss','minss','maxss'):
     need(ops[0],1);need(ops[1],1);put(ops[0],1)
    elif m in('sqrtss','rsqrtss','rcpss'):need(ops[1],1);put(ops[0],1)
    elif m=='shufps':
     imm=ops[2].imm;need(ops[0],(1<<(imm&3))|(1<<((imm>>2)&3)));need(ops[1],(1<<((imm>>4)&3))|(1<<((imm>>6)&3)));put(ops[0],15)
    elif m in('unpcklps','unpckhps'):
     mask=3 if m=='unpcklps'else 12;need(ops[0],mask);need(ops[1],mask);put(ops[0],15)
    elif m in('movlps','movhps'):
     mask=3 if m=='movlps'else 12;need(ops[1],mask);put(ops[0],mask)
    elif m in('movhlps','movlhps'):
     need(ops[1],12 if m=='movhlps'else 3);put(ops[0],3 if m=='movhlps'else 12)
    elif m in('comiss','ucomiss'):need(ops[0],1);need(ops[1],1)
    elif m in('cvttss2si','cvtss2si'):need(ops[1],1)
    elif m=='cvtsi2ss':put(ops[0],1)
    else:unknown.append(dict(instruction=pc,assembly=current[1]))
   else:assert not i.mnemonic.startswith('j'),'Branch requires explicit dataflow merge'
   pc+=i.size
 walk(0x325810);report=dict(passed=not incoming and not unknown,original_sha256=NEW_PIN,entry=0x325810,executed_closed_instructions=len(instructions),unique_sites=len(set(instructions)),incoming_xmm_reads_before_definition=incoming,unqualified_sse_instructions=unknown,final_defined_lane_masks=defined,scope='Conservative source register-lane reads through actual closed calls; ordinary data loads define their lanes. Reports any incoming consumption even if later dead.',source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__), 'tools/audit_native_cached_motion_tu15.py':sha(ROOT/'tools/audit_native_cached_motion_tu15.py')},retained_large_artifacts=[]);a.output.mkdir(parents=True,exist_ok=True);(a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print('Incoming XMM reads',len(incoming),'unqualified opcodes',len(unknown));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())

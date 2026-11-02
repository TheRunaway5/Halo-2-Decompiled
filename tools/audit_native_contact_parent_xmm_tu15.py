#!/usr/bin/env python3
"""Conservative local XMM-lane liveness for original contact parent bodies.

Calls are recorded as explicit interprocedural boundaries. They neither
manufacture an XMM definition nor erase incoming taint in this local audit.
The separate physical entry-context audit covers retained nested real calls.
"""
import argparse,collections,hashlib,json,os
from pathlib import Path
from capstone.x86_const import X86_OP_REG,X86_OP_MEM,X86_OP_IMM
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body

def sha(path):return hashlib.file_digest(Path(path).open('rb'),'sha256').hexdigest()
def audit(code,owner):
 body=complete_body(code,owner);states={owner:(0,)*8};queue=collections.deque([owner]);incoming={};unknown={};calls={}
 while queue:
  pc=queue.popleft();i=code.at(pc);defined=list(states[pc]);ops=i.operands
  def reg(op):
   if op.type==X86_OP_REG:
    name=i.reg_name(op.reg)
    if name.startswith('xmm'):return int(name[3:])
   return None
  def need(op,mask):
   r=reg(op)
   if r is not None and mask&~defined[r]:incoming[(pc,r)]=dict(instruction=pc,assembly=i.mnemonic+' '+i.op_str,register=r,read_lane_mask=mask,undefined_lane_mask=mask&~defined[r])
  def put(op,mask):
   r=reg(op)
   if r is not None:defined[r]|=mask
  m=i.mnemonic;dest=reg(ops[0])if ops else None;source=reg(ops[1])if len(ops)>1 else None
  if m=='call':calls[pc]=dict(instruction=pc,target=ops[0].imm if ops[0].type==X86_OP_IMM else None,assembly=m+' '+i.op_str)
  if any(reg(op)is not None for op in ops):
   if m in('movaps','movups','movdqa','movdqu'):need(ops[1],15);put(ops[0],15)
   elif m=='movss':
    need(ops[1],1)
    if dest is not None:put(ops[0],15 if ops[1].type==X86_OP_MEM else 1)
   elif m=='movsd':
    need(ops[1],3)
    if dest is not None:put(ops[0],15 if ops[1].type==X86_OP_MEM else 3)
   elif m=='movd':need(ops[1],1);put(ops[0],15)
   elif m in('xorps','pxor')and dest==source:put(ops[0],15)
   elif m in('addps','subps','mulps','divps','minps','maxps','andps','andnps','orps','xorps'):need(ops[0],15);need(ops[1],15);put(ops[0],15)
   elif m in('addss','subss','mulss','divss','minss','maxss'):need(ops[0],1);need(ops[1],1);put(ops[0],1)
   elif m in('sqrtss','rsqrtss','rcpss'):need(ops[1],1);put(ops[0],1)
   elif m=='shufps':
    imm=ops[2].imm;need(ops[0],(1<<(imm&3))|(1<<((imm>>2)&3)));need(ops[1],(1<<((imm>>4)&3))|(1<<((imm>>6)&3)));put(ops[0],15)
   elif m in('unpcklps','unpckhps'):mask=3 if m=='unpcklps'else 12;need(ops[0],mask);need(ops[1],mask);put(ops[0],15)
   elif m in('movlps','movhps'):mask=3 if m=='movlps'else 12;need(ops[1],mask);put(ops[0],mask)
   elif m in('movhlps','movlhps'):need(ops[1],12 if m=='movhlps'else 3);put(ops[0],3 if m=='movhlps'else 12)
   elif m in('comiss','ucomiss'):need(ops[0],1);need(ops[1],1)
   elif m in('cmpps','cmpltps','cmpleps','cmpeqps','cmpneqps','cmpnltps','cmpnleps','cmpordps','cmpunordps'):need(ops[0],15);need(ops[1],15);put(ops[0],15)
   elif m=='movmskps':need(ops[1],15)
   elif m in('cvttss2si','cvtss2si'):need(ops[1],1)
   elif m=='cvtsi2ss':put(ops[0],1)
   else:unknown[pc]=dict(instruction=pc,assembly=m+' '+i.op_str)
  if m.startswith('ret'):successors=[]
  elif m=='jmp':assert ops[0].type==X86_OP_IMM;successors=[ops[0].imm]
  elif m.startswith('j'):assert ops[0].type==X86_OP_IMM;successors=[ops[0].imm,pc+i.size]
  else:successors=[pc+i.size]
  for target in successors:
   assert target in body.code,(hex(owner),hex(target));previous=states.get(target);current=tuple(defined)if previous is None else tuple(a&b for a,b in zip(previous,defined))
   if previous!=current:states[target]=current;queue.append(target)
 listing=''.join(f'{pc:x} {code.at(pc).bytes.hex()} {code.at(pc).mnemonic} {code.at(pc).op_str}\n'for pc in sorted(body.code))
 return dict(entry=owner,body_sites=len(body.code),reachable_sites=len(states),listing_sha256=hashlib.sha256(listing.encode()).hexdigest(),incoming_xmm_reads_before_local_definition=list(incoming.values()),unqualified_sse_instructions=list(unknown.values()),explicit_call_boundaries=list(calls.values()),local_xmm_liveness_passed=not incoming and not unknown)

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);a=p.parse_args();assert os.environ.get('HALO2_TEST_SCRATCH')and sha(NEW_DEFAULT)==NEW_PIN;code=_Code(Xbe(NEW_DEFAULT),'.text');owners=[audit(code,owner)for owner in(0x2f7a50,0x304350,0x2f36f0,0x2e4fe0)];assert [o['body_sites']for o in owners]==[40,534,575,256];report=dict(passed=all(o['local_xmm_liveness_passed']for o in owners),original_sha256=NEW_PIN,owners=owners,source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__)},scope='All reachable local source paths with conservative lane-mask intersection at control-flow merges; calls do not supply assumed XMM definitions',transitive_callee_liveness_proven=False,native_kernel_validated=False,retained_large_artifacts=[]);a.output.mkdir(parents=True,exist_ok=False);(a.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');print([(hex(o['entry']),len(o['incoming_xmm_reads_before_local_definition']),len(o['unqualified_sse_instructions']))for o in owners]);return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())

#!/usr/bin/env python3
"""Original TU1.5 reserve caller frontier audit, never a runtime parity claim.

Authenticate direct call sites with full CFG owners, then track reads of
callee-volatile GP/SIMD/arithmetic flags before overwrite. Unknown child ABI,
indirect branches, and caller return contracts remain explicit frontiers.
"""
import argparse,bisect,csv,hashlib,json,os,struct
from pathlib import Path
from capstone.x86_const import X86_OP_IMM,X86_OP_REG,X86_OP_MEM
import capstone.x86_const as xc
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
TARGET=0x2e31e0
ALIASES={}
for base,word,low,high in [('eax','ax','al','ah'),('ecx','cx','cl','ch'),('edx','dx','dl','dh')]:
 for name,mask in[(base,0xffffffff),(word,0xffff),(low,0xff),(high,0xff00)]:ALIASES[name]=(base,mask)
REGS=('eax','ecx','edx',*(f'xmm{n}'for n in range(8)),'flags')
FBITS={'CF':1,'PF':4,'AF':16,'ZF':64,'SF':128,'OF':2048}
def sha(p):
 with Path(p).open('rb')as f:return hashlib.file_digest(f,'sha256').hexdigest()
def accesses(i):
 # Architectural identity padding preserves live values; treating its named
 # operand as a read or a new definition would invent a consumer or kill taint.
 ops=i.operands
 same=len(ops)>=2 and ops[0].type==ops[1].type==X86_OP_REG and ops[0].reg==ops[1].reg
 if i.mnemonic=='nop' or(same and i.mnemonic in('mov','movaps','movups','movapd','movupd','movdqa','movdqu','movss','movsd','xchg')):return {},{}
 if i.mnemonic=='lea'and len(ops)==2 and ops[0].type==X86_OP_REG and ops[1].type==X86_OP_MEM:
  m=ops[1].mem
  if m.base==ops[0].reg and not m.index and m.disp==0:return {},{}
 reads,writes=i.regs_access();r={};w={}
 def add(out,name,mask):out[name]=out.get(name,0)|mask
 for regs,out in((reads,r),(writes,w)):
  for reg in regs:
   name=i.reg_name(reg)
   if name in ALIASES:add(out,*ALIASES[name])
   elif name.startswith('xmm'):add(out,name,15)
 for flag,mask in FBITS.items():
  if i.eflags&getattr(xc,'X86_EFLAGS_TEST_'+flag,0):add(r,'flags',mask)
  if any(i.eflags&getattr(xc,'X86_EFLAGS_'+kind+'_'+flag,0)for kind in('MODIFY','RESET','SET','UNDEFINED')):add(w,'flags',mask)
 ops=i.operands
 same=len(ops)>=2 and ops[0].type==ops[1].type==X86_OP_REG and ops[0].reg==ops[1].reg
 if same and i.mnemonic in('xor','sub','cmp','pxor','xorps','xorpd'):
  name=i.reg_name(ops[0].reg);r.pop(ALIASES[name][0]if name in ALIASES else name,None)
 if i.mnemonic in('movss','addss','subss','mulss','divss','minss','maxss','sqrtss','rsqrtss','comiss','ucomiss','cvttss2si','cvtss2si'):
  for name in list(r):
   if name.startswith('xmm'):r[name]=1
  for name in list(w):
   if name.startswith('xmm'):w[name]=15 if i.mnemonic=='movss'and len(ops)>1 and ops[1].type==X86_OP_MEM else 1
 if i.mnemonic=='shufps'and len(ops)==3:
  dst,src=(i.reg_name(ops[n].reg)for n in(0,1));v=ops[2].imm;r.pop(dst,None);r.pop(src,None);add(r,dst,(1<<(v&3))|(1<<((v>>2)&3)));add(r,src,(1<<((v>>4)&3))|(1<<((v>>6)&3)))
 # A conditionally written destination is not a dominating overwrite.
 # Unknown/zero masked shift counts likewise preserve original flags on one
 # path; retain their taint rather than silently proving those bits dead.
 if i.mnemonic.startswith('cmov')and ops and ops[0].type==X86_OP_REG:
  name=i.reg_name(ops[0].reg);w.pop(ALIASES[name][0]if name in ALIASES else name,None)
 if i.mnemonic in('shl','sal','shr','sar','rol','ror','rcl','rcr','shld','shrd')and ops:
  count=ops[-1]
  if count.type!=X86_OP_IMM or(count.imm&31)==0:w.pop('flags',None)
 if i.mnemonic.startswith(('rep ','repe ','repne ')):
  for name in list(w):
   if name in ALIASES or name in('flags',):w.pop(name,None)
 return r,w

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();assert 'HALO2_TEST_SCRATCH'in os.environ;assert sha(NEW_DEFAULT)==NEW_PIN;a.output.mkdir(parents=True,exist_ok=True)
 image=Xbe(NEW_DEFAULT);code=_Code(image,'.text');section=image.section('.text');raw=image.section_bytes(section);registry=ROOT/'analysis/multiplayer/title-update-1.5/decompiled/functions.tsv';starts=sorted(int(r['address'],16)for r in csv.DictReader(registry.open(),delimiter='\t'));bodies={};errors={}
 def body(entry):
  if entry not in bodies:
   try:bodies[entry]={pc:code.at(pc)for pc in complete_body(code,entry).code}
   except Exception as e:errors[hex(entry)]=repr(e);bodies[entry]={}
  return bodies[entry]
 def owner(pc):
  end=bisect.bisect_right(starts,pc)
  for entry in reversed(starts[max(0,end-16):end]):
   b=body(entry)
   if pc in b:return entry,b
  return None,{}
 calls=[];tails=[];unknown=[]
 for opcode,dest in((0xe8,calls),(0xe9,tails)):
  index=raw.find(bytes([opcode]))
  while 0<=index<len(raw)-4:
   pc=section.va+index
   if(pc+5+struct.unpack_from('<i',raw,index+1)[0])&0xffffffff==TARGET:
    entry,b=owner(pc);i=b.get(pc)
    if i is None or i.mnemonic!=('call'if opcode==0xe8 else'jmp')or i.operands[0].type!=X86_OP_IMM or i.operands[0].imm!=TARGET:unknown.append(dict(raw_pc=hex(pc),opcode=hex(opcode),reason='Full original CFG did not authenticate this raw rel32 candidate'))
    else:dest.append(dict(pc=pc,owner=entry,bytes=i.bytes.hex()))
   index=raw.find(bytes([opcode]),index+1)
 initial=tuple(0xffffffff if n in('eax','ecx','edx')else sum(FBITS.values())if n=='flags'else 15 for n in REGS);reports=[]
 for call in calls:
  ins=body(call['owner']);pending=[(call['pc']+5,initial)];seen=set();consumers={};frontiers=[]
  while pending:
   pc,state=pending.pop()
   if(pc,state)in seen:continue
   seen.add((pc,state));i=ins.get(pc)
   if i is None:frontiers.append(dict(pc=hex(pc),kind='outside_authenticated_owner'));continue
   read,write=accesses(i);used={n:read.get(n,0)&mask for n,mask in zip(REGS,state)if read.get(n,0)&mask}
   if used:consumers[pc]=dict(pc=hex(pc),instruction=i.mnemonic+' '+i.op_str,consumed_masks={n:hex(v)for n,v in used.items()})
   after=tuple(mask&~write.get(n,0)for n,mask in zip(REGS,state));remaining={n:hex(v)for n,v in zip(REGS,after)if v}
   if i.mnemonic=='call':frontiers.append(dict(pc=hex(pc),kind='downstream_callee_input_ABI_unknown',instruction=i.mnemonic+' '+i.op_str,remaining_masks=remaining));continue
   if i.mnemonic.startswith('ret'):frontiers.append(dict(pc=hex(pc),kind='caller_return_contract_not_proven',remaining_masks=remaining));continue
   following=pc+i.size
   if i.mnemonic=='jmp':
    if i.operands[0].type==X86_OP_IMM:targets=[i.operands[0].imm]
    else:frontiers.append(dict(pc=hex(pc),kind='indirect_jump_frontier',instruction=i.mnemonic+' '+i.op_str,remaining_masks=remaining));continue
   elif i.mnemonic.startswith('j')or i.mnemonic.startswith('loop'):
    if i.operands[0].type!=X86_OP_IMM:frontiers.append(dict(pc=hex(pc),kind='unresolved_conditional'));continue
    targets=[following,i.operands[0].imm]
   else:targets=[following]
   pending.extend((t,after)for t in targets)
  reports.append(dict(call_pc=hex(call['pc']),full_cfg_owner=hex(call['owner']),call_bytes=call['bytes'],explicit_consumers_before_overwrite=list(consumers.values()),frontiers=frontiers,postreturn_states_visited=len(seen),all_volatile_liveness_proven=False))
 absolute=[]
 for sec in image.sections:
  try:data=image.section_bytes(sec)
  except Exception:continue
  offset=data.find(struct.pack('<I',TARGET))
  while offset>=0:absolute.append(dict(address=hex(sec.va+offset),section=sec.name,role='Absolute target reference; not automatically an indirect callable owner'));offset=data.find(struct.pack('<I',TARGET),offset+1)
 result=dict(completed=True,runtime_parity_proven=False,native_helper_seam_activated=False,image_sha256=NEW_PIN,registry_sha256=sha(registry),source_sha256=sha(__file__),target=hex(TARGET),direct_call_count=len(calls),direct_callers=reports,tail_calls=[{k:hex(v)if isinstance(v,int)else v for k,v in row.items()}for row in tails],unauthenticated_rel32_candidates=unknown,absolute_target_references=absolute,owner_decode_failures=errors,all_direct_or_indirect_caller_liveness_proven=False,method='FullCFG originalowner authentication; perbitGP/perlaneSSE/arithmeticflags read-before-overwrite walk. Conditionaldestinations/variable-or-zero-shiftflags donotkilltaint. All downstream call inputABI, caller returncontracts andindirectfrontiers remainopen. This audit doesnot invent or provide volatilepostimages.',limits=['No inference that an unknown child callee ignores remainingvolatiles','Absolute refs are not proof of a complete indirect-caller inventory','Explicit consumption can still depend on proven-preserved incoming values; requires actual callee clobber analysis','No final native shim activation from this frontier report alone'],owned_scratch=os.environ['HALO2_TEST_SCRATCH'],retained_large_artifacts=[])
 (a.output/'report.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(completed=True,direct_call_count=len(calls),tail_call_count=len(tails),explicit_consumer_callers=sum(bool(r['explicit_consumers_before_overwrite'])for r in reports),unknown_rel32=len(unknown),absolute_refs=len(absolute))))
if __name__=='__main__':main()

#!/usr/bin/env python3
"""Offline all-path incoming GP/XMM liveness of convex-query service owners."""
import argparse,collections,hashlib,inspect,json,os,struct
from pathlib import Path
import audit_native_contact_parent_xmm_tu15 as xmm
from capstone.x86_const import X86_OP_IMM,X86_OP_REG
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
OWNERS=(0x2f8630,0x30e930,0x30d350,0x2e4760)
SAVES={0x2f8630,0x2f863c,0x2f8642,0x2f8643,0x30e930,0x30e939,0x30e941,0x30e94a,0x30d350,0x30d35f,0x2e4760}
ALIASES={}
for n,(full,word,lo,hi)in enumerate((('eax','ax','al','ah'),('ecx','cx','cl','ch'),('edx','dx','dl','dh'),('ebx','bx','bl','bh'),('esp','sp',None,None),('ebp','bp',None,None),('esi','si',None,None),('edi','di',None,None))):
 for name,mask in((full,15),(word,3),(lo,1),(hi,2)):
  if name:ALIASES[name]=(n,mask)
def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def successors(image,code,pc):
 i=code.at(pc);m=i.mnemonic
 if m.startswith('ret'):return[]
 if m=='jmp':
  if i.operands[0].type==X86_OP_IMM:return[i.operands[0].imm]
  assert pc==0x30ea37
  return list(struct.unpack('<11I',image.read(0x30f6e0,44)))
 if m.startswith('j'):return[i.operands[0].imm,pc+i.size]
 return[pc+i.size]
def gp_audit(image,code,owner):
 body=complete_body(code,owner);states={owner:(0,15,0,0,15,0,0,0)};queue=collections.deque([owner]);incoming={};saved=[]
 while queue:
  pc=queue.popleft();i=code.at(pc);defined=list(states[pc]);reads,writes=i.regs_access()
  save=pc in SAVES
  if save:assert i.mnemonic=='push'and i.operands[0].type==X86_OP_REG
  for r in reads:
   name=i.reg_name(r)
   if name not in ALIASES:continue
   n,mask=ALIASES[name]
   if mask&~defined[n]:
    if save and name not in('esp','eip'):saved.append(dict(instruction=pc,register=name,control_only_prologue_save=True));continue
    incoming[(pc,name)]=dict(instruction=pc,assembly=i.mnemonic+' '+i.op_str,register=name,undefined_byte_mask=mask&~defined[n])
  for r in writes:
   name=i.reg_name(r)
   if name in ALIASES:
    n,mask=ALIASES[name];defined[n]|=mask
  for target in successors(image,code,pc):
   assert target in body.code,(hex(owner),hex(pc),hex(target));previous=states.get(target);current=tuple(defined)if previous is None else tuple(a&b for a,b in zip(previous,defined))
   if previous!=current:states[target]=current;queue.append(target)
 return dict(entry=owner,body_sites=len(body.code),allowed_incoming_semantic_GP=['ECX current typed receiver','ESP original test ABI arguments only'],incoming_GP_before_local_definition=list(incoming.values()),excluded_prologue_control_saves=list({(x['instruction'],x['register']):x for x in saved}.values()),passed=not incoming)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);a=p.parse_args();assert os.environ.get('HALO2_TEST_SCRATCH')and sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
 source=inspect.getsource(xmm.audit);old="elif m=='jmp':assert ops[0].type==X86_OP_IMM;successors=[ops[0].imm]";new="elif m=='jmp':\n   if ops[0].type==X86_OP_IMM:successors=[ops[0].imm]\n   else:\n    assert pc==0x30ea37\n    successors=list(struct.unpack('<11I',image.read(0x30f6e0,44)))";assert source.count(old)==1;source=source.replace(old,new);xmm.__dict__.update(struct=struct,image=image);exec(source,xmm.__dict__);lanes=[xmm.audit(code,o)for o in OWNERS];gp=[gp_audit(image,code,o)for o in OWNERS]
 report=dict(passed=all(o['local_xmm_liveness_passed']for o in lanes)and all(o['passed']for o in gp),original_sha256=NEW_PIN,owners_XMM=lanes,owners_GP=gp,immutable_switch_tables_sha256=hashlib.sha256(image.read(0x30f6e0,69)).hexdigest(),source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__),str(Path(xmm.__file__).relative_to(ROOT)):sha(xmm.__file__)},XMM_audit_function_sha256=hashlib.sha256(source.encode()).hexdigest(),scope='All local authenticated CFG paths. Calls remain explicit boundaries; no callee GP/XMM postimage or future-use closure is inferred.',transitive_callee_liveness_proven=False,native_kernel_validated=False,retained_large_artifacts=[]);a.output.mkdir(parents=True,exist_ok=False);(a.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');print([(hex(o['entry']),len(o['incoming_GP_before_local_definition']))for o in gp]);return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())

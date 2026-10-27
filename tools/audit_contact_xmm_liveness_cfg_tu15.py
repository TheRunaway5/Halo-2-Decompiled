#!/usr/bin/env python3
"""Original TU1.5 local CFG frontiers for incoming contact-copy XMM3.

An unexamined call is an explicit frontier, never an assumed register kill.
This is source diagnosis, not a physics or whole-frame acceptance replacement.
"""
import argparse,collections,hashlib,json,os,struct
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body

def sha(p):
 with Path(p).open('rb')as f:return hashlib.file_digest(f,'sha256').hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);code=_Code(image,'.text');entries=[0x303800,0x2e47b0,0x30cd00,0x30cf40,0x303d00,0x304130,0x304350,0x304ac0,0x303ba0,0x303ee0,0x321a70,0x2e44d0,0x325170];bodies={e:{pc:code.at(pc)for pc in complete_body(code,e).code}for e in entries}
 rows=[]
 for entry,start in[(e,e)for e in entries]+[(0x304130,0x3042ca),(0x304350,0x30482f),(0x304ac0,0x304ae5),(0x321a70,0x321b61)]:
  body=bodies[entry];queue=collections.deque([(start,[])]);seen=set();frontiers=[];edges=[]
  while queue:
   pc,path=queue.popleft()
   if pc in seen:continue
   seen.add(pc);i=body[pc];path=path+[pc];reads,writes=i.regs_access();rr={i.reg_name(r)for r in reads};wr={i.reg_name(r)for r in writes};kind=None
   if i.mnemonic=='call':kind='unexamined_call'
   elif i.mnemonic.startswith('ret'):kind='return_with_prior_register_value'
   elif 'xmm3'in rr:kind='incoming_register_read'
   elif 'xmm3'in wr:
    assert i.mnemonic=='movaps'and i.operands[0].type==1 and i.reg_name(i.operands[0].reg)=='xmm3';kind='full_prior_register_definition'
   if kind:
    f=dict(kind=kind,pc=hex(pc),bytes=i.bytes.hex(),mnemonic=i.mnemonic,operands=i.op_str,path=[hex(q)for q in path]);frontiers.append(f);continue
   if i.mnemonic=='jmp':successors=[i.operands[0].imm]if i.operands[0].type==2 else[]
   elif i.mnemonic.startswith('j'):assert i.operands[0].type==2;successors=[pc+i.size,i.operands[0].imm]
   else:successors=[pc+i.size]
   for nxt in successors:
    if nxt not in body:frontiers.append(dict(kind='unexamined_external_edge',pc=hex(pc),target=hex(nxt),path=[hex(q)for q in path]));continue
    edges.append((pc,nxt));queue.append((nxt,path))
  rows.append(dict(entry=hex(entry),start=hex(start),body_sites=len(body),visited_before_frontier=len(seen),frontiers=frontiers,edge_digest=hashlib.sha256(json.dumps(sorted(edges)).encode()).hexdigest()))
 sec=image.section('.text');raw=image.section_bytes(sec);calls={}
 for target in(0x304130,0x304350,0x304ac0):
  hits=[];index=raw.find(b'\xe8')
  while index>=0 and index+5<=len(raw):
   pc=sec.va+index
   if(pc+5+struct.unpack_from('<i',raw,index+1)[0])&0xffffffff==target:
    owners=[e for e,body in bodies.items()if pc in body and body[pc].mnemonic=='call'and body[pc].operands[0].type==2 and body[pc].operands[0].imm==target];hits.append(dict(pc=hex(pc),authenticated_full_cfg_owners=[hex(e)for e in owners]));assert owners,'A direct-call byte candidate lacks an authenticated caller CFG'
   index=raw.find(b'\xe8',index+1)
  calls[hex(target)]=hits
 listings={hex(e):[dict(pc=hex(pc),bytes=i.bytes.hex(),mnemonic=i.mnemonic,operands=i.op_str)for pc,i in sorted(body.items())]for e,body in bodies.items()};(a.output/'listings.json').write_text(json.dumps(dict(original_sha256=NEW_PIN,bodies=listings),indent=2)+'\n')
 report=dict(completed=True,original_sha256=NEW_PIN,source_sha256={str(Path(__file__).relative_to(ROOT)):sha(__file__)},listings_sha256=sha(a.output/'listings.json'),local_cfg_audits=rows,direct_call_sites=calls,indirect_callers_authenticated=False,all_callee_register_abis_proven=False,owned_scratch=os.environ['HALO2_TEST_SCRATCH'],retained_large_artifacts=[],scope='Exact source CFG first read/full definition/call/return frontiers. Calls and indirect callers remain explicit; no assumption that a call clobbers XMM3 or that a returned volatile register is a physics result. Raw gates unchanged.')
 (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(completed=True,audits=len(rows),source_entries=len(entries))))
if __name__=='__main__':main()

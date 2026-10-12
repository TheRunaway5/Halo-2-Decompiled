#!/usr/bin/env python3
"""Authenticate the complete early physics initializer and its arena client."""
import argparse,csv,hashlib,json
from pathlib import Path
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code,_trace,PIN
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN,norm,full_tokens
ENTRIES=(0x1c5f00,0x1f15b0)
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args()
 oldpath=ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'
 assert sha(oldpath)==PIN and sha(NEW_DEFAULT)==NEW_PIN
 old,new=Xbe(oldpath),Xbe(NEW_DEFAULT);oc,nc=_Code(old,'.text'),_Code(new,'.text')
 os={int(r['address'],16)for r in csv.DictReader((ROOT/'analysis/map-pack-update/functions.tsv').open(),delimiter='\t')}|set(ENTRIES)
 md=Cs(CS_ARCH_X86,CS_MODE_32);md.detail=True;md.skipdata=True;sec=new.section('.text')
 linear=list(md.disasm(new.section_bytes(sec),sec.va));tokens=[norm(i,new)if i.id else ''for i in linear]
 ns={i.operands[0].imm for i in linear if i.id and i.mnemonic=='call' and i.operands[0].type==2 and nc.inside(i.operands[0].imm)}
 rows=[];entries={};imap={};amap={};a.output.mkdir(parents=True,exist_ok=False)
 for p in ENTRIES:
  of=_trace(oc,p,os);oi=[oc.at(t)for t in sorted(of.code)];prefix=[norm(i,old)for i in oi[:6]];matches=[]
  for n in range(len(tokens)-len(prefix)):
   if tokens[n:n+len(prefix)]!=prefix:continue
   q=linear[n].address
   # An actual paired caller/pointer operand constrains small ambiguous bodies.
   if p in amap and amap[p]!=q:continue
   nf=_trace(nc,q,ns|{q});ni=[nc.at(t)for t in sorted(nf.code)]
   if full_tokens(oi,old)==full_tokens(ni,new):matches.append((q,nf,ni))
  assert len(matches)==1,(hex(p),[hex(r[0])for r in matches]);q,nf,ni=matches[0];ns.add(q)
  assert not of.tables and not nf.tables;entries[f'{p:08x}']=q
  instructions=[]
  for x,y in zip(oi,ni):
   imap[f'{x.address:08x}']=y.address
   for u,v in zip(x.operands,y.operands):
    assert u.type==v.type and u.size==v.size;p0=p1=None
    if u.type==2:p0,p1=u.imm&0xffffffff,v.imm&0xffffffff
    if u.type==3:p0,p1=u.mem.disp&0xffffffff,v.mem.disp&0xffffffff
    if p0 is not None and p0!=p1:
     assert p0 not in amap or amap[p0]==p1;amap[p0]=p1
   instructions.append(dict(old=x.address,later=y.address,old_bytes=x.bytes.hex(),later_bytes=y.bytes.hex()))
  for label,ins in [('disc',oi),('later',ni)]:
   (a.output/f'{p:08x}-{label}.asm').write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins))
  rows.append(dict(old=p,later=q,instructions=instructions,tables=[]))
 for p,q in entries.items():
  p=int(p,16);assert p not in amap or amap[p]==q;amap[p]=q
 imports={}
 for p in ():
  q=amap[p];imports[f'{p:08x}']=dict(later=q,hex=new.read(q,4).hex())
 report=dict(schema=1,tool_sha256=sha(__file__),original_sha256=PIN,later_original_sha256=NEW_PIN,
  complete_bodies=len(ENTRIES),entries=entries,instruction_map=imap,address_map={f'{p:08x}':q for p,q in amap.items()},routines=rows,
  imports=imports,captured_storage=False,live_world_startup=False)
 p=a.output/'complete-report.json';p.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n');print(json.dumps(dict(sha256=sha(p),entries=entries,instructions=len(imap))))
if __name__=='__main__':main()

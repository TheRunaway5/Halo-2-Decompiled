#!/usr/bin/env python3
"""Read-only relocated instruction candidate audit for the owned later update.

Masked addresses locate candidates only. Full branch topology and every paired
image-memory operand are retained separately, without a gameplay-equivalence
claim or treating annotation names/comments as instruction authority.
"""
import argparse,collections,csv,hashlib,json,re
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code,_trace,PIN
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from capstone.x86 import X86_OP_MEM,X86_OP_IMM

NEW_PIN='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
NEW_DEFAULT=Path('/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe')
ENTRIES=(0xbffe0,0xd6660,0xb8820,0xb8a30,0x11d1c0,0x11d2b0,0x11d330,0x1d5830,0x1d5a60,0x1d5e90,0x1d5c70,0x1d6f40,0x1d56f0,0x1c7050,0x1d98b0,0xd7160,0xdc0a0,0xd73e0,0xdb710,0xdb470,0xdb360,0xd8280,0xd9180,0xd7c60,0x1243c0,0x1ee380,0x1ee8f0,0x16af90,0x1f3500,0x1f3900,0xda0e0,0x16a5c0,0x16a560,0xbfb90,0x18a9c0)
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def containing(image,p):return next((s for s in image.sections if s.va<=p<s.va+s.vsize),None)
def norm(i,image):
 op=i.op_str
 if(i.mnemonic.startswith('j')or i.mnemonic=='call')and op.startswith('0x'):op='DIRECT'
 else:op=re.sub(r'0x[0-9a-f]+',lambda m:'IMAGE_ADDRESS'if containing(image,int(m.group(),16))else m.group(),op)
 return i.mnemonic+' '+op
def full_tokens(ins,image):
 positions={i.address:n for n,i in enumerate(ins)};r=[]
 for i in ins:
  token=norm(i,image)
  if i.mnemonic.startswith('j')and len(i.operands)==1 and i.operands[0].type==X86_OP_IMM:
   target=i.operands[0].imm;token+=' INDEX='+str(positions[target])if target in positions else' EXTERNAL'
  r.append(token)
 return r
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--new',type=Path,default=NEW_DEFAULT);ap.add_argument('--old',type=Path,default=ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');ap.add_argument('--output',type=Path,default=ROOT/'analysis/multiplayer/sandbox-source/later-update-functions/report.json');args=ap.parse_args()
 oldp=args.old;assert sha(oldp)==PIN;assert sha(args.new)==NEW_PIN
 old=Xbe(oldp);new=Xbe(args.new);oc=_Code(old,'.text');nc=_Code(new,'.text')
 starts={int(r['address'],16)for r in csv.DictReader((ROOT/'analysis/map-pack-update/functions.tsv').open(),delimiter='\t')}
 md=Cs(CS_ARCH_X86,CS_MODE_32);md.skipdata=True;sec=new.section('.text');linear=list(md.disasm(new.section_bytes(sec),sec.va));tokens=[norm(i,new)for i in linear]
 wanted={};oldfuncs={};oldtraces={}
 for a in ENTRIES:
  f=_trace(oc,a,starts|set(ENTRIES));ins=[oc.at(p)for p in sorted(f.code)];oldfuncs[a]=ins;oldtraces[a]=f
  for n in(16,12,8):
   if len(ins)>=n:wanted.setdefault(n,set()).add(tuple(norm(i,old)for i in ins[:n]))
 indices={n:collections.defaultdict(list)for n in wanted}
 for n,keys in wanted.items():
  for i in range(len(tokens)-n):
   key=tuple(tokens[i:i+n])
   if key in keys:indices[n][key].append(linear[i].address)
 mapped={};rows=[]
 for a,ins in oldfuncs.items():
  candidates=[];prefix=0
  for n in(16,12,8):
   if len(ins)<n:continue
   candidates=indices[n].get(tuple(norm(i,old)for i in ins[:n]),[]);prefix=n
   if len(candidates)==1:break
  row={'disc_1_1_entry':f'{a:08x}','prefix_instructions':prefix,'candidates':[f'{x:08x}'for x in candidates]};rows.append(row)
  if len(candidates)==1:mapped[a]=candidates[0]
 newstarts=set(mapped.values())
 # Actual direct-call targets, rather than unverified annotation boundaries.
 detail=Cs(CS_ARCH_X86,CS_MODE_32);detail.detail=True;detail.skipdata=True
 for i in detail.disasm(new.section_bytes(sec),sec.va):
  if i.mnemonic=='call'and len(i.operands)==1 and i.operands[0].type==X86_OP_IMM and nc.inside(i.operands[0].imm):newstarts.add(i.operands[0].imm)
 args.output.parent.mkdir(parents=True,exist_ok=True);insdir=args.output.parent/'instructions';insdir.mkdir(exist_ok=True)
 for row in rows:
  a=int(row['disc_1_1_entry'],16)
  if a not in mapped:continue
  b=mapped[a];oi=oldfuncs[a];f=_trace(nc,b,newstarts);ni=[nc.at(p)for p in sorted(f.code)]
  same=full_tokens(oi,old)==full_tokens(ni,new);row.update(later_entry=f'{b:08x}',old_instructions=len(oi),later_instructions=len(ni),normalized_instruction_and_internal_branch_topology_equal=same)
  for label,ins,image in(('disc',oi,old),('later',ni,new)):
   p=insdir/f'{a:08x}-{label}-{a if label=="disc"else b:08x}.asm';p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins));row[label+'_listing']={'file':p.name,'sha256':sha(p)}
  if same:
   tables=[];of=oldtraces[a];oldpositions={i.address:n for n,i in enumerate(oi)};newpositions={i.address:n for n,i in enumerate(ni)}
   assert len(of.tables)==len(f.tables),(a,of.tables,f.tables)
   for left,right in zip(of.tables,f.tables):
    op,width,n=left;np,nwidth,nn=right;assert(width,n)==(nwidth,nn)
    ob=old.read(op,width*n);nb=new.read(np,width*n)
    if width==4:
     import struct
     ov=struct.unpack('<'+'I'*n,ob);nv=struct.unpack('<'+'I'*n,nb)
     ot=[oldpositions.get(v,'ZERO'if v==0 else'EXTERNAL')for v in ov];nt=[newpositions.get(v,'ZERO'if v==0 else'EXTERNAL')for v in nv]
     equal=ot==nt
    else:ot=nt=None;equal=ob==nb
    tables.append({'old_address':f'{op:08x}','later_address':f'{np:08x}','width':width,'entries':n,'old_hex':ob.hex(),'later_hex':nb.hex(),'old_target_instruction_indices':ot,'later_target_instruction_indices':nt,'mapped_table_equal':equal})
   row['switch_tables']=tables;row['switch_table_topology_equal']=all(r['mapped_table_equal']for r in tables)
   refs=[];calls=[]
   for x,y in zip(oi,ni):
    for xo,yo in zip(x.operands,y.operands):
     if xo.type==yo.type==X86_OP_MEM and not xo.mem.base and not yo.mem.base and containing(old,xo.mem.disp)and containing(new,yo.mem.disp):
      width=xo.size;assert width==yo.size;before=old.read(xo.mem.disp,width);after=new.read(yo.mem.disp,width)
      refs.append({'old_site':f'{x.address:08x}','later_site':f'{y.address:08x}','old_address':f'{xo.mem.disp:08x}','later_address':f'{yo.mem.disp:08x}','width':width,'old_hex':before.hex(),'later_hex':after.hex(),'bytes_equal':before==after,'old_section':containing(old,xo.mem.disp).name,'later_section':containing(new,yo.mem.disp).name})
    if x.mnemonic=='call'and x.operands[0].type==X86_OP_IMM and y.operands[0].type==X86_OP_IMM:
     target=x.operands[0].imm;actual=y.operands[0].imm;calls.append({'old_site':f'{x.address:08x}','later_site':f'{y.address:08x}','old_target':f'{target:08x}','later_target':f'{actual:08x}','mapped_target_matches':mapped.get(target)==actual if target in mapped else None})
   row['paired_image_memory_operands']=refs;row['paired_direct_calls']=calls
   row['all_paired_memory_bytes_equal']=all(r['bytes_equal']for r in refs)
 report={'tool_sha256':sha(__file__),'disc_1_1_sha256':PIN,'later_owned_xbe_sha256':NEW_PIN,'later_path':str(args.new),'scope':'Candidate correspondence from genuine owned image bytes, normalized instruction shape/internal branch topology and separately retained absolute memory/direct call evidence. No gameplay equivalence, latest-ever version, native integration or original source execution claim. No annotation scripts, names/comments or assumed addresses used.','routines':rows,'native_gameplay_update_applied':False}
 args.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({'mapped':len(mapped),'total':len(ENTRIES),'normalized_equal':sum(r.get('normalized_instruction_and_internal_branch_topology_equal',False)for r in rows),'output':str(args.output)}))
if __name__=='__main__':main()

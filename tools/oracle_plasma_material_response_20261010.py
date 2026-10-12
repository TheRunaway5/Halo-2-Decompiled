#!/usr/bin/env python3
"""Unchanged complete material lookup + bounded projectile response choice.

Actual Lockout matg/proj banks. The response segment has declared angle/speed,
collision-kind/header-type/RNG controls; it is not a world impact producer.
"""
import argparse,csv,hashlib,itertools,json,struct,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path[:0]=[str(ROOT/'tools'),str(ROOT/'tools/reference/halo2-decompiled/tools')]
from oracle_grenade_scene_state import IMAGES,digest
from xbe import Xbe
from inspect_multiplayer import Cache
from functions import _Code,_trace
from research_mp_original_data_u5 import norm
from research_mp_live_locomotion_owners_u5 import raw_prefix_candidates
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
from capstone.x86 import X86_OP_MEM,X86_OP_IMM

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();assert not a.output.exists();a.output.mkdir(parents=True)
 images=[Xbe(path) for path,_,_ in IMAGES];codes=[_Code(x,'.text') for x in images]
 starts=[{int(r['address'],16)for r in csv.DictReader(path.open(),delimiter='\t')}for _,_,path in IMAGES]
 pairs={};report=dict(schema='plasma-material-response-20261010-v1',tool_sha256=digest(Path(__file__)),images=[],pairs=[],boundaries=[
 'Complete1015A0 lookup executes unchanged original bytes with actual Lockout metadata; explicit global/pool/tag ABI locators are direct-kernel inputs, not source constructors.',
 'FEB40 FF090..FF1D9 segment starts after angle/speed/perturbation production. Declared angle/speed/collision/header type/RNG controls are consumer inputs only; no impact/contact/attachment/effect/physics output is supplied.',
 'Complete101640 is instruction-paired only. Its lower transform/attachment/physics/AI owners are not executed or returned successfully.',
 'Masked x87 PC53/MXCSR context. Result/store/RNG comparison only; no Xbox hardware exception/sticky-state claim.'])
 for entry in (0x1015a0,0xfeb40,0x101640):
  old=[codes[0].at(x) for x in sorted(_trace(codes[0],entry,starts[0]).code)];pos={i.address:n for n,i in enumerate(old)};tokens=[norm(i,images[0],pos)for i in old];matches=[]
  for target in raw_prefix_candidates(codes[0],images[0],codes[1],entry,12):
   try:
    new=[codes[1].at(x)for x in sorted(_trace(codes[1],target,starts[1]).code)];np={i.address:n for n,i in enumerate(new)}
    if [norm(i,images[1],np)for i in new]==tokens:matches.append((target,new))
   except Exception:pass
  assert len(matches)==1,(hex(entry),[hex(x)for x,_ in matches]);target,new=matches[0];pairs[entry]=(old,new)
  report['pairs'].append(dict(old=f'{entry:08x}',u5=f'{target:08x}',instructions=len(old),mapping=[dict(old=f'{x.address:08x}',u5=f'{y.address:08x}')for x,y in zip(old,new)]))
  for label,ins in [('tu11',old),('u5',new)]:
   (a.output/f'{entry:08x}-{label}.asm').write_text(''.join(f'{i.address:08x} {bytes(i.bytes).hex()} {i.mnemonic} {i.op_str}\n'for i in ins))
 cache=Cache(ROOT/'assets/disc/maps/lockout.map');matg=next(t for t in cache.tags.values()if t['group']=='matg');ga=matg['offset']+cache.magic
 parents=[cache.h(x+8)for x in cache.block(matg['offset']+0x150,0xb4)]
 defs=[cache.tags[cache.u(x+0x28)]for x in cache.block(matg['offset']+0x100,0x2c)]
 banks=[b''.join(bytes(cache.data[x:x+0x58])for x in cache.block(t['offset']+0x19c,0x58))for t in defs]
 report['cache']=dict(path=str(cache.path),sha256=digest(cache.path),magic=f'{cache.magic:08x}',globals=f'{ga:08x}',parents=parents,
  definitions=[dict(datum=t["datum"],address=f'{t["offset"]+cache.magic:08x}',name=t['name'],rows_hex=b.hex())for t,b in zip(defs,banks)])
 lookup_cases=[(di,m)for di in range(2)for m in [-32768,-2,-1,*range(len(parents)+2),32767]]
 choice_cases=[]
 for di,bank in enumerate(banks):
  for ri in range(len(bank)//0x58):
   row=bank[ri*0x58:(ri+1)*0x58]
   for ctype,otype,angle,speed in itertools.product((1,4),(0,1,2,9,31,32),(0.,1.,3.),(0.,1.,10.)):
    choice_cases.append((di,ri,row,ctype,otype,angle,speed,1,'actual authored row / varied consumer inputs'))
 # Explicit branch controls extend actual plasma row; they do not mutate Cache.
 for chance,flags,limits,angle,speed,seed in itertools.product((0.,0.5,1.,float('nan')),(0,1,2,3),(False,True),(-1.,0.,1.,float('nan')),(0.,1.,2.,float('nan')),(0,1,0xffffffff)):
  row=bytearray(banks[1][:0x58]);struct.pack_into('<Hf',row,0x16,flags,chance)
  if limits:struct.pack_into('<4f',row,0x1c,0.,1.,0.,1.)
  choice_cases.append((1,0,bytes(row),4,0,angle,speed,seed,'declared chance/type/range/RNG branch control'))
 # Declared RNG threshold controls include accepted, rejected and exact-tie
 # comparisons. The LCG inverse supplies seeds; no original result is supplied.
 scale_value=struct.unpack('<f',images[0].read(0x4650c0,4))[0]
 inverse=pow(0x19660d,-1,1<<32)
 for high in (0,1,16384,32768,49152,65535):
  threshold=high*scale_value
  bits=struct.unpack('<I',struct.pack('<f',threshold))[0]
  for chance_bits in (max(0,bits-1),bits,bits+1):
   row=bytearray(banks[1][:0x58]);struct.pack_into('<H',row,0x16,0);struct.pack_into('<I',row,0x18,chance_bits)
   seed=((high<<16)-0x3c6ef35f)*inverse&0xffffffff
   choice_cases.append((1,0,bytes(row),1,0,0.,0.,seed,'declared RNG threshold below/equal/above control'))
 canonical_lookup=[];canonical_choice=[];default_rows=[]
 for ii,(image,(path,pin,_))in enumerate(zip(images,IMAGES)):
  assert digest(path)==pin
  g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x900000)
  for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
  page=cache.magic&~4095;g.mem_map(page,((len(cache.data)+cache.magic-page+4095)//4096)*4096);g.mem_write(cache.magic,bytes(cache.data))
  g.mem_map(0x1000000,0x200000);stack,pool,headers,rowaddr,packet,rng,stop=0x11f0000,0x1000000,0x1001000,0x1100000,0x1101000,0x1102000,0x11ff000
  w=lambda p,v:g.mem_write(p,struct.pack('<I',v&0xffffffff))
  lookup_ins=pairs[0x1015a0][ii];lookup_entry=lookup_ins[0].address
  globals_ref=next(o.mem.disp for i in lookup_ins for o in i.operands if o.type==X86_OP_MEM and not o.mem.base and not o.mem.index)
  default=next(o.imm for i in lookup_ins for o in i.operands if i.mnemonic=='mov'and o.type==X86_OP_IMM and o.imm>=0x10000)
  w(globals_ref,ga);default_rows.append(image.read(default,0x58))
  loc=[]
  for case_id,(di,material)in enumerate(lookup_cases):
   w(stack,stop);w(stack+4,defs[di]['offset']+cache.magic);w(stack+8,material)
   g.reg_write(UC_X86_REG_ESP,stack);g.emu_start(lookup_entry,stop,count=10000);result=g.reg_read(UC_X86_REG_EAX)
   base=cache.u(defs[di]['offset']+0x1a0);index=-1 if result==default else (result-base)//0x58
   assert result==default or base<=result<base+len(banks[di])and(result-base)%0x58==0
   row=dict(id=case_id,definition=di,material=material,row=index,returned=f'{result:08x}');loc.append(row)
   if ii==0:canonical_lookup.append((di,material,index))
   else:assert index==canonical_lookup[case_id][2]
  old,new=pairs[0xfeb40];mapping={x.address:y.address for x,y in zip(old,new)}
  begin=0xff090 if ii==0 else mapping[0xff090];end=0xff1d9 if ii==0 else mapping[0xff1d9]
  one_old=0x465030;scale_old=0x4650c0;pool_old=0x4e78d0;rng_old=0x4ee9d8
  refs={}
  for x,y in zip(old,new):
   for xo,yo in zip(x.operands,y.operands):
    if xo.type==yo.type==X86_OP_MEM and xo.mem.disp in (one_old,scale_old,pool_old,rng_old):refs[xo.mem.disp]=xo.mem.disp if ii==0 else yo.mem.disp
  w(refs[pool_old],pool);w(pool+0x44,headers);w(refs[rng_old],rng)
  one=bytes(g.mem_read(refs[one_old],4));scale=bytes(g.mem_read(refs[scale_old],4));choices=[];stores=[]
  def write_hook(uc,access,address,size,value,user):
   if address==rng:stores.append(dict(instruction=f'{uc.reg_read(UC_X86_REG_EIP):08x}',bytes=size,value=value&0xffffffff))
  g.hook_add(UC_HOOK_MEM_WRITE,write_hook)
  for case_id,(di,ri,row,ctype,otype,angle,speed,seed,kind)in enumerate(choice_cases):
   g.mem_write(rowaddr,row);w(stack+0x14,rowaddr);g.mem_write(stack+0x20,struct.pack('<f',angle));g.mem_write(stack+0x10,struct.pack('<f',speed));w(packet,ctype);w(packet+0x40,0x57100002);g.mem_write(headers+2*12+3,bytes([otype]));w(rng,seed);stores.clear()
   g.reg_write(UC_X86_REG_ESP,stack);g.reg_write(UC_X86_REG_EBP,packet);g.reg_write(UC_X86_REG_FPCW,0x27f);g.reg_write(UC_X86_REG_MXCSR,0x1f80);g.emu_start(begin,end,count=1000)
   result=struct.unpack('<h',bytes(g.mem_read(stack+0x10,2)))[0];after=struct.unpack('<I',bytes(g.mem_read(rng,4)))[0]
   item=dict(id=case_id,definition=di,row_index=ri,row_hex=row.hex(),collision_type=ctype,object_type=otype,angle_hex=struct.pack('<f',angle).hex(),speed_hex=struct.pack('<f',speed).hex(),rng_before=seed,rng_after=after,response=result,stores=stores.copy(),boundary=kind);choices.append(item)
   if ii==0:canonical_choice.append((row,ctype,otype,angle,speed,seed,after,result))
   else:assert (result,after)==(canonical_choice[case_id][-1],canonical_choice[case_id][-2]),case_id
  report['images'].append(dict(path=str(path),sha256=pin,lookup_entry=f'{lookup_entry:08x}',default_address=f'{default:08x}',default_hex=default_rows[-1].hex(),globals_ref=f'{globals_ref:08x}',choice_begin=f'{begin:08x}',choice_end=f'{end:08x}',one_hex=one.hex(),rng_scale_hex=scale.hex(),lookup_cases=loc,choice_cases=choices))
 assert default_rows[0]==default_rows[1]
 material=b'PMS1'+struct.pack('<II',len(parents),len(banks))+struct.pack('<'+'h'*len(parents),*parents)
 for bank in banks:material+=struct.pack('<I',len(bank))+bank
 material+=struct.pack('<I',len(canonical_lookup))+b''.join(struct.pack('<Ihi',*row)for row in canonical_lookup)
 choice=b'PRS1'+one+scale+struct.pack('<I',len(canonical_choice))
 for row,ctype,otype,angle,speed,seed,after,result in canonical_choice:choice+=row+struct.pack('<iBffIIh',ctype,otype,angle,speed,seed,after,result)
 (a.output/'materials.bin').write_bytes(material);(a.output/'choices.bin').write_bytes(choice)
 (a.output/'report.json').write_text(json.dumps(report,sort_keys=True,separators=(',',':'))+'\n')
 print(json.dumps(dict(lookup_cases=len(canonical_lookup),choice_cases=len(canonical_choice),report_sha256=digest(a.output/'report.json'))))
if __name__=='__main__':main()

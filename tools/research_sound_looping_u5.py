#!/usr/bin/env python3
"""Pair complete looping-sound storage/reset/request owners and literal policy."""
import argparse,json
from pathlib import Path
import research_animation_selection_u5 as a
from oracle_mp_title_update_display import OLD,NEW,Xbe,_Code
OWNERS=(0x18a200,0x18a2a0,0x18a790,0x18a9c0,0x16d070,0x16d0d0,0x16d220,0x16d2c0,0x223a90,0x21b900,0x1656a0,0x165700,0x125e50,0x223960,0x2239b0)
BOUNDARIES=(0x2230e0,0x327d50,0x326970)

def text(x,address):
 raw=bytearray()
 for n in range(4096):
  c=x.read(address+n,1);raw+=c
  if c==b'\0':return bytes(raw)
 raise AssertionError(('Unterminated original string',hex(address)))

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);q=p.parse_args()
 a.OWNERS=OWNERS;a.BOUNDARIES=BOUNDARIES;a.run(q.output)
 report=json.loads((q.output/'report.json').read_text());rows={int(r['old'],16):r for r in report['owners']};images=[Xbe(OLD),Xbe(NEW)]
 bindings=[]
 for side,x in enumerate(images):
  mem={int(m['old_va'],16):int(m['old_va'if side==0 else'new_va'],16)for r in report['owners']for m in r['memory']}
  def instruction(owner,site):
   left=(q.output/'instructions'/f'{owner:08x}-old.asm').read_text().splitlines();right=(q.output/'instructions'/f'{owner:08x}-u5.asm').read_text().splitlines();at=next(i for i,s in enumerate(left)if s.startswith(f'{site:08x} '));return _Code(x,'.text').at(int((left if side==0 else right)[at].split()[0],16))
  def pushed(owner,site):return next(o.imm for o in instruction(owner,site).operands if o.type==2)
  mem[0x4ed65c]=pushed(0x18a200,0x18a256)
  mem[0x566084]=instruction(0x18a790,0x18a79b).operands[1].mem.disp
  table=pushed(0x223a90,0x223a9e);pool=pushed(0x18a200,0x18a21c);empty=pushed(0x18a790,0x18a7b1);dialogue=pushed(0x18a790,0x18a7e8)
  names=[]
  for n in range(54):
   ptr=int.from_bytes(x.read(table+4*n,4),'little');names.append(dict(pointer=f'{ptr:08x}',raw_hex=text(x,ptr).hex()))
  bindings.append(dict(image='old'if side==0 else'u5',one_bits=x.read(mem[0x465030],4).hex(),minimum_db_bits=x.read(mem[0x45120c],4).hex(),game_allocator_method=rows[0x125e50]['old'if side==0 else'u5'],name_table=f'{table:08x}',name_table216_hex=x.read(table,216).hex(),names=names,pool_name=dict(address=f'{pool:08x}',raw_hex=text(x,pool).hex()),empty_selector=dict(address=f'{empty:08x}',raw_hex=text(x,empty).hex()),dialogue_selector=dict(address=f'{dialogue:08x}',raw_hex=text(x,dialogue).hex()),mutable_runtime_addresses={f'{z:08x}':f'{mem[z]:08x}'for z in(0x51841c,0x4ed650,0x4ed654,0x4ed65c,0x4f4858,0x4f485c,0x4f10b8,0x566084,0x5098f0,0x5098f4)}))
 for k in('one_bits','minimum_db_bits'):assert bindings[0][k]==bindings[1][k]
 for k in('pool_name','empty_selector','dialogue_selector'):assert bindings[0][k]['raw_hex']==bindings[1][k]['raw_hex']
 assert[b['raw_hex']for b in bindings[0]['names']]==[b['raw_hex']for b in bindings[1]['names']]
 a.save(q.output/'providers.json',a.encoded(dict(schema='sound-looping-immutable-v1',bindings=bindings,mutable_addresses_are_operand_correspondence_not_runtime_values=True)))

if __name__=='__main__':main()

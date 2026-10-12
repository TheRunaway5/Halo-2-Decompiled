#!/usr/bin/env python3
"""Execute the original empty-controller walker on actual deduplicated pmov packets.

The packet pointer is an explicit consumer input. Tag-table resolution, particle
creation, world stepping and the surrounding caller are not supplied or claimed.
"""
import argparse,csv,hashlib,json,struct,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path[:0]=[str(ROOT/'tools'),str(ROOT/'tools/reference/halo2-decompiled/tools')]
from oracle_grenade_scene_state import IMAGES,digest
from inspect_multiplayer import Cache
from xbe import Xbe
from functions import _Code,_trace
from research_mp_original_data_u5 import norm
from research_mp_live_locomotion_owners_u5 import raw_prefix_candidates
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_READ
from unicorn.x86_const import *

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args();assert not a.output.exists();a.output.mkdir(parents=True)
 inventory=ROOT/'analysis/multiplayer/integration/vehicle-object-coverage-20261010/root-repeat.json'
 maps=json.loads(inventory.read_text())['maps'];images=[Xbe(p)for p,_,_ in IMAGES];codes=[_Code(x,'.text')for x in images]
 starts=[{int(r['address'],16)for r in csv.DictReader(p.open(),delimiter='\t')}for _,_,p in IMAGES]
 old=[codes[0].at(p)for p in sorted(_trace(codes[0],0x2c06c0,starts[0]).code)];positions={i.address:n for n,i in enumerate(old)};tokens=[norm(i,images[0],positions)for i in old];matched=[]
 for entry in raw_prefix_candidates(codes[0],images[0],codes[1],0x2c06c0,12):
  try:
   candidate=[codes[1].at(p)for p in sorted(_trace(codes[1],entry,starts[1]).code)];pos={i.address:n for n,i in enumerate(candidate)}
   if [norm(i,images[1],pos)for i in candidate]==tokens:matched.append(candidate)
  except Exception:pass
 assert len(matched)==1;paired=[old,matched[0]];cases=[]
 for m in maps:
  path=ROOT/m['path'];assert digest(path)==m['sha256'];cache=Cache(path)
  for datum,tag in cache.tags.items():
   if tag['group']!='pmov' or tag['offset'] is None or tag['size']:continue
   owned=[(d,t)for d,t in cache.tags.items()if t['group']==tag['group']and t['offset']==tag['offset']and t['size']]
   assert owned and len({t['size']for _,t in owned})==1
   packet=bytes(cache.data[tag['offset']:tag['offset']+owned[0][1]['size']]);assert len(packet)>=0x14 and struct.unpack_from('<I',packet,0xc)[0]==0
   cases.append(dict(map=m['path'],map_sha256=m['sha256'],datum=f'{datum:08x}',backing_datum=f'{owned[0][0]:08x}',address=tag['offset']+cache.magic,packet_hex=packet.hex()))
  cache.data.close();cache.stream.close()
 assert len(cases)==69
 report=dict(schema='original-empty-pmov-alias-v1',tool_sha256=digest(Path(__file__)),inventory_sha256=digest(inventory),images=[],cases=cases,boundaries=[
  'Actual same-address alias and positive-size same-group owned packet read from each of23 maps. No guessed extent, replacement definition or imported payload in repository source.',
  'EDI is the explicit authentic packet pointer. EAX system and ECX head are intentionally unmapped diagnostics because this actual count0 path must not consume them.',
  'Complete controller walker bytes paired and executed in both images. Surrounding tag lookup, particle creation, lifecycle, world and graphics are not executed.'])
 for ii,(image,(path,pin,_))in enumerate(zip(images,IMAGES)):
  assert digest(path)==pin;instructions=paired[ii];(a.output/('tu11.asm'if ii==0 else'u5.asm')).write_text(''.join(f'{i.address:08x} {bytes(i.bytes).hex()} {i.mnemonic} {i.op_str}\n'for i in instructions));results=[]
  for c in cases:
   g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x900000)
   for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
   address=c['address'];page=address&~4095;packet=bytes.fromhex(c['packet_hex']);g.mem_map(page,((address-page+len(packet)+4095)//4096)*4096);g.mem_write(address,packet)
   stack,stop=0x1000000,0x1001000;g.mem_map(stack-0x1000,0x3000);g.mem_write(stack,struct.pack('<4I',stop,0,0x3d088889,0))
   executed=[];reads=[];g.hook_add(UC_HOOK_CODE,lambda uc,p,n,u:executed.append(p));g.hook_add(UC_HOOK_MEM_READ,lambda uc,access,p,n,value,u:reads.append((p,n)))
   g.reg_write(UC_X86_REG_ESP,stack);g.reg_write(UC_X86_REG_EDI,address);g.reg_write(UC_X86_REG_EAX,0xdead0000);g.reg_write(UC_X86_REG_ECX,0xffffffff)
   g.emu_start(instructions[0].address,stop,count=1000)
   assert g.reg_read(UC_X86_REG_ESP)==stack+16 and g.reg_read(UC_X86_REG_EAX)==0
   assert [(p,n)for p,n in reads if page<=p<page+4096]==[(address+0xc,4)]
   assert not any(codes[ii].at(p).mnemonic=='call'for p in executed)
   results.append(dict(datum=c['datum'],map=c['map'],executed=[f'{p:08x}'for p in executed],packet_read=[f'{address+0xc:08x}',4],stack_cleanup=16,controller_calls=0))
  report['images'].append(dict(path=str(path),sha256=pin,entry=f'{instructions[0].address:08x}',instructions=len(instructions),results=results))
 (a.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');print(json.dumps(dict(cases=len(cases)*2,owned_maps=len(maps),controller_calls=0,report_sha256=digest(a.output/'report.json'))))
if __name__=='__main__':main()

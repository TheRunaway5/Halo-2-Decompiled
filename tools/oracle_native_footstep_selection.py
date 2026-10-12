#!/usr/bin/env python3
"""Unchanged base 1883E0 with actual Lockout matg/foot payloads.

Declared ABI controls select material/kind/variant and bypass distance only.
No unit/contact/PVS/driver/readiness result is supplied or claimed.
"""
import argparse,hashlib,json,struct,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path[:0]=[str(ROOT/'tools'),str(ROOT/'tools/reference/halo2-decompiled/tools')]
from inspect_multiplayer import Cache
from xbe import Xbe
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EBX

def main():
 p=argparse.ArgumentParser();p.add_argument('--output',type=Path,required=True);p.add_argument('--image',choices=['base','1.1','1.5'],default='base');a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True)
 paths={'base':ROOT/'assets/disc/default.xbe','1.1':ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03','1.5':Path('/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe')}
 pins={'base':'03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d','1.1':'aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377','1.5':'21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'}
 entry,global_va,table_va={'base':(0x1883e0,0x4e034c,0x4e3b44),'1.1':(0x189e10,0x4e791c,0x4eb114),'1.5':(0x18b1d0,0x4ecc1c,0x4f0414)}[a.image]
 x=Xbe(paths[a.image]);c=Cache(ROOT/'assets/disc/maps/lockout.map')
 assert hashlib.sha256(x.data).hexdigest()==pins[a.image]
 u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x800000)
 for s in x.sections:u.mem_write(s.va,x.section_bytes(s))
 page=c.magic&~4095;u.mem_map(page,((len(c.data)+c.magic-page+4095)//4096)*4096);u.mem_write(c.magic,bytes(c.data))
 u.mem_map(0x1000000,0x100000);table=0x1000000;stack=0x10f0000;stop=0x10fe000;out=0x10fd000
 for datum,t in c.tags.items():
  if t['offset'] is not None:u.mem_write(table+(datum&65535)*16+8,struct.pack('<I',t['offset']+c.magic))
 matg=next(t for t in c.tags.values()if t['group']=='matg');bipd=next(t for t in c.tags.values()if t['name']=='objects\\characters\\masterchief\\masterchief_mp')
 foot=c.u(bipd['offset']+0x58);variant=c.data[bipd['offset']+0x1a]
 u.mem_write(global_va,struct.pack('<I',matg['offset']+c.magic));u.mem_write(table_va,struct.pack('<I',table))
 rows=[]
 for material in(0,1,2,106,107,144,167):
  for kind in(0,1,3,6,7,13,14,16,17,19,20):
   for v in(-1,variant):
    args=[1,0,material,material,kind,v]+[out+i*4 for i in range(6)]
    u.mem_write(out,b'\xa5'*24);u.mem_write(stack,struct.pack('<13I',stop,*[i&0xffffffff for i in args]))
    u.reg_write(UC_X86_REG_ESP,stack);u.reg_write(UC_X86_REG_EBX,foot);u.emu_start(entry,stop,count=1000000)
    result=struct.unpack('<6I',u.mem_read(out,24));rows.append(dict(material=material,kind=kind,variant=v,sounds=[f'{result[i]:08x}'for i in(0,2,4)],all_outputs=[f'{i:08x}'for i in result]))
 text=''.join(f'{r["material"]}\t{r["kind"]}\t{r["variant"]}\t'+ '\t'.join(r['sounds'])+'\n'for r in rows)
 (a.output/'fixtures.tsv').write_text(text)
 info=dict(scope=__doc__,xbe_sha256=hashlib.sha256(x.data).hexdigest(),map_sha256=hashlib.sha256(c.data).hexdigest(),foot=f'{foot:08x}',variant=variant,cases=rows)
 (a.output/'report.json').write_text(json.dumps(info,sort_keys=True,indent=2)+'\n')
 print(f'PASS {len(rows)} original whole {entry:08x} {a.image} selection calls, no substituted callee')
if __name__=='__main__':main()

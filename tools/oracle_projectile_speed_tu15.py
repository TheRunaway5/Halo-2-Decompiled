#!/usr/bin/env python3
"""Execute original101990/1028B0 and1ED030 on actual projectile/globals metadata.

Flags, full player datum, game mode/difficulty and missing globals/table controls
are declared data inputs. Both complete callees execute without code patches or
supplied return values. This proves a speed scalar, not complete projectile birth.
"""
import argparse,hashlib,json,struct
from pathlib import Path
from oracle_smg_firing_1_1 import Trace,Cache,ROOT,GAME
from oracle_weapon_graph_timing_tu15 import Original
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_XMM0

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--fixture',type=Path,required=True);a=p.parse_args()
 c=Cache(ROOT/'assets/disc/maps/lockout.map')
 proj=next(t for t in c.tags.values() if t['group']=='proj' and t['name'].endswith('\\plasma_pistol_bolt'))
 globals_=next(t for t in c.tags.values() if t['group']=='matg');gp=globals_['offset']+c.magic
 record=c.u(globals_['offset']+0xfc);count=c.u(globals_['offset']+0xf8)
 assert count and record
 table=struct.unpack_from('<4I',c.data,record-c.magic+0xa0)
 lines=['# image flags player campaign difficulty table_kind table0..3 result'];reports=[]
 for version,cls,entry,global_address,game_address in ((11,Trace,0x101990,0x4e791c,0x4edf18),(15,Original,0x1028b0,0x4ecc1c,0x4f3218)):
  t=cls();t.w(game_address,GAME);reached={}
  t.g.hook_add(4,lambda g,pc,size,u:reached.setdefault(pc,bytes(g.mem_read(pc,size)).hex()))
  for flags in (0,1,0x400,0x401,0xffffffff):
   t.w(proj['offset']+c.magic+0xbc,flags)
   for player in (0xffffffff,0,0x80000003):
    for campaign in (0,1):
     t.w(GAME+8,1 if campaign else 2)
     for difficulty in (-32768,-1,0,1,2,3,4,32767):
      t.sh(GAME+0x132,difficulty)
      for kind in range(4):
       t.w(global_address,0 if kind==0 else gp)
       t.w(gp+0xf8,0 if kind==1 else count);t.w(gp+0xfc,0 if kind==2 else record)
       t.run(entry,(player,),((UC_X86_REG_EAX,int(proj['datum'],16)),))
       result=t.g.reg_read(UC_X86_REG_XMM0)&0xffffffff
       lines.append(' '.join(f'{v&0xffffffff:08x}' for v in (version,flags,player,campaign,difficulty,kind,*table,result)))
  for pc,raw in reached.items():assert t.image.read(pc,len(bytes.fromhex(raw)))==bytes.fromhex(raw)
  reports.append(dict(image=version,xbe_sha256=hashlib.sha256(t.image.data).hexdigest(),reached_instruction_bytes={f'{pc:08x}':raw for pc,raw in sorted(reached.items())}))
 a.fixture.write_text('\n'.join(lines)+'\n');a.output.write_text(json.dumps(dict(scope=__doc__,cases=len(lines)-1,images=reports,
  fixture_sha256=hashlib.sha256(a.fixture.read_bytes()).hexdigest(),cache_sha256=hashlib.sha256(c.data).hexdigest(),
  tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),instruction_patches=[],supplied_results=[]),indent=2,sort_keys=True)+'\n')
 print('original speed scalar cases',len(lines)-1,flush=True)
if __name__=='__main__':main()

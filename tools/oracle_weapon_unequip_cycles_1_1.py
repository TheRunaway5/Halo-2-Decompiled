#!/usr/bin/env python3
"""Complete original E9A90 clear caller and subsequent holstered weapon clocks.

Actual Lockout weapon/matg representation metadata and declared single-hand
object/player/time storage are inputs. All weapon cleanup leaves and graph
queries execute unchanged. Model-parent detachment and object-world dirty/
activation notifications are supplied boundaries, as are inherited failed
projectile/effect allocation and unavailable unit animation feedback. The
own-model manager is explicitly absent. Dropped cases then declare the raw
post-detach owner/item/parent ABI; the full inventory/world release is not run. No full world or rendered parity.
"""
import argparse,gc,hashlib,json,struct
from pathlib import Path
from oracle_smg_firing_1_1 import Trace,Cache,ROOT,WEAPON,UNIT,WI,UI,FPS,TIME,PLAYER_DATA,SEED
from unicorn.x86_const import UC_X86_REG_EAX

NAMES=('magnum','battle_rifle','covenant_carbine','shotgun','sniper_rifle','brute_shot',
       'needler','rocket_launcher','smg','plasma_rifle','brute_plasma_rifle','beam_rifle')

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--fixture',type=Path,required=True);a=p.parse_args()
 a.output.mkdir(parents=True,exist_ok=True);cache=Cache(ROOT/'assets/disc/maps/lockout.map')
 globals_=next(t for t in cache.tags.values() if t['group']=='matg');representations=cache.block(globals_['offset']+0x138,0xbc)
 lines=['# I/T weapon representation pattern: clocks6 magazine4 selected13 alternate13 trigger_state ticks flags input own_state seed'];cases=[]
 for name in NAMES:
  tag=next(t for t in cache.tags.values() if t['group']=='weap' and t['name'].split('\\')[-1]==name)
  trigger=cache.block(tag['offset']+0x2c8,0x40)[0];index=cache.h(trigger+8);barrels=cache.block(tag['offset']+0x2d0,0xec);magazines=cache.block(tag['offset']+0x2c0,0x5c);mag=magazines[0] if magazines else None
  for rep in (2,3):
   for context in ('holstered','dropped'):
    for pattern in (('burst','reload_early','reload_late','chamber') if mag is not None else ('burst',)):
     t=Trace(1);t.w(WEAPON,int(tag['datum'],16));t.w(FPS+0x10,0xffffffff);t.w(UNIT,cache.u(representations[rep]+0xb4));t.g.mem_write(PLAYER_DATA+0x88,bytes([rep]))
     t.services.update({0xefe30:('unit activity notification: unavailable',0,0),0xba980:('model-parent detach: unavailable',0,0),0xbac10:('object Havok owner admission: unavailable',0,0),0xb9a60:('object active-set notification: unavailable',0,0),0xb81f0:('object visibility notification: unavailable',0,0),0xb8180:('object activation notification: unavailable',0,0)})
     t.active='constructor';t.run(0x101bb0,(WI,));t.w(WEAPON+4,1);t.sh(WEAPON+0x12a,-1);t.sh(WEAPON+0x16c,0x20);t.sh(WEAPON+0x16e,2);t.sh(WEAPON+0x17c,7)
     t.g.mem_write(WEAPON+0x20c,b'\x02');t.sh(WEAPON+0x20e,7);t.w(WEAPON+0x210,0x20)
    
     if mag is not None:t.sh(WEAPON+0x22c,cache.h(mag+10)-2);t.sh(WEAPON+0x22a,4)
     if pattern.startswith('reload'):
      for off,value in ((0x224,1),(0x226,30),(0x230,18 if pattern=='reload_early' else 12),(0x232,30)):t.sh(WEAPON+off,value)
     elif pattern=='chamber':
      for off,value in ((0x224,6),(0x226,7),(0x230,-1),(0x232,-1)):t.sh(WEAPON+off,value)
     for n in range(len(barrels)):
      at=WEAPON+0x1a4+n*52;t.g.mem_write(at,b'\x01\x01');t.sh(at+2,5);t.sh(at+6,1)
      for off,value in ((0x18,1.),(0x1c,.5),(0x20,.5),(0x28,.25)):t.f(at+off,value)
     def words():
      result=[t.u(WEAPON+0x19e,2),t.u(WEAPON+0x180),t.u(WEAPON+0x184),t.u(WEAPON+0x188),t.u(WEAPON+0x16c,2),t.u(WEAPON+0x17c,2)]
      result.extend(struct.unpack('<4I',bytes(t.g.mem_read(WEAPON+0x224,16))))
      result.extend(struct.unpack('<13I',bytes(t.g.mem_read(WEAPON+0x1a4+index*52,52))))
      result.extend(struct.unpack('<13I',bytes(t.g.mem_read(WEAPON+0x1a4+(1-index)*52,52))) if len(barrels)==2 else [0]*13)
      result.extend((t.u(WEAPON+0x20c,1),t.u(WEAPON+0x20e,2),t.u(WEAPON+0x210),t.u(WEAPON+0x16e,2),t.u(WEAPON+0x178),t.u(SEED)))
      assert len(result)==42;return result
     initial=words();t.active='clear_current';t.run(0xe9a90,(UI,),((UC_X86_REG_EAX,1),));cleared=words();rows=[]
     assert t.u(UNIT+0x212,1)==255 and t.u(WEAPON+0x12c,1)&2 and t.u(WEAPON+0x154)==UI
     if context=='dropped':
      #Explicit post-detach ABI, not execution ofCFED0/CF8C0/world release.
      t.w(WEAPON+0x154,0xffffffff);t.w(WEAPON+0x14,0xffffffff);t.w(UNIT+0x218,0xffffffff);t.g.mem_write(WEAPON+0x12c,b'\x00')
     for tick in range(60):
      t.tick=tick;t.w(TIME+8,tick);t.active='passive_update';t.run(0x1021d0,(WI,));t.active='projectile_postpass';t.run(0x1019f0);rows.append(words())
     report=dict(scope=__doc__,weapon=tag,representation=rep,context=context,pattern=pattern,initial=initial,cleared=cleared,rows=rows,events=t.events,supplied_services=t.services,instruction_patches=[],xbe_sha256=hashlib.sha256(t.image.data).hexdigest(),cache_sha256=hashlib.sha256(cache.data).hexdigest())
     path=a.output/f'{name}-{rep}-{context}-{pattern}.json';path.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
     lines.append(' '.join(['I',name,str(rep),context,pattern,*(f'{v:08x}' for v in initial)]))
     for row in rows:lines.append(' '.join(['T',name,str(rep),context,pattern,*(f'{v:08x}' for v in row)]))
     cases.append(dict(weapon=name,representation=rep,context=context,pattern=pattern,path=str(path),sha256=hashlib.sha256(path.read_bytes()).hexdigest(),ticks=60));print(name,rep,context,pattern,flush=True);del t,report;gc.collect()
 a.fixture.write_text('\n'.join(lines)+'\n');(a.output/'report.json').write_text(json.dumps(dict(scope=__doc__,cases=cases,fixture_sha256=hashlib.sha256(a.fixture.read_bytes()).hexdigest(),tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),inherited_tool_sha256=hashlib.sha256((ROOT/'tools/oracle_smg_firing_1_1.py').read_bytes()).hexdigest(),instruction_patches=[]),indent=2,sort_keys=True)+'\n')
if __name__=='__main__':main()

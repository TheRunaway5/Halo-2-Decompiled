#!/usr/bin/env python3
"""Original plasma-pistol constructor/input/update and both projectile postpasses.

Both authored barrels and their real projectile references execute. Raw unit,
player, view/time and no-own-model storage are declared ABI inputs. Generic
effect/projectile allocation and animation/world notifications are unavailable
boundaries inherited from Trace, not successful original world execution.
"""
import argparse,hashlib,json,struct
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,PIN,CACHE_PIN,Trace,Cache,WEAPON,UNIT,FPS,PLAYER_DATA
class ChargedTrace(Trace):
 def state(self):
  r=super().state()
  r.update(barrels_hex=bytes(self.g.mem_read(WEAPON+0x1a4,104)).hex(),
    trigger_hex=bytes(self.g.mem_read(WEAPON+0x20c,12)).hex(),analog_hex=bytes(self.g.mem_read(WEAPON+0x170,6)).hex(),
    heat_bits=f'{self.u(WEAPON+0x180):08x}',spent_bits=f'{self.u(WEAPON+0x184):08x}',
    inhibitor_bits=f'{self.u(WEAPON+0x188):08x}',blocking=self.u(WEAPON+0x17c,2))
  return r

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--fixture',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True)
 c=Cache(ROOT/'assets/disc/maps/lockout.map');tag=next(t for t in c.tags.values() if t['group']=='weap' and t['name'].endswith('\\plasma_pistol'))
 globals_=next(t for t in c.tags.values() if t['group']=='matg');representations=c.block(globals_['offset']+0x138,0xbc)
 patterns={'tap':([False]*5+([True]+[False]*20)*3+[False]*60,0),
   'early_release':([False]*5+[True]*10+[False]*120,0),
   'charge_edge':([False]*5+[True]*18+[False]*140,0),
   'full_charge':([False]*5+[True]*60+[False]*160,0),
   'repeat_charge':([False]*5+([True]*25+[False]*100)*2,0),
   'spent':([False]*5+([True]+[False]*12)*20+[False]*120,0.99)}
 lines=[];cases=[]
 for representation in (2,3):
  biped=c.tags[c.u(representations[representation]+0xb4)]
  for local in (False,True):
   for name,(pattern,spent) in patterns.items():
    class Controlled(ChargedTrace):
     def run(self,address,*args,**kwargs):
      super().run(address,*args,**kwargs)
      if address==0x101bb0:self.f(WEAPON+0x184,spent)
    t=Controlled(1);t.w(WEAPON,int(tag['datum'],16));t.w(UNIT,int(biped['datum'],16));t.g.mem_write(PLAYER_DATA+0x88,bytes([representation]))
    if not local:t.w(FPS+0x10,0xffffffff)
    t.services[0x177a10]=('generic overheat effect allocation: unavailable',0xffffffff,4)
    t.services[0xd2310]=('unit camouflage disruption: inherited unavailable owner',0,8)
    report=t.execute(pattern=pattern);report.update(scope=__doc__,tag=tag,representation=representation,local_view=local,initial_spent=spent)
    file=a.output/f'{representation}-{int(local)}-{name}.json';file.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    for row in report['rows']:
     s=row['post_dispatch'];first,last=row['event_range'];events=report['events'][first:last]
     effects=[e for e in events if e.get('service')=='00107b50'];projectiles=[e for e in events if e.get('service')=='000b8a30']
     words=[row['tick'],int(row['held']),int(s['heat_bits'],16),int(s['spent_bits'],16),int(s['weapon_flags'],16)&15,int(s['seed'],16),*s['hand_timers'],s['blocking'],int(s['inhibitor_bits'],16)]
     words.extend(struct.unpack('<26I',bytes.fromhex(s['barrels_hex'])));words.extend(struct.unpack('<3I',bytes.fromhex(s['trigger_hex'])))
     analog=bytes.fromhex(s['analog_hex']);words.extend([int.from_bytes(analog[:4],'little'),int.from_bytes(analog[4:],'little'),len(effects),len(projectiles)])
     for e in effects:words.extend(int(v,16) for v in (e['registers']['eax'],*e['stack_arguments']))
     for e in projectiles:words.extend((int(e['request_definition'],16),*map(lambda v:int(v,16),e['request_forward']['bits'])))
     lines.append(' '.join([str(representation),str(int(local)),name,*(f'{v:08x}' for v in words)]))
    cases.append(dict(representation=representation,local=local,pattern=name,ticks=len(pattern),requests=report['summary']['projectile_requests'],path=str(file),sha256=hashlib.sha256(file.read_bytes()).hexdigest()))
    print(representation,local,name,'ticks',len(pattern),'projectiles',report['summary']['projectile_requests'],flush=True)
 a.fixture.write_text('\n'.join(lines)+'\n');(a.output/'report.json').write_text(json.dumps(dict(scope=__doc__,cases=cases,fixture_sha256=hashlib.sha256(a.fixture.read_bytes()).hexdigest(),tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),base_oracle_sha256=hashlib.sha256((ROOT/'tools/oracle_smg_firing_1_1.py').read_bytes()).hexdigest(),xbe_sha256=PIN,cache_sha256=CACHE_PIN,instruction_patches=[]),indent=2,sort_keys=True)+'\n')
if __name__=='__main__':main()

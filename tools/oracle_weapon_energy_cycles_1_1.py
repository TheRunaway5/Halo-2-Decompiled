#!/usr/bin/env python3
"""Original plasma/beam input/update/postpass timelines with named boundaries.

Actual constructor, input, complete update and two postpasses execute unchanged.
Inherited raw equipped storage and absent projectile/animation owners remain
explicit. Generic overheat allocation is supplied NONE at177A10; no success
or particle behavior is inferred. Registered and unregistered view cases both
retain original representation-based authored graph/event/timing lookup.
Initial spent fractions are declared state controls after construction.
"""
import argparse,hashlib,json,struct
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,PIN,CACHE_PIN,Trace,Cache,WEAPON,FPS

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True)
    p.add_argument('--fixture',type=Path,required=True);a=p.parse_args()
    c=Cache(ROOT/'assets/disc/maps/lockout.map');a.output.mkdir(parents=True,exist_ok=True)
    patterns={'held_release':[False]*5+[True]*300+[False]*180,
              'pulses':[False]*5+[True,False]*150+[False]*180,
              'exhaustion':[False]*5+[True,False]*100+[False]*180}
    lines=['# Original TU1.1 plasma/beam input/update/postpasses. Named allocation and optional-view boundaries.'];cases=[]
    for name in ('plasma_rifle','brute_plasma_rifle','beam_rifle'):
        tag=next(t for t in c.tags.values()if t['group']=='weap'and t['name'].split('\\')[-1]==name)
        for local in (False,True):
            for pattern,inputs in patterns.items():
                initial_spent=0.99 if pattern=='exhaustion'else 0.
                class EnergyTrace(Trace):
                    def state(self):
                        r=super().state()
                        r.update(heat_bits=f'{self.u(WEAPON+0x180):08x}',spent_bits=f'{self.u(WEAPON+0x184):08x}')
                        return r
                    def run(self,address,*args,**kwargs):
                        super().run(address,*args,**kwargs)
                        if address==0x101bb0:self.f(WEAPON+0x184,initial_spent)
                t=EnergyTrace(1);t.w(WEAPON,int(tag['datum'],16))
                t.services[0x177a10]=('generic effect allocation: explicitly unavailable',0xffffffff,4)
                if not local:t.w(FPS+0x10,0xffffffff)
                report=t.execute(pattern=inputs)
                report.update(scope=__doc__,actual_weapon_tag=tag,local_view=local,initial_spent=initial_spent)
                path=a.output/f'{name}-{int(local)}-{pattern}.json';path.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
                for row in report['rows']:
                    state=row['post_dispatch'];raw=bytes.fromhex(state['barrel_state_hex'])
                    first,last=row['event_range'];requests=[e for e in report['events'][first:last]
                        if e.get('service')=='00107b50'and e['registers']['eax']!='ffffffff']
                    values=[row['tick'],int(row['held']),int(state['heat_bits'],16),int(state['spent_bits'],16),
                        int(state['weapon_flags'],16)&6,raw[0],raw[1],int.from_bytes(raw[2:4],'little'),
                        int.from_bytes(raw[6:8],'little'),struct.unpack_from('<I',raw,0x10)[0],
                        struct.unpack_from('<I',raw,0x28)[0],len(requests)]
                    values[-1:-1]=[struct.unpack_from('<I',raw,0x1c)[0],struct.unpack_from('<I',raw,0x20)[0],int(state['seed'],16)]
                    for request in requests:
                        values.extend(int(v,16)for v in(request['registers']['eax'],*request['stack_arguments'][1:3]))
                    lines.append(' '.join([name,str(int(local)),pattern,*(f'{v:08x}'for v in values)]))
                cases.append(dict(name=name,local=local,pattern=pattern,ticks=len(report['rows']),report=str(path),
                    sha256=hashlib.sha256(path.read_bytes()).hexdigest(),shots=report['summary']['projectile_requests']))
                print(name,local,pattern,len(report['rows']),report['summary']['projectile_requests'],flush=True)
    a.fixture.parent.mkdir(parents=True,exist_ok=True);a.fixture.write_text('\n'.join(lines)+'\n')
    (a.output/'report.json').write_text(json.dumps(dict(scope=__doc__,cases=cases,xbe_sha256=PIN,cache_sha256=CACHE_PIN,
        fixture_sha256=hashlib.sha256(a.fixture.read_bytes()).hexdigest(),
        tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),instruction_patches=[]),indent=2,sort_keys=True)+'\n')
if __name__=='__main__':main()

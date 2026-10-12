#!/usr/bin/env python3
"""Needler prepared effects through unchanged equipped pickup and weapon tick.

Inherits the bounded equipped Unit/resource/services;107B50 remains explicit
failed allocation. This records original requests and scalars, not successful
whole original effect/projectile/world/audio execution. No instructions edited.
"""
import json,hashlib
from pathlib import Path
from oracle_covenant_pickup_shader_state_1_1 import Trace,ROOT,PIN,CACHE_PIN
from oracle_smg_firing_1_1 import TIME
from unicorn.x86_const import UC_X86_REG_XMM0

def main():
 t=Trace(6);t.seeded_duration(2,232);t.poll('original_needler_233ms_pickup',{2:255},milliseconds=1)
 rows=[];requests=[];previous=False;p=t.target_pointer;datum=t.target_datum
 for n in range(100):
  held=30<=n<67 or 82<=n<95
  t.tick=n;t.w(TIME+8,n);t.active='declared_needler_input_then_original1021D0'
  t.run(0x1045e0,(datum,(2 if held else 0)|(8 if held and not previous else 0),0x3f800000 if held else 0));previous=held
  first=len(t.events);t.run(0x1021d0,(datum,),((UC_X86_REG_XMM0,0),))
  calls=[]
  for event in t.events[first:]:
   if event.get('service')!='00107b50':continue
   row=dict(tick=n,effect=event['registers']['eax'],weapon=event['registers']['ecx'],
      previous=event['stack_arguments'][0],a=event['stack_arguments'][1],b=event['stack_arguments'][2],loaded=t.u(p+0x22c,2))
   calls.append(row);requests.append(row)
  rows.append(dict(tick=n,fire=held,loaded=t.u(p+0x22c,2),rate=t.u(p+0x1b4),calls=calls))
 out=ROOT/'analysis/multiplayer/integration/needler-firing-effects-original-20261012.json'
 out.write_text(json.dumps(dict(scope=__doc__,xbe_sha256=PIN,cache_sha256=CACHE_PIN,rows=rows,requests=requests,
   instruction_patches=[],oracle_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
   inherited_service_boundaries={f'{e:08x}':v for e,v in t.services.items()}),indent=2,sort_keys=True)+'\n')
 lines=['# Actual original Needler prepared-effect calls; inherited explicit services in provenance.']
 for r in rows:
  values=[r['tick'],int(r['fire']),r['loaded'],r['rate'],len(r['calls'])]
  for c in r['calls']:values.extend(int(c[k],16)for k in ('effect','a','b'))
  lines.append(' '.join(f'{v:08x}'for v in values))
 (ROOT/'tests/fixtures/needler_firing_effects_original.tsv').write_text('\n'.join(lines)+'\n')
 print(json.dumps(dict(report=str(out),ticks=len(rows),requests=len(requests))),flush=True)
if __name__=='__main__':main()

#!/usr/bin/env python3
"""Separate explicit original HUD branch controls; not ordinary-state producers."""
import argparse,hashlib,json,struct
from pathlib import Path
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ESI
from oracle_mp_scope_presentation_1_1 import PresentationTrace,ROOT,UNIT,STATUS,OUTPUT,MODELS,WATCH,sha,h

def main():
 p=argparse.ArgumentParser();p.add_argument('--output',type=Path,required=True);a=p.parse_args();t=PresentationTrace()
 wp=t.weapon_pointer(t.initial_datum) if hasattr(t,'weapon_pointer') else t.u(t.u(0x4e78d0)+0x44)+(t.initial_datum&65535)*12
 if not hasattr(t,'weapon_pointer'):wp=t.u(wp+8)
 definition=t.u(t.u(0x4eb114)+(t.u(wp)&65535)*16+8)
 original_count=struct.unpack('<h',t.g.mem_read(definition+0x1fe,2))[0]
 original_magazine=t.u(definition+0x2c0)
 original_mode=struct.unpack('<h',t.g.mem_read(wp+0x224,2))[0]
 original_index=bytes(t.g.mem_read(UNIT+0x212,1))
 cases=[]
 def sample(name,current,primary=True,count=None,modes=None,zero_magazines=False):
  t.active=name;t.tick=len(cases)
  t.g.mem_write(UNIT+0x240,struct.pack('<b',current))
  t.g.mem_write(UNIT+0x212,original_index if primary else b'\xff')
  t.sh(definition+0x1fe,original_count if count is None else count)
  t.w(definition+0x2c0,0 if zero_magazines else original_magazine)
  t.sh(wp+0x224,original_mode if modes is None else modes[0])
  calls=len(t.presentation_calls);stores=len(t.presentation_stores)
  status=t.status_prefix();t.active=name+'_mask_builder'
  t.run(0x13b7c0,(0,1,OUTPUT,OUTPUT+4,OUTPUT+6,OUTPUT+8,OUTPUT+10),((UC_X86_REG_ESI,STATUS),))
  masks=list(struct.unpack('<4H',t.g.mem_read(OUTPUT+4,8)))
  cases.append(dict(name=name,explicit_modified_context=dict(current_signed_byte=current,primary_present=primary,
   authored_zoom_count=original_count if count is None else count,authored_magazine_count=0 if zero_magazines else original_magazine,
   magazine_modes=[]if zero_magazines else[original_mode if modes is None else modes[0]]),
   source_status_current=status['current_status'],source_status_binocular=status['binocular_status'],source_zoom_mask=masks[0]&0x5c0,
   full_mask_builder_outputs=masks,source_status_prefix=status,call_range=[calls,len(t.presentation_calls)],store_range=[stores,len(t.presentation_stores)]))
 for current in range(-128,128):sample('stock_current_'+str(current),current)
 for count in (0,1,-1,32767,-32768):
  for mode in (-1,0,1,2,3,4,5,6,7):sample(f'count_{count}_mode_{mode}',0,count=count,modes=[mode])
 for current in (-128,-1,0,1,127):sample('no_primary_'+str(current),current,primary=False)
 for count in (0,1,-1):sample('zero_magazines_count_'+str(count),0,count=count,zero_magazines=True)
 t.g.mem_write(UNIT+0x212,original_index);t.sh(definition+0x1fe,original_count);t.w(definition+0x2c0,original_magazine);t.sh(wp+0x224,original_mode)
 a.output.mkdir(parents=True,exist_ok=True)
 report=dict(schema=1,tool_sha256=sha(__file__),presentation_dependency_sha256=sha(ROOT/'tools/oracle_mp_scope_presentation_1_1.py'),
  original_xbe_sha256=sha(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'),cases=cases,calls=t.presentation_calls,ordered_stores=t.presentation_stores,
  boundaries=['All modified byte/count/mode/primary-index fields are explicit branch stimuli on the frozen real allocated/registered SMG context; this suite does not claim their ordinary producer/lifecycle.',
   'Original HUD prefix and full mask builder execute unchanged. The zero-magazine branch deliberately varies original authored definition count; no allocation/admission/visibility service return is supplied.'])
 (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps(dict(report_sha256=sha(a.output/'report.json'),cases=len(cases))),flush=True)

if __name__=='__main__':main()

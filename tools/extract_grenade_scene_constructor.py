#!/usr/bin/env python3
"""Extract original captured constructor/release field provenance without rewriting it."""
import argparse,hashlib,json,struct
from pathlib import Path
from oracle_grenade_scene_state import PRIOR,PRIOR_PIN,IMAGES,digest
from xbe import Xbe

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
 assert digest(PRIOR)==PRIOR_PIN;r=json.loads(PRIOR.read_text());base=0x82d89b50
 fields=[(4,4),(0xa0,4),(0xb1,1),(0x10a,2),(0x114,4),(0x118,4),(0x12a,2),(0xc1,1)]
 writes=[s for s in r['stores'] if any(int(s['address'],16)<base+off+size and base+off<int(s['address'],16)+s['size'] for off,size in fields)]
 states=[]
 for i in (100,130,160,183,184,240):
  o=next(o for o in r['states'][i]['objects'] if o['datum']=='e2710002');b=bytes.fromhex(o['body_hex'])
  states.append(dict(state=i,object=o,fields={f'{off:x}':b[off:off+size].hex() for off,size in fields}))
 calls=[c for c in r['calls'] if c['entry']=='000b8a30' and any(p.get('raw_hex','').startswith('2306a8e7') for p in c.get('packets',[])+c.get('after_packets',[]))]
 assert len(calls)==1
 one=[]
 for (path,pin,_),address in zip(IMAGES,(0x465030,0x46a0d0)):
  assert digest(path)==pin;image=Xbe(path);value=image.read(address,4);assert value.hex()=='0000803f';one.append(dict(image_sha256=pin,address=f'{address:08x}',raw_hex=value.hex()))
 result=dict(schema='grenade-scene-constructor-fields-v1',scope=__doc__,prior=str(PRIOR),prior_sha256=PRIOR_PIN,tool_sha256=digest(Path(__file__)),states=states,stores=writes,original_constructor_call=calls[0],constructor_one=one,
  limits='The original allocated/released record is frag. Plasma in the separate consumer suite is explicitly a changed-definition direct ABI control. No plasma allocation, successful registration for translated controls or angular physics is asserted.')
 (a.output/'report.json').write_text(json.dumps(result,sort_keys=True,indent=2)+'\n')
if __name__=='__main__':main()

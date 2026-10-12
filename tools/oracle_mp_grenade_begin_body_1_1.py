#!/usr/bin/env python3
"""Observe full admitted E95E0 and every direct service without guest edits."""
import argparse,hashlib,json,sys
from pathlib import Path
import oracle_mp_grenade_effect_pool_1_1 as owning
ROOT=owning.ROOT
DIRECT={0x1055f0,0x112270,0xb9c20,0xcf440,0xefee0,0xe7ff0,0xe7ef0,
        0xe8d60,0xe8f30,0x115650,0x1ca8d0,0x148160,0xef630,0xefe30,
        0xc9a40,0x1781f0,0xbcd80,0xa99f0,0x14f5d0,0x204173}
def digest(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def encoded(v):return (json.dumps(v,sort_keys=True,indent=2)+'\n').encode()
def save(p,b):
 p.parent.mkdir(parents=True,exist_ok=True)
 if p.exists():assert p.read_bytes()==b,p
 else:p.write_bytes(b)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve()
 old=owning.GrenadeEffectTrace
 class BeginTrace(old):
  def packets(self,entry,registers,stack):
   result=super().packets(entry,registers,stack)
   if entry==0xb9c20:
    result.append(dict(field='ebx_marker_output12',pointer=f'{registers["ebx"]:08x}',size=12,raw_hex=self.raw(registers['ebx'],12)))
   return result
 owning.ENTRIES.update(DIRECT);owning.motion.ENTRIES.update(DIRECT);owning.motion.STATE_ENTRIES.update(DIRECT);owning.GrenadeEffectTrace=BeginTrace
 # The original owning composition executes unchanged. Only its observer class
 # and routine inventory grow; its original preparation/services stay explicit.
 saved=sys.argv
 try:
  sys.argv=[saved[0],'--output',str(out/'composition/report.json')];owning.main()
 finally:sys.argv=saved;owning.GrenadeEffectTrace=old
 r=json.loads((out/'composition/report.json').read_text());prior_path=ROOT/'analysis/multiplayer/integration/grenade-effect-pool/measured/report.json';prior=json.loads(prior_path.read_text())
 begins=[c for c in r['calls']if c['entry']=='000e95e0'];previous=[c for c in prior['calls']if c['entry']=='000e95e0'];assert len(begins)==len(previous)==1
 def state(report,index):return {k:v for k,v in report['states'][index].items()if k not in('id','sha256')}
 for c,pc in zip(begins,previous):
  assert c['packets']==pc['packets'];assert c['return_cpu']==pc['return_cpu'];assert state(r,c['before_state'])==state(prior,pc['before_state']);assert state(r,c['after_state'])==state(prior,pc['after_state'])
 # Direct caller return sites come from unchanged E95E0 CALL instructions.
 listing=(out/'composition/instructions/000e95e0.asm').read_text().splitlines();sites=set()
 for line in listing:
  words=line.split()
  if len(words)>=4 and words[2]=='call':sites.add(int(words[0],16)+len(bytes.fromhex(words[1])))
 body=[]
 for c in begins:
  services=[s for s in r['calls']if c['order']<s['order']<c['return_order']and int(s['caller'],16)in sites]
  body.append(dict(call=c,direct_services=services,before=state(r,c['before_state']),after=state(r,c['after_state'])))
 result=dict(schema='source-complete-grenade-begin-v1',tool_sha256=digest(__file__),owning_tool_sha256=digest(Path(owning.__file__)),owning_composition_report_sha256=digest(out/'composition/report.json'),prior_admitted_report_sha256=digest(prior_path),admitted_input_output_identical=True,body=body,new_guest_inputs=[],new_gameplay_returns=[],instruction_patches=[],boundaries=r['boundaries']+['Additional direct-owner observation only. The admitted before/after whole state, request packet and returned CPU reproduce the earlier effect-pool freeze exactly. All original lower owners and inherited platform/feedback/resource boundaries remain explicit.'])
 save(out/'report.json',encoded(result));print(json.dumps(dict(report=str(out/'report.json'),sha256=digest(out/'report.json'),direct_services=[s['entry']for s in body[0]['direct_services']])))
if __name__=='__main__':main()

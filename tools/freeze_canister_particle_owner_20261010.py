#!/usr/bin/env python3
"""Freeze the separate original canister effect admission/constructor stops."""
import hashlib,json,shutil
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
SCOPE=ROOT/'analysis/multiplayer/integration/canister-particle-owner-20261010'
OUT=SCOPE/'frozen'
OWN=['tools/oracle_canister_particle_owner_20261010.py','tools/freeze_canister_particle_owner_20261010.py','docs/MP_CANISTER_PARTICLE_OWNER_RESEARCH.md']
MODES=['empty','player','player-mask','decal-prefix']
def sha(p):
 h=hashlib.sha256()
 with p.open('rb')as f:
  for b in iter(lambda:f.read(1024*1024),b''):h.update(b)
 return h.hexdigest()
def encoded(v):return json.dumps(v,sort_keys=True,indent=2)+'\n'
def pools(r,glob):return next(p for p in r['final_effect_pools']if p['global_address']==glob)
def main():
 assert not OUT.exists()
 inputs={};cases=[];reports={}
 for name in MODES:
  a=SCOPE/'final-measured'/name;b=SCOPE/'final-repeated'/name
  files=['report.json']+[str(p.relative_to(a))for p in (a/'instructions').glob('*.asm')]
  for f in files:assert(a/f).read_bytes()==(b/f).read_bytes(),(name,f)
  r=json.loads((a/'report.json').read_text());reports[name]=r
  assert r['factory_correlates_byte_identically'] and r['tool_sha256']==sha(ROOT/OWN[0])
  assert all(not s['used_services']for s in r['stages'])
  inputs.update(r['protected_inputs'])
  cases.append(dict(name=name,repeat_identical_files=len(files),report_sha256=sha(a/'report.json'),roots=len(r['stages']),returns=sum(s['completed']for s in r['stages']),calls=len(r['calls']),stores=len(r['stores']),stop=r['stages'][-1]['invalid']))
 assert not pools(reports['empty'],'004f1f0c')['occupied']
 assert next(c for c in reports['empty']['calls']if c['entry']=='0011d650')['return_cpu']['eax']=='00000000'
 for name in ['player','player-mask']:
  r=reports[name];eff=pools(r,'004f1f0c')['occupied'];assert len(eff)==1 and eff[0]['datum']=='e6650000'
  assert next(c for c in r['calls']if c['entry']=='0011d650')['return_cpu']['eax']=='00000001'
  assert len(pools(r,'004f1f08')['occupied'])==1
  assert not pools(r,'00526514')['occupied']
 assert reports['player-mask']['stages'][-1]['stopped_eip']=='00180b12'
 assert int.from_bytes(bytes.fromhex(pools(reports['player-mask'],'004f1f0c')['occupied'][0]['raw_hex'])[0x5c:0x5e],'little')==1
 context=reports['player-mask']['stages'][-1]['contexts_after']['004f4850'];assert context['pointer']!='00000000'
 assert bytes.fromhex(context['raw_hex'])[:0x4201]==b'\xff'*0x4201
 r=reports['decal-prefix'];assert r['stages'][-1]['invalid']['arguments']==['00000000','0000000c','00101000','00000004']
 assert pools(r,'004f1f20')['capacity']==1024 and pools(r,'004f1f20')['stride']==64
 assert r['stages'][-1]['contexts_after']['004f1f1c']['pointer']=='82f09818'
 inputs.update({'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03':reports['empty']['source_xbe_sha256'],'assets/disc/maps/lockout.map':reports['empty']['cache_sha256'],'assets/disc/maps/shared.map':'084c817fa82bb9bc416d9ba847cec05be431abc419e3a1087cdf211fbf47f5af'})
 for p,h in inputs.items():assert sha(ROOT/p)==h,p
 prior_path=ROOT/'analysis/multiplayer/integration/particle-physics-movement-20261010/frozen/manifest.json'
 prior=json.loads(prior_path.read_text());prior_pins={}
 for bank in ('artifact_files','owned_live_files','protected_inputs'):prior_pins.update(prior[bank])
 for p,h in prior_pins.items():assert sha(ROOT/p)==h,p
 OUT.mkdir()
 for name in OWN:
  target=OUT/'source'/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/name,target)
 logs=OUT/'logs';logs.mkdir()
 for p in Path('/tmp').glob('canister-particle-owner-*.log'):shutil.copy2(p,logs/p.name)
 summary=dict(cases=cases,case_count=4,roots=sum(c['roots']for c in cases),returns=sum(c['returns']for c in cases),stops=4,calls=sum(c['calls']for c in cases),stores=sum(c['stores']for c in cases),repeat_files=sum(c['repeat_identical_files']for c in cases),new_source_services=0,particle_births=0)
 assert (summary['roots'],summary['returns'],summary['calls'],summary['stores'])==(39,35,80,21203)
 (OUT/'summary.json').write_text(encoded(summary))
 commands=dict(cwd=str(ROOT),environment={'PYTHONPATH':'tools'},commands=[json.loads((SCOPE/'final-measured'/n/'command.json').read_text())for n in MODES],independent_commands=[json.loads((SCOPE/'final-repeated'/n/'command.json').read_text())for n in MODES],scope='CPU original instructions only; no native runtime/GPU/whole effect admission. Recorded complete compatibility context and earlier explicit storage/resource/default-variant/clock delivery remain boundaries.')
 (OUT/'commands.json').write_text(encoded(commands))
 artifacts={str(p.relative_to(ROOT)):sha(p)for p in SCOPE.rglob('*')if p.is_file()}
 manifest=dict(schema='original-canister-effect-owner-freeze-v1',artifact_files=artifacts,owned_live_files={p:sha(ROOT/p)for p in OWN},protected_inputs=inputs,independently_reaudited_physics0_pins=prior_pins,summary=summary,bounds=['All four reports and196 instruction files repeat byte identically. Earlier exploratory observations/tool versions remain separate.','The selected source-created pool/world/player state is a bounded original root composition, not complete title startup, source local controller ownership or natural bullet damage.','The new extension supplies zero service results. Missing breakable/decal/VirtualAlloc owners retain distinct stopped original frames.','No native full-effect admission, canister particle birth, collider1/class8, continuous lifetime, platform semantics or GPU result is inferred.'])
 (OUT/'manifest.json').write_text(encoded(manifest))
 pins=artifacts|manifest['owned_live_files']|inputs|prior_pins
 mismatch=[p for p,h in pins.items()if sha(ROOT/p)!=h];assert not mismatch,mismatch
 (OUT/'audit.json').write_text(encoded(dict(unique_pins=len(pins),mismatches=mismatch,manifest_sha256=sha(OUT/'manifest.json'))))
 print(encoded(dict(manifest=str(OUT/'manifest.json'),sha256=sha(OUT/'manifest.json'),artifact_files=len(artifacts),unique_pins=len(pins),mismatches=0,summary=summary)))
if __name__=='__main__':main()

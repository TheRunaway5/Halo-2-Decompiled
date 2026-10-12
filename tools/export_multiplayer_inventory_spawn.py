#!/usr/bin/env python3
"""Read-only original constructor/initial-carry fields for live Match tests."""
import argparse, hashlib, json, struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'analysis/multiplayer/integration/inventory-source/native-retained-inventory'
PINS = {
 'analysis/multiplayer/integration/spawn-source/live-body-world/report.json':
 '2bf6fd373182b3db29d7409012f9ae566dcf2d9d496bbf15700184c4c8858951',
 'analysis/multiplayer/integration/spawn-source/player-visibility/report.json':
 'f58232e963b26bcb4a6471db8c40488f2bb195c4be5cf8e0617331ae4d5f78dd',
 'analysis/multiplayer/integration/inventory-source/equipped-pickup/report.json':
 'd6cec76de0a155f2c5817b61d8073d4555ddb70efa48e731b349fd8ff742c00b',
 'tools/oracle_mp_spawn_live_body_world.py':
 '5b2550b5f09ff214372c51e832407353e3f5da8e18713aca46d89855ee0c2586',
 'tools/oracle_mp_spawn_player_visibility.py':
 'aeafe79cff80e9d607e843d5bdb9f7c86b2814df15ec1b1179ebf4fe505e8572',
 'tools/oracle_mp_inventory_equipped_1_1.py':
 '47f56a9236ea2efe870d1ad82af16ace214f4598294f8c826966cced94dee655',
}

def sha(p): return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def save(p, data):
 p.parent.mkdir(parents=True, exist_ok=True)
 if p.exists(): assert p.read_bytes() == data, p
 else: p.write_bytes(data)
def fields(raw):
 return dict(inventory_sequence=struct.unpack_from('<H', raw, 0x210)[0],
  replicated_sequence=struct.unpack_from('<H', raw, 0x214)[0],
  current=list(struct.unpack_from('<bb', raw, 0x212)),
  requested=list(struct.unpack_from('<bb', raw, 0x216)),
  slots=[f'{v:08x}' for v in struct.unpack_from('<4I', raw, 0x218)],
  ready_ticks=list(struct.unpack_from('<4I', raw, 0x228)))

def main():
 p=argparse.ArgumentParser(description=__doc__); p.add_argument('--output-dir',type=Path,default=OUT)
 a=p.parse_args()
 for path, pin in PINS.items(): assert sha(ROOT/path)==pin,path
 body=json.loads((ROOT/next(iter(PINS))).read_text())
 player=json.loads((ROOT/'analysis/multiplayer/integration/spawn-source/player-visibility/report.json').read_text())
 equipped=json.loads((ROOT/'analysis/multiplayer/integration/inventory-source/equipped-pickup/report.json').read_text())
 rows=[]
 for index in (1,2):
  case=body['cases'][index]
  stages=[s for s in case['stages'] if s['entry']=='000b8a30']
  assert len(stages)==1 and stages[0]['completion']=='complete_original_return'
  obj=stages[0]['state']['objects'][0]; raw=bytes.fromhex(obj['raw_hex'])
  f=fields(raw)
  assert f['slots']==['ffffffff']*4
  rows.append(dict(kind='constructor',label=case['case'],source_path=f'/cases/{index}/stages/{case["stages"].index(stages[0])}/state/objects/0',
   source_report=next(iter(PINS)),source_unit=obj['datum'],unit_inventory_raw_hex=raw[0x210:0x238].hex(),
   source_weapon='ffffffff',definition='ffffffff',reserve=0,loaded=0,weapon_state=0,blocking_ticks=0,**f))
 for index in (1,2):
  case=player['cases'][index]
  stages=[s for s in case['stages'] if s['name']=='original_normal_player_spawn']
  assert len(stages)==1 and stages[0]['completion']=='complete_original_return'
  objects=stages[0]['world']['objects']; unit=objects[0]; raw=bytes.fromhex(unit['raw_hex']); f=fields(raw)
  weapons=[o for o in objects if o['datum']==f['slots'][0]]
  assert len(weapons)==1
  weapon=weapons[0]; w=bytes.fromhex(weapon['raw_hex'])
  rows.append(dict(kind='equipped',label=case['case'],source_path=f'/cases/{index}/stages/{case["stages"].index(stages[0])}/world/objects',
   source_report='analysis/multiplayer/integration/spawn-source/player-visibility/report.json',
   source_unit=unit['datum'],source_weapon=weapon['datum'],definition=weapon['definition'],
   unit_inventory_raw_hex=raw[0x210:0x238].hex(),weapon_magazine_raw_hex=w[0x224:0x234].hex(),
   weapon_state=struct.unpack_from('<I',w,0x178)[0],blocking_ticks=struct.unpack_from('<h',w,0x17c)[0],
   reserve=struct.unpack_from('<h',w,0x22a)[0],loaded=struct.unpack_from('<h',w,0x22c)[0],**f))
 # This separately frozen preallocated-unit/actual-weapon fixture must agree
 # in fields; its arena-dependent datums are intentionally not translated.
 correlation=[]
 for index,c in enumerate(equipped['cases']):
  initial=c['initial_inventory']
  for k in ('current','requested','ready_ticks','inventory_sequence','replicated_sequence'):
   assert initial[k]==rows[2][k],(index,k)
  assert initial['slots'][1:]==rows[2]['slots'][1:]
  correlation.append(dict(case=index,source_unit_scope='explicit preallocated unit; original allocated SMG',
   original_slots=initial['slots'],matching_fields=['current','requested','ready_ticks','inventory_sequence','replicated_sequence','empty_slots1..3']))
 columns=['kind','label','source_unit','source_weapon','definition','inventory_sequence','replicated_sequence','current0','current1','requested0','requested1',
          'slot0','slot1','slot2','slot3','timestamp0','timestamp1','timestamp2','timestamp3','loaded','reserve','weapon_state','blocking_ticks']
 lines=['# '+ '\t'.join(columns)]
 for r in rows:
  values=[r['kind'],r['label'],r['source_unit'],r['source_weapon'],r['definition'],r['inventory_sequence'],r['replicated_sequence'],
          *r['current'],*r['requested'],*r['slots'],*r['ready_ticks'],r['loaded'],r['reserve'],r['weapon_state'],r['blocking_ticks']]
  lines.append('\t'.join(map(str,values)))
 fixture=a.output_dir/'fixtures.tsv'; save(fixture,('\n'.join(lines)+'\n').encode())
 report=dict(schema=1,tool_sha256=sha(__file__),frozen_pins=PINS,rows=rows,equipped_correlation=correlation,
  fixture=dict(file='fixtures.tsv',sha256=sha(fixture),columns=columns),
  scope='Original frozen outputs only, no native-derived expectations. Full datum identity is retained in export; native arena/pool datums are checked by relation, not asserted equal to oracle addresses/salts. Constructor/initial carry fields only; no second carry, switch, action clock, full frame or physics parity.',
  source_clock_timestamp_owner=dict(old_instruction='000e9ecb',source='E9D50 reads original518444+8 and writes selected slot+228. Frozen initial clock is0; a later native spawn is checked against its delivered current tick, not an invented fixed duration.'),
  production_derived_expected_values=False)
 save(a.output_dir/'report.json',(json.dumps(report,sort_keys=True,indent=2)+'\n').encode())
 for path,pin in PINS.items(): assert sha(ROOT/path)==pin,path
 print(json.dumps(dict(rows=len(rows),fixture=str(fixture),sha256=sha(fixture),report_sha256=sha(a.output_dir/'report.json'))))

if __name__=='__main__': main()

#!/usr/bin/env python3
"""Export original GJK inputs/stores and fully composed narrowphase calls, with measured instruction estimates only."""
import argparse,json
from pathlib import Path
from oracle_mp_power_gjk_owners_1_1 import ROOT,verify,sha
from oracle_mp_item_current_producers_1_1 import encoded,frozen_write
from export_mp_physics_contact_lifecycle import add,segments
from export_mp_power_body_slots import full_snapshot
from oracle_smg_firing_1_1 import Xbe
SELECT={'002e14e0','002f4bd0','0030ae40','002e2320','003039c0','001f1da0','002e4320','0030eb30','00309900'}
SEMANTIC={0x3039d8,0x3039e5,0x3039f1,0x303ae5,0x303afc,0x303b40,
 0x2e235d,0x2e2360,0x1f1dce,0x2e433c,0x2e4348,
 0x30eb76,0x30ebb6,0x30ebea,0x30ebfe,0x30ec0a,0x30ec12,0x30ec1e,0x30ec2c,0x30ec42,
 0x30ecb3,0x30ecc2,0x30ece2,0x30ecfd,0x30ed45,0x30ed4c,0x30ed67,0x30ed71,0x30ed8a,0x30edaf,0x30edb8,
 0x309955,0x309a2e,0x309a3e,0x309a88,0x309a96,0x309a99,0x309aa5,0x309aa9,0x309ab9,0x309abf,
 0x303a10,0x303a17,0x303a1e,0x303a2b,0x303a4a,0x303a51,0x303a58,0x303a64,
 0x303a83,0x303a8a,0x303a95,0x303a9e,0x303abd,0x303ac4,0x303ad5,0x303ad8,
 0x303b1b,0x303b22,0x303b29,0x303b36}
from export_mp_power_child_agents import SEMANTIC as CHILD_SEMANTIC
SEMANTIC.update(CHILD_SEMANTIC)
from export_mp_power_narrowphase_owners import SEMANTIC as NARROW_SEMANTIC
SEMANTIC.update(NARROW_SEMANTIC)
SEMANTIC.update({3190798, 3190812, 3191324, 3192861, 3190819, 3193391, 3192370, 3190838, 3192379, 3191887, 3192400, 3190356, 3192407, 3192414, 3190372, 3192934, 3190381, 3190895, 3193456, 3190900, 3190389, 3191417, 3192447, 3192449, 3190914, 3191428, 3193482, 3191952, 3191982, 3193519, 3191472, 3193022, 3191490, 3193541, 3191496, 3193547, 3193555, 3191001, 3193561, 3193567, 3190521, 3193090, 3190531, 3191576, 3192088, 3191067, 3193629, 3190563, 3191075, 3193638, 3190574, 3192111, 3190578, 3191094, 3192120, 3193657, 3192638, 3191622, 3192647, 3193675, 3192665, 3193181, 3190621, 3192159, 3192669, 3192673, 3193185, 3191140, 3192164, 3193710, 3192689, 3192178, 3191158, 3192698, 3190651, 3191686, 3191690, 3191180, 3193743, 3193754, 3191196, 3193759, 3193764, 3191209, 3193779, 3190726, 3193811, 3190743, 3193306, 3191261, 3191263, 3190772, 3191802, 3190781})
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--report',type=Path,required=True);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args();verify()
 assert sha(a.report)in('13dca353f10d75b3e9211a220f177104c300301d0c368a969a4dd139d37954c2','3c65f9d03b1f5016ca9d5fdceb3a5178f8c902f6e169ff036d40da56ce84241c')
 r=json.loads(a.report.read_text());im=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');rows=[];audit=[]
 for c in r['calls']:
  if c['entry']not in SELECT:continue
  begin=c['order'];end=c['return_order']
  memory={};state=c['before_state']
  for p,b in full_snapshot(r['states'][state]):add(memory,p,b)
  for q in c['packets']:add(memory,int(q['pointer'],16),bytes.fromhex(q['before_hex']))
  current=dict(memory);writes=[];ignored=[]
  events=sorted([('R',e)for e in r['reads']if begin<e['order']<end]+[('W',e)for e in r['stores']if begin<e['order']<end],key=lambda x:x[1]['order'])
  for kind,e in events:
   p=int(e['address'],16);b=bytes.fromhex(e['raw_hex']if kind=='R'else e['before_hex'])
   for n,v in enumerate(b):
    if p+n not in current:memory[p+n]=v;current[p+n]=v
    assert current[p+n]==v,(c['id'],kind,e)
   if kind=='W':
    site=int(e['instruction'],16)
    (writes if not e['source_stack']or site in SEMANTIC else ignored).append(e)
    for n,v in enumerate(bytes.fromhex(e['written_hex'])):current[p+n]=v
  for q in c['packets']:
   if q['name']=='shape_40':
    vt=int.from_bytes(bytes.fromhex(q['before_hex'])[:4],'little');add(memory,vt+0x34,im.read(vt+0x34,4))
  after=dict(memory)
  for e in writes:
   p=int(e['address'],16);logical=bytes(after[p+n]for n in range(e['size']))
   assert e['source_stack']or logical==bytes.fromhex(e['before_hex']),(c['id'],e)
   for n,v in enumerate(bytes.fromhex(e['written_hex'])):after[p+n]=v
  matched=0
  for p,b in full_snapshot(r['states'][c['after_state']]):
   for n,v in enumerate(b):
    if p+n in after:assert after[p+n]==v,(c['id'],f'{p+n:08x}');matched+=1
  entry=c['entry'];eax=c['registers']['eax']
  rows.append(' '.join(['C',f'call_{c["id"]}_{entry}',entry,eax,c['registers']['ecx'],c['returned_eax'],c['entry_esp'],c['x87_control_word'],c['mxcsr'],c['registers']['edx'],c['registers']['esi'],c['registers']['edi']]+c['stack16']))
  estimates=[]
  for e in writes:
   if e['instruction']=='0030bb1d' and e['size']==8 and int(e['address'],16)%16==0:
    estimates.append(dict(input_word=e['written_hex'][:8],input_address=e['address'],input_order=e['order']))
   if e['instruction']=='0030bb26':
    assert estimates and 'output_word'not in estimates[-1]
    estimates[-1].update(output_word=e['written_hex'],output_address=e['address'],output_order=e['order'])
  for q in estimates:
   assert 'output_word'in q
   rows.append('Q '+bytes.fromhex(q['input_word'])[::-1].hex()+' '+bytes.fromhex(q['output_word'])[::-1].hex())
  for p,b in segments(memory):rows.append(f'M {p:08x} {b.hex()}')
  for p,b in segments(after):rows.append(f'A {p:08x} {b.hex()}')
  for e in writes:rows.append(f'{"WS"if e["source_stack"]else"W"} {e["instruction"]} {e["address"]} {e["before_hex"]} {e["written_hex"]}')
  audit.append(dict(original_rsqrtss_estimates=estimates,call_id=c['id'],entry=entry,input_bytes=len(memory),stores=len(writes),excluded_abi_stack_stores=len(ignored),retained_snapshot_correlated_bytes=matched,complete_original_call=True,complete_original_continuation=False,required_original_owner=None))
 fixture=a.output/'fixtures.tsv';frozen_write(fixture,('\n'.join(rows)+'\n').encode());frozen_write(a.output/'report.json',encoded(dict(schema=1,source_report_sha256=sha(a.report),exporter_sha256=sha(__file__),fixture_sha256=sha(fixture),cases=audit,native_expected_data_used=False,contact_callback_outputs_exported=False,prefix_expectations='Complete literal original call timelines/full retained buffers. Q records are actual original RSQRTSS input/output words at30BB1D/30BB26; no workspace/contact replay service or native-derived expected data.')))
 print(json.dumps(dict(cases=len(audit),complete=sum(q['complete_original_call']for q in audit),continuations=sum(q['complete_original_continuation']for q in audit),required_stops=sum(q['required_original_owner']is not None for q in audit),stores=sum(c['stores']for c in audit),fixture=sha(fixture))))
if __name__=='__main__':main()

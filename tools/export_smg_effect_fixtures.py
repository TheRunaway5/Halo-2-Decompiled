#!/usr/bin/env python3
"""Export literal original effect requests/visibility and declared lower boundaries."""
import argparse,json,struct
from pathlib import Path
from oracle_smg_effect_owner_1_1 import ROOT,sha,save,encoded,CACHE_PIN
from oracle_smg_firing_1_1 import Cache
def number(s):return int(s,16)
def word(raw,off,n=4):return int.from_bytes(bytes.fromhex(raw)[off:off+n],'little')
def packet(c,field,after=False):return next(v['raw_hex']for v in c['after_packets'if after else'packets']if v['field']==field)
def eax(c):return number(c['return_cpu']['registers']['eax'])
def export(sources,out):
 cache=ROOT/'assets/disc/maps/lockout.map';assert sha(cache)==CACHE_PIN
 cache_data=Cache(cache);data=cache_data.data;lines=[];requests=visibility=retirements=0;pins={}
 for source in sources:
  source=source.resolve()
  r=json.loads(source.read_text());pins[str(source.relative_to(ROOT))]=sha(source);states={v['id']:v for v in r['states']};calls=r['calls']
  for c in calls:
   if c['entry']=='00107b50':
    nested=[q for q in calls if c['order']<q['order']<c['return_order']]
    creates=[q for q in nested if q['entry']=='00177a10']
    if not creates:continue # Other groups/refresh branches are not made-up fixtures.
    assert len(creates)==1
    creator=creates[0];raw=packet(creator,'effect_creation60');directions=packet(creator,'creation_directions');i=c['effect_inputs'];st=states[c['before_state']]
    assert directions[:24]==directions[56:80]
    g,u=r['metadata']['source_direction_globals'];assert g['global_address']=='0046fc5c'and u['global_address']=='0046fc50'
    lines.append('D '+' '.join([source.parent.name+':'+str(c['id']),*[i[k]for k in('effect','weapon','previous','a_bits','b_bits')],f'{eax(c):08x}',raw,directions,directions[:24],g['raw12_hex'],u['raw12_hex'],f'{word(raw,0x1c):08x}','65666665']))
    for ob in st['objects']:
     tag=cache_data.tags[number(ob['definition'])];model=int.from_bytes(data[tag['offset']+0x38:tag['offset']+0x3c],'little')
     lines.append(f'O {ob["datum"]} {word(ob["header_hex"],2,1):02x} {model:08x} {ob["body_hex"]}')
    lines.append('E');requests+=1
   if c['entry']=='0017b1a0'and'return_order'in c:
    st=states[c['before_state']];nested=[q for q in calls if c['order']<q['order']<c['return_order']]
    special=next(q for q in nested if q['entry']=='0003eb10')
    flag=word(r['metadata']['effects'][0]['root']['raw_hex'],0)
    camera=st['camera']['rawc_hex'];override=bool(camera and word(camera,5,1))
    lines.append('V '+' '.join([source.parent.name+':'+str(c['id']),f'{flag:08x}',packet(c,'effect190'),packet(c,'effect190',True),str(eax(c)&255),str(eax(special)&255),str(int(override)),st['game']['local_pvs1178_hex'],st['game']['pvs1138_hex']]))
    retires=[q for q in nested if q['entry']=='00179080']
    assert len(retires)<=1
    if retires:
     q=retires[0];lines.append(f'R {packet(q,"effect190")} {packet(q,"effect190",True)} {eax(q)&255}');retirements+=1
    else:lines.append('R - - 0')
    lines.append('E');visibility+=1
 body=('\n'.join(lines)+'\n').encode();save(out/'fixtures.txt',body)
 report=dict(schema='source-smg-effect-native-fixtures-v1',exporter_sha256=sha(__file__),original_reports=pins,cache_sha256=CACHE_PIN,fixture_sha256=sha(out/'fixtures.txt'),requests=requests,visibility=visibility,retirement_callbacks=retirements,
  expected_data='Unchanged original call-entry/return packets and object/PVS/scalar fields only. Create/retire callbacks are literal independently measured original lower-service boundaries; neither native output nor invented flash/contact/pose is expected data.')
 save(out/'report.json',encoded(report));print(json.dumps(report))
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--source',type=Path,action='append',required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();export(a.source,a.output)

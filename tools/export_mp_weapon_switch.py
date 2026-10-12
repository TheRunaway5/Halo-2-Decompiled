#!/usr/bin/env python3
"""Source-only full-buffer export for bounded ordinary weapon switch owners."""
import argparse,hashlib,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
SOURCE=ROOT/'analysis/multiplayer/integration/inventory-source/weapon-switch-measured/admission-complete/report.json'
SOURCE_PIN='4d69b9dcbf5eaa13877e2d9433db3eeb41e6c2800db197cbfcab6b93694d7244'
TOOL_PIN='896f032db055cb2f7ac5dcd802be10e126c86154668fd38037c757e3a248331d'
OUT=ROOT/'analysis/multiplayer/integration/inventory-source/weapon-switch-native'
OWN={0xe9fe0,0xe9b40,0xea180,0xea270,0xea370,0xe9a90,0xe9d50}
SITES={0xea160,0xe9b06,0xe9b21,0xe9ec1,0xe9ecb,0xe9f2a,0xe9f30,0xe9f36,0xe9f42,0xe9f4a,0xe9f52}
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
def save(p,data):
 p.parent.mkdir(parents=True,exist_ok=True)
 if p.exists():assert p.read_bytes()==data,p
 else:p.write_bytes(data)
def buffers(s):
 r={}
 names={'unit_prefix':'U','primary_manager':'M','extension':'X','extra':'E','control':'C'}
 for x in s['regions']:r[names[x['kind']]]=x['hex']
 for x in s['weapons']:r['W'+x['datum']]=x['raw_hex']
 return r
def abi(c):
 e=int(c['entry'],16);reg={k:int(v,16)for k,v in c['registers'].items()};st=[int(x,16)for x in c['stack']]
 if e==0x112270:a=st[:1]
 elif e==0xe8610:a=[reg['eax'],reg['ecx']]
 elif e==0xceaf0:a=st[:2]
 elif e==0xe7ff0:a=[reg['ecx'],reg['edi']]
 elif e==0x115800:a=[reg['eax'],reg['edi']]
 elif e==0x115870:a=[reg['edi'],st[0]]
 elif e==0x1042e0:a=st[:2]
 elif e==0xc9a40:a=[reg['eax'],st[0]]
 elif e==0xefe30:a=[reg['ecx'],reg['eax']]
 elif e==0x115650:a=[st[0],reg['ecx'],*st[1:4]]
 elif e==0x105750:a=[reg['eax']]
 elif e==0x108230:a=st[:1]
 elif e==0xfb3f0:a=[reg['eax']]
 elif e==0x111490:a=[reg['eax'],*st[:3]]
 elif e==0x111500:a=[st[0],reg['ecx']&255,reg['eax'],st[1]]
 elif e==0xd1d20:a=st[:2]
 elif e==0xefee0:a=[reg['ecx'],st[0]]
 elif e==0x1040f0:a=[reg['edi'],st[0]&255,st[1]&255]
 elif e==0xe9c50:a=[st[0],st[1]&255]
 elif e==0x10a140:a=[reg['ebx'],st[0]]
 elif e==0x104500:a=[reg['edx']]
 else:raise ValueError(hex(e))
 return a
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output-dir',type=Path,default=OUT);a=p.parse_args()
 assert sha(SOURCE)==SOURCE_PIN
 assert sha(ROOT/'tools/oracle_mp_weapon_switch_measured_1_1.py')==TOOL_PIN
 r=json.loads(SOURCE.read_text());definitions={};rows=[];correlations=[]
 for case in r['cases']:
  for c in case['calls']:
   if 'duration_weapon_definition'in c:
    d=c['duration_weapon_definition'];key=f"{d['tag']['datum']:08x}" if isinstance(d['tag']['datum'],int) else d['tag']['datum']
    if key in definitions:assert definitions[key]==d['raw360_hex']
    definitions[key]=d['raw360_hex']
 for key,raw in sorted(definitions.items()):rows.append(f'D {key} {raw}')
 for case in r['cases']:
  calls=sorted(case['calls'],key=lambda q:q['order']);parents={};stack=[]
  for c in calls:
   while stack and stack[-1]['return_order']<c['order']:stack.pop()
   if stack:assert c['return_order']<stack[-1]['return_order'];parents[c['order']]=stack[-1]['order']
   stack.append(c)
  index={q['order']:q for q in calls}
  for root in calls:
   entry=int(root['entry'],16)
   if entry not in OWN:continue
   native={root['order']};external=[]
   for c in calls:
    if parents.get(c['order'])in native:
     if int(c['entry'],16)in OWN:native.add(c['order'])
     else:external.append(c)
   unit=int(root['stack'][0],16)
   primary=(int(root['registers']['eax'],16)&255)!=0
   flags=[0,0];mode=0;request='-'
   if entry==0xe9fe0:request=root['request32_hex'];primary=int.from_bytes(bytes.fromhex(request)[:4],'little')==8
   elif entry==0xe9b40:
    primary=(int(root['registers']['ebx'],16)&255)!=0;flags=[int(x,16)&255 for x in root['stack'][1:3]]
   elif entry in (0xea180,0xea270,0xea370):mode=int(root['stack'][1],16);primary=mode==8
   elif entry==0xe9d50:mode=int(root['stack'][1],16);flags=[int(x,16)&255 for x in root['stack'][2:4]]
   rows.append(' '.join(['C',str(case['case']),str(root['order']),root['entry'],f'{unit:08x}',str(int(primary)),*[str(x)for x in flags],f'{mode:08x}',f"{root['before']['tick']:08x}",str(int(root['return_eax'],16)&255),request]))
   for key,raw in sorted(buffers(root['before']).items()):rows.append(f'B {key} {raw}')
   for key,raw in sorted(buffers(root['after']).items()):rows.append(f'F {key} {raw}')
   for c in external:
    args=abi(c);assert int(c['entry'],16)not in OWN
    rows.append(' '.join(['K',c['entry'],str(len(args)),*[f'{v:08x}'for v in args],c['return_eax']]))
    for key,raw in sorted(buffers(c['before']).items()):rows.append(f'I {key} {raw}')
    for key,raw in sorted(buffers(c['after']).items()):rows.append(f'O {key} {raw}')
   stores=[]
   for s in case['stores']:
    if not root['order']<s['order']<root['return_order']:continue
    if any(c['order']<s['order']<c['return_order']for c in external):continue
    assert int(s['instruction'],16)in SITES,s
    key={'unit_prefix':'U','control':'C'}.get(s['kind'],'W'+s.get('datum',''))
    assert key in buffers(root['before'])
    rows.append(' '.join(['W',s['instruction'],key,s['offset'],str(s['bytes']),s['before_hex'],s['after_hex']]))
    stores.append(s)
   correlations.append(dict(case=case['case'],order=root['order'],entry=root['entry'],native_descendant_orders=sorted(native),external_leaf_orders=[c['order']for c in external],stores=len(stores)))
 fixture=a.output_dir/'fixtures.tsv';save(fixture,('\n'.join(rows)+'\n').encode())
 report=dict(schema=1,tool_sha256=sha(__file__),source_report_sha256=SOURCE_PIN,source_tool_sha256=TOOL_PIN,
  fixture_sha256=sha(fixture),definitions=list(definitions),cases=correlations,
  expected_authority='Literal original captured buffers, original leaf arguments/returns and original ordered owning stores only. No native execution or recomputed expected pose/clock/contact.',
  boundary='Each callback entry AND exit buffer is compared. Recorded leaf results are supplied ONLY by the bounded native differential; the public runtime API requires real owning services and never supplies success/admission/ticks by default.')
 save(a.output_dir/'report.json',(json.dumps(report,sort_keys=True,indent=2)+'\n').encode())
 print(json.dumps(dict(fixture=str(fixture),sha256=sha(fixture),cases=len(correlations),leaves=sum(len(c['external_leaf_orders'])for c in correlations),stores=sum(c['stores']for c in correlations))))
if __name__=='__main__':main()

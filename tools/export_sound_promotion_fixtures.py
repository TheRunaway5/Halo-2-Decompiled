#!/usr/bin/env python3
"""Export only untouched original promotion/clock outputs and literal backing."""
import argparse,hashlib,json
from pathlib import Path
from oracle_mp_title_update_display import ROOT,sha,save,encoded

def export(report_path,providers_path,out):
    report=json.loads(report_path.read_text());providers=json.loads(providers_path.read_text())
    constants=[next(w for w in providers['words']if w['old_va']==a)['old_hex']for a in('0046513c','004650e0','00465030')]
    constants=[int.from_bytes(bytes.fromhex(s),'little')for s in constants]
    lines=['SOUND_PROMOTION_NATIVE_V1','K\t'+'\t'.join(f'{x:08x}'for x in constants)]
    calls=writes=suppressed=selected=none=0;maps=[]
    for m in report['maps']:
        state=m['state'];lines.append('\t'.join(('M',m['image'],m['map']['path'],state['system'],state['tag_table'],state['current_ugh'],state['options'])))
        regions=[dict(address='0a000000',raw_hex=m['initialization']['arena_hex']),dict(address='0a400000',raw_hex='00000000')]+m['regions']
        mutable=[(int(r['address'],16),bytearray.fromhex(r['raw_hex']))for r in regions]
        for r in regions:lines.append('\t'.join(('R',r['address'],r['raw_hex'])))
        def apply(s):
            at=int(s['address'],16);size=s['bytes'];matches=[(a,b)for a,b in mutable if a<=at and at+size<=a+len(b)];assert len(matches)==1
            a,b=matches[0];before=int.from_bytes(b[at-a:at-a+size],'little');assert before==int(s['before'],16),(m['map'],s,hex(before))
            b[at-a:at-a+size]=int(s['after'],16).to_bytes(size,'little')
        for q in m['cases']:
            lines.append('\t'.join(('C',q['kind'],q.get('sound','00000000'),f'{q.get("tick",0):08x}',q.get('return_eax','00000000'),q.get('selector','00000000'))))
            for s in q['stores']:
                apply(s);lines.append('\t'.join(('S',s['instruction'],s['address'],str(s['bytes']),s['before'],s['after'])))
            lines.append('E');calls+=1;writes+=len(q['stores'])
            suppressed+=q.get('return_eax')=='00000001';selected+=q.get('kind')=='promotion'and q['selector']!='ffffffff';none+=q.get('selector')=='ffffffff'
        assert mutable[0][1].hex()==m['final_arena_hex']
        assert next(b.hex()for a,b in mutable if a==int(state['current_ugh'],16))==m['final_ugh_hex']
        lines.append('ENDMAP');maps.append(dict(image=m['image'],path=m['map']['path'],sha256=m['map']['sha256'],rows=m['promotion_rows'],sounds_with_promotion=len(m['promoted_sounds']),region_bytes=sum(len(b)for a,b in mutable),final_region_sha256=[dict(address=f'{a:08x}',sha256=hashlib.sha256(b).hexdigest())for a,b in mutable]))
    save(out/'fixtures.tsv',('\n'.join(lines)+'\n').encode())
    save(out/'fixtures.json',encoded(dict(schema='original-sound-promotion-native-fixtures-v1',exporter_sha256=sha(__file__),original_report=dict(path=str(report_path),sha256=sha(report_path)),providers=dict(path=str(providers_path),sha256=sha(providers_path)),fixture_sha256=sha(out/'fixtures.tsv'),calls=calls,ordered_writes=writes,suppressed=suppressed,selectors=selected,no_promotion=none,maps=maps,expected_values_from_native=False,whole_map_or_voice_admission=False)))
    print(json.dumps(dict(calls=calls,writes=writes,suppressed=suppressed,selectors=selected,no_promotion=none,fixture_sha256=sha(out/'fixtures.tsv'))))
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--providers',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();export(a.report,a.providers,a.output)

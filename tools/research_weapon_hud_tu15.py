#!/usr/bin/env python3
"""Complete original weapon-to-HUD producer and scalar consumer mapping.

Owned TU1.1/TU1.5 complete body/branch/table evidence; retail source consumers
are located by unique full-body correspondence before being used. This is
static code evidence, not a rendered Xbox HUD or complete object-world run.
"""
import argparse,csv,json,struct
from pathlib import Path
import research_weapon_effect_u5 as correspondence
from weapon_reference_versions import images

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 source={label:image for label,image,_ in images()};disc=next(image for label,image,_ in images() if label not in ('TU1.1','TU1.5'))
 old=source['TU1.1'];base_code=correspondence._Code(disc,'.text');old_code=correspondence._Code(old,'.text')
 starts={int(row['address'],16) for row in csv.DictReader((correspondence.ROOT/'analysis/map-pack-update/functions.tsv').open(),delimiter='\t')};buckets={}
 for entry in starts:buckets.setdefault(tuple(correspondence.prefix(old_code,old,entry)),[]).append(entry)
 base_rows=[];entries=[0x1046d0,0x104aa0,0x104cf0,0x13c719]
 for entry in (0x13ad48,0x13a050,0x13ac87):
  f=correspondence._trace(base_code,entry,set());before=[base_code.at(x) for x in sorted(f.code)];positions={x.address:n for n,x in enumerate(before)};expected=[correspondence.norm(x,disc,positions) for x in before]
  candidates=sorted(set(buckets.get(tuple(correspondence.prefix(base_code,disc,entry)),[])+correspondence.raw_prefix_candidates(base_code,disc,old_code,entry)));matches=[]
  for target in candidates:
   g=correspondence._trace(old_code,target,starts);after=[old_code.at(x) for x in sorted(g.code)];mapping={x.address:n for n,x in enumerate(after)}
   if [correspondence.norm(x,old,mapping) for x in after]==expected:matches.append((target,after))
  assert len(matches)==1,(hex(entry),[hex(m[0]) for m in matches])
  target,after=matches[0];entries.append(target);base_rows.append(dict(retail_entry=f'{entry:08x}',tu11_entry=f'{target:08x}',rows=len(before),unique_complete_body_equal=True))
  for label,rows in (('retail',before),('old',after)):
   path=a.output/'retail-consumers'/f'{entry:08x}-{label}.asm';path.parent.mkdir(parents=True,exist_ok=True);path.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n' for i in rows))
 correspondence.OWNERS=tuple(entries);correspondence.BOUNDARIES=();correspondence.run(a.output)
 path=a.output/'report.json';report=json.loads(path.read_text());mapping={r['old']:r['u5'] for r in report['owners']}
 for row in base_rows:row['tu15_entry']=mapping[row['tu11_entry']]
 report.update(schema='source-weapon-hud-tu15-correspondence-v1',scope=__doc__,retail_consumers=base_rows,retail_xbe_sha256=correspondence.sha(disc.path) if hasattr(disc,'path') else __import__('hashlib').sha256(disc.data).hexdigest(),correspondence_tool_sha256=report['tool_sha256'],tool_sha256=correspondence.sha(__file__))
 path.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
if __name__=='__main__':main()

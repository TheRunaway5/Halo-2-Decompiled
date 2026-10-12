#!/usr/bin/env python3
"""Export literal complete wrapper states and explicit lower callback boundaries."""
import argparse,hashlib,json,struct
from pathlib import Path

def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();r=json.loads(a.report.read_text());calls=r['calls'];m=r['metadata'];lines=['# original-object-type-construction-v1','# report '+sha(a.report),'BANK '+m['bank']]
 for address,raw in m['ranges'].items():lines.append('MEM '+address+' '+raw)
 groups={q['signed_index']:q['tag_group']for q in calls if q['entry']=='0010ca30'}
 for index,group in sorted(groups.items()):lines.append(f'GROUP {index&65535:x} {group}')
 roots=[q for q in calls if not q['ancestors']];count=callbacks=whole=0
 for q in roots:
  entry=int(q['entry'],16);assert entry in (0x10ca30,0x10cbd0,0x10cc20)
  lines.append(f'CASE {q["id"]} {entry:x} {q.get("unit",4294967295):x} {int(q.get("definition","ffffffff"),16):x} {int(q["return_eax"],16):x}')
  for name,raw in q['before'].items():lines.append('BEFORE '+name+' '+raw)
  children=[v for v in calls if v['ancestors']==[q['id']]and int(v['entry'],16)!=0x10ca30]
  for v in children:
   lines.append(f'CALL {v["entry"]} {int(v["return_eax"],16)&255:x}')
   for name,raw in v['before'].items():lines.append('CALLBEFORE '+name+' '+raw)
   for name,raw in v['after'].items():lines.append('CALLAFTER '+name+' '+raw)
   lines.append('ENDCALL');callbacks+=1
  for v in q.get('reads',[]):
   address=int(v['address'],16);size=v['bytes']
   if any(int(p,16)<=address and address+size<=int(p,16)+len(raw)//2 for p,raw in m['ranges'].items()):lines.append(f'READ {address:x} {v["raw_hex"]}')
  for name,raw in q['after'].items():lines.append('AFTER '+name+' '+raw);whole+=len(raw)//2
  lines.append('ENDCASE');count+=1
 data=('\n'.join(lines)+'\n').encode();a.output.parent.mkdir(parents=True,exist_ok=True)
 if a.output.exists():assert a.output.read_bytes()==data
 else:a.output.write_bytes(data)
 print(json.dumps(dict(cases=count,callbacks=callbacks,compared_whole_output_bytes=whole,sha256=sha(a.output))))
if __name__=='__main__':main()

#!/usr/bin/env python3
"""Export complete original request/resource buffers and ordered stores."""
import argparse,hashlib,json,struct
from pathlib import Path

def words(s):return list(struct.unpack('<'+'I'*(len(s)//8),bytes.fromhex(s)))
def nums(v):return' '.join(f'{x:x}'for x in v)
def save(p,b):
 p.parent.mkdir(parents=True,exist_ok=True)
 if p.exists():assert p.read_bytes()==b,'Immutable fixture differs'
 else:p.write_bytes(b)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('report',type=Path);p.add_argument('output',type=Path);a=p.parse_args();r=json.loads(a.report.read_text());lines=['# literal-animation-resolution-v1','# source-report-sha256 '+hashlib.sha256(a.report.read_bytes()).hexdigest()]
 for g in r['graphs']:
  lines+=['GRAPH '+g['datum']+' '+g['pointer'],'MEM '+g['pointer']+' '+g['complete_payload_hex']]
 count=stores=demands=0
 for c in r['selection_calls']:
  e=int(c['entry'],16)
  if e not in(0x1cfde0,0x1deb60):continue
  regs=c['registers'];s=[int(v,16)for v in c['stack']];out=c['outputs_before'];after=c['outputs_after']
  if e==0x1cfde0:
   args=s[:5];before=[out[k]for k in('requested','resolved','packed')];end=[after[k]for k in('requested','resolved','packed')];locations={k:(c['output_locations'][k][0],i,0,c['output_locations'][k][1])for i,k in enumerate(('requested','resolved','packed'))}
  else:
   args=[s[0],s[2],s[3],s[4],regs['eax'],sum((1<<i)for i,k in enumerate(('mode','weapon_class','weapon'))if k in out)]
   before=['00'*16,''.join(out.get(k,'00000000')for k in('mode','weapon_class','weapon'))+'00000000',out['packed']];end=[before[0],''.join(after.get(k,'00000000')for k in('mode','weapon_class','weapon'))+'00000000',after['packed']]
   locations={'packed':(s[1],2,0,4)}
   for i,k in enumerate(('mode','weapon_class','weapon')):
    if k in out:locations[k]=(c['output_locations'][k][0],1,i*4,4)
  lines.append('CASE '+nums([c['id'],e,int(c['return_eax'],16)&255,len(args)]+args));lines.append('MANAGER '+c['before_manager_hex'])
  for k,b,endb in zip(('requested','resolved','packed'),before,end):lines+=['BEFORE '+k+' '+b,'AFTER '+k+' '+endb]
  expected=[x for x in r['resource_calls']if c['id']in x['selection_ancestors']]
  gates={x['gates_before']for x in expected};assert len(gates)<=1;lines.append('GATES '+(next(iter(gates))if gates else'0000'))
  for d in expected:
   de=int(d['entry'],16)
   if de==0x1e13c0:lines.append('STATE')
   if de==0x124e40:lines.append('DEMAND '+nums([0x10000+d['id'],int(d['descriptor'],16),d['registers']['ecx']&255]));demands+=1
  for st in r['selection_stores']:
   if c['id']not in st['calls']:continue
   p=int(st['address'],16);n=st['size']
   for at,field,offset,extent in locations.values():
    if at<=p and p+n<=at+extent:
     lines.append('STORE '+nums([int(st['instruction'],16),field,offset+p-at,n,int.from_bytes(bytes.fromhex(st['before']),'little'),int.from_bytes(bytes.fromhex(st['written']),'little')]));stores+=1;break
  lines.append('END');count+=1
 for c in r['resource_calls']:
  if c['entry']not in('001e13c0','00124e40'):continue
  e=int(c['entry'],16);reg=c['registers'];args=[reg['edx'],int(c['stack'][0],16)]if e==0x1e13c0 else[reg['eax'],reg['ecx']&255]
  lines.append('CASE '+nums([0x10000+c['id'],e,0,len(args)]+args));lines.append('GATES '+c['gates_before'])
  if e==0x1e13c0:
   lines.append('STATE')
   for d in r['resource_calls']:
    if c['id']in d['resource_ancestors']and d['entry']=='00124e40':lines.append('DEMAND '+nums([0x10000+d['id'],int(d['descriptor'],16),d['registers']['ecx']&255]));demands+=1
  if e==0x124e40:
   for name,kind in(('runtime_before','RBEFORE'),('runtime_after','RAFTER')):
    for at,raw in c.get(name,{}).items():lines.append(kind+' '+at+' '+raw)
   for d in r['resource_calls']:
    if c['id']not in d['resource_ancestors']:continue
    if d['entry']=='00124dd0':lines.append('QUEUED_RESULT '+d['return_eax'])
    if d['entry']=='0016d2c0':
     lines.append('ALLOC '+d['return_eax']+' '+d['allocation_pool_return'])
     for name,kind in(('runtime_before','ABEF'),('runtime_after','AAFT')):
      for at,raw in d[name].items():lines.append(kind+' '+at+' '+raw)
    if d['entry']=='00125120':
     lines.append('SCHEDULE')
     for name,kind in(('runtime_before','SBEF'),('runtime_after','SAFT')):
      for at,raw in d[name].items():lines.append(kind+' '+at+' '+raw)
   for st in r['resource_stores']:
    if c['id']in st['resource_calls']and int(st['instruction'],16)in(0x124e8b,0x124e8f,0x124eb0,0x124ee9):lines.append('RSTORE '+nums([int(st['instruction'],16),int(st['address'],16),int.from_bytes(bytes.fromhex(st['before']),'little'),int.from_bytes(bytes.fromhex(st['written']),'little')]))

  if e==0x124e40:lines+=['DESCRIPTOR_BEFORE '+c['descriptor_before'],'DESCRIPTOR_AFTER '+c['descriptor_after']]
  lines.append('END');count+=1
 save(a.output,('\n'.join(lines)+'\n').encode());print(json.dumps(dict(cases=count,output_stores=stores,ordered_demands=demands,sha256=hashlib.sha256(a.output.read_bytes()).hexdigest())))
if __name__=='__main__':main()

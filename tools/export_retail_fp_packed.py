#!/usr/bin/env python3
"""Export independent packed-model fixtures from retained original GPU buffers.

Inputs are read-only RenderDoc exports from original_capture_8.rdc, not native
model vertices. SNORM input boundary and packed normal behavior are separately
observed in packed-inputs.json DebugVertex traces. UVs are actual post-VS outputs.
"""
from pathlib import Path
import argparse,struct,json,hashlib
ROOT=Path(__file__).resolve().parents[1]
def f32(x):return struct.unpack('<f',struct.pack('<f',x))[0]
def bits(x):return struct.unpack('<I',struct.pack('<f',x))[0]
def snorm(x):return max(-1.0,f32(x/32767.0))
def normal(p):
 out=[]
 for shift,n in [(0,11),(11,11),(22,10)]:
  x=(p>>shift)&((1<<n)-1);x=x-(1<<n)if x&(1<<(n-1))else x
  out.append(f32(x/((1<<(n-1))-1)))
 return out
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--capture-exports',type=Path,default=ROOT/'analysis/multiplayer-shading/original-xemu');p.add_argument('--output',type=Path,default=ROOT/'tests/fixtures/retail_fp_packed.tsv');args=p.parse_args();base=args.capture_exports;audit=base/'fp-output-audit'
records=json.loads((audit/'packed-inputs.json').read_text());rows={};sources={};tracechecks=0
for event,kind,section in [(848,0,0),(856,0,0),(871,0,0),(887,1,0),(879,1,1)]:
 r=next(r for r in records if r['event']==event);uniform=(base/f'fp-{event}-0-0.bin').read_bytes();constants=struct.unpack_from('<12f',uniform,170*16);post=(base/f'fp-{event}-postvs.bin').read_bytes();indices=struct.unpack('<'+'I'*((base/f'fp-{event}-indices.bin').stat().st_size//4),(base/f'fp-{event}-indices.bin').read_bytes());first=min(indices);shortattrs=[x for x in r['attributes']if x['format_name'].endswith('_SNORM')];packedattrs=[x for x in r['attributes']if x['name'].endswith('_cmp')]
 def get(attr,i,fmt):
  b=next(b for b in r['buffers']if b['slot']==attr['slot']);raw=(audit/b['file']).read_bytes();return struct.unpack_from(fmt,raw,i*b['stride']+attr['offset'])
 for i in sorted(set(indices)):
  rawpos=get(shortattrs[0],i,'<3h');rawuv=get(shortattrs[1],i,'<2h');packed=[get(a,i,'<I')[0]for a in packedattrs];position=[f32(f32(snorm(rawpos[k])*constants[k])+constants[k+4])for k in range(3)];uv=struct.unpack_from('<2f',post,(i-first)*240+144);uvexpected=[f32(f32(snorm(rawuv[k])*constants[k+8])+constants[k+10])for k in range(2)]
  assert [bits(x)for x in uv]==[bits(x)for x in uvexpected],(event,i,uv,uvexpected)
  key=(kind,section,i);row=(*position,*uv,*(v for p in packed for v in normal(p)))
  if key in rows:assert rows[key]==row
  rows[key]=row
 for b in r['buffers']:
  f=audit/b['file'];sources[str(f.relative_to(base))]=hashlib.sha256(f.read_bytes()).hexdigest()
 for f in [base/f'fp-{event}-0-0.bin',base/f'fp-{event}-postvs.bin',base/f'fp-{event}-indices.bin']:
  sources[str(f.relative_to(base))]=hashlib.sha256(f.read_bytes()).hexdigest()
 # Source paint traces retain exactly the local decoded position and all three
 # NORMPACKED outputs, including actual drawn negative endpoint vertices.
 if event==848:
  for d in r['debug']:
   i=d['index'];row=rows[kind,section,i]
   assert [bits(x)for x in d['final']['_466']['value']]==[bits(x)for x in row[:3]]
   for n,name in enumerate(['_378','_383','_388']):assert [bits(x)for x in d['final'][name]['value'][:3]]==[bits(x)for x in row[5+n*3:8+n*3]]
   tracechecks+=1
args.output.parent.mkdir(parents=True,exist_ok=True)
args.output.write_text('# Original Xbox FP model inputs: kind section source_vertex then position3 UV2 normal3 tangent3 binormal3 float bits.\n# Position/NORMPACKED values derive from captured raw buffers and actual source VP operations; UV2 is captured post-VS.\n'+''.join(' '.join(map(str,key))+' '+' '.join(f'{bits(x):08X}'for x in row)+'\n'for key,row in sorted(rows.items())))
report=dict(unique_vertices=len(rows),expanded_vertices=10344,actual_debugged_paint_vertices=tracechecks,postvs_uv_component_checks=2*len(rows),fixture_sha256=hashlib.sha256(args.output.read_bytes()).hexdigest(),original_capture_sha256=hashlib.sha256((base/'original_capture_8.rdc').read_bytes()).hexdigest(),source_exports=sources,boundary='Original xemu GPU behavior, source1C330/35790 declaration, source1CDD0 constants. Full real Xbox hardware parity not claimed.')
(audit/'packed-fixture-provenance.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k!='source_exports'},indent=2))

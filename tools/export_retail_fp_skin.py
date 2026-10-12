#!/usr/bin/env python3
"""Export source FP weights/bones and hand world basis from original GPU exports.

The first influence is the reverse-order complement emitted by the actual
compiled source shader classes (one through four). The other weights are
captured UBYTE_NORM inputs. Global bones derive from authored node maps, not
native SkinBinding. Hand basis is actual original environment post-VS output.
"""
from pathlib import Path
import argparse,struct,json,hashlib,sys
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
from audit_rendering import Cache
f32=lambda x:struct.unpack('<f',struct.pack('<f',x))[0]
bits=lambda x:struct.unpack('<I',struct.pack('<f',x))[0]
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--capture-exports',type=Path,default=ROOT/'analysis/multiplayer-shading/original-xemu');args=p.parse_args();base=args.capture_exports;audit=base/'fp-output-audit'
c=Cache(ROOT/'assets/disc/maps/cyclotron.map');records=json.loads((audit/'packed-inputs.json').read_text());local={}
for line in (ROOT/'tests/fixtures/retail_fp_packed.tsv').read_text().splitlines():
 if not line or line[0]=='#':continue
 s=line.split();local[tuple(map(int,s[:3]))]=[struct.unpack('<f',struct.pack('<I',int(x,16)))[0]for x in s[3:]]
weights={};world={};sources={};checks=0
for event,kind,section,mode,env in [(848,0,0,0xe6b70542,None),(856,0,0,0xe6b70542,None),(871,0,0,0xe6b70542,None),(887,1,0,0xeb1a0995,903),(879,1,1,0xeb1a0995,895)]:
 r=next(x for x in records if x['event']==event);_,p=c.tag(mode,'mode');s=c.block(p+0x24,0x5c)[section];ptr=c.u32(s+0x38);size=c.u32(s+0x3c);header=c.u32(s+0x44);assert ptr>>30==2
 with (ROOT/'assets/disc/maps/shared.map').open('rb')as f:f.seek(ptr&0x3fffffff);raw=f.read(size)
 nodemap_resource=next(q for q in c.block(s+0x48,16)if c.u16(q+4)==100);start=size-header-4+c.u32(nodemap_resource+12);nodemap=raw[start:start+c.u32(nodemap_resource+8)]
 constants=struct.unpack('<'+'f'*((base/f'fp-{event}-0-0.bin').stat().st_size//4),(base/f'fp-{event}-0-0.bin').read_bytes());idx=struct.unpack('<'+'I'*((base/f'fp-{event}-indices.bin').stat().st_size//4),(base/f'fp-{event}-indices.bin').read_bytes());attr=next(x for x in r['attributes']if x['name']=='v1');n=attr['format']['compCount'];bonebuffer=next(b for b in r['buffers']if b['slot']==attr['slot']);bonebytes=(audit/bonebuffer['file']).read_bytes()
 if kind:
  wattr=next(x for x in r['attributes']if x['name']=='v2');wbuf=next(b for b in r['buffers']if b['slot']==wattr['slot']);wbytes=(audit/wbuf['file']).read_bytes();post=(base/f'fp-{env}-postvs.bin').read_bytes();ei=struct.unpack('<'+'I'*((base/f'fp-{env}-indices.bin').stat().st_size//4),(base/f'fp-{env}-indices.bin').read_bytes());assert sorted(set(ei))==sorted(set(idx));first=min(ei)
 for i in sorted(set(idx)):
  key=(kind,section,i);bones=list(bonebytes[i*bonebuffer['stride']+attr['offset']:i*bonebuffer['stride']+attr['offset']+n]);globalbones=[nodemap[b]for b in bones];w=[1.0]
  if kind:
   w=[f32(x/255.0)for x in wbytes[i*wbuf['stride']+wattr['offset']:i*wbuf['stride']+wattr['offset']+n]];w[0]=1.0
   for j in range(n-1,0,-1):w[0]=f32(w[0]-w[j])
  row=(n,*globalbones,*([0]*(4-n)),*w,*([0.0]*(4-n)))
  if key in weights:assert weights[key]==row
  weights[key]=row
  if kind:
   address=[int(f32(f32(b/255.0)*constants[7*4+3]))for b in bones];assert address==[b*3 for b in bones]
   mat=[]
   for j in range(12):
    v=f32(w[-1]*constants[(50+address[-1])*4+j])
    for k in range(n-2,-1,-1):v=f32(v+f32(w[k]*constants[(50+address[k])*4+j]))
    mat.append(v)
   loc=local[key];pos=[]
   for axis in range(3):
    v=f32(f32(mat[axis*4]*loc[0])+f32(mat[axis*4+1]*loc[1]));v=f32(v+f32(mat[axis*4+2]*loc[2]));pos.append(f32(v+mat[axis*4+3]))
   tex=[struct.unpack_from('<3f',post,(i-first)*240+off)for off in (128,144,160)]
   # Source oT1/2/3 contain world binormal/tangent/normal rows.
   basis=[tex[axis][2]for axis in range(3)]+[tex[axis][1]for axis in range(3)]+[tex[axis][0]for axis in range(3)]
   world[key]=(*pos,*basis)
   if i in (311 if section==0 else 91,):
    d=next(x for x in r['debug']if x['index']==i);names=['_607','_624','_641']if section==0 else['_538','_555','_572']
    for j,name in enumerate(names):assert [bits(v)for v in d['final'][name]['value']]==[bits(v)for v in mat[j*4:j*4+4]]
    if section==0:
     assert [bits(d['final'][name]['value'][0])for name in ('_650','_661','_672')]==[bits(v)for v in pos]
    checks+=1
 for b in r['buffers']:
  f=audit/b['file'];sources[str(f.relative_to(base))]=hashlib.sha256(f.read_bytes()).hexdigest()
 for f in [base/f'fp-{event}-0-0.bin',base/f'fp-{event}-indices.bin']+([base/f'fp-{env}-postvs.bin',base/f'fp-{env}-indices.bin']if kind else[]):sources[str(f.relative_to(base))]=hashlib.sha256(f.read_bytes()).hexdigest()
wpath=ROOT/'tests/fixtures/retail_fp_skin_weights.tsv';wpath.write_text('# Original source kind section vertex active_count global_bones4 then effective_weights4 float bits.\n# First weight is the source compiled VP reverse complement; the stored first byte is ignored.\n'+''.join(' '.join(map(str,k))+' '+' '.join(map(str,v[:5]))+' '+' '.join(f'{bits(x):08X}'for x in v[5:])+'\n'for k,v in sorted(weights.items())))
bpath=ROOT/'tests/fixtures/retail_fp_world_basis.tsv';bpath.write_text('# Original kind section vertex then world_position3 normal3 tangent3 binormal3 float bits.\n# Basis is actual original env post-VS output; position is original palette/weight DPH, debugged at worst-case vertices.\n'+''.join(' '.join(map(str,k))+' '+' '.join(f'{bits(x):08X}'for x in v)+'\n'for k,v in sorted(world.items())))
# Interpret the actual ADD instructions from every compiled influence variant.
# This independently covers the source four-influence/negative-complement path,
# which is not exercised by the supplied caches' real meshes.
classes=json.loads((audit/'skin-classes.json').read_text());class_rows=[]
for n in range(1,5):
 for raw in [(128,128,0,0),(10,127,118,0),(0,128,128,0),(1,250,200,100),(255,0,0,0),(0,0,0,255),(255,64,64,64),(0,253,1,1),(15,128,127,1)]:
  v=[f32(x/255.0)for x in raw];r9=1.0;ops=[]
  def operand(s):
   neg=s.startswith('-');s=s.lstrip('-')
   value=v['xyzw'.index(s.split('.')[1][0])]if s.startswith('v2.')else 1.0 if s=='c4.wwww'else r9 if s=='R9.xxxx'else None
   assert value is not None,s
   return -value if neg else value
  for ins in classes[n+2]['instructions']:
   if ins['temp_destination']==9 and ins['temp_mask']==8 and ins['mac']==3:
    r9=f32(operand(ins['a'])+operand(ins['c']));ops.append(ins['words'])
  assert len(ops)==n-1
  class_rows.append((n,raw,[r9]+v[1:n]+[0.0]*(4-n)))
class_path=ROOT/'tests/fixtures/retail_skin_weight_classes.tsv';class_path.write_text('# Source compiled stencil_env_bumped_tangentspace classes3..6; count stored4 then effective4 float bits.\n# ADD instructions are interpreted directly from captured cache compiled code, not the native weight formula.\n'+''.join(str(n)+' '+' '.join(map(str,raw))+' '+' '.join(f'{bits(w):08X}'for w in values)+'\n'for n,raw,values in class_rows))
report=dict(weight_vertices=len(weights),actual_gpu_basis_vertices=len(world),actual_debugged_weight_matrix_cases=checks,compiled_variant_fixtures=len(class_rows),source_exports=sources,weights_sha256=hashlib.sha256(wpath.read_bytes()).hexdigest(),world_basis_sha256=hashlib.sha256(bpath.read_bytes()).hexdigest(),compiled_variant_fixture_sha256=hashlib.sha256(class_path.read_bytes()).hexdigest(),original_capture_sha256=hashlib.sha256((base/'original_capture_8.rdc').read_bytes()).hexdigest(),boundary='Actual source Xbox guest shaders through xemu. No original mesh has four influences in supplied21maps; original compiled4-influence class is retained in skin-classes.json.')
(audit/'skin-fixture-provenance.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k!='source_exports'},indent=2));c.close()

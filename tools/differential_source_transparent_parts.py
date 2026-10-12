#!/usr/bin/env python3
"""Untouched45CE0/1E1C0/1E280 metadata arithmetic with explicit resident banks."""
from pathlib import Path
import collections,hashlib,json,random,struct,subprocess,sys
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'));from xbe import Xbe
sys.path.insert(0,str(ROOT/'tools'));from inspect_multiplayer import Cache
PIN='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
def packed(v):return struct.pack('<'+'I'*len(v),*v)
def words(v):return list(struct.unpack('<'+'I'*(len(v)//4),v))
def bits(v):return words(struct.pack('<f',v))[0]
def main():
 x=Xbe(ROOT/'assets/disc/default.xbe');assert hashlib.sha256(x.data).hexdigest()==PIN
 u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x600000)
 for s in x.sections:u.mem_write(s.va,x.section_bytes(s))
 BASE=0x2100000;u.mem_map(BASE,0x200000);STOP=0x3000000;u.mem_map(STOP,4096)
 BSP,CLUSTER,PROTOTYPE,INSTANCE,MODE,TAGS,MATERIAL,HEADER,PART,OUTPUT,HASH,NODES,PLANES,POSE=[BASE+i*0x1000 for i in range(14)]
 STACK=BASE+0x1fff00;counts=collections.Counter()
 def put(a,*v):u.mem_write(a,packed(v))
 def trace(_u,a,_s,_d):
  if a in [0x45ce0,0x1e1c0,0x1e280,0x13e2d0,0x1e8c0,0x1e8f0,0x1d6b0,0x4c0d0,0x16d180]:counts[f'{a:X}']+=1
  if a==0x184000:
   # Literal original resident material visibility is an explicit admitted
   #service input; no center/plane/pose/bias/LOD arithmetic is replaced.
   u.reg_write(UC_X86_REG_EAX,1);sp=u.reg_read(UC_X86_REG_ESP);u.reg_write(UC_X86_REG_EIP,words(u.mem_read(sp,4))[0]);u.reg_write(UC_X86_REG_ESP,sp+4)
 u.hook_add(UC_HOOK_CODE,trace)
 def run(a,regs,args=()):
  for r in [UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP]:u.reg_write(r,0)
  for r,v in regs.items():u.reg_write(r,v)
  put(STACK,STOP,*args);u.reg_write(UC_X86_REG_ESP,STACK);u.reg_write(UC_X86_REG_FPCW,0x27f);u.reg_write(UC_X86_REG_MXCSR,0x1f80);u.reg_write(UC_X86_REG_EFLAGS,2)
  u.emu_start(a,STOP,count=10000);assert u.reg_read(UC_X86_REG_EIP)==STOP
 def original(row):
  mode,shader,lod,*rest=row;center=rest[:3];present=rest[3];plane=rest[4:8];tail=rest[8:]
  for a,n in [(BSP,0x200),(CLUSTER,0xb0),(PROTOTYPE,200),(INSTANCE,88),(MODE,0x80),(TAGS,32),(MATERIAL,32),(HEADER,16),(PART,72),(OUTPUT,44),(POSE,0x1000),(0x4ba138,32),(HASH,0x200),(NODES,0x100),(PLANES,40)]:u.mem_write(a,bytes(n))
  put(0x4e0348,BSP);put(0x4e3b44,TAGS);put(TAGS+24,MODE);put(MODE+0x28,CLUSTER);put(MODE+0x64,MATERIAL)
  put(BSP+0xa0,CLUSTER);put(BSP+0xa8,MATERIAL);put(BSP+0x13c,PROTOTYPE);put(BSP+0x144,INSTANCE)
  put(CLUSTER+0x50,HEADER);put(CLUSTER+0x34,HEADER);put(PROTOTYPE+0x50,HEADER);put(HEADER+4,PART)
  put(PART+0x10,*center);put(PART+0x2c,lod);put(MATERIAL+12,shader);u.mem_write(MATERIAL+28,b'\xff')
  put(0x4ba13c,1 if mode==2 else 0xffffffff);put(0x4ba138,0x1000 if mode==1 else 0)
  # Original1E1C0 inserts actual key+plane records into a caller-owned hash
  #allocation. It executes original hash/compare; no plane lookup hook.
  put(0x51f400,HASH);put(HASH+0x20,1);put(HASH+0x28,16);put(HASH+0x2c,0x1e8c0,0x1e8f0)
  put(HASH+0x38,NODES);put(NODES+8,NODES+28);put(NODES+28+8,0)
  if mode!=2 and present:
   # Duplicate key: last authored record must be selected by original bucket
   #head insertion, instead of the first matching metadata record.
   key=int(mode==1)<<16;put(PLANES,key,*[bits(12)]*4);put(PLANES+20,key,*plane);put(BSP+0x158,2,PLANES)
  else:put(BSP+0x158,0,PLANES)
  run(0x1e1c0,{})
  if mode==1:
   assert len(tail)==16;u.mem_write(INSTANCE,packed(tail[:13]));u.mem_write(INSTANCE+0x3c,packed(tail[13:]))
  if mode==2:
   rigid,flags,mapped,bias,n,*nodewords=tail;assert len(nodewords)==n*3
   u.mem_write(CLUSTER+0x2c,struct.pack('<H',rigid&65535));u.mem_write(MODE+4,struct.pack('<H',flags));put(0x4ba140,POSE-0x487b1c)
   u.mem_write(0x4ba153,bytes([bias]));u.mem_write(POSE+4,struct.pack('<H',mapped&65535))
   for i in range(n):
    for axis,offset in enumerate([0xc,0x1c,0x2c]):put(POSE+0x44+i*0x30+offset,nodewords[i*3+axis])
  run(0x45ce0,{UC_X86_REG_EAX:OUTPUT,UC_X86_REG_ECX:0},[0,0]);assert u.reg_read(UC_X86_REG_EAX)&255==1
  return words(u.mem_read(OUTPUT,44))
 inventory=json.loads((ROOT/'analysis/multiplayer-shading/transparent-authored-inventory.json').read_text());cases=[];assetlines=[];bias_records=[]
 geom=json.loads((ROOT/'analysis/multiplayer-shading/object-geometry-lod-original.json').read_text())
 bymap=collections.defaultdict(list)
 for p in inventory['parts']:bymap[p['map']].append(p)
 for name,parts in bymap.items():
  path=ROOT/'assets/disc/maps'/(name+'.map')
  if not path.exists():path=ROOT/'assets/map-pack/maps'/(name+'.map')
  c=Cache(path);planeindex={(p['category'],p['first'],p['index']):p['plane'] for p in inventory['bsp_planes']if p['map']==name}
  model_sequence=[s for s in geom['sections']if s['map']==name for _ in range(s['resources'][0]['size']//72)];model_ordinal=0
  for p in parts:
   with (ROOT/p['raw_file']).open('rb')as f:f.seek(p['raw_file_offset']);raw=f.read(72)
   shader=p['shader'];priority=command=0;masks=[]
   if shader!=0xffffffff:
    sr=c.tags[shader]['offset'];priority=struct.unpack_from('<H',c.data,sr+0x16)[0];command=struct.unpack_from('<H',c.data,sr+0x3c)[0]
    defs=c.block(sr+32,124)
    if defs:
     t=c.tags[c.u(defs[0])]['offset'];tp=c.block(t+88,40)
     if tp:masks=[c.u(l+2)for l in c.block(tp[0],10)]
   kind={'bsp_cluster':0,'bsp_instance_definition':1,'model_section':2}[p['kind']]
   if kind==2:
    # Recover actual render-model datum from independent source section fixture.
    s=model_sequence[model_ordinal];model_ordinal+=1
    assert s['section']==p['unit']and s['raw_file']==p['raw_file']and s['resources'][0]['file_offset']+p['part']*72==p['raw_file_offset']
    a,b=s['render_model'],p['unit'];prefix=[p['part']];plane=None
   else:a,b=p['unit'][1],p['part'];prefix=[];plane=planeindex.get((kind,a,b))
   expected=[*prefix,*words(raw),shader,priority,command,len(masks),*masks,int(plane is not None),*[bits(v)for v in plane or [0]*4]]
   assetlines.append(' '.join(map(str,[name,kind,a,b,*expected])))
   if kind==0:cases.append([0,shader,words(raw[44:48])[0],*words(raw[16:28]),int(plane is not None),*[bits(v)for v in plane or [0]*4]])
  bp=c.block(c.scenario+0x210,68)[0];start,size,memory=[c.u(bp+i)for i in [0,4,8]];base=start+16;magic=memory-start
  def block(at,stride):return [c.u(at+4)-magic+i*stride for i in range(c.u(at))]
  for i,at in enumerate(block(base+0x140,88)):
   definition=struct.unpack_from('<H',c.data,at+0x34)[0];matrix=words(c.data[at:at+52]);center=words(c.data[at+0x3c:at+0x48])
   assetlines.append(' '.join(map(str,[name,3,i,0,definition,*matrix,*center,c.u(at+0x48)])))
   for p in [p for p in parts if p['kind']=='bsp_instance_definition'and p['unit'][1]==definition]:
    plane=planeindex.get((1,definition,p['part']));cases.append([1,p['shader'],bits(p['lod_scale']),*[bits(v)for v in p['part_center']],int(plane is not None),*[bits(v)for v in plane or [0]*4],*matrix,*center])
  for i,p in enumerate([p for p in inventory['bsp_planes']if p['map']==name]):assetlines.append(' '.join(map(str,[name,4,i,0,p['first'],p['second'],*[bits(v)for v in p['plane']]])))
  for i,at in enumerate(block(base+0x9c,176)):assetlines.append(' '.join(map(str,[name,6,i,0,*words(c.data[at+0x54:at+0x6c])])) )
  for o in [o for o in inventory['owner_contexts']if o['map']==name]:
   d=o['definition'];g=next(g for g in geom['placements']if g['map']==name and g['type']==o['type']and g['placement']==o['placement'])
   dr=c.tags[d]['offset'];model=c.u(dr+0x38);mr=c.tags[model]['offset'];variant_name=g['variant']or c.u(dr+0x30)
   variants=c.block(mr+0x50,56);n=c.u(mr+0x70)
   POOL,HANDLES,OBJECT,DEFINITION,VARIANTS,REGIONS=[BASE+0x10000+i*0x1000 for i in range(6)]
   put(0x4e3b44,TAGS);put(0x4e0300,POOL);put(POOL+0x44,HANDLES);put(HANDLES+8,OBJECT);put(OBJECT,1);put(TAGS+24,DEFINITION);put(DEFINITION+0x38,2);put(TAGS+40,MODE)
   put(MODE+0x50,len(variants),VARIANTS);put(MODE+0x70,n)
   for i,v in enumerate(variants):u.mem_write(VARIANTS+i*56,c.data[v:v+56])
   run(0x16d180,{UC_X86_REG_ECX:2,UC_X86_REG_EDI:variant_name});vi=u.reg_read(UC_X86_REG_EAX);vi=vi if vi<0x80000000 else vi-0x100000000
   u.mem_write(OBJECT+0xb1,bytes([vi&255]));expected=[0]*n
   if vi!=-1:
    v=variants[vi];rs=c.block(v+0x14,20);put(VARIANTS+vi*56+0x18,REGIONS)
    for i,r in enumerate(rs):u.mem_write(REGIONS+i*20,c.data[r:r+20])
    for i in range(n):
     idx=struct.unpack_from('<b',c.data,v+4+i)[0]
     if idx!=-1:expected[i]=struct.unpack_from('<H',c.data,rs[idx]+0x10)[0]
   for model_count in [0,1,2]:
    u.mem_write(OUTPUT,bytes(0x200));run(0x4c0d0,{UC_X86_REG_EAX:0,UC_X86_REG_ECX:OUTPUT},[model_count]);actual=list(struct.unpack('<'+'H'*n,u.mem_read(OUTPUT+0xc2,n*2)))
    assert actual==(expected if model_count==1 else [0]*n)
   assetlines.append(' '.join(map(str,[name,5,d,vi&0xffffffff,n,*expected])))
   assetlines.append(' '.join(map(str,[name,7,d,g['variant'],*g['active']])))
   bias_records.append(dict(map=name,type=o['type'],placement=o['placement'],definition=d,variant=vi,orders=expected))
  c.data.close();c.stream.close()
 rng=random.Random(0x45ce0)
 for i in range(4096):
  mode=i%3;prefix=[mode,0x12340001,bits(rng.choice([-1.,0.,.25,1.,3.])),*[bits(rng.uniform(-100,100))for _ in range(3)],i%2,*[bits(rng.uniform(-5,5))for _ in range(4)]]
  if mode==1:prefix.extend(bits(rng.uniform(-3,3))for _ in range(16))
  if mode==2:
   rigid=rng.choice([-1,0,1,2]);flags=rng.choice([0,4]);prefix.extend([rigid&65535,flags,rng.randrange(3),rng.randrange(16),3,*[bits(rng.uniform(-1000,1000))for _ in range(9)]])
  cases.append(prefix)
 expected=[original(row)for row in cases];binary=ROOT/'build-mp-lightmap/source_transparent_parts_tests'
 native=subprocess.run([str(binary),'--probe'],input=''.join(' '.join(map(str,r))+'\n'for r in cases),text=True,capture_output=True,check=True)
 actual=[list(map(int,l.split()))for l in native.stdout.splitlines()];assert len(actual)==len(expected)
 failures=[dict(index=i,input=cases[i],expected=e,actual=a)for i,(e,a)in enumerate(zip(expected,actual))if e!=a]
 scope='Untouched45CE0 complete resident geometry routine,1D6B0 actual pose bank addressing,1E1C0/1E280/13E2D0/hash+compare. Explicit material visibility184000 service returns admitted; no math/pose/plane/bias/LOD-output hooks. Actual authored21map BSPparts/instances+synthetic model actualpose/remap/nibble inputs. Metadata census includes407selected-placement/LOD modelsections from independent fixture; fourunresolvedZanzibar regions retainexplicitselectorfixtures, not productionprimaryRNGclaim. Actual16D180/4C0D0 executes all741automaticrenderedowners withmodelcounts0/1/2; allselectedvariant regionorders arezero. Doesnotproveactualper-view shaderLOD/visibility/material-residency chronology or framebufferparity.'
 report=dict(executable_sha256=PIN,cases=len(cases),words=len(cases)*11,entries=dict(counts),mismatches=failures,services=['184000 explicit material visibility admitted'],model_sort_orders=bias_records,model_sort_order_nonzero_owners=sum(any(r['orders'])for r in bias_records),scope=scope)
 out=ROOT/'analysis/multiplayer-shading/source-transparent-parts-original.json';out.write_text(json.dumps(report,indent=2)+'\n');assert not failures,failures[:3]
 subset=list(range(min(100,len(cases))))+list(range(100,len(cases),29));fixture=ROOT/'tests/fixtures/retail_source_transparent_parts.tsv'
 fixture.write_text(''.join(' '.join(map(str,[*cases[i],*expected[i]]))+'\n'for i in subset));assets=ROOT/'tests/fixtures/retail_source_transparent_parts_assets.tsv';assets.write_text('\n'.join(assetlines)+'\n')
 for f in [fixture,assets]:f.with_suffix('.provenance.json').write_text(json.dumps(dict(executable_sha256=PIN,oracle=str(Path(__file__).relative_to(ROOT)),oracle_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),fixture_sha256=hashlib.sha256(f.read_bytes()).hexdigest(),rows=len(f.read_text().splitlines()),scope=scope),indent=2)+'\n')
 print(json.dumps({k:v for k,v in report.items()if k!='model_sort_orders'},indent=2))
if __name__=='__main__':main()

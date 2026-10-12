#!/usr/bin/env python3
"""Complete unchanged44720 sphere branch/linear45560 with explicit source banks.

No callee stubs or source patches. --accelerated-output additionally executes
the complete positive2DDBD0/2DCE60/458D0 branch with controlled resident banks.
Frustum44E90 remains a separate service. Retained32893 post-state replay
establishes no additional calls; other cases are controlled source inputs.
"""
from pathlib import Path
import argparse,collections,hashlib,json,math,random,struct,subprocess,sys
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
PIN='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d';NONE=0xffffffff
MASK_ID=0
BASE,HEADER,HANDLES,TAGS,BSP,ARENA,STACK,STOP=0x2100000,0x2200000,0x2201000,0x2300000,0x2400000,0x2500000,0x6fff000,0xa100000
CONTEXT=BASE

def pack(v):return struct.pack('<'+'I'*len(v),*v)
def words(v):return list(struct.unpack('<'+'I'*(len(v)//4),v))
def bits(v):return struct.unpack('<I',struct.pack('<f',v))[0]
def real(v):return struct.unpack('<f',struct.pack('<I',v))[0]
def sha(v):return hashlib.sha256(v).hexdigest()
def mat(position=(0,0,0),angle=0,scale=1):
 c,s=math.cos(angle),math.sin(angle);return list(map(bits,[scale,c,s,0,-s,c,0,0,0,1,*position]))
def geom(parts,bounds,mask=None,pose=None,count=None,accelerator=0):
 return {'parts':parts,'bounds':bounds,'mask':mask if mask is not None else [0]*((len(parts)+31)//32),'pose':pose,'count':len(parts)if count is None else count,'accelerator':accelerator}
def unit(flags=0x40,model=NONE,section=0,geometry=None,transform=None,instance=None):
 global MASK_ID
 MASK_ID+=1
 w=[flags,model,0x4352414e,section<<9,0x12340000,0x55000000+MASK_ID*4,0x00ff0000,0]
 if instance is not None:w[0]|=0x1000;w[3]=instance<<18
 if transform is not None:w[7]=0x87654321
 return {'w':w,'geometry':geometry,'transform':transform}
def row(units,center=(0,0,0),radius=1,mode=2,prefix=0,total=None,query=1):
 return {'units':units,'center':list(map(bits,center)),'radius':bits(radius),'mode':mode,'prefix':prefix,'total':len(units)if total is None else total,'query':query}
def serialize(r):
 version=r.get('version',1);v=[version,r['prefix'],r['total'],r['mode'],r['query'],*r['center'],r['radius'],len(r['units'])]
 for item in r['units']:
  v.extend(item['w']);v.append(int(item['transform']is not None))
  if item['transform']is not None:v.extend(item['transform'])
  g=item['geometry'];v.append(int(g is not None))
  if g is None:continue
  v.extend([g['accelerator'],g['count']&NONE,len(g['parts']),len(g['bounds']),len(g['mask']),int(g['pose']is not None),*g['parts']])
  for b in g['bounds']:v.extend([*map(bits,b[:4]),b[4]])
  v.extend(g['mask'])
  if g['pose']is not None:
   p=g['pose'];v.extend([p['owner'],len(p['nodes']),len(p['inverse'])]);v.extend(x for m in p['nodes']for x in m);v.extend(x for m in p['inverse']for x in m)
  if version>=2:
   tree=g.get('tree',b'');mapping=g.get('mapping',[]);v.extend([len(tree),len(mapping),0])
   v.extend(words(tree+bytes((-len(tree))%4)));v.extend(mapping)
 if version==3:v.extend([r['arena_address'],r['arena_used'],*r['arena']])
 return v

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--native',type=Path);ap.add_argument('--count',type=int,default=1024);group=ap.add_mutually_exclusive_group();group.add_argument('--accelerated-output',type=Path);group.add_argument('--shared-arena-output',type=Path);a=ap.parse_args()
 output_directory=a.accelerated_output or a.shared_arena_output
 if output_directory:output_directory.mkdir(parents=True,exist_ok=False)
 x=Xbe(ROOT/'assets/disc/default.xbe');assert sha(x.data)==PIN
 u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x600000)
 for s in x.sections:u.mem_write(s.va,x.section_bytes(s))
 u.mem_map(BASE,0x5000000);u.mem_map(STOP,4096)
 u.mem_write(0x4e0300,pack([HEADER]));u.mem_write(HEADER+0x44,pack([HANDLES]));u.mem_write(0x4e3b44,pack([TAGS]));u.mem_write(0x4e0348,pack([BSP]));u.mem_write(0x4c0b80,pack([CONTEXT]))
 calls=collections.Counter();executed={};trace=[];cursor=ARENA
 def observe(g,pc,size,_):
  if pc==STOP:g.emu_stop();return
  if pc not in executed:assert bytes(g.mem_read(pc,size))==x.read(pc,size);executed[pc]=size
  if pc in [0x44720,0x45560,0x320560,0xb8c00,0x142a60,0x142700,0x2ddbd0,0x2dce60,0x458d0]:calls[f'{pc:08X}']+=1
  if pc==0x44e90 or (pc==0x2ddbd0 and not output_directory):raise RuntimeError(f'unresolved source service reached{pc:08X}')
  if pc==0x2d91d0:raise RuntimeError('original accelerator stack lists require allocation')
  if pc==0x45560:
   idx=g.reg_read(UC_X86_REG_ECX)&65535;ptr,radius=words(g.mem_read(g.reg_read(UC_X86_REG_ESP)+4,8));trace.append([idx,*words(g.mem_read(ptr,12)),radius])
 u.hook_add(UC_HOOK_CODE,observe)
 def alloc(size):
  nonlocal cursor
  p=cursor;cursor=(cursor+size+15)&~15;assert cursor<STACK-0x20000;u.mem_write(p,bytes(size));return p
 def original(r):
  nonlocal cursor
  cursor=ARENA;trace.clear();context=bytearray(0x2ae0);struct.pack_into('<II',context,0x2a78,r['mode'],r['query']);context[0x2a68:0x2a78]=pack([*r['center'],r['radius']]);u.mem_write(CONTEXT,bytes(context));u.mem_write(0x4c0b78,pack([r['total']]));u.mem_write(0x4c0b7c,pack([r['prefix']]))
  clusters,definitions,instances=alloc(512*176),alloc(2048*200),alloc(2048*88);u.mem_write(BSP+0xa0,pack([clusters]));u.mem_write(BSP+0x13c,pack([definitions]));u.mem_write(BSP+0x144,pack([instances]));masks=[];mask_backings={}
  shared=alloc(512*4)if r.get('version')==3 else None
  if shared is not None:u.mem_write(shared,pack(r['arena']))
  for i,item in enumerate(r['units']):
   w=item['w'].copy();t=item['transform'];g=item['geometry'];mask=None
   if t is not None:w[7]=alloc(52);u.mem_write(w[7],pack(t))
   if g is not None:
    geo=alloc(0x40);p=alloc(max(8,len(g['parts'])*8));b=alloc(max(20,len(g['bounds'])*20));mask=mask_backings.get(w[5])
    if shared is not None:
     offset=w[5]-r['arena_address'];assert offset>=0 and offset%4==0 and offset+len(g['mask'])*4<=2048;mask=shared+offset;assert words(u.mem_read(mask,len(g['mask'])*4))==g['mask']
    elif mask is None:mask=alloc(max(4,len(g['mask'])*4));mask_backings[w[5]]=mask;u.mem_write(mask,pack(g['mask']))
    else:assert words(u.mem_read(mask,len(g['mask'])*4))==g['mask']
    w[5]=mask
    u.mem_write(geo+8,pack([g['count']&NONE,p]));u.mem_write(geo+0x14,pack([b]));u.mem_write(geo+0x30,pack([g['accelerator']&NONE]))
    if g.get('tree'):
     tree=alloc(len(g['tree']));mapping=alloc(len(g['mapping'])*2);u.mem_write(tree,g['tree']);u.mem_write(mapping,struct.pack('<'+'H'*len(g['mapping']),*g['mapping']));u.mem_write(geo+0x2c,pack([tree]));u.mem_write(geo+0x34,pack([mapping]))
    for j,idx in enumerate(g['parts']):u.mem_write(p+j*8+4,struct.pack('<H',idx))
    for j,bound in enumerate(g['bounds']):u.mem_write(b+j*20,pack([*map(bits,bound[:4]),bound[4]]))
   else:geo=0
   if w[1]==NONE:
    if w[0]&0x1000:
     idx=(w[3]>>18)&2047;u.mem_write(instances+idx*88+0x34,struct.pack('<h',i));u.mem_write(definitions+i*200+0x50,pack([geo]))
    else:u.mem_write(clusters+((w[3]>>9)&511)*176+0x50,pack([geo]))
   elif g is not None:
    mode=alloc(0x68);sections=alloc(512*0x5c);u.mem_write(TAGS+(w[1]&65535)*16+8,pack([mode]));u.mem_write(mode+0x28,pack([sections]));u.mem_write(sections+((w[3]>>9)&511)*0x5c+0x34,pack([geo]))
    pose=g['pose'];w[4]=alloc(4);u.mem_write(w[4],pack([pose['owner']if pose else NONE]))
    if pose:
     body=alloc(0x120+52*len(pose['nodes']));u.mem_write(HANDLES+(pose['owner']&65535)*12+8,pack([body]));u.mem_write(body+0x114,struct.pack('<hh',52*len(pose['nodes']),0x120));u.mem_write(body+0x120,pack([v for m in pose['nodes']for v in m]));ib=alloc(max(96,96*len(pose['inverse'])));u.mem_write(mode+0x4c,pack([ib]))
     for j,m in enumerate(pose['inverse']):u.mem_write(ib+j*96+0x28,pack(m))
   u.mem_write(0x4ba138+i*32,pack(w));masks.append(mask)
  for reg in [UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP]:u.reg_write(reg,0)
  u.reg_write(UC_X86_REG_ESP,STACK);u.mem_write(STACK,pack([STOP]));u.reg_write(UC_X86_REG_EFLAGS,2);u.reg_write(UC_X86_REG_FPCW,0x027f);u.reg_write(UC_X86_REG_FPSW,0);u.reg_write(UC_X86_REG_FPTAG,0xffff);u.reg_write(UC_X86_REG_MXCSR,0x1f80)
  u.emu_start(0x44720,STOP+1,count=3000000);assert u.reg_read(UC_X86_REG_EIP)==STOP and u.reg_read(UC_X86_REG_ESP)==STACK+4
  out=[]
  for i,item in enumerate(r['units']):
   w=words(u.mem_read(0x4ba138+i*32,32));w[4:6]=item['w'][4:6];w[7]=item['w'][7];out.extend(w)
   if masks[i]is not None:out.extend(words(u.mem_read(masks[i],len(item['geometry']['mask'])*4)))
  out.append(len(trace));out.extend(v for call in trace for v in call)
  if shared is not None:out.extend([r['arena_used'],*words(u.mem_read(shared,2048))])
  return out
 rows=[];labels=[]
 def add(r,label):rows.append(r);labels.append(label)
 directory=ROOT/'analysis/multiplayer-shading/shadow-live-phase-20261008/capture-opengl';manifest=json.loads((directory/'capture.json').read_text());phase=next(p for p in manifest['phases']if p['name']=='32893-after-part-admission');supplement=json.loads((directory/'supplement.json').read_text());f=next(f for f in supplement['files']if f['file']=='32893-source-cpu-draw-state.bin');raw=(directory/f['file']).read_bytes();assert len(raw)==f['size']and sha(raw)==f['sha256'];ctxf=next(f for f in phase['files']if f['file'].endswith('-secondary-context.bin'));ctx=(directory/ctxf['file']).read_bytes();assert len(ctx)==ctxf['size']and sha(ctx)==ctxf['sha256']
 prefix,total=struct.unpack_from('<II',raw,0x4c0b78-0x4ba000);total,prefix=prefix,total
 actual={'units':[{'w':words(raw[0x138+i*32:0x158+i*32]),'geometry':None,'transform':None}for i in range(total)],'center':words(ctx[0x2a68:0x2a74]),'radius':words(ctx[0x2a74:0x2a78])[0],'mode':words(ctx[0x2a78:0x2a7c])[0],'query':words(ctx[0x2a7c:0x2a80])[0],'prefix':prefix,'total':total}
 assert prefix==57 and total==63 and actual['query']==1 and all(item['w'][0]&1 for item in actual['units'][prefix:]);add(actual,'actual32893 AFTER-state replay: six admitted secondaryunits; geometry not consumed')
 rng=random.Random(0x4472045560)
 add(row([unit()]),'explicit currentBSP null residentgeometry')
 nonepose={'owner':NONE,'nodes':[], 'inverse':[]}
 add(row([unit(model=0xe1000060,geometry=geom([0],[(0,0,0,1,255)],pose=nonepose))]),'explicit NONE contextowner preservesquerypoint withoutnode255 service')
 for mode in [0,1,2,0xffffffff]:
  for prefix in [0,1,3,5,0x10003]:
   for total in [0,3,5,0x10005]:
    items=[unit(flags,section=i,geometry=geom([0,0xffff],[(0,0,0,1,0)]))for i,flags in enumerate([0,1,0x40,0x41,0x48])]
    add(row(items,mode=mode,prefix=prefix,total=total),'literal prefix truncation and flag1/40 skip order')
 for radius in [0,1,-1,2]:
  for boundradius in [0,1,-1,2]:
   edge=bits(abs(radius+boundradius))
   for delta in range(-3,4):
    distance=real(edge+delta)if edge+delta>=0 else 0
    add(row([unit(geometry=geom([0,0xffff],[(distance,0,0,boundradius,0)]))],radius=radius),'adjacent inclusive sphere boundary plus unconditionalFFFF part')
 for scale in [0,.5,1,2,-1]:
  for center in [(0,0,0),(1,2,3),(-2,3,-4)]:
   add(row([unit(geometry=geom([0,1],[(0,0,0,1,0),(2,0,0,1,0)]),transform=mat((1,2,3),.625,scale))],center,radius=.5),'actual unit transform, unchanged queryradius')
 for scale in [1e-30,-1e-30]:
  add(row([unit(geometry=geom([0],[(0,0,0,1,0)]),transform=mat(scale=scale))],(1e30,1e30,1e30)),'finite overflow-derivedNaN COMISS/JB rejection')
 for scale in [1e30,-1e30]:
  pose={'owner':0xe0000050,'nodes':[mat((1e30,1e30,1e30),scale=scale)],'inverse':[mat((1e30,1e30,1e30),scale=scale)]}
  add(row([unit(model=0xe1000060,geometry=geom([0],[(0,0,0,1,0)],pose=pose))]),'finite posecomposition-overflow retainsderivedIEEE COMISS/JB rejection')
 for count in [-2,-1,0,1,2,65]:
  add(row([unit(geometry=geom([0xffff]*65,[],count=count,mask=[0,0x80000000,0x76543210]))]),'signedpartcount,inactiveprefix,existingmask retention')
 # Consecutive/repeated/alternating node IDs exercise cached point and actual
 # B8C00/142A60/142700 calls. Empty bank preserves original point/cache.
 for nn in [0,1,3,256]:
  ids=[0,0,0]if nn<3 else [0,0,2,1,2,0]
  if nn==256:ids+=[255,255,0]
  for flags in [0x40,0x48,0x1040]:
   pose={'owner':0xe0000050,'nodes':[mat((j*.02,j*.03,j*.04),j*.01,.5 if j%2 else 1)for j in range(nn)],'inverse':[mat((-j*.01,0,0),-.05)for j in range(nn)]}
   g=geom(list(range(len(ids))),[(j*.05,0,0,.2,node)for j,node in enumerate(ids)],pose=pose)
   add(row([unit(flags=flags,model=0xe1000060,geometry=g)],(.2,.1,.05),.1),'source nodecache:repeat/alternate/255 and zero currentnodecount')
 for i in range(a.count):
  n=rng.randrange(1,6);items=[]
  for j in range(n):
   count=rng.randrange(1,66);bounds=[(*[rng.uniform(-2,2)for _ in range(3)],rng.uniform(0,2),rng.randrange(3))for k in range(count)];parts=[0xffff if rng.randrange(8)==0 else k for k in range(count)];mask=[rng.getrandbits(32)&rng.getrandbits(32)for _ in range((count+31)//32)]
   pose={'owner':0xe0000050+j,'nodes':[mat((rng.uniform(-1,1),0,0),rng.uniform(-1,1),rng.choice([0,.5,1,2,-1]))for k in range(3)],'inverse':[mat((0,rng.uniform(-1,1),0),rng.uniform(-1,1))for k in range(3)]}if i%3 else None
   g=geom(parts,bounds,mask,pose);flags=rng.choice([0x40,0x48,0x41,0]);model=0xe1000060+j if pose else NONE
   items.append(unit(flags,model,j,g,mat((.25,.5,.75),.3,rng.choice([0,.5,1,2,-1]))if i%4==0 else None,instance=j if i%5==0 and model==NONE else None))
  add(row(items,tuple(rng.uniform(-2,2)for _ in range(3)),rng.uniform(0,2),rng.choice([0,2]),rng.randrange(n+1)),'controlled source-valid owner/BSP/instance literal banks')
 # Literal mask alias: firstunit marks onlyFFFFpart0; secondunit sees it and
 # must not gainflag1 from a prior sharedmaskbit. Its remaining sphere misses.
 shared=[unit(section=j,geometry=geom([0xffff,0],[(20,0,0,0,0)]))for j in range(3)]
 for item in shared[1:]:item['w'][5]=shared[0]['w'][5]
 add(row(shared),'ordered sharedpartmask retainsfirstunitonly mark; laterunitdoesnotgainflag1')
 # Meaningful singleunit final positive control used by rejection tests.
 add(row([unit(geometry=geom([0,0xffff],[(0,0,0,1,0)]))]),'linear part bank positive control')
 if a.accelerated_output:
  for n in range(1024):
   key=rng.choice([1,30,62]);scale=rng.choice([32768.,65536.,262144.]);cell=65536/scale
   header=bytearray(48);struct.pack_into('<4f',header,0,*[rng.uniform(-2,2)for _ in range(3)],scale)
   code=bytes([0x26,0,4,0x27,0,4,0x28,0,4,0x10,2,2,2,0x50,key,0x50,key+1]);tree=bytes(header)+code
   perm=list(range(64));rng.shuffle(perm);items=[]
   for j in range(1+n%3):
    g=geom([0]*64,[],mask=[rng.getrandbits(32),rng.getrandbits(32)],accelerator=128);g['tree']=tree;g['mapping']=perm+list(range(64))
    model=0xe1000060+j if n%4==0 else NONE
    items.append(unit(rng.choice([0x40,0x40,0x48,0x41]),model,j,g,mat((.25,.5,.75),.3,rng.choice([0,.5,1,2,-1]))if n%5==0 else None,instance=j if n%3==0 and model==NONE else None))
   if n%7==0:
    for item in items[1:]:item['w'][5]=items[0]['w'][5];item['geometry']['mask']=items[0]['geometry']['mask'][:]
   origin=struct.unpack_from('<3f',header);center=tuple(origin[i]+cell*rng.uniform(-2,6)for i in range(3));radius=cell*rng.choice([0,.125,1,2,10,-1])
   add(row(items,center,radius,mode=rng.choice([0,2]),prefix=rng.randrange(len(items)+1)),'whole positive sphere accelerator and458D0 mapped masks, repeated unit/owner/transform inputs')
  for r in rows:r['version']=2
 if a.shared_arena_output:
  rows=[];labels=[]
  def shared_row(items,label,center=(0,0,0),radius=1,prefix=0,mode=1,initial=None,used=511):
   r=row(items,center,radius,mode,prefix);r.update(version=3,arena_address=0x55000000,arena_used=used,arena=initial or [0]*512)
   for item in items:
    g=item['geometry']
    if g is not None:
     offset=(item['w'][5]-r['arena_address'])//4;g['mask']=r['arena'][offset:offset+len(g['mask'])]
   add(r,label)
  # The late multiword allocation at offset0 overlaps both earlier starts.
  # Unit1 marks bit0 at offset1; unit2 must observe that bit as already set,
  # so it must not gain flag1 merely from its unconditional subpart32.
  for reverse in [False,True]:
   items=[unit(section=0,geometry=geom([0xffff],[])),unit(section=1,geometry=geom([0xffff],[])),unit(section=2,geometry=geom([0]*32+[0xffff],[(20,0,0,0,0)]))]
   for item,offset in zip(items,[0,1,0]):item['w'][5]=0x55000000+offset*4
   if reverse:items.reverse()
   shared_row(items,'partially overlapping0/1/0 mask starts; source-order flag propagation')
  for i in range(max(1024,a.count)):
   items=[];initial=[rng.getrandbits(32)&rng.getrandbits(32)for _ in range(512)]
   for j in range(3+i%5):
    count=rng.choice([1,31,32,33,63,64,65,96]);offset=rng.choice([0,1,2,509]);offset=min(offset,512-(count+31)//32)
    parts=[0xffff if rng.randrange(4)==0 else 0 for _ in range(count)]
    g=geom(parts,[(rng.choice([0,20]),0,0,rng.choice([0,1]),0)])
    if i%3==0:
     header=bytearray(48);struct.pack_into('<4f',header,0,0,0,0,65536.)
     g['accelerator']=count*2;g['tree']=bytes(header)+bytes([0x50,0,0x50,count-1]);g['mapping']=list(range(count))*2
    item=unit(rng.choice([0x40,0x40,0x41,0]),section=j,geometry=g,transform=mat((.25,.5,.75),.3,.5)if i%7==0 else None,instance=j if i%5==0 else None)
    item['w'][5]=0x55000000+offset*4;items.append(item)
   shared_row(items,'controlled shared512-word arena, overlapping linear/accelerated writes and retained prefix',prefix=i%3,mode=rng.choice([0,1,2]),initial=initial,used=rng.choice([0,1,510,511]))
  item=unit(geometry=geom([0,0xffff],[(0,0,0,1,0)]));item['w'][5]=0x55000000
  shared_row([item],'shared-arena linear positive control')
 expected=[original(r)for r in rows];serialized=[serialize(r)for r in rows];failures=[]
 if a.native:
  result=subprocess.run([str(a.native.resolve()),'--probe'],input=''.join(' '.join(f'{v:x}'for v in r)+'\n'for r in serialized),text=True,capture_output=True)
  if result.returncode:raise RuntimeError(result.stderr)
  actualout=[[int(v,16)for v in line.split()]for line in result.stdout.splitlines()];assert len(actualout)==len(expected)
  for i,(out,e)in enumerate(zip(actualout,expected)):
   if out!=e:failures.append({'case':i,'label':labels[i],'differences':[{'word':j,'native':f'{v:08X}','source':f'{w:08X}'}for j,(v,w)in enumerate(zip(out,e))if v!=w]})
 fixture=(output_directory/'fixture.tsv')if output_directory else ROOT/'tests/fixtures/retail_source_draw_unit_cull.tsv';fixture.write_text('# Complete unchanged44720 sphere branch/45560; pointers explicitly relocated. Shared-arena version3 compares all512 words.\n'+''.join(' '.join(f'{v:x}'for v in r)+' | '+' '.join(f'{v:x}'for v in e)+'\n'for r,e in zip(serialized,expected)))
 report={'executable_sha256':PIN,'source_unchanged':True,'cases':len(rows),'checked_words':sum(map(len,expected)),'calls':dict(calls),'executed_instructions':len(executed),'native_comparison':bool(a.native),'native_binary_sha256':sha(a.native.read_bytes())if a.native else None,'native_failed_cases':len(failures),'native_failures':failures[:20],'fixture_sha256':sha(fixture.read_bytes()),'source_function_bytes':{f'{pc:08X}':{'size':size,'sha256':sha(x.read(pc,size))}for pc,size in [(0x44720,543),(0x45560,872),(0xb8c00,56),(0x142a60,273),(0x142700,240)]},'retained_control':{'phase':phase['name'],'tick':phase['clock_tick'],'view':phase['view_index'],'frame':supplement['frame'],'main_prefix':actual['prefix'],'draw_count':actual['total'],'query_mode':1,'secondary_units':list(range(actual['prefix'],actual['total'])),'independently_checked_words':actual['total']*8,'post_state_replay_part_calls':expected[0][-1],'files':[f,ctxf]},'boundaries':['No guest IO/advance, source patches, callee return hooks or surrogate culling. Complete44720→45560→B8C00/142A60/142700 executes unchanged.','Actual32893 units are independently observed after2CBC0/44720; all six secondaryunits haveflag1 in this AFTER-state. Re-execution proves preserved32-byte units and zero additional partcalls only. No before44720 unitarray was exported: unit57 could have acquiredflag1 inside45560 during the actual invocation. No actual partbounds or original partcallcount is established.','Other cases supply controlled valid relocated source geometry/bounds/pose banks. No actual shadow gameplayadmission or successful collector chronology claim.','Unit pointerwords+10/+14/+1C are explicitly relocated for source execution and normalized to literal input pointers in comparison; every other unitword and all mutablepartmaskwords are compared.','Positive2DDBD0 accelerator and44E90 frustum branch remain unresolved services and are rejected when consumed. Missing consumedfields reject nativeinputs; skipped/admittedunits require no unusedmetadata.']}
 if output_directory:
  report['boundaries'][-1]='Positive45560→2DDBD0→2DCE60→458D0 executes unchanged with controlled resident header/bytecode/mapping and actual unit transform inputs. No live grenade geometry chronology or GPU appearance is established. Frustum44E90 remains outside this sphere seam.'
  report['tool_sha256']=sha(Path(__file__).read_bytes())
 if a.shared_arena_output:
  report.pop('retained_control')
  report['boundaries']=['Complete unchanged44720/45560 and positive2DDBD0/2DCE60/458D0 execute with controlled valid relocated BSP/instance geometry and one shared512-word arena.','Distinct and repeated mask starts overlap; every final arena word and unit word is compared in original execution order. Arena used is retained because these routines do not allocate.1332B0 allocator behavior is covered separately.','No live original receiver collector, material draw schedule or GPU appearance is established. Frustum culling is a separate service.']
 output=(output_directory/'report.json')if output_directory else ROOT/'analysis/multiplayer-shading/draw-unit-cull-original.json';output.write_text(json.dumps(report,indent=2)+'\n');fixture.with_suffix('.provenance.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k not in ['retained_control','boundaries','source_function_bytes','native_failures']},indent=2));return int(bool(failures))
if __name__=='__main__':raise SystemExit(main())

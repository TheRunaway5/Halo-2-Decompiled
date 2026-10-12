#!/usr/bin/env python3
"""Untouched ordinary FP offset hierarchy and post-bank camera composition.

Graph/controller/local/root banks are explicit inputs. This does not produce
original animation phase, mirrored/IK state, source visibility or GPU outputs.
"""
from pathlib import Path
import argparse,collections,hashlib,json,math,random,struct,subprocess,sys
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
PIN='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
def packed(v):return struct.pack('<'+'I'*len(v),*v)
def words(v):return list(struct.unpack('<'+'I'*(len(v)//4),v))
def f(v):return struct.unpack('<f',packed([v]))[0]
def bits(v):return words(struct.pack('<f',v))[0]
def rounded(v):return f(bits(v))
IDENTITY=list(map(bits,[1,1,0,0,0,1,0,0,0,1,0,0,0]))
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--binary',type=Path,default=ROOT/'build-mp-lightmap/source_model_root_tests');a=p.parse_args()
 x=Xbe(ROOT/'assets/disc/default.xbe');assert hashlib.sha256(x.data).hexdigest()==PIN
 u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x600000)
 for s in x.sections:u.mem_write(s.va,x.section_bytes(s))
 BASE=0x2100000;u.mem_map(BASE,0x400000);STOP=0x3000000;u.mem_map(STOP,4096)
 TAGS,GRAPH,GRAPH_NODES,CONTROLLER,LOCAL,ROOT_TRANSFORM,WORLD,CAMERA,CONTROL,INVERSE,FINAL_ROOT,POST,MODE,MODE_NODES,PALETTE,TEMP=[BASE+i*0x10000 for i in range(16)]
 STACK=BASE+0x3fff00;calls=collections.Counter()
 def trace(g,pc,n,_):
  if pc in [0x1416c0,0x1cbd50,0x1dd1c0,0x1421f0,0x141590,0x142a60,0x165e9f,0x1376c0]:calls[f'{pc:X}']+=1
 u.hook_add(UC_HOOK_CODE,trace)
 def put(at,*v):u.mem_write(at,packed(v))
 def run(pc,regs,stack):
  for r in [UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP]:u.reg_write(r,0)
  for i in range(8):u.reg_write(UC_X86_REG_XMM0+i,0)
  for r,v in regs.items():u.reg_write(r,v)
  u.mem_write(STACK,packed(stack));u.reg_write(UC_X86_REG_ESP,STACK);u.reg_write(UC_X86_REG_FPCW,0x27f);u.reg_write(UC_X86_REG_FPTAG,0xffff);u.reg_write(UC_X86_REG_MXCSR,0x1f80);u.reg_write(UC_X86_REG_EFLAGS,2)
  u.emu_start(pc,STOP,count=200000);assert u.reg_read(UC_X86_REG_EIP)==STOP
 def original(row):
  n,control=row[:2];offset=row[2:5];camera=row[5:18];at=18;parents=[];locals=[]
  for i in range(n):parents.append(struct.unpack('<i',packed([row[at]]))[0]);locals.append(row[at+1:at+9]);at+=9
  assert at==len(row) and parents[0]==-1 and all(0<=p<i for i,p in enumerate(parents[1:],1))
  u.mem_write(GRAPH,bytes(0x10000));u.mem_write(GRAPH_NODES,bytes(0x10000));put(GRAPH+0xc,n,GRAPH_NODES)
  for i,parent in enumerate(parents):
   children=[j for j,p in enumerate(parents)if p==i];siblings=[j for j,p in enumerate(parents)if p==parent];k=siblings.index(i)
   u.mem_write(GRAPH_NODES+i*32+4,struct.pack('<hhh',children[0]if children else -1,siblings[k+1]if k+1<len(siblings)else -1,parent))
  put(0x4e3b44,TAGS);put(TAGS+24,GRAPH);put(CONTROLLER+0x68,1);u.mem_write(LOCAL,packed([v for local in locals for v in local]));put(TEMP,*offset)
  run(0x1416c0,{UC_X86_REG_EAX:ROOT_TRANSFORM,UC_X86_REG_ECX:TEMP},[STOP])
  run(0x1cbd50,{UC_X86_REG_EAX:ROOT_TRANSFORM,UC_X86_REG_ECX:LOCAL,UC_X86_REG_EDX:WORLD},[STOP,CONTROLLER])
  pre=words(u.mem_read(WORLD,n*52));u.mem_write(CAMERA,packed(camera))
  if control!=0xffffffff:
   ctl=pre[control*13:(control+1)*13]
   for i in range(3):ctl[10+i]=bits(f(ctl[10+i])-f(offset[i]))
   u.mem_write(CONTROL,packed(ctl));run(0x141590,{UC_X86_REG_ECX:CONTROL,UC_X86_REG_EAX:INVERSE},[STOP])
  else:u.mem_write(INVERSE,packed(IDENTITY))
  run(0x142a60,{UC_X86_REG_ECX:CAMERA,UC_X86_REG_EDX:INVERSE},[STOP,FINAL_ROOT])
  run(0x165e9f,{UC_X86_REG_EAX:0x12340000,UC_X86_REG_ECX:n,UC_X86_REG_EDX:WORLD},[STOP,0xe0000000,0xe0000001,FINAL_ROOT,0,POST])
  assert words(u.mem_read(POST,12))==[0x12340000,0xe0000000,0xe0000001]
  post=words(u.mem_read(POST+12,n*52));put(TAGS+8,MODE);put(MODE+0x48,n,MODE_NODES)
  for i in range(n):u.mem_write(MODE_NODES+i*96+0x28,packed(IDENTITY))
  run(0x1376c0,{UC_X86_REG_EAX:0,UC_X86_REG_EDX:PALETTE},[STOP,POST+12]);palette=words(u.mem_read(PALETTE,n*48))
  # CPU application control only: source1376C0 matrices above are actual
  # original outputs; these diagnostic point results are not GPU exports.
  points=[]
  for i in range(n):
   matrix=list(map(f,palette[i*12:(i+1)*12]))
   for position in [(0,0,0),(1,0,0),(0,1,0),(0,0,1)]:
    for axis in range(3):
     r=axis*4;products=[rounded(rounded(matrix[r+j]*1.)*position[j])for j in range(3)]
     points.append(bits(rounded(rounded(rounded(products[0]+products[1])+products[2])+rounded(matrix[r+3]*1.))))
  return [*pre,*post,*palette,*points,*points],dict(pre_world_words=pre,post_world_words=post,palette_words=palette)
 rng=random.Random(0x1cbd50);rows=[];labels=[]
 rotations=[[0,0,0,1],[.5,.5,.5,.5],[0,0,1,0],[math.sqrt(.5),0,0,math.sqrt(.5)],[.125,-.25,.5,.875]]
 for i in range(240):
  n=2+i%11;control=n-1 if i%2 else 0xffffffff
  offset=list(map(bits,[rng.uniform(-.3,.3),rng.uniform(-.3,.3),rng.uniform(-.3,.3)]))
  camera=IDENTITY.copy();camera[10:13]=list(map(bits,[rng.uniform(-100,100)for _ in range(3)]))
  if i%3==0:camera[1:10]=list(map(bits,[0,1,0,-1,0,0,0,0,1]))
  row=[n,control,*offset,*camera]
  for j in range(n):
   parent=-1 if j==0 else rng.randrange(j);q=rotations[(i+j)%len(rotations)]
   row.extend([parent&0xffffffff,*map(bits,q),*map(bits,[rng.choice([0.,-0.,rng.uniform(-3,3)])for _ in range(3)]),bits(1 if j==control else rng.choice([.25,1.,1.25]))])
  rows.append(row);labels.append(f'explicit-ordinary-graph{i}')
 # Literal model-order raw local values, explicitly declared graph topology.
 # This does not assert source jmad mapping/controller state for these modes.
 for line in (ROOT/'tests/fixtures/retail_source_model_pose_assets.tsv').read_text().splitlines():
  if not line or line.startswith('#'):continue
  raw=line.split('|')[0].split();name,datum=raw[:2];values=[int(v,16)for v in raw[2:]];n=values[0]
  if n>64:continue
  camera=IDENTITY.copy();camera[10:13]=list(map(bits,[13.25,-7.5,9.125]));row=[n,0xffffffff,*map(bits,[.0125,-.03,.07]),*camera]
  for j in range(n):row.extend(values[1+j*10+1:1+(j+1)*10])
  rows.append(row);labels.append(f'literal-mode-locals-declared-graph:{name}:{datum}')
 results=[original(row)for row in rows];goldens=[result[0]for result in results]
 text=''.join(' '.join(f'{v:x}'for v in row)+'\n'for row in rows)
 actual=[list(map(lambda v:int(v,16),line.split()))for line in subprocess.check_output([str(a.binary.resolve()),'--probe'],input=text,text=True).splitlines()]
 assert len(actual)==len(goldens)
 failures=[]
 for i,(native,expected)in enumerate(zip(actual,goldens)):
  if native!=expected:
   different=next(j for j,(a,b)in enumerate(zip(native,expected))if a!=b)
   failures.append(dict(case=i,label=labels[i],word=different,native=native[different],expected=expected[different]))
 report=dict(executable_sha256=PIN,binary_sha256=hashlib.sha256(a.binary.read_bytes()).hexdigest(),cases=len(rows),nodes=sum(row[0]for row in rows),source_calls=dict(calls),original_bank_words=sum(len(r[1]['pre_world_words'])+len(r[1]['post_world_words'])+len(r[1]['palette_words'])for r in results),native_compared_words=sum(map(len,goldens)),fp_control_word='027F',mxcsr='1F80',source_services_replaced=[],failures=failures,scope='Untouched1416C0→1CBD50→1DD1C0 hierarchy, explicit original141590 camera-control inversion and142A60 camera composition, untouched165E9F post-bank caller then1376C0. Locals/root/camera and connected graph topology are declared inputs; literal mode locals do not claim actual jmad mapping/phase. Native diagnostic vertices apply recorded original palettes on CPU, not GPU exports. Ordinary path only, mirrored1CBD80/1DD290 and IK/lifetime/physical reciprocal-seed unresolved.')
 out=ROOT/'analysis/multiplayer-shading/model-root-fp-original.json';out.write_text(json.dumps(report,indent=2)+'\n');assert not failures,failures[:5]
 fixture=ROOT/'tests/fixtures/retail_source_model_root_fp.tsv';fixture.write_text('# Declared ordinary graph offset-before-hierarchy, camera-after-bank original callers; CPU palette vertex application checks.\n'+''.join(' '.join(f'{v:x}'for v in row)+' | '+' '.join(f'{v:x}'for v in golden)+'\n'for row,golden in zip(rows,goldens)))
 fixture.with_suffix('.provenance.json').write_text(json.dumps(dict(report,fixture_sha256=hashlib.sha256(fixture.read_bytes()).hexdigest()),indent=2)+'\n')
 print(json.dumps(report,indent=2))
if __name__=='__main__':main()

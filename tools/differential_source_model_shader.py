#!/usr/bin/env python3
"""Untouched model-owner coverage, collector ABI and shader-selection oracle."""
import argparse,hashlib,json,random,struct,subprocess,sys
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
PIN='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
BASE,STACK,STOP=0x2100000,0x22fef00,0x3000000
TABLE,SHADER,POST,TEMPLATE,TP,LODS,RESULT,AUX,CONTEXT,OBJECT=[BASE+i for i in range(0,0xa0000,0x10000)]
def packed(v):return struct.pack('<'+'I'*len(v),*(x&0xffffffff for x in v))
def words(v):return list(struct.unpack('<'+'I'*(len(v)//4),v))
def bits(v):return struct.unpack('<I',struct.pack('<f',v))[0]
def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--binary',type=Path,default=ROOT/'analysis/multiplayer-shading/source_model_shader_tests')
 p.add_argument('--count',type=int,default=4096);p.add_argument('--write-fixture',type=Path)
 p.add_argument('--output',type=Path,default=ROOT/'analysis/multiplayer-shading/source-model-shader-original.json');a=p.parse_args()
 x=Xbe(ROOT/'assets/disc/default.xbe');assert hashlib.sha256(x.data).hexdigest()==PIN
 u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x600000)
 for s in x.sections:u.mem_write(s.va,x.section_bytes(s))
 u.mem_map(BASE,0x200000);u.mem_map(STOP,0x1000)
 u.mem_write(0x4e3b44,packed([TABLE]));u.mem_write(TABLE+16,packed([0x73686164,1,SHADER,0x5c]))
 u.mem_write(TABLE+32,packed([0x7374656d,2,TEMPLATE,0x60]))
 u.mem_write(SHADER+0x24,packed([POST]));u.mem_write(POST,packed([2]));u.mem_write(TEMPLATE+0x5c,packed([TP]))
 stop_target=STOP;entries={at:0 for at in [0x1337df,0x133f70,0x2cbf0,0x2cded,0x16ae0,0x1beb0,0x410f8,0x41101,0x1bf50,0x40e30,0x3b9d0]}
 def trace(_u,address,_size,_data):
  if address in entries:entries[address]+=1
  if address==stop_target:_u.emu_stop()
 u.hook_add(UC_HOOK_CODE,trace)
 def run(at,args=(),end=STOP):
  nonlocal stop_target
  stop_target=end;u.mem_write(STACK,packed([STOP,*args]));u.reg_write(UC_X86_REG_ESP,STACK)
  u.reg_write(UC_X86_REG_FPCW,0x027f);u.reg_write(UC_X86_REG_MXCSR,0x1f80)
  u.reg_write(UC_X86_REG_FPTAG,0xffff);u.reg_write(UC_X86_REG_FPSW,0)
  try:u.emu_start(at,end,count=100000)
  except Exception as e:raise RuntimeError(f'original model slice{at:x} failed at{u.reg_read(UC_X86_REG_EIP):x}')from e
  if u.reg_read(UC_X86_REG_EIP)!=end:raise RuntimeError(f'original model slice{at:x} incomplete')
 def scalar(register):return u.reg_read(register)&0xffffffff
 abi=[];children=[];fp_bytes=[]
 def collector_abi(coverage):
  # Whole untouched133F70 up to its first2CBF0 call. No callees replaced.
  # One explicit model/section; no weapon-specific sorted-context branch.
  u.mem_write(CONTEXT,bytes(0x1e4));u.mem_write(CONTEXT+0x18,packed([1]));u.mem_write(CONTEXT+0x166,b'\x01\0')
  u.mem_write(CONTEXT+0x1b4,packed([coverage]));u.reg_write(UC_X86_REG_EAX,CONTEXT)
  run(0x133f70,[0],0x2cbf0)
  value=words(u.mem_read(u.reg_read(UC_X86_REG_ESP)+0x28,4))[0]
  if value!=coverage:raise RuntimeError('actual133F70 tenth stack coverage argument differs')
  abi.append([coverage,value])
 def child_abi(coverage):
  # Complete133AB0's actual child branch with an explicitly empty parent
  # section collector. It copies the owner collector, then stops at4B5D0;
  # object/model registration and geometry rebuilding remain caller services.
  pool,handles,render_context=BASE+0xa0000,BASE+0xa1000,BASE+0xa2000
  u.mem_write(CONTEXT,bytes(0x1e4));u.mem_write(CONTEXT+0x1b4,packed([coverage]))
  u.mem_write(0x4e0300,packed([pool]));u.mem_write(pool+0x44,packed([handles]))
  u.mem_write(handles+8,packed([OBJECT]));u.mem_write(OBJECT+0x10,packed([1]))
  u.mem_write(render_context+0x2a78,packed([0]));u.reg_write(UC_X86_REG_ECX,render_context)
  run(0x133ab0,[CONTEXT,1,0,0],0x4b5d0)
  sp=u.reg_read(UC_X86_REG_ESP);geometry=words(u.mem_read(sp+0x10,4))[0]
  copied=words(u.mem_read(geometry-4+0x1b4,4))[0];argument=words(u.mem_read(sp+0xc,4))[0]
  if copied!=coverage or argument!=coverage:raise RuntimeError('actual attached child coverage differs from owner')
  children.append([coverage,copied,argument])
 def fp_byte(value):
  u.mem_write(0x4c0b84+7,bytes([value]));u.mem_write(0x4c0ed6+7,b'\xff')
  u.reg_write(UC_X86_REG_ESI,7);u.reg_write(UC_X86_REG_EAX,0)
  run(0x403c1,end=0x403dd)
  copied=u.mem_read(STACK+0x1c,1)[0]
  if copied!=value:raise RuntimeError('402B0 first-person parallel coverage byte differs')
  fp_bytes.append([value,copied])
 def original(row):
  camera=row[:5];center=row[5:9];object_multiplier,part_multiplier=row[9:11]
  top,bottom,present,auxiliary,modifier,scale,bias,count=row[11:19];thresholds=row[19:]
  assert count==len(thresholds)
  for address,value in zip([0x4b9e20,0x4b9e2c,0x4b9e38,0x4b9e44,0x4b9ed0],camera):u.mem_write(address,packed([value]))
  u.mem_write(OBJECT+0x30,packed(center));u.mem_write(0x4ba028,packed([object_multiplier]));u.reg_write(UC_X86_REG_EAX,OBJECT)
  run(0x1337df,end=0x1338bb);coverage=scalar(UC_X86_REG_XMM1)
  u.mem_write(STACK+0x38,packed([coverage]));u.reg_write(UC_X86_REG_EBX,0)
  run(0x2cded,end=0x2ce7b);coverage_byte=u.mem_read(0x4c0b84,1)[0]
  u.mem_write(0x4b9dd0,struct.pack('<H',top&65535));u.mem_write(0x4b9dd4,struct.pack('<H',bottom&65535))
  u.mem_write(0x510c50,packed([AUX if present else 0]));u.mem_write(AUX+5,bytes([bool(auxiliary)]));run(0x1beb0)
  u.mem_write(CONTEXT+0x28,packed([scale]));u.mem_write(0x4670e4,packed([part_multiplier]))
  u.reg_write(UC_X86_REG_EAX,coverage_byte);u.reg_write(UC_X86_REG_ECX,CONTEXT);run(0x410f8,end=0x410fc)
  run(0x41101,end=0x41116);metric=scalar(UC_X86_REG_XMM0)
  u.mem_write(TP,packed([count,LODS]));u.mem_write(LODS,bytes(count*10))
  for i,t in enumerate(thresholds):u.mem_write(LODS+i*10+6,packed([t]))
  u.mem_write(0x4ba019,bytes([bool(modifier)]));u.reg_write(UC_X86_REG_EAX,bias)
  run(0x1bf50,[1,metric,RESULT]);index=words(u.mem_read(RESULT,4))[0];accepted=u.reg_read(UC_X86_REG_EAX)&255
  # Actual40E30 wrapper invokes complete1BF50 again, then retains chosen
  # shaderLOD in EBX for3B9D0. Stop before the separate draw-list service.
  if count:
   u.reg_write(UC_X86_REG_EAX,0);u.reg_write(UC_X86_REG_EDI,1)
   run(0x40e30,[metric,bias,0,0,0,0,0,0],0x3b9d0)
   if u.reg_read(UC_X86_REG_EBX)!=index:raise RuntimeError('40E30 actual selected shaderLOD differs')
  return [coverage,coverage_byte,metric,index,accepted]
 rng=random.Random(0x13379041040);rows=[];labels=[]
 def add(row,label):rows.append([x&0xffffffff for x in row]);labels.append(label)
 special=[0,0x80000000,bits(1),bits(-1),0x7f800000,0xff800000,0x7fc00000,0xffc01234,0x7fa12345]
 for raw in special+[bits(-1024),bits(1020),bits(1024)]:collector_abi(raw);child_abi(raw)
 for raw in range(256):fp_byte(raw)
 # Each original view/global input is explicit; no registration/default
 # owner/camera state is supplied by this oracle.
 for height,players,object_scale,part_scale in [(480,1,1,.85),(240,2,.8,.3),(240,3,.8,.3),(240,4,.7,.25),(2160,1,1,.85)]:
  for depth in [.1,1,10,100]:
   for radius in [.1,1,10]:
    for bias in range(7):
     for modifier in [0,1]:
      add([0,0,bits(-1),0,bits(height/2),0,0,bits(-depth),bits(radius),bits(object_scale),bits(part_scale),0,height,0,0,modifier,bits(1),bias,3,bits(.5),bits(.1),0],f'ordinary/FP explicit view{height} players{players}')
 for value in special:
  for axis in range(11):
   row=[0,0,bits(-1),0,bits(240),bits(1),bits(2),bits(-3),bits(1),bits(1),bits(.85),0,480,0,0,0,bits(1),0,3,bits(.5),bits(.1),0]
   row[axis]=value;add(row,'source model nonfinite/signedzero')
 for i in range(a.count):
  camera=[bits(rng.uniform(-1,1)) for _ in range(3)]+[bits(rng.uniform(-10,10)),bits(rng.choice([120,240,480,1080]))]
  center=[bits(rng.uniform(-100,100)) for _ in range(3)]+[bits(rng.uniform(0,10))]
  thresholds=[bits(rng.uniform(-.1,1)) for _ in range(rng.randrange(6))]
  add([*camera,*center,bits(rng.choice([1,.8,.7])),bits(rng.choice([.85,.3,.25])),0,rng.choice([120,240,480,720,1080,2160]),rng.randrange(2),rng.randrange(2),rng.randrange(2),bits(rng.uniform(.01,10)),rng.randrange(7),len(thresholds),*thresholds],'random original model collector/part bank')
 inventory=json.loads((ROOT/'analysis/multiplayer-shading/shader-lod-authored-inventory.json').read_text())
 parts=json.loads((ROOT/'analysis/multiplayer-shading/transparent-authored-inventory.json').read_text())
 shader_bank={(s['map'],s['datum']):s for s in inventory['shaders']};literal=0;seen=set()
 for part in parts['parts']:
  if part['kind']!='model_section' or part['shader']==0xffffffff:continue
  shader=shader_bank[part['map'],part['shader']];assert len(shader['definitions'])==1
  scale=part['lod_scale'] if part['lod_scale']>0 else 1
  key=(part['map'],part['shader'],bits(scale))
  if key in seen:continue
  seen.add(key);ts=[v['threshold_bits']for v in shader['definitions'][0]['lods']]
  for depth in [.1,10,100]:
   add([0,0,bits(-1),0,bits(240),0,0,bits(-depth),bits(1),bits(1),bits(.85),0,480,0,0,0,bits(scale),shader['bias'],len(ts),*ts],f"literal model {part['map']}:{shader['name']}");literal+=1
 expected=[original(row)for row in rows];encoded=[' '.join(f'{v:x}'for v in [len(row),*row])for row in rows]
 result=subprocess.run([str(a.binary.resolve()),'--probe'],input='\n'.join(encoded)+'\n',text=True,capture_output=True,check=True)
 actual=[[int(v,16)for v in line.split()]for line in result.stdout.splitlines()]
 if len(actual)!=len(expected):raise RuntimeError('model shader case count differs')
 for i,(want,got)in enumerate(zip(expected,actual)):
  if want!=got:raise RuntimeError(f'model shader case{i} {labels[i]} source{want} native{got} input{rows[i]}')
 report=dict(xbe_sha256=PIN,native_sha256=hashlib.sha256(a.binary.read_bytes()).hexdigest(),cases=len(rows),compared_words=sum(map(len,expected)),literal_model_part_contexts=literal,collector_abi_cases=len(abi),collector_abi_words=abi,child_abi_cases=len(children),child_abi_words=children,fp_byte_cases=len(fp_bytes),fp_byte_words=fp_bytes,all_bit_exact=True,fpcw='027F',mxcsr='1F80',source_entries={f'{k:08X}':v for k,v in entries.items()},scope='Actual unmodified133790 coverage slice -> 2CBF0 quantizer/full16AE0 -> full1BEB0 ->41040metric slices ->full1BF50. Complete40E30 and1BF50 stop before3B9D0 list service. Complete133F70 argument producer stops before2CBF0, verifies entryESP+28 receives literal wholecollector+1B4. Complete133AB0 child copy stops before4B5D0, verifies copied ownercoverage and argumentESP+C; bounded402B0 byte copy verifies all256 values. No callees or results stubbed. Owner lifetime/registration, FP model selection and visibility are explicit caller boundaries.')
 a.output.write_text(json.dumps(report,indent=2)+'\n')
 if a.write_fixture:
  chosen=sorted(set([*range(min(940,len(rows))),*range(940,len(rows),max(1,(len(rows)-940)//600))]))
  a.write_fixture.write_text('# Original model133790/133F70/2CBF0/41040/1BF50/40E30; explicit owner/view/part inputs.\n'+'\n'.join(encoded[i]+' '+' '.join(f'{v:x}'for v in expected[i])for i in chosen)+'\n')
 print(json.dumps({k:v for k,v in report.items()if k not in ['collector_abi_words','child_abi_words','fp_byte_words']},indent=2))
if __name__=='__main__':main()

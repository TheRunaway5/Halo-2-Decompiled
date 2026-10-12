#!/usr/bin/env python3
"""Untouched 1BEB0/ordinary 1BF50 and 41040 part-metric source differential."""
import argparse,hashlib,json,random,struct,subprocess,sys
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
PIN='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
BASE,STACK,STOP=0x2100000,0x22fef00,0x3000000
TABLE,SHADER,POST,TEMPLATE,TP,LODS,RESULT,AUX,CONTEXT=[BASE+i for i in [0,0x10000,0x11000,0x12000,0x13000,0x14000,0x15000,0x16000,0x17000]]
PART=BASE+0x18000
def packed(v):return struct.pack('<'+'I'*len(v),*v)
def words(v):return list(struct.unpack('<'+'I'*(len(v)//4),v))
def bits(v):return struct.unpack('<I',struct.pack('<f',v))[0]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--binary',type=Path,default=ROOT/'analysis/multiplayer-shading/source_shader_lod_tests')
 p.add_argument('--count',type=int,default=4096);p.add_argument('--write-fixture',type=Path)
 p.add_argument('--inventory',type=Path,default=ROOT/'analysis/multiplayer-shading/shader-lod-authored-inventory.json')
 p.add_argument('--output',type=Path,default=ROOT/'analysis/multiplayer-shading/source-shader-lod-original.json');a=p.parse_args()
 x=Xbe(ROOT/'assets/disc/default.xbe');assert hashlib.sha256(x.data).hexdigest()==PIN
 u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x600000)
 for s in x.sections:u.mem_write(s.va,x.section_bytes(s))
 u.mem_map(BASE,0x200000);u.mem_map(STOP,0x1000)
 u.mem_write(0x4e3b44,packed([TABLE]));u.mem_write(TABLE+16,packed([0x73686164,1,SHADER,0x5c]));u.mem_write(TABLE+32,packed([0x7374656d,2,TEMPLATE,0x60]))
 u.mem_write(SHADER+0x24,packed([POST]));u.mem_write(POST,packed([2]));u.mem_write(TEMPLATE+0x5c,packed([TP]))
 entries={at:0 for at in [0x1beb0,0x1bf50,0x410f8,0x2c490,0x2cded,0x16ae0,0x1333da,0x133582,0x1337df,0x13362d,0x322b0c,0x1c1b0,0x1c290,0x444b2,0x22370f,0x2b660,0x12a50]}
 stop_target=STOP
 def trace(_u,address,_size,_data):
  if address==stop_target:_u.emu_stop();return
  if address in entries:entries[address]+=1
 u.hook_add(UC_HOOK_CODE,trace)
 def run(at,args=(),end=STOP):
  nonlocal stop_target
  stop_target=end
  u.mem_write(STACK,packed([STOP,*args]));u.reg_write(UC_X86_REG_ESP,STACK)
  u.reg_write(UC_X86_REG_FPCW,0x027f);u.reg_write(UC_X86_REG_MXCSR,0x1f80)
  u.reg_write(UC_X86_REG_FPTAG,0xffff);u.reg_write(UC_X86_REG_FPSW,0)
  try:u.emu_start(at,end,count=2000000)
  except Exception as error:raise RuntimeError(f'original slice{at:x} failed at{u.reg_read(UC_X86_REG_EIP):x} ESP{u.reg_read(UC_X86_REG_ESP):x}') from error
  if u.reg_read(UC_X86_REG_EIP)!=end:raise RuntimeError(f'original shader LOD{at:x} did not finish')
 run(0x1c1b0) #Complete source initializer, literal43F188 positions.
 def view(top,bottom,present,aux):
  u.mem_write(0x4b9dd0,struct.pack('<H',top&65535));u.mem_write(0x4b9dd4,struct.pack('<H',bottom&65535))
  u.mem_write(0x510c50,packed([AUX if present else 0]));u.mem_write(AUX+5,bytes([bool(aux)]));run(0x1beb0)
  return words(u.mem_read(0x485b28,28))
 def choice(bias,modifier,metric,bank,thresholds):
  u.mem_write(0x485b28,packed(bank));u.mem_write(0x4ba019,bytes([bool(modifier)]))
  u.mem_write(TP,packed([len(thresholds),LODS]));u.mem_write(LODS,bytes(len(thresholds)*10))
  for i,t in enumerate(thresholds):u.mem_write(LODS+i*10+6,packed([t]))
  u.reg_write(UC_X86_REG_EAX,bias);run(0x1bf50,[1,metric,RESULT]);return [*words(u.mem_read(RESULT,4)),u.reg_read(UC_X86_REG_EAX)&255]
 def original(row):
  mode=row[0]
  if mode==3:
   u.reg_write(UC_X86_REG_EAX,row[1]);run(0x2c490);return words(u.mem_read(0x4670e4,4))
  if mode==4:
   u.mem_write(STACK+0x38,packed(row[1:]));u.reg_write(UC_X86_REG_EBX,0);run(0x2cded,end=0x2ce7b)
   return [u.mem_read(0x4c0b84,1)[0]]
  if mode==8:
   u.mem_write(STACK+0x30,packed([CONTEXT]));u.mem_write(CONTEXT+0x18,packed([POST]));u.mem_write(POST+8,packed([LODS]));u.mem_write(LODS,bytes(2))
   u.reg_write(UC_X86_REG_XMM0,row[1]);u.reg_write(UC_X86_REG_XMM2,0);u.reg_write(UC_X86_REG_EBX,0)
   run(0x13362d,end=0x13368f);return [u.reg_read(UC_X86_REG_EAX)&255]
  if mode==9:
   u.reg_write(UC_X86_REG_EAX,row[1]);run(0x4d656,end=0x4d66b)
   u.reg_write(UC_X86_REG_XMM0,words(u.mem_read(STACK+8,4))[0]);run(0x1c290)
   address=u.reg_read(UC_X86_REG_EAX);return [(address-0x487288)//128,*words(u.mem_read(address,128))]
  if mode==10:
   u.mem_write(0x485898,packed([row[2]]));u.reg_write(UC_X86_REG_ESI,row[1]*36)
   run(0x444b2,end=0x444cc);return [u.reg_read(UC_X86_REG_EAX)&65535]
  if mode==11:
   run(0x22370f,end=0x223714);u.reg_write(UC_X86_REG_ESI,u.reg_read(UC_X86_REG_ECX));u.reg_write(UC_X86_REG_EDX,0)
   run(0x2b660,end=0x2b67a);run(0x12a50,end=0x12a7e);return words(u.mem_read(0x485898,4))
  if mode in [5,6,7]:
   camera=row[1:6]
   for at,value in zip([0x4b9e20,0x4b9e2c,0x4b9e38,0x4b9e44,0x4b9ed0],camera):u.mem_write(at,packed([value]))
   if mode==5:
    u.mem_write(PART+0x54,packed(row[6:]));u.reg_write(UC_X86_REG_ECX,PART);u.reg_write(UC_X86_REG_EDX,0);u.reg_write(UC_X86_REG_EBX,CONTEXT)
    u.reg_write(UC_X86_REG_XMM0,bits(.5)) #1333C1 producer before bounded slice.
    run(0x1333da,end=0x1334e7);value=words(u.reg_read(UC_X86_REG_XMM1).to_bytes(16,'little')[:4])[0]
   elif mode==6:
    u.mem_write(PART+0x3c,packed(row[6:]));u.mem_write(STACK+0x30,packed([CONTEXT]));u.mem_write(CONTEXT+0x18,packed([POST]));u.mem_write(POST+0x14,packed([LODS]))
    u.reg_write(UC_X86_REG_ESI,PART);u.reg_write(UC_X86_REG_EBX,0);run(0x133582,end=0x13362d);value=words(u.mem_read(STACK+0x18,4))[0]
   else:
    u.mem_write(PART+0x30,packed(row[6:10]));u.mem_write(0x4ba028,packed(row[10:11]));u.reg_write(UC_X86_REG_EAX,PART)
    run(0x1337df,end=0x1338bb);value=words(u.reg_read(UC_X86_REG_XMM1).to_bytes(16,'little')[:4])[0]
   return [value,*original([4,value])]
  if mode==0:return view(*row[1:])
  if mode==1:return choice(row[1],row[2],row[3],row[4:11],row[12:])
  if mode!=2:raise RuntimeError('invalid shader LOD fixture operation')
  bank=view(*row[1:5]);value,scale,players,bias,modifier,n=row[5:11]
  u.mem_write(CONTEXT+0x28,packed([scale]));u.mem_write(0x4670e4,packed([players]))
  u.reg_write(UC_X86_REG_EAX,value);u.reg_write(UC_X86_REG_ECX,CONTEXT);run(0x410f8,end=0x410fc)
  #410FC reads unrelated part type and TEST, so restore its exact already
  #converted metric input and continue the actual multiply instructions.
  run(0x41101,end=0x41116);metric=words(u.reg_read(UC_X86_REG_XMM0).to_bytes(16,'little')[:4])[0]
  return [*bank,metric,*choice(bias,modifier,metric,bank,row[11:])]
 rng=random.Random(0x1beb01bf50);rows=[];labels=[]
 def add(row,label):rows.append(row);labels.append(label)
 special=[0,0x80000000,0x3f800000,0xbf800000,0x7f800000,0xff800000,0x7fc00000,0xffc01234,0x7fa12345]
 for top,bottom in [(0,480),(0,240),(0,1080),(0,0),(5,-5),(-32768,32767),(32767,-32768)]:
  for present in [0,1]:
   for aux in [0,1]:add([0,top&65535,bottom&65535,present,aux],'full1BEB0 source signed bounds/auxiliary')
 normal=[bits(1/480),0,0,bits(2/480),bits(4/480),0,bits(1/480)]
 for bias in range(7):
  for modifier in [0,1]:
   for metric in special:
    for threshold in special:add([1,bias,modifier,metric,*normal,1,threshold],'NaN/infinity/signedzero ordered threshold')
   for thresholds in [[],[bits(1),bits(.5),0]]:add([1,bias,modifier,0,*normal,len(thresholds),*thresholds],'empty/biasNever source AL/index')
 nan_values=[0x7fc00000,0xffc00000,0x7fc00001,0xffc00001,0x7fc01234,0xffc01234,0x7fa00001,0xffa00001,0x7fa00002,0xffa00002]
 for scale in nan_values:
  for multiplier in nan_values:
   add([2,0,480,0,0,1,scale,multiplier,0,0,1,0],'original SSE quiet/signaling NaN class/significand propagation')
 for i in range(a.count):
  bank=[rng.getrandbits(32) if i%5==0 else bits(rng.uniform(-2,2)) for _ in range(7)]
  metric=rng.getrandbits(32) if i%3==0 else bits(rng.uniform(-1e4,1e4))
  ts=[rng.getrandbits(32) if i%7==0 else bits(rng.uniform(-100,100)) for _ in range(rng.randrange(33))]
  add([1,rng.randrange(7),rng.randrange(2),metric,*bank,len(ts),*ts],'random actual table/threshold raw float banks')
  height=rng.randrange(-32768,32768);scale=rng.choice(special) if i%13==0 else bits(rng.uniform(-10,10));players=rng.choice(special) if i%17==0 else bits(rng.uniform(-2,2))
  add([2,0,height&65535,rng.randrange(2),rng.randrange(2),rng.randrange(256),scale,players,rng.randrange(7),rng.randrange(2),len(ts),*ts],'full view+actual41040 multiplies+ordinary choice')
 for players in [0,1,2,3,4,5,0x7fffffff,0xffffffff]:add([3,players],'full2C490 actual local player multiplier')
 for value in special+[bits(v) for v in [-2**35,-2**33,-2**31,-4,-.001,3.999999,4,4.000001,1019.9999,1020,1024,2**33]]:
  add([4,value],'source2CBF0/16AE0 quantization CVTT/wrapping/NaN')
 for i in range(a.count):
  add([4,rng.getrandbits(32)],'random source quantization raw float')
  add([8,rng.getrandbits(32)],'random instance alpha raw float')
  camera=[bits(rng.uniform(-2,2)) for _ in range(3)]+[bits(rng.uniform(-100,100)),bits(rng.uniform(.1,1000))]
  bounds=[]
  for axis in range(3):
   lo=rng.uniform(-100,100);bounds.extend([bits(lo),bits(lo+rng.uniform(0,20))])
  add([5,*camera,*bounds],'133390 actual cluster AABB/view/projection inputs')
  sphere=[bits(rng.uniform(-100,100)) for _ in range(3)]+[bits(rng.choice([.12999998,.13,.13000001]) if i%5==0 else rng.uniform(0,10))]
  add([6,*camera,*sphere],'133520 actual instance sphere smallradius boundary')
  add([7,*camera,*sphere,bits(rng.choice([1,.8,.7]))],'133790 actual object cached sphere and4BA028')
 for value in special:
  add([8,value],'instance alpha nonfinite/signedzero')
  for axis in range(5):
   camera=[0,0,bits(-1),0,bits(480)];camera[axis]=value
   add([5,*camera,*map(bits,[-1,1,-2,2,-3,3])],'cluster camera nonfinite/cancellation branch')
   add([6,*camera,*map(bits,[1,2,3,.13])],'instance camera nonfinite/cancellation branch')
   add([7,*camera,*map(bits,[1,2,3,.13,.8])],'object camera nonfinite/cancellation branch')
 for value in [24.999998,25,25.000002,25+7/255,31.999998,32,32.000002]:
  add([8,bits(value)],'instance alpha command admission threshold')
 for alpha in range(256):add([9,alpha],'all instance fade bytes complete1C1B0/1C290 stipple bank')
 add([11],'actual2234E0→2B5D0→12A50 gameplay frame descriptor')
 for layer in range(25):
  for view_type in [-2147483648,-1,0,1,2,3,4,5,6,7,8,2147483647]:
   add([10,layer,view_type&0xffffffff],'actual44370 layer fade mode table/special frame branches')
 literal_geometry=0
 for line in (ROOT/'tests/fixtures/retail_source_transparent_parts_assets.tsv').read_text().splitlines():
  fields=line.split();kind=int(fields[1])
  if kind not in [3,6]:continue
  bank=list(map(int,fields[4:]));values=bank[-4:] if kind==3 else bank
  for height in [240,480,1080]:
   add([6 if kind==3 else 5,0,0,bits(-1),bits(100),bits(height),*values],
       f'literal {fields[0]} authored '+('instance sphere' if kind==3 else 'cluster bounds'));literal_geometry+=1
 inventory=json.loads(a.inventory.read_text());literal=0
 for shader in inventory['shaders']:
  if not shader['authored_part_used']:continue
  assert len(shader['definitions'])==1
  ts=[t['threshold_bits'] for t in shader['definitions'][0]['lods']]
  for height,players in [(480,.85),(240,.3),(240,.25),(720,.85)]:
   for value in [0,16,255]:
    add([2,0,height,0,0,value,bits(1),bits(players),shader['bias'],0,len(ts),*ts],f"literal {shader['map']}:{shader['name']}");literal+=1
 expected=[original(row) for row in rows]
 encoded=[' '.join(f'{v&0xffffffff:x}' for v in [len(row),*row]) for row in rows]
 result=subprocess.run([str(a.binary.resolve()),'--probe'],input='\n'.join(encoded)+'\n',text=True,capture_output=True,check=True)
 actual=[[int(v,16) for v in line.split()] for line in result.stdout.splitlines()]
 if len(actual)!=len(expected):raise RuntimeError('shader LOD native case count differs')
 for i,(want,got) in enumerate(zip(expected,actual)):
  if want!=got:
   j=next((j for j,(x,y) in enumerate(zip(want,got)) if x!=y),min(len(want),len(got)))
   raise RuntimeError(f'shader LOD case{i} {labels[i]} field{j} source{want[j:j+1]} native{got[j:j+1]} input{rows[i]}')
 report={'xbe_sha256':PIN,'native_sha256':hashlib.sha256(a.binary.read_bytes()).hexdigest(),'cases':len(rows),'compared_words':sum(map(len,expected)),
  'literal_part_shader_cases':literal,'literal_geometry_coverage_cases':literal_geometry,'source_entries':{f'{k:08X}':v for k,v in entries.items()},'all_bit_exact':True,'fpcw':'027F','mxcsr':'1F80',
  'scope':'Complete untouched1BEB0/ordinaryshad1BF50/2C490/16AE0/322B0C/1C1B0/1C290; bounded actual410F8+41101..41116 multiplies,2CDED..2CE7Bquantizer,1333DA..1334E7cluster,133582..13362Dinstance,13362D..13368Falpha,1337DF..1338BBobjectcoverage,4D656..4D66Bstipple fraction,444B2..444CC layer mode,22370F/2B660/12A50 descriptor producer. No callees stubbed. Actual sourcecamera/geometry/collector inputs explicit. DynamicMTPR/prt3template services and nativepassselection remain separate.'}
 a.output.write_text(json.dumps(report,indent=2)+'\n')
 if a.write_fixture:
  chosen=sorted(set([*range(28),*range(28,len(rows),max(1,(len(rows)-28)//768)),
                    *[i for i,row in enumerate(rows) if row[0] in [3,9,10,11] or (row[0]==8 and 'random' not in labels[i]) or 'NaN class/significand propagation' in labels[i]]]))
  a.write_fixture.write_text('# Untouched source1BEB0/ordinary1BF50/41040metric;FPCW027F MXCSR1F80.\n'+'\n'.join(encoded[i]+' '+' '.join(f'{v:x}' for v in expected[i]) for i in chosen)+'\n')
 print(json.dumps(report,indent=2))
if __name__=='__main__':main()

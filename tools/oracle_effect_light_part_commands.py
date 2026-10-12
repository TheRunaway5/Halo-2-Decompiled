#!/usr/bin/env python3
"""Whole unchanged retail44AC0 secondary BSP part-command replay.

Retained frag RAM supplies draw units, geometry, materials, shader templates,
visibility and command storage. Declared mutations exercise the source gates.
No instruction/callee is replaced. Default mode starts at44AC0. --frame joins
whole secondary40E90 and its observed empty41C80 queue. Main mode0, nonempty
object commands, dynamic models, opaque sort and GPU submission remain outside.
"""
import argparse,hashlib,json,random,struct,subprocess,sys
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_UNMAPPED
from unicorn import x86_const as R
from oracle_effect_light_snapshot import Snapshot,u
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
NONE=0xffffffff
PIN='68271994d841fcc777be6b2b9271c311cb784c00521020aa8b20e9694c45ff0e'
def pack(v):return struct.pack('<'+'I'*len(v),*(x&NONE for x in v))
def words(v):return list(struct.unpack('<'+'I'*(len(v)//4),v))
def bits(v):return u(struct.pack('<f',v))
def sha(v):return hashlib.sha256(v).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--snapshot',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--native',type=Path);p.add_argument('--frame',action='store_true',help='Join unchanged secondary40E90 and its actual empty41C80 queue');a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
 snapshot=Snapshot(a.snapshot);assert sha(snapshot.ram)==PIN
 xbe=Xbe(ROOT/'assets/disc/default.xbe');assert sha(xbe.data)=='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
 g=snapshot.guest();stack,stop=0x700fe000,0x700fff00;g.mem_map(0x70000000,0x100000)
 def read(vm,at,n):
  regions=list(vm.mem_regions())
  for page in range(at&~4095,(at+n+4095)&~4095,4096):
   if not any(b<=page<=e for b,e,_ in regions):vm.mem_map(page,4096);vm.mem_write(page,snapshot.read(page,4096))
  return bytes(vm.mem_read(at,n))
 def val(vm,at):return u(read(vm,at,4))
 def half(vm,at):return struct.unpack('<H',read(vm,at,2))[0]
 def write(vm,at,raw):read(vm,at,len(raw));vm.mem_write(at,raw)
 def call(vm,entry,end,args=()):
  vm.mem_write(stack,pack([stop,*args]));vm.reg_write(R.UC_X86_REG_ESP,stack);vm.reg_write(R.UC_X86_REG_FPCW,0x023f);vm.reg_write(R.UC_X86_REG_MXCSR,0x1fa0)
  vm.emu_start(entry,end,count=10000000);assert vm.reg_read(R.UC_X86_REG_EIP)==end,hex(vm.reg_read(R.UC_X86_REG_EIP))
 call(g,0xc15a0,stop);call(g,0x31c80,0x31170)
 context=val(g,0x4c0b80);bsp=val(g,0x4e0348);table=val(g,0x4e3b44);visibility=val(g,0x4ed280)
 object_queue_count=half(g,val(g,context+0x14)+4)
 if a.frame:assert object_queue_count==0,'Nonempty41C80 requires native41C20 production'
 nc,cp=val(g,bsp+0x9c),val(g,bsp+0xa0);nd,dp=val(g,bsp+0x138),val(g,bsp+0x13c);ni,ip=val(g,bsp+0x140),val(g,bsp+0x144);nm,mp=val(g,bsp+0xa4),val(g,bsp+0xa8)
 metadata=[cp+i*176 for i in range(nc)]+[dp+i*200 for i in range(nd)]
 def geometry(vm,at):
  pointer=val(vm,at+0x50)
  if not pointer:return [0,0]
  count=val(vm,pointer);assert count<=256
  return [1,count,*words(read(vm,val(vm,pointer+4),count*72))]
 def lod(vm,shader):
  assert val(vm,table+(shader&65535)*16)==0x73686164,hex(shader)
  root=val(vm,table+(shader&65535)*16+8);template=val(vm,val(vm,root+0x24));tr=val(vm,table+(template&65535)*16+8);post=val(vm,tr+0x5c);n=val(vm,post);ptr=val(vm,post+4);assert 0<n<=64
  return root,ptr,[shader,half(vm,root+0x3c),n,*[val(vm,ptr+i*10+6)for i in range(n)],*[val(vm,ptr+i*10+2)for i in range(n)]]
 def shader_ids(vm,total):
  ids={val(vm,mp+i*32+12)for i in range(nm)}-{NONE}
  for i in range(total):
   slot=read(vm,0x4c0ed6+i,1)[0]
   if slot!=255:
    id=val(vm,0x4c1228+slot*4)
    if id!=NONE:ids.add(id)
  return sorted(ids)
 total=val(g,0x4c0b78);prefix=val(g,0x4c0b7c);assert prefix==31 and total==36
 for at in metadata:geometry(g,at)
 shaders=shader_ids(g,total)
 for shader in shaders:lod(g,shader)
 read(g,visibility,0x4204);read(g,ip,ni*88);read(g,mp,nm*32);read(g,context+0x25c,2048);read(g,0x4ba138,total*32);read(g,0x4c0b84,total);read(g,0x4c0ed6,850+1024)
 headers=[0x4c1a48+i*24 for i in range(3)]
 for h in headers:read(g,val(g,h+20),val(g,h)*24)
 special=0x4c62f8+val(g,0x509440)*0x408;read(g,special,0x408)
 saved=g.context_save();regions=[(b,e,p,read(g,b,e-b+1))for b,e,p in list(g.mem_regions())]
 executed={};trace=[]
 points={0x44ac0,0x449e0,0x45ce0,0x184000,0x460d0,0x41040,0x1bf50,0x40e30,0x3b9d0}
 if a.frame:points.update({0x40e90,0x41c80,0x41c20})
 def hook(vm,pc,size,_):
  if pc not in executed:
   raw=bytes(vm.mem_read(pc,size));assert raw==xbe.read(pc,size),f'Original instruction differs at{pc:08x}';executed[pc]=raw
  if pc in points:trace.append(pc)
 def clone():
  vm=Uc(UC_ARCH_X86,UC_MODE_32)
  for b,e,p,raw in regions:vm.mem_map(b,e-b+1,p);vm.mem_write(b,raw)
  def missing(v,_a,at,_s,_x,_d):
   page=at&~4095
   try:raw=snapshot.read(page,4096)
   except Exception:return False
   v.mem_map(page,4096);v.mem_write(page,raw);return True
  vm.hook_add(UC_HOOK_MEM_UNMAPPED,missing);vm.hook_add(UC_HOOK_CODE,hook);vm.context_restore(saved);return vm
 def output(vm):
  out=[]
  for h in headers:
   header=words(read(vm,h,24));capacity=header[0];assert capacity<=4096
   out.extend(header);out.append(capacity);out.extend(words(read(vm,header[5],capacity*24)))
  out.append(half(vm,special+0x404));out.extend(words(read(vm,special,1024)));return out
 def inputs(vm,mode,mask,group,selector):
  n=val(vm,0x4c0b78)
  out=[mode,mask,group,val(vm,0x4c0b7c),n,context+0x25c,half(vm,0x4686c4),val(vm,0x4670e4),read(vm,0x4ba019,1)[0],*words(read(vm,0x485b28,28)),n]
  if a.frame:out=[selector,object_queue_count,*out]
  out.extend(words(read(vm,0x4ba138,n*32)));out.extend(read(vm,0x4c0b84,n));out.extend(read(vm,0x4c0ed6,n));out.extend(words(read(vm,0x4c1228,1024)));out.extend(words(read(vm,context+0x25c,2048)))
  for n,start,stride in [(nc,cp,176),(nd,dp,200)]:
   out.append(n)
   for i in range(n):out.extend(geometry(vm,start+i*stride))
  out.append(ni);out.extend(half(vm,ip+i*88+0x34)for i in range(ni));out.append(nm)
  for i in range(nm):out.extend([val(vm,mp+i*32+12),read(vm,mp+i*32+28,1)[0]])
  out.append(0x4204//4);out.extend(words(read(vm,visibility,0x4204)))
  ids=shader_ids(vm,n);out.append(len(ids))
  for id in ids:out.extend(lod(vm,id)[2])
  out.extend(output(vm));return out
 variants=[('actual frag receiver',{})]
 for mode in [1,3,4,NONE]:variants.append(('secondary mode',dict(mode=mode)))
 for mask in [0,1,2,4,8,0x40,0x400,0x1000,0x1fffff]:variants.append(('literal unit flag filter',dict(mask=mask)))
 for group in [0,30]:variants.append(('literal group filter',dict(group=group)))
 for flags in [0,1,2,3,4,5,7,0x40,0x41,0x47,0x400,0x407]:variants.append(('unit flags and special queue',dict(flags=flags)))
 for count in [0,31,32]:variants.append(('special queue capacity',dict(flags=0x407,special_count=count)))
 for coverage in [0,1,25,128,254,255]:variants.append(('collector byte',dict(coverage=coverage)))
 for v in [0,1,255]:variants.append(('source alpha gate',dict(alpha=v)))
 for arena in ['zero','ones','alternating','random']:variants.append(('subpart admission',dict(arena=arena)))
 for count in [0,1,12,13,1024]:variants.append(('retained command capacity',dict(capacity=count)))
 for count in [0,1,12,13,1024]:variants.append(('retained command prefix',dict(command_count=count)))
 for kind in [0,4,5]:variants.append(('authored part type',dict(part_type=kind)))
 for scale in [0.,-1.,.25,10.]:variants.append(('authored part LOD scale',dict(part_scale=scale)))
 for vis in ['zero','ones','random']:variants.append(('material visibility',dict(visibility=vis)))
 for index in [0,31,32,254,255]:variants.append(('material visibility index',dict(visibility_index=index)))
 for multiplier in [0.,.25,.3,.85,10.]:variants.append(('actual player multiplier',dict(multiplier=multiplier)))
 for lodview in [0.,1.,1000.]:variants.append(('actual shader view multipliers',dict(lodview=lodview)))
 variants.extend([('view modifier',dict(modifier=1)),('null current geometry',dict(null_geometry=True)),('shader NONE',dict(shader_none=True)),('source shader override',dict(override=True)),('retained command bytes',dict(random_storage=True))])
 if a.frame:
  variants=[(label,v)for label,v in variants if 'mask'not in v]
  variants += [('40E90 selector mask',dict(selector=n))for n in [0,1,2,3,NONE,0x100]]
 rng=random.Random(0x44ac041040);fixture=[0x434d5042,2 if a.frame else 1,len(variants)];cases=[]
 chosen=next(id for id in shaders if lod(g,id)[2][2]==1)
 for label,v in variants:
  vm=clone();mode=v.get('mode',2);mask=v.get('mask',NONE);group=v.get('group',31)
  for h in headers[1:]:write(vm,h+4,pack([0,0,0]))
  for i in range(prefix,total):
   at=0x4ba138+i*32
   if 'flags'in v:write(vm,at,pack([(val(vm,at)&~0x7ff)|v['flags']]))
   if 'alpha'in v:write(vm,at+26,bytes([v['alpha']]))
   if 'coverage'in v:write(vm,0x4c0b84+i,bytes([v['coverage']]))
   if v.get('override'):write(vm,0x4c0ed6+i,b'\0')
  if v.get('override'):write(vm,0x4c1228,pack([chosen]))
  if 'special_count'in v:write(vm,special+0x404,struct.pack('<H',v['special_count']))
  if 'arena'in v:
   name=v['arena'];data=[0 if name=='zero' else NONE if name=='ones' else 0xaaaaaaaa if name=='alternating' else rng.getrandbits(32)for _ in range(512)];write(vm,context+0x25c,pack(data))
  if 'visibility'in v:
   name=v['visibility'];write(vm,visibility,bytes(0x4204)if name=='zero' else b'\xff'*0x4204 if name=='ones' else bytes(rng.randrange(256)for _ in range(0x4204)))
  for i in range(nm):
   if 'visibility_index'in v:write(vm,mp+i*32+28,bytes([v['visibility_index']]))
   if v.get('shader_none'):write(vm,mp+i*32+12,pack([NONE]))
  seen=set()
  for at in metadata:
   geo=val(vm,at+0x50)
   if v.get('null_geometry'):write(vm,at+0x50,pack([0]));continue
   if geo and geo not in seen:
    seen.add(geo);n=val(vm,geo);ptr=val(vm,geo+4)
    for i in range(n):
     if 'part_type'in v:write(vm,ptr+i*72,struct.pack('<H',v['part_type']))
     if 'part_scale'in v:write(vm,ptr+i*72+44,struct.pack('<f',v['part_scale']))
  if 'multiplier'in v:write(vm,0x4670e4,pack([bits(v['multiplier'])]))
  if 'lodview'in v:write(vm,0x485b28,pack([bits(v['lodview'])]*7))
  if 'modifier'in v:write(vm,0x4ba019,bytes([v['modifier']]))
  for h in headers:
   capacity=min(v.get('capacity',val(vm,h)),val(vm,h));write(vm,h,pack([capacity]))
   if 'command_count'in v:write(vm,h+4,pack([v['command_count']]))
   if v.get('random_storage'):
    write(vm,h+8,pack([rng.getrandbits(32),rng.getrandbits(32),rng.getrandbits(32)]));write(vm,val(vm,h+20),pack([rng.getrandbits(32)for _ in range(capacity*6)]))
  selector=v.get('selector',0)
  before=inputs(vm,mode,mask,group,selector);trace.clear()
  if a.frame:
   write(vm,context+0x2a78,pack([mode]));vm.reg_write(R.UC_X86_REG_EAX,selector);call(vm,0x40e90,stop)
   assert 0x41c20 not in trace,'Unchanged41C80 consumed nonempty object commands'
  else:call(vm,0x44ac0,stop,[mode,mask,group])
  after=output(vm)+[len(trace),*trace]
  fixture.extend([len(before),len(after),*before,*after]);cases.append(dict(label=label,mutation=v,checked_words=len(after),commands=[val(vm,h+4)for h in headers],calls=len(trace)))
 path=a.output/'fixture.bin';path.write_bytes(pack(fixture));report=dict(boundary=__doc__,snapshot_sha256=PIN,original_xbe_sha256=sha(xbe.data),intercepted_instructions=0,executed_instructions=len(executed),executed_instruction_bank_sha256=sha(b''.join(pack([pc])+raw for pc,raw in sorted(executed.items()))),cases=cases,checked_words=sum(c['checked_words']for c in cases),fixture_sha256=sha(path.read_bytes()),tool_sha256=sha(Path(__file__).read_bytes()))
 report.update(joined_secondary_frame=a.frame,object_queue_count=object_queue_count,native_object_commands='explicit empty queue only; nonempty41C20 unjoined' if a.frame else 'unjoined')
 if a.native:
  result=subprocess.run([str(a.native.resolve()),str(path.resolve())],text=True,capture_output=True);report.update(native_binary_sha256=sha(a.native.read_bytes()),native_returncode=result.returncode,native_stdout=result.stdout,native_stderr=result.stderr)
 (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');path.with_suffix('.provenance.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k!='cases'},indent=2))
 if a.native and result.returncode:raise SystemExit(result.returncode)
if __name__=='__main__':main()

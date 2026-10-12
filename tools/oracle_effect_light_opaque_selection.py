#!/usr/bin/env python3
"""Whole unchanged3BAB0/4CBF0 ordinary-sphere layer12 selection and sort.

Actual frag RAM supplies whole40E90's13 commands and all original banks.
Native selects using independent Lockout Cache shader/geometry metadata.
Declared command/unit/data variants preserve every retained output byte.
4BA014 is explicitly1, as31170 sets for the ordinary source sphere. No
instruction/callee is replaced;4D0B0/GPU submission is not executed.
"""
import argparse,hashlib,json,random,struct,subprocess,sys
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_UNMAPPED
from unicorn import x86_const as R
from oracle_effect_light_snapshot import Snapshot,u
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'));from xbe import Xbe
NONE=0xffffffff;PIN='68271994d841fcc777be6b2b9271c311cb784c00521020aa8b20e9694c45ff0e'
def pack(v):return struct.pack('<'+'I'*len(v),*(x&NONE for x in v))
def words(v):return list(struct.unpack('<'+'I'*(len(v)//4),v))
def sha(v):return hashlib.sha256(v).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--snapshot',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--native',type=Path);p.add_argument('--map',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
 s=Snapshot(a.snapshot);assert sha(s.ram)==PIN;x=Xbe(ROOT/'assets/disc/default.xbe');assert sha(x.data)=='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
 g=s.guest();stack,stop,output=0x700fe000,0x700fff00,0x70000000;g.mem_map(0x70000000,0x100000)
 def read(vm,at,n):
  regions=list(vm.mem_regions())
  for page in range(at&~4095,(at+n+4095)&~4095,4096):
   if not any(b<=page<=e for b,e,_ in regions):vm.mem_map(page,4096);vm.mem_write(page,s.read(page,4096))
  return bytes(vm.mem_read(at,n))
 def val(vm,at):return u(read(vm,at,4))
 def write(vm,at,raw):read(vm,at,len(raw));vm.mem_write(at,raw)
 def call(vm,entry,end,args=()):
  vm.mem_write(stack,pack([stop,*args]));vm.reg_write(R.UC_X86_REG_ESP,stack);vm.reg_write(R.UC_X86_REG_FPCW,0x023f);vm.reg_write(R.UC_X86_REG_MXCSR,0x1fa0);vm.emu_start(entry,end,count=30000000);assert vm.reg_read(R.UC_X86_REG_EIP)==end,hex(vm.reg_read(R.UC_X86_REG_EIP))
 call(g,0xc15a0,stop);call(g,0x31c80,0x31197);header=0x4c1a60;h=words(read(g,header,24));assert h[0]==1024 and h[1]==13 and h[3]==0
 actual=[words(read(g,h[5]+i*24,24))for i in range(h[1])];nunits=val(g,0x4c0b78);read(g,0x4ba138,nunits*32)
 table=val(g,0x4e3b44);bsp=val(g,0x4e0348)
 def template(vm,shader):
  root=val(vm,table+(shader&65535)*16+8);stem=val(vm,val(vm,root+0x24));stem_root=val(vm,table+(stem&65535)*16+8);return val(vm,stem_root+0x5c)
 layer_words=set();vertex_classes=set();vertex_sizes=set();vertex_ids=set()
 for c in actual:
  post=template(g,c[2]);lod=val(g,post+4)+(c[5]>>8&255)*10;layer=val(g,post+12)+(struct.unpack('<H',read(g,lod,2))[0]&511)*2+24;layer_words.add(layer);packed=struct.unpack('<H',read(g,layer,2))[0];entry=val(g,post+20)+(packed&511)*10;spas=val(g,entry+4);spas_root=val(g,table+(spas&65535)*16+8);implementation=val(g,val(g,spas_root+0x20)+4)+306;vertex_ids.add(implementation+0x100);vrtx=val(g,implementation+0x100)
  if vrtx!=NONE:
   vr=val(g,table+(vrtx&65535)*16+8);vertex_classes.add(vr+4);n=val(g,vr+4);ptr=val(g,vr+8)
   for i in range(n):vertex_sizes.add(ptr+i*28+12);read(g,ptr+i*28,28)
 missing_shader=None;nm,mp=val(g,bsp+0xa4),val(g,bsp+0xa8)
 for i in range(nm):
  shader=val(g,mp+i*32+12)
  if shader==NONE:continue
  post=template(g,shader);lod=val(g,post+4);layer=val(g,post+12)+((struct.unpack('<H',read(g,lod,2))[0]&511)+12)*2
  if not(struct.unpack('<H',read(g,layer,2))[0]>>9):missing_shader=shader;break
 saved=g.context_save();regions=[(b,e,p,read(g,b,e-b+1))for b,e,p in list(g.mem_regions())];executed={};calls={}
 def hook(vm,pc,size,_):
  if pc not in executed:
   raw=bytes(vm.mem_read(pc,size));assert raw==x.read(pc,size),f'Original instruction differs at{pc:08x}';executed[pc]=raw
  if pc in [0x3bab0,0x4cbf0,0x15d70,0x30da0,0x44940,0x1cb20,0x4cbb0,0x13da70,0x13dcd0,0x3bb80]:calls[f'{pc:08x}']=calls.get(f'{pc:08x}',0)+1
 def clone():
  vm=Uc(UC_ARCH_X86,UC_MODE_32)
  for b,e,p,raw in regions:vm.mem_map(b,e-b+1,p);vm.mem_write(b,raw)
  def missing(v,_a,at,_sz,_x,_d):
   page=at&~4095
   try:raw=s.read(page,4096)
   except Exception:return False
   v.mem_map(page,4096);v.mem_write(page,raw);return True
  vm.hook_add(UC_HOOK_MEM_UNMAPPED,missing);vm.hook_add(UC_HOOK_CODE,hook);vm.context_restore(saved);return vm
 variants=[('actual13 commands',{})]
 for n in [0,1,2,7,8,9,14,31,64,127,512,1024]:variants.append(('literal command count and duplicate keys',dict(count=n)))
 for first in [1,5,12,13,14]:variants.append(('already processed command prefix',dict(first=first)))
 for mask in [0,0x1000,NONE]:variants.append(('literal layer filter',dict(layer_mask=mask)))
 for classification in [0,1,2,3,7]:variants.append(('retained draw-unit geometry classification',dict(classification=classification)))
 for size in [0,1,15,16,32]:variants.append(('actual compiled vertex program size',dict(vertex_size=size)))
 for n in [0,1,2,3]:variants.append(('actual compiled vertex class count',dict(vertex_classes=n)))
 variants.extend([('missing layer12 pass',dict(no_layer=True)),('vertex program NONE',dict(vertex_none=True)),('random retained output',dict(random_retained=True))])
 if missing_shader is not None:variants.append(('authored shader without layer12 pass',dict(missing_shader=missing_shader)))
 rng=random.Random(0x3bab04cbf0);fixture=[0x534c504f,2,len(variants)];cases=[]
 for label,v in variants:
  vm=clone();count=v.get('count',13);hh=h.copy();hh[1]=count;hh[3]=v.get('first',0);write(vm,header,pack(hh));write(vm,0x4ba014,pack([1]))
  commands=[actual[i%13].copy()for i in range(count)]
  for c in commands:
   if 'layer_mask'in v:c[4]=v['layer_mask']
   if 'missing_shader'in v:c[2]=v['missing_shader'];c[5]&=0xffff00ff
  write(vm,h[5],pack([w for c in commands for w in c]))
  if 'classification'in v:
   for i in range(nunits):
    at=0x4ba138+i*32+12;write(vm,at,pack([(val(vm,at)&0x1fffffff)|(v['classification']<<29)]))
  patches=[]
  def patch(at,raw):write(vm,at,raw);patches.append([at,len(raw),*raw])
  if 'vertex_size'in v:
   for at in sorted(vertex_sizes):patch(at,pack([v['vertex_size']]))
  if 'vertex_classes'in v:
   for at in sorted(vertex_classes):patch(at,pack([v['vertex_classes']]))
  if v.get('vertex_none'):
   for at in sorted(vertex_ids):patch(at,pack([NONE]))
  if v.get('no_layer'):
   for at in sorted(layer_words):patch(at,b'\0\0')
  retained=[rng.getrandbits(32)if v.get('random_retained')else 0xa5a5a5a5 for _ in range(1024*6)];write(vm,output,pack(retained))
  before=[nunits,*words(read(vm,0x4ba138,nunits*32)),*hh,count,*[w for c in commands for w in c],len(patches),*[w for p in patches for w in p],1024,*retained]
  call(vm,0x3bab0,stop,[header,0,12,output]);selected=vm.reg_read(R.UC_X86_REG_EAX);assert selected<=count
  after=[selected,*words(read(vm,output,1024*24))];fixture.extend([*before,*after]);cases.append(dict(label=label,mutation=v,selected=selected,checked_words=1024*6))
 # Complete literal1CB20 dispatcher with controlled real VRTX count service.
 dispatch=[];g.hook_add(UC_HOOK_CODE,hook)
 any_vrtx=val(g,next(iter(vertex_ids)));vr=val(g,table+(any_vrtx&65535)*16+8);old_count=val(g,vr+4)
 for selector in range(4):
  for auxiliary in [-3,-2,-1,0,1,2,127,128,65535]:
   for class_count in [0,1,2,3,4,65535]:
    write(g,vr+4,pack([class_count]));g.reg_write(R.UC_X86_REG_EAX,selector);g.reg_write(R.UC_X86_REG_ECX,any_vrtx);call(g,0x1cb20,stop,[auxiliary]);dispatch.append([selector,auxiliary,class_count,g.reg_read(R.UC_X86_REG_EAX)])
 write(g,vr+4,pack([old_count]));fixture.extend([len(dispatch),*[w for row in dispatch for w in row]])
 path=a.output/'fixture.bin';path.write_bytes(pack(fixture));(a.output/'tool.py').write_bytes(Path(__file__).read_bytes());report=dict(boundary=__doc__,snapshot_sha256=PIN,original_xbe_sha256=sha(x.data),intercepted_instructions=0,executed_instructions=len(executed),executed_instruction_bank_sha256=sha(b''.join(pack([pc])+raw for pc,raw in sorted(executed.items()))),calls=calls,cases=cases,vertex_dispatch_cases=len(dispatch),selected_records=sum(c['selected']for c in cases),checked_words=sum(c['checked_words']for c in cases),fixture_sha256=sha(path.read_bytes()),tool_sha256=sha(Path(__file__).read_bytes()),native_map_sha256=sha(a.map.read_bytes()))
 if a.native:
  result=subprocess.run([str(a.native.resolve()),str(path.resolve()),str(a.map.resolve())],text=True,capture_output=True);report.update(native_binary_sha256=sha(a.native.read_bytes()),native_returncode=result.returncode,native_stdout=result.stdout,native_stderr=result.stderr)
 (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');path.with_suffix('.provenance.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k!='cases'},indent=2))
 if a.native and result.returncode:raise SystemExit(result.returncode)
if __name__=='__main__':main()

#!/usr/bin/env python3
"""Whole unchanged3D4F0/137650 against native model/node input production.

Actual calls come from unchanged main2CBC0/133790/4B5D0. Controlled cases
declare current object, first-person cache, output alias, model and section
banks. Native reads independent pre-call data, not source outputs. Every
original instruction executes unchanged; live pose/node/cache construction,
resource publication and framebuffer parity are separate required owners.
"""
import argparse,hashlib,json,struct,subprocess,sys
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_MEM_UNMAPPED,UC_HOOK_CODE,UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_effect_light_snapshot import Snapshot,u
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
NONE=0xffffffff
PIN='68271994d841fcc777be6b2b9271c311cb784c00521020aa8b20e9694c45ff0e'
def pack(v):return struct.pack('<'+'I'*len(v),*(x&NONE for x in v))
def words(v):return list(struct.unpack('<'+'I'*(len(v)//4),v))
def sha(v):return hashlib.sha256(v).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--snapshot',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--native',type=Path);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
 s=Snapshot(a.snapshot);assert sha(s.ram)==PIN;x=Xbe(ROOT/'assets/disc/default.xbe');assert sha(x.data)=='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
 stack,stop,cached_models=0x700fe000,0x700fff00,0x4c1bd8;executed={};entries={0:[],1:[]}
 def read(vm,at,n):
  regions=list(vm.mem_regions())
  for page in range(at&~4095,(at+n+4095)&~4095,4096):
   if not any(b<=page<=e for b,e,_ in regions):vm.mem_map(page,4096);vm.mem_write(page,s.read(page,4096))
  return bytes(vm.mem_read(at,n))
 def val(vm,at):return u(read(vm,at,4))
 def write(vm,at,raw):read(vm,at,len(raw));vm.mem_write(at,raw)
 def unchanged(vm,pc,size,_):
  if pc not in executed:
   raw=bytes(vm.mem_read(pc,size));assert raw==x.read(pc,size),f'Original instruction differs at{pc:08x}';executed[pc]=raw
 def run(vm,entry,sp,args=(),regs=None):
  vm.mem_write(sp,pack([stop,*args]));vm.reg_write(R.UC_X86_REG_ESP,sp);vm.reg_write(R.UC_X86_REG_FPCW,0x023f);vm.reg_write(R.UC_X86_REG_MXCSR,0x1fa0)
  for register,value in (regs or {}).items():vm.reg_write(register,value)
  vm.emu_start(entry,stop,count=20000000);assert vm.reg_read(R.UC_X86_REG_EIP)==stop,hex(vm.reg_read(R.UC_X86_REG_EIP))
 def clone(regions):
  vm=Uc(UC_ARCH_X86,UC_MODE_32)
  for b,e,perms,raw in regions:vm.mem_map(b,e-b+1,perms);vm.mem_write(b,raw)
  def missing(guest,_access,at,_n,_v,_d):
   page=at&~4095
   try:raw=s.read(page,4096)
   except Exception:return False
   guest.mem_map(page,4096);guest.mem_write(page,raw);return True
  vm.hook_add(UC_HOOK_MEM_UNMAPPED,missing);vm.hook_add(UC_HOOK_CODE,unchanged);return vm
 g=s.guest();g.mem_map(0x70000000,0x100000)
 def capture(vm,pc,_size,_):
  if pc not in [0x3d4f0,0x137650]:return
  kind=int(pc==0x137650);sp=vm.reg_read(R.UC_X86_REG_ESP)
  args=[vm.reg_read(R.UC_X86_REG_EDX),vm.reg_read(R.UC_X86_REG_EBX)]if kind else [*words(read(vm,sp+4,24)),vm.reg_read(R.UC_X86_REG_EBX)]
  entries[kind].append((vm.reg_read(R.UC_X86_REG_EAX),sp,args,[(b,e,perms,bytes(vm.mem_read(b,e-b+1)))for b,e,perms in vm.mem_regions()]))
 g.hook_add(UC_HOOK_CODE,unchanged);g.hook_add(UC_HOOK_CODE,capture)
 write(g,0x4c0b80,pack([0x547f88]));run(g,0x2cbc0,stack,[0]);assert entries[0] and entries[1]
 variants=[('actual model input call',dict(kind=k,index=i))for k in [0,1]for i in range(len(entries[k]))]
 variants += [('cached first-person entry and count cap',dict(kind=0,cached=1,slot=i,node_count=n))for i in range(4)for n in [0,1,63,64,65,255,256,NONE]]
 variants += [('cached flag low byte',dict(kind=0,cached=n,slot=0))for n in [0,1,255,256,257,NONE]]
 variants += [('cache index low word',dict(kind=0,cached=1,slot=n))for n in [0x10000,0x10001,0x10002,0x10003]]
 variants += [('aliased model input outputs',dict(kind=0,cached=c,slot=0,alias=[i,j]))for c in [0,1]for i in range(5)for j in range(i+1,5)]
 variants += [('signed node byte extent and offset',dict(kind=0,cached=0,node_extent=n,node_offset=o))for n in [-32768,-53,-52,-1,0,51,52,32767]for o in [-32768,-1,0,32767]]
 variants += [('normal count output aliases node extent',dict(kind=0,cached=0,body_count=True))]
 variants += [('normal transforms output aliases model node count',dict(kind=0,cached=0,model_count_alias=True))]
 variants += [('cached model output aliases cached flags',dict(kind=0,cached=1,slot=0,cached_flags_alias=True))]
 variants += [('authored geometry node LOD',dict(kind=1,lod=n))for n in range(6)]
 variants += [('selected section byte and compact flag',dict(kind=1,sections=n,compact=c))for n in [0,1,127,128,254,255]for c in [0,1]]
 variants += [('signed region count and wrapped node sum',dict(kind=1,regions=n,node_count=m,compact=c))for n in [NONE,0,1,2,16]for m in [0,0x7fffffff,NONE]for c in [0,1]]
 variants += [('signed authored LOD group selector',dict(kind=1,group=n,compound_count=m))for n in [-128,-1,0,1,127]for m in [0,1,40,NONE,0x80000000]]
 fixture=[0x49424d53,1,len(variants)];cases=[]
 for label,v in variants:
  kind=v['kind'];eax,sp,args,regions=entries[kind][v.get('index',0)];vm=clone(regions);args=list(args)
  tags,objects=val(vm,0x4e3b44),val(vm,0x4e0300)
  def metadata(tag):return val(vm,tags+(tag&65535)*16+8)
  owned=[]
  if not kind:
   owner=args[6];body=val(vm,val(vm,objects+0x44)+(owner&65535)*12+8)
   model=val(vm,metadata(val(vm,metadata(val(vm,body))+0x38))+4);m=metadata(model)
   if label!='actual model input call':
    args[1:6]=[0x70006000+i*16 for i in range(5)]
    for at in args[1:6]:write(vm,at,pack([0x5c6d7e8f]*4))
   if 'cached'in v:eax=v['cached'];args[0]=v.get('slot',0)
   if eax&255 and label!='actual model input call':
    entry=cached_models+(args[0]&65535)*0xd0c
    write(vm,entry,pack([model,owner,0xa53c7e91]))
   if 'node_count'in v:write(vm,m+0x48,pack([v['node_count']]))
   if 'node_extent'in v:write(vm,body+0x114,struct.pack('<hh',v['node_extent'],v['node_offset']))
   if 'alias'in v:i,j=v['alias'];args[1+j]=args[1+i]
   if v.get('body_count'):args[5]=body+0x114;owned.append((body+0x110,16))
   if v.get('model_count_alias'):args[4]=m+0x48;owned.append((m+0x44,16))
   if v.get('cached_flags_alias'):args[3]=cached_models+8;owned.append((cached_models,16))
   owned += [(at,16 if label!='actual model input call'and 0x70006000<=at<0x70006100 else 4)for at in args[1:6]]
  else:
   m=metadata(eax)
   if 'lod'in v:args[0]=v['lod']
   if label!='actual model input call':
    args[1]=0x70007000;write(vm,args[1],bytes([v.get('sections',0)])*16)
    table=0x70008000;raw=bytearray(256*0x5c)
    for i in range(256):raw[i*0x5c+0x25]=[0,1,40,255][i%4]
    write(vm,table,bytes(raw));write(vm,m+0x28,pack([table]));owned += [(args[1],16)]
   if 'compact'in v:
    flags=read(vm,m+4,1)[0];write(vm,m+4,bytes([(flags&~4)|(4 if v['compact']else 0)]))
   if 'regions'in v:write(vm,m+0x1c,pack([v['regions']]))
   if 'node_count'in v:write(vm,m+0x48,pack([v['node_count']]))
   if 'group'in v:
    flags=read(vm,m+4,1)[0];write(vm,m+4,bytes([flags&~4]))
    groups=0x7000e000;write(vm,m+0x38,pack([groups]));write(vm,m+0x3c+(args[0]&NONE),struct.pack('<b',v['group']))
    write(vm,groups+v['group']*12+4,pack([v['compound_count']]))
   owned += [(m,0x50)]
  for at,n in owned:read(vm,at,n)
  initial_pages={at:bytes(vm.mem_read(at,4096))for b,e,_ in vm.mem_regions()for at in range(b,e+1,4096)}
  def initial_read(at,n):
   out=b''
   while n:
    page=at&~4095;count=min(n,4096-(at&4095));raw=initial_pages.get(page)
    if raw is None:raw=s.read(page,4096)
    out+=raw[at-page:at-page+count];at+=count;n-=count
   return out
  ranges=[(at&~3,(at+n+3)&~3)for at,n in owned]
  def access(_guest,_access,at,n,_value,_data):ranges.append((at&~3,(at+n+3)&~3))
  vm.hook_add(UC_HOOK_MEM_READ,access);vm.hook_add(UC_HOOK_MEM_WRITE,access)
  regs={R.UC_X86_REG_EAX:eax,R.UC_X86_REG_EDX:args[0],R.UC_X86_REG_EBX:args[1]if kind else args[6]}
  try:run(vm,0x137650 if kind else 0x3d4f0,sp,[]if kind else args[:6],regs)
  except Exception as error:raise RuntimeError(f'Case{len(cases)} {label} {v}, PC{vm.reg_read(R.UC_X86_REG_EIP):08x}: {error}')from error
  result=vm.reg_read(R.UC_X86_REG_EAX)if kind else 0
  inputs=[kind,eax,*args,tags,objects,cached_models,len(owned),*[w for at,n in owned for w in [at,n//4]]]
  merged=[]
  for start,end in sorted(ranges):
   if merged and start<=merged[-1][1]:merged[-1][1]=max(merged[-1][1],end)
   else:merged.append([start,end])
  inputs.append(len(merged))
  for start,end in merged:inputs.extend([start,(end-start)//4,*words(initial_read(start,end-start))])
  after=[result,*[w for at,n in owned for w in words(read(vm,at,n))]]
  fixture.extend([len(inputs),len(after),*inputs,*after]);cases.append(dict(label=label,mutation=v,result=result,checked_words=len(after)))
 out=a.output/'fixture.bin';out.write_bytes(pack(fixture));report=dict(boundary=__doc__,snapshot_sha256=PIN,original_xbe_sha256=sha(x.data),declared_main_context='00547f88',original_inputs={str(k):len(v)for k,v in entries.items()},intercepted_instructions=0,executed_instructions=len(executed),executed_instruction_bank_sha256=sha(b''.join(pack([pc])+raw for pc,raw in sorted(executed.items()))),cases=cases,checked_words=sum(c['checked_words']for c in cases),fixture_sha256=sha(out.read_bytes()),tool_sha256=sha(Path(__file__).read_bytes()))
 if a.native:
  result=subprocess.run([str(a.native.resolve()),str(out.resolve())],text=True,capture_output=True);report.update(native_binary_sha256=sha(a.native.read_bytes()),native_returncode=result.returncode,native_stdout=result.stdout,native_stderr=result.stderr)
 (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');out.with_suffix('.provenance.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k!='cases'},indent=2))
 if a.native and result.returncode:raise SystemExit(result.returncode)
if __name__=='__main__':main()

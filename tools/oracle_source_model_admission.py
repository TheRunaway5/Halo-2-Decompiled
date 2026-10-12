#!/usr/bin/env python3
"""Whole unchanged4BCC0 over actual main model-admission inputs.

Native4BCC0 joins raw B9CE0/3D3E0/4C0D0, geometry-LOD choice and native3DDD0
cache acquisition.12DE70 resource andBAB40 scalar results remain explicit
ordered owner boundaries. Every original callee executes unchanged. Current
bank controls are declared; this is not live4B5D0/node/framebuffer parity.
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
def bits(v):return u(struct.pack('<f',v))
def sha(v):return hashlib.sha256(v).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--snapshot',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--native',type=Path);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
 s=Snapshot(a.snapshot);assert sha(s.ram)==PIN;x=Xbe(ROOT/'assets/disc/default.xbe');assert sha(x.data)=='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
 stack,stop=0x700fe000,0x700fff00;executed={};entries=[]
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
  if pc!=0x4bcc0:return
  sp=vm.reg_read(R.UC_X86_REG_ESP);out=vm.reg_read(R.UC_X86_REG_EDI);read(vm,out,0x1e4)
  entries.append((out,sp,words(read(vm,sp+4,24)),[(b,e,perms,bytes(vm.mem_read(b,e-b+1)))for b,e,perms in vm.mem_regions()]))
 g.hook_add(UC_HOOK_CODE,unchanged);g.hook_add(UC_HOOK_CODE,capture)
 write(g,0x4c0b80,pack([0x547f88]));run(g,0x2cbc0,stack,[0]);assert entries
 variants=[('actual main model admission',dict(index=i))for i in range(len(entries))]
 variants += [('caller mode',dict(mode=n))for n in [0,1,2,255,256,257,NONE]]
 variants += [('first person and force low byte',dict(fp=1,force=n))for n in [0,1,255,256,257,NONE]]
 variants += [('signed first-person model count',dict(fp=1,fp_count=n))for n in [-32768,-1,0,1,2,4,32767]]
 variants += [('parent/header/body visibility',dict(hidden=k,fp=f))for k in ['header','body','parent','header_null_output']for f in [0,1]]
 variants += [('absent model chain',dict(absent=k,fp=f))for k in ['hlmt','mode']for f in [0,1]]
 variants += [('actual geometry metric and view',dict(metric=bits(n),view=v,modifier=m))for n in [-1.,0.,1.,2.,4.,8.,10.,30.]for v in [0,1,2,3,-1,4]for m in [0,1]]
 variants += [('cached LOD hysteresis and cap',dict(metric=bits(n),prior=p,maximum=c))for n in [1.,4.,8.,16.]for p in [-1,0,1,2,3,4,5]for c in [0,2,4]]
 variants += [('force-highest and control metric scale',dict(force_lod=f,control=c,modifier=m))for f in [0,1]for c in [0,1]for m in [0,1]]
 variants += [('unordered metric and first threshold',dict(metric=n,bank=b))for n in [0x7fc01234,0x7f800000,0xff800000,0,0x80000000]for b in ['valid','nan_first','nan_threshold']]
 variants += [('signed ordinary cap',dict(metric=bits(2.),maximum=n))for n in [-2,-1,0,1,5,6]]
 variants += [('overlay shader and camouflage fields',dict(overlay=True,fp=f,mode=c,model_flags=mf,recent=r,shield=h,camouflage=z))for f in [0,1]for c in [0,1]for mf in [0,1,2,4,7]for r,h,z in [(0.,1.,0.),(1.,.5,.7),(1.,2.,1.)]]
 variants += [('parallel region sort bank',dict(sort=True,variant=n,sort_count=c))for n in [-128,-1,0,1,127]for c in [0,1,2,16,80]]
 variants += [('declared PRT resource admission',dict(prt_count=n,prt_ready=r,fp=f,mode=c))for n in [-1,0,1]for r in [0,1]for f in [0,1]for c in [0,1]]
 variants += [('native prior LOD cache allocation',dict(cache_mode=c,view=v))for c in ['empty','hole','pressure']for v in range(4)]
 fixture=[0x41444d53,1,len(variants)];cases=[]
 for label,v in variants:
  out,sp,args,regions=entries[v.get('index',0)];vm=clone(regions);args=list(args);owner,metric,mode,fp,force,lod=args
  tags,objects,cache_pool=val(vm,0x4e3b44),val(vm,0x4e0300),val(vm,0x509434)
  records=val(vm,cache_pool+0x44);bitbank=val(vm,cache_pool+0x48)
  def metadata(tag):return val(vm,tags+(tag&65535)*16+8)
  headers=val(vm,objects+0x44);header=headers+(owner&65535)*12;body=val(vm,header+8)
  definition=metadata(val(vm,body));m=metadata(val(vm,definition+0x38))
  if 'mode'in v:args[2]=v['mode']
  if 'metric'in v:args[1]=v['metric']
  if 'force'in v:args[4]=v['force']
  if 'fp'in v:write(vm,0x4c1bd0,pack([owner if v['fp']else NONE]))
  if 'fp_count'in v:write(vm,0x4c1bd4,struct.pack('<h',v['fp_count']))
  if 'modifier'in v:write(vm,0x4ba019,bytes([v['modifier']]))
  if 'view'in v:write(vm,0x4ba050,pack([v['view']]))
  if 'maximum'in v:write(vm,0x4ba01c,pack([v['maximum']]))
  if 'prior'in v:
   token=val(vm,body+0xcc);assert token!=NONE
   write(vm,records+(token&65535)*256+0xbc,bytes([v['prior']&255])*4)
  if 'force_lod'in v:
   flags=val(vm,body+4);write(vm,body+4,pack([(flags&~0x80000)|(0x80000 if v['force_lod']else 0)]))
   flags=read(vm,body+0x10a,1)[0];write(vm,body+0x10a,bytes([(flags&~4)|(4 if v['control']else 0)]))
  if 'bank'in v:
   bank=[bits(n)for n in [30.,0.,0.,16.,12.,8.,4.,1.,0.]]
   if v['bank']=='nan_first':bank[0]=0x7fc05678
   if v['bank']=='nan_threshold':bank[3]=0x7fc05678
   write(vm,m+0x28,pack(bank))
  if 'hidden'in v:
   if v['hidden'] in ['header','header_null_output']:
    flags=read(vm,header+2,1)[0];write(vm,header+2,bytes([flags|16]))
    if v['hidden']=='header_null_output':args[3]=0
   elif v['hidden']=='body':write(vm,body+4,pack([val(vm,body+4)|1]))
   else:
    other=entries[1][2][0];assert other!=owner
    parent_body=val(vm,headers+(other&65535)*12+8)
    write(vm,body+0x14,pack([other]));write(vm,parent_body+4,pack([val(vm,parent_body+4)|1]))
  if 'absent'in v:write(vm,definition+0x38 if v['absent']=='hlmt'else m+4,pack([NONE]))
  if v.get('overlay'):
   raster=val(vm,0x485a80);shader=val(vm,raster+0xd0)
   write(vm,m+0x9c,pack([v['model_flags']]))
   write(vm,body+0xf4,pack([bits(v['recent'])]));write(vm,body+0xf0,pack([bits(v['shield'])]))
   write(vm,body+0x2b0,pack([bits(v['camouflage'])]));write(vm,body+0xaa,b'\0')
   table=0x70018000;write(vm,table,pack([shader])*64);write(vm,m+0x60,pack([1,table]))
   write(vm,m+0xf4,pack([shader,0]));write(vm,body+0xc0,pack([val(vm,body+0xc0)&~0x200]))
  if v.get('sort'):
   write(vm,body+0xb1,struct.pack('<b',v['variant']))
   table,palette=0x7000c000,0x70014000
   write(vm,table-128*0x38,bytes([255])*(256*0x38));write(vm,palette-128*20,pack([0x56781234])*(256*5))
   entry=table+v['variant']*0x38
   write(vm,entry+0x18,pack([palette]));write(vm,m+0x54,pack([table]));write(vm,m+0x70,pack([v['sort_count']]))
  if 'prt_count'in v:
   mode_bank=metadata(val(vm,m+4));sections=val(vm,mode_bank+0x28)
   published=[sections+i*0x5c+0x38 for i in range(val(vm,mode_bank+0x24))if read(vm,sections+i*0x5c+0x38+0x1e,1)!=b'\0']
   assert published,'No published model resource for declared PRT header control'
   # This is a declared resource-header bank, not authored PRT payload or
   # live publication. The unchanged original12DE70 supplies its true return.
   table=0x70020000;resource=table+0x34
   write(vm,resource,read(vm,published[0],0x24));write(vm,mode_bank+0x74,pack([v['prt_count'],table]))
   if not v['prt_ready']:write(vm,resource+0x1e,b'\0');write(vm,resource+8,pack([0]))
  if 'cache_mode'in v:
   write(vm,records,pack([0xa53c7e91])*16384);write(vm,cache_pool+0x34,pack([0,0,0,0xa5a5e163]))
   write(vm,bitbank,bytes(32));write(vm,body+0xcc,pack([NONE]))
   if v['cache_mode']in ['hole','pressure']:
    occupied=[NONE]*8
    if v['cache_mode']=='hole':occupied[77//32]^=1<<(77&31)
    write(vm,bitbank,pack(occupied));write(vm,cache_pool+0x34,pack([37 if v['cache_mode']=='hole'else 256,256,sum(n.bit_count()for n in occupied)]))
    for i in range(256):write(vm,records+i*256,pack([0x5a5a0000|((0xe163+i)&65535),owner^0x10000,0x12345678,(val(vm,0x4ba034)-[1,1000,1001,-1,0][i%5])&NONE]))
  view=val(vm,0x4ba050);frame=val(vm,0x4ba034);fp_owner=val(vm,0x4c1bd0);fp_count=struct.unpack('<h',read(vm,0x4c1bd4,2))[0]
  modifier=read(vm,0x4ba019,1)[0];maximum=val(vm,0x4ba01c);raster=val(vm,0x485a80)
  owned=[(out,0x1e4),(records,256*256),(cache_pool,0x4c),(bitbank,32),(body+0xcc,4),(args[5]&~3,4)]
  if args[3]:owned.append((args[3]&~3,4))
  for at,n in owned:read(vm,at,n)
  initial_pages={at:bytes(vm.mem_read(at,4096))for b,e,_ in vm.mem_regions()for at in range(b,e+1,4096)}
  def initial_read(at,n):
   raw=b''
   while n:
    page=at&~4095;count=min(n,4096-(at&4095));data=initial_pages.get(page)
    if data is None:data=s.read(page,4096)
    raw+=data[at-page:at-page+count];at+=count;n-=count
   return raw
  ranges=[(at&~3,(at+n+3)&~3)for at,n in owned];events=[];pending={}
  def access(_guest,_access,at,n,_value,_data):ranges.append((at&~3,(at+n+3)&~3))
  vm.hook_add(UC_HOOK_MEM_READ,access);vm.hook_add(UC_HOOK_MEM_WRITE,access)
  def observe(guest,pc,_size,_):
   if pc in pending:
    row,scalar=pending.pop(pc);row.append(guest.reg_read(R.UC_X86_REG_EAX)&(NONE if row[0]==1 else 255))
    if scalar:row.append(val(guest,scalar))
   if pc in [0x12de70,0x3ddd0,0xbab40]:
    esp=guest.reg_read(R.UC_X86_REG_ESP);scalar=0
    if pc==0x12de70:row=[0,guest.reg_read(R.UC_X86_REG_EAX),guest.reg_read(R.UC_X86_REG_ECX)]
    elif pc==0x3ddd0:row=[1,val(guest,esp+4)]
    else:row=[2,val(guest,esp+4),val(guest,esp+8)];scalar=val(guest,esp+12)
    events.append(row);pending[val(guest,esp)]=(row,scalar)
  vm.hook_add(UC_HOOK_CODE,observe)
  try:run(vm,0x4bcc0,sp,args,{R.UC_X86_REG_EDI:out})
  except Exception as error:raise RuntimeError(f'Case{len(cases)} {label} {v}, PC{vm.reg_read(R.UC_X86_REG_EIP):08x}: {error}')from error
  assert not pending
  result=vm.reg_read(R.UC_X86_REG_EAX)&65535
  inputs=[*args,out,tags,objects,cache_pool,frame,view,fp_owner,fp_count&NONE,modifier,maximum,raster,len(owned),*[w for at,n in owned for w in [at,n//4]]]
  merged=[]
  for start,end in sorted(ranges):
   if merged and start<=merged[-1][1]:merged[-1][1]=max(merged[-1][1],end)
   else:merged.append([start,end])
  inputs.append(len(merged))
  for start,end in merged:inputs.extend([start,(end-start)//4,*words(initial_read(start,end-start))])
  inputs.append(len(events))
  for row in events:inputs.extend([len(row),*row])
  after=[result,*[w for at,n in owned for w in words(read(vm,at,n))]]
  fixture.extend([len(inputs),len(after),*inputs,*after]);cases.append(dict(label=label,mutation=v,result=result,events=len(events),event_kinds=[sum(row[0]==k for row in events)for k in range(3)],checked_words=len(after)))
 out=a.output/'fixture.bin';out.write_bytes(pack(fixture));report=dict(boundary=__doc__,snapshot_sha256=PIN,original_xbe_sha256=sha(x.data),declared_main_context='00547f88',original_inputs=len(entries),intercepted_instructions=0,executed_instructions=len(executed),executed_instruction_bank_sha256=sha(b''.join(pack([pc])+raw for pc,raw in sorted(executed.items()))),cases=cases,checked_words=sum(c['checked_words']for c in cases),events=sum(c['events']for c in cases),event_kinds=[sum(c['event_kinds'][k]for c in cases)for k in range(3)],fixture_sha256=sha(out.read_bytes()),tool_sha256=sha(Path(__file__).read_bytes()))
 if a.native:
  result=subprocess.run([str(a.native.resolve()),str(out.resolve())],text=True,capture_output=True);report.update(native_binary_sha256=sha(a.native.read_bytes()),native_returncode=result.returncode,native_stdout=result.stdout,native_stderr=result.stderr)
 (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');out.with_suffix('.provenance.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k!='cases'},indent=2))
 if a.native and result.returncode:raise SystemExit(result.returncode)
if __name__=='__main__':main()

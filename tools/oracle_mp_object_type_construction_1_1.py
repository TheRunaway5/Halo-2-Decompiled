#!/usr/bin/env python3
"""Observe unchanged type preparation/construction on genuine allocated units."""
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_EIP,UC_X86_REG_ESP
import oracle_mp_spawn_player_visibility as owning
from functions import _trace
ROOT=owning.ROOT
OWN=(0x10ca30,0x10cbd0,0x10cc20)
RETURNS={0x10ca30:4,0x10cbd0:0,0x10cc20:8}
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def save(p,v):
 b=(json.dumps(v,sort_keys=True,indent=2)+'\n').encode();p.parent.mkdir(parents=True,exist_ok=True)
 if p.exists():assert p.read_bytes()==b,p
 else:p.write_bytes(b)
class TypeTrace(owning.PlayerTrace):
 def __init__(self,*a,**k):
  self.type_enabled=False;self.type_calls=[];self.type_pending=[];self.type_order=0;self.type_executed=set();self.type_control=None;super().__init__(*a,**k)
  self.type_routines={e:_trace(self.code,e,self.starts|set(OWN))for e in OWN};self.type_addresses={a for f in self.type_routines.values()for a in f.code}
  self.g.hook_add(unicorn.UC_HOOK_MEM_READ,self.type_read)
 def type_read(self,g,access,p,n,value,user):
  if self.type_enabled and g.reg_read(UC_X86_REG_EIP) in self.type_addresses:
   for _,_,row in self.type_pending:
    if int(row['entry'],16) in OWN:row.setdefault('reads',[]).append(dict(instruction=f'{g.reg_read(UC_X86_REG_EIP):08x}',address=f'{p:08x}',bytes=n,raw_hex=self.raw(p,n)))
 def raw(self,p,n):return bytes(self.g.mem_read(p,n)).hex()
 def type_metadata(self):
  ranges={};types=[]
  def retain(p,n):
   v=self.raw(p,n);key=f'{p:08x}';assert key not in ranges or ranges[key]==v;ranges[key]=v
  retain(0x46fad0,13*4)
  for i in range(13):
   p=self.u(0x46fad0+i*4);retain(p,8);bank=[]
   for j in range(100):
    slot=self.u(p+0x84+j*4);bank.append(f'{slot:08x}')
    if not slot:break
    retain(slot+0x28,8)
   else:raise AssertionError('Original type bank has no bounded sentinel')
   retain(p+0x84,len(bank)*4);types.append(dict(type=i,address=f'{p:08x}',bank=bank))
  return dict(bank='0046fad0',types=types,ranges=ranges)
 def buffers(self,row):
  pool=self.u(0x4e78d0);heads=self.u(pool+0x44);cap=self.u(pool+0x20);stride=self.u(pool+0x24);assert stride==12
  result=dict(P=self.raw(pool,0x4c),K=self.raw(heads,cap*stride),W=self.raw(self.u(0x4e58c4),0x84))
  for i in range(self.u(pool+0x38)):
   h=heads+i*12;p=self.u(h+8);n=self.u(h+6,2)
   if p and n:result[f'U{i}']=self.raw(p,n)
  if row.get('descriptor'):result['D']=self.raw(row['descriptor'],0xc4)
  if row.get('failure'):result['F']=self.raw(row['failure'],1)
  return result
 def finish_type(self,a):
  sp=self.g.reg_read(UC_X86_REG_ESP)
  for q in reversed(self.type_pending[:]):
   ret,esp,row=q
   if ret==a and sp>esp:
    row.update(after=self.buffers(row),return_eax=f'{self.g.reg_read(UC_X86_REG_EAX):08x}',return_order=self.type_order);self.type_order+=1;self.type_pending.remove(q)
 def hook(self,g,a,size,user):
  if self.type_enabled:
   self.finish_type(a)
   if a in self.type_addresses:self.type_executed.add(a)
   sp=g.reg_read(UC_X86_REG_ESP);ret=self.u(sp)
   callback=ret in(0x10cbff,0x10cc76) and not any(q[0]==ret and q[1]==sp for q in self.type_pending)
   if a in OWN or callback:
    r={k:int(v,16)for k,v in self.registers().items()};stack=[self.u(sp+4+i*4)for i in range(3)];row=dict(id=len(self.type_calls),entry=f'{a:08x}',caller=f'{ret:08x}',phase=self.active,registers=r,stack=[f'{v:08x}'for v in stack],order=self.type_order,ancestors=[q[2]['id']for q in self.type_pending])
    parent=next((q[2]for q in reversed(self.type_pending)if int(q[2]['entry'],16)in(0x10cbd0,0x10cc20)),None)
    if parent:
     for name in('descriptor','failure','unit'):
      if name in parent:row[name]=parent[name]
    if a==0x10cbd0:row.update(descriptor=r['ebx'])
    elif a==0x10cc20:row.update(unit=r['ebx'],descriptor=stack[0],failure=stack[1])
    elif a==0x10ca30:
     index=struct.unpack('<h',struct.pack('<H',stack[0]&65535))[0];group=self.u(self.u(0x4eb114)+index*16);row.update(definition=f'{stack[0]:08x}',signed_index=index,tag_group=f'{group:08x}')
    row['before']=self.buffers(row);self.type_calls.append(row);self.type_pending.append((ret,sp,row));self.type_order+=1
    if callback and self.type_control is not None:
     value=self.type_control.pop(0);row['declared_callback_al']=value
     g.reg_write(UC_X86_REG_EAX,(g.reg_read(UC_X86_REG_EAX)&0xffffff00)|value);g.reg_write(UC_X86_REG_ESP,sp+(16 if ret==0x10cc76 else 8));g.reg_write(UC_X86_REG_EIP,ret);return
  super().hook(g,a,size,user)
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True);args=ap.parse_args();out=args.output.resolve();out.mkdir(parents=True,exist_ok=True)
 for p,h in owning.FROZEN.items():assert sha(ROOT/p)==h,p
 with(out/'host-provider.stderr.log').open('w')as err:
  proc=subprocess.Popen(['wine',str(owning.CONTEXT_OUT/'generator.exe')],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=err,text=True,env=dict(os.environ,WINEDEBUG='-all',WINEPREFIX=str(owning.CONTEXT_OUT/'wine-prefix')))
  try:
   data=json.loads(proc.stdout.readline());t=owning.prepare(data,trace_type=TypeTrace);assert t.player_stages[-1]['completion']=='complete_original_return';t.type_enabled=True
   assert owning.spawn_and_activate(t,activate=False);t.finish_type(owning.STOP);assert not t.type_pending
   baseline=t.state();old=json.loads((ROOT/'analysis/multiplayer/integration/spawn-source/player-visibility/report.json').read_text());assert t.player_stages==old['cases'][2]['stages'][:4]
   groups={}
   for tag in t.cache.tags.values():
    datum=int(tag['datum'],16);group=t.u(t.u(0x4eb114)+(datum&65535)*16)
    groups.setdefault(group,datum)
   matches={t.u(t.u(0x46fad0+i*4)+4)for i in range(13)}
   selected=[groups[q]for q in sorted(groups)if q in matches];other=next(v for k,v in sorted(groups.items())if k not in matches);selected.append(other)
   for datum in selected:
    t.active=f'explicit_original_type_query_{datum:08x}';t.run(0x10ca30,(datum,));t.finish_type(owning.STOP);assert not t.type_pending
   assert t.state()==baseline
   natural=list(t.type_calls)
   constructor=next(q for q in natural if q['entry']=='0010cc20')
   control=0x6e000000;t.g.mem_map(control,0x1000)
   t.g.mem_write(control,bytes.fromhex(constructor['before']['D']));t.g.mem_write(control+0xc4,bytes.fromhex(constructor['before']['F']))
   for name,values in [('first_false',[0]),('second_false',[2,0]),('nonzero_normalization',[255,128])]:
    before=t.state();t.active='explicit_callback_al_boundary_'+name;t.type_control=list(values)
    t.run(0x10cc20,(control,control+0xc4),((UC_X86_REG_EBX,constructor['unit']),));t.finish_type(owning.STOP)
    assert not t.type_pending and not t.type_control and t.state()==before
   t.type_control=None
   metadata=t.type_metadata();listings=[]
   for e,f in t.type_routines.items():
    ins=[t.code.at(a)for a in sorted(f.code)]
    for i in ins:assert t.image.read(i.address,len(i.bytes))==i.bytes and bytes(t.g.mem_read(i.address,len(i.bytes)))==i.bytes
    p=out/'instructions'/f'{e:08x}.asm';p.parent.mkdir(exist_ok=True);p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins));listings.append(dict(entry=f'{e:08x}',rows=len(ins),executed_rows=sum(i.address in t.type_executed for i in ins),sha256=sha(p)))
   save(out/'report.json',dict(schema='source-object-type-construction-v1',tool_sha256=sha(__file__),image_sha256=sha(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'),map_sha256=sha(ROOT/'assets/disc/maps/lockout.map'),calls=t.type_calls,metadata=metadata,source_stages=t.player_stages,source_state=baseline,listings=listings,new_guest_inputs=['Separate control roots copy the literal source-produced C4 request and failure byte into host-owned scratch; actual source-created unit/body remain unchanged.'],new_success_returns=['Three separately declared callback-AL-only controls [0], [2,0], [255,128]; all natural callbacks run unchanged.'],instruction_patches=[],boundaries=['Immutable PlayerTrace original preparation and exact4 owning stages retained. Platform/context/options/property/resource delivery remain inherited explicit boundaries.','Actual natural B8A30 callers invoke unchanged10CBD0/10CC20; every nonNULL original type callback executes unchanged. Extra10CA30 roots use actual authored tag identities only.','Full captured pool/header/body/request/failure outputs are differential evidence, never a live final Unit restore. Complete generic B8A30 and lower type/world/resource ownership remain required.']))
   print(json.dumps(dict(report=str(out/'report.json'),sha256=sha(out/'report.json'),calls=len(t.type_calls),wrappers=sum(int(c['entry'],16)in(0x10cbd0,0x10cc20)for c in t.type_calls))),flush=True)
  finally:
   if proc.poll()is None:
    proc.stdin.write('x');proc.stdin.flush();proc.stdin.close();proc.wait(timeout=60)
   if proc.stdout:proc.stdout.close()
   assert proc.returncode==0,proc.returncode
if __name__=='__main__':main()

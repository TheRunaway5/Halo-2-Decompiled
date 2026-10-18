#!/usr/bin/env python3
"""Unchanged TU1.5 scripted request, spatial gains and stereo routing.

Actual opening sound/class/mixer metadata, declared mathematical listener and
world-point inputs. Stops before source submission, effect processing and the
first SDK call. No readiness, voice admission or platform return is supplied.
"""
import argparse,hashlib,json,struct,sys
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_INVALID,UC_MEM_READ_UNMAPPED,UC_MEM_WRITE,UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_sound_control import Xbe,Cache,NEW

def bits(x):return struct.unpack('<I',struct.pack('<f',x))[0]
def real(x):return struct.unpack('<f',struct.pack('<I',x))[0]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
 x=Xbe(NEW);pin=hashlib.sha256(x.data).hexdigest();assert pin=='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
 path=Path('assets/disc/maps/00a_introduction.map')
 with path.open('rb')as f:map_pin=hashlib.file_digest(f,'sha256').hexdigest()
 assert map_pin=='5093e2f8ef7f0df41c11c6621bac5785a36f78a1cff3b8b2bc6c14b561c02a28'
 c=Cache(path);g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x800000)
 for s in x.sections:g.mem_write(s.va,x.section_bytes(s))
 table,base,stack,stop=0x1000000,0x10e0000,0x10f0000,0x10fe000;g.mem_map(table,0x100000)
 def put(a,b):g.mem_write(a,b)
 def w(a,v,n=4):put(a,(v&((1<<(n*8))-1)).to_bytes(n,'little'))
 def u(a,n=4):return int.from_bytes(g.mem_read(a,n),'little')
 def f(a):return real(u(a))
 pages=set();executed=set();active=set();recording=False;initial={};stores=[];native_cases=[]
 native=a.output/'native';native.mkdir()
 register_names=('EAX','ECX','EDX','EBX','ESP','EBP','ESI','EDI')
 def registers():return [g.reg_read(getattr(R,'UC_X86_REG_'+n)) for n in register_names]
 def xmm():return [list(struct.unpack('<4I',g.reg_read(getattr(R,f'UC_X86_REG_XMM{i}')).to_bytes(16,'little')))for i in range(8)]
 def access(machine,kind,address,size,value,user):
  if not recording:return
  before=bytes(machine.mem_read(address,size))
  for i,b in enumerate(before):initial.setdefault(address+i,b)
  if kind==UC_MEM_WRITE:stores.append(dict(pc=machine.reg_read(R.UC_X86_REG_EIP),address=address,before=before.hex(),written=(value&((1<<(8*size))-1)).to_bytes(size,'little').hex()))
 def banks(data):
  result={};start=None;values=bytearray();last=None
  for address,b in sorted(data.items()):
   if last is not None and address!=last+1:result[f'{start:08x}']=values.hex();values=bytearray();start=None
   if start is None:start=address
   values.append(b);last=address
  if start is not None:result[f'{start:08x}']=values.hex()
  return result
 def fault(machine,access,address,size,value,user):
  if access!=UC_MEM_READ_UNMAPPED:return False
  page=address&~4095;offset=page-c.magic
  if offset<0 or offset+4096>len(c.data)or page in pages:return False
  machine.mem_map(page,4096);machine.mem_write(page,bytes(c.data[offset:offset+4096]));pages.add(page);return True
 def ins(machine,pc,n,user):
  assert bytes(machine.mem_read(pc,n))==x.read(pc,n),(hex(pc),'instruction mismatch')
  executed.add(pc);active.add(pc)
 g.hook_add(UC_HOOK_MEM_INVALID,fault);g.hook_add(UC_HOOK_CODE,ins)
 g.hook_add(UC_HOOK_MEM_READ|UC_HOOK_MEM_WRITE,access)
 for datum,t in c.tags.items():
  if t['offset']is not None:w(table+(datum&65535)*16+8,t['offset']+c.magic)
 addr=lambda group:next(t['offset']+c.magic for t in c.tags.values()if t['group']==group)
 matg,ugh=addr('matg'),addr('ugh!')
 for va,value in((0x4f0414,table),(0x4ecc1c,matg),(0x52d664,ugh),(0x4f2c48,base),(0x4f2c4c,base+0x100),(0x4f2c50,base+0x1000)):w(va,value)
 source=base+0x200;w(base+0x144,source)
 listener=base+0x1000+0x90
 for i in range(13):w(listener+i*4,bits(1)if i in(0,1,5,9)else 0)
 def run(entry,end=stop,args=(),regs=()):
  nonlocal recording
  active.clear();put(stack,struct.pack('<'+'I'*(1+len(args)),stop,*args))
  for reg,v in((R.UC_X86_REG_ESP,stack),(R.UC_X86_REG_EFLAGS,2),(R.UC_X86_REG_FPCW,0x027f),(R.UC_X86_REG_FPSW,0),(R.UC_X86_REG_FPTAG,0xffff),(R.UC_X86_REG_MXCSR,0x1f80),*regs):g.reg_write(reg,v)
  gp_before,xmm_before=registers(),xmm();initial.clear();stores.clear();recording=True
  try:g.emu_start(entry,end,count=20000)
  except Exception as error:raise RuntimeError(f'Original {entry:08x} stopped at {g.reg_read(R.UC_X86_REG_EIP):08x}: {error}')from error
  finally:recording=False
  assert g.reg_read(R.UC_X86_REG_EIP)==end,hex(g.reg_read(R.UC_X86_REG_EIP))
  if entry in(0x12c5b0,0x12ccc0,0x2b8650,0x2b92e0):
   name=f'{len(native_cases):04d}-{entry:08x}'
   before=banks(initial);after={at:bytes(g.mem_read(int(at,16),len(raw)//2)).hex()for at,raw in before.items()}
   row=dict(name=name,entry=entry,gp=gp_before,xmm=xmm_before,gp_after=registers(),xmm_after=xmm(),eflags_after=g.reg_read(R.UC_X86_REG_EFLAGS),before=before,after=after,stores=list(stores),external_targets=[]if end==stop else[end],external_boundary=[]if end==stop else[end])
   (native/(name+'.json')).write_text(json.dumps(row,separators=(',',':'))+'\n');native_cases.append(dict(file=name+'.json'))
  return len(active)
 # Original constructor copies the map's 64-byte snmx mixer controls before
 # its first SDK call. Retain that output without supplying the call's result.
 globals_pointer=c.u(matg-c.magic+0xc4)-c.magic
 mixer_datum=c.u(globals_pointer+0x14);mixer=c.tags[mixer_datum]['offset']
 mixer_raw=bytes(c.data[mixer+0x18:mixer+0x58]);put(base+0x2000,mixer_raw)
 run(0x2b8650,0x3866c4,(base+0x2000,base+0x2100,0),((R.UC_X86_REG_ECX,0x48b8a0),))
 assert bytes(g.mem_read(0x48b8a4,64))==mixer_raw
 rows=[];lines=['OPENING_SCRIPTED_ROUTING_TU15_V1']
 for line in Path('tests/fixtures/opening_dialogue_timing_tu15.tsv').read_text().splitlines():
  script,tick,datum=line.split();datum=int(datum,16);tag=c.tags[datum];sound=tag['offset'];cl_index=c.data[sound+2]
  # The reported voices all use this actual cinematic dialogue class. Other
  # classes need their own original routing branch and are not inferred here.
  if cl_index!=48:continue
  for distance in(1.,17.5,25.):
   request=base+0x3000;put(request,b'\0'*0x58)
   w(request+3,0x11,1);w(request+4,bits(1));w(request+0xc,bits(distance));w(request+0x44,0xffffffff);w(request+0x48,0xffffffff)
   run(0x18cdd0,0x223b20,(datum,),((R.UC_X86_REG_EAX,request),))
   raw_address=g.reg_read(R.UC_X86_REG_EAX);raw=bytes(g.mem_read(raw_address,160))
   assert int.from_bytes(raw[4:6],'little')==3 and int.from_bytes(raw[12:14],'little')&0x800
   put(source,b'\0'*0xbc);w(source+6,int.from_bytes(raw[4:6],'little'),2);w(source+0xc,datum);put(source+0x18,raw[12:80]);w(base,0xe1230000)
   gain,out=base+0x3100,base+0x3200;w(gain,0);put(out,b'\0'*56);w(base+0xa,0,1)
   run(0x12c5b0,args=(gain,out),regs=((R.UC_X86_REG_EAX,0),))
   spatial=bytes(g.mem_read(out,56));assert u(gain)==0
   driver=base+0x3300;put(driver,b'\0'*0x500);w(source+0xa4,0xffff,2);w(base+0xe,0xffff,2)
   run(0x12ccc0,0x229180,(source,),((R.UC_X86_REG_EAX,0),(R.UC_X86_REG_EBX,out),(R.UC_X86_REG_EDI,driver)))
   # The original226490 routing-packet assembly, before dispatch/SDK effects.
   w(stack-0x230,0);record=base+0x3900
   run(0x2266da,0x226905,regs=((R.UC_X86_REG_EDI,driver),(R.UC_X86_REG_EBX,record),(R.UC_X86_REG_EDX,0)))
   routing=bytes(g.mem_read(stack+0x1c,24));routing_address=base+0x3a00;put(routing_address,routing)
   outputs=[]
   for policy in(0,1):
    for channels in(0,1):
     bins=base+0x3b00;put(bins,b'\0'*0x44)
     run(0x2b92e0,args=(routing_address,policy,channels,bins),regs=((R.UC_X86_REG_ECX,0x48b8a0),))
     count=u(bins+0x40);pairs=[struct.unpack('<Ii',bytes(g.mem_read(bins+8*i,8)))for i in range(count)]
     outputs.append(dict(policy=policy,channels=channels,pairs=pairs))
   row=dict(script=int(script),tick=int(tick),datum=datum,name=tag['name'],distance=distance,raw_request=raw.hex(),spatial=spatial.hex(),routing=routing.hex(),outputs=outputs)
   rows.append(row)
   lines.append(f'{datum:08x} {bits(distance):08x} {u(out+8):08x} {outputs[0]["pairs"][0][1]} {outputs[0]["pairs"][1][1]}')
 assert rows
 (a.output/'fixtures.tsv').write_text('\n'.join(lines)+'\n')
 report=dict(scope=__doc__,xbe_sha256=pin,map_sha256=map_pin,mixer_datum=f'{mixer_datum:08x}',mixer64=mixer_raw.hex(),cases=rows,executed_instruction_count=len(executed),executed_instructions=[f'{v:08x}'for v in sorted(executed)],instruction_patches=[],supplied_callee_returns=[],full_engine_verified=False)
 (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
 (native/'report.json').write_text(json.dumps(dict(suite='audio_routing',xbe_sha256=pin,cases=native_cases,external_targets=[],instruction_patches=[],supplied_external_results=[]),indent=2)+'\n')
 print(f'PASS {len(rows)} actual cinematic dialogue metadata/declared world-point cases; {len(executed)} unchanged instructions; no supplied returns')
 print('First route:',rows[0]['outputs'])
if __name__=='__main__':main()

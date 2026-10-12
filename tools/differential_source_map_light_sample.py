#!/usr/bin/env python3
"""Whole unchanged D1E10 MAP calls on immutable actual ready geometry/bitmap banks.

Baseline inputs are the independently completed D2A50 diagnostic contacts.
Part-type controls change only the literal two-byte metadata in the disposable
clone. No source math, bitmap, residency, lookup or sample result is replaced.
"""
from pathlib import Path
import collections,hashlib,json,re,struct,sys,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_UNMAPPED,UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
from differential_source_object_admission_snapshot import Snapshot,PIN,uint,packed,words
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'));from xbe import Xbe
SYNTH,STACK,STOP,P,OUT=0x20000000,0x200fe000,0x200ff000,0x20000100,0x20000300
def main():
 source_report=ROOT/'analysis/multiplayer-shading/object-light-rays-original.json'
 parent=json.loads(source_report.read_text());snapshot=Snapshot(ROOT/'analysis/multiplayer-shading/registration-snapshot-restore-20261008');image=Xbe(ROOT/'assets/disc/default.xbe')
 assert hashlib.sha256(image.data).hexdigest()==PIN and parent['source_xbe_sha256']==PIN
 u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(SYNTH,0x100000);pages={};dirty=set();verified=set();calls=collections.Counter();observed={};pending_uv=None;forced_type=None;surfaces={}
 def page(address):
  p=address&~4095
  if p not in pages:raw=snapshot.read(p,4096);u.mem_map(p,4096);u.mem_write(p,raw);pages[p]=raw
 def read(address,size):
  for p in range(address&~4095,((address+size-1)&~4095)+4096,4096):
   if not SYNTH<=p<SYNTH+0x100000:page(p)
  return bytes(u.mem_read(address,size))
 def unmapped(g,access,address,size,value,data):
  for p in range(address&~4095,((address+size-1)&~4095)+4096,4096):page(p)
  return True
 def code(g,address,size,data):
  nonlocal pending_uv
  if pending_uv and address==pending_uv[0]:observed['uv']=words(read(pending_uv[1],4))+words(read(pending_uv[2],4));pending_uv=None
  if address==0xd1e65:
   part=g.reg_read(UC_X86_REG_EAX);observed['part_pointer']=part;observed['original_part_type']=struct.unpack('<H',read(part,2))[0]
   if forced_type is not None:u.mem_write(part,struct.pack('<H',forced_type));dirty.add(part&~4095)
   observed['part_type']=struct.unpack('<H',read(part,2))[0]
  if address==0xd2dc0:pending_uv=(uint(read(g.reg_read(UC_X86_REG_ESP),4)),g.reg_read(UC_X86_REG_EBX),g.reg_read(UC_X86_REG_EDI))
  if address==0xd31c0:
   geom=uint(read(g.reg_read(UC_X86_REG_ESP)+4,4));header=read(geom,0x40);count=uint(header[0x38:0x3c]);desc=uint(header[0x3c:0x40]);selected=None;streams=[]
   for i in range(count):
    raw=read(desc+i*32,32);streams.append({'raw':raw.hex(),'pointer':uint(raw[12:16])})
    if i>=3 and selected is None and raw[0]==48:selected=i
   if selected is None and count>3:selected=count-1
   indices=read(uint(header[0x24:0x28])+g.reg_read(UC_X86_REG_EBX)*6,6)
   observed['incident_geometry']={'pointer':geom,'header':header.hex(),'streams':streams,'selected':selected,'indices':indices.hex()}
   if selected is not None:
    data=streams[selected]['pointer'];observed['incident_geometry']['packed']=[uint(read(data+i*4,4))for i in struct.unpack('<3H',indices)]
  if address==0xd30d0:observed['bitmap']=g.reg_read(UC_X86_REG_EAX)
  if address==0xd15e0:
   p=g.reg_read(UC_X86_REG_EAX);observed.setdefault('runtime_records',[]).append({'pointer':p,'raw':read(p,116).hex()})
  if address==0x1d2f0:
   p=uint(read(g.reg_read(UC_X86_REG_ESP)+4,4));observed.setdefault('gpu_surfaces',[]).append({'pointer':p,'prefix':read(p,0x20).hex()})
  if address==0x1362b0:
   record=g.reg_read(UC_X86_REG_EAX);header=read(record,116);entry={'record':record,'prefix':header[:0x18].hex(),'header':header.hex(),'detail':g.reg_read(UC_X86_REG_XMM1)&0xffffffff,'uv':words(read(uint(read(g.reg_read(UC_X86_REG_ESP)+4,4)),8))}
   # Only the D30D0 surface is a dedicated diffuse texture. D2F90's P8
   # image has a separate authored palette and is already independently exact.
   if 'bitmap' in observed:
    size=uint(header[0x34:0x38]);assert 0<size<=16*1024*1024
    pixels=read(uint(header[0x54:0x58]),size);digest=hashlib.sha256(header[:0x18]+pixels).hexdigest();key=int(digest[:8],16)
    bank={'width':struct.unpack_from('<H',header,4)[0],'height':struct.unpack_from('<H',header,6)[0],'format':struct.unpack_from('<H',header,12)[0],'flags':struct.unpack_from('<H',header,14)[0],'pixels':pixels.hex(),'sha256':hashlib.sha256(pixels).hexdigest()}
    assert key not in surfaces or surfaces[key]==bank;surfaces[key]=bank;entry['resident_surface_key']=key
   observed.setdefault('bitmap_samples',[]).append(entry)
  if address in [0xd1e10,0x12de70,0x223c70,0x13d9f0,0xd2dc0,0xd2f90,0xd30d0,0x1362b0]:calls[f'{address:08X}']+=1
  if address in verified or SYNTH<=address<SYNTH+0x100000:return
  for section in image.sections:
   if section.va<=address and address+size<=section.va+section.rsize:
    assert bytes(g.mem_read(address,size))==image.section_bytes(section)[address-section.va:address-section.va+size],f'changed source instruction:{address:08X}';verified.add(address);return
  raise RuntimeError(f'uncaptured original service:{address:08X}')
 def write(g,access,address,size,value,data):
  if not SYNTH<=address<SYNTH+0x100000:
   for p in range(address&~4095,((address+size-1)&~4095)+4096,4096):dirty.add(p)
 u.hook_add(UC_HOOK_MEM_UNMAPPED,unmapped);u.hook_add(UC_HOOK_CODE,code);u.hook_add(UC_HOOK_MEM_WRITE,write)
 gdtr=re.search(r'GDT=\s*([0-9a-fA-F]+)\s+([0-9a-fA-F]+)',snapshot.report['registers_before']);ga,gl=(int(v,16)for v in gdtr.groups());read(ga,gl+1);u.reg_write(UC_X86_REG_GDTR,(0,ga,gl,0))
 for reg,selector in [(UC_X86_REG_CS,8),(UC_X86_REG_SS,0x10),(UC_X86_REG_DS,0x10),(UC_X86_REG_ES,0x10),(UC_X86_REG_FS,0x20)]:u.reg_write(reg,selector)
 def restore():
  for p in dirty:u.mem_write(p,pages[p])
  dirty.clear()
 def reset():
  for reg in [UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP]:u.reg_write(reg,0)
  u.reg_write(UC_X86_REG_ESP,STACK);u.reg_write(UC_X86_REG_EFLAGS,2);u.reg_write(UC_X86_REG_FPCW,0x023f);u.reg_write(UC_X86_REG_MXCSR,0x1f80)
 cases=[]
 for r in parent['records']:
  if r['unknown'] or r['label'].startswith(('suffix_','empty_cache_')):continue
  for i,s in enumerate(r['steps']):
   if s.get('status') is not None and s['packet'][2]==0xffffffff:cases.append((r['label']+'_'+str(i),s['sample_flags'],s['sample_initial'],s['packet'],s['sample_incoming'],None,[s['status'],*s['sample']]))
 assert len(cases)==2438
 for label,flags,initial,packet,seed,ty,expected in cases[:24]:
  for ty in [4,5]:
   for initial in [0,1]:cases.append(('type_%d_%d_'%(ty,initial)+label,flags,initial,packet,seed,ty,None))
 rows=['# Whole original D1E10 MAP. Q actual completed D2A50 contacts; T explicit part-type controls. Input flags/initial/packet14/caller5/forcedtype; output status/sample5/actualtype/UV2/bitmap.']
 records=[];begin=time.monotonic()
 for n,(label,flags,initial,packet,seed,forced_type,expected)in enumerate(cases):
  restore();reset();observed={};pending_uv=None;u.mem_write(P,packed(packet));u.mem_write(OUT,packed(seed));u.mem_write(STACK,packed([STOP,P,OUT,flags,initial]))
  u.emu_start(0xd1e10,STOP,count=5000000);assert u.reg_read(UC_X86_REG_EIP)==STOP and u.reg_read(UC_X86_REG_ESP)==STACK+20
  result=[u.reg_read(UC_X86_REG_EAX),*words(read(OUT,20))]
  if expected is not None:assert result==expected,(label,result,expected)
  inp=[flags,initial,*packet,*seed,0xffffffff if forced_type is None else forced_type]
  surface_key=next((s['resident_surface_key']for s in observed.get('bitmap_samples',[])if'resident_surface_key'in s),0xffffffff)
  incident=observed.get('incident_geometry');incident_words=[0xffffffff]*9
  if incident:
   selected=incident['selected'];incident_words=[len(incident['streams']),0xffffffff if selected is None else selected,0xffffffff if selected is None else bytes.fromhex(incident['streams'][selected]['raw'])[0],*incident.get('packed',[0xffffffff]*3),*struct.unpack('<3H',bytes.fromhex(incident['indices']))]
  out=[*result,observed['part_type'],*observed.get('uv',[0xffffffff]*2),observed.get('bitmap',0xffffffff),surface_key,*incident_words]
  rows.append(('Q 'if forced_type is None else'T ')+label+'\t'+' '.join(f'{v:08x}'for v in inp)+' | '+' '.join(f'{v:08x}'for v in out));records.append({'label':label,**observed,'result':result})
  if n%512==0:print(n,round(time.monotonic()-begin,2),flush=True)
 restore()
 fixture=ROOT/'tests/fixtures/retail_source_map_light_sample.tsv'
 for key,s in surfaces.items():rows.append('D '+f'{key:08x}'+' '+' '.join(f'{s[k]:08x}'for k in ['width','height','format','flags'])+' '+s['pixels'])
 fixture.write_text('\n'.join(rows)+'\n')
 report={'cases':len(cases),'actual_calls':2438,'part_type_controls':96,'records':records,'resident_surfaces':{f'{k:08x}':{p:v for p,v in s.items()if p!='pixels'}for k,s in surfaces.items()},'bitmap_globals':{f'{p:08X}':snapshot.read(p,n).hex()for p,n in [(0x4e647b,1),(0x468841,1),(0x4e647c,4),(0x4e6488,4)]},'source_xbe_sha256':PIN,'snapshot_ram_sha256':hashlib.sha256(snapshot.ram).hexdigest(),'snapshot_tick':snapshot.report['clock_tick'],'snapshot_eip':'0012B33E','fpcw':'023F','mxcsr':'1F80','source_rays_report_sha256':hashlib.sha256(source_report.read_bytes()).hexdigest(),'fixture_sha256':hashlib.sha256(fixture.read_bytes()).hexdigest(),'tool_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'source_calls':dict(calls),'instructions_verified':len(verified),'boundary':__doc__}
 (ROOT/'analysis/multiplayer-shading/map-light-sample-original.json').write_text(json.dumps(report,indent=2)+'\n');fixture.with_suffix('.provenance.json').write_text(json.dumps({k:v for k,v in report.items()if k!='records'},indent=2)+'\n');print(json.dumps({k:report[k]for k in ['cases','actual_calls','part_type_controls','source_calls','instructions_verified']},indent=2),flush=True)
if __name__=='__main__':main()

#!/usr/bin/env python3
"""Execute unchanged D2A50 and complete contact/sample callees over retained banks.

Owner-center one/five/custom-ray diagnostics and explicit bank/cache controls
execute the entire original routine without service replacement. Separately
labeled D2B72 controls supply a sample-return ABI state after an actually run
D1E10; only subsequent original orchestration is claimed by those controls.
Unknown kernel services never become an invented source status or sample.
"""
from pathlib import Path
import argparse,collections,hashlib,json,re,struct,sys,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_UNMAPPED,UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
from differential_source_object_admission_snapshot import Snapshot,PIN,uint,packed,words,fw
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'));from xbe import Xbe
SYNTH,STACK,STOP,P,D,OUT,START=0x20000000,0x200fe000,0x200ff000,0x20000100,0x20000200,0x20000300,0x20000400

def main():
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--fixture',type=Path,default=ROOT/'tests/fixtures/retail_source_object_light_rays.tsv');parser.add_argument('--output',type=Path,default=ROOT/'analysis/multiplayer-shading/object-light-rays-original.json');args=parser.parse_args()
 snapshot=Snapshot(ROOT/'analysis/multiplayer-shading/registration-snapshot-restore-20261008');image=Xbe(ROOT/'assets/disc/default.xbe');assert hashlib.sha256(image.data).hexdigest()==PIN
 u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(SYNTH,0x100000);pages={};verified=set();calls=collections.Counter();steps=[];pending=None;observing=False;phase="outer";actions=collections.Counter()
 def page(address):
  p=address&~4095
  if p not in pages:raw=snapshot.read(p,4096);u.mem_map(p,4096);u.mem_write(p,raw);pages[p]=raw
 def read(address,size):
  for p in range(address&~4095,((address+size-1)&~4095)+4096,4096):
   if not SYNTH<=p<SYNTH+0x100000:page(p)
  return bytes(u.mem_read(address,size))
 def put(address,*values):read(address,len(values)*4);u.mem_write(address,packed(values))
 def unmapped(g,access,address,size,value,data):
  for p in range(address&~4095,((address+size-1)&~4095)+4096,4096):page(p)
  return True
 def code(g,address,size,data):
  nonlocal pending,phase,suffix_applied
  if allow_block and address==0x12de70:
   at=g.reg_read(UC_X86_REG_ECX);desc=snapshot.read(at,0x24)
   if residency_addresses.get(at,2)==2 and desc[0x1e]==0 and struct.unpack_from('<i',desc,8)[0]>=1 and struct.unpack_from('<i',desc,12)[0]>=0:
    raise RuntimeError('source-residency-pending:%08X'%at)
  if address in [0xd2a50,0x14af40,0xd1e10,0x12de70,0x223c70,0x1362b0,0xd2f90,0xd3630,0x24490,0x241c0]:calls[f'{address:08X}']+=1
  if observing:
   if pending and address==pending['return']:
    if pending['kind']=='contact':
     pending['step']['hit']=g.reg_read(UC_X86_REG_EAX)&255;pending['step']['packet']=words(read(pending['output'],56));phase='outer'
    else:
     if suffix_control is not None and not suffix_applied:
      pending['step']['original_sample_status']=g.reg_read(UC_X86_REG_EAX)
      g.reg_write(UC_X86_REG_EAX,suffix_control['status'])
      if suffix_control['write_sample']:u.mem_write(pending['output'],packed([fw(.125),fw(-.5),fw(2.),0x12345678,0x90abcdef]))
      suffix_applied=True
     pending['step']['status']=g.reg_read(UC_X86_REG_EAX);pending['step']['sample']=words(read(pending['output'],20));phase='outer'
    pending=None
   if address==0x14af40:
    a=words(read(g.reg_read(UC_X86_REG_ESP),20));step={'origin':words(read(g.reg_read(UC_X86_REG_ECX),12)),'delta':words(read(a[1],12)),'excluded':a[2],'initial':g.reg_read(UC_X86_REG_EAX)&255,'incoming':words(read(a[4],56))}
    steps.append(step);pending={'kind':'contact','return':a[0],'output':a[4],'step':step};phase='contact'
   if address==0xd1e10:
    a=words(read(g.reg_read(UC_X86_REG_ESP),20));assert pending is None and steps[-1]['hit']
    steps[-1]['sample_flags']=a[3];steps[-1]['sample_initial']=a[4]&255;steps[-1]['sample_incoming']=words(read(a[2],20))
    pending={'kind':'sample','return':a[0],'output':a[2],'step':steps[-1]};phase='sample'
  if address in verified or SYNTH<=address<SYNTH+0x100000:return
  for section in image.sections:
   if section.va<=address and address+size<=section.va+section.rsize:
    assert bytes(g.mem_read(address,size))==image.section_bytes(section)[address-section.va:address-section.va+size],f'changed original instruction:{address:08X}';verified.add(address);return
  raise RuntimeError(f'uncaptured original service:{address:08X}')
 def memory_write(g,access,address,size,value,data):
  if not SYNTH<=address<SYNTH+0x100000:
   for p in range(address&~4095,((address+size-1)&~4095)+4096,4096):dirty.add(p)
 u.hook_add(UC_HOOK_MEM_UNMAPPED,unmapped);u.hook_add(UC_HOOK_CODE,code);u.hook_add(UC_HOOK_MEM_WRITE,memory_write)
 def restore():
  for p in dirty:u.mem_write(p,pages[p])
  dirty.clear()
 gdtr=re.search(r'GDT=\s*([0-9a-fA-F]+)\s+([0-9a-fA-F]+)',snapshot.report['registers_before']);ga,gl=(int(v,16)for v in gdtr.groups());read(ga,gl+1);u.reg_write(UC_X86_REG_GDTR,(0,ga,gl,0))
 for reg,selector in [(UC_X86_REG_CS,8),(UC_X86_REG_SS,0x10),(UC_X86_REG_DS,0x10),(UC_X86_REG_ES,0x10),(UC_X86_REG_FS,0x20)]:u.reg_write(reg,selector)
 def reset():
  for reg in [UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP]:u.reg_write(reg,0)
  u.reg_write(UC_X86_REG_ESP,STACK);u.reg_write(UC_X86_REG_EFLAGS,2);u.reg_write(UC_X86_REG_FPCW,0x023f);u.reg_write(UC_X86_REG_MXCSR,0x1f80)
 lm=uint(snapshot.read(0x4e0344,4));bsp=uint(snapshot.read(0x4e0348,4));gp=uint(snapshot.read(lm+0x84,4));assert uint(snapshot.read(lm+0x80,4))>0 and uint(snapshot.read(gp+0x1c,4))!=0xffffffff and snapshot.read(gp+4,4)==snapshot.read(bsp+8,4)
 cn,cp=words(snapshot.read(bsp+0x9c,8));dn,dp=words(snapshot.read(bsp+0x138,8));resident=[];residency_addresses={};residency_probe=[];dirty=set();blocked=[];allow_block=False
 for count,base,stride in [(cn,cp,176),(dn,dp,200)]:
  resident.append(count)
  for i in range(count):
   at=base+i*stride+0x28;desc=snapshot.read(at,0x24)
   restore();reset();put(STACK,STOP);u.reg_write(UC_X86_REG_EAX,3);u.reg_write(UC_X86_REG_ECX,at)
   boundary=None
   try:
    u.emu_start(0x12de70,STOP,count=2000000);assert u.reg_read(UC_X86_REG_EIP)==STOP
    state=u.reg_read(UC_X86_REG_EAX)&255
   except RuntimeError as e:
    if not str(e).startswith('uncaptured original service:'):raise
    state=2;boundary=str(e).split(':')[1]
   residency_probe.append({'descriptor':'%08X'%at,'raw_descriptor':desc.hex(),'state':state,'uncaptured_service':boundary})
   resident.append(state);residency_addresses[at]=state
 restore();allow_block=True
 cases=[];suffix_cases={};suffix_keys=set();current_count=uint(snapshot.read(0x5093e8,4));suffix_control=None;suffix_applied=False
 for owner in snapshot.report['objects']:
  h=int(owner['handle'],16);body=snapshot.file('object-'+owner['handle']+'.bin');origin=words(body[0x30:0x3c])
  for initial in [False,True]:
   for flags in [0,1,5]:cases.append((f'owner_{h:08X}_{flags}_{int(initial)}',True,flags,h,initial,origin,None))
   for axis in range(3):
    for sign in [-1,1]:
     ray=[0]*3;ray[axis]=fw(sign*(40. if axis==2 else 10.));cases.append((f'custom_{h:08X}_{axis}_{sign}_{int(initial)}',True,4,h,initial,origin,ray))
 for i in range(24):
  owner=snapshot.report['objects'][i];h=int(owner['handle'],16);body=snapshot.file('object-'+owner['handle']+'.bin');origin=words(body[0x30:0x3c])
  for flags in [0,1,4,5]:cases.append((f'guard_{i}_{flags}',False,flags,h,bool(i&1),origin,None))
 rows=['# Whole unchanged D2A50 over retained banks; suffix_ rows separately supply sample-return ABI state at D2B72. Unknown kernel paths are not source misses.']
 stats=collections.Counter();began=time.monotonic();blocked=[];actualrecords=[]
 for i,(label,valid,flags,excluded,initial,origin,custom)in enumerate(cases):
  restore();steps.clear();pending=None;phase='outer';observing=False;suffix_control=suffix_cases.get(label);suffix_applied=False;put(0x5093e8,0 if label.startswith('empty_cache_')else current_count)
  caller_contact=[(0x13570000+j*0x10203)&0xffffffff for j in range(14)];caller_sample=[0xcdcdcdcd]*5
  u.mem_write(STACK-0x100,bytes([0xcd])*0x100);u.mem_write(STACK-0x38,packed(caller_contact))
  u.mem_write(P,packed(origin));u.mem_write(D,packed(custom or [0,0,0]));u.mem_write(OUT,packed(caller_sample));put(0x4e0344,lm if valid else 0)
  reset();put(STACK,STOP,flags,excluded,int(initial),OUT,D);u.reg_write(UC_X86_REG_EBX,P);observing=True
  unknown=None
  try:
   u.emu_start(0xd2a50,STOP,count=5000000);assert u.reg_read(UC_X86_REG_EIP)==STOP and u.reg_read(UC_X86_REG_ESP)==STACK+24
  except RuntimeError as e:
   if not str(e).startswith(('source-residency-pending:','uncaptured original service:')):raise
   unknown={'phase':phase,'boundary':str(e),'input_label':label}
  observing=False;sample=words(read(OUT,20));packet=words(read(STACK-0x38,56));status=u.reg_read(UC_X86_REG_EAX)if unknown is None else 0xffffffff
  if steps:assert steps[0]['incoming']==caller_contact
  inp=[int(valid),flags,excluded,int(initial),*origin,int(custom is not None),*(custom or[0]*3),*caller_contact,*caller_sample]
  trace=[len(steps)]
  for st in steps:
   trace +=[*st['origin'],*st['delta'],st['excluded'],st['initial'],*st['incoming'],st.get('hit',0xffffffff),*st.get('packet',[0xffffffff]*14),st.get('status',0xffffffff),*st.get('sample',[0xffffffff]*5),st.get('sample_flags',0xffffffff),st.get('sample_initial',0xffffffff),*st.get('sample_incoming',[0xffffffff]*5)]
  expected=[status,*sample,*packet,*trace]
  rows.append(('U 'if unknown else'Q ')+label+'\t'+' '.join(f'{v:08x}'for v in inp)+' | '+' '.join(f'{v:08x}'for v in expected))
  stats['inputs']+=1;stats['complete'if unknown is None else'unknown']+=1;stats['contact_calls']+=len(steps);stats['sample_calls']+=sum('status'in st for st in steps)
  if unknown:blocked.append(unknown)
  else:stats['status_%d'%status]+=1
  if unknown is None and not label.startswith(('empty_cache_','suffix_')) and any('status'in st and st['packet'][2]!=0xffffffff for st in steps):
   cases.append(('empty_cache_'+label,valid,flags,excluded,initial,origin,custom))
  if unknown is None and not label.startswith(('empty_cache_','suffix_')) and flags not in suffix_keys and any('status'in st for st in steps):
   suffix_keys.add(flags)
   for supplied_status in [0,1,2,3]:
    for write_sample in [False,True]:
     control_label=f'suffix_{flags}_{supplied_status}_{int(write_sample)}';suffix_cases[control_label]={'status':supplied_status,'write_sample':write_sample};cases.append((control_label,valid,flags,excluded,initial,origin,custom))
  stats['whole_inputs'if suffix_control is None else'suffix_inputs']+=1
  if unknown is None:
   stats['whole_complete'if suffix_control is None else'suffix_complete']+=1
   stats[('whole_status_'if suffix_control is None else'suffix_status_')+str(status)]+=1
  actualrecords.append({'label':label,'suffix_control':suffix_control,'unknown':unknown,'status':None if unknown else status,'sample_words':sample,'contact_words':packet,'steps':list(steps)})
  if i%512==0:print(i,'elapsed',round(time.monotonic()-began,1),flush=True)
 restore();put(0x4e0344,lm);put(0x5093e8,current_count);protected=0
 for e in snapshot.report['files']:
  if e['address_space']=='CPU0 virtual' and ('object-'in e['file']or any(k in e['file']for k in ['collidable','noncollidable'])):
   at=int(e['address'],16);assert read(at,e['size'])==snapshot.read(at,e['size']),e['file'];protected+=1
 args.fixture.write_text('\n'.join(rows)+'\n')
 provenance={'version':1,'source_xbe_sha256':PIN,'snapshot_ram_sha256':hashlib.sha256(snapshot.ram).hexdigest(),'snapshot_manifest_sha256':hashlib.sha256((snapshot.directory/'export.json').read_bytes()).hexdigest(),'snapshot_tick':snapshot.report['clock_tick'],'snapshot_eip':'0012B33E','fpcw':'023F','mxcsr':'1F80','fixture_sha256':hashlib.sha256(args.fixture.read_bytes()).hexdigest(),'tool_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'counts':dict(stats),'residency_probes':residency_probe,'unknown_calls':blocked,'suffix_controls':suffix_cases,'source_calls':dict(calls),'instructions_verified':len(verified),'protected_banks':protected,'records':actualrecords,'boundary':'Whole-call cases run unchangedD2A50/contact/sample callees; separately labeled suffix cases supply explicit sample-return/output state atD2B72 after originalD1E10 executes and claim only original suffix/remaining-ray control. All calls operate over retained actual banks and explicit diagnostic query inputs. Missing kernel/async service is not a source status or sample. No source service replacement, native registry/pose/residency activation, guestIO or gameplay query scheduling claim.'}
 args.output.write_text(json.dumps(provenance,indent=2)+'\n');args.fixture.with_suffix('.provenance.json').write_text(json.dumps({k:v for k,v in provenance.items()if k!='records'},indent=2)+'\n');print(json.dumps({k:provenance[k]for k in ['counts','source_calls','instructions_verified','protected_banks']},indent=2),flush=True)

if __name__=='__main__':main()

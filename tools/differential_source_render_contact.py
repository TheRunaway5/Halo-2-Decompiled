#!/usr/bin/env python3
"""Execute complete unchanged14AF40 over retained actual source banks.

Diagnostic original3792 rays plus explicit guard/caller-word controls. Source
1697C0/14AC60/BF9A0/14A8E0/residency execute without service replacement.
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
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--fixture',type=Path,default=ROOT/'tests/fixtures/retail_source_render_contact.tsv');parser.add_argument('--output',type=Path,default=ROOT/'analysis/multiplayer-shading/render-contact-original.json');args=parser.parse_args()
 snapshot=Snapshot(ROOT/'analysis/multiplayer-shading/registration-snapshot-restore-20261008');image=Xbe(ROOT/'assets/disc/default.xbe');assert hashlib.sha256(image.data).hexdigest()==PIN
 u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(SYNTH,0x100000);pages={};verified=set();calls=collections.Counter();attempts=[];pending=None;observing=False;used_models=set();actions=collections.Counter()
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
  nonlocal pending
  if allow_block and address==0x12de70:
   at=g.reg_read(UC_X86_REG_ECX);desc=snapshot.read(at,0x24)
   if residency_addresses.get(at,2)==2 and desc[0x1e]==0 and struct.unpack_from('<i',desc,8)[0]>=1 and struct.unpack_from('<i',desc,12)[0]>=0:
    raise RuntimeError('source-residency-pending:%08X'%at)
  if address in [0x14af40,0x1697c0,0x14ac60,0xbf9a0,0x14a8e0,0x12de70]:calls[f'{address:08X}']+=1
  if observing:
   if pending and address==pending['return']:
    pending['hit']=g.reg_read(UC_X86_REG_EAX)&255;pending['packet']=words(read(pending['output'],96));pending=None
   if address==0x1697c0:
    esp=g.reg_read(UC_X86_REG_ESP);a=words(read(esp,28));pending={'return':a[0],'flags':a[1],'excluded':a[4],'origin':words(read(a[2],12)),'input':words(read(a[6],96)),'output':a[6],'targets':[]};attempts.append(pending)
   if address==0x1695a0:
    assert pending is not None;pending['targets'].append(g.reg_read(UC_X86_REG_EBX))
   if address==0x14a8e0:used_models.add(uint(read(g.reg_read(UC_X86_REG_ESP)+8,4)))
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
 vector_fixture=ROOT/'tests/fixtures/retail_source_vector_contact.tsv';bank=next(l for l in vector_fixture.read_text().splitlines()if l.startswith('B '));rows=['# Complete unchanged14AF40. Diagnostic retained rays; no live query-phase/registration claim.',bank,'L '+' '.join(f'{v:08x}'for v in resident)]
 table=uint(snapshot.read(0x4e3b44,4));owner_metadata={}
 def tag(d):return uint(snapshot.read(table+(d&65535)*16+8,4))
 for owner in snapshot.report['objects']:
  h=int(owner['handle'],16);body=snapshot.file('object-'+owner['handle']+'.bin');datum=uint(body);typ=body[0xaa];meta=tag(datum);policy=struct.unpack('<h',snapshot.read(meta+0xc0,2))[0]if typ==6 else-1
  model=uint(snapshot.read(meta+0x38,4));render=collision=0xffffffff;prt=leaves=0
  if model!=0xffffffff:
   m=tag(model);render=uint(snapshot.read(m+4,4));collision=uint(snapshot.read(m+12,4))
   if render!=0xffffffff and collision!=0xffffffff:r=tag(render);prt=uint(snapshot.read(r+0x74,4));leaves=uint(snapshot.read(r+0x7c,4))
  owner_metadata[h]={'type':typ,'policy':policy,'available':model!=0xffffffff and render!=0xffffffff and collision!=0xffffffff and struct.unpack('<i',packed([prt]))[0]>0 and struct.unpack('<i',packed([leaves]))[0]>0}
  rows.append('O '+' '.join(f'{v&0xffffffff:08x}'for v in [h,typ,datum,model,render,collision,prt,leaves,policy]))
 queries=ROOT/'analysis/multiplayer-shading/dynamic-object-contact-snapshot/queries.jsonl';original=[json.loads(l)for l in queries.read_text().splitlines()];cases=[]
 for i,q in enumerate(original):cases.append((f'actual_{i}',True,q,[0]*14))
 for i,q in enumerate(original[::64]):
  for valid in [False,True]:cases.append((f'guard_words_{i}_{int(valid)}',valid,q,[(0x13570000+j*0x10203)&0xffffffff for j in range(14)]))
 stats=collections.Counter();began=time.monotonic()
 for index,(label,valid,q,caller)in enumerate(cases):
  restore();attempts.clear();pending=None;
  if label.startswith('actual_') and q['hit'] and q['result_words'][0]==4:
   m=owner_metadata[q['result_words'][16]];action='reject-scenery'if m['type']==6 and m['policy']!=2 else'attribute-model'if m['available']else'fallback-map';actions[action]+=1
  u.mem_write(P,packed(q['origin_words']));u.mem_write(D,packed(q['ray_words']));u.mem_write(OUT,packed(caller));u.mem_write(START,bytes(12));put(0x4e0344,lm if valid else 0);put(STACK,STOP,D,int(q['excluded'],16),0,OUT);reset()
  initial=q['flags']=='4480000D';u.reg_write(UC_X86_REG_EAX,int(initial));u.reg_write(UC_X86_REG_ECX,P);u.reg_write(UC_X86_REG_EDI,START);observing=True
  try:
   u.emu_start(0x14af40,STOP,count=3000000);assert u.reg_read(UC_X86_REG_EIP)==STOP
  except RuntimeError as e:
   if not str(e).startswith('source-residency-pending:'):raise
   observing=False;at=int(str(e).split(':')[1],16);collision=attempts[0]['input']if attempts else[0]*24
   inp=[int(valid),int(initial),int(q['excluded'],16),*q['origin_words'],*q['ray_words'],*caller,*collision]
   rows.append('U '+label+'\t'+' '.join(f'{v:08x}'for v in inp)+' | '+f'{at:08x}')
   blocked.append({'label':label,'descriptor':'%08X'%at,'raw_descriptor':snapshot.read(at,0x24).hex(),'first_type':q['result_words'][0]if q['hit']else None});stats['blocked']+=1;continue
  observing=False
  hit=u.reg_read(UC_X86_REG_EAX)&255;out=words(read(OUT,56));start=words(read(START,12));assert pending is None
  collision=attempts[0]['input']if attempts else[0]*24
  if label.startswith('actual_'):
   assert attempts and attempts[0]['hit']==q['hit']
   # Only input-dependent/initialized packet words are compared with the old
   # zero-caller dispatcher run; internal stack words are explicitly preserved.
   for field in [0,1,2,3,4,5,6,7,8,14,15,16]:assert attempts[0]['packet'][field]==q['result_words'][field]
  expected=[hit,*start,*out,len(attempts)]
  for a in attempts:expected +=[a['flags'],a['excluded'],*a['origin'],a['hit'],*a['packet'],len(a['targets']),*a['targets']]
  inp=[int(valid),int(initial),int(q['excluded'],16),*q['origin_words'],*q['ray_words'],*caller,*collision]
  rows.append(label+'\t'+' '.join(f'{v:08x}'for v in inp)+' | '+' '.join(f'{v:08x}'for v in expected));stats['cases']+=1;stats['hits']+=bool(hit);stats['vector_queries']+=len(attempts);stats['retries']+=max(0,len(attempts)-1)
  stats['map_fallback_queries']+=sum(a['flags']==0x04800005 for a in attempts)
  if index%512==0:print(index,'elapsed',round(time.monotonic()-began,1),flush=True)
 restore();put(0x4e0344,lm);assert actions=={'reject-scenery':80,'attribute-model':65,'fallback-map':12},actions
 protected=0
 for e in snapshot.report['files']:
  if e['address_space']=='CPU0 virtual' and ('object-'in e['file']or any(k in e['file']for k in ['collidable','noncollidable'])):
   at=int(e['address'],16);assert read(at,e['size'])==snapshot.read(at,e['size']),e['file'];protected+=1
 available_models={int(line.split()[1],16)for line in (ROOT/'tests/fixtures/retail_source_object_light_attribution.tsv').read_text().splitlines()if line.startswith('M ')}
 assert used_models<=available_models,{'missing_actual_attribution_banks':[f'{v:08X}'for v in used_models-available_models]}
 args.fixture.write_text('\n'.join(rows)+'\n')
 provenance={'version':1,'source_xbe_sha256':PIN,'snapshot_ram_sha256':hashlib.sha256(snapshot.ram).hexdigest(),'snapshot_manifest_sha256':hashlib.sha256((snapshot.directory/'export.json').read_bytes()).hexdigest(),'snapshot_tick':snapshot.report['clock_tick'],'snapshot_eip':'0012B33E','fpcw':'023F actual captured FCW','mxcsr':'1F80','fixture_sha256':hashlib.sha256(args.fixture.read_bytes()).hexdigest(),'counts':dict(stats),'first_type4_actions':dict(actions),'attribution_models':[f'{v:08X}'for v in sorted(used_models)],'cluster_residency':resident[1:1+cn],'definition_residency':resident[cn+2:],'residency_probes':residency_probe,'blocked_calls':blocked,'residency_state_2':'Original mode3 execution reached an uncaptured kernel service during isolated descriptor replay. Whole upper-query stops before that unresolved12DE70 call, without replacement or source miss output. Ready headers execute complete original223C70 fixup. Each probe and case restores all written original pages.','source_calls':dict(calls),'instructions_verified':len(verified),'protected_banks':protected,'input_guards':{'lightmap':'%08X'%lm,'group_count':uint(snapshot.read(lm+0x80,4)),'bitmap':'%08X'%uint(snapshot.read(gp+0x1c,4)),'checksum':'%08X'%uint(snapshot.read(gp+4,4))},'boundary':'Unchanged complete14AF40/1697C0/14AC60/BF9A0/14A8E0 and complete source-ready mode3 residency over immutable retained current raw banks. Proved unresolved kernel/scheduling calls stop before12DE70 and remain unknown, not source misses. Original3792 rays plus explicitly controlled active-bank guard and caller words. Read-only observers; no source services replaced, guest IO/advance, live native activation or original query scheduling claim.'}
 args.output.write_text(json.dumps(provenance,indent=2)+'\n');args.fixture.with_suffix('.provenance.json').write_text(json.dumps(provenance,indent=2)+'\n');print(json.dumps(provenance,indent=2),flush=True)

if __name__=='__main__':main()

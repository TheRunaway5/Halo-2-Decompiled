#!/usr/bin/env python3
"""Complete unchanged3DC90/BAF80 with explicit external3DDD0 input boundary.

Controlled cases supply the callback's token; the original then reads the
literal cache+3 byte itself. Actual retained-phase rows execute complete3DDD0
on independently corroborated existing owner-token records, without allocation.
No guest IO, state advance, instruction patch, or shader/material inference.
"""
import argparse,hashlib,json,random,struct,subprocess,sys
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EIP,UC_X86_REG_ESP,UC_X86_REG_EFLAGS
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
NONE=0xffffffff; PIN='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
def pack(v):return struct.pack('<'+'I'*len(v),*(x&NONE for x in v))
def word(raw,at=0):return struct.unpack_from('<I',raw,at)[0]
def sha(raw):return hashlib.sha256(raw).hexdigest()
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--native',type=Path);args=ap.parse_args()
 image=Xbe(ROOT/'assets/disc/default.xbe');assert sha(image.data)==PIN
 u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x600000)
 for section in image.sections:u.mem_write(section.va,image.section_bytes(section))
 BASE=0x2100000;u.mem_map(BASE,0x200000)
 HEADER,TABLE,OBJECTS,CACHE_HEADER,RECORDS,DEFAULT,STACK,STOP=[BASE+i for i in [0,0x1000,0x10000,0x100000,0x101000,0x180000,0x1fe000,0x1ff000]]
 def put(at,*values):u.mem_write(at,pack(values))
 put(0x4e0300,HEADER);put(HEADER+0x44,TABLE);put(0x509434,CACHE_HEADER);put(CACHE_HEADER+0x44,RECORDS);put(0x4de2f4,DEFAULT)
 code={};calls=[];controlled=True;supplied=NONE;current=NONE
 def hook(g,pc,size,unused):
  nonlocal current
  assert bytes(g.mem_read(pc,size))==image.read(pc,size),(hex(pc),'changed original instruction');code[pc]=size
  if pc==0x3dc94:current=g.reg_read(UC_X86_REG_ECX)
  if pc==0x3ddd0:
   sp=g.reg_read(UC_X86_REG_ESP);owner=word(g.mem_read(sp+4,4));calls.append(owner)
   if controlled:
    g.reg_write(UC_X86_REG_EAX,supplied);g.reg_write(UC_X86_REG_EIP,word(g.mem_read(sp,4)));g.reg_write(UC_X86_REG_ESP,sp+8)
 u.hook_add(UC_HOOK_CODE,hook)
 def original(identity,owners,token,valid,cache_records=None):
  nonlocal controlled,supplied,current,calls
  controlled=cache_records is None;supplied=token;current=NONE;calls=[]
  u.mem_write(TABLE,bytes(2048*12));u.mem_write(OBJECTS,bytes(0x100000-0x10000));u.mem_write(RECORDS,bytes(65536))
  for i,(slot,parent,flags,owner_token)in enumerate(owners):
   pointer=OBJECTS+i*0x100;put(TABLE+slot*12+8,pointer);put(pointer+4,flags);put(pointer+0x14,parent);put(pointer+0xcc,owner_token)
  if cache_records is not None:u.mem_write(RECORDS,cache_records)
  for slot,value in valid:u.mem_write(RECORDS+slot*256+3,bytes([value]))
  put(STACK,STOP,identity);u.reg_write(UC_X86_REG_ESP,STACK);u.reg_write(UC_X86_REG_EFLAGS,2);u.emu_start(0x3dc90,STOP,count=100000)
  assert u.reg_read(UC_X86_REG_EIP)==STOP
  pointer=u.reg_read(UC_X86_REG_EAX)
  storage=0 if not pointer else 1 if pointer==DEFAULT+0x1c else 2
  if storage==2:assert pointer==RECORDS+(token&65535)*256+0x54
  return [storage,current,token if calls else NONE,len(calls),calls[0]if calls else NONE],pointer
 def encode(identity,owners,token,valid,version=1):return [version,identity,token,len(owners),len(valid),*[v for slot,parent,flags,unused in owners for v in [slot,parent,flags]],*[v for row in valid for v in row]]
 rng=random.Random(0x3dc90);cases=[];outputs=[];kinds={0:0,1:0,2:0};live=[]
 for index in range(2048):
  count=1+index%12;slots=rng.sample(range(2048),count);owners=[]
  for i,slot in enumerate(slots):
   parent=NONE if i==count-1 else (rng.randrange(0x8000,0xfffe)<<16)|slots[i+1]
   flags=rng.getrandbits(32)&~0x01000000
   if rng.randrange(7)==0:flags|=0x01000000
   owners.append([slot,parent,flags,NONE])
  identity=(rng.randrange(0x8000,0xfffe)<<16)|slots[0];token=NONE if index%5==0 else (rng.randrange(0x8000,0xfffe)<<16)|rng.randrange(256)
  valid=[]if token==NONE else [[token&65535,[0,1,2,127,255][index%5]]]
  out,pointer=original(identity,owners,token,valid);cases.append(encode(identity,owners,token,valid));outputs.append(out);kinds[out[0]]+=1
 # Same-phase actual202 object fields/cache pool. Only complete3DDD0 reuse
 # paths with full owner identity already matching are admitted as live rows.
 directory=ROOT/'analysis/multiplayer-shading/shadow-live-phase-20261008/capture-opengl'
 manifest=json.loads((directory/'capture.json').read_text());phase=next(p for p in manifest['phases']if p['name']=='32410-cached-opacity');files={f['file']:f for f in phase['files']}
 def data(suffix):
  record=files['32410-cached-opacity-'+suffix+'.bin'];raw=(directory/record['file']).read_bytes();assert len(raw)==record['size'] and sha(raw)==record['sha256'];return raw
 records=data('render-cache-records');owners=[];bodies={}
 for info in phase['objects']:
  identity=int(info['handle'],16);body=data('object-'+info['handle']);bodies[identity&65535]=body
  owners.append([identity&65535,word(body,0x14),word(body,4),word(body,0xcc)])
 byslot={r[0]:r for r in owners}
 for info in phase['objects']:
  identity=int(info['handle'],16);resolved=identity
  if not byslot[resolved&65535][2]&0x01000000:
   while byslot[resolved&65535][1]!=NONE:resolved=byslot[resolved&65535][1]
  if byslot[resolved&65535][2]&0x01000000:token=NONE;valid=[]
  else:
   token=byslot[resolved&65535][3]
   if token==NONE or token&65535>=256 or word(records,(token&65535)*256+4)!=resolved:continue
   valid=[ [token&65535,records[(token&65535)*256+3]] ]
  out,pointer=original(identity,owners,token,valid,records);cases.append(encode(identity,owners,token,valid,2));outputs.append(out);kinds[out[0]]+=1
  live.append({'owner':f'{identity:08X}','resolved_owner':f'{resolved:08X}','cache_token':f'{token:08X}','source_pointer':f'{pointer:08X}','words':out})
  if identity==phase['owner']:
   assert out[0]==2 and token&65535==9
   assert records[9*256+0x54:9*256+0x54+84]==data('cached-lighting')
   live[-1]['independent_observed_pointer']=f"{phase['cache_state_address']:08X}";assert phase['cache_state_address']==int(files['32410-cached-opacity-render-cache-records.bin']['address'],16)+9*256+0x54
 fixture=ROOT/'tests/fixtures/retail_source_shadow_lighting.tsv'
 fixture.write_text('# Complete unchanged3DC90/BAF80; controlled external3DDD0 inputs, final actual same-phase full3DDD0 reuse cases\n'+''.join(' '.join(f'{v:x}'for v in row)+' | '+' '.join(f'{v:x}'for v in out)+'\n'for row,out in zip(cases,outputs)))
 differences=[]
 if args.native:
  run=subprocess.run([str(args.native.resolve()),'--probe'],input=''.join(' '.join(f'{v:x}'for v in row)+'\n'for row in cases),text=True,capture_output=True,check=True)
  actual=[[int(v,16)for v in row.split()]for row in run.stdout.splitlines()];assert len(actual)==len(outputs)
  differences=[{'case':i,'source':s,'native':n}for i,(s,n)in enumerate(zip(outputs,actual))if s!=n]
 report={'source_executable_sha256':PIN,'cases':len(cases),'controlled_cases':2048,'same_phase_existing_cache_cases':len(live),'output_words':sum(map(len,outputs)),'storage_cases':kinds,'verified_original_instruction_addresses':len(code),'fixture_sha256':sha(fixture.read_bytes()),'native_binary_sha256':sha(args.native.read_bytes())if args.native else None,'live_cases':live,'differences':differences[:4],'boundaries':['Complete unchanged3DC90/BAF80 executes for every case. Controlled rows supply external3DDD0 token at its service boundary; source executes the subsequent actual validity-byte read.','Final actual rows supply same32410phase202 object fields and complete256 cache-record bytes. Existing-token full-owner matching paths execute unchanged3DDD0 too, without any service override. Allocation/pressure paths remain independently proven by source_render_cache tests.','Original selected Chief pointer and all84 lighting bytes independently match captured32410cachedopacity data. Global default lighting contents are not a new default material/color.','No guest IO/advance, renderer activation, native owner registration/history/cache-query producer, or code modification.']}
 fixture.with_suffix('.provenance.json').write_text(json.dumps(report,indent=2)+'\n');(ROOT/'analysis/multiplayer-shading/source-shadow-lighting-original.json').write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps({k:v for k,v in report.items()if k!='live_cases'},indent=2));assert not differences
if __name__=='__main__':main()

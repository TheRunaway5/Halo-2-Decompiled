#!/usr/bin/env python3
"""Whole original King event recording with a fixed captured original clock.

Unchanged retail/TU1.1/TU1.5 event, CRT conversion, pose and ring owners execute.
The real captured Xbox kernel ordinal128 executes against its unchanged saved
system-time data. Original loader/IAT resolution, isolated player/object API
records and event flags/timestamps/ring bytes are explicit inputs. No clock or
other return, instruction patch, live timer advance or complete boot is supplied.
"""
import argparse,bisect,csv,hashlib,itertools,json,struct
from pathlib import Path
from capstone import Cs,CS_ARCH_X86,CS_MODE_32,CS_AC_WRITE
from capstone.x86 import X86_OP_MEM
from unicorn import UC_HOOK_CODE,UC_HOOK_MEM_INVALID
from unicorn.x86_const import *
from oracle_king_engine import KingTrace,OWNERS as KING,P,R,H,U,G,M
from oracle_game_over import SPECS
from oracle_mp_full_scoreboard_1_5 import literal,complete_tokens,references,sha
from research_mp_lobby_later_update import table_pair
from differential_source_object_admission_snapshot import Snapshot
from oracle_camouflage_pass_services import SNAPSHOT,RAM_PIN
from oracle_game_variants import ROOT

OUT=ROOT/'analysis/multiplayer/game-variants/king-recording-event-source-20261012'
ARGS=0x4030000
SPEC={
 'retail':dict(event=0x1972a0,append=0x197120,time=0x321aae,pose=0x1994d0,
  enabled=0x510ca0,index=0x510ca4,last=0x510ca8,start=0x511024,ring=0x515c34,complete=0x510cb1),
 'tu11':dict(event=0x198da0,append=0x198c20,time=0x32803e,pose=0x19afd0,
  enabled=0x518490,index=0x518494,last=0x518498,start=0x518814,ring=0x51d424,complete=0x5184a1),
 'u5':dict(event=0x19bca0,append=0x19bb20,time=0x32bbae,pose=0x19de80,
  enabled=0x51dfa0,index=0x51dfa4,last=0x51dfa8,start=0x51e324,ring=0x522f34,complete=0x51dfb1),
}
# These are complete original instruction owners, not native callbacks or
# guessed address deltas. Startup/completion differ between the three images.
STATIC={
 'retail':dict(startup=0x195a40,stop=0x195e90,completion=0x196390,
  refresh_context=0x195ed0,session_disable=0x6ae20,session_enable=0x6aee0,
  timestamp_start=0x199460,local_machine=0x199290,restore=0x1973f0,
  prepare=0x157ae0,phase=0x15c550,post_world=0x15ca10,
  scoring=0x2beee0,score_changed=0x19f470,network=0xa7c50,local=0x19eb30),
 'tu11':dict(startup=0x197450,stop=0x1978a0,completion=0x197ea0,
  refresh_context=0x1978e0,session_disable=0x6b2f0,session_enable=0x6b3b0,
  timestamp_start=0x19af60,local_machine=0x19ad90,restore=0x198ef0,
  prepare=0x159180,phase=0x15dbc0,post_world=0x15e070,
  scoring=0x2c5220,score_changed=0x1a2dd0,network=0xa8a30,local=0x1a2490),
 'u5':dict(startup=0x19a430,stop=0x19a7f0,completion=0x19acb0,
  refresh_context=0x19a830,session_disable=0x6c4d0,session_enable=0x6c590,
  timestamp_start=0x19de10,local_machine=0x19dc90,restore=0x19bdf0,
  prepare=0x15a560,phase=0x15efa0,post_world=0x15f450,
  scoring=0x2c8ca0,score_changed=0x1a9080,network=0xb1e80,local=0x1a8740),
}
def uint(raw):return int.from_bytes(raw,'little')
def digest(raw):return hashlib.sha256(raw).hexdigest()
def pack(*args):return struct.pack('<'+'I'*len(args),*(v&0xffffffff for v in args))

def write_inventory(t):
 # Bounded absolute-write inventory: byte-pattern locations are admitted only
 # when the discovered complete original CFG contains a decoded write operand.
 # This does not claim an exhaustive alias/indirect-store search.
 r=t.rs;fields={r['enabled']:'enabled',r['enabled']+1:'event_freeze',
  r['index']:'index',r['last']:'last',r['start']:'start',r['complete']:'completion'}
 starts=sorted(t.starts);cache={};rows=[];unresolved=[]
 for address,field in fields.items():
  raw=pack(address);offset=0
  while (offset:=t.code.bytes.find(raw,offset))>=0:
   at=t.code.lo+offset;offset+=1;p=bisect.bisect_right(starts,at)-1
   if p<0:continue
   entry=starts[p]
   if entry not in cache:cache[entry]=literal(t.code,entry,t.starts)[1]
   matches=[i for i in cache[entry]if i.address<=at<i.address+i.size]
   if not matches:
    unresolved.append(dict(field=field,literal_address=at,candidate_owner=entry));continue
   for i in matches:
    for op in i.operands:
     if op.type==X86_OP_MEM and not op.mem.base and not op.mem.index and op.mem.disp==address and op.access&CS_AC_WRITE:
      rows.append(dict(field=field,address=address,owner=entry,instruction=i.address,
       raw_hex=i.bytes.hex(),mnemonic=i.mnemonic,operands=i.op_str,width=op.size))
 return dict(scope='Verified direct absolute write operands only, plus separately listed startup REP clear; indirect aliases and platform/session ownership are not exhaustively recovered.',
  writes=rows,unresolved_literal_locations=unresolved)

class Trace(KingTrace):
 def __init__(self,name,snapshot,kernel):
  super().__init__(name);self.rs=SPEC[name];self.events=[];self.kernel_observations=[];self.pending={};self.stopped=None
  self.on=False;self.operation='initial';self.kernel=kernel;self.snapshot=snapshot
  self.g.hook_add(UC_HOOK_CODE,self.observe);self.g.hook_add(UC_HOOK_MEM_INVALID,self.invalid)
  for page in kernel['mapped_pages']:
   self.g.mem_map(page,4096);self.g.mem_write(page,snapshot.read(page,4096))
  _,ins=literal(self.code,self.rs['time'],self.starts)
  calls=[int(i.op_str,16)for i in ins if i.mnemonic=='call'and i.op_str.startswith('0x')]
  assert len(calls)==2;self.rs=dict(self.rs,query_thunk=calls[0],divide=calls[1])
  _,thunk=literal(self.code,calls[0],self.starts);assert len(thunk)==1 and thunk[0].mnemonic=='jmp'
  iat=int(thunk[0].op_str.split('[0x')[1].split(']')[0],16)
  assert self.image.read(iat,4)==pack(0x80000080)
  self.loader=dict(iat=iat,encoded_original_hex=self.image.read(iat,4).hex(),ordinal=128,
   resolved_target=kernel['entry'],captured_retail_iat=0x411588,captured_retail_iat_hex=snapshot.read(0x411588,4).hex(),
   basis='Actual captured kernel PE export ordinal128 equals captured original retail IAT target. Same declared loader binding for update ordinal128.')
  self.w(iat,kernel['entry'])
 def observe(self,g,address,size,_):
  if not self.on:return
  if address in self.pending:
   row=self.pending.pop(address);row['output_after_hex']=bytes(g.mem_read(row['output_pointer'],8)).hex()
  if address==self.kernel['entry']:
   sp=g.reg_read(UC_X86_REG_ESP);ptr=self.u(sp+4)
   row=dict(entry=address,return_address=self.u(sp),output_pointer=ptr,output_before_hex=bytes(g.mem_read(ptr,8)).hex(),
    system12_hex=bytes(g.mem_read(self.kernel['system_time'],12)).hex())
   self.kernel_observations.append(row);self.pending[row['return_address']]=row
  if address==self.rs['append']:
   pointer=g.reg_read(UC_X86_REG_EDX)
   self.events.append(dict(entry=address,record36_hex=bytes(g.mem_read(pointer,36)).hex(),
    index_before=self.u(self.rs['index'])))
 def invalid(self,g,access,address,size,value,_):
  self.stopped=dict(operation=self.operation,eip=g.reg_read(UC_X86_REG_EIP),address=address,size=size,access=access,value=value)
  return False
 def run(self,operation,entry,args=(),regs=()):
  self.operation=operation;self.events=[];self.kernel_observations=[];self.pending={};self.on=True
  try:return self.call(entry,args,regs)
  finally:self.on=False

def clock_context(s):
 assert digest(s.ram)==RAM_PIN
 base=0x80010000;dos=s.read(base,64);assert dos[:2]==b'MZ'
 nt=base+uint(dos[60:64]);assert s.read(nt,4)==b'PE\0\0'
 export_rva,export_size=struct.unpack('<II',s.read(nt+24+96,8))
 header=struct.unpack('<IIHHIIIIIII',s.read(base+export_rva,40))
 ordinal_base,function_count,functions=header[5],header[6],header[8]
 entry=base+uint(s.read(base+functions+(128-ordinal_base)*4,4))
 assert uint(s.read(0x411588,4))==entry
 ins=[]
 for i in Cs(CS_ARCH_X86,CS_MODE_32).disasm(s.read(entry,128),entry):
  ins.append(i)
  if i.mnemonic=='ret':break
 assert ins[-1].mnemonic=='ret'and ins[-1].op_str=='4'
 system=int(next(i.op_str for i in ins if i.mnemonic=='mov'and i.op_str.startswith('ecx, dword ptr [0x')).split('[0x')[1].split(']')[0],16)
 raw=s.read(system,12);assert raw[4:8]==raw[8:12]
 return dict(kernel_base=base,nt_header=nt,export_rva=export_rva,export_size=export_size,
  ordinal_base=ordinal_base,function_count=function_count,function_table=base+functions,
  export_ordinal128_slot=base+functions+(128-ordinal_base)*4,
  export_ordinal128_raw_hex=s.read(base+functions+(128-ordinal_base)*4,4).hex(),entry=entry,
  instructions=len(ins),code_hex=b''.join(bytes(i.bytes)for i in ins).hex(),
  system_time=system,system12_hex=raw.hex(),captured_filetime=uint(raw[:8]),
  mapped_pages=sorted({entry&~4095,system&~4095}),
  pages=[dict(address=a,physical=s.physical(a),sha256=digest(s.read(a,4096)),raw_hex=s.read(a,4096).hex())for a in sorted({entry&~4095,system&~4095})],
  export_page_sha256=digest(s.read(base,4096)),snapshot_manifest_sha256=sha(SNAPSHOT/'export.json'))

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'complete');a=ap.parse_args()
 a.output.mkdir(parents=True,exist_ok=False);s=Snapshot(SNAPSHOT);kernel=clock_context(s)
 seed=bytes((i*197+(i>>8)*31+17)&255 for i in range(36000));cases=[]
 flags=((0,0,0),(0,1,1),(1,0,0),(1,1,0),(1,0,1),(255,0,255))
 for n,(flag,delta,pose,index,slot)in enumerate(itertools.product(flags,(0,1,2,3,4,0xffffffff,0xfffffffc,0x80000000,0x7fffffff),('current','fallback','none'),(0,999),(0,15))):
  cases.append(dict(enabled=flag[0],event_freeze=flag[1],completion=flag[2],last_minus_now=delta,
   pose=pose,index=index,slot=slot,player=(0x9abc0000|slot),first_arg=(4,0x12345678,0xffffffff)[n%3],
   second_arg=(0xffffffff,0,0x87654321)[n%3],start_minus_now=(0,1,0xffffffff,0x80000000)[n%4],
   xyz_bits=[[0x3fa00000,0xc0200000,0x40700000],[0x80000000,0,0x3f000000]][n%2]))
 versions={};traces={};listings=[];pairs=[];static_pairs=[]
 try:
  for name in SPEC:
   t=Trace(name,s,kernel);r=t.rs;traces[name]=t
   t.g.mem_write(ARGS,b'\xa5'*64)
   now=t.run('actual_time32_optional_output',r['time'],(ARGS,))&0xffffffff
   assert t.u(ARGS)==now;clock_probe=list(t.kernel_observations)
   outputs=[]
   for n,row in enumerate(cases):
    t.g.mem_write(r['ring'],seed);t.g.mem_write(r['enabled'],bytes([row['enabled'],row['event_freeze']]))
    t.g.mem_write(r['complete'],bytes([row['completion']]))
    t.w(r['index'],row['index']);t.w(r['last'],now+row['last_minus_now']);t.w(r['start'],now+row['start_minus_now'])
    at=R+row['slot']*0x21c;unit=0x8abc0000|row['slot'];body=U+row['slot']*0x100
    t.w(at+0x2c,unit if row['pose']=='current'else -1);t.w(at+0x30,unit if row['pose']=='fallback'else -1)
    t.w(H+row['slot']*12+8,body);t.w(body+0x14,-1);t.g.mem_write(body+0x64,pack(*row['xyz_bits']))
    before12=bytes(t.g.mem_read(r['enabled'],12));result=t.run(f'event-{n}',r['event'],(row['first_arg'],row['second_arg']),((UC_X86_REG_EBX,row['player']),))
    ring=bytes(t.g.mem_read(r['ring'],36000));after12=bytes(t.g.mem_read(r['enabled'],12));events=list(t.events)
    if events:
     assert len(events)==1;offset=row['index']*36
     assert ring[:offset]==seed[:offset]and ring[offset+36:]==seed[offset+36:]
     assert ring[offset:offset+36].hex()==events[0]['record36_hex']
    else:assert ring==seed
    assert t.g.mem_read(r['complete'],1)[0]==row['completion']
    outputs.append(dict(before12_hex=before12.hex(),after12_hex=after12.hex(),index=t.u(r['index']),last=t.u(r['last']),
     ring_sha256=digest(ring),appended36_hex=events[0]['record36_hex']if events else None,
     kernel_queries=len(t.kernel_observations),clock_observations=list(t.kernel_observations)))
   append_rows=[]
   for flag,index in itertools.product(flags,(0,1,998,999)):
    t.g.mem_write(r['ring'],seed);t.g.mem_write(r['enabled'],bytes(flag[:2]));t.g.mem_write(r['complete'],bytes(flag[2:]));t.w(r['index'],index)
    record=bytes((i*53+index*17)&255 for i in range(36));t.g.mem_write(ARGS,record)
    t.run('direct_append',r['append'],regs=((UC_X86_REG_EDX,ARGS),));ring=bytes(t.g.mem_read(r['ring'],36000))
    append_rows.append(dict(enabled=flag[0],event_freeze=flag[1],completion=flag[2],index_before=index,index_after=t.u(r['index']),
      source36_hex=record.hex(),destination36_hex=ring[index*36:index*36+36].hex(),ring_sha256=digest(ring)))
   # Whole startup with the genuine null selected-engine branch. This proves
   # the actual early clear/index/time writes, not populated startup services.
   owners=STATIC[name];_,startup_ins=literal(t.code,owners['startup'],t.starts)
   count=next(i.operands[1].imm for i in startup_ins if i.mnemonic=='mov'and i.op_str.startswith('ecx, '))
   context=r['enabled']+16;context_size=count*4
   t.g.mem_write(context,b'\x5a'*context_size);t.g.mem_write(r['enabled'],bytes.fromhex('7f55aabb785634129abcdef0'))
   t.w(M+0xc14,0);assert t.u(t.spec['table'])==0
   startup_before=bytes(t.g.mem_read(r['enabled'],12)).hex()
   t.run('whole_startup_selected_engine_null',owners['startup'])
   startup=dict(entry=owners['startup'],selected_engine_index=0,selected_engine_pointer=0,
    before12_hex=startup_before,after12_hex=bytes(t.g.mem_read(r['enabled'],12)).hex(),
    context=context,context_size=context_size,context_before_byte=0x5a,
    context_after_sha256=digest(bytes(t.g.mem_read(context,context_size))),
    index=t.u(r['index']),last=t.u(r['last']),start=t.u(r['start']),completion=uint(t.g.mem_read(r['complete'],1)),
    clock_observations=list(t.kernel_observations))
   assert bytes(t.g.mem_read(context,context_size))==bytes(context_size)
   assert t.u(r['index'])==0 and t.u(r['last'])==now and uint(t.g.mem_read(r['enabled'],2))==0x557f
   # Whole start-time producer through its actual retained-machine search;
   # empty and explicitly declared occupied/local rows are raw API inputs.
   start_rows=[]
   for machine in(-1,0,15):
    t.g.mem_write(context,bytes(context_size));t.w(r['start'],0x12345678);patches=[]
    if machine>=0:
     for offset in(6,8):
      address=context+0xdc24+machine*11+offset;t.g.mem_write(address,b'\x01');patches.append(dict(address=address,value=1))
    before=bytes(t.g.mem_read(context,context_size));t.run('whole_recording_timestamp_start',owners['timestamp_start'])
    after=bytes(t.g.mem_read(context,context_size));changed=[dict(offset=i,before=b,after=c)for i,(b,c)in enumerate(zip(before,after))if b!=c]
    start_rows.append(dict(entry=owners['timestamp_start'],context=context,context_size=context_size,
     declared_local_machine=machine,initial_context='zero except initial start12345678 and listed one-byte patches',
     patches=patches,before_sha256=digest(before),after_sha256=digest(after),changed_bytes=changed,
     started=uint(t.g.mem_read(r['start']-4,1)),start=t.u(r['start']),clock_observations=list(t.kernel_observations)))
    assert t.u(r['start'])==now and uint(t.g.mem_read(r['start']-4,1))==1
    if machine>=0:assert uint(t.g.mem_read(context+0xdc24+machine*11+9,1))==1
   executed=hashlib.sha256();decoder=Cs(CS_ARCH_X86,CS_MODE_32);kernel_executed=set()
   for address in sorted(t.executed):
    if address>=0x80000000:
     i=next(decoder.disasm(s.read(address,15),address,count=1));expected=bytes(i.bytes);kernel_executed.add(address)
    else:i=t.code.at(address);expected=bytes(i.bytes)
    actual=bytes(t.g.mem_read(address,i.size));assert actual==expected,(name,hex(address));executed.update(pack(address)+actual)
   assert bytes(t.g.mem_read(kernel['system_time'],12)).hex()==kernel['system12_hex']
   versions[name]=dict(image_path=t.spec['path'],image_sha256=t.spec['pin'],owners=r,loader_binding=t.loader,actual_now=now,clock_probe=clock_probe,
    cases=outputs,direct_appends=append_rows,observed_instructions=len(t.executed),executed_instruction_sha256=executed.hexdigest(),
    kernel_executed_addresses=sorted(kernel_executed),kernel_data_unchanged=True,
    whole_startup_null_engine=startup,whole_timestamp_start=start_rows,write_inventory=write_inventory(t),
    static_owners=owners)
   for key,entry in {**{k:r[k]for k in('event','append','time','pose','query_thunk','divide')},'object_position':KING[name]['position']}.items():
    f,ins=literal(t.code,entry,t.starts);p=a.output/f'{name}-{key}-{entry:08x}.asm';p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins))
    listings.append(dict(image=name,owner=key,entry=entry,instructions=len(ins),file=p.name,sha256=sha(p)))
   for key,entry in owners.items():
    if key in('network','local'):continue # Adjacent service locator only.
    _,ins=literal(t.code,entry,t.starts);p=a.output/f'{name}-static-{key}-{entry:08x}.asm'
    p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins))
    listings.append(dict(image=name,owner=key,entry=entry,instructions=len(ins),file=p.name,sha256=sha(p),
     kind='whole_original_static_listing',whole_execution=key in('startup','timestamp_start','local_machine')))
    if key=='score_changed':
     calls=[dict(instruction=i.address,target=int(i.op_str,16))for i in ins if i.mnemonic=='call'and i.op_str.startswith('0x')]
     assert [v['target']for v in calls]==[owners['network'],owners['local']]*3
     versions[name]['score_event_locator']=dict(entry=entry,abi='EAXplayer; stack(updated_score); RET4',
      thresholds=[dict(remaining=10,ffa=47,team=48),dict(remaining=30,ffa=9,team=10),dict(remaining=60,ffa=7,team=8)],
      team_predicate='selected engine pointer nonnull AND options184bit0',mode4_suppresses=True,
      words=['0','threshold subtype','FFFFFFFF','full player datum','signed playerC0','FFFFFFFF','FFFFFFFF','0'],
      tail_u16='FFFF',event_bytes=34,calls=calls,
      service_abis=dict(network='EDXevent*',local='EDIevent*'),evidence='Complete static score owner only; no new presentation execution claim.')
  # Actual records/indices/query counts match; observed source return PCs are
  # relocated and retained raw instead of described as byte-identical.
  def behavior(v):return[{k:x for k,x in row.items()if k!='clock_observations'}for row in v['cases']]
  assert behavior(versions['retail'])==behavior(versions['tu11'])==behavior(versions['u5'])
  assert versions['retail']['direct_appends']==versions['tu11']['direct_appends']==versions['u5']['direct_appends']
  for later in('tu11','u5'):
   old,new=traces['retail'],traces[later]
   for key in('event','append','time','pose','query_thunk','divide'):
    oe,ne=old.rs[key],new.rs[key];of,oi=literal(old.code,oe,old.starts);nf,ni=literal(new.code,ne,new.starts)
    assert complete_tokens(oi,old.image)==complete_tokens(ni,new.image),(later,key)
    topology=table_pair(of,nf,oi,ni,old.image,new.image);assert all(r['topology_equal']for r in topology)
    pairs.append(dict(image=later,owner=key,complete_shape_equal=True,retail_entry=oe,later_entry=ne,
      relocated_operands=references(oi,ni,old.image,new.image),jump_table_topology=topology))
   for key in STATIC['retail']:
    if key in('network','local'):continue
    oe,ne=STATIC['retail'][key],STATIC[later][key]
    _,oi=literal(old.code,oe,old.starts);_,ni=literal(new.code,ne,new.starts)
    equal=complete_tokens(oi,old.image)==complete_tokens(ni,new.image)
    static_pairs.append(dict(image=later,owner=key,retail_entry=oe,later_entry=ne,
     retail_instructions=len(oi),later_instructions=len(ni),complete_shape_equal=equal,
     relocated_operands=references(oi,ni,old.image,new.image)if equal else [],
     basis='Each unequal complete body is preserved independently; no update equivalence inferred.'))
 except Exception as e:
  (a.output/'failure.json').write_text(json.dumps(dict(error=repr(e),boundaries={n:t.stopped for n,t in traces.items()}),indent=2)+'\n');raise
 kernel_listing=a.output/'captured-kernel-KeQuerySystemTime-80019f60.asm'
 kernel_listing.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in Cs(CS_ARCH_X86,CS_MODE_32).disasm(bytes.fromhex(kernel['code_hex']),kernel['entry'])))
 report=dict(schema='original-king-recording-event-source-v1',scope=__doc__,tool_sha256=sha(__file__),
  snapshot=str(SNAPSHOT.relative_to(ROOT)),snapshot_ram_sha256=RAM_PIN,kernel=kernel,
  inputs=cases,ring_seed_hex=seed.hex(),versions=versions,listings=listings,complete_correspondence=pairs,
  static_correspondence=static_pairs,
  dependencies={str(p.relative_to(ROOT)):sha(p)for p in[ROOT/'tools'/n for n in(
   'oracle_king_engine.py','oracle_game_variants.py','oracle_game_over.py',
   'differential_source_object_admission_snapshot.py','oracle_camouflage_pass_services.py',
   'oracle_mp_full_scoreboard_1_5.py','research_mp_lobby_later_update.py','oracle_smg_firing_1_1.py',
   'reference/halo2-decompiled/tools/xbe.py','reference/halo2-decompiled/tools/functions.py')]},
  summary=dict(images=3,event_calls=3*len(cases),direct_append_calls=3*len(versions['retail']['direct_appends']),
    whole_time_probe_calls=3,whole_startup_null_engine_calls=3,whole_timestamp_start_calls=9,
    complete_pairs=len(pairs),static_pairs=len(static_pairs),static_equal_pairs=sum(v['complete_shape_equal']for v in static_pairs),instruction_listings=len(listings)+1,
    appending_event_calls=sum(bool(row['appended36_hex'])for v in versions.values()for row in v['cases']),
    original_kernel_queries=sum(row['kernel_queries']for v in versions.values()for row in v['cases'])+3+
     sum(len(v['whole_startup_null_engine']['clock_observations'])+sum(len(row['clock_observations'])for row in v['whole_timestamp_start'])for v in versions.values())),
  instruction_patches=[],supplied_returns=[],limits=['Fixed authentic captured system-time data; no advancing wall time, timer interrupt or original boot.',
   'Unparented object position/current or fallback unit, declared valid low16 player slots0/15 only.',
   'Ring publication is in-memory retention, not a wire packet, file save, complete identity/report export or native admission.'])
 (a.output/'report.json').write_text(json.dumps(report,sort_keys=True,indent=2)+'\n');print(json.dumps(report['summary']),flush=True)

if __name__=='__main__':main()

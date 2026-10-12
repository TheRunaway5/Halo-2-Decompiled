#!/usr/bin/env python3
"""Unchanged loop owners on genuine retained pool/engine/driver-prefix ABI bytes."""
import argparse,json,struct,hashlib
from pathlib import Path
from unicorn import Uc,UcError,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
from oracle_mp_title_update_display import ROOT,OLD,NEW,Xbe,_Code,sha,save,encoded
from oracle_sound_driver_master_gain_native import DRIVER,DRIVER_PIN
from oracle_sound_startup_native import AUDIO_ARENA
BASE=ROOT/'analysis/multiplayer/integration/sound-looping-native';PAIR=BASE/'paired-v5';STACK,STOP=0x02008000,0x03000000

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--preparation',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();assert not a.output.exists()
 prep=json.loads(a.preparation.read_text());pair=json.loads((PAIR/'report.json').read_text());providers=json.loads((PAIR/'providers.json').read_text());assert sha(DRIVER)==DRIVER_PIN;driver=json.loads(DRIVER.read_text());banks=[]
 pcs={};entries={};vas={}
 for row in pair['owners']:
  e=int(row['old'],16);entries[e]=int(row['u5'],16)
  left=(PAIR/'instructions'/f'{e:08x}-old.asm').read_text().splitlines();right=(PAIR/'instructions'/f'{e:08x}-u5.asm').read_text().splitlines();pcs.update({int(l.split()[0],16):int(r.split()[0],16)for l,r in zip(left,right,strict=True)})
  vas.update({int(m['old_va'],16):int(m['new_va'],16)for m in row['memory']})
 actual_request=prep['actual_request_calls'][0];assert actual_request['result']!='ffffffff'
 field_order=('allocator_pointer','arena_base_pointer','arena_used_pointer','arena_crc_pointer','pool_pointer','state_pointer','engine_pointer','engine_table','class_fades_pointer','class_fades_flag','driver_pointer','master_output_globals')
 old_bindings=(0x51841c,0x4ed650,0x4ed654,0x4ed65c,0x4f485c,0x4f4858,0x4f10b8,0x566084,0x5098f0,0x5098f4,0x526474,0x477544)
 for side,path in enumerate((OLD,NEW)):
  x=Xbe(path);code=_Code(x,'.text');packet=providers['bindings'][side];mutable={int(k,16):int(v,16)for k,v in packet['mutable_runtime_addresses'].items()};mutable.update({0x526474:vas[0x526474]if side else 0x526474,0x477544:vas[0x477544]if side else 0x477544})
  binding={name:(old if not side else mutable[old])for name,old in zip(field_order,old_bindings,strict=True)};reverse={v:k for k,v in pcs.items()};image_tag='old'if side==0 else'u5'
  plans=[('constructed_reset',prep['stages'][0]['after'],[(0x18a2a0,'actual_absent_driver_reset',[],None)],False),('class_construct',prep['stages'][2]['before'],[(0x223960,'actual_class_constructor',[],None)],False),('class_reset',prep['stages'][3]['before'],[(0x2239b0,'actual_class_reset',[],None)],False),('engine_reset',prep['stages'][4]['before'],[(0x18a790,'actual_engine_class_setup',[],None)],False)]
  if side==0:plans.insert(0,('loop_construct',prep['stages'][0]['before'],[(0x18a200,'actual_loop_constructor',[],None)],False))
  plans.append(('composed_driver_reset',prep['stages'][4]['before'],[(0x18a2a0,'explicit_driver_prefix_reset_ABI',[],None)],True))
  req=[int(actual_request[k],16)for k in('definition','object','marker','parameter')]
  plans.append(('actual_registration',actual_request['before'],[(0x18a9c0,'actual_BFCC6_registration',req,None),(0x18a9c0,'definition_NONE_raw_ABI',[0xffffffff,*req[1:]],None),(0x18a9c0,'detached_raw_ABI',[req[0],0xffffffff,0xffff,0],None)],False))
  plans.append(('capacity_sequence',actual_request['before'],[(0x18a9c0,f'repeated_raw_ABI_{i}',req,None)for i in range(1025)],False))
  selectors=[packet['empty_selector'],packet['dialogue_selector']]+[q for q in packet['names']if q['raw_hex']!='00'][:3]
  gains=(0,0xc2800000,0xc3000000,0xbf800000,0x3f800000,0x7fc00001,0xff800000,0x7f800000)
  times=(0,0x80000000,0x40000000,0xbf800000,0x7fc00001)
  controls=[(0x223a90,f'explicit_class_{i}_{g:08x}_{t:08x}',[int(q.get('address',q.get('pointer')),16),g,t],q['raw_hex'])for i,q in enumerate(selectors)for g in gains for t in times]
  plans.append(('class_controls',prep['final'],controls,False))
  executed=set()
  for name,state,calls,with_driver in plans:
   regions=[dict(address=state['arena_base'],raw_hex=state['arena_backing_hex'])]
   if name=='loop_construct':
    arena=bytes.fromhex(state['arena_backing_hex']);offset=int(state['allocator'],16)-int(state['arena_base'],16);vtable=int.from_bytes(arena[offset:offset+4],'little');regions.append(dict(address=f'{vtable:08x}',raw_hex=x.read(vtable,4).hex()))
   values={0x51841c:int(state['allocator'],16),0x4ed650:int(state['arena_base'],16),0x4ed654:int(state['arena_used'],16),0x4ed65c:int(state['arena_crc'],16),0x4f485c:int(state['pool'],16),0x4f4858:int(state['state'],16),0x4f10b8:int(state['engine'],16),0x5098f0:int(state['class_fades'],16),0x5098f4:int(state['class_fades_flag'],16),0x526474:0}
   for old,value in values.items():regions.append(dict(address=f'{binding[field_order[old_bindings.index(old)]]:08x}',raw_hex=value.to_bytes(1 if old==0x5098f4 else 4,'little').hex()))
   regions.append(dict(address=f'{binding["engine_table"]-4:08x}',raw_hex=state['engine_table44_hex']))
   regions.append(dict(address=f'{binding["master_output_globals"]:08x}',raw_hex=x.read(binding['master_output_globals'],48).hex()))
   if with_driver:
    q=driver['cases'][side*4];regions.append(dict(address=f'{AUDIO_ARENA:08x}',raw_hex=q['output_arena_hex']));v=int(q['after']['driver_address'],16);next(r for r in regions if int(r['address'],16)==binding['driver_pointer'])['raw_hex']=struct.pack('<I',v).hex()
   g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x800000)
   for s in x.sections:g.mem_write(s.va,x.section_bytes(s))
   ranges=[]
   for q in regions:
    lo=int(q['address'],16)&~4095;hi=(int(q['address'],16)+len(bytes.fromhex(q['raw_hex']))+4095)&~4095
    if lo>=0x810000:ranges.append((lo,hi))
   merged=[]
   for lo,hi in sorted(ranges):
    if merged and lo<=merged[-1][1]:merged[-1]=(merged[-1][0],max(hi,merged[-1][1]))
    else:merged.append((lo,hi))
   for lo,hi in merged:g.mem_map(lo,hi-lo)
   g.mem_map(0x2000000,0x10000);g.mem_map(STOP,4096)
   spans=[(int(q['address'],16),len(bytes.fromhex(q['raw_hex'])))for q in regions]
   def u(at,n=4):return int.from_bytes(g.mem_read(at,n),'little')
   def snapshots():return[(at,bytes(g.mem_read(at,n)))for at,n in spans]
   rows=[];stores=[];record=False
   def ins(_g,at,n,z):
    if at!=STOP and code.at(at):executed.add(at)
   def store(_g,k,at,n,value,z):
    if record and any(base<=at and at+n<=base+extent for base,extent in spans):
     pc=g.reg_read(UC_X86_REG_EIP);stores.append(dict(instruction=f'{pc if not side else reverse[pc]:08x}',address=f'{at:08x}',bytes=n,before=bytes(g.mem_read(at,n)).hex(),after=(value&((1<<(n*8))-1)).to_bytes(n,'little').hex()))
   g.hook_add(UC_HOOK_CODE,ins);g.hook_add(UC_HOOK_MEM_WRITE,store)
   for ordinal,(entry,label,args,selector)in enumerate(calls):
    if name!='capacity_sequence'or ordinal==0:
     for q in regions:g.mem_write(int(q['address'],16),bytes.fromhex(q['raw_hex']))
    for reg in(UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP):g.reg_write(reg,0)
    stack_args=args if entry==0x223a90 else args[2:]if entry==0x18a9c0 else[]
    if entry==0x18a9c0:g.reg_write(UC_X86_REG_ESI,args[0]);g.reg_write(UC_X86_REG_EDI,args[1])
    g.mem_write(STACK,struct.pack('<'+'I'*(len(stack_args)+1),STOP,*stack_args));g.reg_write(UC_X86_REG_ESP,STACK);g.reg_write(UC_X86_REG_FPCW,0x027f);g.reg_write(UC_X86_REG_MXCSR,0x1f80)
    before=snapshots();stores=[];record=True;returned=True
    try:g.emu_start(entry if not side else entries[entry],STOP,count=1000000)
    except UcError as error:returned=False;failure=str(error)
    record=False;stop=g.reg_read(UC_X86_REG_EIP);expected_stop=0x22310c if not side else pcs[0x22310c]
    assert (stop==STOP if returned else(name=='constructed_reset'and stop==expected_stop)),(image_tag,name,label,hex(stop),failure if not returned else'no return')
    after=snapshots();expected={at:bytearray(raw)for at,raw in before}
    for s in stores:
     at=int(s['address'],16);base=max(b for b in expected if b<=at);off=at-base;assert expected[base][off:off+s['bytes']].hex()==s['before'];expected[base][off:off+s['bytes']]=bytes.fromhex(s['after'])
    assert all(expected[at]==raw for at,raw in after)
    rows.append(dict(name=label,entry=f'{entry:08x}',arguments=[f'{v:08x}'for v in args],selector_hex=selector,returned=returned,result=f'{g.reg_read(UC_X86_REG_EAX):08x}'if entry==0x18a9c0 else'00000000',stores=stores,full_region_sha256=[dict(address=f'{at:08x}',sha256=hashlib.sha256(raw).hexdigest())for at,raw in after]))
   banks.append(dict(image=image_tag,name=name,bindings={k:f'{v:08x}'for k,v in binding.items()},constants=packet,regions=regions,cases=rows,sequential=name=='capacity_sequence',driver_prefix_composition=with_driver,source_gameplay_or_platform_success_supplied=False));print(json.dumps(dict(image=image_tag,bank=name,cases=len(rows))),flush=True)
  seq=[code.at(pc)for pc in sorted(executed)]
  for i in seq:assert i.bytes==x.read(i.address,len(i.bytes))
  save(a.output/'instructions'/f'{image_tag}-executed.asm',''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in seq).encode())
 report=dict(schema='sound-looping-native-original-v1',tool_sha256=sha(__file__),preparation=dict(path=str(a.preparation),sha256=sha(a.preparation)),paired_report_sha256=sha(PAIR/'report.json'),driver_allocation_prefix_sha256=DRIVER_PIN,banks=banks,source_success_returns_supplied=[],instruction_patches=[],limits=['Driver-prefix reset is explicitly composed raw ABI backing, not natural startup, device success or sound admission.','Only1.1 whole pool constructor executes on its actual allocator/vtable. U5 constructor correspondence is static; u5 remaining owners consume explicitly retained1.1 source arena/engine ABI backing.','Capacity sequence and class scalar controls are direct raw ABIs, not ordinary world object production or live audio.','Natural absent-driver reset remains unreturned; no saved setting, voice, local listener or consumption is manufactured.'])
 save(a.output/'report.json',encoded(report));print(json.dumps(dict(report_sha256=sha(a.output/'report.json'),calls=sum(len(b['cases'])for b in banks))))
if __name__=='__main__':main()

#!/usr/bin/env python3
"""Original command/control kernel on captured owner records and declared stimuli.

No full world preparation is claimed by this isolated ABI fixture. Complete
actual owner records come from the immutable source-created Spartan capture;
all changed gate/command words are explicit. Execution stops at C8140 entry,
after the actual packet producer and target+134 store. No return is supplied.
"""
import argparse,json,struct
import unicorn
from unicorn.x86_const import *
from oracle_mp_title_update_display import Xbe,_Code,_trace,OLD,NEW,PINS as IMAGES,sha,save,encoded
from export_player_control_fixtures import ROOT,BASE,PINS,packet,unit,u
from research_mp_original_data_u5 import norm
OUT=ROOT/'analysis/multiplayer/integration/inventory-source/player-control-kernel'
STATIC=ROOT/'analysis/multiplayer/integration/inventory-source/live-locomotion-owners-u5/complete/report.json'
EXTRA={0xccc60:0xcdcf0,0xc8250:0xc93d0,0x1218c0:0x122b60}
STACK,STOP,COMMAND=0x021f8000,0x03000000,0x01011000
POOLDESC,PLAYDESC,HEADERS=0x01000000,0x01000100,0x01002000
def h(v,n=4):return f'{v:0{n*2}x}'
def setup():
 for p,pin in {**PINS,**{__import__('pathlib').Path(k):v for k,v in IMAGES.items()}}.items():assert sha(p)==pin,p
 original=json.loads((BASE/'report.json').read_text());owners=json.loads(STATIC.read_text())['owners']
 mapping={int(q['old_entry'],16):int(q['u5_entry'],16)for q in owners}
 mapping.update(EXTRA)
 globals_map={int(p['old_va'],16):int(p['u5_va'],16)for q in owners for p in q['absolute_image_memory_operands']}
 images=[Xbe(OLD),Xbe(NEW)];codes=[_Code(x,'.text')for x in images];extra=[]
 for old,new in EXTRA.items():
  ins=[]
  for i,e in enumerate((old,new)):
   r=_trace(codes[i],e,{e});v=[codes[i].at(a)for a in sorted(r.code)];positions={q.address:n for n,q in enumerate(v)}
   ins.append((v,[norm(q,images[i],positions)for q in v]))
  assert ins[0][1]==ins[1][1],h(old)
  extra.append(dict(old_entry=h(old),u5_entry=h(new),full_instruction_and_internal_branch_topology_equal=True,
   old_rows=len(ins[0][0]),u5_rows=len(ins[1][0])))
 return original,mapping,globals_map,images,codes,extra
def inputs(original):
 cases=[]
 for case in original['cases']:
  name=case['case'];call=next(q for q in original['calls']if q['probe']==name and q['entry']=='00151c90')
  state=original['states'][call['before_state']]
  cases.append(dict(id='captured_'+name,state=state,command=packet(call,'stack1_command5c'),changes=[]))
 base=cases[9] # Genuine authored-facing forward command/owner state.
 assert base['id']=='captured_authored_heading_forward'
 variants=[('no_current_unit','player',0x2c,4,0xffffffff),
  ('wrong_object_type','header',3,1,2),('has_parent','body',0x14,4,0xe2700001),
  ('physical_zero','body',0x3dc,1,0),('physical_three','body',0x3dc,1,3),
  ('restricted_unit','body',0x348,1,1)]
 for name,where,offset,width,value in variants:
  changes=[dict(buffer=where,offset=offset,width=width,bits=h(value,width))]
  if name=='restricted_unit':changes.append(dict(buffer='body',offset=0x399,width=1,bits='06'))
  cases.append(dict(id='declared_'+name,state=base['state'],command=base['command'],changes=changes))
 for name,x,y in [('signed_zero',0x80000000,0x80000000),('above_unit_diagonal',0x40000000,0x40000000),
                  ('negative_diagonal',0xbf800000,0xbf800000),('epsilon_axes',0x38d1b717,0),
                  ('maximum_finite_axes',0x7f7fffff,0x7f7fffff)]:
  command=bytearray(base['command']);struct.pack_into('<II',command,0xc,x,y)
  cases.append(dict(id='declared_'+name,state=base['state'],command=bytes(command),changes=[dict(buffer='command',offset=0xc,width=8,raw_hex=struct.pack('<II',x,y).hex())]))
 for name,pitch in [('vertical_up',0x3fc90fdb),('vertical_down',0xbfc90fdb)]:
  command=bytearray(base['command']);struct.pack_into('<I',command,8,pitch)
  cases.append(dict(id='declared_'+name,state=base['state'],command=bytes(command),changes=[dict(buffer='command',offset=8,width=4,bits=h(pitch))]))
 cases.append(dict(id='declared_clear_target_flag',state=base['state'],command=base['command'],
  changes=[dict(buffer='body',offset=0x134,width=4,bits='c2345678')]))
 command=bytearray(base['command'])
 # Literal declared packet values test owner copies, including byte58 gate.
 changes=[]
 for at,raw in [(0,struct.pack('<I',0xa5b67c81)),(0x14,struct.pack('<II',0x3e800000,0x3f000000)),
                (0x1e,bytes.fromhex('34120102cdab7856')),(0x34,bytes(range(0x24))),
                (0x58,b'\x01')]:
  command[at:at+len(raw)]=raw;changes.append(dict(buffer='command',offset=at,width=len(raw),raw_hex=raw.hex()))
 cases.append(dict(id='declared_literal_packet_copies',state=base['state'],command=bytes(command),changes=changes))
 return cases
def execute(case,image,code,mapping,globals_map,index,out):
 state=case['state'];player=bytearray.fromhex(state['player']['raw21c_hex']);datum=u(player,0x2c)
 obj=unit(state,datum);body=bytearray.fromhex(obj['body_hex']);header=bytearray.fromhex(obj['header_hex'])
 for q in case['changes']:
  if q['buffer']=='command':continue
  raw=bytes.fromhex(q.get('raw_hex',''))if'raw_hex'in q else int(q['bits'],16).to_bytes(q['width'],'little')
  target={'player':player,'body':body,'header':header}[q['buffer']];target[q['offset']:q['offset']+q['width']]=raw
 # Supplied storage pointer relocation is ABI setup, not a new ownership claim.
 g=unicorn.Uc(unicorn.UC_ARCH_X86,unicorn.UC_MODE_32);g.mem_map(0x10000,0x800000)
 for s in image.sections:g.mem_write(s.va,image.section_bytes(s))
 for p,n in [(0x01000000,0x20000),(0x02000000,0x200000),(STOP,0x1000),(0x82d40000,0x10000),(0x82d80000,0x10000)]:g.mem_map(p,n)
 bp=int(obj['body_address'],16);pp=int(state['player']['pointer'],16);clock=int(state['clock']['pointer'],16)
 g.mem_write(bp,bytes(body));g.mem_write(pp,bytes(player));g.mem_write(HEADERS,bytes(header))
 assert u(header,8)==bp and len(header)==12
 def global_address(p):return globals_map[p]if index else p
 for p,v in [(0x4e78d0,POOLDESC),(0x4f01f4,PLAYDESC),(0x518444,clock)]:g.mem_write(global_address(p),struct.pack('<I',v))
 g.mem_write(POOLDESC+0x44,struct.pack('<I',HEADERS));g.mem_write(PLAYDESC+0x44,struct.pack('<I',pp))
 g.mem_write(clock,bytes.fromhex(state['clock']['raw24_hex']));g.mem_write(COMMAND,case['command'])
 g.mem_write(STACK,struct.pack('<III',STOP,datum,COMMAND));g.reg_write(UC_X86_REG_ESP,STACK)
 g.reg_write(UC_X86_REG_EAX,int(state['player']['datum'],16));g.reg_write(UC_X86_REG_FPCW,0x027f);g.reg_write(UC_X86_REG_MXCSR,0x1f80)
 entries={e:(mapping[e]if index else e)for e in [0x151c90,0x151aa0,0x151a20,0x151980,0xe7b10,0xe5680,0xc8250,0x30bd0,0x1218c0,0xccc60,0xc8140]}
 executed=[];reads=[];stores=[];calls=[];pending=[];observed={};math=[];order=0
 def raw(p,n):return bytes(g.mem_read(p,n)).hex()
 def regs():return {name:h(g.reg_read(r))for name,r in [('eax',UC_X86_REG_EAX),('ebx',UC_X86_REG_EBX),('ecx',UC_X86_REG_ECX),('edx',UC_X86_REG_EDX),('esi',UC_X86_REG_ESI),('edi',UC_X86_REG_EDI),('esp',UC_X86_REG_ESP)]}
 def fp():return dict(fpcw=h(g.reg_read(UC_X86_REG_FPCW)),fpsw=h(g.reg_read(UC_X86_REG_FPSW)),
  mxcsr=h(g.reg_read(UC_X86_REG_MXCSR)),x87=[list(g.reg_read(UC_X86_REG_FP0+i))for i in range(8)],
  xmm=[f'{g.reg_read(UC_X86_REG_XMM0+i):032x}'for i in range(8)])
 def packets(old,sp):
  r=regs();args=[int.from_bytes(g.mem_read(sp+4+i*4,4),'little')for i in range(3)];v=[]
  def add(name,p,n):v.append(dict(field=name,pointer=h(p),size=n,raw_hex=raw(p,n)))
  if old==0x151c90:add('command5c',args[1],0x5c)
  if old==0x151aa0:
   add('axes8',int(r['ebx'],16),8)
   for name,p in zip(('aim12','facing12_output','motion12_output'),args):add(name,p,12)
  if old==0x151980:
   for name,k,n in [('aim12','eax',12),('fallback12','edi',12),('basis24_output','esi',24)]:add(name,int(r[k],16),n)
  if old in(0x30bd0,0x1218c0):
   add('vector12_inout',int(r['eax'],16),12)
   if old==0x1218c0:add('fallback12',int(r['ecx'],16),12)
  if old==0xe7b10:
   for name,k in [('world_movement12','eax'),('unit_forward12','edi'),('aim12','esi')]:add(name,int(r[k],16),12)
   add('selected_facing12_output',args[1],12)
  if old==0xc8250:add('control7c_output',int(r['edx'],16),0x7c)
  if old==0xccc60:add('aim12_inout',args[0],12)
  if old==0xc8140:add('control7c_consumer',int(r['ecx'],16),0x7c)
  return v
 def hook(_g,p,n,_):
  nonlocal order
  executed.append(p)
  for ret,sp,row in list(reversed(pending)):
   if ret==p:
    row.update(return_order=order,return_registers=regs(),return_fp=fp(),return_body_hex=raw(bp,len(body)),
     after_packets=[dict(q,raw_hex=raw(int(q['pointer'],16),q['size']))for q in row['packets']]);order+=1;pending.remove((ret,sp,row))
  if p in entries.values():
   old=next(k for k,v in entries.items()if v==p);sp=g.reg_read(UC_X86_REG_ESP)
   row=dict(entry=h(p),corresponding_old_entry=h(old),order=order,registers=regs(),entry_fp=fp(),stack20_hex=raw(sp,0x20),body_hex=raw(bp,len(body)),packets=packets(old,sp))
   calls.append(row);order+=1
   if old==0xc8140:
    observed.update(control7c_hex=raw(g.reg_read(UC_X86_REG_ECX),0x7c),consumer_registers=regs());g.emu_stop();return
   pending.append((int.from_bytes(g.mem_read(sp,4),'little'),sp,row))
  instruction=code.at(p)
  if instruction.mnemonic.startswith(('f','j'))or instruction.mnemonic.endswith('ss'):
   math.append(dict(order=order,instruction=h(p),registers=regs(),fp=fp(),stack40_hex=raw(g.reg_read(UC_X86_REG_ESP),0x40)));order+=1
  order+=1
 def read(_g,k,p,n,v,_):
  nonlocal order
  reads.append(dict(order=order,instruction=h(g.reg_read(UC_X86_REG_EIP)),address=h(p),size=n,raw_hex=raw(p,n)));order+=1
 def write(_g,k,p,n,v,_):
  nonlocal order
  stores.append(dict(order=order,instruction=h(g.reg_read(UC_X86_REG_EIP)),address=h(p),size=n,before_hex=raw(p,n),written_hex=(v&((1<<(8*n))-1)).to_bytes(n,'little').hex()));order+=1
 g.hook_add(unicorn.UC_HOOK_CODE,hook);g.hook_add(unicorn.UC_HOOK_MEM_READ,read);g.hook_add(unicorn.UC_HOOK_MEM_WRITE,write)
 g.emu_start(entries[0x151c90],STOP,count=100000)
 assert g.reg_read(UC_X86_REG_EIP)==entries[0xc8140] and 'control7c_hex'in observed
 for p in set(executed):i=code.at(p);assert i.bytes==image.read(p,i.size)==bytes(g.mem_read(p,i.size))
 admit=next(c for c in calls if c['corresponding_old_entry']=='00151a20');restricted=next((c for c in calls if c['corresponding_old_entry']=='000e5680'),None)
 current=dict(datum=h(u(player,0x2c)),object_type=h(header[3],1),parent=h(u(body,0x14)),physical_state=h(body[0x3dc],1),
  restricted_e5680=int(restricted['return_registers']['eax'],16)&255 if restricted else 0,
  forward12_hex=bytes(body[0x70:0x7c]).hex(),related_object_13c=h(u(body,0x13c)),seat=h(u(body,0x1fc,2),2))
 row=dict(id=('u5_'if index else'old_')+case['id'],image='authenticated_u5'if index else'owned_1_1',
  command5c_hex=case['command'].hex(),current=current,target_flags134_before=h(u(body,0x134)),target_flags134_after=h(int.from_bytes(g.mem_read(bp+0x134,4),'little')),
  admitted_al=int(admit['return_registers']['eax'],16)&255,**observed,
  input_player21c_hex=bytes(player).hex(),input_header12_hex=bytes(header).hex(),input_body_hex=bytes(body).hex(),
  output_body_hex=raw(bp,len(body)),input_clock24_hex=state['clock']['raw24_hex'],declared_changes=case['changes'],calls=calls,reads=reads,stores=stores,
  arithmetic_before=math,executed_instructions=[h(p)for p in executed],source_state_id=state['id'],stop='Original C8140 consumer entry, no supplied return or native service')
 return row,set(executed)
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=__import__('pathlib').Path,default=OUT/'report.json');a=ap.parse_args()
 original,mapping,globals_map,images,codes,extra=setup();cases=[];executed=[set(),set()]
 for case in inputs(original):
  pair=[]
  for index in range(2):
   row,addresses=execute(case,images[index],codes[index],mapping,globals_map,index,a.output.parent);cases.append(row);pair.append(row);executed[index]|=addresses
  for key in ['control7c_hex','target_flags134_after','admitted_al','output_body_hex']:assert pair[0][key]==pair[1][key],(case['id'],key)
  if case['id'].startswith('captured_'):
   name=case['id'][9:];consumer=next(c for c in original['calls']if c['probe']==name and c['entry']=='000c8140')
   assert pair[0]['control7c_hex']==packet(consumer,'ecx_control7c').hex(),name
 listings=[]
 for index,addresses in enumerate(executed):
  for label,addresses2 in [('executed',addresses)]+[(h(e),_trace(codes[index],mapping[e]if index else e,{mapping[e]if index else e}).code)for e in EXTRA]:
   path=a.output.parent/'instructions'/f'{index}-{label}.asm';ins=[codes[index].at(p)for p in sorted(addresses2)]
   save(path,''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins).encode());listings.append(dict(file=str(path.relative_to(a.output.parent)),sha256=sha(path),rows=len(ins)))
 report=dict(schema=1,oracle_sha256=sha(__file__),source_pins={str(p):pin for p,pin in {**PINS,**IMAGES}.items()},
  extra_complete_correspondences=extra,cases=cases,listings=listings,instruction_patches=[],supplied_service_results=[],
  scope='Isolated source kernel ABI with literal captured source-owned records, explicit storage relocation and listed declared gate/command stimuli. No world/local-device/ready/frame admission claim. Existing captured control outputs correlate exactly before extending. Stops at C8140 before its consumer executes.',
  fp='FPCW027F/MXCSR1F80; host Unicorn x87 boundary retained, not Xbox transcendental hardware parity')
 save(a.output,encoded(report));print(json.dumps(dict(cases=len(cases),report=str(a.output),sha256=sha(a.output))))
if __name__=='__main__':main()

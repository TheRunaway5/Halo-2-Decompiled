#!/usr/bin/env python3
"""Untouched paired-image physics world runs on explicitly rebound retained raw inputs.

The TU1.5 inputs are a declared rebinding of the captured 1.1 call state, not a
claim that its full game world or caller was executed by TU1.5. Output packets,
branches, stores and registers are produced only by the selected executable.
"""
import argparse,json,hashlib,struct,sys
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code,PIN
from capstone.x86_const import X86_OP_REG,X86_OP_MEM
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from oracle_mp_spawn_biped_platform import segment
from oracle_physics_world_runtime_20261011 import captured_abi as abi
from export_mp_physics_contact_lifecycle import segments

def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--input',type=Path,required=True);ap.add_argument('--correspondence',type=Path,required=True);ap.add_argument('--bindings',type=Path,required=True);ap.add_argument('--image',choices=('1.1','1.5'),required=True);ap.add_argument('--output',type=Path,required=True);ap.add_argument('--x87-stress',action='store_true',help='Declared FP input perturbations; expected results come only from original execution');a=ap.parse_args()
 original=json.loads(a.input.read_text());paired=json.loads(a.correspondence.read_text());latest=a.image=='1.5'
 assert original['correspondence_sha256']==sha(a.correspondence)
 mapping={int(k,16):v for key in ('address_map','instruction_map','entries') for k,v in paired[key].items()};sites={v:int(k,16) for k,v in paired['instruction_map'].items()}
 bindings=json.loads(a.bindings.read_text());assert bindings['tool_sha256']==sha(ROOT/'tools/research_physics_world_runtime_extra_bindings_20261011.py')
 assert bindings['disc_sha256']==PIN and bindings['later_sha256']==NEW_PIN
 for k,v in bindings['address_map'].items():
  p=int(k,16);assert p not in mapping or mapping[p]==v;mapping[p]=v
 regions=bindings['input_regions']
 old=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');path=NEW_DEFAULT if latest else old.path if hasattr(old,'path') else ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'
 assert sha(path)==(NEW_PIN if latest else PIN);image=Xbe(path);runtime_code=_Code(image,'.text')
 oldbytes=bytearray(0x810000)
 for s in old.sections:oldbytes[s.va:s.va+s.rsize]=old.section_bytes(s)
 def selected(p):
  if not latest:return p
  if p in mapping:return mapping[p]
  for r in regions:
   if r['old']<=p<r['old']+r['size']:return r['later']+p-r['old']
  return p
 def rebase(raw):
  raw=bytearray(raw)
  if latest:
   for i in range(0,len(raw)-3,4):
    p=int.from_bytes(raw[i:i+4],'little')
    raw[i:i+4]=struct.pack('<I',selected(p))
  return bytes(raw)
 input_cases=[]
 for c in original['cases']:
  if not a.x87_stress:input_cases.append(c);continue
  for depth in (0,1,3):
   for top in range(8):
    row=json.loads(json.dumps(c));state=row['entry_abi'];state['x87_status']=(state['x87_status']&~0x3800)|(top<<11);tag=65535
    values=['0000000000000080ff3f','0000000000000080febf','00000000000000800040'][:depth]
    for n in range(depth):tag&=~(3<<(((top+n)&7)*2))
    state['x87_tag']=tag;state['x87_values']=values;row['input_perturbation']=dict(original_case=c['id'],top=top,depth=depth,values=values);row['id']=len(input_cases);input_cases.append(row)
 cases=[];known={int(k,16) for k in paired['instruction_map']}
 for c in input_cases:
  g=Uc(UC_ARCH_X86,UC_MODE_32);pages=set()
  def ensure(p,n):
   for q in range(p&~4095,(p+n+4095)&~4095,4096):
    if q not in pages:g.mem_map(q,4096);pages.add(q)
  for s in image.sections:ensure(s.va,s.vsize);g.mem_write(s.va,image.section_bytes(s))
  before={};declared=[]
  for b in c['initial_banks']:
   p=b['address'];raw=bytes.fromhex(b['hex'])
   source=next((s for s in old.sections if s.va<=p<s.va+s.vsize),None)
   if latest and source:
    if raw==bytes(oldbytes[p:p+len(raw)]):continue # actual selected image owns immutable/unchanged bytes
    # Mutable operands are authenticated at their actual referenced base.
    for offset in range(0,len(raw)):
     if raw[offset]==oldbytes[p+offset]:continue
     address=p+offset;anchors=[k for k in mapping if k<=address<k+4 and oldbytes[k:k+4]!=raw[max(0,k-p):max(0,k-p)+4]]
     assert anchors or any(r['old']<=address<r['old']+r['size'] for r in regions),('unbound mutable image input',hex(address),hex(p))
    if p in mapping:q=mapping[p]
    elif any(r['old']<=p<r['old']+r['size'] for r in regions):q=selected(p)
    else:
     anchors=[k for k in mapping if k<=p<k+4];assert anchors,('unbound image bank',hex(p));k=max(anchors);q=mapping[k]+p-k
   else:q=p
   value=rebase(raw) if p%4==0 else raw
   ensure(q,len(value));g.mem_write(q,value);declared.append(dict(old=p,selected=q,bytes=len(raw)))
  state=json.loads(json.dumps(c['entry_abi']))
  if latest:
   state['gp']=[selected(x) for x in state['gp']]
   state['xmm']=[rebase(bytes.fromhex(x)[::-1])[::-1].hex() for x in state['xmm']]
  GDT=0x7000000;ensure(GDT,4096);g.mem_write(GDT+8,segment(0,0x9b)+segment(0,0x93)+segment(state['fs_base'],0x93));g.reg_write(R.UC_X86_REG_GDTR,(0,GDT,0xfff,0))
  for name,value in (('CS',8),('DS',16),('ES',16),('SS',16),('FS',24)):g.reg_write(getattr(R,'UC_X86_REG_'+name),value)
  for i,name in enumerate(('EAX','ECX','EDX','EBX','ESP','EBP','ESI','EDI')):g.reg_write(getattr(R,'UC_X86_REG_'+name),state['gp'][i])
  for i,x in enumerate(state['xmm']):g.reg_write(getattr(R,f'UC_X86_REG_XMM{i}'),int(x,16))
  for name,key in (('EFLAGS','eflags'),('FPCW','x87_control_word'),('MXCSR','mxcsr'),('FPSW','x87_status'),('FPTAG','x87_tag')):g.reg_write(getattr(R,'UC_X86_REG_'+name),state[key])
  top=(state['x87_status']>>11)&7
  for n,raw in enumerate(state.get('x87_values',[])):
   value=bytes.fromhex(raw);g.reg_write(getattr(R,f'UC_X86_REG_FP{(top+n)&7}'),(int.from_bytes(value[:8],'little'),int.from_bytes(value[8:],'little')))
  trace=[];events=[];packets=[];estimates=[];timestamps=[];pending=None;exit_entry=None
  def read(_g,_a,p,n,_x,_u):events.append(dict(kind='R',instruction=sites.get(g.reg_read(R.UC_X86_REG_EIP),g.reg_read(R.UC_X86_REG_EIP)) if latest else g.reg_read(R.UC_X86_REG_EIP),address=p,raw=bytes(g.mem_read(p,n)).hex()))
  def write(_g,_a,p,n,x,_u):events.append(dict(kind='W',instruction=sites.get(g.reg_read(R.UC_X86_REG_EIP),g.reg_read(R.UC_X86_REG_EIP)) if latest else g.reg_read(R.UC_X86_REG_EIP),address=p,before=bytes(g.mem_read(p,n)).hex(),written=(x&((1<<(8*n))-1)).to_bytes(n,'little').hex()))
  def code(_g,p,_n,_u):
   nonlocal pending,exit_entry
   if pending is not None:
    kind,site,target,value=pending;pending=None
    if kind=='rsqrtss':estimates.append(dict(instruction=site,input=value,output=g.reg_read(getattr(R,'UC_X86_REG_'+target.upper()))&0xffffffff))
    else:timestamps.append(dict(instruction=site,output=(g.reg_read(R.UC_X86_REG_EDX)<<32)|g.reg_read(R.UC_X86_REG_EAX)))
   if p==selected(c['return_address']):
    exit_entry=p;g.emu_stop();return
   canonical=sites.get(p) if latest else p
   # TU addresses can numerically overlap an unrelated old instruction. Only
   # an authenticated correspondence admits execution in the selected image.
   if canonical is None or canonical not in known:
    exit_entry=p;g.emu_stop();return
   trace.append(canonical)
   instruction=runtime_code.at(p)
   if instruction.mnemonic=='rsqrtss':
    operand=instruction.operands[1]
    if operand.type==X86_OP_REG:value=g.reg_read(getattr(R,'UC_X86_REG_'+instruction.reg_name(operand.reg).upper()))&0xffffffff
    else:
     assert operand.type==X86_OP_MEM and not operand.mem.segment
     address=operand.mem.disp
     if operand.mem.base:address+=g.reg_read(getattr(R,'UC_X86_REG_'+instruction.reg_name(operand.mem.base).upper()))
     if operand.mem.index:address+=operand.mem.scale*g.reg_read(getattr(R,'UC_X86_REG_'+instruction.reg_name(operand.mem.index).upper()))
     value=int.from_bytes(g.mem_read(address&0xffffffff,4),'little')
    pending=('rsqrtss',canonical,instruction.reg_name(instruction.operands[0].reg),value)
   elif instruction.mnemonic=='rdtsc':pending=('rdtsc',canonical,None,None)
   if canonical==0x300100:
    sp=g.reg_read(R.UC_X86_REG_ESP);q=int.from_bytes(g.mem_read(sp+4,4),'little');packets.append(dict(input=q,destination=g.reg_read(R.UC_X86_REG_ECX),input_hex=bytes(g.mem_read(q,128)).hex()))
  g.hook_add(UC_HOOK_CODE,code);g.hook_add(UC_HOOK_MEM_READ,read);g.hook_add(UC_HOOK_MEM_WRITE,write)
  try:g.emu_start(selected(c['entry']),0,count=1000000)
  except Exception as e:
   edx=g.reg_read(R.UC_X86_REG_EDX)
   raise RuntimeError(f'case {c["id"]}: last {trace[-1]:08x}, EIP {g.reg_read(R.UC_X86_REG_EIP):08x}, EDX {edx:08x}, slot30 {bytes(g.mem_read(edx+0x30,4)).hex()}: {e}') from e
  assert exit_entry is not None
  memory={};current={}
  for e in events:
   p=e['address'];raw=bytes.fromhex(e['raw'] if e['kind']=='R' else e['before'])
   for n,v in enumerate(raw):
    if p+n not in current:memory[p+n]=v;current[p+n]=v
    assert current[p+n]==v,(c['id'],e)
   if e['kind']=='W':
    for n,v in enumerate(bytes.fromhex(e['written'])):current[p+n]=v
  row=dict(id=c['id'],phase=c['phase'],entry=c['entry'],entry_abi=state,return_address=selected(c['return_address']),exit=abi(g),exit_entry=exit_entry,complete=exit_entry==selected(c['return_address']),
   initial_banks=[dict(address=p,hex=b.hex()) for p,b in segments(memory)],final_banks=[dict(address=p,hex=b.hex()) for p,b in segments(current)],
   writes=[e for e in events if e['kind']=='W'],instructions=trace,packets=packets,estimates=estimates,timestamps=timestamps,declared_raw_input_rebindings=declared)
  if 'input_perturbation' in c:row['input_perturbation']=c['input_perturbation']
  if not latest and not a.x87_stress:
   for key in ('entry_abi','exit','exit_entry','complete','initial_banks','final_banks','writes','instructions','packets','estimates','timestamps'):
    assert row[key]==c[key],('1.1 retained capture replay differs',c['id'],key)
  cases.append(row)
 result=dict(schema=1,image=a.image,tool_sha256=sha(__file__),input_fixture_sha256=sha(a.input),correspondence_sha256=sha(a.correspondence),input_bindings_sha256=sha(a.bindings),original_sha256=sha(path),cases=cases,
  declared_x87_input_perturbations=a.x87_stress,source_instruction_patches=[],native_expected_data_used=False,live_gameplay_proof=False,scope=__doc__)
 a.output.mkdir(parents=True,exist_ok=False);p=a.output/'fixture.json';p.write_text(json.dumps(result,sort_keys=True,separators=(',',':'))+'\n');print(json.dumps(dict(sha256=sha(p),cases=len(cases),instructions=sum(len(c['instructions']) for c in cases),packets=sum(len(c['packets']) for c in cases))))
if __name__=='__main__':main()

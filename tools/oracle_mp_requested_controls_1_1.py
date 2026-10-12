#!/usr/bin/env python3
"""Separate original device→command/control requests for requested native keys.

Keyboard binding is a host intent adapter, not a claimed retail key layout.
Unchanged original polling/profile/control/packet instructions generate all
action bits. Hardware packet/duration and inherited equipped/world services
remain explicit. No melee hit, grenade launch, zoom timer or action success is
inserted. Prior equipped/attachment suites remain read-only.
"""
import argparse,csv,hashlib,json,struct
from pathlib import Path
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDI,UC_X86_REG_ESP,UC_X86_REG_EIP
from oracle_mp_weapon_attachment_1_1 import AttachmentTrace
from oracle_mp_inventory_equipped_1_1 import ROOT,PIN,CACHE_PIN,UNIT,UI,TIME
from oracle_mp_inventory_controls_1_1 import ACTION,LOGICAL,RAW,PROFILE,CONTROL
from oracle_smg_firing_1_1 import _trace

ATTACHMENT_PIN='9895480d4d864d67ee92307d627c04a005e4ffb8f06761be68c9de0d7274563c'
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def h(v):return f'{v&0xffffffff:08x}'
WATCH=(0x150dd0,0x150f00,0x151c90,0xc8140,0xc6c50,0xe7f30,0xe7ef0,
 0xe95e0,0xe99b0,0xe9a50,0xeb530,0xeb080,0xeb700,0xeb830,0xc9920,
 0xc9cc0,0x1051c0,0xcf300,0xe9fe0,0xea180,0xe9d50,0x1045e0)
class Trace(AttachmentTrace):
 def __init__(self):
  self.measure=False;self.measured_calls=[];self.measured_returns=[];self.request_writes=[]
  super().__init__()
  starts={int(r['address'],16)for r in csv.DictReader((ROOT/'analysis/map-pack-update/functions.tsv').open(),delimiter='\t')}
  self.measured_cleanup={}
  for p in WATCH:
   f=_trace(self.code,p,starts|set(WATCH));returns=[self.code.at(q)for q in sorted(f.code)if self.code.at(q).mnemonic.startswith('ret')]
   cleanup={int(i.op_str,0)if i.op_str else 0 for i in returns}
   assert len(cleanup)==1,(h(p),cleanup)
   self.measured_cleanup[p]=next(iter(cleanup))
 def hook(self,g,p,size,user):
  if self.measure:
   sp=g.reg_read(UC_X86_REG_ESP)
   for target,return_sp,row in self.measured_returns[:]:
    if p==target and sp==return_sp:
     row['returned_eax']=h(g.reg_read(UC_X86_REG_EAX));row['after_unit_hex']=bytes(g.mem_read(UNIT,0x800)).hex()
     self.measured_returns.remove((target,return_sp,row))
   if p in WATCH:
    row={'phase':self.active,'entry':h(p),'return_site':h(self.u(sp)),
     'eax':h(g.reg_read(UC_X86_REG_EAX)),'ecx':h(g.reg_read(UC_X86_REG_ECX)),
     'edi':h(g.reg_read(UC_X86_REG_EDI)),'stack_args':[h(self.u(sp+4+n*4))for n in range(4)],
     'entry_esp':h(sp),'source_ret_cleanup':self.measured_cleanup[p],
     'before_unit_hex':bytes(g.mem_read(UNIT,0x800)).hex(),
     'before_control_hex':bytes(g.mem_read(UNIT+self.u(UNIT+0x346,2),0x44)).hex()}
    if p==0xe7f30:row['request_hex']=bytes(g.mem_read(g.reg_read(UC_X86_REG_EDI),0x20)).hex()
    self.measured_calls.append(row);self.measured_returns.append((self.u(sp),sp+4+self.measured_cleanup[p],row))
  super().hook(g,p,size,user)
 def mem_write(self,g,access,p,size,value,user):
  if self.measure and (UNIT<=p and p+size<=UNIT+0x800 or CONTROL<=p and p+size<=CONTROL+0x300 or ACTION<=p and p+size<=ACTION+0x5c):
   self.request_writes.append({'phase':self.active,'instruction':h(g.reg_read(UC_X86_REG_EIP)),'address':h(p),'size':size,
    'before_hex':bytes(g.mem_read(p,size)).hex(),'after_hex':(value&((1<<(size*8))-1)).to_bytes(size,'little').hex()})
  super().mem_write(g,access,p,size,value,user)

def decode_command(b):
 return {'gameplay_flags':h(struct.unpack_from('<I',b)[0]),'action_flags':f'{struct.unpack_from("<H",b,0x1c)[0]:04x}',
  'sequence':struct.unpack_from('<H',b,0x1e)[0],'requested_primary':struct.unpack_from('<b',b,0x20)[0],
  'requested_secondary':struct.unpack_from('<b',b,0x21)[0],'requested_grenade':struct.unpack_from('<h',b,0x22)[0],
  'requested_zoom':struct.unpack_from('<h',b,0x24)[0],'primary_trigger_bits':h(struct.unpack_from('<I',b,0x14)[0]),
  'secondary_trigger_bits':h(struct.unpack_from('<I',b,0x18)[0]),'full_5c_hex':b.hex()}

def measure_button(button,process=False):
 t=Trace();t.measure=True;rows=[];failure=None
 initial={'initial_smG':t.initial_equipped,'player_pose':t.player_pose,'unit_hex':bytes(t.g.mem_read(UNIT,0x800)).hex(),
  'profile_hex':bytes(t.g.mem_read(PROFILE,28)).hex(),'services':[{ 'entry':h(p),'contract':v}for p,v in t.services.items()]}
 for tick,(phase,down)in enumerate((('neutral',False),('neutral',False),('press',True),('held',True),('held',True),('release',False),('released',False))):
  t.tick=tick;t.w(TIME+8,tick)
  analog={button:255}if down and button<8 else {};digital=1<<(button-8)if down and button>=8 else 0
  try:
   sample=t.poll(f'button_{button}_{tick}_{phase}',analog=analog,digital=digital,milliseconds=33,process=process)
   b=bytes.fromhex(sample['generated']['action_hex'])
   rows.append({'tick':tick,'phase':phase,'supplied_hardware_packet_hex':sample['supplied_packet_hex'],
    'supplied_interval_ms':33,'raw_device_hex':sample['raw_hex'],'source_logical_hex':sample['generated']['logical_hex'],
    'source_generated_command':decode_command(b),'processed':sample['processed'],'inventory':sample['inventory']})
  except Exception as e:
   failure={'tick':tick,'phase':phase,'exception':type(e).__name__+': '+str(e),'eip':h(t.g.reg_read(UC_X86_REG_EIP)),
    'last_instructions':[{'site':h(p),'bytes':t.code.at(p).bytes.hex(),'assembly':t.code.at(p).mnemonic+' '+t.code.at(p).op_str}for p in list(t.recent)[-16:]],
    'partial_command':decode_command(bytes(t.g.mem_read(ACTION,0x5c))), 'unit_hex':bytes(t.g.mem_read(UNIT,0x800)).hex()};break
 report={'device_button_index':button,'player_phase_executed':process,'initial':initial,'rows':rows,'stop':failure,
  'original_calls':t.measured_calls,'original_ordered_stores':t.request_writes,
  'source_service_events':t.events,'source_calls_or_results_added_for_this_experiment':[],
  'scope':'Original allocated/equipped stock-SMG fixture with existing declared backing/world/audio/transport services. Original hardware packet polling/profile mapping/full request generation execute. Processed cases additionally execute bounded original14ED0C..14ED4F player phase. No full engine frame, local-session producer, melee hit, grenade creation, voice or presentation claim.'}
 t.cache.data.close();t.cache.stream.close();return report

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--output',type=Path,required=True);ap.add_argument('--buttons',type=int,nargs='*',default=list(range(16)));ap.add_argument('--process',action='store_true');a=ap.parse_args()
 assert sha(ROOT/'tools/oracle_mp_weapon_attachment_1_1.py')==ATTACHMENT_PIN
 a.output.mkdir(parents=True,exist_ok=True);cases=[]
 for n in a.buttons:
  assert 0<=n<16;r=measure_button(n,a.process);p=a.output/f'button-{n:02}.json';p.write_text(json.dumps(r,indent=2)+'\n')
  cases.append({'button':n,'file':p.name,'sha256':sha(p),'returned_rows':len(r['rows']),'stop':r['stop'],
   'trajectory':[{'phase':v['phase'],'gameplay_flags':v['source_generated_command']['gameplay_flags'],'action_flags':v['source_generated_command']['action_flags'],
    'grenade':v['source_generated_command']['requested_grenade'],'zoom':v['source_generated_command']['requested_zoom'],
    'primary':v['source_generated_command']['requested_primary']}for v in r['rows']]})
  print(json.dumps(cases[-1]),flush=True)
 report={'tool_sha256':sha(__file__),'attachment_dependency_sha256':ATTACHMENT_PIN,'original_xbe_sha256':PIN,'cache_sha256':CACHE_PIN,
  'cases':cases,'scope':'Generated retail command intent and original request admission, independent of native key labels. Hardware device samples/durations and inherited explicit equipped fixture inputs are boundaries; action success is never manufactured.','native_gameplay_update_applied':False}
 (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
if __name__=='__main__':main()

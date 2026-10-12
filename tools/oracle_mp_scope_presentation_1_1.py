#!/usr/bin/env python3
"""Independent stock SMG zoom HUD/status and genuine registered model requests.

Input/local-view and render-camera services remain the frozen clock fixture's
declared boundary. No draw/owner/visibility result is supplied. The HUD status
prefix stops before unrelated engine status; the whole mask builder consumes
that actual prefix buffer. This does not claim full HUD/graphics completion.
"""
import argparse,hashlib,json,struct
from pathlib import Path
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_ESP,UC_X86_REG_EIP
from oracle_smg_fp_clock_1_1 import ClockTrace,ROOT,CAMERA,UNIT,UI,TIME,REQUEST,MANAGER_OFFSET
from oracle_smg_firing_1_1 import STACK,STOP,_trace
from oracle_spartan_equipped_animation_1_1 import CONTROL_INPUT
from inspect_multiplayer_hud import HudCache

STATUS=CAMERA+0x3f0000
OUTPUT=STATUS+0x1000
MODELS=CAMERA+0x300000
WATCH=(0x13b09a,0x13b7c0,0xc9bc0,0xcd1a0,0x105120,0xcf440,
 0x167a0d,0x167996,0x3d2b0,0x169ddc,0x156eb0,0x11cf00)
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def h(v):return f'{v&0xffffffff:08x}'

class PresentationTrace(ClockTrace):
 def __init__(self):
  self.presentation_ready=False;self.presentation_calls=[];self.presentation_stores=[]
  super().__init__();self.presentation_ready=True
 def presentation_state(self):
  p=self.u(0x4f1198)
  return dict(unit_current=struct.unpack('<b',self.g.mem_read(UNIT+0x240,1))[0],
   unit_requested=struct.unpack('<b',self.g.mem_read(UNIT+0x241,1))[0],
   unit_motion_bits=h(self.u(UNIT+0x26c)),unit_800_hex=bytes(self.g.mem_read(UNIT,0x800)).hex(),
   fps_view_20cc_hex=bytes(self.g.mem_read(p,0x20cc)).hex(),
   hands=[dict(flags=h(self.u(p+0xc+n*0x1010)),weapon=h(self.u(p+0x10+n*0x1010)))for n in range(2)],
   source_hud_status_270_hex=bytes(self.g.mem_read(STATUS,0x270)).hex(),
   render_camera_globals_74_hex=bytes(self.g.mem_read(0x4c12c0,0x74)).hex())
 def hook(self,g,at,size,user):
  if self.presentation_ready and at in WATCH:
   sp=g.reg_read(UC_X86_REG_ESP)
   self.presentation_calls.append(dict(phase=self.active,entry=h(at),return_site=h(self.u(sp)),
    registers=self.registers(),arguments=[h(self.u(sp+4+n*4))for n in range(7)]))
  super().hook(g,at,size,user)
 def mem_write(self,g,access,at,size,value,user):
  if self.presentation_ready and any(p<=at and at+size<=p+n for p,n in [(STATUS,0x270),(OUTPUT,0x100),(MODELS,0x3430)]):
   self.presentation_stores.append(dict(phase=self.active,instruction=h(g.reg_read(UC_X86_REG_EIP)),address=h(at),size=size,
    before_hex=bytes(g.mem_read(at,size)).hex(),after_hex=(value&((1<<(size*8))-1)).to_bytes(size,'little').hex()))
  super().mem_write(g,access,at,size,value,user)
 def status_prefix(self):
  self.active=f'frame_{self.tick}_original_hud_status_prefix'
  self.count=0;self.w(STACK,STOP);self.w(STACK+4,0)
  self.g.reg_write(UC_X86_REG_ESP,STACK);self.g.reg_write(UC_X86_REG_EAX,STATUS)
  self.g.emu_start(0x13b09a,0x13b228,count=1000000)
  assert self.g.reg_read(UC_X86_REG_EIP)==0x13b228,('source_hud_prefix_stop',h(self.g.reg_read(UC_X86_REG_EIP)))
  return dict(start='0013b09a',end_exclusive='0013b228',registers=self.registers(),
   current_status=struct.unpack('<h',self.g.mem_read(STATUS+0x3c,2))[0],binocular_status=self.u(STATUS+0x3e,1),
   complete_prefix_status_hex=bytes(self.g.mem_read(STATUS,0x270)).hex())

def main():
 p=argparse.ArgumentParser();p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 dependencies=['tools/oracle_smg_fp_clock_1_1.py','tools/oracle_smg_fp_registration_1_1.py',
  'tools/oracle_smg_constructor_equipped_1_1.py','analysis/multiplayer/integration/scope-camera/manifest.json']
 t=PresentationTrace();t.services[0xb8a30]=('unchanged bounded failed projectile allocation after actual source loadout',-1,4)
 t.services[0x1e20c0]=('unchanged bounded clear muzzle segment',0,16)
 initial=t.presentation_state();frames=[]
 for tick,selector in enumerate((-1,0,0,0,-1,-1)):
  t.tick=tick;t.w(TIME+8,tick);t.active=f'frame_{tick}_declared_input'
  t.run(0x14e780,regs=((UC_X86_REG_EDX,CONTROL_INPUT),))
  t.g.mem_write(CONTROL_INPUT+0x1e,bytes(t.g.mem_read(UNIT+0x214,4)));t.sh(CONTROL_INPUT+0x24,selector)
  incoming=bytes(t.g.mem_read(CONTROL_INPUT,0x5c)).hex();before=t.presentation_state()
  call_start=len(t.presentation_calls);write_start=len(t.presentation_stores)
  for name,entry,args,regs in [('pressed_edges',0x150dd0,(),((UC_X86_REG_EAX,0),(UC_X86_REG_ECX,CONTROL_INPUT))),
   ('player_actions',0x150f00,(0,UI,CONTROL_INPUT),()),('unit_input',0x151c90,(UI,CONTROL_INPUT),((UC_X86_REG_EAX,0),)),
   ('unit_blend',0x1d0ed0,(),((UC_X86_REG_ESI,UNIT+MANAGER_OFFSET),)),('unit',0xc7e90,(UI,),((UC_X86_REG_EAX,0xc7e90),)),
   ('weapon',0x1021d0,(t.initial_datum,),()),('actions',0xe7e60,(UI,),()),('animation',0x112e10,(UI,REQUEST),()),
   ('pose',0xbdf80,(UI,),()),('postpass',0x1019f0,(),()),('first_person',0x1678c1,(),())]:
   t.active=f'frame_{tick}_{name}';t.pose_active=name=='pose';t.run(entry,args,regs);t.pose_active=False
  after_clock=t.presentation_state();status=t.status_prefix()
  t.active=f'frame_{tick}_original_masks_from_partial_status'
  t.run(0x13b7c0,(0,1,OUTPUT,OUTPUT+4,OUTPUT+6,OUTPUT+8,OUTPUT+10),((UC_X86_REG_ESI,STATUS),))
  masks=list(struct.unpack('<4H',t.g.mem_read(OUTPUT+4,8)))
  t.active=f'frame_{tick}_original_model_request_owner'
  t.run(0x167a0d,(0,UI,4,MODELS));count=t.g.reg_read(UC_X86_REG_EAX)
  assert 0<=count<=4
  frames.append(dict(tick=tick,native_incoming_selector=selector,incoming5c_hex=incoming,before=before,after_original_fp_clock=after_clock,
   status_prefix=status,masks=masks,model_count=count,original_models_hex=bytes(t.g.mem_read(MODELS,count*0xd0c)).hex(),
   after_original_model_owner=t.presentation_state(),call_range=[call_start,len(t.presentation_calls)],store_range=[write_start,len(t.presentation_stores)]))
 cache=HudCache(ROOT/'assets/disc/maps/lockout.map');hud=[]
 for tag in cache.tags.values():
  if tag['group']=='nhdt'and tag['name']in ['ui\\hud\\masterchief','ui\\hud\\smg']:hud.append(cache.hud(tag))
 a.output.mkdir(parents=True,exist_ok=True);(a.output/'instructions').mkdir(exist_ok=True)
 for entry in WATCH:
  f=_trace(t.code,entry,t.starts|set(WATCH));ins=[t.code.at(q)for q in sorted(f.code)]
  (a.output/'instructions'/f'{entry:08x}.asm').write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins))
 report=dict(schema=1,tool_sha256=sha(__file__),dependencies={name:sha(ROOT/name)for name in dependencies},
  original_xbe_sha256=sha(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'),map_sha256=sha(ROOT/'assets/disc/maps/lockout.map'),
  initial=initial,frames=frames,calls=t.presentation_calls,ordered_stores=t.presentation_stores,authored_hud_definitions=hud,
  inherited_services=[dict(entry=h(entry),contract=value)for entry,value in t.services.items()],
  boundaries=['Actual original registered FP allocation/profile conversion/SMG clock setup are inherited unchanged. The preallocated player/local-view and raw heap/resource/camera backing remain explicit; no genuine local session producer is substituted.',
   'Native incoming selector/tick and neutral buttons are supplied inputs. Harness unit/weapon/action phases are composed in the prior clock order, not claimed as ordinary attached-world dispatcher ordering.',
   'Source HUD prefix returns to its declared stop before motion-sensor/shield/engine status. The full mask builder reads that actual partly constructed buffer; only captured zoom/binocular fields establish the zoom visibility subset.',
   'Original model request constructor executes against source-created registered hand/model/node mappings. Recorded render-camera global input bytes are inherited initial state, not a GPU/render-frame producer. No Direct3D draw, active flag, model-count result or camera completion is supplied.',
   'Xbox hardware RCPSS/matrix bits and fully rasterized binocular overlay coverage remain unverified; a positive model request is not a visible GPU pixel claim.'])
 (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps(dict(report_sha256=sha(a.output/'report.json'),frames=[dict(selector=f['native_incoming_selector'],current=f['status_prefix']['current_status'],binocular=f['status_prefix']['binocular_status'],masks=f['masks'],models=f['model_count'])for f in frames])),flush=True)

if __name__=='__main__':main()

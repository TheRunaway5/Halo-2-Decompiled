#!/usr/bin/env python3
"""Execute original multiplayer death camera with its saved-cluster lifecycle.

Raw arena backing, local view/player ownership release, RNG and frame-clock inputs
are explicit native adapter boundaries. Camera/cluster/query/observer instructions
execute unchanged. This is not a fatal-hit or natural corpse-physics scheduler.
"""
import argparse,hashlib,json,struct
from pathlib import Path
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP
from oracle_mp_death_camera import DeathCameraTrace
from oracle_mp_scope_camera_1_1 import INITIALIZERS,OBSERVER,PACKET,COMMAND,PERSISTENT
from oracle_smg_firing_1_1 import ROOT,PIN,CACHE_PIN,PLAYER_DATA,USERS,UNIT,UI,SEED,TIME,STOP
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI

def h(v):return f'{v&0xffffffff:08x}'
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def words(raw):return ' '.join(h(v)for v in struct.unpack('<'+'I'*(len(raw)//4),raw))
class Trace(DeathCameraTrace):
 def __init__(self):
  self.casts=[];self.cast_pending=[];self.trace_casts=False
  self.probes=[];self.probe_pending=[];self.locations=[];self.location_pending=[];self.visibility=[]
  super().__init__()
 def hook(self,g,a,size,user):
  sp=g.reg_read(UC_X86_REG_ESP)
  if self.trace_casts:
   for ret,end,row in self.location_pending[:]:
    if a==ret and sp==end:
     row['result']=bytes(g.mem_read(row.pop('output'),8)).hex()
     self.location_pending.remove((ret,end,row))
   if a==0x11d4a0 and self.u(sp)==0x1731d7:
    row=dict(position=bytes(g.mem_read(g.reg_read(UC_X86_REG_EDX),12)).hex(),output=g.reg_read(UC_X86_REG_ESI))
    self.locations.append(row);self.location_pending.append((self.u(sp),sp+4,row))
   if a==0x3f4f0:self.visibility.append(g.reg_read(UC_X86_REG_EAX))
   for ret,end,row in self.probe_pending[:]:
    if a==ret and sp==end:
     row['result']=h(self.u(row['distance_pointer']))
     row['casts']=self.casts[row.pop('first_cast'):]
     self.probe_pending.remove((ret,end,row))
   if a==0x1732b0:
    row=dict(up=bytes(g.mem_read(g.reg_read(UC_X86_REG_EAX),12)).hex(),
     forward=bytes(g.mem_read(g.reg_read(UC_X86_REG_ECX),12)).hex(),
     target=bytes(g.mem_read(self.u(sp+4),12)).hex(),distance_pointer=self.u(sp+8),
     distance=h(self.u(self.u(sp+8))),margin=h(self.u(sp+12)),first_cast=len(self.casts))
    self.probes.append(row);self.probe_pending.append((self.u(sp),sp+16,row))
   if a==0x173369:
    self.probe_pending[-1][2]['fog_side']=bool(self.u(sp+0x13,1))
   for ret,end,row in self.cast_pending[:]:
    if a==ret and sp==end:
     row['hit']=bool(g.reg_read(UC_X86_REG_EAX)&255)
     row['fraction']=h(self.u(row['output']+4))
     self.cast_pending.remove((ret,end,row))
   if a==0x16b240:
    row=dict(flags=h(self.u(sp+4)),origin=bytes(g.mem_read(self.u(sp+8),12)).hex(),
      delta=bytes(g.mem_read(self.u(sp+12),12)).hex(),output=self.u(sp+24))
    self.casts.append(row);self.cast_pending.append((self.u(sp),sp+28,row))
  super().hook(g,a,size,user)

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True)
 ap.add_argument('--fixture',type=Path,required=True);args=ap.parse_args();t=Trace();stages=[];rows=[];lines=[]
 # This fixture's raw host arena is disjoint from object/cache/stack storage.
 # Original170A30 owns allocation offset, CRC and the actual camera-bank pointer.
 t.g.mem_map(0x7500000,0x400000);t.w(0x4ed650,0x7500000)
 for entry in (*INITIALIZERS,0x37d540,0x37d560):stages.append(t.measure('initializer_'+h(entry),entry))
 stages.append(t.measure('source_camera_cluster_allocation',0x170a30))
 stages.append(t.measure('source_camera_map_reset',0x170b60))
 assert t.u(0x518460)==0x7500000 and bytes(t.g.mem_read(0x7500000,20))==b'\xff'*20
 t.w(PLAYER_DATA+0x2c,-1);t.w(PLAYER_DATA+0x30,UI);t.w(USERS+12,0)
 t.w(COMMAND,0);t.f(COMMAND+8,1/30)
 constants=[0x465ad0,0x4650c0,0x465080,0x46507c,0x4457bc,0x465640,0x46563c,
  0x465044,0x465068,0x4656a4,0x465030,0x46502c,0x45ac88,0x4650d8,0x46504c,0x44aff4,0x44aff8,0x44aff8]
 lines.append('K '+' '.join(h(t.u(p))for p in constants))
 scope=[0x465ad0,0x4650c4,0x465540,0x4650c4,0x465540,0x46570c,0x465708,
  0x465030,0x47885c,0x4653b8,0x4701c8,0x46502c,0x465044,0x465060,
  0x465068,0x46506c,0x465070,0x465078,0x46507c,0x465080,0x465084,0x465218,0x44b020]
 lines.append('G '+' '.join(h(t.u(p))for p in scope)+' '+bytes(t.g.mem_read(0x465130,8))[::-1].hex()+
  ' '+bytes(t.g.mem_read(0x4701d0,12)).hex()+' '+bytes(t.g.mem_read(0x4701dc,12)).hex()+
  ' '+bytes(t.g.mem_read(0x44b010,24)).hex()+' '+
  ' '.join(h(t.u(p))for p in (0x46504c,0x4650e0,0x465090,0x54f214,0x54f218,0x465088)))
 lines.append('H '+' '.join(h(t.u(p))for p in (0x452044,0x5563b0,0x465474))+' 3ca3d70a '+
  ' '.join(h(t.u(p))for p in (0x46508c,0x44b004))+' '+bytes(t.g.mem_read(0x465968,8))[::-1].hex()+' '+
  ' '.join(bytes(t.g.mem_read(t.u(p),12)).hex()for p in (0x46fc48,0x46fc50,0x46fc44)))
 for seed in (0x12345678,0,0xffffffff,0x78a8):
  t.measure('source_observer_reset',0x170f30,regs=((UC_X86_REG_ESI,OBSERVER),))
  t.g.mem_write(PERSISTENT,bytes(64));t.w(SEED+4,seed)
  position=bytes(t.g.mem_read(OBSERVER+0xb8,12));before=bytes(t.g.mem_read(PERSISTENT,64))
  t.measure('source_death_camera_create',0x23fb00,(-1,),((UC_X86_REG_EAX,PERSISTENT),(UC_X86_REG_EDX,0)))
  lines.append('C '+h(seed)+' '+position.hex()+' '+before.hex()+' '+bytes(t.g.mem_read(PERSISTENT,64)).hex()+' '+h(t.u(SEED+4)))
  for frame in range(100):
   t.g.mem_write(PACKET,bytes(0xac));before=bytes(t.g.mem_read(PERSISTENT,64))
   pos=bytes(t.g.mem_read(OBSERVER+0xb8,12));up=bytes(t.g.mem_read(OBSERVER+0xe4,12))
   node=bytes(t.g.mem_read(UNIT+t.u(UNIT+0x116,2)+0x28,12))
   t.measure('source_death_camera_frame',0x23ff50,(PERSISTENT,COMMAND,PACKET))
   after=bytes(t.g.mem_read(PERSISTENT,64));packet=bytes(t.g.mem_read(PACKET,0xac))
   lines.append('F '+h(seed)+' '+h(frame)+' '+h(t.u(COMMAND+8))+' '+str(t.u(t.u(0x518444)+2,2))+' '+pos.hex()+' '+up.hex()+' '+node.hex()+' '+before.hex()+' '+after.hex()+' '+packet.hex())
   t.measure('source_camera_saved_clusters',0x170a70,(0,))
   observer_before=bytes(t.g.mem_read(OBSERVER,0x358)).hex()
   t.measure('source_camera_register',0x170c10,regs=((UC_X86_REG_EAX,PACKET),(UC_X86_REG_ECX,0)))
   t.measure('source_camera_adopt',0x170ff0,(0,))
   observer_adopted=bytes(t.g.mem_read(OBSERVER,0x358)).hex()
   t.w(0x21fd004,t.u(COMMAND+8));t.g.reg_write(UC_X86_REG_ESP,0x21fd000)
   t.g.emu_start(0x170d00,0x170d1c,count=20)
   t.measure('source_camera_motion',0x171910,regs=((UC_X86_REG_EDI,0),))
   lines.append('O '+observer_before+' '+packet.hex()+' '+observer_adopted+' '+h(t.u(0x4f11a0))+' '+bytes(t.g.mem_read(OBSERVER,0x358)).hex())
   n=len(t.casts);l=len(t.locations);v=len(t.visibility);before_world=bytes(t.g.mem_read(OBSERVER,0x358)).hex();t.trace_casts=True
   t.measure('source_camera_world',0x172a50,(0,));t.trace_casts=False
   lines.append('W '+before_world+' '+bytes(t.g.mem_read(OBSERVER,0x358)).hex()+' '+
    str(t.u(0x46fb64,2))+' '+str(t.u(t.u(0x4e7918)+0x9c))+' '+h(t.u(PLAYER_DATA+0x2c))+' '+str(t.u(PLAYER_DATA+2,2))+' '+
    bytes(t.g.mem_read(t.u(0x518460),20)).hex()+' '+str(len(t.casts)-n)+' '+str(len(t.locations)-l)+' '+str(len(t.visibility)-v)+' '+
    str(int(t.probes[-1]['fog_side'])) if len(t.casts)>n else 'W_NO_PROBE')
   for q in t.casts[n:]:lines.append('Q '+' '.join(str(q[k])for k in ('flags','origin','delta'))+' '+str(int(q['hit']))+' '+q['fraction'])
   for q in t.locations[l:]:lines.append('Y '+q['position']+' '+q['result'])
   for cluster in t.visibility[v:]:lines.append('V '+str(cluster))
   rows.append(dict(seed=h(seed),frame=frame,controller_hex=after.hex(),packet_hex=packet.hex(),
    render_hex=bytes(t.g.mem_read(OBSERVER+0xb8,0x50)).hex(),casts=t.casts[n:],
    saved_clusters_hex=bytes(t.g.mem_read(t.u(0x518460),20)).hex()))
 for row in t.probes:
  assert 'result' in row and 'fog_side' in row
  lines.append('D '+' '.join(str(row[k])for k in ('forward','up','target','distance','margin'))+
   ' '+str(int(row['fog_side']))+' '+row['result']+' '+str(len(row['casts'])))
  for q in row['casts']:
   lines.append('Q '+' '.join(str(q[k])for k in ('flags','origin','delta'))+' '+str(int(q['hit']))+' '+q['fraction'])
 args.output.parent.mkdir(parents=True,exist_ok=True);args.fixture.parent.mkdir(parents=True,exist_ok=True)
 args.fixture.write_text('\n'.join(lines)+'\n')
 report=dict(tool_sha256=sha(__file__),source_sha256=PIN,cache_sha256=CACHE_PIN,
  stages=stages,frames=rows,probes=t.probes,constants={h(p):h(t.u(p))for p in constants},
  supplied_boundaries=__doc__,instruction_patches=[],original_query_replacements=[],fixture_sha256=sha(args.fixture))
 args.output.write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps(dict(frames=len(rows),casts=len(t.casts),report=str(args.output))))
if __name__=='__main__':main()

#!/usr/bin/env python3
"""Unchanged retail hill CPU emission corridor with an explicit composed frame.

Whole D4BC0 fills the real Lockout record; unchanged19F680 prefix produces its
distance locals, and unchanged19F75F..19F844 walks the actual captured shader
metadata to select UV1. A declared post-preparation frame resumes19F8AD and
stops before19FC12 end/restore. Bitmap preparation, dirty flush and missing
services are not executed or supplied successful returns. Genuine captured
device/cursor/capacity remain inputs. Raw CPU packets do not prove GPU output.
"""
import argparse,json,struct
from pathlib import Path
from oracle_camouflage_pass_services import *
from oracle_camouflage_scene_copy import decode_packets

OUT=ROOT/'analysis/multiplayer/game-variants/king-hill-emission-source-20261012'
VISUAL=ROOT/'analysis/multiplayer/game-variants/king-visual-consumers-source-20261012/complete/report.json'
DRAW=ROOT/'analysis/multiplayer/game-variants/king-draw-snapshot-source-20261012/diagnostic-0/report.json'
POINTS=SYNTH+0x40000;CENTER=POINTS+0x164;COLOR=POINTS+0x200;RECORD=POINTS+0x300
GPRS=(('eax',UC_X86_REG_EAX),('ebx',UC_X86_REG_EBX),('ecx',UC_X86_REG_ECX),('edx',UC_X86_REG_EDX),
 ('esi',UC_X86_REG_ESI),('edi',UC_X86_REG_EDI),('ebp',UC_X86_REG_EBP),('esp',UC_X86_REG_ESP))

class EmissionTrace(Trace):
 def __init__(self,*args):
  super().__init__(*args);self.phase=None;self.fistps=[];self.pairs=[];self.declarations=[];self.phase_instructions={};self.metadata_backing=[]
 def state(self):
  return dict(registers={k:f'{self.g.reg_read(v):08x}'for k,v in GPRS},
   fpcw=f'{self.g.reg_read(UC_X86_REG_FPCW):04x}',mxcsr=f'{self.g.reg_read(UC_X86_REG_MXCSR):08x}',
   fpsw=f'{self.g.reg_read(UC_X86_REG_FPSW):04x}',fptag=f'{self.g.reg_read(UC_X86_REG_FPTAG):04x}')
 def declare(self,address,raw,reason):
  self.declarations.append(dict(address=f'{address:08x}',before_hex=self.read(address,len(raw)).hex(),after_hex=raw.hex(),reason=reason));self.put(address,raw)
 def code(self,g,pc,size,unused):
  super().code(g,pc,size,unused)
  if self.boundary:return
  self.phase_instructions.setdefault(self.phase,set()).add(pc)
  stops={'prefix':0x19f73c,'metadata':0x19f844,'emission':0x19fc12}
  if pc==stops.get(self.phase):
   self.boundary=dict(kind='Explicit corridor stop before unchanged instruction',pc=pc,phase=self.phase,state=self.state());g.emu_stop();return
  if self.phase=='metadata'and pc==0x19f803:
   p=g.reg_read(UC_X86_REG_ECX);self.metadata_backing.append(dict(pc=f'{pc:08x}',implementation=f'{p:08x}',implementation132_hex=self.read(p,0x132).hex(),vertex_datum=f'{self.u(p+0x100):08x}'))
  if self.phase=='metadata'and pc==0x19f819:
   p=g.reg_read(UC_X86_REG_ECX);count=self.u(p+4);semantic=self.u(p+8);size=self.u(p+0xc);code=self.u(p+0x10)
   self.metadata_backing.append(dict(pc=f'{pc:08x}',classification=f'{p:08x}',classification1c_hex=self.read(p,0x1c).hex(),semantic_pointer=f'{semantic:08x}',semantic_ids=list(struct.unpack('<'+'h'*count,self.read(semantic,count*2))),
    vertex_code_pointer=f'{code:08x}',vertex_code_hex=self.read(code,size).hex()))
  if self.phase!='emission':return
  sp=g.reg_read(UC_X86_REG_ESP)
  if pc in(0x19f8f0,0x19f906):
   self.fistps.append(dict(pc=f'{pc:08x}',input_binary32_hex=self.read(sp+0x58,4).hex(),before=self.state()))
  if pc in(0x19f8f4,0x19f90a):
   self.fistps[-1].update(after=self.state(),output_hex=self.read(sp+0x48,4).hex(),output_signed=struct.unpack('<i',self.read(sp+0x48,4))[0])
  if pc==0x19fa82:
   self.pairs.append(dict(loop_j=g.reg_read(UC_X86_REG_EBP),point_i=g.reg_read(UC_X86_REG_EDI),
    rgba_bits=words(self.read(sp+0x14,12)+self.read(sp+0x58,4)),uv_u_bits=self.u(sp+0x48),
    lower_position_bits=words(self.read(sp+0x28,12)),upper_position_bits=words(self.read(sp+0x34,12)),state=self.state()))
 def execute(self,phase,entry,stop=STOP):
  self.phase=phase;self.boundary=None;before=self.state()
  try:self.g.emu_start(entry,stop,count=200000)
  except UcError:
   if not self.boundary:raise
  self.finish_writes();returned=not self.boundary and self.g.reg_read(UC_X86_REG_EIP)==STOP
  assert returned or self.boundary
  return dict(entry=f'{entry:08x}',complete_original_return=returned,stop=self.boundary,before=before,after=self.state())
 def call(self,phase,entry,args,registers,cw,mxcsr):
  self.put(STACK,packed([STOP,*args]));self.g.reg_write(UC_X86_REG_ESP,STACK)
  for reg,value in((UC_X86_REG_EFLAGS,2),(UC_X86_REG_FPCW,cw),(UC_X86_REG_MXCSR,mxcsr),(UC_X86_REG_FPTAG,0xffff),(UC_X86_REG_FPSW,0),*registers):self.g.reg_write(reg,value)
  return self.execute(phase,entry)

def inventory(geometry,captured_tick,colors):
 cx,cy,cz=struct.unpack_from('<3f',geometry,0x164);radius=struct.unpack_from('<f',geometry,0x170)[0]
 cameras=[('center',0.,0.),('edge',1.,0.),('outer_fade',2.,0.),('diagonal',1.4,.8)]
 result=[]
 for name,x,y in cameras:
  for tick in(captured_tick,0,59,60):result.append(dict(camera=name,camera_bits=[fw(cx+x*radius),fw(cy+y*radius),fw(cz)],tick=tick,requested_fpcw=0x023f,requested_mxcsr=0x1fa0,rgb_name='view_first',rgb_hex=colors['view_first']))
 for cw in(0x063f,0x0a3f,0x0e3f,0x027f):result.append({**result[0],'requested_fpcw':cw})
 for key in('view_second','neutral'):result.append({**result[0],'rgb_name':key,'rgb_hex':colors[key]})
 return result

def one(snapshot,image,rows,geometry,shader,case):
 t=EmissionTrace(snapshot,image,rows);device=t.u(0x407488);begin=t.u(device);limit=t.u(device+4);clock=t.u(0x510c54)
 initial=dict(device=f'{device:08x}',device_header_hex=t.read(device,0x40).hex(),command_begin=f'{begin:08x}',command_limit=f'{limit:08x}',
  available_bytes=limit-begin,camera_hex=t.read(0x4b9da0,12).hex(),clock_pointer=f'{clock:08x}',clock30_hex=t.read(clock,0x30).hex(),
  source_buffer_before_hex=t.read(begin,2780).hex())
 t.declare(POINTS,geometry,'Actual whole original initialized/built Lockout King state, relocated caller input')
 t.declare(COLOR,bytes.fromhex(case['rgb_hex']),'Measured original hill-color literal, explicit caller RGB input')
 t.declare(RECORD,b'\xa5'*24,'Declared caller output storage for real complete D4BC0')
 t.declare(0x4b9da0,packed(case['camera_bits']),'Declared finite camera XYZ; only XY is consumed by this helper')
 t.declare(clock+8,packed([case['tick']]),'Declared tick, retaining actual captured signed16 clock rate and all other clock bytes')
 cw,mxcsr=case['requested_fpcw'],case['requested_mxcsr']
 fill=t.call('fill',0xd4bc0,[shader,0,0,15,0,0xffffffff,RECORD],(),cw,mxcsr)
 assert fill['complete_original_return']and t.g.reg_read(UC_X86_REG_EAX)&255
 fill.update(record24_hex=t.read(RECORD,24).hex(),defined_al=t.g.reg_read(UC_X86_REG_EAX)&255)
 assert t.u(device)==begin
 arguments=[15,RECORD,POINTS+0x64,32,CENTER,uint(geometry,0x170),uint(geometry,0x17c),COLOR,fw(.8)]
 prefix=t.call('prefix',0x19f680,arguments,((UC_X86_REG_EAX,0),(UC_X86_REG_ECX,0),(UC_X86_REG_EDX,shader)),cw,mxcsr)
 assert prefix['stop']and prefix['stop']['pc']==0x19f73c
 sp=t.g.reg_read(UC_X86_REG_ESP);assert sp==STACK-0x40
 prefix['frame68_hex']=t.read(sp,0x68).hex()
 # Explicit discontinuity: no original4B2D0 result or resource readiness is
 # supplied. The callee-saved shader/LOD/layer/impl variables already exist
 # in the observed prefix; this range performs its real metadata traversal.
 metadata=t.execute('metadata',0x19f75f)
 assert metadata['stop']and metadata['stop']['pc']==0x19f844
 metadata.update(frame68_hex=t.read(sp,0x68).hex(),uv_register=t.u(sp+0x44),backing=t.metadata_backing)
 assert metadata['uv_register']==1 and t.u(device)==begin
 # These are literal loads by19F88C/19F897 in the omitted preparation tail.
 # The captured device/header are unchanged, including its actual capacity.
 before_registers=t.state();t.g.reg_write(UC_X86_REG_ESI,device);t.g.reg_write(UC_X86_REG_EAX,begin)
 composed=dict(register_assignments=[dict(register='esi',value=f'{device:08x}',source='captured[407488], original19F88C'),dict(register='eax',value=f'{begin:08x}',source='captured[device], original19F897')],
  before_assignments=before_registers,frame68_hex=t.read(sp,0x68).hex(),state=t.state(),
  omitted_ranges=['19F73C..19F75E graphics/material preparation','19F844..19F8AC declaration/cull/program/texture/dirty-flush/capacity preparation'],
  entry='0019f8ad',stop_before='0019fc12',whole_helper_claim=False,preparation_success_claim=False)
 emission=t.execute('emission',0x19f8ad)
 assert emission['stop']and emission['stop']['pc']==0x19fc12
 end=t.u(device);assert begin<=end<=limit and end-begin==2780
 raw=t.read(begin,end-begin);packets=decode_packets(raw);assert len(packets)==166
 assert packets[0]['method']==0x17fc and packets[0]['values']==[6]
 vertices=[]
 for i in range(33):
  color,uv0,pos0,uv1,pos1=packets[1+i*5:6+i*5]
  assert [p['method']for p in(color,uv0,pos0,uv1,pos1)]==[0x1ab0,0x1888,0x1a00,0x1888,0x1a00]
  assert t.pairs[i]['rgba_bits']==color['values']
  for uv,pos in((uv0,pos0),(uv1,pos1)):vertices.append(dict(rgba11_bits=color['values'],uv1_bits=uv['values'],position0_bits=pos['values']))
 assert len(t.fistps)==2 and len(t.pairs)==33 and len(vertices)==66
 assert vertices[0]['position0_bits']==vertices[64]['position0_bits']and vertices[1]['position0_bits']==vertices[65]['position0_bits']
 assert all(begin<=w['address']and w['address']+w['size']<=end or device<=w['address']and w['address']+w['size']<=device+12 for w in t.writes)
 return dict(input=case,initial=initial,declarations=t.declarations,fill=fill,arguments=arguments,prefix=prefix,metadata=metadata,declared_post_preparation=composed,
  emission=emission,final_frame68_hex=t.read(sp,0x68).hex(),clock30_after_hex=t.read(clock,0x30).hex(),device_header_after_hex=t.read(device,0x40).hex(),
  command_end=f'{end:08x}',command_bytes=len(raw),command_hex=raw.hex(),command_sha256=sha(raw),packets=packets,vertices=vertices,pairs=t.pairs,fistps=t.fistps,
  source_writes=t.writes,verified_original_instructions=[dict(address=f'{pc:08x}',bytes=image.read(pc,n).hex())for pc,n in sorted(t.executed.items())],
  phase_instruction_addresses={k:[f'{pc:08x}'for pc in sorted(v)]for k,v in t.phase_instructions.items()},
  mapped_snapshot_pages={f'{p:08x}':sha(v)for p,v in sorted(t.pages.items())})

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'complete/report.json');a=ap.parse_args();a.output.parent.mkdir(parents=True,exist_ok=True)
 snapshot=Snapshot(SNAPSHOT);assert sha(snapshot.ram)==RAM_PIN;image=Xbe(ROOT/'assets/disc/default.xbe');assert sha(image.data)==PIN
 visual=json.loads(VISUAL.read_text());draw=json.loads(DRAW.read_text());geometry=bytes.fromhex(visual['geometry_hex'])
 center_case=next(c for c in draw['cases']if c['mode']=='declared_center');assert center_case['draw']['stop']['pc']==0x12ce00
 shader=int(draw['shader'],16);assert shader==0xf2c81143
 clock=uint(snapshot.read(0x510c54,4));tick=uint(snapshot.read(clock+8,4));assert struct.unpack('<h',snapshot.read(clock+2,2))[0]==30
 literals=visual['results']['retail']['constants'];colors=dict(view_first=literals['00468c84']['hex'],view_second=literals['00468c94']['hex'],neutral=literals['00468714']['pointed_rgb_hex'])
 inputs=inventory(geometry,tick,colors);results=[one(snapshot,image,read_rows(FUNCTIONS_CSV),geometry,shader,c)for c in inputs]
 instructions={int(x['address'],16):bytes.fromhex(x['bytes'])for c in results for x in c['verified_original_instructions']}
 path=a.output.parent/'observed-original.asm';path.write_text('\n'.join(line for pc,raw in sorted(instructions.items())for line in listing(raw,pc,{}))+'\n')
 body=image.read(0x19f680,0x19fc6e-0x19f680);path2=a.output.parent/'whole-19f680.asm';path2.write_text('\n'.join(listing(body,0x19f680,read_rows(FUNCTIONS_CSV)))+'\n')
 kept=a.output.parent/'preserved-whole-center.json';kept.write_bytes(DRAW.read_bytes())
 dependencies=['tools/research_king_hill_emission.py','tools/oracle_camouflage_pass_services.py','tools/oracle_camouflage_scene_copy.py','tools/differential_source_object_admission_snapshot.py','tools/research_king_draw_snapshot.py','tools/oracle_mp_spawn.py','tools/reference/halo2-decompiled/tools/xbe.py','tools/reference/halo2-decompiled/tools/disasm.py','tools/reference/halo2-decompiled/tools/inventory.py',str(Path(FUNCTIONS_CSV).relative_to(ROOT))]
 report=dict(schema='king-hill-emission-source-v1',scope=__doc__,tool_sha256=sha(Path(__file__).read_bytes()),snapshot=str(SNAPSHOT.relative_to(ROOT)),snapshot_ram_sha256=RAM_PIN,
  snapshot_manifest_sha256=sha((SNAPSHOT/'export.json').read_bytes()),retail_xbe_sha256=PIN,visual_report_sha256=sha(VISUAL.read_bytes()),preserved_whole_draw_report_sha256=sha(DRAW.read_bytes()),
  dependencies={p:sha((ROOT/p).read_bytes())for p in dependencies},geometry_hex=geometry.hex(),shader=f'{shader:08x}',cases=results,
  summary=dict(cases=len(results),whole_record_fills=len(results),prefixes=len(results),metadata_corridors=len(results),completed_emission_corridors=len(results),whole_positive_draw_calls=0,
   vertices=sum(len(c['vertices'])for c in results),pairs=sum(len(c['pairs'])for c in results),actual_fistps=sum(len(c['fistps'])for c in results),
   command_bytes=sum(c['command_bytes']for c in results),distinct_command_streams=len({c['command_sha256']for c in results})),
  listings={p.name:sha(p.read_bytes())for p in(path,path2)},instruction_patches=[],supplied_returns=[],graphics_readiness_changes=[])
 a.output.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n');print(json.dumps(report['summary'],indent=2))

if __name__=='__main__':main()

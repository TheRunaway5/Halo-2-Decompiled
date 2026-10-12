#!/usr/bin/env python3
"""Finite unchanged hill preparation over captured renderer backing.

Whole original record fill and helper prefix/metadata establish the caller
frame. The omitted authored pass still has its genuine bitmap-request stop.
One explicit composition additionally executes the actual whole1C590 hill
program-selection owner that4B2D0 would call after that pass. No pass success,
bitmap residency, I/O result, SDK result or instruction is supplied. The
unchanged19F844..19F8AD corridor then runs through real captured services;
stopping before its first hill primitive does not establish GPU execution.
"""
import argparse,json,struct,sys
from pathlib import Path
from research_king_hill_emission import (EmissionTrace,Snapshot,SNAPSHOT,RAM_PIN,
 ROOT,PIN,Xbe,read_rows,FUNCTIONS_CSV,sha,packed,uint,words,fw,SYNTH,STACK,STOP,
 POINTS,CENTER,COLOR,RECORD,UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,
 UC_X86_REG_ESI,UC_X86_REG_ESP)
from oracle_camouflage_scene_copy import decode_packets
from oracle_king_engine import KingTrace
from oracle_mp_full_scoreboard_1_5 import literal,complete_tokens,references,_Code
from research_mp_lobby_later_update import table_pair,alignment

OUT=ROOT/'analysis/multiplayer/game-variants/king-hill-preparation-source-20261012'
VISUAL=ROOT/'analysis/multiplayer/game-variants/king-visual-consumers-source-20261012/complete/report.json'
DRAW=ROOT/'analysis/multiplayer/game-variants/king-draw-snapshot-source-20261012/diagnostic-0/report.json'
OWNERS={0x19f680:'hill_helper',0x1bd50:'owner_context',0x1cdd0:'range_constants',
 0x4b2d0:'material_and_program_caller',0x1c6b0:'vertex_reset',0x1c710:'vertex_prepare',
 0x1c330:'declaration_build',0x35790:'format_enum',0x35850:'format_bytes',
 0x1c590:'program_load',0x1c2e0:'program_lookup',0x3f6c60:'sdk_program_load',
 0x3f6b70:'sdk_program_packets',0x3f7440:'sdk_stream_inputs',
 0x3f6cf0:'sdk_program_select',0x3f6d50:'sdk_program_layout',
 0x1cf50:'texture_publish',0x3f8320:'sdk_texture_bind',0x400fa0:'dirty_flush',
 0x3f53d0:'sdk_cull',0x3f7330:'sdk_constant_write',0x403400:'dirty_vertex_inputs',
 0x403360:'dirty_vertex_attribute',0x3ffa80:'dirty_texture_control',
 0x400000:'dirty_vertex_mode',0x3f6f40:'dirty_fog_constants',0x400d90:'dirty_misc'}
OBSERVE={0x1c6b0,0x1c710,0x1c330,0x1c590,0x3f7440,0x3f6cf0,
 0x3f6d50,0x1cf50,0x400fa0,0x3f6c60,0x3f7330,0x3f53d0}
STACK_ARGUMENTS={0x1c710:1,0x1c330:2,0x1c590:1,0x3f7440:3,0x3f6c60:2,0x3f7330:1,0x3f53d0:1}

class PreparationTrace(EmissionTrace):
 def __init__(self,*args):
  super().__init__(*args);self.events=[];self.pending={}
 def context(self):
  d=self.u(0x407488);program=self.u(d+0x794)
  return dict(vertex_context_hex=self.read(0x51f0f0,0x2d8).hex(),device=f'{d:08x}',
   device_header_hex=self.read(d,0x40).hex(),device_program_words=words(self.read(d+0x794,12)),
   device_program_header_hex=self.read(program,0x114).hex(),sdk_scratch_header_hex=self.read(0x408518,0x114).hex(),
   device_stream_count=self.u(d+0xb04),device_streams_hex=self.read(d+0xb00,0xc4).hex(),
   dirty_word=self.u(0x407490),cull=self.u(0x4076e4),front_winding=self.u(0x4076e0),
   texture_current_hex=self.read(0x51f3c8,16).hex(),texture_wanted_hex=self.read(0x51f3d8,16).hex(),
   hill_bitmap_record_address='804e2434',hill_bitmap_record116_hex=self.read(0x804e2434,116).hex(),
   range_constants_cache_flag=self.read(0x485af1,1).hex(),
   material_vertex_constant_bank_hex=self.read(0x485b48+0x1530,0x200).hex(),
   device_vertex_program_shadow_hex=self.read(d+0x10a8,0xc00).hex())
 def code(self,g,pc,n,u):
  super().code(g,pc,n,u)
  if self.boundary:return
  if pc in self.pending:
   row=self.pending[pc].pop()
   if not self.pending[pc]:del self.pending[pc]
   row.update(completed=True,after=self.context(),returned_state=self.state())
  if self.phase=='preparation'and pc==0x19f8ad:
   self.boundary=dict(kind='Explicit partial-owner stop before hill BEGIN/geometry',pc=pc,state=self.state());g.emu_stop();return
  if pc in OBSERVE:
   sp=g.reg_read(UC_X86_REG_ESP);r=self.u(sp)
   row=dict(phase=self.phase,entry=f'{pc:08x}',return_address=f'{r:08x}',
    arguments=words(self.read(sp+4,STACK_ARGUMENTS.get(pc,0)*4))if pc in STACK_ARGUMENTS else[],
    stack12_hex=self.read(sp+4,12).hex(),before=self.context(),state=self.state())
   if pc==0x3f6c60:
    p=self.u(sp+4);count=int.from_bytes(self.read(p+2,2),'little')
    row.update(program_pointer=f'{p:08x}',program_hex=self.read(p,4+16*count).hex(),instruction_count=count)
   self.events.append(row);self.pending.setdefault(r,[]).append(row)

def one(snapshot,image,rows,geometry,rgb,load_hill,repeat):
 t=PreparationTrace(snapshot,image,rows);d=t.u(0x407488);begin=t.u(d);limit=t.u(d+4)
 initial=t.context();t.declare(POINTS,geometry,'Declared relocated whole-original Lockout King geometry')
 t.declare(COLOR,rgb,'Declared original first-view hill RGB input')
 t.declare(RECORD,b'\xa5'*24,'Declared output storage for real whole record filler')
 t.declare(0x4b9da0,geometry[0x164:0x170],'Declared center cameraXYZ; XY consumed by helper')
 fill=t.call('fill',0xd4bc0,[0xf2c81143,0,0,15,0,0xffffffff,RECORD],(),0x023f,0x1fa0)
 assert fill['complete_original_return']and t.g.reg_read(UC_X86_REG_EAX)&255
 args=[15,RECORD,POINTS+0x64,32,CENTER,uint(geometry,0x170),uint(geometry,0x17c),COLOR,fw(.8)]
 prefix=t.call('prefix',0x19f680,args,((UC_X86_REG_EAX,0),(UC_X86_REG_ECX,0),(UC_X86_REG_EDX,0xf2c81143)),0x023f,0x1fa0)
 assert prefix['stop']['pc']==0x19f73c
 frame=t.g.reg_read(UC_X86_REG_ESP);prefix['frame68_hex']=t.read(frame,0x68).hex()
 metadata=t.execute('metadata',0x19f75f);assert metadata['stop']['pc']==0x19f844
 metadata.update(frame68_hex=t.read(frame,0x68).hex(),backing=t.metadata_backing)
 frame_state=t.state();selection=None
 if load_hill:
  # This is an independent whole original call with actual resident program
  # bytes, NOT a successful return from omitted material preparation.
  # Use a separate declared call stack so the previously observed helper
  # frame and semantic-selected UV local are not overwritten by this call.
  call_stack=SYNTH+0x80000;t.put(call_stack,packed([STOP,0]));t.g.reg_write(UC_X86_REG_ESP,call_stack)
  t.g.reg_write(UC_X86_REG_ESI,0x51f0f0);t.g.reg_write(UC_X86_REG_EAX,0xf2cc1147)
  selection=t.execute('program_selection',0x1c590)
  assert selection['complete_original_return']
  for event in t.pending.pop(STOP,[]):
   event.update(completed=True,after=t.context(),returned_state=t.state(),observed_return_at_emulation_endpoint=True)
  t.g.reg_write(UC_X86_REG_ESP,frame)
  assert t.read(frame,0x68).hex()==metadata['frame68_hex']
  selection['declared_frame_resume']=dict(esp=f'{frame:08x}',independent_call_stack=f'{call_stack:08x}',
   frame68_hex=t.read(frame,0x68).hex(),reason='Resume untouched previously observed hill caller frame after independent whole source program selection')
 before=t.context();prep=[]
 for index in range(2 if repeat else 1):
  before_command=t.u(d);result=t.execute('preparation',0x19f844)
  assert result['stop']['pc']==0x19f8ad
  result.update(before_command=f'{before_command:08x}',after_command=f'{t.u(d):08x}',frame68_hex=t.read(frame,0x68).hex(),after=t.context())
  prep.append(result)
 end=t.u(d);assert begin<=end<=limit
 raw=t.read(begin,end-begin)
 assert t.u(0x4076e4)==initial['cull']
 assert t.read(0x485b48+0x1530,0x200).hex()==initial['material_vertex_constant_bank_hex']
 assert t.read(0x804e2434,116).hex()==initial['hill_bitmap_record116_hex']
 return dict(context_kind='declared_whole_hill_program_composition'if load_hill else'captured_weather_program',repeat=repeat,
  initial=initial,declarations=t.declarations,fill=fill,prefix=prefix,metadata=metadata,
  metadata_state=frame_state,program_selection=selection,before_preparation=before,preparation=prep,
  final=t.context(),events=t.events,original_calls=[f'{p:08x}'for p in t.sequence],
  command_begin=f'{begin:08x}',command_end=f'{end:08x}',command_limit=f'{limit:08x}',
  command_hex=raw.hex(),command_sha256=sha(raw),packets=decode_packets(raw),source_writes=t.writes,
  original_instructions=[dict(address=f'{p:08x}',bytes_hex=image.read(p,n).hex())for p,n in sorted(t.executed.items())],
  mapped_snapshot_pages={f'{p:08x}':sha(raw)for p,raw in sorted(t.pages.items())})

def static(directory):
 traces={name:KingTrace(name)for name in('retail','tu11','u5')}
 entries={name:{0x19f680:p}for name,p in(('retail',0x19f680),('tu11',0x1a2fe0),('u5',0x1a9290))}
 pairs=[];pending=[0x19f680];seen=set();owners={}
 while pending:
  key=pending.pop(0)
  if key in seen:continue
  seen.add(key);bodies={};functions={}
  for name,t in traces.items():
   entry=entries[name][key]
   section=next(s for s in t.image.sections if s.va<=entry<s.va+s.vsize)
   code=_Code(t.image,section.name)
   # SDK inventories can list internal basic-block labels as function starts.
   # Complete CFG traversal must retain those original branch targets.
   f,ins=literal(code,entry,{entry}if section.name=='D3D'else t.starts);assert ins;functions[name]=f;bodies[name]=ins
   p=directory/f'{name}-{OWNERS[key]}-{entry:08x}.asm'
   p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins))
   owners.setdefault(name,{})[OWNERS[key]]=dict(entry=f'{entry:08x}',file=p.name,sha256=sha(p.read_bytes()),instructions=len(ins))
  oi=bodies['retail']
  for name in('tu11','u5'):
   ni=bodies[name];same=complete_tokens(oi,traces['retail'].image)==complete_tokens(ni,traces[name].image)
   tables=table_pair(functions['retail'],functions[name],oi,ni,traces['retail'].image,traces[name].image)
   matched,changes=alignment(oi,ni,traces['retail'].image,traces[name].image)
   pairs.append(dict(owner=OWNERS[key],later=name,complete_shape_equal=same,switch_tables=tables,literal_changes=changes,
    relocated_operands=references([oi[k]for k in matched],[ni[matched[k]]for k in matched],traces['retail'].image,traces[name].image)))
   for oldindex,newindex in matched.items():
    x,y=oi[oldindex],ni[newindex]
    if x.mnemonic=='call'and x.op_str.startswith('0x'):
     target=int(x.op_str,16)
     if target in OWNERS:
      value=int(y.op_str,16)
      assert target not in entries[name]or entries[name][target]==value
      entries[name][target]=value;entries['retail'][target]=target
      if target not in seen:pending.append(target)
 assert seen==set(OWNERS),(set(OWNERS)-seen)
 constants={}
 for name,t in traces.items():
  # Map these exact operands from the complete hill owner, not address deltas.
  refs=next(p['relocated_operands']for p in pairs if p['owner']=='hill_helper'and p['later']==name)if name!='retail'else[]
  mapping={int(p['old_address'],16):int(p['later_address'],16)for p in refs}
  constants[name]={}
  for old,size in((0x43f8df,20),(0x51f0f0,0x2d8),(0x4076e4,4),(0x51f1bc,4),(0x51f17c,4),(0x51f2fc,1)):
   addr=old if name=='retail'else mapping[old]
   constants[name][f'{old:08x}']=dict(address=f'{addr:08x}',executable_bytes_hex=t.image.read(addr,size).hex())
 return dict(owners=owners,pairs=pairs,constants=constants,
  images={name:dict(path=str(t.spec['path']),sha256=sha(t.image.data))for name,t in traces.items()})

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'complete/report.json');a=ap.parse_args()
 a.output.parent.mkdir(parents=True,exist_ok=True);idir=a.output.parent/'instructions';idir.mkdir(exist_ok=True)
 s=Snapshot(SNAPSHOT);assert sha(s.ram)==RAM_PIN;i=Xbe(ROOT/'assets/disc/default.xbe');assert sha(i.data)==PIN
 visual=json.loads(VISUAL.read_text());geometry=bytes.fromhex(visual['geometry_hex']);rgb=bytes.fromhex(visual['results']['retail']['constants']['00468c84']['hex'])
 cases=[one(s,i,read_rows(FUNCTIONS_CSV),geometry,rgb,load,repeat)for load,repeat in((False,False),(True,False),(True,True))]
 evidence=static(idir)
 preserved=a.output.parent/'preserved-whole-center.json';preserved.write_bytes(DRAW.read_bytes())
 assert next(c for c in json.loads(DRAW.read_text())['cases']if c['mode']=='declared_center')['draw']['stop']['record']==0x804e2434
 observed={int(x['address'],16):bytes.fromhex(x['bytes_hex'])for c in cases for x in c['original_instructions']}
 observed_path=a.output.parent/'observed-original.asm'
 from disasm import listing
 observed_path.write_text('\n'.join(line for p,raw in sorted(observed.items())for line in listing(raw,p,{}))+'\n')
 dependencies=sorted({Path(m.__file__).resolve()for m in sys.modules.values()if getattr(m,'__file__',None)and Path(m.__file__).is_file()and Path(m.__file__).resolve().is_relative_to(ROOT/'tools')})
 result=dict(schema='king-hill-preparation-source-v1',scope=__doc__,tool_sha256=sha(Path(__file__).read_bytes()),
  snapshot=str(SNAPSHOT.relative_to(ROOT)),snapshot_ram_sha256=RAM_PIN,snapshot_manifest_sha256=sha((SNAPSHOT/'export.json').read_bytes()),
  visual_report_sha256=sha(VISUAL.read_bytes()),preserved_whole_draw_report_sha256=sha(preserved.read_bytes()),
  dependencies={str(p.relative_to(ROOT)):sha(p.read_bytes())for p in dependencies},
  functions_index=dict(path=str(Path(FUNCTIONS_CSV).relative_to(ROOT)),sha256=sha(Path(FUNCTIONS_CSV).read_bytes())),
  observed_listing=dict(file=observed_path.name,sha256=sha(observed_path.read_bytes())),cases=cases,static=evidence,
  instruction_patches=[],supplied_returns=[],manufactured_readiness=[],
  summary=dict(cases=len(cases),partial_preparation_corridors=sum(len(c['preparation'])for c in cases),
   whole_record_fills=len(cases),whole_hill_program_selections=sum(c['program_selection']is not None for c in cases),
   service_entries=sum(len(c['events'])for c in cases),complete_service_returns=sum(bool(e.get('completed'))for c in cases for e in c['events']),
   static_owners_per_image=len(OWNERS),correspondence_pairs=len(evidence['pairs']),
   complete_shape_equal_pairs=sum(p['complete_shape_equal']for p in evidence['pairs']),
   unique_executed_original_instructions=len(observed),
   command_bytes=sum(len(bytes.fromhex(c['command_hex']))for c in cases),whole_positive_hill_draws=0))
 a.output.write_text(json.dumps(result,indent=2,sort_keys=True)+'\n');print(json.dumps(result['summary'],indent=2))

if __name__=='__main__':main()

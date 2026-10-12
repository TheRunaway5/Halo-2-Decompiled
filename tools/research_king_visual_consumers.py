#!/usr/bin/env python3
"""Bounded King visual-consumer primary-source inventory and unchanged CPU probes.

Original retail/TU1.1/TU1.5 engine registrations, executable bytes and owned
Xbox cache tags are authorities. Declared player/pool/view/geometry records
are isolated inputs, not natural world or renderer boot. No instruction patch,
game return or graphics readiness is supplied. Draw/service stops are explicit.
"""
import argparse,collections,json,struct
from pathlib import Path
from unicorn import UC_HOOK_CODE,UC_HOOK_MEM_INVALID,UcError
from unicorn.x86_const import *
from oracle_king_engine import KingTrace,OWNERS as KING,G,M,P,R,O,H,U,K,S,F,C,Q,D,source_maps
from oracle_game_variants import STACK,STOP
from oracle_mp_full_scoreboard_1_5 import ROOT,HudCache,literal,complete_tokens,references,sha
from research_mp_lobby_later_update import normalized,table_pair,alignment
from inspect_multiplayer import MAPS

OUT=ROOT/'analysis/multiplayer/game-variants/king-visual-consumers-source-20261012'
SLOTS={13:'world_submission',14:'markers',28:'text_consumer'}
SEEDS={'world_submission':0x2bd240,'markers':0x2bd820,'text_consumer':0x2bd910,
 'hill_color':0x2be3b0,'hill_draw':0x2be5d0,'submission_dispatch':0x2be650,
 'submit_hill':0x2be670,'hill_marker':0x2be6d0,'draw_polygon':0x19f680,
 'world_caller':0x1595d0,'marker_caller':0x15ea50,'score_query':0x15f3a0,
 'hud_score_owner':0x13992a,'hud_mask_owner':0x13a050,'world_render_owner':0x2bcd0,
 'hud_render_owner':0x13954b,'marker_projection':0x24e59f,'marker_arrow':0x24e9c7,
 'marker_icons':0x24ecae,'marker_text':0x24f0fc,'active_iterator':0x19f300,
 'team_color':0x7f720,'opponent_marker':0x162550,'sort_submission':0x41490,'sort_allocate':0x1e2d0,
 'fill_record':0xd4bc0,'dispatch_sorted':0x40f60}
GLOBALS={'retail':dict(matg=0x4e034c,tags=0x4e3b44,local=0x4e8c20,view=0x4b9ed8,hudg=0x510c94),
 'tu11':dict(matg=0x4e791c,tags=0x4eb114,local=0x4f01f0),
 'u5':dict(matg=0x4ecc1c,tags=0x4f0414,local=0x4f54f0)}
L=D+0x22000;B=D+0x22100;OUTPUT=D+0x22200;TAGS=0x6000000

def listing(t,e,directory):
 f,ins=literal(t.code,e,t.starts)
 p=directory/f'{t.name}-{e:08x}.asm'
 p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins))
 return f,ins,dict(file=p.name,sha256=sha(p),instructions=len(ins),entry=f'{e:08x}')

def direct(ins):return [int(i.op_str,16) for i in ins if i.mnemonic=='call'and i.op_str.startswith('0x')]

def locate(traces,seeds):
 old=traces['retail'];wanted={k:literal(old.code,e,old.starts)[1]for k,e in seeds.items()}
 keys=collections.defaultdict(list)
 for k,ins in wanted.items():keys[tuple(normalized(i,old.image)for i in ins[:8])].append(k)
 lengths={len(k)for k in keys}
 result={'retail':dict(seeds)}
 for name,t in traces.items():
  if name=='retail':continue
  candidates=collections.defaultdict(list);prefix_candidates=collections.defaultdict(list)
  # The published TU function index omits actual King callback starts.
  # Candidate locations additionally come from unchanged direct CALL and
  # code-valued MOV/PUSH operands; candidates are accepted only by whole CFG.
  possible=set(t.starts);t.code.md.skipdata=True
  for a,n,m,op in t.code.md.disasm_lite(t.code.bytes,t.code.lo):
   if m in('call','push')and op.startswith('0x'):
    p=int(op,16)
    if t.code.inside(p):possible.add(p)
  v=t.u(t.u(t.spec['table']+16));possible.update(t.u(v+i*4)for i in range(51))
  t.code.md.skipdata=False
  for e in sorted(possible):
   prefix=list(t.code.md.disasm(t.image.read(e,128),e,count=8))
   tokens=tuple(normalized(i,t.image)for i in prefix)
   for length in lengths:
    key=tokens[:length]
    if key not in keys:continue
    _,ins=literal(t.code,e,t.starts)
    for k in keys[key]:
     prefix_candidates[k].append(e)
     if complete_tokens(ins,t.image)==complete_tokens(wanted[k],old.image):candidates[k].append(e)
  # A unique complete eight-instruction prefix can locate a changed owner;
  # its full CFG differences remain explicit in the final correspondence.
  for k in seeds:
   if not candidates[k]and len(prefix_candidates[k])==1:candidates[k]=prefix_candidates[k]
  result[name]={k:v[0]for k,v in candidates.items()if len(v)==1}
  for k in seeds:
   if k in result[name]:continue
   constrained=set()
   for owner,e in result[name].items():
    _,ni=literal(t.code,e,t.starts)
    for oi,newi in zip(wanted[owner],ni):
     for op,nop in zip(oi.operands,newi.operands):
      if op.type==nop.type==2 and op.imm==seeds[k]:constrained.add(nop.imm)
   chosen=set(candidates[k])&constrained
   assert len(chosen)==1,(name,k,[hex(v)for v in candidates[k]],constrained)
   result[name][k]=chosen.pop()
 return result

class VisualTrace(KingTrace):
 def __init__(self,name,cache,entries):
  super().__init__(name);self.e=entries;self.v=GLOBALS[name];self.events=[];self.stop=None;self.blocks=set();self.recent=collections.deque(maxlen=16)
  page=cache.magic&~4095;self.g.mem_map(page,((len(cache.data)+cache.magic-page+4095)//4096)*4096)
  self.g.mem_write(cache.magic,bytes(cache.data));self.g.mem_map(TAGS,0x100000)
  for d,t in cache.tags.items():
   # The literal four-word tag index, including actual class and size.
   self.g.mem_write(TAGS+(d&65535)*16,struct.pack('<4I',int.from_bytes(t['group'].encode()[::-1],'little'),d,cache.magic+t['offset']if t['offset']is not None else 0,t['size']))
  matg=next(t for t in cache.tags.values()if t['group']=='matg');interface=cache.block(matg['offset']+0x110,0x98)[0];hudg=cache.tags[cache.u(interface+0x34)]
  self.w(self.v['matg'],cache.magic+matg['offset']);self.w(self.v['tags'],TAGS)
  self.w(self.v['hudg'],cache.magic+hudg['offset']);self.w(self.v['local'],L)
  self.g.hook_add(UC_HOOK_CODE,self.observe);self.g.hook_add(UC_HOOK_MEM_INVALID,self.invalid)
 def observe(self,g,a,n,_):
  self.recent.append(a)
  assert bytes(g.mem_read(a,n))==self.image.read(a,n),(self.name,hex(a),'instruction modified')
  if a in self.blocks:
   sp=g.reg_read(UC_X86_REG_ESP)
   self.stop=dict(kind='stop_before_original_service',entry=f'{a:08x}',registers={k:f'{g.reg_read(v):08x}'for k,v in(('eax',UC_X86_REG_EAX),('ecx',UC_X86_REG_ECX),('edx',UC_X86_REG_EDX),('edi',UC_X86_REG_EDI))},stack_hex=bytes(g.mem_read(sp,64)).hex())
   if a==self.e['marker_projection']:
    p=self.u(sp+4);raw=bytes(g.mem_read(p,0x90));self.stop.update(marker90_hex=raw.hex(),defined_ranges=[[0,2],[4,0x6c]])
   if a==self.e['sort_submission']:
    self.stop.update(shader_datum=f'{g.reg_read(UC_X86_REG_EAX):08x}',center_hex=bytes(g.mem_read(self.u(sp+32),12)).hex())
   if a==self.e['draw_polygon']:
    self.stop.update(polygon_hex=bytes(g.mem_read(self.u(sp+12),256)).hex(),center_hex=bytes(g.mem_read(self.u(sp+20),12)).hex(),color_hex=bytes(g.mem_read(self.u(sp+32),12)).hex())
   if a==self.e['polygon_graphics']:
    self.stop.update(distance_setup_stack_hex=bytes(g.mem_read(sp,0x70)).hex(),xmm_bits={f'xmm{i}':f'{g.reg_read(r):032x}'for i,r in enumerate((UC_X86_REG_XMM0,UC_X86_REG_XMM1,UC_X86_REG_XMM2,UC_X86_REG_XMM3,UC_X86_REG_XMM4))})
   g.emu_stop()
 def invalid(self,g,access,a,n,v,_):
  ip=g.reg_read(UC_X86_REG_EIP);sp=g.reg_read(UC_X86_REG_ESP);instruction=next(self.code.md.disasm(self.image.read(ip,15),ip))
  self.stop=dict(kind='unavailable_original_memory',eip=f'{ip:08x}',access=access,address=f'{a:08x}',size=n,recent=[f'{p:08x}'for p in self.recent],
   instruction=dict(bytes=instruction.bytes.hex(),mnemonic=instruction.mnemonic,operands=instruction.op_str),
   registers={k:f'{g.reg_read(r):08x}'for k,r in(('eax',UC_X86_REG_EAX),('ebx',UC_X86_REG_EBX),('ecx',UC_X86_REG_ECX),('edx',UC_X86_REG_EDX),('esi',UC_X86_REG_ESI),('edi',UC_X86_REG_EDI),('ebp',UC_X86_REG_EBP),('esp',UC_X86_REG_ESP))},
   stack_hex=bytes(g.mem_read(sp,64)).hex());return False
 def run_probe(self,key,args=(),regs=(),blocks=()):
  self.blocks=set(blocks);self.stop=None;self.recent.clear();before=set(self.executed)
  self.w(STACK,STOP);self.g.mem_write(STACK-0x1000,b'\xa5'*0x1000)
  for i,v in enumerate(args):self.w(STACK+4+i*4,v)
  self.g.reg_write(UC_X86_REG_ESP,STACK);self.g.reg_write(UC_X86_REG_FPCW,0x27f);self.g.reg_write(UC_X86_REG_MXCSR,0x1f80)
  for r,v in regs:self.g.reg_write(r,v&0xffffffff)
  try:self.g.emu_start(self.e[key],STOP,count=200000)
  except UcError:assert self.stop is not None
  eip=self.g.reg_read(UC_X86_REG_EIP);assert eip==STOP or self.stop is not None
  return dict(completion='complete_original_return'if eip==STOP else'stopped_original_instruction',stop=self.stop,
   fpcw=f'{self.g.reg_read(UC_X86_REG_FPCW):04x}',mxcsr=f'{self.g.reg_read(UC_X86_REG_MXCSR):08x}',
   esp_after=f'{self.g.reg_read(UC_X86_REG_ESP):08x}')
 def setup_players(self,row):
  self.g.mem_write(G,bytes(0x1200));self.g.mem_write(M,bytes(0x1000));self.g.mem_write(R,bytes(16*0x21c));self.g.mem_write(L,b'\xff'*0x40)
  self.w(P+0x20,16);self.w(P+0x24,0x21c);self.w(P+0x38,16);self.w(P+0x44,R);self.w(P+0x48,B)
  self.w(M+0xc14,4 if row.get('engine',True)else 0);self.w(G+0x184,int(row.get('team_mode',False)))
  mask=0
  for p in row['players']:
   slot=p['slot'];mask|=1<<slot;at=R+slot*0x21c;self.h(at,0x9abc);self.h(at+2,p.get('flags',1));self.w(at+0x2c,0x8aaa0000+slot if p.get('unit',True)else -1)
   self.g.mem_write(at+0xc0,bytes([p['team']&255]));self.g.mem_write(at+0x88,bytes([p.get('type',0)]))
   body=U+slot*0x100;self.w(H+slot*12+8,body);self.w(body+0x14,-1);self.g.mem_write(body+0x64,struct.pack('<3f',*p.get('position',(0,0,0))))
  self.w(B,mask);self.w(L+0xc,0x9abc0000+row['local_slot']if row.get('local_slot',-1)!=-1 else -1)
  self.w(self.v['view'],row.get('view',0));self.h(K+0x1a8,row.get('occupants',0))

def color_inputs():
 rows=[]
 rosters=[[],[dict(slot=0,team=0)],[dict(slot=0,team=0),dict(slot=3,team=1)],
  [dict(slot=0,team=0),dict(slot=3,team=0),dict(slot=15,team=7)],
  [dict(slot=0,team=-1),dict(slot=3,team=-1),dict(slot=15,team=7)],
  [dict(slot=0,team=0,unit=False),dict(slot=3,team=1,flags=2),dict(slot=15,team=7)],
  [dict(slot=0,team=0),dict(slot=3,team=-1),dict(slot=15,team=127)]]
 for roster in rosters:
  for team_mode in (False,True):
   for occupants in (0,1,8,0x8000,9,0x8001,0x8008,0xffff):
    for query in (-1,0,3,15):
     rows.append(dict(players=roster,team_mode=team_mode,occupants=occupants,query=query,local_slot=0,view=0))
 for kind in (0,1,2,3,255):
  for view in (-1,0):
   for local in (-1,0):
    for query in (-1,0):rows.append(dict(players=[dict(slot=0,team=0,type=kind)],team_mode=False,occupants=1,query=query,local_slot=local,view=view))
 for team_mode in (False,True):
  for occupants in (0,1,8,9):
   rows.append(dict(players=[dict(slot=0,team=0),dict(slot=3,team=1)],team_mode=team_mode,occupants=occupants,query=0,local_slot=0,view=0,engine=False))
 return rows

def cache_inventory(cache):
 matg=next(t for t in cache.tags.values()if t['group']=='matg');mulg=cache.tags[cache.u(matg['offset']+0x16c)];rt=cache.block(mulg['offset']+8,0x568)[0]
 settings=cache.u(rt+0x534)-cache.magic;shader=cache.tags[cache.u(settings+0xc4)];interface=cache.block(matg['offset']+0x110,0x98)[0];hudg=cache.tags[cache.u(interface+0x34)];hr=cache.block(hudg['offset']+0x410,0x18)[6]
 bitm=cache.tags[cache.u(hr+4)];seq=struct.unpack_from('<3h',cache.data,hr+0x10)
 bm=cache.bitmap(bitm)
 arrows=cache.block(hudg['offset']+0x408,0x24);ar=arrows[1]
 un=cache.block(mulg['offset'],0x20)[0];messages=cache.tags[cache.u(un+0x1c)]
 total,size,ix,ss=struct.unpack_from('<4I',cache.data,matg['offset']+0x190);time_label=[]
 for i in range(total):
  sid,offset=struct.unpack_from('<II',cache.data,ix+i*8)
  if sid==0x07000232:time_label.append(dict(index=i,string_id=f'{sid:08x}',sid_name=cache.sid(sid),text=cache.text(ss+offset),index8_hex=cache.data[ix+i*8:ix+i*8+8].hex()))
 template=cache.template(cache.tags[cache.u(shader['offset']+4)]);vertex=[]
 for pass_tag in template['passes']:
  for postprocess in pass_tag['postprocess']:
   for implementation in postprocess['implementations']:
    at=int(implementation['offset'],16);tag=cache.tags[cache.u(at+0x100)];classifications=[]
    assert tag['group']=='vrtx'
    for q in cache.block(tag['offset']+4,0x1c):
     semantics=[cache.h(s)for s in cache.block(q+4,2)];size=cache.u(q+0xc);code=cache.u(q+0x10)-cache.magic
     classifications.append(dict(file_offset=q,raw1c_hex=cache.data[q:q+0x1c].hex(),semantic_ids=semantics,
      first_uv_semantic_register=next((i for i,s in enumerate(semantics)if s==3),None),
      code_file_offset=code,code_size=size,code_hex=cache.data[code:code+size].hex(),
      code_words=[f'{cache.u(s):08x}'for s in range(code,code+size,4)]))
    vertex.append(dict(pass_datum=pass_tag['datum'],implementation_file_offset=at,vertex_reference_hex=cache.data[at+0xfc:at+0x104].hex(),tag=tag,classifications=classifications))
 return dict(path=str(cache.path.relative_to(ROOT)),cache_sha256=sha(cache.path),matg=matg,mulg=mulg,runtime_file_offset=rt,
  runtime_hill_reference_hex=cache.data[rt+0x20:rt+0x28].hex(),settings_pointer_hex=f'{cache.u(rt+0x534):08x}',settings_file_offset=settings,
  settings_hill_reference_hex=cache.data[settings+0xc0:settings+0xc8].hex(),hill_shader=shader,hill_shader_metadata_hex=cache.data[shader['offset']:shader['offset']+shader['size']].hex(),
  hill_shader_decoded=cache.shader(shader),hill_template=template,hill_vertex_programs=vertex,
  hudg=hudg,hud_interface_file_offset=interface,hud_interface_reference_hex=cache.data[interface+0x30:interface+0x38].hex(),
  marker_row6_file_offset=hr,marker_row6_hex=cache.data[hr:hr+0x18].hex(),marker_bitmap=bitm,marker_shader=cache.reference(cache.u(hr+0xc)),
  marker_sequences=seq,marker_sequence_metadata=[bm['sequences'][s]if s>=0 else None for s in seq],
  marker_bitmap_images=bm['images'],arrow1_file_offset=ar,arrow1_hex=cache.data[ar:ar+0x24].hex(),arrow_bitmap=cache.reference(cache.u(ar+4)),arrow_shader=cache.reference(cache.u(ar+0xc)),arrow_border=cache.reference(cache.u(ar+0x20)),
  time_label=time_label,multiplayer_text=messages)

def main():
 ap=argparse.ArgumentParser(description=__doc__)
 ap.add_argument('--output',type=Path,default=OUT/'complete/report.json')
 a=ap.parse_args();directory=a.output.parent/'instructions';directory.mkdir(parents=True,exist_ok=True)
 traces={name:KingTrace(name)for name in KING};entries={};pairs=[];listings={}
 for name,t in traces.items():
  v=t.u(t.u(t.spec['table']+16));entries[name]={k:t.u(v+slot*4)for slot,k in SLOTS.items()}
 located=locate(traces,SEEDS)
 for name in entries:entries[name].update(located[name])
 # Registered callbacks determine counterparts; complete matching bodies
 # constrain their direct-call counterparts without assumed address deltas.
 for oldname in ('retail','tu11'):
  old=traces[oldname];new=traces['u5'];todo=[(k,entries[oldname][k],entries['u5'][k],0)for k in SLOTS.values()]
  todo += [(k,entries[oldname][k],entries['u5'][k],2)for k in SEEDS if k not in SLOTS.values()]
  seen=set()
  while todo:
   key,oe,ne,depth=todo.pop(0)
   if(oe,ne)in seen:continue
   seen.add((oe,ne));of,oi,ol=listing(old,oe,directory);listings[ol['file']]=ol
   if ne is None:continue
   nf,ni,nl=listing(new,ne,directory);listings[nl['file']]=nl
   same=complete_tokens(oi,old.image)==complete_tokens(ni,new.image)
   tables=table_pair(of,nf,oi,ni,old.image,new.image);assert tables is not None and all(x['topology_equal']for x in tables)
   _,changes=alignment(oi,ni,old.image,new.image)
   pairs.append(dict(earlier=oldname,key=key,old_entry=f'{oe:08x}',u5_entry=f'{ne:08x}',old_instructions=len(oi),u5_instructions=len(ni),complete_shape_equal=same,
    switch_tables=tables,literal_changes=changes,relocated_operands=references(oi,ni,old.image,new.image)))
   entries[oldname][key]=oe;entries['u5'][key]=ne
   if same and depth<2:
    for j,(x,y)in enumerate(zip(direct(oi),direct(ni))):todo.append((f'{key}.call{j}',x,y,depth+1))
 # Resolve render globals from paired actual absolute-memory operands.
 for name,t in traces.items():
  if name=='retail':continue
  for key,oldglobal in (('view',0x4b9ed8),('hudg',0x510c94)):
   seed='hill_color'if key=='view'else'marker_icons'
   _,oi=literal(traces['retail'].code,SEEDS[seed],traces['retail'].starts);_,ni=literal(t.code,entries[name][seed],t.starts)
   candidates={int(v['later_address'],16)for v in references(oi,ni,traces['retail'].image,t.image)if int(v['old_address'],16)==oldglobal}
   assert len(candidates)==1,(name,key,candidates);GLOBALS[name][key]=candidates.pop()
 for name,t in traces.items():
  assert entries[name]['sort_submission']==direct(literal(t.code,entries[name]['submit_hill'],t.starts)[1])[0]
  entries[name]['polygon_graphics']=direct(literal(t.code,entries[name]['draw_polygon'],t.starts)[1])[0]
 cache=HudCache(ROOT/'assets/disc/maps/lockout.map');assert sha(cache.path)=='14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9'
 colors=color_inputs();results={};geometries={}
 maprow=next(x for x in source_maps()if x['name']=='lockout')
 for name,old in traces.items():
  t=VisualTrace(name,cache,entries[name]);traces[name]=t;t.setup(bytes.fromhex(maprow['rows_hex']));t.run('initialize');t.run('start');geometry=t.raw();geometries[name]=geometry.hex();rr=[]
  for row in colors:
   t.setup_players(row);t.g.mem_write(OUTPUT,b'\xa5'*16)
   before=bytes(t.g.mem_read(R,16*0x21c));probe=t.run_probe('hill_color',(OUTPUT,),((UC_X86_REG_EAX,0x9abc0000+row['query']if row['query']!=-1 else -1),))
   assert probe['completion']=='complete_original_return';assert bytes(t.g.mem_read(R,16*0x21c))==before
   rr.append(dict(rgb_hex=bytes(t.g.mem_read(OUTPUT,12)).hex(),**probe))
  marker=[]
  for count in (-1,0,1,3,4,5,6,16,32):
   for distance in (0,0.5,1,1.25,1.5,2,3):
    row=dict(players=[dict(slot=0,team=0,position=(struct.unpack_from('<f',geometry,0x164)[0]+struct.unpack_from('<f',geometry,0x170)[0]*distance,struct.unpack_from('<f',geometry,0x168)[0],0))],team_mode=False,occupants=1,local_slot=0,view=0)
    t.setup_players(row);t.g.mem_write(K,geometry);t.w(K+0x60,count);t.h(K+0x1a8,1)
    probe=t.run_probe('hill_marker',(K,),((UC_X86_REG_EDI,0),),blocks=(t.e['marker_projection'],))
    marker.append(dict(input=dict(count=count,distance_radius_multiple=distance,**row),**probe))
  # Entire registered callback/adapters complete for their genuine invalid-hill gates.
  gates=[]
  for count in (-1,0,1,3,5):
   row=dict(players=[],team_mode=False,occupants=0,local_slot=-1,view=-1);t.setup_players(row);t.g.mem_write(K,geometry);t.w(K+0x60,count)
   for key,args,regs in (('world_submission',(0x9abc0000,),()),('markers',(-1,),()),('hill_draw',(0,0,0,0,0,0,0),())):
    probe=t.run_probe(key,args,regs);assert probe['completion']=='complete_original_return';gates.append(dict(count=count,owner=key,**probe))
  world=[]
  for key,args,blocks in (('world_submission',(0x9abc0000,),(t.e['sort_submission'],)),('world_submission',(0x9abc0000,),())):
   t.setup_players(dict(players=[dict(slot=0,team=0)],team_mode=False,local_slot=0,view=0));t.g.mem_write(K,geometry)
   world.append(dict(owner=key,**t.run_probe(key,args,blocks=blocks)))
  draw=[]
  _,oi=literal(traces['retail'].code,SEEDS['draw_polygon'],traces['retail'].starts);_,ni=literal(t.code,t.e['draw_polygon'],t.starts)
  camera={int(v['old_address'],16):int(v['later_address'],16)for v in references(oi,ni,traces['retail'].image,t.image)}
  cx,cy,cz=struct.unpack_from('<3f',geometry,0x164);radius=struct.unpack_from('<f',geometry,0x170)[0]
  for distance in (0,1,2,2.5,3,10):
   t.setup_players(dict(players=[dict(slot=0,team=0)],team_mode=False,local_slot=0,view=0));t.g.mem_write(K,geometry);t.h(K+0x1a8,1)
   for oldat,value in ((0x4b9da0,cx+radius*distance),(0x4b9da4,cy)):
    t.g.mem_write(camera[oldat],struct.pack('<f',value))
   for stop in ('draw_polygon','polygon_graphics'):
    probe=t.run_probe('hill_draw',(0x100,0x101,0x102,0x103,0x104,0x105,0x106),blocks=(t.e[stop],))
    draw.append(dict(input=dict(camera=[cx+radius*distance,cy,0],distance_radius_multiple=distance,callback_arguments=[0x100+i for i in range(7)]),boundary=stop,**probe))
  observed=directory/f'{name}-executed.asm';observed.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for address in sorted(t.executed)for i in[t.code.at(address)]))
  constants={}
  for key,entry in(('neutral',t.e['hill_color']),('boundary',t.e['draw_polygon'])):
   _,oi=literal(traces['retail'].code,SEEDS['hill_color'if key=='neutral'else'draw_polygon'],traces['retail'].starts);_,ni=literal(t.code,entry,t.starts)
   refs={int(v['old_address'],16):int(v['later_address'],16)for v in references(oi,ni,traces['retail'].image,t.image)}
   for at,size in ([(0x468714,4),(0x468c84,12),(0x468c94,12)]if key=='neutral'else[(0x47ff94,4),(0x45dc0c,4),(0x45dbdc,4),(0x45dce8,4)]):
    p=refs[at];constants[f'{at:08x}']=dict(address=f'{p:08x}',hex=t.image.read(p,size).hex())
    if at==0x468714:constants[f'{at:08x}']['pointed_rgb_hex']=t.image.read(int.from_bytes(t.image.read(p,4),'little'),12).hex()
  # Actual shader/layer15 implementation0 record-filler arguments from40F60.
  # This declared API composition does not fabricate a sorted-record result.
  shader=cache.u(cache.u(cache.block(cache.tags[cache.u(next(x for x in cache.tags.values()if x['group']=='matg')['offset']+0x16c)]['offset']+8,0x568)[0]+0x534)-cache.magic+0xc4)
  t.setup_players(dict(players=[dict(slot=0,team=0)],team_mode=False,local_slot=0,view=0));t.g.mem_write(K,geometry);t.g.mem_write(OUTPUT,b'\xa5'*0x100)
  for oldat,value in ((0x4b9da0,cx),(0x4b9da4,cy)):t.g.mem_write(camera[oldat],struct.pack('<f',value))
  fill=t.run_probe('fill_record',(shader,0,0,15,0,0xffffffff,OUTPUT));fill['record18_hex']=bytes(t.g.mem_read(OUTPUT,24)).hex();fill['defined_al']=t.g.reg_read(UC_X86_REG_EAX)&255
  realdraw=None
  if fill['completion']=='complete_original_return'and fill['defined_al']:
   realdraw=t.run_probe('hill_draw',(shader,0,0,15,0,0xffffffff,OUTPUT))
  observed.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for address in sorted(t.executed)for i in[next(t.code.md.disasm(t.image.read(address,15),address))]))
  results[name]=dict(colors=rr,markers=marker,gates=gates,world=world,draw=draw,actual_fill=fill,declared_filled_record_draw=realdraw,globals={k:f'{v:08x}'for k,v in t.v.items()},constants=constants,executed_instructions=len(t.executed),executed_listing=dict(file=observed.name,sha256=sha(observed)))
 assert len(set(geometries.values()))==1
 for name in ('tu11','u5'):
  assert [x['rgb_hex']for x in results[name]['colors']]==[x['rgb_hex']for x in results['retail']['colors']]
  assert [(x['completion'],x['stop']['marker90_hex']if x['stop']else None)for x in results[name]['markers']]==[(x['completion'],x['stop']['marker90_hex']if x['stop']else None)for x in results['retail']['markers']]
  assert results[name]['actual_fill']['record18_hex']==results['retail']['actual_fill']['record18_hex']
 for r in results.values():assert r['actual_fill']['completion']=='complete_original_return'and r['actual_fill']['defined_al']==1
 inventory=[cache_inventory(HudCache(ROOT/'assets/disc/maps'/f'{name}.map'))for name in MAPS]
 # Whole callback bytes are always emitted, including unmatched bodies.
 result=dict(schema='king-visual-consumers-source-v1',scope=__doc__,tool_sha256=sha(__file__),
  dependencies={f'tools/{n}.py':sha(ROOT/'tools'/f'{n}.py')for n in('oracle_king_engine','oracle_game_variants','oracle_game_over','oracle_mp_full_scoreboard_1_5','research_mp_lobby_later_update','inspect_multiplayer_hud','inspect_multiplayer')},
  versions={name:dict(image_path=str(t.spec['path']),image_sha256=t.spec['pin'],registered_entries={k:f'{v:08x}'for k,v in entries[name].items()})for name,t in traces.items()},
  complete_pairs=pairs,instruction_listings=list(listings.values()),color_inputs=colors,geometry_hex=geometries['retail'],geometry_source_map=maprow,results=results,cache_inventory=inventory,instruction_patches=[],supplied_returns=[],
  summary=dict(pairs=len(pairs),listings=len(listings),whole_colors=len(colors)*3,marker_calls=63*3,
   marker_whole_returns=sum(x['completion']=='complete_original_return'for r in results.values()for x in r['markers']),
   marker_positive_prefixes=sum(x['stop']is not None for r in results.values()for x in r['markers']),
   whole_gate_calls=15*3,world_prefixes=3,world_sort_allocator_stops=3,draw_calls=36,
   whole_draw_far_returns=sum(x['completion']=='complete_original_return'for r in results.values()for x in r['draw']),
   whole_actual_record_fills=3,declared_filled_record_draw_sdk_stops=3,maps=len(inventory)))
 a.output.write_text(json.dumps(result,sort_keys=True,indent=2)+'\n');print(json.dumps(result['summary'],indent=2))

if __name__=='__main__':main()

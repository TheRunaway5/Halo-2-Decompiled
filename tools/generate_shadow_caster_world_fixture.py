#!/usr/bin/env python3
"""Freeze actual generation intermediates and literal model identity, never native goldens.

The retained translated shader initializes private variables in source order.
The immutable DebugVertex export preserves first-change insertion order. Their
parallel names/IDs identify actual final R0 and the c172 UV-decompression
register; no native pose or pixel result is used to choose these values.
"""
from pathlib import Path
import hashlib,json,re,struct,sys
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from audit_rendering import Cache,file_sha
from audit_world_vertex_declarations import raw_model_streams
from xbe import Xbe
SRC=ROOT/'analysis/multiplayer-shading/original-xemu/shadow-gpu-audit'
OUT=ROOT/'tests/fixtures'
EVENTS=(7713,7721,7729,7737,7742,7750,7758)
CLASS={7713:2,7721:2,7729:1,7737:4,7742:4,7750:5,7758:1}
def sha(b):return hashlib.sha256(b).hexdigest()
def words(b):return struct.unpack('<'+'I'*(len(b)//4),b)
def text(values):return ' '.join(f'{v:08X}'for v in values)
def read_stream(record):
 with (ROOT/record['file']).open('rb')as f:f.seek(record['file_offset']);raw=f.read(record['bytes'])
 assert sha(raw)==record['sha256'];return raw

def main():
 c=Cache(ROOT/'assets/disc/maps/zanzibar.map');xbe=Xbe(ROOT/'assets/disc/default.xbe')
 records=json.loads((SRC/'original20-executed-shadow-draws.json').read_text())['records'][:7]
 assert tuple(r['event']for r in records)==EVENTS
 modes=[]
 for t in c.tags.values():
  if t['group']!='mode':continue
  try:ss=raw_model_streams(c,t['datum'],xbe)
  except StopIteration:continue
  for s in ss:modes.append((t,s,[read_stream(v)for v in s['streams']]))
 authored=json.loads((SRC/'authored-shadow-assets-21.json').read_text())['maps'][11]['programs'][0]
 _,vrtx=c.tag(int(authored['datum'],16),'vrtx');classes=c.block(vrtx+4,28)
 existing={}
 for line in (OUT/'retail_shadow_vertex.tsv').read_text().splitlines():
  f=line.split()
  if f and f[0]=='V':existing[(int(f[1]),int(f[2]))]=[int(v,16)for v in f[3:]]
 lines=['# Actual original20 generation DebugVertex intermediates, complete original bank and raw attributes.']
 proof=[];samples=0
 for r in records:
  e=r['event'];cap=(SRC/r['vertex_buffers'][0]['file']).read_bytes()
  matches=[(t,s,raws)for t,s,raws in modes if raws[0].startswith(cap)]
  assert len(matches)==1,(e,len(matches))
  t,s,raws=matches[0];root=t['address']-c.magic;section=c.block(root+0x24,0x5c)[s['section']]
  geometry_pointer,geometry_size=c.u32(section+0x38),c.u32(section+0x3c)
  raw_path=c.path if geometry_pointer>>30==0 else c.path.parent/('','mainmenu.map','shared.map','single_player_shared.map')[geometry_pointer>>30]
  with raw_path.open('rb')as f:f.seek(geometry_pointer&0x3fffffff);geometry=f.read(geometry_size)
  base=geometry_size-c.u32(section+0x44)-4;resources=c.block(section+0x48,16)
  def resource(kind,channel):
   rr=next(a for a in resources if c.u16(a+4)==kind and c.u16(a+6)==channel)
   return geometry[base+c.u32(rr+12):base+c.u32(rr+12)+c.u32(rr+8)]
  parts=resource(0,72);indices=resource(32,2)
  authored_indices=struct.unpack('<'+'H'*(len(indices)//2),indices)
  captured_indices=words((SRC/r['indices']['file']).read_bytes())
  pm=[]
  for part in range(len(parts)//72):
   kind,flags,material,first,count=struct.unpack_from('<5H',parts,part*72)
   if tuple(authored_indices[first:first+count])==captured_indices:pm.append((part,kind,flags,material,first,count))
  assert len(pm)==1,(e,pm)
  part,kind,flags,material,first,count=pm[0];shader=c.u32(c.block(root+0x60,32)[material]+12)
  class_at=classes[CLASS[e]];n=c.u32(class_at+4);decl=list(struct.unpack_from('<'+'H'*n,c.data,c.u32(class_at+8)-c.magic))
  semantic_sources={}
  for channel,stream in enumerate(s['streams']):
   table=bytes.fromhex(stream['declaration']);offset=0
   for at in range(1,21,2):
    semantic,fmt=table[at:at+2]
    if semantic==255:break
    if semantic==254:offset+=fmt;continue
    target=struct.unpack('<I',xbe.read(0x35888+fmt*4,4))[0]
    instruction=xbe.read(target,5);assert instruction[0]==0xb8
    width=struct.unpack_from('<I',instruction,1)[0]
    semantic_sources[semantic]=(channel,offset,width)
    offset+=width
  stream_proof=[];buffers={v['slot']:v for v in r['vertex_buffers']}
  for attribute in r['attributes']:
   register=int(attribute['name'][1:]);semantic=decl[register]
   channel,offset,width=semantic_sources[semantic];vb=buffers[attribute['vertexBuffer']]
   raw=(SRC/vb['file']).read_bytes();assert sha(raw)==vb['sha256']
   stride=vb['binding']['byteStride'];size=attribute['format']['compCount']*attribute['format']['compByteWidth']
   assert size==width,(e,semantic,size,width)
   vertex_count=len(raw)//stride
   captured=b''.join(raw[v*stride+attribute['byteOffset']:v*stride+attribute['byteOffset']+size]for v in range(vertex_count))
   source=b''.join(raws[channel][v*s['streams'][channel]['stride']+offset:v*s['streams'][channel]['stride']+offset+size]for v in range(vertex_count))
   assert captured==source,(e,register,semantic)
   stream_proof.append(dict(register=register,semantic=semantic,slot=vb['slot'],authored_channel=channel,authored_offset=offset,used_bytes=len(captured),sha256=sha(captured)))
  code=(SRC/next(v['file']for v in authored['variants']if v['class_index']==CLASS[e])).read_bytes()
  uv=[]
  for off in range(4,len(code),16):
   _,y,z,w=struct.unpack_from('<4I',code,off)
   if (y>>21&15)==4 and (y>>13&255)==172:
    assert w>>24&15==12 # .xy temporary write
    uv.append(f'R{w>>20&15}')
  assert len(uv)==1
  asm=(SRC/f'original20-{e}-0.spvasm').read_text();initial=asm[asm.index('  *oPos ='):];initial=initial[:initial.index('= _MUL(')]
  names=[]
  for name in re.findall(r'\*(\w+) =',initial):
   if name not in names:names.append(name)
  names=names[:names.index('R11')+1]
  uniform=(SRC/f'original20-{e}-0-uniform0.bin').read_bytes();conversion=(*words(uniform[5336:5344]),words(uniform[3076:3080])[0])
  lines.append(f'B {e} {CLASS[e]} {t["datum"]:08X} {s["section"]} {part} {shader:08X} '+text(conversion))
  for index in range(192):lines.append(f'C {e} {index} '+text(words(uniform[index*16:index*16+16])))
  observations=[]
  for d in r['debug']:
   assert d['valid'];values=list(d['final'].items());assert len(values)>len(names)
   mapping={name:values[i]for i,name in enumerate(names)}
   assert mapping['A0'][1]['columns']==1
   assert all(mapping[f'R{i}'][1]['columns']==4 for i in range(12))
   for index,name in enumerate(['oPos','oD0','oD1','oB0','oB1','oPts','oFog','oT0','oT1','oT2','oT3']):
    assert names[index]==name
   world=[int(v,16)for v in mapping['R0'][1]['words']]
   tex=[int(v,16)for v in mapping[uv[0]][1]['words']]
   old=existing[(e,d['index'])];assert len(old)==92
   lines.append(f'V {e} {d["index"]} '+text(old[:64]+world+tex+old[64:]))
   observations.append(dict(vertex=d['index'],world_private_id=mapping['R0'][0],uv_register=uv[0],uv_private_id=mapping[uv[0]][0],world_words=[f'{v:08X}'for v in world],uv_words=[f'{v:08X}'for v in tex]))
   samples+=1
  proof.append(dict(event=e,classification=CLASS[e],declaration=decl,program_sha256=sha(code),model=f'{t["datum"]:08X}',model_name=t['name'],section=s['section'],part=part,part_type=kind,part_flags=flags,shader=f'{shader:08X}',shader_name=c.tags[shader]['name'],authored_index_first=first,indices=count,index_sha256=sha((SRC/r['indices']['file']).read_bytes()),streams=stream_proof,observations=observations,initialization_order=names))
 # Full GPU postVS words for the independently corroborated section19.
 # Layout is retained original SPIR-V output order: position, D0/D1 and T0..3.
 r=next(r for r in records if r['event']==7737)
 raw=(SRC/r['postvs']['file']).read_bytes();assert sha(raw)==r['postvs']['sha256']
 stride=r['postvs_description']['vertexByteStride'];assert stride==240
 ii=words((SRC/r['indices']['file']).read_bytes());pi=words((SRC/r['postvs_indices']['file']).read_bytes())
 assert ii==pi and set(ii)==set(range(672))
 for vertex in range(672):
  w=words(raw[vertex*stride:vertex*stride+stride]);output=sum((list(w[at:at+4])for at in (0,8,12,28,32,36,40)),[])
  lines.append(f'S 7737 {vertex} '+text(output))
 # This later RAM bank is a separate actual input. Only independent matching
 # GPU palette words justify using it for a captured section in the test.
 ownerfile=ROOT/'analysis/multiplayer-shading/original-xemu/zanzibar-player20-unit.bin'
 owner=ownerfile.read_bytes();owner_definition=struct.unpack_from('<I',owner)[0];assert owner_definition==0xedf10c6b
 _,owner_def=c.tag(owner_definition,'bipd');owner_model=c.u32(owner_def+0x38);assert owner_model==0xedf20c6c
 _,owner_hlmt=c.tag(owner_model,'hlmt');assert c.u32(owner_hlmt+4)==0xe9b90844
 size,offset=struct.unpack_from('<HH',owner,0x114)
 assert size%52==0 and offset+size<=len(owner)
 lines.append(f'N E9B90844 {size//52} '+text(words(owner[offset:offset+size])))
 target=OUT/'retail_shadow_caster_world.tsv';target.write_text('\n'.join(lines)+'\n')
 result=dict(fixture=str(target.relative_to(ROOT)),fixture_sha256=file_sha(target),draws=proof,vertices=samples,source_capture_sha256=file_sha(SRC.parent/'original_capture_20.rdc'),cache_sha256=file_sha(c.path),source_export_sha256=file_sha(SRC/'original20-executed-shadow-draws.json'),later_owner_file=str(ownerfile.relative_to(ROOT)),later_owner_sha256=sha(owner),later_owner_definition=f'{owner_definition:08X}',later_owner_model=f'{owner_model:08X}',later_owner_render_model='E9B90844',complete_gpu_vertices=672,complete_gpu_postvs_sha256=r['postvs']['sha256'],tool_sha256=file_sha(Path(__file__)),scope=__doc__,boundary='The source raw programs are actual executed classes1/2/4/5; class2 and3 have identical compiled bytes. Class0 is an authored decoded WORLD variant, not an executed class0 caster draw. Paused unit nodes are postinterval and require separate per-section consumed palette equality. No native goldens or simultaneous whole-owner pose claim.')
 target.with_suffix('.provenance.json').write_text(json.dumps(result,indent=2)+'\n')
 print(json.dumps(dict(draws=7,vertices=samples,fixture_sha256=file_sha(target),identities=[(p['event'],p['model'],p['section'],p['part'])for p in proof])))
 c.close()
if __name__=='__main__':main()

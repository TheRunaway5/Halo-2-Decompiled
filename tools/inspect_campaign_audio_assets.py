#!/usr/bin/env python3
"""Read original compact snd!/lsnd metadata without changing native loaders."""
from pathlib import Path
import struct, hashlib, json, collections

root=Path(__file__).resolve().parents[1]
(root/'analysis/campaign').mkdir(parents=True,exist_ok=True)
classes=json.loads((root/'analysis/campaign/audio-research-retail.json').read_text())['classes']
result=[]
for cache_name in ['01a_tutorial','01b_spacestation']:
 path=root/f'assets/disc/maps/{cache_name}.map';data=path.read_bytes()
 def u32(offset):return struct.unpack_from('<I',data,offset)[0]
 def u16(offset):return struct.unpack_from('<H',data,offset)[0]
 def i16(offset):return struct.unpack_from('<h',data,offset)[0]
 def f32(offset):return struct.unpack_from('<f',data,offset)[0]
 def text(offset):return data[offset:data.index(b'\0',offset)].decode('utf-8')
 assert u32(0)==0x68656164 and u32(4)==8
 index=u32(16);count=u32(index+24)
 table=u32(index+8)-(u32(index)-(index+32))
 magic=u32(table+8)-(index+u32(20))
 names=u32(708);name_offsets=u32(716)
 tags=[]
 for i in range(count):
  p=table+i*16
  tags.append(dict(group=struct.pack('>I',u32(p)).decode('ascii'),datum=u32(p+4),address=u32(p+8),size=u32(p+12),name=text(names+u32(name_offsets+i*4))))
 tag_by_id={t['datum']:t for t in tags}
 sounds=[]
 for t in tags:
  if t['group']!='snd!' or not t['address']:continue
  p=t['address']-magic
  assert t['size']>=20 and p>=0 and p+20<=len(data)
  c=data[p+2];assert c<54
  sounds.append(dict(datum=f"{t['datum']:08x}",name=t['name'],flags=u16(p),sound_class=c,class_name=classes[c]['name'],rate=data[p+3],encoding=data[p+4],compression=data[p+5],playback_parameters=i16(p+6),custom_playback=data[p+13],definition_bytes=data[p:p+20].hex()))
 by_sound_id={int(t['datum'],16):t for t in sounds}
 samples=[t for t in sounds if any(s in t['name'].lower() for s in ['\\x02','\\x01','bridge','space_station','elevator'])]
 loops=[]
 for t in tags:
  if t['group']!='lsnd' or not t['address'] or not any(s in t['name'].lower() for s in ['\\x02','\\x01','spacestation','space_station','elevator','bridge']):continue
  p=t['address']-magic;n=u32(p+28);first=u32(p+32)-magic;tracks=[]
  for i in range(n):
   a=first+i*88
   phases={}
   for phase,offset in [('start',24),('loop',32),('stop',40),('alternate',48),('alternate_stop',56)]:
    datum=u32(a+offset)
    phases[phase]=by_sound_id.get(datum,{'datum':f'{datum:08x}'})
   tracks.append(dict(flags=u32(a+4),gain_db=f32(a+8),fade_in_seconds=f32(a+12),fade_out_seconds=f32(a+16),phases=phases))
  detail_count=u32(p+36);first_detail=u32(p+40)-magic;details=[]
  for i in range(detail_count):
   a=first_detail+i*52;datum=u32(a+8)
   details.append(dict(name_string_id=f'{u32(a):08x}',sound=by_sound_id.get(datum,{'datum':f'{datum:08x}'}),period_seconds=[f32(a+12),f32(a+16)],unknown14=f32(a+20),flags=u32(a+24),yaw_radians=[f32(a+28),f32(a+32)],pitch_radians=[f32(a+36),f32(a+40)],distance=[f32(a+44),f32(a+48)],definition_bytes=data[a:a+52].hex()))
  loops.append(dict(datum=f"{t['datum']:08x}",name=t['name'],flags=u32(p),tracks=tracks,details=details))
 scenario=tag_by_id[u32(index+12)];p=scenario['address']-magic
 objectives_id=u32(p+0x3bc);objectives=tag_by_id.get(objectives_id)
 objective_tag=None
 if objectives:
  a=objectives['address']-magic
  objective_tag=dict(datum=f'{objectives_id:08x}',name=objectives['name'],group=objectives['group'],definition_bytes=data[a:a+objectives['size']].hex())
 hist=collections.Counter(t['sound_class'] for t in sounds)
 result.append(dict(cache=cache_name,sha256=hashlib.sha256(data).hexdigest(),sound_count=len(sounds),class_histogram=[dict(index=i,name=classes[i]['name'],count=n) for i,n in sorted(hist.items())],sample_sounds=samples,looping_sounds=loops,objectives_tag=objective_tag))
out=root/'analysis/campaign/audio-research-assets.json'
out.write_text(json.dumps(result,indent=2)+'\n')
print('Wrote',out)

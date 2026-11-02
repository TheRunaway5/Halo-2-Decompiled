#!/usr/bin/env python3
"""Read-only census of authored imported-MCC BSP conveyor/material operands.

The offsets are the actual TU1.5 184C50/355D0 consumers. This reports candidate
authored definitions; it never changes assets, spawns entities, supplies physics
state, or claims that a candidate was reached by a live contact.
"""
import argparse,hashlib,json,os,struct
from pathlib import Path
from mcc_content import MccCache
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--mcc-root',required=True,type=Path);p.add_argument('--maps',nargs='+',default=['coagulation','elongation','colossus','cyclotron','zanzibar','headlong']);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),mcc_root=str(a.mcc_root.resolve()),assets_modified=False,physics_state_supplied=False,live_candidate_reached=False,maps=[],retained_large_artifacts=[],source_consumer_sha256={n:sha(ROOT/n)for n in('src/physics/native_tu15_surface_conveyor.cpp','src/physics/native_tu15_part_contact_response.cpp')})
 try:
  for name in a.maps:
   path=a.mcc_root/'halo2/h2_maps_win64_dx11'/(name+'.map');before=path.stat();row=dict(map=name,path=str(path.resolve()),encoded_sha256=sha(path),bsps=[]);cache=None
   try:
    cache=MccCache(path)
    for tag in cache.tags.values():
     if tag['group']!='sbsp'or tag['offset']is None:continue
     at=tag['offset'];materials=cache.block(at+0xc,20);vectors=cache.block(at+0xb4,24);bsp=dict(datum=tag['datum'],path=tag['name'],materials=len(materials),basis_count=len(vectors),conveyor_materials=[])
     for index,material in enumerate(materials):
      encoded=cache.u16(material+0xa)
      if encoded==0xffff:continue
      signed=struct.unpack('<h',struct.pack('<H',encoded))[0];shader=cache.u32(material+0x10);candidate=dict(material=index,basis_index=signed,basis_in_bounds=0<=signed<len(vectors),shader=shader)
      if 0<=signed<len(vectors):candidate['basis']=list(struct.unpack_from('<6f',cache.data,vectors[signed]))
      try:
       shader_tag,shader_at=cache.tag(shader,'shad');candidate['shader_path']=shader_tag['name'];definition=cache.pointer(cache.u32(shader_at+0x24),cache.magic,0x68);functions=cache.block(definition+0x3c,20);bindings=cache.block(definition+0x44,4);packed=cache.block(definition+0x4c,2);maps=cache.block(definition+0x5c,4);candidate.update(function_count=len(functions),binding_count=len(bindings),map_count=len(maps),eligible_maps=[])
       for map_index in range(min(2,len(maps))):
        entry=struct.unpack_from('<h',cache.data,maps[map_index]+2)[0]
        if entry<0:continue
        assert entry<len(packed),(name,'packed',entry,len(packed))
        value=cache.u16(packed[entry]);first=value&0x1ff;count=value>>9;channels=[]
        assert first+count<=len(bindings),(name,'binding',first,count,len(bindings))
        for binding in bindings[first:first+count]:
         function=struct.unpack_from('<h',cache.data,binding)[0];kind=cache.u16(binding+2)
         if kind not in(4,5):continue
         channel=dict(kind=kind,function_index=function)
         if 0<=function<len(functions):
          f=functions[function];channel['period']=struct.unpack_from('<f',cache.data,f+8)[0];size=cache.u32(f+12);pointer=cache.u32(f+16);channel['function_bytes']=size
          if size and pointer:blob=cache.pointer(pointer,cache.magic,min(size,20));channel['source_kind']=cache.data[blob];channel['function_header_hex']=bytes(cache.data[blob:blob+min(size,20)]).hex()
         channels.append(channel)
        if channels:candidate['eligible_maps'].append(dict(map_index=map_index,channels=channels))
      except Exception as e:candidate['shader_resolution_error']=str(e)
      bsp['conveyor_materials'].append(candidate)
     selected={c['material']:c for c in bsp['conveyor_materials']}
     for record_index,record in enumerate(cache.block(at+0x14,0x40)):
      surfaces=cache.block(record+0x28,8);edges=cache.block(record+0x30,12);vertices=cache.block(record+0x38,16);planes=cache.block(record+8,16)
      for surface_index,surface in enumerate(surfaces):
       material=cache.u16(surface+6)
       if material not in selected or cache.data[surface+4]&16:continue
       candidate=selected[material];candidate['collision_surface_count']=candidate.get('collision_surface_count',0)+1
       plane=cache.u16(surface);normal=list(struct.unpack_from('<3f',cache.data,planes[plane&0x7fff]));normal=[-x for x in normal]if plane&0x8000 else normal
       if normal[2]<0.5:continue
       candidate['walkable_surface_count']=candidate.get('walkable_surface_count',0)+1
       samples=candidate.setdefault('walkable_samples',[])
       if len(samples)>=4:continue
       first=cache.u16(surface+2);edge_index=first;polygon=[]
       while True:
        assert len(polygon)<=len(edges)and edge_index<len(edges)
        edge=edges[edge_index];left=cache.u16(edge+8)==surface_index;assert left or cache.u16(edge+10)==surface_index
        vertex=cache.u16(edge+(0 if left else 2));polygon.append(list(struct.unpack_from('<3f',cache.data,vertices[vertex])));edge_index=cache.u16(edge+(4 if left else 6))
        if edge_index==first:break
       samples.append(dict(record=record_index,surface=surface_index,normal=normal,centroid=[sum(v[n]for v in polygon)/len(polygon)for n in range(3)],minimum=[min(v[n]for v in polygon)for n in range(3)],maximum=[max(v[n]for v in polygon)for n in range(3)]))
     row['bsps'].append(bsp)
    scenario,scenario_at=cache.tag(cache.u32(cache.index+12),'scnr');row['scenario']=scenario['name'];row['player_starts']=[]
    for index,start in enumerate(cache.block(scenario_at+0x100,0x34)):
     row['player_starts'].append(dict(index=index,position=list(struct.unpack_from('<3f',cache.data,start)),facing=struct.unpack_from('<f',cache.data,start+12)[0],team=struct.unpack_from('<h',cache.data,start+16)[0],game_types=list(struct.unpack_from('<4h',cache.data,start+0x14))))
    row['authored_object_placements']=[]
    for kind,offset,stride in((6,0x50,0x5c),(1,0x70,0x54),(7,0xa8,0x48)):
     palette=cache.block(scenario_at+offset+8,0x28)
     for index,placement in enumerate(cache.block(scenario_at+offset,stride)):
      choice=struct.unpack_from('<h',cache.data,placement)[0]
      if choice<0 or choice>=len(palette):continue
      datum=cache.u32(palette[choice]+4);definition=cache.tags.get(datum)
      if definition is None:continue
      if not any(k in definition['name'].lower()for k in('conveyor','crate','box','belt')):continue
      row['authored_object_placements'].append(dict(type=kind,index=index,datum=datum,path=definition['name'],position=list(struct.unpack_from('<3f',cache.data,placement+8)),rotation=list(struct.unpack_from('<3f',cache.data,placement+0x14)),flags=cache.u32(placement+4)))
    row['local_object_candidates']=[dict(datum=t['datum'],group=t['group'],path=t['name'])for t in cache.tags.values()if t['group']in('scen','bloc','mach','vehi','phmo')and any(k in t['name'].lower()for k in('conveyor','crate','box','belt'))]
   except Exception as e:row['error']=str(e)
   finally:
    if cache is not None:cache.close()
   after=path.stat();assert(before.st_size,before.st_mtime_ns,before.st_ctime_ns)==(after.st_size,after.st_mtime_ns,after.st_ctime_ns);report['maps'].append(row)
  report['passed']=all('error'not in row for row in report['maps'])
 finally:
  report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());(a.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())

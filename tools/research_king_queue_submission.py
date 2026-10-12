#!/usr/bin/env python3
"""Whole original King world submission with a declared resident sort arena.

Actual Lockout shader/LOD metadata and initialized hill bytes are inputs.
The allocated112-byte arena, prior count/bytes, camera and optional shader flag
edits are explicit inputs. No allocator, shader-selection or game return is
supplied. This proves the submission record, not drain services or pixels.
"""
import argparse,hashlib,json,struct,sys
from pathlib import Path
from research_king_visual_consumers import *
OUT=ROOT/'analysis/multiplayer/game-variants/king-queue-submission-source-20261012'
REPORT=ROOT/'analysis/multiplayer/game-variants/king-visual-consumers-source-20261012/complete/report.json'
ARENA=0x6500000
def digest(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'complete/report.json');args=ap.parse_args()
 prior=json.loads(REPORT.read_text());assert digest(REPORT)=='5921a37a3dc5c5e4af9539bf770f3455885222e1d08505c78473e174b887288a'
 cache=HudCache(ROOT/'assets/disc/maps/lockout.map');traces={};entries={}
 for name in('retail','tu11','u5'):
  entries[name]={k:int(v,16)for k,v in prior['versions'][name]['registered_entries'].items()}
  GLOBALS[name].update({k:int(v,16)for k,v in prior['results'][name]['globals'].items()})
  traces[name]=VisualTrace(name,cache,entries[name])
 def address(t,key,old):
  if t.name=='retail':return old
  _,oi=literal(traces['retail'].code,entries['retail'][key],traces['retail'].starts)
  _,ni=literal(t.code,entries[t.name][key],t.starts)
  matches={int(v['later_address'],16)for v in references(oi,ni,traces['retail'].image,t.image)if int(v['old_address'],16)==old}
  assert len(matches)==1,(t.name,key,hex(old),matches);return matches.pop()
 inputs=[]
 for count in(0,1,1023,1024):
  for flags in(None,0,1,2,3):
   for camera in((1.,2.,3.,1.,0.,0.),(-17.25,8.125,5.5,.25,-.5,.75)):
    inputs.append(dict(prior_count=count,shader_flags=flags,camera=list(camera),marker_count=8))
 for marker_count in(-1,0,1,3,5):inputs.append(dict(prior_count=0,shader_flags=None,camera=[1.,2.,3.,1.,0.,0.],marker_count=marker_count))
 results={}
 for name,t in traces.items():
  t.g.mem_map(ARENA,0x20000)
  count_at=address(t,'sort_allocate',0x4b6280);arena_at=address(t,'sort_allocate',0x4b6288)
  camera_at=address(t,'sort_submission',0x485618)
  shader=int(next(x for x in prior['cache_inventory']if x['path'].endswith('lockout.map'))['hill_shader']['datum'],16)
  tag=cache.tags[shader];shader_at=cache.magic+tag['offset'];original_flags=struct.unpack('<H',bytes(t.g.mem_read(shader_at+0x16,2)))[0]
  rr=[]
  for index,row in enumerate(inputs):
   t.setup_players(dict(players=[dict(slot=0,team=0)],team_mode=False,local_slot=0,view=0))
   t.g.mem_write(K,bytes.fromhex(prior['geometry_hex']));t.w(K+0x60,row['marker_count'])
   t.w(count_at,row['prior_count']);t.w(arena_at,ARENA)
   notice_at=count_at+0x11;t.g.mem_write(notice_at,b'\x01')
   raw=bytes(((index*31+j*17)&255)for j in range(112));slot=ARENA+min(row['prior_count'],1023)*112;t.g.mem_write(slot,raw)
   t.g.mem_write(camera_at,struct.pack('<6f',*row['camera']))
   t.h(shader_at+0x16,original_flags if row['shader_flags']is None else row['shader_flags'])
   result=t.run_probe('world_submission',(0x9abc0000,))
   assert result['completion']=='complete_original_return',result
   rr.append(dict(**result,seed_hex=raw.hex(),record_hex=bytes(t.g.mem_read(slot,112)).hex(),
    count_after=t.u(count_at),overflow_notice_after=bytes(t.g.mem_read(notice_at,1))[0]))
  t.h(shader_at+0x16,original_flags)
  results[name]=dict(source_sha256=t.spec['pin'],entries=prior['versions'][name]['registered_entries'],
   count_address=f'{count_at:08x}',arena_pointer_address=f'{arena_at:08x}',camera_address=f'{camera_at:08x}',
   original_shader_flags=original_flags,cases=rr,
   executed=[dict(address=f'{pc:08x}',bytes=t.image.read(pc,t.code.at(pc).size).hex())for pc in sorted(t.executed)])
 deps=sorted({Path(m.__file__).resolve()for m in sys.modules.values()if getattr(m,'__file__',None)and Path(m.__file__).is_file()and Path(m.__file__).resolve().is_relative_to(ROOT/'tools')})
 report=dict(schema='king-queue-submission-source-v1',scope=__doc__,tool_sha256=digest(__file__),
  dependencies={str(p.relative_to(ROOT)):digest(p)for p in deps},visual_report_sha256=digest(REPORT),
  cache_sha256=digest(cache.path),geometry_hex=prior['geometry_hex'],shader=f'{shader:08x}',
  inputs=inputs,results=results,whole_calls=len(inputs)*3,instruction_patches=[],supplied_returns=[])
 args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
 print('whole original world submissions',report['whole_calls'],flush=True)
if __name__=='__main__':main()

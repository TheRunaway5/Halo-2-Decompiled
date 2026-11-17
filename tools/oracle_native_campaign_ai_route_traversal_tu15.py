#!/usr/bin/env python3
"""Physical unchanged-i386 reference for native segment and clearance traversal.

Independent authored navigation graphs, no patched instruction or supplied
callee result. Retains small compressed reference; raw banks use test scratch.
"""
import argparse,gzip,json,os,struct
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
import oracle_native_campaign_ai_area_perform_tu15 as reference
import oracle_native_campaign_ai_path_search_tu15 as search
import oracle_native_campaign_ai_route_tu15 as route
OWNED=(0x2029e0,0x276090,0x2762a0,0x2761f0,0x2769c0,0x2768f0,0x2cbaf0)
ENTRIES=OWNED+(0x202970,0x186e30,0x27a310)
STOP_ENTRIES=()
DATA=search.BASE+0x28000
START,DIRECTION,END,OUTPUT,EXCLUSIONS,OBSTACLES,AXIS=(DATA+n for n in (0,0x20,0x40,0x100,0x200,0x300,0x500))
def inputs(image):
 rows=[]
 def make(entry,name,*,flags=0x10,start=(1.,1.,0.),direction=(1.,0.,0.),extent=2.,sector=0,target=-1,radius=.25,obstacles=False,skipnav=False,adjust=False,profile=None,exclude=False,links=False,device=None):
  world,_=search.grid(3,2,size=4.,flags=flags,links=links)
  if device is not None:
   for cell in range(len(world.sectors)):
    world.link(cell,7,raw4=2,raw8=0xffffffff)
   world.put(search.DEVTABLE+2*8+4,8,1);world.put(search.DEVTABLE+2*8+5,3,1)
   world.put(search.DEVBITS+33,8 if device else 0)
  ws=search.Workspace(nodes=0,heap=0)
  data=bytearray([0xa5]*0x600)
  for at,xyz in [(START,start),(DIRECTION,direction),(END,tuple(a+b*extent for a,b in zip(start,direction))),(AXIS,(0.,0.,1.))]:
   for k,v in enumerate(xyz):route.put(data,at-DATA+4*k,search.f(v))
  route.put(data,EXCLUSIONS-DATA,1,2);route.put(data,EXCLUSIONS-DATA+2,1,2);route.put(data,EXCLUSIONS-DATA+4,0xffff,2);route.put(data,EXCLUSIONS-DATA+8,0)
  route.put(data,OBSTACLES-DATA+2,2,2)
  for n,(x,y,r) in enumerate(((2.,1.,.3),(5.,2.,.5))):
   at=OBSTACLES-DATA+12+n*20;route.put(data,at,n,1);route.put(data,at+2,10+n,2)
   for k,v in enumerate((x,y,r)):route.put(data,at+8+k*4,search.f(v))
  extra=[(DATA,data)]
  for at,size in [(0x4502c4,4),(0x46a6a0,8),(0x46a698,8),(0x46a430,8),(0x46a2c0,8),(0x46a838,4)]:extra.append((at,image.read(at,size)))
  search.make_case(image,rows,name,entry,ws,world,fp=profile,extra=tuple(extra),globals_={'gravity_vector':AXIS})
  row=rows[-1];row['gp'][6]=OUTPUT
  exc=EXCLUSIONS if exclude else 0
  if entry==0x2762a0:
   row['gp'][0]=sector&0xffffffff;args=[search.NAV,START,target&0xffffffff,DIRECTION,search.f(extent),exc]
  elif entry in (0x2761f0,0x2768f0):
   row['gp'][0]=START;row['gp'][1]=END;args=[search.NAV,sector&0xffffffff,target&0xffffffff,exc]
  elif entry==0x2769c0:
   args=[search.NAV,OBSTACLES if obstacles else 0,0xffffffff,START,sector&0xffffffff,target&0xffffffff,DIRECTION,search.f(radius),search.f(extent),int(skipnav),int(adjust),0,exc,OUTPUT]
  elif entry==0x2cbaf0:
   row['gp'][1]=OUTPUT;row['gp'][2]=int(adjust);row['gp'][3]=DIRECTION;row['gp'][7]=OBSTACLES;args=[0xffffffff,START,search.f(radius),search.f(extent)]
  else:raise ValueError(entry)
  row.update(matrix=args[0],source=args[1],stack_arguments=args[2:],stop_before_entries=[])
 for profile in search.FP_PROFILES:
  for entry in (0x2762a0,0x2761f0,0x2768f0):
   for extent in (0.,.01,1.,3.,5.,10.,20.):
    for direction in ((1.,0.,0.),(0.,1.,0.),(1.,.5,0.),(-1.,0.,0.)):
     make(entry,f'{entry:x}_d{direction}_t{extent}_fp{profile}',extent=extent,direction=direction,profile=profile)
  for entry in (0x2762a0,0x2769c0):
   for sector in (-1,0xffff,0,2):
    for target in (-1,0,1,3):make(entry,f'{entry:x}_s{sector}_g{target}_fp{profile}',sector=sector,target=target,profile=profile)
  for entry in (0x2769c0,0x2cbaf0):
   for radius in (0.,.25,2.):
    for extent in (1.,4.,8.):
     for adjust in (False,True):make(entry,f'{entry:x}_r{radius}_t{extent}_adj{adjust}_fp{profile}',radius=radius,extent=extent,adjust=adjust,obstacles=True,profile=profile)
 for flags in (0,1,2,3,0x10,0x11,0x13):
  for entry in (0x2762a0,0x2769c0):
   for exclude in (False,True):make(entry,f'{entry:x}_flags{flags}_exclude{exclude}',flags=flags,extent=8.,exclude=exclude)
 for profile in search.FP_PROFILES:
  for entry in (0x2762a0,0x2769c0):
   for flags in (0x10,0x11,0x13):
    for device in (None,False,True):
     for d in ((1.,0.,0.),(-1.,0.,0.),(0.,1.,0.)):
      make(entry,f'linked_{entry:x}_flags{flags}_device{device}_d{d}_fp{profile}',start=(5.,1.,0.),sector=1,extent=8.,direction=d,flags=flags,links=True,device=device,profile=profile)
 return rows

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--report', required=True, type=Path)
    parser.add_argument('--select')
    args = parser.parse_args()
    assert not args.output.exists()
    assert reference.sha(NEW_DEFAULT) == NEW_PIN
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    image = Xbe(NEW_DEFAULT)
    meta = reference.metadata(image, ENTRIES)
    owned = reference.prepare(image, meta, scratch)
    selected = inputs(image)
    if args.select:
        import re
        selected = [r for r in selected if re.search(args.select, r['name'])]
        for n, row in enumerate(selected):
            row['id'] = n
    rows = search.physical(owned, selected, 'complete')
    for row in rows:
        assert row['completed'] and row.get('stopped_at', search.STOP)==search.STOP, row['name']
    reached = set().union(*(set(row['executed_sites']) for row in rows))
    code = _Code(image, '.text')
    coverage = {hex(e): dict(reached=len(set(complete_body(code, e).code) & reached),
                            total=len(complete_body(code, e).code)) for e in ENTRIES}
    fixture = dict(image_sha256=NEW_PIN, backend='physical-unchanged-i386-offline', owners=meta['owners'],
                   instruction_patches=[], supplied_original_outcomes=False,
                   stop_entries=list(STOP_ENTRIES), bank_encoding='sparse-runs-v1',
                   write_encoding='rep-runs-v1', cases=reference.compact(rows))
    data = gzip.compress(json.dumps(fixture, separators=(',', ':')).encode(), mtime=0)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(data)
    report = dict(original_sha256=NEW_PIN, cases=len(rows), coverage=coverage,
                  reads=sum(len(r['reads']) for r in rows), stores=sum(len(r['writes']) for r in rows),
                  named_refusals={hex(e): sum(r.get('stopped_at') == e for r in rows) for e in STOP_ENTRIES},
                  fixture=str(args.output), fixture_bytes=len(data), fixture_sha256=reference.sha(args.output),
                  source_sha256={str(p.relative_to(reference.ROOT)): reference.sha(p) for p in
                                 (Path(__file__), Path(reference.__file__), Path(search.__file__))},
                  owned_scratch=str(scratch), retained_large_artifacts=[])
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report))


if __name__ == '__main__':
    main()

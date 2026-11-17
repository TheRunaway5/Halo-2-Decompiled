#!/usr/bin/env python3
"""Unchanged physical i386 corridor searches; no instruction or outcome substitution.

The declared bank is the complete corridor context, obstacle records, and input
points. Deep caller-private stack and dead volatile registers are not a public
contract; defined return bits, saved GP, FP and ordered retained writes are.
"""
import argparse,gzip,json,os,struct
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
import oracle_native_campaign_ai_area_perform_tu15 as reference
import oracle_native_campaign_ai_path_search_tu15 as search
import oracle_native_campaign_ai_route_tu15 as route
import oracle_native_campaign_ai_route_traversal_tu15 as traversal
OWNED=(0x2cdeb0,0x2cd780,0x2cdbc0,0x2cd870,0x2cd500,0x2cd3d0,0x2cd440,0x2cdb40,0x2cbc10,0x2caac0,0x2cba80)
ENTRIES=tuple(dict.fromkeys(OWNED+traversal.ENTRIES))
STOP_ENTRIES=()
DATA=search.BASE+0x28000
CONTEXT,OBSTACLES,START,TARGET,VECTOR,OUT,REQUEST=(DATA+n for n in (0,0x1000,0x1800,0x1820,0x1840,0x1900,0x1a00))
def inputs(image,baseline=False):
 rows=[]
 def make(entry,name,*,profile,obstacles=(),start=(1.,1.),target=(9.,1.),radius=.25,flags=0x11,sector=0,target_sector=2,last=0,retry=0,mode=0,threshold=None):
  world,_=search.grid(4,3,size=4.,flags=flags,portal=0x40)
  data=bytearray([0xa5]*0x2000)
  def put(at,v,width=4):route.put(data,at-DATA,v,width)
  def real(at,v):put(at,search.f(v))
  def xy(at,v):
   for k,x in enumerate(v):real(at+k*4,x)
  xy(START,(*start,0.));xy(TARGET,(*target,0.));xy(VECTOR,(.6,.8,0.))
  put(OBSTACLES,0,2);put(OBSTACLES+2,len(obstacles),2)
  for n,(x,y,r,identity) in enumerate(obstacles):
   at=OBSTACLES+12+n*20;put(at,0,1);put(at+2,identity,2);xy(at+8,(x,y,r))
  real(CONTEXT,radius);put(CONTEXT+4,0,1);put(CONTEXT+8,OBSTACLES);put(CONTEXT+12,search.DEVROOT)
  xy(CONTEXT+0x10,target);put(CONTEXT+0x18,target_sector);put(CONTEXT+0x1c,10 if obstacles else -1,2)
  for off in (0x1e,0x20,0xdae):put(CONTEXT+off,-1,2)
  real(CONTEXT+0x24,3.4028234663852886e38);put(CONTEXT+0x28,0,1);put(CONTEXT+0x29,retry,1)
  put(CONTEXT+0x2a,4,2);put(CONTEXT+0xd2c,4,2)
  for n in range(8):
   at=CONTEXT+0x2c+n*0x34;xy(at,(*start,));put(at+8,sector);xy(at+0xc,(1.,0.));real(at+0x14,abs(target[0]-start[0]))
   put(at+0x18,10 if n==0 and obstacles else -1,2);put(at+0x1a,0 if obstacles else -1,2);put(at+0x1c,0,1);put(at+0x1e,-1)
   real(at+0x24,float((7-n)*2));put(at+0x28,-1,2);put(at+0x2a,1,2);put(at+0x2c,0,2);put(at+0x2e,-1,2);put(at+0x30,0,1);put(CONTEXT+0xd2e+n*2,n,2)
  real(REQUEST+0x6c,(0. if last==0 else 100.) if threshold is None else threshold)
  args=[0,0];gp={};xmm={}
  if entry==0x2cdeb0:
   gp={0:CONTEXT,1:target_sector,2:sector,6:TARGET};args=[search.NAV,REQUEST,0,OBSTACLES,search.f(radius),START,last,retry]
  elif entry==0x2cd780:
   gp={0:search.DEVROOT,1:0,2:OBSTACLES,7:CONTEXT};args=[search.f(radius),START,sector,TARGET,target_sector,retry]
  elif entry==0x2cdbc0:
   gp={0:CONTEXT};args=[search.NAV,last];put(CONTEXT+0xd2c,mode,2)
  elif entry==0x2cd870:args=[search.NAV,CONTEXT,0,0]
  elif entry==0x2cd500:
   gp={1:CONTEXT,3:mode%2};args=[START,sector,10 if obstacles else -1,0 if obstacles else -1,search.f(2.5),mode//2-1]
   if mode==8:put(CONTEXT+0x2a,64,2);args[-1]=-1
   if mode>=10:
    args[-1]=1;args[2]=10;side=mode%2;gp[3]=side
    put(CONTEXT+0x2c+0x34+0x18,-1,2)
    put(CONTEXT+0x2c+0x34+0x1e+(0 if side else 2),2,2)
    xy(CONTEXT+0x2c+0x34+0xc,(1.,-.6 if mode<14 else .6))
    xy(CONTEXT+0x2c+2*0x34+0xc,(-1.,.3) if mode>=16 else (1.,.3))
   if mode in (6,7):
    put(CONTEXT+0x2c+2*0x34+0x18,10,2);put(CONTEXT+0x2c+2*0x34+0x28,0,2)
  elif entry==0x2cd3d0:gp={0:mode,1:CONTEXT}
  elif entry==0x2cd440:gp={0:CONTEXT,1:mode}
  elif entry==0x2cdb40:
   gp={0:mode,6:CONTEXT};args=[10]
   for n in range(1,4):put(CONTEXT+0x2c+n*0x34+0x28,n-1,2)
  elif entry==0x2cba80:gp={6:OBSTACLES,2:TARGET,3:mode-1};xmm={3:search.f(radius)}
  elif entry==0x2caac0:gp={0:VECTOR,1:OUT+0x10,2:OUT};args=[search.f(float(mode)),search.f(radius),OUT+0x20]
  elif entry==0x2cbc10:gp={6:START,7:OUT+0x20,1:OUT+0x10,2:OUT};args=[OBSTACLES,0,search.f(radius)]
  else:raise ValueError(entry)
  args=[v&0xffffffff for v in args]
  extra=[(DATA,data)]
  for at,size in [(0x46a4d4,4),(0x4502c4,4),(0x46a6a0,8),(0x46a698,8),(0x46a430,8),(0x46a2c0,8),(0x46a838,4)]:extra.append((at,image.read(at,size)))
  search.make_case(image,rows,name,entry,search.Workspace(nodes=0,heap=0),world,fp=profile,extra=extra,max_steps=100000)
  row=rows[-1]
  for n,v in gp.items():row['gp'][n]=v&0xffffffff
  for n,v in xmm.items():row['xmm'][n][0]=v
  row.update(matrix=args[0],source=args[1] if len(args)>1 else 0,stack_arguments=args[2:])
 for profile in search.FP_PROFILES:
  for entry in (0x2cd3d0,0x2cd440,0x2cdb40):
   for mode in (0,1,3,4):make(entry,f'helper_{entry:x}_m{mode}_fp{profile}',profile=profile,mode=mode)
  for entry in (0x2caac0,0x2cbc10,0x2cba80):
   for radius in (0.,.25,2.,10.):
    for mode in (0,1,3):make(entry,f'math_{entry:x}_r{radius}_m{mode}_fp{profile}',profile=profile,radius=radius,mode=mode,obstacles=((2.,1.,.5,10),),target=(2.,1.) if mode==0 else (7.,2.))
  for mode in (range(9) if baseline else (*range(9),10,11,14,15,16,17)):make(0x2cd500,f'append_m{mode}_fp{profile}',profile=profile,mode=mode,obstacles=((3.,1.,.5,10),))
  for entry in (0x2cd780,0x2cdbc0,0x2cd870,0x2cdeb0):
   for obstacles in ((),((3.,1.,.5,10),),((3.,1.,.5,10),(5.,1.,.5,11))):
    if entry==0x2cd870 and not obstacles:continue
    for target,sector in (((3.,1.),0),((9.,1.),2),((9.,5.),6)):
     make(entry,f'search_{entry:x}_o{len(obstacles)}_t{target}_fp{profile}',profile=profile,obstacles=obstacles,target=target,target_sector=sector,mode=1)
  for last in (0,1):
   for retry in (0,1):
    for target in ((3.,1.),(3.1,1.),(5.,1.)):
     make(0x2cdeb0,f'terminal_l{last}_retry{retry}_t{target}_fp{profile}',profile=profile,obstacles=((3.,1.,1.,10),),target=target,target_sector=0,last=last,retry=retry)
  if baseline:continue
  for entry in (0x2cbc10,0x2cd870,0x2cd500):
   make(entry,f'zero_direction_{entry:x}_fp{profile}',profile=profile,start=(3.,1.),target=(3.,1.),obstacles=((3.,1.,.5,10),),radius=.25)
  for entry in (0x2cdeb0,0x2cdbc0):
   for radius in (.05,.25,.5):
    for threshold in (0.,.1,100.):
     for target in ((3.,1.),(3.8,1.),(3.95,1.)):
      make(entry,f'margin_{entry:x}_r{radius}_threshold{threshold}_t{target}_fp{profile}',profile=profile,obstacles=((3.,1.,1.,10),),target=target,target_sector=0,radius=radius,last=1,threshold=threshold,mode=1)
 return rows

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--report', required=True, type=Path)
    parser.add_argument('--select')
    parser.add_argument('--baseline',action='store_true',help='Reproduce the original 612-case cohort')
    args = parser.parse_args()
    assert not args.output.exists()
    assert reference.sha(NEW_DEFAULT) == NEW_PIN
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    image = Xbe(NEW_DEFAULT)
    meta = reference.metadata(image, ENTRIES)
    owned = reference.prepare(image, meta, scratch)
    selected = inputs(image,baseline=args.baseline)
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

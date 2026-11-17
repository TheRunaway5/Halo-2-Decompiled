#!/usr/bin/env python3
"""Physical unchanged-i386 2CE030 smoothing and obstacle clustering.

Inputs declare real route rows and obstacle geometry. No patched instruction
or supplied child result. Generated observer/banks live in owned scratch.
"""
import argparse,copy,gzip,json,os,struct,subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
import oracle_native_campaign_ai_area_perform_tu15 as reference
import oracle_native_campaign_ai_path_search_tu15 as search
import oracle_native_campaign_ai_route_tu15 as route
import oracle_native_campaign_ai_route_obstacles_tu15 as obstacles
import oracle_native_campaign_ai_route_traversal_tu15 as traversal
OWNED=(0x2ce030,0x2cbe30,0x2cbcf0)
ENTRIES=tuple(dict.fromkeys(OWNED+obstacles.ENTRIES+traversal.ENTRIES+(0x32a660,0x30bd0,0x218780,0x218960,0x219030,0x145070,0x145200,0x218fe0,0x2cdeb0,0x2cd780,0x2cdbc0,0x2cd870,0x2cd500,0x2cd3d0,0x2cd440,0x2cdb40,0x2cbc10,0x2caac0,0x2cba80)))
STOP_ENTRIES=()
DATA=search.BASE+0x28000
ROWS,OUTPUT,COUNT,FLAG,OBJECT,KIND,FINAL,POOL,ACTORS,CONTROL,CACHE=(DATA+n for n in (0,0x200,0x400,0x404,0x408,0x40c,0x410,0x500,0x600,0x1000,0x60000))

def inputs(image):
 rows=[]
 def make(name,count=3,kind=2,already=0,actor=-1,actor_object=-1,actor_mode=0,policy=0,request_flags=0,request_controls=0,cache=False,cache_active=False,optional=True,fp=None):
  world,_=search.grid(4,2,size=4.,flags=0x11,portal=0x40)
  ws=search.Workspace(nodes=0,heap=0);ws.put(0x140b8,request_flags);ws.put(0x140bc,request_controls);ws.put(0x50,CACHE if cache else 0)
  data=bytearray([0xa5]*0x1100)
  for n in range(max(count,1)):
   at=n*28;route.put(data,at,kind,2);route.put(data,at+2,0,2);route.put(data,at+4,n);route.put(data,at+8,-1,2);route.point(data,at+12,(float(n+1),1.,0.),0xffff,upper=0x84ab)
  route.put(data,COUNT-DATA,already,2);route.put(data,FLAG-DATA,1,1)
  route.put(data,POOL-DATA+0x44,ACTORS);route.put(data,ACTORS-DATA+0x26c,actor_object);route.put(data,ACTORS-DATA+0x86,actor_mode,2)
  route.put(data,CONTROL-DATA+8,4);route.put(data,CONTROL-DATA+4,7)
  cached=bytearray([0xa5]*0x19300)
  if cache:route.put(cached,0x147b0,int(cache_active),1)
  extra=[(DATA,data),(0x5020c8,struct.pack('<I',POOL)),(0x47b49c,struct.pack('<I',CONTROL)),(0x47b908,struct.pack('<I',3)),(0x4fafc0,struct.pack('<I',3)),(0x56ae84,struct.pack('<I',0x200))]
  if cache:extra.append((CACHE,cached))
  for at in (0x46a16c,0x46a120,0x46a118):extra.append((at,image.read(at,4)))
  search.make_case(image,rows,name,0x2ce030,ws,world,fp=fp,extra=extra)
  row=rows[-1];row.update(matrix=actor&0xffffffff,source=search.W,stack_arguments=[count&0xffffffff,ROWS,policy,COUNT,OUTPUT,FLAG,OBJECT if optional else 0,KIND,FINAL if optional else 0])
  low=row['gp'][4]-0x1f98
  row['banks'].append(dict(address=low,before=search.history(low,0x1fc8,row['id']).hex()))
  row['implicit_saved_words']=[low+n*4 for n in range(4)]
  row['declared_caller_frame']=low
 for fp in search.FP_PROFILES:
  for count in (-1,0,1,2,3,4,6):
   for already in (0,3,4):make(f'rows_n{count}_used{already}_fp{fp}',count=count,already=already,fp=fp)
  for actor in (-1,route.ACTOR):
   for actor_object in (-1,0xe3610002):
    for policy in (0,1):
     for mode in (0,3):make(f'policy_actor{actor}_object{actor_object}_p{policy}_mode{mode}_fp{fp}',actor=actor,actor_object=actor_object,policy=policy,actor_mode=mode,request_flags=0x38,request_controls=0x18e3,fp=fp)
  for optional in (False,True):
   for active in (False,True):make(f'cache_optional{optional}_active{active}_fp{fp}',cache=True,cache_active=active,optional=optional,fp=fp)
 for fp in search.FP_PROFILES:
  for entry in (0x2cbe30,0x2cbcf0):
   for count in (0,1,2,3,31,32,33,64):
    for radius in (0.,.125,.5,2.):
     for gap in (.5,2.):
      if count>=31 and radius<2.:continue
      world,_=search.grid(1,1);ws=search.Workspace(nodes=0,heap=0);data=bytearray([0xa5]*0x1000)
      route.put(data,2,count,2)
      for n in range(count):
       at=12+n*20;route.put(data,at,1,2);route.put(data,at+2,-1,2);route.put(data,at+4,0xe3610000+n)
       for k,value in enumerate((n*gap,0.,.25)):route.put(data,at+8+k*4,search.f(value))
      search.make_case(image,rows,f'cluster_{entry:x}_n{count}_r{radius}_gap{gap}_fp{fp}',entry,ws,world,fp=fp,extra=((DATA,data),),max_steps=1000000)
      row=rows[-1];row['gp'][6]=DATA;row['gp'][3]=DATA+0x800;row['gp'][0]=0 if count else 0xffff;row['xmm'][3][0]=search.f(radius)
      row.update(matrix=search.f(radius) if entry==0x2cbe30 else DATA,source=0,stack_arguments=[])
      size=0x14 if entry==0x2cbe30 else 0x8c;low=row['gp'][4]-size
      row['banks'].append(dict(address=low,before=search.history(low,size+0xc,row['id']).hex()))
      row['implicit_saved_words']=[low,low+4,low+8] if entry==0x2cbcf0 else [low,low+4,low+8]
      row['declared_caller_frame']=low
 return rows

def composed_inputs(image):
 rows=[];request=search.BASE+0x40000
 def overlay(banks,address,data):
  end=address+len(data)
  for old,raw in list(banks.items()):
   stop=old+len(raw)
   if old<end and address<stop:
    del banks[old]
    if old<address:banks[old]=raw[:address-old]
    if stop>end:banks[end]=raw[end-old:]
  banks[address]=bytearray(data)
 for base in obstacles.inputs(image):
  label=base['name'].rsplit('_',1)[0]
  if label not in ('empty','base','range_miss','actor_unattached','device_open','device_closed','unit_dead','step','jump','climb'):continue
  for count in (1,4):
   for special in (False,True):
    row=copy.deepcopy(base);world,_=search.grid(4,2,size=4.,flags=0x11,portal=0x40)
    ws=search.Workspace(nodes=0,heap=0,full=True);ws.raw.extend(bytes(12));ws.put(0x140d0,-1);ws.point(0x14,(0.,0.,0.),0xffff);ws.put(0x8,0xea000002);ws.put(0xc,-1);ws.put(0x140b8,0);ws.put(0x140bc,0)
    ws.put(0x28,int(special),1);ws.put(0x44,int(special),1);ws.point(0x2c,(2.,.3,0.),0xffff);ws.put(0x38,0xea000001);ws.putf(0x3c,.5)
    banks={b['address']:bytearray.fromhex(b['before']) for b in row['banks']}
    overlay(banks,search.WORLD,world.pack());overlay(banks,request,ws.raw)
    data=bytearray([0xa5]*0x500)
    for n in range(count):
     at=n*28;route.put(data,at,-1,2);route.put(data,at+2,0,2);route.put(data,at+4,n);route.put(data,at+8,-1,2);route.point(data,at+12,(float(n+1)*2,1.,0.),0xffff,upper=0x9ab1)
    route.put(data,COUNT-DATA,0,2);route.put(data,FLAG-DATA,1,1);overlay(banks,DATA,data)
    for at,size in search.CONSTANTS+((0x46a118,4),(0x46a120,4),(0x46a16c,4),(0x4502c4,4),(0x46a6a0,8),(0x46a698,8),(0x46a430,8),(0x46a2c0,8),(0x46a838,4)):
     overlay(banks,at,image.read(at,size))
    low=row['gp'][4]-0x1f98;overlay(banks,low,search.history(low,0x1fc8,len(rows)))
    row.update(id=len(rows),name=f'composed_{label}_n{count}_special{special}_fp{base["before_fp"]}',entry=0x2ce030,
      matrix=base['matrix'],source=request,stack_arguments=[count,ROWS,0,COUNT,OUTPUT,FLAG,OBJECT,KIND,FINAL],
      banks=[dict(address=a,before=bytes(d).hex()) for a,d in sorted(banks.items())],
      implicit_saved_words=[low+n*4 for n in range(4)],declared_caller_frame=low,max_steps=1000000,
      compare_reads=False,strict_store_pcs=False)
    rows.append(row)
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
    for site in meta['sites']:
        if site['bytes'].startswith('0f17'):site['operands'][0].update(read=False,write=True)
    owned = reference.prepare(image, meta, scratch)
    # The private observer's diagnostic budget is enlarged for64-node
    # cluster/corridor loops; every original byte and recorded event is unchanged.
    observer_source=reference.ROOT/'tools/observe_native_campaign_ai_vector_math_tu15.cpp'
    private_observer=scratch/'bounded-smoothing-observer.cpp'
    private_observer.write_text(observer_source.read_text().replace('max_steps<=100000u','max_steps<=1000000u'))
    subprocess.run(['c++','-std=c++20','-O2',str(private_observer),'-o',str(owned/'observer')],check=True)
    selected = inputs(image) + composed_inputs(image)
    for n,row in enumerate(selected):row['id']=n
    if args.select:
        import re
        selected = [r for r in selected if re.search(args.select, r['name'])]
        for n, row in enumerate(selected):
            row['id'] = n
    rows = search.physical(owned, selected, 'complete')
    for row in rows:
        assert row['completed'], row['name']
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
                  owned_scratch=str(scratch), retained_large_artifacts=[],observer_instruction_limit=1000000,
                  private_observer_sha256=reference.sha(private_observer),observer_source_sha256=reference.sha(observer_source))
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report))


if __name__ == '__main__':
    main()

#!/usr/bin/env python3
"""Physical unchanged2FA9A0/2FAB70/3128C0 and real collector comparison.

Only before DATA, genuine clock inputs and caller FP controls are supplied.
All retained bytes, original ordered operands/stores and FP are compared.
Run through run_test.py; builds/raw fixtures remain in its owned scratch.
"""
import argparse,copy,hashlib,json,os,random,shutil,struct,subprocess,sys
from pathlib import Path
from capstone import CS_AC_READ,CS_AC_WRITE
from capstone.x86_const import X86_OP_MEM
import validate_native_island_agent_phase_tu15 as physical
import validate_native_sphere_overlap_tu15 as sphere
import validate_native_wrapped_contact_query_tu15 as wrapped
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
BASE=0x1000000
FIRST,SECOND,MS,MC,S,C,COLLECTOR,TABLE=[BASE+n for n in (0x100,0x200,0x300,0x400,0x500,0x600,0x700,0x800)]
BACKEND='unchanged-tu15-sphere-capsule-overlap-v1'
OWNERS=(0x2fab70,0x2fa9a0,0x3128c0,0x30b960,0x30ba80,0x2f0dd0,0x37ea60)
UNITS=('native_tu15_convex_overlap_query.cpp','native_tu15_inverse_geometry.cpp','native_tu15_geometry.cpp','native_tu15_wrapped_contact_query.cpp','native_tu15_contact_pair_collectors.cpp')
def sha(p):
 with Path(p).open('rb')as f:return hashlib.file_digest(f,'sha256').hexdigest()
def save(p,j):p.write_text(json.dumps(j,indent=2)+'\n')
def metadata(image):
 code=_Code(image,'.text');regs={n:i for i,n in enumerate(('eax','ecx','edx','ebx','esp','ebp','esi','edi'))};sites={};counts={};pins={}
 for owner in OWNERS:
  body=complete_body(code,owner);counts[hex(owner)]=len(body.code);listing=[]
  for pc in sorted(body.code):
   i=code.at(pc);listing.append(f'{pc:x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n');ops=[]
   for o in i.operands:
    if o.type!=X86_OP_MEM or i.mnemonic=='lea':continue
    if o.mem.segment:assert i.reg_name(o.mem.segment)=='fs';continue
    access=CS_AC_WRITE if i.mnemonic in('fst','fstp')else o.access
    if i.mnemonic in('cmp','test'):access|=CS_AC_READ
    ops.append(dict(base=regs.get(i.reg_name(o.mem.base),-1),index=regs.get(i.reg_name(o.mem.index),-1),scale=o.mem.scale,displacement=o.mem.disp&0xffffffff,bytes=o.size,read=bool(access&CS_AC_READ),write=bool(access&CS_AC_WRITE)))
   sites[pc]=dict(instruction=pc,operands=ops,timestamp=i.mnemonic=='rdtsc')
  pins[hex(owner)]=hashlib.sha256(''.join(listing).encode()).hexdigest()
 assert [counts[hex(x)]for x in OWNERS[:5]]==[25,107,72,8,2],counts
 word=lambda p:int.from_bytes(image.read(p,4),'little')
 assert word(0x46bc34)==0x19930520 and word(0x46bc38)==1
 action=word(0x46bc3c);assert word(action)==0xffffffff and word(action+4)==0x37ea60
 return dict(sites=list(sites.values()),body_sites=counts,listing_sha256=pins,regions=[],boundaries=[])
def cases(image):
 rows=[]
 def add(name,reverse=False,distance=0x3f800000,z=0,profile=False,fp=None,changes=(),seed=False,degenerate=False):
  rng=random.Random(len(rows));p=physical.SP-(0x70 if reverse else 0x50);closest=p-0x30;local=physical.SP-0x18
  banks={BASE:bytearray(rng.randbytes(0x1000)),0x48c974:bytearray(8),0x46a0e8:bytearray(image.read(0x46a0e8,4)),0x41ca40:bytearray(image.read(0x41ca40,16)),p+12:bytearray(rng.randbytes(52)),closest+8:bytearray(rng.randbytes(8))}
  if reverse:banks[local]=bytearray(rng.randbytes(12))
  def put(p,n):
   lo=next(lo for lo,b in banks.items()if lo<=p and p+4<=lo+len(b));struct.pack_into('<I',banks[lo],p-lo,n&0xffffffff)
  def vec(p,v):
   for i,f in enumerate(v):put(p+i*4,struct.unpack('<I',struct.pack('<f',f))[0])
  for a,n in ((FIRST,C if reverse else S),(FIRST+8,MC if reverse else MS),(SECOND,S if reverse else C),(SECOND+8,MS if reverse else MC),(S+12,0x3f800000),(C+12,0x3f800000),(COLLECTOR,TABLE),(TABLE+4,0x30ba80),(0x48c974,BASE+0xe00 if profile else 0),(0x48c978,BASE+0xe80 if profile else 0)):put(a,n)
  for o,v in ((0x20,[1,0,0,0]),(0x30,[0,1,0,0]),(0x40,[0,0,1,0]),(0x50,[0,0,0,1])):vec(MC+o,v)
  vec(MS+0x50,[0,0,0,1]);put(MS+0x50,distance);put(MS+0x58,z)
  vec(C+16,[0,0,-1,0]);vec(C+32,[0,0,-1 if degenerate else 1,0])
  for a,n in changes:put(a,n)
  rows.append(dict(name=('fault_seed_'if seed else'')+('reverse_'if reverse else'direct_')+name,kind='reverse'if reverse else'direct',entry=0x2fab70 if reverse else 0x2fa9a0,receiver=0xdeadbeef,arguments=[FIRST,SECOND,0xbad0bad0,COLLECTOR],first=FIRST,second=SECOND,collector=COLLECTOR,data=p,closest_point=closest,local_collector=local,configuration=0xbad0bad0,banks=[dict(address=a,before=b.hex())for a,b in sorted(banks.items())],before_fp=fp or[0x27f,0x453f,0x1f80],stop_pc=physical.STOP,boundary=None,declared_boundary=None))
 for reverse in(False,True):
  for precision in(0,2,3):
   for rounding in range(4):
    for sse in(0,0x40,0x8000,0x8040):
     for distance in(0x3f800000,0x40000000,0x40400000):add(f'fp_{precision}_{rounding}_{sse}_{distance:x}',reverse,distance,fp=[0x7f|(precision<<8)|(rounding<<10),0x453f,0x1f80|(rounding<<13)|sse])
  for z in(0xc0400000,0xbf800000,0,0x3f800000,0x40400000):add(f'endpoint_{z:x}',reverse,z=z)
  add('degenerate',reverse,degenerate=True)
  for position in(MS+0x50,C+16,C+28,MC+0x50,S+12,C+12):
   for value in(0x80000000,1,0x7f800000,0xff800000,0x7fc12345,0x7f812345):add(f'edge_{position:x}_{value:x}',reverse,changes=((position,value),))
  # Actual squared-distance strict-boundary controls, with a deliberately invalid unused collector.
  for distance in(0x40000000,0x40400000,0x7fc12345):add(f'no_collect_{distance:x}',reverse,distance,changes=((COLLECTOR,0),))
  for profile in(False,True):
   for distance in(0,0x3f800000,0x40000000,0x40400000):add(f'profile_{profile}_{distance:x}',reverse,distance,profile=profile)
  add('affine',reverse,changes=((MC+0x20,0),(MC+0x24,0x3f800000),(MC+0x30,0xbf800000),(MC+0x34,0),(MC+0x50,0x3f000001),(MC+0x54,0x3e000001)))
  add('overlap',reverse,seed=True);add('profile_overlap',reverse,profile=True,seed=True)
 return rows

def observer_source():
 wrapped.ORIGINAL_OBSERVER=sphere.observer_source
 return wrapped.observer_source().replace('0x37e878u','0x37ea68u').replace('0x37e870u','0x37ea60u')

def driver_source():
 text=(ROOT/'tests/native_tu15_sphere_overlap_tests.cpp').read_text();text=text.replace('#include "halo2/native_tu15_convex_overlap_query.hpp"','#include "halo2/native_tu15_convex_overlap_query.hpp"\n#include "halo2/native_tu15_contact_pair_collectors.hpp"').replace('unchanged-tu15-sphere-overlap-v1',BACKEND)
 text=text.replace('native_tu15_query_sphere_overlap(m,{1},{2},{3},{0x1000u},fp,depth,services);','native_tu15_query_sphere_capsule_overlap(m,{1},{2},{3},{0x1000u,0x1100u,0x1200u},fp,depth,services);')
 start=text.index('  services.collect=');end=text.index('  const auto cw=',start)
 text=text[:start]+'''  services.collect=[&](auto receiver,auto owner,auto a,auto b,auto&state){
   if(owner.identity==0x30b960u){NativeTu15ContactPairCollectorServices next;next.emit_pair=[&](auto target,auto method,auto first,auto second,auto&context){services.collect(target,method,first,second,context);};native_tu15_forward_reversed_contact_pair(m,receiver,a,b,state,next);return;}
   check(receiver.identity==c.at("collector")&&owner.identity==0x30ba80u&&a.identity==c.at("first")&&b.identity==c.at("second"),name+" collector ABI differs");native_tu15_mark_contact_query_hit(m,receiver,observe);
  };
''' +text[end:]
 a='binding.query({0xdeadbeefu},{0x2f9ca0u},{c.at("first")},{c.at("second")},{0xbad0bad0u},{c.at("collector")},fp);';b='binding.query({0xdeadbeefu},{c.at("entry")},{c.at("first")},{c.at("second")},{0xbad0bad0u},{c.at("collector")},fp);';assert a in text;text=text.replace(a,b).replace('c.at("data").get<std::uint32_t>()-16u','c.at("data").get<std::uint32_t>()-0x40u')
 a='native_tu15_query_sphere_overlap(m,{c.at("first")},{c.at("second")},{c.at("collector")},{c.at("data")},fp,0,services,observe);';b='''if(c.at("kind")=="reverse")native_tu15_query_reversed_sphere_capsule_overlap(m,{c.at("first")},{c.at("second")},{c.at("collector")},{c.at("data"),c.at("closest_point"),c.at("local_collector")},fp,0,services,observe);
   else native_tu15_query_sphere_capsule_overlap(m,{c.at("first")},{c.at("second")},{c.at("collector")},{c.at("data"),c.at("closest_point"),c.at("local_collector")},fp,0,services,observe);''';assert a in text;text=text.replace(a,b)
 # Binding unknown-owner controls must refuse before any source read or store.
 text=text.replace('name+" store count differs"','name+" store count differs native="+std::to_string(stores.size())+" original="+std::to_string(expected.size())+" expected="+expected.dump()')
 text=text.replace(' for(unsigned depth=1;', '''#if defined(HALO2_SPHERE_QUERY_BINDING)
 for(auto owner:{0u,0x2fab71u,0x2fa9a1u,0xffffffffu}){Memory m;m.log_reads=true;NativeTu15PhysicsFrameFp fp{0x27fu,0x453fu,0x1f80u};NativeTu15ConvexOverlapQueryServices services;NativeTu15OverlapQueryBinding binding(m,{0x1000u,512u},services);bool refused{};try{binding.query({1},{owner},{2},{3},{4},{5},fp);}catch(const std::runtime_error&){refused=true;}check(refused&&m.reads.empty()&&m.banks.empty(),"Unknown overlap owner entered DATA");}
#endif
 for(unsigned depth=1;''')
 return text

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True);ap.add_argument('--no-prefixes',action='store_true');a=ap.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,scope=__doc__,owned_scratch=str(scratch),retained_large_artifacts=[],outcomes_supplied=False)
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);meta=metadata(image);physical.observer_source=observer_source;physical.boundary=lambda c,d:None
  rows=physical.physical(image,meta,cases(image),scratch);assert all(c["boundary_gp"][4]==physical.SP+4 for c in rows),"Original cdecl4 query must RET4 only; its parent owns POP16";prefixes=[];code=_Code(image,'.text')
  if not a.no_prefixes:
   for seed in(c for c in rows if c['name'].startswith('fault_seed_')):
    for category in('reads','writes'):
     seen=set();occurrences={}
     for e in seed[category]:
      key=e['address'],e['bytes']if category=='reads'else len(e['written'])//2;occurrences[key]=occurrences.get(key,0)+1
      if e['instruction']in seen:continue
      seen.add(e['instruction']);c=copy.deepcopy(seed);c.update(name=seed['name']+'_'+category+'_'+hex(e['instruction']),stop_pc=e['instruction'],compose_unwind=seed['kind']=='reverse',fault=dict(address=key[0],bytes=key[1],occurrence=occurrences[key],access='read'if category=='reads'else'write'));prefixes.append(c)
    seen=set()
    for n,e in enumerate(seed['writes']):
     if e['instruction']in seen:continue
     seen.add(e['instruction']);c=copy.deepcopy(seed);c.update(name=seed['name']+'_observer_'+str(n),stop_pc=e['instruction']+code.at(e['instruction']).size,compose_unwind=seed['kind']=='reverse',observer_fault=n+1);prefixes.append(c)
   for c in prefixes:rows+=physical.physical(image,dict(meta,compose_unwind=c.get("compose_unwind",False)),[c],scratch)
  fixture=scratch/'original.json';save(fixture,dict(image_sha256=NEW_PIN,required_backend=BACKEND,cases=rows));project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include');pins={}
  for n in(*UNITS,'native_tu15_contact_unwind_internal.hpp'):
   src=ROOT/'src/physics'/n;shutil.copyfile(src,project/n);pins[str(src.relative_to(ROOT))]=sha(src)
  (project/'driver.cpp').write_text(driver_source());source=(project/UNITS[0]).read_text();start=source.index('void native_tu15_query_sphere_capsule_overlap(');end=source.index('void native_tu15_query_reversed_sphere_capsule_overlap(',start);prefix,body,suffix=source[:start],source[start:end],source[end:]
  mutations={'include_equal':('if(std::popcount(unsigned((relation>>8u)&5u))&1u){','if((relation&0x4000u)||(std::popcount(unsigned((relation>>8u)&5u))&1u)){'),'wrong_radius':('radius_b=m.word(second_shape+12u)','radius_b=m.word(second_shape+8u)'),'wrong_endpoint':('second_shape+16u+n*16u','second_shape+16u+n*0u'),'missing_collect':('services.collect(collector,owner,first,second,fp);','(void)collector;(void)owner;'),'wrong_profile_stride':('0x48c974u,cursor+12u','0x48c974u,cursor+8u')}
  for name,(before,after)in mutations.items():assert body.count(before)==1,(name,body.count(before));(project/(name+'.cpp')).write_text(prefix+body.replace(before,after)+suffix)
  (project/'negative.py').write_text('import subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nassert r.returncode!=0 and any(s in r.stderr for s in ("differs","differ")),r.stderr\nprint("Original sphere/capsule query rejects mutation",r.stderr.strip())\n')
  cmake='cmake_minimum_required(VERSION 3.20)\nproject(sphere_capsule_query LANGUAGES CXX)\ninclude(CTest)\n'
  targets=[]
  for name in('direct','binding',*mutations):
   target='sphere_capsule_'+name;targets.append(target);unit=UNITS[0]if name in('direct','binding')else name+'.cpp';cmake+=f'add_executable({target} driver.cpp {unit} '+ ' '.join(UNITS[1:])+f')\ntarget_include_directories({target} PRIVATE include)\ntarget_compile_features({target} PRIVATE cxx_std_20)\ntarget_compile_options({target} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n'
   if name=='binding':cmake+=f'target_compile_definitions({target} PRIVATE HALO2_SPHERE_QUERY_BINDING=1)\n'
   command=''if name in('direct','binding')else f'"{sys.executable}" "{project}/negative.py" ';cmake+=f'add_test(NAME {target} COMMAND {command}$<TARGET_FILE:{target}> "{fixture}")\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in('g++','clang++'):
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','2','--test-jobs','1',*sum((['--target',t]for t in targets),[]),'--filter','^sphere_capsule_'];r=subprocess.run(command,capture_output=True,text=True);(a.output/(compiler+'-runner.log')).write_text(r.stdout+r.stderr);results.append(dict(compiler=compiler,exit_code=r.returncode));assert r.returncode==0,r.stdout[-1800:]+r.stderr[-1800:]
  for n in('include/halo2/native_tu15_convex_overlap_query.hpp','include/halo2/native_tu15_overlap_query_binding.hpp','tests/native_tu15_sphere_overlap_tests.cpp','tools/validate_native_sphere_capsule_overlap_tu15.py','tools/validate_native_island_agent_phase_tu15.py','tools/validate_native_sphere_overlap_tu15.py','tools/validate_native_wrapped_contact_query_tu15.py','tools/observe_native_constraint_step_tu15.cpp'):pins[n]=sha(project/n)if(project/n).is_file()else sha(ROOT/n)
  assert all(sha(ROOT/n)==h for n,h in pins.items());visited={pc for c in rows for pc in c['visited']}
  report.update(passed=True,original_sha256=NEW_PIN,listing_sha256=meta['listing_sha256'],body_coverage={hex(n):dict(covered=sum(pc in visited for pc in complete_body(code,n).code),sites=meta['body_sites'][hex(n)])for n in OWNERS},cases_per_compiler=len(rows),fault_prefixes=sum('fault'in c for c in rows),observer_fault_prefixes=sum('observer_fault'in c for c in rows),genuine_clock_inputs=sum(len(c['timestamps'])for c in rows),negative_controls=list(mutations),compiler_results=results,source_hashes=pins,full_live_world_qualification=False,physical_instructions=sum(c['original_instructions']for c in rows))
 except BaseException as e:report['error']=str(e);raise
 finally:save(a.output/'report.json',report)
 return 0
if __name__=='__main__':raise SystemExit(main())

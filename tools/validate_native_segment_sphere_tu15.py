#!/usr/bin/env python3
"""Private complete120FA0 segment sphere against unchangedi386."""
import argparse,copy,hashlib,json,os,random,struct,subprocess,sys
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe
from validate_native_tu15_prt_helpers import PIN
from native_mopp_ray_dense_original import physical_original_dense
REGIONS=((0x61000000,0x4000),(0x62000000,0x1000))
FILES=('tools/native_segment_sphere_mutations.py','include/halo2/native_tu15_segment_sphere_value.hpp','tools/native_segment_sphere_replacements.py','tools/native_segment_sphere_inputs.py','tests/native_tu15_segment_sphere_reference.inc','src/physics/native_tu15_camera_query_leaves.cpp','src/physics/physics_live_support_tu15_generated.cpp','src/campaign/ai/source_ai_frame_tu15_generated.cpp','tools/specialize_native_operands.py','include/halo2/native_tu15_segment_sphere_kernel.hpp','src/physics/runtime/native_tu15_segment_sphere.hpp','tests/native_tu15_segment_sphere_tests.cpp','tools/validate_native_segment_sphere_tu15.py','src/physics/runtime/native_tu15_mopp_ray_caller_frame.hpp','src/physics/runtime/native_tu15_raw_x87_scope.hpp','src/physics/runtime/native_tu15_sse_mmx_scope.hpp','src/physics/runtime/physics_contact_runtime_internal.hpp','src/physics/runtime/physics_world_runtime_internal.hpp','tools/native_mopp_ray_dense_original.py','tools/validate_native_tu15_prt_helpers.py','tools/observe_native_campaign_ai_vector_math_tu15.cpp')
def hashes():return {name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest()for name in FILES}
def corpus(smoke):
 rng=random.Random(0x120fa0);rows=[];live=('0000000000000080ff3f','00000000000000000080','35c26821a2da0fc90040','45230100000000c0ff7f','00000000000000800040','00000000000000c0ff3f','00000000000000000000','0000000000000080ffbf')
 f32=lambda n:struct.unpack('<I',struct.pack('<f',n))[0]
 z=(0,0,0);p=(0x40000000,0,0);geometries=[(z,p,(0xc0400000,0,0),0x3f800000),(z,p,(0x40400000,0,0),0x3f800000),(z,z,z,0x3f800000),(z,p,(0xbf800000,0x3f800000,0),0x3f800000),(z,p,(0xbf800000,0,0),0x3f800000),(z,p,z,0x3f800000),(z,p,(0xc0000000,0,0),0x3f800000)]
 if not smoke:
  for value in(0,0x80000000,1,0x80000001,0x00800000,0x7f7fffff,0xff7fffff,0x7f800000,0xff800000,0x7fc12345,0xffc54321,0x7f812345,0xff854321):geometries.append(((value,0x3f000000,0xbe800000),(0x3f800000,value,0x40000000),(0xbf800000,0,value),value))
 for family,(first_values,center_values,direction_values,radius)in enumerate(geometries):
  for variant in range(1 if smoke else 10):
   n=len(rows);sp=0x62000800+n%16;first=0x61000100;center=0x61000200;direction=0x61000300
   if variant==4:direction=sp-12
   if variant==5:direction=sp-8
   if variant==6:first=sp-12
   if variant==7:center=sp-12
   banks={base:bytearray(rng.randbytes(size))for base,size in REGIONS}
   def put(p,value):
    base=next(base for base,size in REGIONS if base<=p and p+4<=base+size);banks[base][p-base:p-base+4]=int(value).to_bytes(4,'little')
   for address,values in((first,first_values),(center,center_values),(direction,direction_values)):
    for k,v in enumerate(values):put(address+k*4,v)
   if variant in(8,9):put(center+4,0x7fc12345 if variant==8 else 0x7f812345);put(first+4,0x7fc54321 if variant==8 else 0x7f854321)
   put(sp,0x63000000);put(sp+4,radius);put(sp+8,0)
   gp=[rng.getrandbits(32)for _ in range(8)];gp[0]=first;gp[1]=center;gp[2]=direction;gp[4]=sp
   rows.append(dict(id=n,name=f'family{family}_variant{variant}',phase='normal',entry=0x120fa0,gp=gp,eflags=0x202|(rng.getrandbits(12)&0xcd5),xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],live_x87=list(live[:(0,1,4,8)[n%4]]),before_fp=[(0x7f,0x27f,0x37f)[n%3]|((n%4)<<10),((n%8)<<11)|(n&0x473f),0x1f80|((n%4)<<13)|(0,0x40,0x8000,0x8040)[n%4]],banks=[dict(address=base,before=bytes(data).hex())for base,data in banks.items()],matrix=radius,source=0,max_steps=500))
 return rows

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--xbe',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--source-copies',action='store_true');p.add_argument('--smoke',action='store_true');p.add_argument('--mmx',action='store_true');p.add_argument('--require-binding',action='store_true');p.add_argument('--semantic-mutants',action='store_true');p.add_argument('--compiler',action='append');p.add_argument('--sanitizers',action='store_true');args=p.parse_args()
 if not args.xbe.is_file():return 77
 scratch=Path(os.environ['HALO2_TEST_SCRATCH']);args.output.mkdir(parents=True,exist_ok=False);report=dict(passed=False,production_binding=args.require_binding,source_before=hashes(),owned_scratch=str(scratch),retained_large_artifacts=[])
 try:
  if args.source_copies:
   from native_segment_sphere_inputs import prepare_public,prepare_source
   report['public_source']=prepare_public(ROOT,scratch);report['source_binding']=prepare_source(ROOT,scratch,args.require_binding)
  assert hashlib.sha256(args.xbe.read_bytes()).hexdigest()==PIN;image=Xbe(args.xbe)
  import oracle_native_campaign_screen_lifecycle_tu15 as common
  common.ENTRIES=(0x120fa0,);original=common.metadata;code=common._Code(image,'.text')
  def metadata(image):
   result=original(image)
   for site in result['sites']:
    if code.at(site['instruction']).mnemonic in('pop','ret'):site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
   return result
  common.metadata=metadata;meta=metadata(image);sites=meta['sites'];assert len(sites)==76
  io=[site['instruction']for site in sites if site['operands']or code.at(site['instruction']).mnemonic=='push'];traces=[dict(entry=pc,gp=True,xmm=True)for pc in io];rows=corpus(args.smoke)
  observed,authority=physical_original_dense(image,rows,scratch,entries=(0x120fa0,),regions=REGIONS,trace_calls=traces,trace_extended=True,preserve_x87_prefix=True)
  prefixes=[];normal_cases=len(rows);prefix_authority=None
  if not args.smoke:
   for row,actual in zip(rows,observed['cases'],strict=True):
    if not(row['name'].endswith(('variant0','variant4','variant5','variant6','variant7'))and int(row['name'].split('_')[0][6:])<7):continue
    for phase,events in(('before_read',actual['reads']),('before_write',actual['writes']),('after_observer',actual['writes'])):
     for ordinal,event in enumerate(events):
      prefix=copy.deepcopy(row);prefix.update(id=len(rows)+len(prefixes),name=row['name']+f'_{phase}_{ordinal}',phase=phase,native_fault_ordinal=ordinal,fault=dict(phase='after_write'if phase=='after_observer'else phase,ordinal=ordinal));prefixes.append(prefix)
   directory=scratch/'prefix';directory.mkdir();more,prefix_authority=physical_original_dense(image,prefixes,directory,entries=(0x120fa0,),regions=REGIONS,trace_calls=traces,trace_extended=True,preserve_x87_prefix=True);rows+=prefixes;observed['cases']+=more['cases']
  mmx_authority=None
  if args.mmx:
   from native_sphere_frustum_mmx_original import physical_original_mmx,PROBE
   rng=random.Random(0x120fa080);extra=[]
   patterns=('00000000000000000000','00000000000000000080','0000000000000080ff3f','01000000000000000000','45230100000000c0ff7f','efcdab8967452301ff3f')
   for index,original_row in enumerate(rows):
    row=copy.deepcopy(original_row);row['id']=len(rows)+index;row['name']='mmx-'+row['name'];row['live_x87']=[];row['before_fp'][1]&=~0x3800
    row['active_mmx']=dict(known_mask=index%256,physical=[patterns[(index+n)%11]if(index+n)%11<len(patterns)else(rng.getrandbits(64).to_bytes(8,'little')+(0xffff,0x3fff,0,0x7fff)[(index+n)%4].to_bytes(2,'little')).hex()for n in range(8)]);extra.append(row)
   directory=scratch/'mmx';directory.mkdir();more,mmx_authority=physical_original_mmx(image,extra,directory,entries=(0x120fa0,),regions=REGIONS+((PROBE,4096),),trace_calls=traces,trace_extended=True,preserve_x87_prefix=True);rows+=extra;observed['cases']+=more['cases']
  executed=set()
  for row,actual in zip(rows,observed['cases'],strict=True):
   row['returned']=row['phase']=='normal';assert actual['completed']==row['returned'];executed.update(actual['executed_sites']);row['expected_pc']=actual['calls'][-1]['entry']if row['returned']else actual['boundary']['instruction']
   row.update({key:actual[key]for key in('boundary_gp','boundary_eflags','boundary_xmm','after_fp','writes','reads','calls')});row['boundary_live_x87']=actual.get('boundary_live_x87',row['live_x87'])
   if 'active_mmx'in row:row['boundary_active_mmx']=actual['active_mmx']
   for packet in row['calls']:packet.pop('arguments')
   after={bank['address']:bank['after']for bank in actual['banks']}
   for bank in row['banks']:bank['after']=after[bank['address']]
  assert len(executed)==76,'Publication source sites missed'
  fixture=scratch/'fixture.json';fixture.write_text(json.dumps(dict(image_sha256=PIN,io_sites=io,cases=rows)));project=scratch/'project';project.mkdir()
  cmake=f'''cmake_minimum_required(VERSION 3.20)
project(camera_sphere LANGUAGES CXX)
include(CTest)
add_executable(sphere "{ROOT/'tests/native_tu15_segment_sphere_tests.cpp'}")
target_include_directories(sphere PRIVATE "{scratch}" "{ROOT/'include'}" "{ROOT/'src'}")
target_compile_features(sphere PRIVATE cxx_std_20)
target_compile_options(sphere PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
add_test(NAME sphere_guards COMMAND sphere --refusals)
add_test(NAME sphere_reference_guard COMMAND ${{CMAKE_COMMAND}} -E env HALO2_REFERENCE_SEGMENT_SPHERE=1 $<TARGET_FILE:sphere> --reference-guard)

add_test(NAME sphere COMMAND sphere "{fixture}" "{scratch/'native-report.json'}")
'''
  if args.source_copies:cmake+=f"""target_compile_definitions(sphere PRIVATE HALO2_SEGMENT_SPHERE_SOURCE_COPIES=1)\nadd_test(NAME sphere_reference COMMAND ${{CMAKE_COMMAND}} -E env HALO2_REFERENCE_SEGMENT_SPHERE=1 HALO2_SPHERE_REFERENCE_TEST=1 $<TARGET_FILE:sphere> "{fixture}" "{scratch/'reference-report.json'}")\n"""
  if args.sanitizers:cmake+='target_compile_options(sphere PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)\ntarget_link_options(sphere PRIVATE -fsanitize=address,undefined)\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in args.compiler or('g++','clang++'):
   command=[sys.executable,'-B',str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(args.output/Path(compiler).name),'--target','sphere','--filter','^sphere','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler]
   result=subprocess.run(command,capture_output=True,text=True);assert result.returncode==0,result.stdout[-7000:]+result.stderr[-1000:];native=json.loads((scratch/'native-report.json').read_text());assert native['comparisons']==sum((24 if args.source_copies else 8)if row['phase']=='normal'else(12 if args.source_copies else 4)for row in rows);reference=json.loads((scratch/'reference-report.json').read_text())if args.source_copies else None;results.append(dict(compiler=compiler,native_observations=native,positive_reference=reference))
  if args.semantic_mutants:
   from native_segment_sphere_mutations import qualify
   report['compiled_controls']=qualify(scratch,args.output,fixture)
  report.update(passed=True,hardware_paths=len(rows),normal_cases=sum(row['phase']=='normal'for row in rows),prefix_cases=sum(row['phase']!='normal'for row in rows),mmx_authority=mmx_authority,prefix_authority=prefix_authority,source_sites=len(executed),authority=authority,compilers=results)
 except BaseException as error:report['error']=str(error)or repr(error);raise
 finally:
  report['source_after']=hashes()
  if report['source_before']!=report['source_after']:report.update(passed=False,source_drift=True)
  (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())

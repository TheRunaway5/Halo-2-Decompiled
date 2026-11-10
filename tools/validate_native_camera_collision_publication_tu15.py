#!/usr/bin/env python3
"""Complete16B9A0 publication against unchangedi386 and authenticated defaults."""
import argparse,copy,hashlib,json,os,random,struct,subprocess,sys
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe
from validate_native_tu15_prt_helpers import PIN
from native_mopp_ray_dense_original import physical_original_dense
REGIONS=((0x61000000,0x4000),(0x62000000,0x1000))
FILES=('tests/native_tu15_camera_collision_publication_reference.inc','tools/native_camera_collision_publication_mutations.py','tools/native_camera_collision_publication_replacements.py','tools/specialize_native_operands.py','src/physics/physics_live_support_tu15_generated.cpp','tools/native_camera_collision_publication_inputs.py','src/physics/native_tu15_camera_dynamic_query_leaves.cpp','include/halo2/native_tu15_camera_collision_publication_kernel.hpp','include/halo2/native_tu15_camera_collision_publication_value.hpp','src/physics/runtime/native_tu15_camera_collision_publication.hpp','tests/native_tu15_camera_collision_publication_tests.cpp','tools/validate_native_camera_collision_publication_tu15.py','src/physics/runtime/native_tu15_mopp_ray_caller_frame.hpp','src/physics/runtime/native_tu15_raw_x87_scope.hpp','src/physics/runtime/native_tu15_sse_mmx_scope.hpp','include/halo2/native_tu15_bsp_surface_2d_kernel.hpp','src/physics/runtime/physics_world_runtime_internal.hpp','src/physics/runtime/physics_contact_runtime_internal.hpp','tools/validate_native_tu15_prt_helpers.py','tools/native_mopp_ray_dense_original.py','tools/observe_native_campaign_ai_vector_math_tu15.cpp')
def hashes():return {name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest()for name in FILES}
def corpus(smoke):
 rng=random.Random(0x16b9a0);rows=[];live=('0000000000000080ff3f','00000000000000000080','35c26821a2da0fc90040','45230100000000c0ff7f','00000000000000800040','00000000000000c0ff3f','00000000000000000000','0000000000000080ffbf')
 f32=lambda n:struct.unpack('<I',struct.pack('<f',n))[0]
 for family in range(4):
  for variant in range(2 if smoke else 32):
   n=len(rows);sp=0x62000800+n%4;out=0x61000100;transform=0x61000800 if family&2 else 0;collision=0x61001000;plane=0x61001200
   if variant==16:out=collision
   if variant==17:out=plane-40
   if variant==18:out=plane-36
   if variant==19:out=0x61000800-4
   if variant==20:out=0x61000800-8
   if variant==21:out=collision-64
   if variant==22:out=sp-8
   if variant==23:out=sp-88
   if variant==24:collision=sp-12
   if variant==25 and transform:transform=sp-52
   banks={base:bytearray(rng.randbytes(size))for base,size in REGIONS}
   def put(p,value,width=4):
    base=next(base for base,size in REGIONS if base<=p and p+width<=base+size);banks[base][p-base:p-base+width]=int(value).to_bytes(width,'little')
   for k,v in enumerate((1,1,0,0,0,1,0,0,0,1,10,20,30)):put((transform or 0x61000800)+k*4,f32(v))
   for k,v in enumerate((1,2,3,4)):put(plane+k*4,f32(v))
   put(collision,f32(.5));put(collision+4,plane);put(collision+8,f32(5));put(collision+12,f32(6));put(collision+16,0x1122b344 if family&1 else 0x11223344);put(collision+20,0x5a,1);put(collision+21,0xa5,1);put(collision+22,0x1234,2)
   if variant==16:
    put(collision,plane+0x100)
    for k,v in enumerate((2,3,4,5)):put(plane+0x100+k*4,f32(v))
   if variant==23:put(collision+20,0,1);put(collision+21,0,1);put(collision+22,0x6300,2)
   if 2<=variant<16:
    edge=(0,0x80000000,1,0x80000001,0x7f800000,0xff800000,0x7fc12345,0x7f812345,0x3f800001,0xbf800001,0x00800000,0x7f7fffff,0x7fc54321,0x7f854321)[variant-2]
    put(plane+(variant%4)*4,edge)
   matrix_base=transform or 0x61000800
   if variant==26:put(plane,0x7fc12345);put(matrix_base+4,0x7fc54321)
   if variant==27:put(plane+4,0x7fc12345);put(matrix_base+16,0x7fc54321)
   if variant==28:put(plane+8,0x7f812345);put(matrix_base+28,0x7f854321)
   if variant==29:put(plane+12,0x7fc12345);put(matrix_base,0x7fc54321)
   if variant==30:put(plane+4,0x7fc12345);put(matrix_base+20,0x7fc54321);put(matrix_base+44,0x7fc76543)
   if variant==31:put(plane,0x7f812345);put(matrix_base+4,0x7f854321)
   for k,v in enumerate((0x63000000,0x11111111,0x22222222)):put(sp+k*4,v)
   gp=[rng.getrandbits(32)for _ in range(8)];gp[0]=out;gp[1]=transform;gp[2]=collision;gp[4]=sp
   rows.append(dict(id=n,name=f'family{family}_variant{variant}',phase='normal',entry=0x16b9a0,gp=gp,eflags=0x202|(rng.getrandbits(12)&0xcd5),xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],live_x87=list(live[:(0,1,4,8)[variant%4]]),before_fp=[(0x7f,0x27f,0x37f)[n%3]|((n%4)<<10),((n%8)<<11)|(n&0x473f),0x1f80|((n%4)<<13)|(0,0x40,0x8000,0x8040)[n%4]],banks=[dict(address=base,before=bytes(data).hex())for base,data in banks.items()],matrix=0x11111111,source=0x22222222,max_steps=500))
 return rows

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--xbe',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--smoke',action='store_true');p.add_argument('--mmx',action='store_true');p.add_argument('--require-binding',action='store_true');p.add_argument('--semantic-mutants',action='store_true');p.add_argument('--compiler',action='append');p.add_argument('--sanitizers',action='store_true');args=p.parse_args()
 if not args.xbe.is_file():return 77
 scratch=Path(os.environ['HALO2_TEST_SCRATCH']);args.output.mkdir(parents=True,exist_ok=False);report=dict(passed=False,production_binding=args.require_binding,source_before=hashes(),owned_scratch=str(scratch),retained_large_artifacts=[])
 try:
  from native_camera_collision_publication_inputs import prepare_public,prepare_source
  report['public_source']=prepare_public(ROOT,scratch);report['source_binding']=prepare_source(ROOT,scratch,args.require_binding)
  assert hashlib.sha256(args.xbe.read_bytes()).hexdigest()==PIN;image=Xbe(args.xbe)
  import oracle_native_campaign_screen_lifecycle_tu15 as common
  common.ENTRIES=(0x16b9a0,);original=common.metadata;code=common._Code(image,'.text')
  def metadata(image):
   result=original(image)
   for site in result['sites']:
    if code.at(site['instruction']).mnemonic in('pop','ret'):site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
   return result
  common.metadata=metadata;meta=metadata(image);sites=meta['sites'];assert len(sites)==107
  io=[site['instruction']for site in sites if site['operands']or code.at(site['instruction']).mnemonic=='push'];traces=[dict(entry=pc,gp=True,xmm=True)for pc in io];rows=corpus(args.smoke)
  observed,authority=physical_original_dense(image,rows,scratch,entries=(0x16b9a0,),regions=REGIONS,trace_calls=traces,trace_extended=True,preserve_x87_prefix=True)
  prefixes=[];normal_cases=len(rows);prefix_authority=None
  if not args.smoke:
   for row,actual in zip(rows,observed['cases'],strict=True):
    if not(row['name'].endswith('variant0')or row['name'].endswith('variant22')or row['name'].endswith('variant23')):continue
    for phase,events in(('before_read',actual['reads']),('before_write',actual['writes']),('after_observer',actual['writes'])):
     for ordinal,event in enumerate(events):
      prefix=copy.deepcopy(row);prefix.update(id=len(rows)+len(prefixes),name=row['name']+f'_{phase}_{ordinal}',phase=phase,native_fault_ordinal=ordinal,fault=dict(phase='after_write'if phase=='after_observer'else phase,ordinal=ordinal));prefixes.append(prefix)
   directory=scratch/'prefix';directory.mkdir();more,prefix_authority=physical_original_dense(image,prefixes,directory,entries=(0x16b9a0,),regions=REGIONS,trace_calls=traces,trace_extended=True,preserve_x87_prefix=True);rows+=prefixes;observed['cases']+=more['cases']
  mmx_authority=None
  if args.mmx:
   from native_sphere_frustum_mmx_original import physical_original_mmx,PROBE
   rng=random.Random(0x16b9a080);extra=[]
   patterns=('00000000000000000000','00000000000000000080','0000000000000080ff3f','01000000000000000000','45230100000000c0ff7f','efcdab8967452301ff3f')
   for index,original_row in enumerate(rows):
    row=copy.deepcopy(original_row);row['id']=len(rows)+index;row['name']='mmx-'+row['name'];row['live_x87']=[];row['before_fp'][1]&=~0x3800
    row['active_mmx']=dict(known_mask=index%256,physical=[patterns[(index+n)%11]if(index+n)%11<len(patterns)else(rng.getrandbits(64).to_bytes(8,'little')+(0xffff,0x3fff,0,0x7fff)[(index+n)%4].to_bytes(2,'little')).hex()for n in range(8)]);extra.append(row)
   directory=scratch/'mmx';directory.mkdir();more,mmx_authority=physical_original_mmx(image,extra,directory,entries=(0x16b9a0,),regions=REGIONS+((PROBE,4096),),trace_calls=traces,trace_extended=True,preserve_x87_prefix=True);rows+=extra;observed['cases']+=more['cases']
  executed=set()
  for row,actual in zip(rows,observed['cases'],strict=True):
   row['returned']=row['phase']=='normal';assert actual['completed']==row['returned'];executed.update(actual['executed_sites']);row['expected_pc']=0x16bb33 if row['returned']else actual['boundary']['instruction']
   row.update({key:actual[key]for key in('boundary_gp','boundary_eflags','boundary_xmm','after_fp','writes','reads','calls')});row['boundary_live_x87']=actual.get('boundary_live_x87',row['live_x87'])
   if 'active_mmx'in row:row['boundary_active_mmx']=actual['active_mmx']
   for packet in row['calls']:packet.pop('arguments')
   after={bank['address']:bank['after']for bank in actual['banks']}
   for bank in row['banks']:bank['after']=after[bank['address']]
  assert len(executed)==107,'Publication source sites missed'
  fixture=scratch/'fixture.json';fixture.write_text(json.dumps(dict(image_sha256=PIN,io_sites=io,cases=rows)));project=scratch/'project';project.mkdir()
  cmake=f'''cmake_minimum_required(VERSION 3.20)
project(camera_publication LANGUAGES CXX)
include(CTest)
add_executable(publication "{ROOT/'tests/native_tu15_camera_collision_publication_tests.cpp'}")
target_include_directories(publication PRIVATE "{scratch}" "{ROOT/'include'}" "{ROOT/'src'}")
target_compile_features(publication PRIVATE cxx_std_20)
target_compile_options(publication PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
add_test(NAME publication_guards COMMAND publication --refusals)
add_test(NAME publication_reference_guard COMMAND ${{CMAKE_COMMAND}} -E env HALO2_REFERENCE_CAMERA_COLLISION_PUBLICATION=1 $<TARGET_FILE:publication> --reference-guard)
add_test(NAME publication_reference COMMAND ${{CMAKE_COMMAND}} -E env HALO2_REFERENCE_CAMERA_COLLISION_PUBLICATION=1 HALO2_PUBLICATION_REFERENCE_TEST=1 $<TARGET_FILE:publication> "{fixture}" "{scratch/'reference-report.json'}")
add_test(NAME publication COMMAND publication "{fixture}" "{scratch/'native-report.json'}")
'''
  if args.sanitizers:cmake+='target_compile_options(publication PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)\ntarget_link_options(publication PRIVATE -fsanitize=address,undefined)\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in args.compiler or('g++','clang++'):
   command=[sys.executable,'-B',str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(args.output/Path(compiler).name),'--target','publication','--filter','^publication','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler]
   result=subprocess.run(command,capture_output=True,text=True);assert result.returncode==0,result.stdout[-7000:]+result.stderr[-1000:];native=json.loads((scratch/'native-report.json').read_text());assert native['comparisons']==sum(16 if row['phase']=='normal'else 8 for row in rows);reference=json.loads((scratch/'reference-report.json').read_text());assert reference['reference_calls']>0 and reference['reference_literals']>0;results.append(dict(compiler=compiler,native_observations=native,positive_reference=reference))
  if args.semantic_mutants:
   from native_camera_collision_publication_mutations import qualify
   report['compiled_controls']=qualify(scratch,args.output,fixture)
  report.update(passed=True,hardware_paths=len(rows),normal_cases=sum(row['phase']=='normal'for row in rows),prefix_cases=sum(row['phase']!='normal'for row in rows),mmx_authority=mmx_authority,prefix_authority=prefix_authority,source_sites=len(executed),authority=authority,compilers=results)
 except BaseException as error:report['error']=str(error)or repr(error);raise
 finally:
  report['source_after']=hashes()
  if report['source_before']!=report['source_after']:report.update(passed=False,source_drift=True)
  (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())

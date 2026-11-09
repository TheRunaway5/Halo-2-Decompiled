#!/usr/bin/env python3
"""Native186E30 admission against unchanged i386; no supplied query outcome."""
import argparse,copy,hashlib,json,os,random,subprocess,sys
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe
from validate_native_tu15_prt_helpers import PIN
from native_mopp_ray_dense_original import physical_original_dense
REGIONS=((0x474000,0x1000),(0x4f9000,0x1000),(0x61000000,0x2000),(0x62000000,0x1000))
FILES=('tools/native_camera_instance_bitmap_mutations.py','tools/native_camera_instance_bitmap_replacements.py','tools/native_camera_instance_bitmap_inputs.py','src/physics/native_tu15_camera_instanced_query.cpp','src/physics/physics_live_support_tu15_generated.cpp','src/campaign/ai/source_ai_frame_tu15_generated.cpp','tools/specialize_native_operands.py','include/halo2/native_tu15_camera_instance_bitmap_kernel.hpp','src/physics/runtime/native_tu15_camera_instance_bitmap.hpp','tests/native_tu15_camera_instance_bitmap_tests.cpp','tools/validate_native_camera_instance_bitmap_tu15.py','src/physics/runtime/native_tu15_mopp_ray_caller_frame.hpp','src/physics/runtime/native_tu15_raw_x87_scope.hpp','src/physics/runtime/native_tu15_sse_mmx_scope.hpp','src/physics/runtime/native_tu15_bsp_surface_polygon.hpp','src/physics/runtime/physics_world_runtime_internal.hpp','src/physics/runtime/physics_contact_runtime_internal.hpp','tools/native_sphere_frustum_mmx_original.py','tools/native_mopp_ray_dense_original.py','tools/observe_native_campaign_ai_vector_math_tu15.cpp')
def hashes():return {name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest()for name in FILES}

def corpus(smoke):
 rng=random.Random(0x186e30);rows=[];live=('0000000000000080ff3f','00000000000000000080','35c26821a2da0fc90040','45230100000000c0ff7f','00000000000000800040','00000000000000c0ff3f','00000000000000000000','0000000000000080ffbf')
 for family,instance in enumerate((0xffffffff,0,1,0x80000000,0x7fffffff,0x12345)):
  for variant in range(2 if smoke else 16):
   n=len(rows);banks={base:bytearray(rng.randbytes(size))for base,size in REGIONS};sp=0x62000800+n%4
   if variant==12:sp=0x474c24
   if variant==13:sp=0x4f9b50
   def put(p,value,width=4):
    base=next(base for base,size in REGIONS if base<=p and p+width<=base+size);banks[base][p-base:p-base+width]=int(value).to_bytes(width,'little')
   put(0x474c24,(0x8000,0xffff,0,1,0x7fff,0x1234)[variant%6],2);put(0x4f9b50,(0,0xffffffff,0x80000000,0x7fffffff,0x61000100,0x62000100)[variant%6])
   for k,value in enumerate((0x63000000,0xa5a5a5a5,0x5a5a5a5a)):put(sp+k*4,value)
   gp=[rng.getrandbits(32)for _ in range(8)];gp[0]=instance;gp[4]=sp
   rows.append(dict(id=n,name=f'family{family}_variant{variant}',entry=0x186e30,phase='normal',gp=gp,eflags=0x202|(rng.getrandbits(12)&0xcd5),xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],live_x87=list(live[:(0,1,4,8)[variant%4]]),before_fp=[(0x7f,0x27f,0x37f)[n%3]|((n%4)<<10),((n%8)<<11)|(n&0x473f),0x1f80|((n%4)<<13)|(0,0x40,0x8000,0x8040)[n%4]],banks=[dict(address=base,before=bytes(data).hex())for base,data in banks.items()],matrix=0xa5a5a5a5,source=0x5a5a5a5a,max_steps=100))
 return rows

def main():
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--xbe',type=Path,required=True);parser.add_argument('--output',type=Path,required=True)
 parser.add_argument('--smoke',action='store_true');parser.add_argument('--sanitizers',action='store_true');parser.add_argument('--compiler',action='append');parser.add_argument('--semantic-mutants',action='store_true');parser.add_argument('--require-binding',action='store_true');parser.add_argument('--mmx',action='store_true');args=parser.parse_args()
 if not args.xbe.is_file():return 77
 scratch=Path(os.environ['HALO2_TEST_SCRATCH']);args.output.mkdir(parents=True,exist_ok=False);report=dict(passed=False,production_binding=False,source_before=hashes(),owned_scratch=str(scratch),retained_large_artifacts=[])
 try:
  from native_camera_instance_bitmap_inputs import prepare
  report['bindings']=prepare(ROOT,scratch,args.require_binding)
  assert hashlib.sha256(args.xbe.read_bytes()).hexdigest()==PIN;image=Xbe(args.xbe)
  import oracle_native_campaign_screen_lifecycle_tu15 as common
  common.ENTRIES=(0x186e30,);original=common.metadata;code=common._Code(image,'.text')
  def metadata(image):
   result=original(image)
   for site in result['sites']:
    if code.at(site['instruction']).mnemonic=='ret':site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
   return result
  common.metadata=metadata;meta=metadata(image);sites=meta['sites'];assert len(sites)==13
  assert not any(o['write']for site in sites for o in site['operands'])
  assert all(code.at(site['instruction']).mnemonic not in ('call','push')for site in sites)
  io=[site['instruction']for site in sites if site['operands']];traces=[dict(entry=pc,gp=True,xmm=True)for pc in io];rows=corpus(args.smoke)
  observed,authority=physical_original_dense(image,rows,scratch,entries=(0x186e30,),regions=REGIONS,trace_calls=traces,trace_extended=True,preserve_x87_prefix=True)
  prefixes=[]
  if not args.smoke:
   for row,actual in zip(rows,observed['cases'],strict=True):
    if not row['name'].endswith('variant0'):continue
    for ordinal,event in enumerate(actual['reads']):
     prefix=copy.deepcopy(row);prefix.update(id=len(rows)+len(prefixes),name=row['name']+f'_read{ordinal}',phase='before_read',native_fault_ordinal=ordinal,fault=dict(phase='before_read',ordinal=ordinal));prefixes.append(prefix)
  prefix_authority=None
  if prefixes:
   directory=scratch/'prefix';directory.mkdir();more,prefix_authority=physical_original_dense(image,prefixes,directory,entries=(0x186e30,),regions=REGIONS,trace_calls=traces,trace_extended=True,preserve_x87_prefix=True);observed['cases']+=more['cases']
  normal=len(rows);rows+=prefixes;mmx_authority=None
  if args.mmx:
   from native_sphere_frustum_mmx_original import physical_original_mmx,PROBE
   mmx_rows=[];rng=random.Random(0x186e3080)
   patterns=('00000000000000000000','00000000000000000080','0000000000000080ff3f','01000000000000000000','45230100000000c0ff7f','efcdab8967452301ff3f')
   for index,original_row in enumerate(rows):
    row=copy.deepcopy(original_row);row['id']=len(rows)+index;row['name']='mmx-'+row['name'];row['live_x87']=[];row['before_fp'][1]&=~0x3800
    row['active_mmx']=dict(known_mask=index%256,physical=[patterns[(index+n)%11]if(index+n)%11<len(patterns)else(rng.getrandbits(64).to_bytes(8,'little')+(0xffff,0x3fff,0,0x7fff)[(index+n)%4].to_bytes(2,'little')).hex()for n in range(8)])
    mmx_rows.append(row)
   for index in range(len(mmx_rows),256):
    row=copy.deepcopy(mmx_rows[index%len(mmx_rows)]);row['id']=len(rows)+index;row['name']='mask'+str(index)+'-'+row['name'];row['active_mmx']['known_mask']=index;mmx_rows.append(row)
   directory=scratch/'mmx';directory.mkdir();mmx,mmx_authority=physical_original_mmx(image,mmx_rows,directory,entries=(0x186e30,),regions=REGIONS+((PROBE,4096),),trace_calls=traces,trace_extended=True,preserve_x87_prefix=True)
   rows+=mmx_rows;observed['cases']+=mmx['cases']
  executed=set()
  for row,actual in zip(rows,observed['cases'],strict=True):
   assert actual['completed']==(row['phase']=='normal');assert not actual['writes'];executed.update(actual['executed_sites']);row['returned']=row['phase']=='normal'
   row.update({key:actual[key]for key in ('boundary_gp','boundary_eflags','boundary_xmm','after_fp','writes','reads','calls')});row['expected_pc']=(0x186e49 if row['gp'][0]==0xffffffff else 0x186e63)if row['returned']else actual['boundary']['instruction']
   row['boundary_live_x87']=actual.get('boundary_live_x87',row['live_x87'])
   if 'active_mmx'in row:row['boundary_active_mmx']=actual['active_mmx']
   for packet in row['calls']:packet.pop('arguments')
   after={bank['address']:bank['after']for bank in actual['banks']}
   for bank in row['banks']:bank['after']=after[bank['address']]
  assert len(executed)==13,'Bitmap source instructions missed'
  fixture=scratch/'fixture.json';fixture.write_text(json.dumps(dict(image_sha256=PIN,io_sites=io,cases=rows)));project=scratch/'project';project.mkdir()
  cmake=f'''cmake_minimum_required(VERSION 3.20)
project(camera_bitmap LANGUAGES CXX)
include(CTest)
add_executable(camera_bitmap "{ROOT/'tests/native_tu15_camera_instance_bitmap_tests.cpp'}")
target_include_directories(camera_bitmap PRIVATE "{scratch}" "{ROOT/'include'}" "{ROOT/'src'}")
target_compile_features(camera_bitmap PRIVATE cxx_std_20)
target_compile_options(camera_bitmap PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
add_test(NAME camera_bitmap_guards COMMAND camera_bitmap --refusals)
add_test(NAME camera_bitmap_reference_guard COMMAND ${{CMAKE_COMMAND}} -E env HALO2_REFERENCE_CAMERA_INSTANCE_BITMAP=1 $<TARGET_FILE:camera_bitmap> --reference-guard)
add_test(NAME camera_bitmap_reference COMMAND ${{CMAKE_COMMAND}} -E env HALO2_REFERENCE_CAMERA_INSTANCE_BITMAP=1 HALO2_CAMERA_BITMAP_REFERENCE_TEST=1 $<TARGET_FILE:camera_bitmap> "{fixture}" "{scratch/'reference-report.json'}")


add_test(NAME camera_bitmap COMMAND camera_bitmap "{fixture}" "{scratch/'native-report.json'}")
'''
  if args.sanitizers:cmake+='target_compile_options(camera_bitmap PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)\ntarget_link_options(camera_bitmap PRIVATE -fsanitize=address,undefined)\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in args.compiler or ('g++','clang++'):
   command=[sys.executable,'-B',str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(args.output/Path(compiler).name),'--target','camera_bitmap','--filter','^camera_bitmap','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler]
   result=subprocess.run(command,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True);assert result.returncode==0,result.stdout[-7000:];native=json.loads((scratch/'native-report.json').read_text());assert native['comparisons']==len(rows)*12;reference=json.loads((scratch/'reference-report.json').read_text());assert reference['reference_calls']>0 and reference['reference_literals']>0;results.append(dict(compiler=compiler,native_observations=native,positive_reference=reference))
  if args.semantic_mutants:
   from native_camera_instance_bitmap_mutations import qualify
   report['compiled_controls']=qualify(scratch,args.output,fixture)
  report['reference_shift_scope']='Exact full original body pins first; only authenticated SHL5/10 expressions use the hardware-qualified ordered primitive; unchanged-i386 authority never modified'
  report.update(passed=True,production_binding=args.require_binding,sanitizers=args.sanitizers,hardware_paths=len(rows),complete_returns=sum(row['returned']for row in rows),read_prefixes=sum(row['phase']=='before_read'for row in rows),mmx_hardware_paths=sum('active_mmx'in row for row in rows),mmx_authority=mmx_authority,source_sites=len(executed),authority=authority,prefix_authority=prefix_authority,compilers=results)
 except BaseException as error:report['error']=str(error)or repr(error);raise
 finally:
  report['source_after']=hashes()
  if report['source_before']!=report['source_after']:report.update(passed=False,source_drift=True)
  (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())

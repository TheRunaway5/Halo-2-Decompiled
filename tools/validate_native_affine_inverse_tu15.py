#!/usr/bin/env python3
"""Private complete144090 affine inverse against unchangedi386."""
import argparse,copy,hashlib,json,os,random,struct,subprocess,sys
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe
from validate_native_tu15_prt_helpers import PIN
from native_mopp_ray_dense_original import physical_original_dense
MAP_PACK_PIN='aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377'
ENTRY=0x144090;ONE=0x46a0d0
REGIONS=((0x46a000,0x1000),(0x61000000,0x4000),(0x62000000,0x1000))
FILES=('tools/native_affine_inverse_mutations.py','tools/native_affine_inverse_replacements.py','tools/native_affine_inverse_inputs.py','tests/native_tu15_affine_inverse_reference.inc','src/rendering/world/native_tu15_camera_projection.cpp','include/halo2/native_tu15_affine_inverse_value.hpp','include/halo2/native_tu15_affine_inverse_kernel.hpp','src/physics/runtime/native_tu15_affine_inverse.hpp','tests/native_tu15_affine_inverse_tests.cpp','tools/validate_native_affine_inverse_tu15.py','src/physics/runtime/native_tu15_mopp_ray_caller_frame.hpp','src/physics/runtime/native_tu15_raw_x87_scope.hpp','src/physics/runtime/native_tu15_sse_mmx_scope.hpp','src/physics/runtime/physics_contact_runtime_internal.hpp','src/physics/runtime/physics_world_runtime_internal.hpp','tools/native_mopp_ray_dense_original.py','tools/validate_native_tu15_prt_helpers.py','tools/observe_native_campaign_ai_vector_math_tu15.cpp')
SOURCE_FILES=('tools/specialize_native_operands.py','src/physics/vehicles/physics_vehicle_motion_internal.hpp','src/physics/vehicles/physics_vehicle_runtime_internal.hpp')
def hashes():return {name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest()for name in FILES}
def corpus(smoke):
 rng=random.Random(0x144090);rows=[];live=('0000000000000080ff3f','00000000000000000080','35c26821a2da0fc90040','45230100000000c0ff7f','00000000000000800040','00000000000000c0ff3f','00000000000000000000','0000000000000080ffbf')
 f32=lambda n:struct.unpack('<I',struct.pack('<f',n))[0]
 scales=(0,0x80000000,0x3f800000,0x40000000,0xbf800000,0x7fc12345)if smoke else(0,0x80000000,0x3f800000,0x40000000,0xbf800000,1,0x80000001,0x7f800000,0xff800000,0x7fc12345,0x7f812345,0x7f7fffff)
 for family,scale in enumerate(scales):
  for direction in(0,0x400):
   for variant in range(1 if smoke else 8):
    n=len(rows);sp=0x62000800+n%16;source=0x61000800;destination=0x61001000
    if variant==1:destination=source
    if variant==2:destination=source+4
    if variant==3:destination=source-4
    if variant==4:destination=sp-52
    if variant==5:source=sp-4
    if variant==6:destination=ONE-4
    if variant==7 and(scale&0x7fffffff)==0 and direction:destination=sp-4
    banks={base:bytearray(rng.randbytes(size))for base,size in REGIONS}
    def put(p,value):
     base=next(base for base,size in REGIONS if base<=p and p+4<=base+size);banks[base][p-base:p-base+4]=int(value).to_bytes(4,'little')
    for k,v in enumerate((scale,*map(f32,(1,2,3,4,5,6,7,8,9,10,11,12)))):put(source+k*4,v)
    put(ONE,0x3f800000);put(sp,0x63000000);put(sp+4,0);put(sp+8,0)
    gp=[rng.getrandbits(32)for _ in range(8)];gp[0]=destination;gp[1]=source;gp[4]=sp
    rows.append(dict(id=n,name=f'family{family}_df{direction}_variant{variant}',phase='normal',entry=ENTRY,gp=gp,eflags=0x202|direction|(rng.getrandbits(12)&0x8d5),xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],live_x87=list(live[:(0,1,4,8)[n%4]]),before_fp=[(0x7f,0x27f,0x37f)[n%3]|((n%4)<<10),((n%8)<<11)|(n&0x473f),0x1f80|((n%4)<<13)|(0,0x40,0x8000,0x8040)[n%4]],banks=[dict(address=base,before=bytes(data).hex())for base,data in banks.items()],matrix=0,source=0,max_steps=500))
 if not smoke:
  patterns=(0x7fc12345,0xffc54321,0x7f812345,0xff854321,0,0x80000000,1,0x80000001,0x7f800000,0xff800000,0x7f7fffff,0xff7fffff)
  originals=list(rows)
  for edge in range(32):
   row=copy.deepcopy(originals[32+(edge%2)*16+edge%8]);row['id']=len(rows);row['name']=f'family{12+edge}_ieee_variant{edge%8}'
   banks={bank['address']:bytearray.fromhex(bank['before'])for bank in row['banks']};source=row['gp'][1]
   def put_edge(address,value):
    base=next(base for base,size in REGIONS if base<=address and address+4<=base+size);banks[base][address-base:address-base+4]=value.to_bytes(4,'little')
   for component in range(1,13):put_edge(source+component*4,patterns[(edge+component)%len(patterns)])
   # Distinct NaN payloads on both MULSS operands establish ownership rather
   # than merely checking that a result is some NaN.
   put_edge(source+4,0x7fc12345);put_edge(source+40,0xffc54321)
   if edge>=16:put_edge(ONE,patterns[edge%len(patterns)])
   # The unchanged-i386 observer installs these declared caller words before
   # entry. Keep exactly that stack in the native fixture, including cases
   # whose source matrix overlaps the live return/argument words.
   put_edge(row['gp'][4],0x63000000);put_edge(row['gp'][4]+4,0);put_edge(row['gp'][4]+8,0)
   row['banks']=[dict(address=base,before=bytes(data).hex())for base,data in banks.items()];rows.append(row)
 return rows

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--xbe',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--code-image',choices=('TitleUpdate15','MapPack11'),default='TitleUpdate15');p.add_argument('--values',action='store_true');p.add_argument('--source-copies',action='store_true');p.add_argument('--smoke',action='store_true');p.add_argument('--mmx',action='store_true');p.add_argument('--require-binding',action='store_true');p.add_argument('--semantic-mutants',action='store_true');p.add_argument('--compiler',action='append');p.add_argument('--sanitizers',action='store_true');args=p.parse_args()
 if not args.xbe.is_file():return 77
 global ENTRY,ONE,REGIONS
 if args.code_image=='MapPack11':ENTRY=0x142d00;ONE=0x465030;REGIONS=((0x464000,0x2000),)+REGIONS[1:]
 expected_pin=MAP_PACK_PIN if args.code_image=='MapPack11'else PIN
 if args.require_binding and not args.source_copies:p.error('Default binding acceptance requires the complete owning source-copy proof')
 if args.values and args.code_image!='TitleUpdate15':p.error('The existing public value API is TU1.5-specific')
 scratch=Path(os.environ['HALO2_TEST_SCRATCH']);args.output.mkdir(parents=True,exist_ok=False);report=dict(passed=False,production_binding=False,code_image=args.code_image,source_before=hashes(),owned_scratch=str(scratch),retained_large_artifacts=[])
 try:
  if args.values:
   from native_affine_inverse_inputs import prepare_public
   report['public_source']=prepare_public(ROOT,scratch)
  if args.source_copies:
   from native_affine_inverse_inputs import prepare_source,UNITS
   report['source_before'].update({name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest()for name in (*UNITS,*SOURCE_FILES)})
   report['source_copies']=prepare_source(ROOT,scratch,args.require_binding)
  assert hashlib.sha256(args.xbe.read_bytes()).hexdigest()==expected_pin;image=Xbe(args.xbe)
  import oracle_native_campaign_screen_lifecycle_tu15 as common
  common.ENTRIES=(ENTRY,);original=common.metadata;code=common._Code(image,'.text')
  def metadata(image):
   result=original(image)
   for site in result['sites']:
    if code.at(site['instruction']).mnemonic in('pop','ret'):site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
   return result
  common.metadata=metadata;meta=metadata(image);sites=meta['sites'];assert len(sites)==82,(len(sites),"Actual inverse opcode count")
  io=[site['instruction']for site in sites if site['operands']or code.at(site['instruction']).mnemonic=='push'];traces=[dict(entry=pc,gp=True,xmm=True)for pc in io];rows=corpus(args.smoke)
  observed,authority=physical_original_dense(image,rows,scratch,entries=(ENTRY,),regions=REGIONS,trace_calls=traces,trace_extended=True,preserve_x87_prefix=True)
  prefixes=[];normal_cases=len(rows);prefix_authority=None
  if not args.smoke:
   for row,actual in zip(rows,observed['cases'],strict=True):
    if not(row['name'].endswith(('variant0','variant4','variant5','variant6','variant7'))and int(row['name'].split('_')[0][6:])<4):continue
    for phase,events in(('before_read',actual['reads']),('before_write',actual['writes']),('after_observer',actual['writes'])):
     for ordinal,event in enumerate(events):
      prefix=copy.deepcopy(row);prefix.update(id=len(rows)+len(prefixes),name=row['name']+f'_{phase}_{ordinal}',phase=phase,native_fault_ordinal=ordinal,fault=dict(phase='after_write'if phase=='after_observer'else phase,ordinal=ordinal));prefixes.append(prefix)
   directory=scratch/'prefix';directory.mkdir();more,prefix_authority=physical_original_dense(image,prefixes,directory,entries=(ENTRY,),regions=REGIONS,trace_calls=traces,trace_extended=True,preserve_x87_prefix=True);rows+=prefixes;observed['cases']+=more['cases']
  mmx_authority=None
  if args.mmx:
   from native_sphere_frustum_mmx_original import physical_original_mmx,PROBE
   rng=random.Random(0x14409080);extra=[]
   patterns=('00000000000000000000','00000000000000000080','0000000000000080ff3f','01000000000000000000','45230100000000c0ff7f','efcdab8967452301ff3f')
   for index,original_row in enumerate(rows):
    row=copy.deepcopy(original_row);row['id']=len(rows)+index;row['name']='mmx-'+row['name'];row['live_x87']=[];row['before_fp'][1]&=~0x3800
    row['active_mmx']=dict(known_mask=index%256,physical=[patterns[(index+n)%11]if(index+n)%11<len(patterns)else(rng.getrandbits(64).to_bytes(8,'little')+(0xffff,0x3fff,0,0x7fff)[(index+n)%4].to_bytes(2,'little')).hex()for n in range(8)]);extra.append(row)
   directory=scratch/'mmx';directory.mkdir();more,mmx_authority=physical_original_mmx(image,extra,directory,entries=(ENTRY,),regions=REGIONS+((PROBE,4096),),trace_calls=traces,trace_extended=True,preserve_x87_prefix=True);rows+=extra;observed['cases']+=more['cases']
  executed=set()
  for row,actual in zip(rows,observed['cases'],strict=True):
   row['returned']=row['phase']=='normal';assert actual['completed']==row['returned'];executed.update(actual['executed_sites']);row['expected_pc']=actual['calls'][-1]['entry']if row['returned']else actual['boundary']['instruction']
   row.update({key:actual[key]for key in('boundary_gp','boundary_eflags','boundary_xmm','after_fp','writes','reads','calls')});row['boundary_live_x87']=actual.get('boundary_live_x87',row['live_x87'])
   if 'active_mmx'in row:row['boundary_active_mmx']=actual['active_mmx']
   for packet in row['calls']:packet.pop('arguments')
   after={bank['address']:bank['after']for bank in actual['banks']}
   for bank in row['banks']:bank['after']=after[bank['address']]
  assert len(executed)==82,'Publication source sites missed'
  fixture=scratch/'fixture.json';fixture.write_text(json.dumps(dict(image_sha256=expected_pin,code_image=args.code_image,io_sites=io,cases=rows)));project=scratch/'project';project.mkdir()
  cmake=f'''cmake_minimum_required(VERSION 3.20)
project(camera_sphere LANGUAGES CXX)
include(CTest)
add_executable(sphere "{ROOT/'tests/native_tu15_affine_inverse_tests.cpp'}")
target_include_directories(sphere PRIVATE "{scratch}" "{ROOT/'include'}" "{ROOT/'src'}")
target_compile_features(sphere PRIVATE cxx_std_20)
target_compile_options(sphere PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
add_test(NAME sphere COMMAND sphere "{fixture}" "{scratch/'native-report.json'}")
'''
  if args.values:cmake+='target_compile_definitions(sphere PRIVATE HALO2_AFFINE_INVERSE_VALUES=1)\n'
  if args.source_copies:cmake+='target_compile_definitions(sphere PRIVATE HALO2_AFFINE_INVERSE_SOURCE_COPIES=1)\n'
  if args.source_copies:cmake+=f'add_test(NAME sphere_reference COMMAND sphere "{fixture}" "{scratch/"reference-report.json"}")\nset_tests_properties(sphere_reference PROPERTIES ENVIRONMENT "HALO2_REFERENCE_AFFINE_INVERSE=1")\n'
  if args.sanitizers:cmake+='target_compile_options(sphere PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)\ntarget_link_options(sphere PRIVATE -fsanitize=address,undefined)\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in args.compiler or('g++','clang++'):
   command=[sys.executable,'-B',str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(args.output/Path(compiler).name),'--target','sphere','--filter','^sphere','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler]
   result=subprocess.run(command,capture_output=True,text=True);assert result.returncode==0,result.stdout[-7000:]+result.stderr[-1000:];native=json.loads((scratch/'native-report.json').read_text());routes=8 if args.code_image=='MapPack11'else 12
   assert native['comparisons']==sum((8 if row['phase']=='normal'else 4)*(routes//2 if args.source_copies else 1)for row in rows)
   reference=None
   if args.source_copies:
    # The CMake runner owns/removes its executable. Positive source execution
    # is another registered test in that same private build, never a stale path.
    reference=json.loads((scratch/'reference-report.json').read_text())
   results.append(dict(compiler=compiler,native_observations=native,positive_reference=reference))
  if args.semantic_mutants:
   from native_affine_inverse_mutations import qualify
   report['compiled_mutations']=qualify(scratch,args.output,fixture)
  report.update(passed=True,production_binding=args.require_binding,hardware_paths=len(rows),normal_cases=sum(row['phase']=='normal'for row in rows),prefix_cases=sum(row['phase']!='normal'for row in rows),mmx_authority=mmx_authority,prefix_authority=prefix_authority,source_sites=len(executed),authority=authority,compilers=results)
 except BaseException as error:report['error']=str(error)or repr(error);raise
 finally:
  report['source_after']=hashes()
  if args.source_copies:
   report['source_after'].update({name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest()for name in (*UNITS,*SOURCE_FILES)})
  if report['source_before']!=report['source_after']:report.update(passed=False,source_drift=True)
  (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())

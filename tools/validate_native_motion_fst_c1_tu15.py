#!/usr/bin/env python3
"""Qualify exactly the29 Motion generator register-FSTP/C1 refinements."""
import argparse,hashlib,json,os,random,re,subprocess,sys
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from native_mopp_ray_dense_original import physical_original_dense
OLD_PIN='aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377'
UNIT='src/physics/vehicles/physics_vehicle_motion_generated.cpp'
PAIR='analysis/multiplayer/sandbox-source/vehicle-motion-20261011/correspondence-verified/report.json'
PAIR_PIN='a68799d56c8318fff1a36db962f001c6b65a2df02f6fe20f93805615da157791'
LINE=re.compile(r'if\(s.services.instruction\)\{s.services.instruction\((0x[0-9a-f]+)u\);\} s.fp.st\(([0-7])\)=s.fp.st\(0\);(?:s.fp.status&=~0x200u;)?s.fp.pop\(\);')
PREVIOUSLY_QUALIFIED=frozenset((0x30beb,0x30bed,0x30bef,0x30bfe))
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def main():
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--compiler',action='append');parser.add_argument('--sanitizers',action='store_true');parser.add_argument('--smoke',action='store_true');args=parser.parse_args()
 scratch=Path(os.environ['HALO2_TEST_SCRATCH']);args.output.mkdir(parents=True,exist_ok=False)
 files=(UNIT,PAIR,'tools/validate_native_motion_fst_c1_tu15.py','tools/generate_vehicle_motion_20261011.py','tests/native_tu15_motion_fst_c1_tests.cpp','src/physics/vehicles/physics_vehicle_motion_internal.hpp','src/physics/vehicles/physics_vehicle_runtime_internal.hpp','src/physics/runtime/physics_contact_runtime_internal.hpp','src/physics/runtime/physics_world_runtime_internal.hpp','src/physics/runtime/native_tu15_raw_x87_scope.hpp','tools/native_mopp_ray_dense_original.py','tools/validate_native_tu15_prt_helpers.py','tools/observe_native_campaign_ai_vector_math_tu15.cpp')
 pins={name:sha(ROOT/name)for name in files};report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],source_before=pins,scope='Declared one-original-instruction continuations; this does not qualify the29 containing whole functions')
 try:
  assert sha(ROOT/PAIR)==PAIR_PIN;pair=json.loads((ROOT/PAIR).read_text());mapping=pair['instruction_map']
  source=(ROOT/UNIT).read_text();matches=list(LINE.finditer(source));assert len(matches)==33
  sites=[(int(m[1],16),int(m[2]))for m in matches if int(m[1],16)not in PREVIOUSLY_QUALIFIED];assert len(sites)==29
  patterns=('0000000000000080ff3f','00000000000000000080','35c26821a2da0fc90040','45230100000000c0ff7f','0000000000000080ff7f','01000000000000000000','fffffffffffffffffe7f','4523010000000080ff7f')
  import oracle_native_campaign_screen_lifecycle_tu15 as common
  original_metadata=common.metadata
  def metadata(image):
   result=original_metadata(image);unique={}
   # Mid-owner entry CFGs overlap. Coalesce only byte/operand-identical site
   # descriptions; execution still stops after exactly one original opcode.
   for site in result['sites']:
    pc=site['instruction'];assert pc not in unique or unique[pc]==site
    unique[pc]=site
   result['sites']=list(unique.values());return result
  common.metadata=metadata
  all_rows=[];authorities=[];rng=random.Random(0xf57c1)
  for image_name,path,pin in(('MapPack11',ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03',OLD_PIN),('TitleUpdate15',NEW_DEFAULT,NEW_PIN)):
   assert sha(path)==pin;image=Xbe(path);code=_Code(image,'.text');entries=[];rows=[];listing=[]
   for old,target in sites:
    pc=old if image_name=='MapPack11'else mapping[f'{old:08x}'];instruction=code.at(pc)
    assert instruction.mnemonic=='fstp'and len(instruction.operands)==1 and instruction.reg_name(instruction.operands[0].reg)==f'st({target})',(hex(pc),instruction.mnemonic,instruction.op_str,target)
    entries.append(pc);listing.append(dict(canonical=old,actual=pc,bytes=instruction.bytes.hex(),target=target))
    for precision in((0x37f,)if args.smoke else(0x7f,0x27f,0x37f)):
     for rounding in(range(1)if args.smoke else range(4)):
      for seed in(range(2)if args.smoke else range(16)):
       depth=target+1 if seed&2 else 8;top=(seed+target)&7;condition=((seed&1)<<9)|((seed&2)<<7)|((seed&4)<<8)|((seed&8)<<11);sticky=(seed*7+rounding*11+precision//0x100)&0x3f;status=(top<<11)|condition|sticky
       bank=bytearray(4096);sp=0x62000800;bank[0x800:0x804]=(0x63000000).to_bytes(4,'little')
       gp=[rng.getrandbits(32)for _ in range(8)];gp[4]=sp
       rows.append(dict(id=len(rows),name=f'{image_name}-{old:x}-pc{precision:x}-rc{rounding}-seed{seed}',entry=pc,image=image_name,target=target,gp=gp,eflags=0x202|(rng.getrandbits(12)&0x8d5),xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],before_fp=[precision|(rounding<<10),status,0x1f80|(rounding<<13)|sticky],live_x87=[patterns[(seed+n+rounding)%len(patterns)]for n in range(depth)],banks=[dict(address=0x62000000,before=bank.hex())],matrix=0,source=0,stop_before_entries=[pc+instruction.size],max_steps=4))
   directory=scratch/image_name;directory.mkdir()
   observed,authority=physical_original_dense(image,rows,directory,entries=tuple(entries),regions=((0x62000000,4096),),trace_calls=[dict(entry=pc,gp=True,xmm=True)for pc in entries],trace_extended=True,allow_unbalanced_selected_stop=True)
   for row,actual in zip(rows,observed['cases'],strict=True):
    assert actual['completed']and actual['stopped_at']==row['stop_before_entries'][0]and not actual['reads']and not actual['writes']
    row.update(expected_gp=actual['boundary_gp'],expected_flags=actual['boundary_eflags'],expected_xmm=actual['boundary_xmm'],expected_fp=actual['after_fp'],expected_live80=actual['boundary_live_x87']);row.pop('banks');all_rows.append(row)
   authorities.append(dict(image=image_name,original_sha256=pin,authority=authority,actual_sites=listing))
  fixture=scratch/'fixture.json';fixture.write_text(json.dumps(dict(cases=all_rows)));project=scratch/'project';project.mkdir()
  cmake=f'''cmake_minimum_required(VERSION 3.20)
project(motion_fst_c1 LANGUAGES CXX)
include(CTest)
add_executable(fst "{ROOT/'tests/native_tu15_motion_fst_c1_tests.cpp'}")
target_include_directories(fst PRIVATE "{ROOT/'include'}" "{ROOT/'src'}")
target_compile_features(fst PRIVATE cxx_std_20)
target_compile_options(fst PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
add_test(NAME fst COMMAND fst "{fixture}" "{scratch/'native.json'}")
'''
  if args.sanitizers:cmake+='target_compile_options(fst PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)\ntarget_link_options(fst PRIVATE -fsanitize=address,undefined)\n'
  rejected=project/'expect_rejection.py';rejected.write_text('import subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nprint((r.stdout+r.stderr)[-1500:])\nsys.exit(0 if r.returncode!=0 and "register FSTP FP status differs"in r.stderr else 1)\n')
  cmake+=f'''add_executable(old_c1 "{ROOT/'tests/native_tu15_motion_fst_c1_tests.cpp'}")
target_include_directories(old_c1 PRIVATE "{ROOT/'include'}" "{ROOT/'src'}")
target_compile_features(old_c1 PRIVATE cxx_std_20)
target_compile_definitions(old_c1 PRIVATE HALO2_MOTION_FST_KEEP_C1=1)
target_compile_options(old_c1 PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
add_test(NAME old_c1 COMMAND "{sys.executable}" "{rejected}" $<TARGET_FILE:old_c1> "{fixture}" "{scratch/'mutant.json'}")
'''
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in args.compiler or('g++','clang++'):
   command=[sys.executable,'-B',str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(args.output/Path(compiler).name),'--target','fst','--target','old_c1','--filter','^(fst|old_c1)$','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler]
   result=subprocess.run(command,capture_output=True,text=True);assert result.returncode==0,(result.stdout+result.stderr)[-6000:];results.append(dict(compiler=compiler,native=json.loads((scratch/'native.json').read_text())))
  report.update(passed=True,cases=len(all_rows),sites_per_image=len(sites),images=authorities,compilers=results,old_uncleared_c1_rejected=True)
 except BaseException as error:report['error']=repr(error);raise
 finally:
  report['source_after']={name:sha(ROOT/name)for name in files}
  if pins!=report['source_after']:report.update(passed=False,source_drift=True)
  (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())

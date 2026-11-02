#!/usr/bin/env python3
"""Actual304 BEFORE DATA, declared2F36F0 formals, native-only service closure.

This is a direct component invocation, not a captured2F caller ABI. The second
payload is computed from the actual outer payload by304350's four plain DATA
assignments. No original service outcome, GP image or private stack image is
supplied to the native call. Independent declared packet poison is used for the
public numeric scratch interface; production private-data admission is separate.
"""
import argparse, hashlib, json, os, shutil, subprocess, sys
from pathlib import Path
from capstone import CS_AC_READ, CS_AC_WRITE
from capstone.x86_const import X86_OP_IMM, X86_OP_MEM
from native_contact_process_before_input_tu15 import NativeContactProcessBeforeInput, sha
from build_native_tu15_constraint_before_reference import build_constraint_reference
from oracle_smg_firing_1_1 import Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import complete_body

ROOT=Path(__file__).resolve().parents[1]
ARCHIVE=ROOT/'analysis/multiplayer/physics-live-tu15-20261012/mcc-native-reorientation-joint-original-v1479-20261102/iteration-01-readonly-process-304350-driving-vehicle-camera/required-native-contact-process-304350-before/actual-native-before.tar.zst'
ARCHIVE_PIN='5ef6aba031fc184d390fcc65ad6da055e16a279212232da4252a36e0b69d5c9c'
UNITS=('contact_candidates','contact_candidate_buffers','contact_agent_dispatch',
 'collision_key_query','collision_component','collision_surface_shape',
 'mopp_aabb_setup','mopp_aabb_traversal','mopp_oriented_aabb_setup',
 'shape_bounds','compound_shape_bounds','geometry','inverse_geometry','array_growth','array_reserve',
 'array_allocator','stack_arena','contact_buffers','contact_records','convex_agent_factory',
 'contact_agent_factory','contact_cache','contact_shape','activation_services',
 'collection_filter','agent_cleanup_children','agent_cleanup')
OWNERS=(0x2f36f0,0x2f33b0,0x2f3430,0x2f34d0,0x2f3140,0x2e4550,
 0x1f6d10,0x1f6fa0,0x1f76a0,0x186e70,0x2e9670,0x30a1e0,0x309940,
 0x2e9640,0x30a020,0x1f7c10,0x1f5fe0,0x1cbe70,0x185020,0x2e98d0,
 0x2e5e80,0x2e9600,0x2e8870,0x2e8120,0x306690,0x306760,0x2e4eb0,0x2f8300,
 0x2e4690,0x2e47b0,0x2e5da0,0x3077c0,0x2e7e10,0x306750,
 0x2e5db0,0x3077f0,0x1f6610,0x306790,0x2e7d90,0x1cb030,
 0xa9520,0xab4b0,0xad390,0x31dd70,0xadb50,0x149da0,0x149e00,
 0x233250,0x233420,0x2e31e0,0x2e3250,0x2e2e30,0x2e10a0,0x2e1110)

def save(p,j):p.write_text(json.dumps(j,indent=2,sort_keys=True)+'\n')
def metadata(image):
 code=_Code(image,'.text');pending=list(OWNERS);seen=set();sites={};listings={}
 regs={n:i for i,n in enumerate(('eax','ecx','edx','ebx','esp','ebp','esi','edi'))}
 while pending:
  entry=pending.pop()
  if entry in seen:continue
  seen.add(entry);body=complete_body(code,entry);lines=[]
  for pc in sorted(body.code):
   i=code.at(pc);lines.append(f'{pc:x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n')
   if i.mnemonic=='call' and i.operands and i.operands[0].type==X86_OP_IMM:
    target=i.operands[0].imm&0xffffffff
    if 0x80000<=target<0x400000 and target not in seen:pending.append(target)
   operands=[]
   if i.mnemonic!='lea' and not i.mnemonic.startswith('prefetch'):
    for op in i.operands:
     if op.type!=X86_OP_MEM:continue
     segment=i.reg_name(op.mem.segment)
     if segment=='fs':continue # compiler SEH chain, not semantic DATA
     assert not segment or segment=='es',(hex(pc),segment)
     access=CS_AC_READ if i.mnemonic in('cmp','test')else CS_AC_WRITE if i.mnemonic in('fst','fstp','fistp')else op.access
     operands.append(dict(base=regs.get(i.reg_name(op.mem.base),-1),index=regs.get(i.reg_name(op.mem.index),-1),scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(access&CS_AC_READ),write=bool(access&CS_AC_WRITE)))
   site=dict(instruction=pc,operands=operands,repeat=i.mnemonic.startswith('rep'))
   if pc in sites:assert sites[pc]==site
   sites[pc]=site
  listings[hex(entry)]=hashlib.sha256(''.join(lines).encode()).hexdigest()
 return dict(sites=list(sites.values()),source_listing_sha256=listings)

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);p.add_argument('--compilers',nargs='+',default=['g++','clang++']);p.add_argument('--convex-header',type=Path);p.add_argument('--dispatch-proof',type=Path);p.add_argument('--overlay',type=Path);p.add_argument('--public-default',action='store_true');p.add_argument('--private-checks',type=Path);p.add_argument('--mutants',action='store_true');a=p.parse_args();out=a.output.resolve();out.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],captured_bank_exclusions=[],captured_gp_or_after_consumed=False,public_shared_algorithm_default_domain=a.public_default)
 try:
  source=NativeContactProcessBeforeInput(ARCHIVE,ARCHIVE_PIN,scratch/'before',0x304350)
  reference=build_constraint_reference(source,scratch/'reference');reference['entry']='Declared original2F36F0 ECX agent,4ordinary formals from actual304 DATA; no captured2F ABI';save(scratch/'reference/reference.json',reference)
  meta=metadata(Xbe(NEW_DEFAULT));meta.update(input=source.input,regions=source.regions,before_provenance=source.receipt)
  if a.dispatch_proof:
   proof=json.loads(a.dispatch_proof.read_text());assert proof['passed']and proof['exhaustive_u8_values']==256 and proof['authored_opcode_classes']==108 and len(proof['negative_controls'])>=3
   for table in proof['immutable_dispatch_CODE_tables']:assert source.read(table['address'],table['bytes']).hex()==table['hex']
   meta['immutable_dispatch_CODE']=proof;report['immutable_dispatch_CODE_proof']=dict(path=str(a.dispatch_proof),sha256=sha(a.dispatch_proof))
  save(scratch/'metadata.json',meta)
  project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
  if a.convex_header:shutil.copyfile(a.convex_header,project/'include/halo2/native_tu15_convex_agent_binding.hpp')
  for n in UNITS:shutil.copyfile(ROOT/f'src/physics/native_tu15_{n}.cpp',project/f'native_tu15_{n}.cpp')
  for n in ('native_tu15_contact_unwind_internal.hpp','native_tu15_joint_constraint_numeric_internal.hpp'):
   shutil.copyfile(ROOT/'src/physics'/n,project/n)
  if a.overlay:
   for path in(a.overlay/'staged').rglob('*'):
    if not path.is_file():continue
    rel=path.relative_to(a.overlay/'staged');target=project/rel if str(rel).startswith('include/')else project/path.name
    target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(path,target)
  if a.public_default:
   assert a.overlay
   path=project/'include/halo2/native_tu15_contact_candidates_binding.hpp';s=path.read_text();old='        native_tu15_update_contact_candidates_defined_data(memory,producer';assert s.count(old)==1;path.write_text(s.replace(old,'        native_tu15_update_contact_candidates(memory,producer'))
  if a.dispatch_proof:
   text=(project/'native_tu15_mopp_aabb_traversal.cpp').read_text();start=text.index('    void run(');body=text[start:text.index('\n};',start)];assert hashlib.sha256(body.encode()).hexdigest()==proof['native_control_sha256']
   text=(project/'native_tu15_collision_component.cpp').read_text();start=text.index('bool native_tu15_collision_component_enabled(');body=text[start:text.index('NativeTu15ContactComponent native_tu15_resolve_surface_component(',start)];assert hashlib.sha256(body.encode()).hexdigest()==proof['native_component_control_sha256']
  shutil.copyfile(ROOT/'tools/observe_native_contact_candidates_bound_actual_tu15.cpp',project/'driver.cpp')
  shutil.copyfile(a.private_checks if a.private_checks else ROOT/'tools/native_contact_candidate_private_data_checks_tu15.hpp',project/'native_contact_candidate_private_data_checks_tu15.hpp')
  variants={};text=(project/'native_tu15_contact_candidates.cpp').read_text()
  if a.mutants:
   for name,before,after in [('wrong_transform','second_motion+0x20u,first_motion+0x20u','first_motion+0x20u,second_motion+0x20u'),('opaque_entry_read','m.word(0x2f3d6b,output+4u,m.word(input+4u))','m.word(0x2f3d6b,output+4u,0)'),('upper_publication','m.vector(0x2f37e7,self+0x30u,maximum)','m.vector(0x2f37e7,self+0x30u,minimum)')]:
    assert text.count(before)==1;(project/(name+'.cpp')).write_text(text.replace(before,after));variants[name]=dict(before=before,after=after)
   (project/'negative.py').write_text('import json,subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nj=json.load(open(sys.argv[-1]));assert r.returncode!=0 and j.get("profiles") and any(x in j.get("error","")for x in ("Captured candidate bank bytes differ","Native persistent read order differs","Native persistent store order/preimages differ","Complete native candidate FP differs")),(r.returncode,j)\nprint("Independent original rejects semantic candidate mutant")\n')
  units=' '.join(f'native_tu15_{n}.cpp'for n in UNITS if n!='contact_candidates')
  cmake='cmake_minimum_required(VERSION 3.20)\nproject(candidates_actual LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json REQUIRED)\nadd_library(driver OBJECT driver.cpp)\nadd_library(dependencies OBJECT '+units+')\n'
  targets=['driver','dependencies']
  for name in ('actual',*variants):
   cmake+='add_executable('+name+' $<TARGET_OBJECTS:driver> $<TARGET_OBJECTS:dependencies> '+('native_tu15_contact_candidates.cpp'if name=='actual'else name+'.cpp')+')\n';targets.append(name)
   result='actual-report.json'if name=='actual'else name+'-report.json';prefix=''if name=='actual'else '"'+sys.executable+'" "'+str(project/'negative.py')+'" '
   cmake+='add_test(NAME native_contact_candidates_'+name+' COMMAND '+prefix+'$<TARGET_FILE:'+name+'> "'+reference['elf']+'" "'+str(scratch/'metadata.json')+'" "${CMAKE_BINARY_DIR}/'+result+'")\n'
  for target in targets:
   cmake+='target_include_directories('+target+' PRIVATE include)\ntarget_compile_features('+target+' PRIVATE cxx_std_20)\ntarget_compile_options('+target+' PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\ntarget_link_libraries('+target+' PRIVATE nlohmann_json::nlohmann_json)\n'
   if a.overlay and not a.public_default:cmake+='target_compile_definitions('+target+' PRIVATE HALO2_CONTACT_CANDIDATE_PRIVATE_DATA=1)\n'
  (project/'CMakeLists.txt').write_text(cmake)
  results=[]
  for compiler in a.compilers:
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(out/compiler),'--target','actual',*sum((['--target',name]for name in variants),[]),'--filter','^native_contact_candidates_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1','--retain-report','actual-report.json',*sum((['--retain-report',name+'-report.json']for name in variants),[])];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command));report['results']=results;save(out/'report.json',report);assert r.returncode==0,'Native candidate actual differs; bounded report retained'
  report.update(passed=True,scope=__doc__,before=source.receipt,original_authority=NEW_PIN,original_code_authentication=reference['code_authentication'],source_listing_sha256=meta['source_listing_sha256'],source_hashes={str(x.relative_to(project)):sha(x)for x in project.rglob('*')if x.is_file()and x.suffix in('.hpp','.cpp')},original_callback_outcomes_supplied=False,native_executor_callbacks=False,production_private_data_admission_qualified=False,mutants=variants)
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(out/'report.json',report)
 return 0
if __name__=='__main__':raise SystemExit(main())

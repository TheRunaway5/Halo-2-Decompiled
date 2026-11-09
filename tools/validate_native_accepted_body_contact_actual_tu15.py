#!/usr/bin/env python3
"""Standalone native281710 on genuine1481 DATA, strict retained RAM/order/FP.

The ordinary original caller uses only the captured typed listener and provided
34 event bytes. It does not reconstruct a captured GP ABI or provide outcomes.
"""
import argparse,hashlib,inspect,json,os,re,shutil,subprocess
from pathlib import Path
from capstone import CS_AC_READ,CS_AC_WRITE
from capstone.x86_const import X86_OP_MEM
from native_created_body_contact_before_input_tu15 import NativeCreatedBodyContactBeforeInput,sha
from build_native_tu15_constraint_before_reference import build_constraint_reference
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
ROOT=Path(__file__).resolve().parents[1]
DRAFT=ROOT/'analysis/physics/native-contact-body-creation-semantic-draft-20261102/staged'
GENERAL=ROOT/'analysis/physics/native-model-constraint-general-entries-stage-20261103/staged'
OBSERVED=ROOT/'analysis/physics/native-contact-shared-observed-union-stage-20261102/staged'
UNITS=('native_tu15_contact_body_creation.cpp','native_tu15_accepted_body_contact.cpp','native_tu15_contact_material.cpp','native_tu15_contact_material_index.cpp','native_tu15_created_contact_row.cpp','native_tu15_contact_agent_factory.cpp','native_tu15_activation_services.cpp','native_tu15_gameplay_phantom_membership.cpp','native_tu15_array_reserve.cpp','native_tu15_array_growth.cpp','native_tu15_array_allocator.cpp','native_tu15_contact_body_update.cpp','native_tu15_body_properties.cpp','native_tu15_model_constraint_lookup.cpp','native_tu15_body_constraint_update.cpp')
def save(p,v):Path(p).write_text(json.dumps(v,indent=2,sort_keys=True)+'\n')
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--archive',type=Path,required=True);p.add_argument('--sha256',required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--compilers',nargs='+',default=['g++','clang++']);p.add_argument('--modes',nargs='+',type=int,default=[0,1,2,3]);p.add_argument('--faults',action='store_true');p.add_argument('--mutants',action='store_true');a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],supplied_original_outcomes=False,captured_original_gp_abi=False)
 try:
  before=NativeCreatedBodyContactBeforeInput(a.archive,a.sha256,scratch/'before');assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
  build=inspect.getsource(build_constraint_reference).replace(' mov %ax,%fs\n',' mov %ax,%fs\n movl $0xffffffff,%fs:0\n');env=dict(build_constraint_reference.__globals__);exec(build,env);authority=env['build_constraint_reference'](before,scratch/'reference');executable=Path(authority['elf'])
  graph=json.loads((ROOT/'analysis/physics/native-created-body-contact-original1481-20261103-third/original-mode0.json').read_text());pcs={r['instruction']for profile in graph['profiles']for r in profile['sites']};sites=[];regs={name:i for i,name in enumerate(('eax','ecx','edx','ebx','esp','ebp','esi','edi'))}
  for pc in sorted(pcs):
   ins=code.at(pc);ops=[]
   assert all(bytes.fromhex(r['original_bytes15'])==image.read(r['instruction'],15)for r in graph['profiles'][0]['sites']if r['instruction']==pc)
   if ins.mnemonic!='lea'and not ins.mnemonic.startswith('prefetch'):
    for op in ins.operands:
     if op.type!=X86_OP_MEM:continue
     if op.mem.segment and ins.reg_name(op.mem.segment)=='fs':continue
     access=CS_AC_WRITE if ins.mnemonic in('fst','fstp')else CS_AC_READ if ins.mnemonic in('test','cmp')else op.access
     assert access&(CS_AC_READ|CS_AC_WRITE),(hex(pc),ins.mnemonic,ins.op_str)
     ops.append(dict(base=regs.get(ins.reg_name(op.mem.base),-1),index=regs.get(ins.reg_name(op.mem.index),-1),scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(access&CS_AC_READ),write=bool(access&CS_AC_WRITE)))
   sites.append(dict(instruction=pc,operands=ops,repeated_string=ins.mnemonic.startswith('rep ')))
  save(scratch/'metadata.json',dict(sites=sites))
  report['original_metadata_sha256']=sha(scratch/'metadata.json');shutil.copyfile(scratch/'metadata.json',a.output/'original-metadata.json')
  project=scratch/'source';project.mkdir();pending=[('src/physics/'+n)for n in UNITS]+['include/halo2/native_tu15_contact_body_update_binding.hpp','include/halo2/native_tu15_contact_candidate_private_data.hpp','include/halo2/native_tu15_pool_array_services.hpp','include/halo2/native_tu15_retained_store_observer.hpp'];pins={};paths={}
  while pending:
   name=pending.pop()
   if name in pins:continue
   selected=next((root/name for root in (DRAFT,GENERAL,OBSERVED,ROOT)if(root/name).is_file()),None);assert selected,name
   content=selected.read_bytes();pins[name]=hashlib.sha256(content).hexdigest();paths[name]=selected
   target=project/name;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(content)
   for inc in re.findall(r'^#include "([^"\n]+)"',content.decode(),re.M):
    child=str(Path('include')/inc)if inc.startswith('halo2/')else str(Path(name).parent/inc)
    if any((root/child).is_file()for root in(DRAFT,GENERAL,OBSERVED,ROOT)):pending.append(child)
  original=(ROOT/'tools/observe_native_contact_producer_tu15.cpp').read_text();bridge=original[:original.index('struct Memory:SourcePhysicsContactMemory')];bridge+=original[original.index('struct Memory:SourcePhysicsContactMemory'):original.index('struct Bridge')]
  driver='#include "halo2/native_tu15_retained_store_observer.hpp"\n#include "halo2/native_tu15_accepted_body_contact.hpp"\n#include "halo2/native_tu15_contact_body_update_binding.hpp"\n#include "halo2/native_tu15_contact_candidate_private_data.hpp"\n#include "halo2/native_tu15_pool_array_services.hpp"\n#include <filesystem>\n#include <functional>\n#include <xmmintrin.h>\n'+bridge+(ROOT/'tools/observe_native_accepted_body_contact_actual_tu15.cpp').read_text();(project/'driver.cpp').write_text(driver);report['compiled_driver_sha256']=hashlib.sha256(driver.encode()).hexdigest();(a.output/'compiled-driver.cpp').write_text(driver);report['compiled_source_pins']=pins;report['effective_source_paths']={n:str(p)for n,p in paths.items()};source=a.output/'source'
  for name in pins:
   target=source/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(project/name,target)
  tools=('tools/validate_native_accepted_body_contact_actual_tu15.py','tools/observe_native_accepted_body_contact_actual_tu15.cpp','tools/native_created_body_contact_before_input_tu15.py','tools/build_native_tu15_constraint_before_reference.py','tools/observe_native_contact_producer_tu15.cpp','tools/run_test.py');tool_pins={n:sha(ROOT/n)for n in tools};report['tool_pins']=tool_pins
  for name in tools:
   target=source/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/name,target)
  report['before_input']=before.receipt;results=[]
  for compiler in a.compilers:
   target=scratch/('accepted-'+compiler);command=[compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-ffunction-sections','-fdata-sections','-I'+str(project/'include'),'-I'+str(project/'src/physics'),str(project/'driver.cpp'),*[str(project/'src/physics'/n)for n in UNITS],'-Wl,--gc-sections','-o',str(target)];r=subprocess.run(command,capture_output=True,text=True);(a.output/(compiler+'-build.log')).write_text(r.stdout+r.stderr);assert r.returncode==0,r.stderr[-5000:]
   result=a.output/(compiler+'-actual.json');r=subprocess.run([str(target),str(executable),str(before.path/'capture-native-before.json'),str(scratch/'metadata.json'),str(result),','.join(map(str,a.modes))],env={**os.environ,**({'HALO2_ACCEPTED_PARENT_FAULTS':'1'}if a.faults else{})});results.append(dict(compiler=compiler,exit_code=r.returncode,report_sha256=sha(result)));assert r.returncode==0,'Native accepted callback differs'
   if a.mutants:
    changes=(('material','src/physics/native_tu15_accepted_body_contact.cpp','r.flags,r.first_coefficient,r.second_coefficient,r.first_material,r.second_material','r.flags,r.first_coefficient,r.second_coefficient,std::uint16_t(r.first_material^1u),r.second_material'),('position','src/physics/native_tu15_contact_body_update.cpp','m.store(0x1d782cu,row+0x1cu,x);','m.store(0x1d782cu,row+0x1cu,x^1u);'),('count','src/physics/native_tu15_accepted_body_contact.cpp','m.word(0x1d87e9u,header+4u,needed);','m.word(0x1d87e9u,header+4u,needed+1u);'),('store_pc','src/physics/native_tu15_accepted_body_contact.cpp','m.word(0x1d87e9u,header+4u,needed);','m.word(0x1d87e8u,header+4u,needed);'))
    for name,unit,old,new in changes:
     text=(project/unit).read_text();assert text.count(old)==1,(name,text.count(old));modified=scratch/(compiler+'-'+name+'.cpp');modified.write_text(text.replace(old,new));binary=scratch/(compiler+'-'+name);cmd=[str(modified)if x==str(project/unit)else str(binary)if x==str(target)else x for x in command];b=subprocess.run(cmd,capture_output=True,text=True);(a.output/(compiler+'-'+name+'-build.log')).write_text(b.stdout+b.stderr);assert b.returncode==0,b.stderr[-4000:]
     output=a.output/(compiler+'-'+name+'.json');r=subprocess.run([str(binary),str(executable),str(before.path/'capture-native-before.json'),str(scratch/'metadata.json'),str(output),'0,1,2,3',name],env={k:v for k,v in os.environ.items()if k!='HALO2_ACCEPTED_PARENT_FAULTS'});assert r.returncode==0,'Mutation did not produce its declared complete-return rejection';results.append(dict(compiler=compiler,mutation=name,exit_code=r.returncode,report_sha256=sha(output)))
  assert all(sha(paths[n])==h for n,h in pins.items());source=a.output/'source'
  for name in pins:target=source/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(project/name,target)
  tools=('tools/validate_native_accepted_body_contact_actual_tu15.py','tools/observe_native_accepted_body_contact_actual_tu15.cpp','tools/native_created_body_contact_before_input_tu15.py','tools/build_native_tu15_constraint_before_reference.py','tools/observe_native_contact_producer_tu15.cpp','tools/run_test.py');tool_pins={n:sha(ROOT/n)for n in tools}
  for n in tools:target=source/n;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/n,target)
  report.update(passed=True,before_input=before.receipt,reference_authority=authority,compiled_source_pins=pins,tool_pins=tool_pins,source_inputs_unchanged=True,results=results,profiles_per_compiler=len(a.modes),declared_parent_faults=11 if a.faults else 0,complete_return_mutants_per_compiler=4 if a.mutants else 0,original_executed_sites=len(sites),scope=__doc__)
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
if __name__=='__main__':main()

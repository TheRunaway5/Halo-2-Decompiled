#!/usr/bin/env python3
"""Actual1472 listener -> nativeB1870 -> same-production nativeBA570.

Every actual BEFORE bank is retained in comparison. Real original remaining
child entries are explicit prefix limits, never supplied outputs or CPU fallback.
"""
import argparse,bisect,copy,hashlib,inspect,json,os,shutil,struct,subprocess,sys,tarfile
from pathlib import Path
import validate_native_object_deletion_tu15 as deletion
import validate_native_array_growth_tu15 as array
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def save(p,v):p.write_text(json.dumps(v,indent=2,sort_keys=True)+'\n')
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--archive',required=True,type=Path);p.add_argument('--archive-sha256',required=True);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();files=['include/halo2/native_tu15_phantom_added_listener.hpp','src/physics/native_tu15_phantom_added_listener.cpp','include/halo2/native_tu15_phantom_added_binding.hpp','include/halo2/native_tu15_phantom_added_input_observer.hpp','include/halo2/native_tu15_object_deletion.hpp','src/physics/native_tu15_object_deletion.cpp','include/halo2/native_tu15_object_deletion_binding.hpp','src/physics/native_tu15_activation_services.cpp','tools/validate_native_phantom_added_actual_tu15.py','tools/validate_native_object_deletion_tu15.py','tools/validate_native_island_agent_phase_tu15.py','tests/native_tu15_object_deletion_tests.cpp'];pins={n:sha(ROOT/n)for n in files};report=dict(passed=False,owned_scratch=str(scratch),source_hashes=pins,retained_large_artifacts=[],original_gp_supplied=False,source_private_raw_ledger_claimed=False,captured_bank_exclusions=[])
 try:
  assert sha(a.archive)==a.archive_sha256 and sha(NEW_DEFAULT)==NEW_PIN;capture=json.loads((a.archive.parent/'capture-native-before.json').read_text());typed=capture['input'];assert typed['owner']==0x282700 and typed['preserved_x87_depth']==0 and not capture['guest_abi_fabricated']and not capture['physics_result_supplied'];wanted={r['before_file']:r for r in capture['regions']};banks={};stream=subprocess.Popen(['zstd','-dc',str(a.archive)],stdout=subprocess.PIPE)
  with tarfile.open(fileobj=stream.stdout,mode='r|')as tar:
   for member in tar:
    name=Path(member.name).name
    if name in wanted:r=wanted[name];raw=tar.extractfile(member).read();assert len(raw)==r['size'];banks[r['address']]=raw
  assert stream.wait()==0 and len(banks)==len(wanted);starts=sorted(banks)
  def read(p,size):
   n=bisect.bisect_right(starts,p)-1;assert n>=0;first=starts[n];assert first<=p and p+size<=first+len(banks[first]),hex(p);return banks[first][p-first:p-first+size]
  def word(p,size=4):return int.from_bytes(read(p,size),'little')
  event=typed['event'];wrapper=word(event+4);assert word(wrapper+0x18)==1;body=word(wrapper+0x20);properties=word(body+0x30);datum=next(word(properties+n*8+4)for n in range(word(body+0x34))if word(properties+n*8)==0x2001);component=word(word(0x52be5c)+0x44)+(datum&0xffff)*160;objdatum=word(component+8);entries=word(word(0x4ecbd0)+0x44);header=entries+(objdatum&0xffff)*12;obj=word(header+8);tags=word(0x4f0414);tag=word(tags+(word(obj)&0xffff)*16+8)
  census=dict(input=typed,event16=read(event,16).hex(),wrapper=wrapper,body=body,component_datum=datum,component=component,component_flags=word(component+4),object_datum=objdatum,object_header=header,header_flags=word(header+2,1),header_type=word(header+3,1),object=obj,object_flags=word(obj+4),object_tag=word(obj),tag_flags=word(tag+2,1),parent=word(obj+0x14),mode=word(word(0x4f3218)+0xc,1),object_D4=word(obj+0xd4),object_C=word(obj+0xc),object_10=word(obj+0x10),object_60=word(obj+0x60),object_AA=word(obj+0xaa,2),spatial_root_4EABB0=word(0x4eabb0));report['input_census']=census;save(a.output/'input-census.json',census);print(json.dumps(census),flush=True)
  image=Xbe(NEW_DEFAULT);meta=deletion.metadata(image);array.OWNERS=(0x282700,0xb1870);extra,extra_auth=array.metadata(image,{});meta['sites']+=extra['sites'];fp=typed['floating_point'];case=dict(name='actual1472_added_to_native_delete',entry=0x282700,receiver=typed['listener'],minimum_input=objdatum,entry_registers={},arguments=[event],datum=objdatum,banks=[dict(address=p,before=b.hex())for p,b in sorted(banks.items())],before_fp=[fp['x87_control_word'],fp['x87_status'],fp['mxcsr']],stop_pc=deletion.physical.STOP,boundary=None)
  def boundary(c,actual):
   owner=actual['actual_stop']
   if owner==deletion.physical.STOP:return None
   return dict(owner=owner,owner_label=f'{owner:X}',scope='Genuine original remaining child entry; no return supplied')
  deletion.physical.boundary=boundary
  # Actual captures include readonly source-text DATA slices. Merge those
  # genuine bytes into the physical text allocation, rather than create
  # overlapping ELF sections. Every selected instruction must stay authentic.
  original_physical=inspect.getsource(deletion.physical.physical)
  old="text.write_bytes(image.section_bytes(section));"
  assert original_physical.count(old)==1
  merged="raw_text=bytearray(image.section_bytes(section));text_first=section.va;text_end=text_first+len(raw_text)\n for p,raw in captured_banks.items():\n  if text_first<=p and p+len(raw)<=text_end:raw_text[p-text_first:p-text_first+len(raw)]=raw\n text.write_bytes(raw_text);"
  original_physical=original_physical.replace(old,merged)
  old="  for n,b in enumerate(c['banks']):";assert original_physical.count(old)==1
  original_physical=original_physical.replace(old,old+"\n   if text_first<=b['address'] and b['address']+len(bytes.fromhex(b['before']))<=text_end:continue")
  code=deletion._Code(image,'.text')
  for site in meta['sites']:
   instruction=code.at(site['instruction']);position=bisect.bisect_right(starts,instruction.address)-1
   if position>=0:
    first=starts[position]
    if first<=instruction.address and instruction.address+instruction.size<=first+len(banks[first]):assert read(instruction.address,instruction.size)==instruction.bytes,hex(instruction.address)
  environment=dict(deletion.physical.__dict__,captured_banks=banks);exec(original_physical,environment);rows=environment['physical'](image,meta,[case],scratch);report['actual_first_boundary']=rows[0]['boundary'];report['actual_original_stop']=rows[0]['actual_stop'];save(a.output/'original-boundary.json',dict(input=census,boundary=rows[0]['boundary'],source_owners=deletion.OWNERS,ordinary_leaf_ABI_only=True));print('ACTUAL ORIGINAL BOUNDARY '+json.dumps(rows[0]['boundary']),flush=True)
  fixture=scratch/'fixture.json';save(fixture,dict(image_sha256=NEW_PIN,required_backend=deletion.BACKEND,body_sites=meta['body_sites'],cases=rows));project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include');shutil.copyfile(ROOT/'src/physics/native_tu15_object_deletion.cpp',project/'delete.cpp');shutil.copyfile(ROOT/'src/physics/native_tu15_phantom_added_listener.cpp',project/'added.cpp');shutil.copyfile(ROOT/'src/physics/native_tu15_activation_services.cpp',project/'activation.cpp')
  driver=(ROOT/'tests/native_tu15_object_deletion_tests.cpp').read_text();driver='#include "halo2/native_tu15_phantom_added_binding.hpp"\n'+driver;driver=driver.replace('NativeTu15ObjectDeletionBindingActivity binding;','NativeTu15ObjectDeletionBindingActivity binding;NativeTu15PhantomAddedBindingActivity added;NativeTu15PhantomAddedListenerServices listener;listener.delete_object=[&](auto object,auto&state){native_tu15_delete_bound_object(m,object,state,observer,&binding);};')
  needle='if(entry==0xba570u){if(production)';assert driver.count(needle)==1;driver=driver.replace(needle,'if(entry==0x282700u)native_tu15_notify_bound_gameplay_phantom_added(m,c.at("receiver"),{c.at("arguments")[0]},fp,0,listener,observer,&added);\n    else '+needle)
  driver=driver.replace('production&&c.at("entry")==0xba570u','production&&(c.at("entry")==0xba570u||c.at("entry")==0x282700u)')
  driver=driver.replace('std::string(e.what()).find(b.at("owner_label").get<std::string>())','std::string(e.what()).find(b.at("owner_label").get<std::string>())')
  (project/'driver.cpp').write_text(driver);(project/'CMakeLists.txt').write_text('cmake_minimum_required(VERSION 3.20)\nproject(actualAdded LANGUAGES CXX)\ninclude(CTest)\nadd_executable(actual driver.cpp delete.cpp added.cpp activation.cpp)\ntarget_include_directories(actual PRIVATE include)\ntarget_compile_features(actual PRIVATE cxx_std_20)\ntarget_compile_options(actual PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\nadd_test(NAME actual_added COMMAND actual "'+str(fixture)+'" --production-binding)\n');results=[];commands=[]
  for compiler in('g++','clang++'):
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','actual','--filter','^actual_added$','--jobs','1','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler];commands.append(command);r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode));assert r.returncode==0,'Actual native added/delete composition differs'
  report.update(passed=all(sha(ROOT/n)==v for n,v in pins.items()),archive_sha256=a.archive_sha256,actual_before_banks=len(banks),actual_before_bytes=sum(map(len,banks.values())),captured_typed_input=typed,ordered_reads=sum(len(c['reads'])for c in rows),ordered_stores=sum(len(c['writes'])for c in rows),commands=commands,results=results,full_return=rows[0]['boundary']is None,scope=__doc__)
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())

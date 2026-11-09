#!/usr/bin/env python3
"""Full typed304 helper: ordered retained DATA, observer FP and admission."""
import argparse,hashlib,json,os,re,shutil,subprocess,sys
from pathlib import Path
import validate_native_contact_candidates_bound_actual_tu15 as candidate
from native_contact_process_before_input_tu15 import NativeContactProcessBeforeInput,sha
from build_native_tu15_constraint_before_reference import build_constraint_reference,save
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
ROOT=Path(__file__).resolve().parents[1]

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--compilers',nargs='+',default=['g++','clang++']);p.add_argument('--modes',nargs='+',type=int,choices=range(4),default=list(range(4)));a=p.parse_args();a.stage=a.stage.resolve();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False)
 scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[])
 try:
  manifest=a.stage/'source-pins.json';stage=json.loads(manifest.read_text());stage_pin=sha(manifest)
  assert all(sha(a.stage/'staged'/n)==row['sha256']for n,row in stage['files'].items())
  names=['tools/observe_native_contact_candidates_bound_actual_tu15.cpp','tools/observe_native_contact_producer_bound_ordered_tu15.cpp',str(Path(__file__).relative_to(ROOT)),'tools/validate_native_contact_candidates_bound_actual_tu15.py','tools/build_native_tu15_constraint_before_reference.py','tools/native_contact_process_before_input_tu15.py'];pins={n:sha(ROOT/n)for n in names}
  before=NativeContactProcessBeforeInput(candidate.ARCHIVE,candidate.ARCHIVE_PIN,scratch/'before',0x304350);reference=build_constraint_reference(before,scratch/'reference')
  assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT)
  candidate.OWNERS=(*candidate.OWNERS,0x304350,0x303ee0,0x303ba0,0x303c70,0x303d00,0x303d80,0x2e4f30,0x3038c0,0x2e4fe0,0x2f8630,0x30e930,0x30de60,0x30d350,0x312620,0x30d3f0,0x314710,0x313bb0,0x32a660,0x313560,0x313390,0x313290,0x2e5e20,0x3070f0,0x2e8d20,0x2e4760,0x30cf40,0x1f6690,0x2e8d40,0x3079c0,0x3067d0)
  meta=candidate.metadata(image);meta.update(input=before.input,regions=before.regions,modes=a.modes)
  proof_path=ROOT/'analysis/physics/native-mopp-aabb-code-dispatch-20261102-fourth/report.json';proof=json.loads(proof_path.read_text());assert sha(proof_path)=='04b72ff2f5e830d6cad907ab2d20680af52f2d8d839c3ab54ce8cf86f37179ad'and proof['passed']
  # These are immutable architecture dispatch operands, independently
  # authenticated as CODE. Every physics DATA read and captured bank stays.
  for pc,table,width,entries in ((0x30ea30,0x30f70c,1,25),(0x30ea37,0x30f6e0,4,11),(0x313bf5,0x3146f8,4,5)):
   raw=image.read(table,width*entries);proof['immutable_dispatch_CODE_tables'].append(dict(address=table,bytes=len(raw),hex=raw.hex(),sha256=hashlib.sha256(raw).hexdigest()));proof['dispatch_fetches'].append(dict(instruction=pc,table=table,bytes=width,entries=entries))
  for table in proof['immutable_dispatch_CODE_tables']:assert before.read(table['address'],table['bytes']).hex()==table['hex']
  meta['immutable_dispatch_CODE']=proof;save(scratch/'metadata.json',meta)
  project=scratch/'project';shutil.copytree(a.stage/'staged',project)
  prefix=(ROOT/names[0]).read_text().split('J activity(')[0];(project/'driver.cpp').write_text(prefix+'}\n'+(ROOT/names[1]).read_text())
  cmake='cmake_minimum_required(VERSION 3.20)\nproject(orderedBoundParent LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json REQUIRED)\nadd_executable(parent driver.cpp '+' '.join(stage['units'])+')\ntarget_include_directories(parent PRIVATE include)\ntarget_compile_features(parent PRIVATE cxx_std_20)\ntarget_compile_options(parent PRIVATE -fno-fast-math -ffp-contract=off -frounding-math -ffunction-sections -fdata-sections)\ntarget_link_options(parent PRIVATE -Wl,--gc-sections)\ntarget_link_libraries(parent PRIVATE nlohmann_json::nlohmann_json)\nfile(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/evidence")\n'
  cmake+=f'add_test(NAME ordered_bound_parent COMMAND parent "{reference["elf"]}" "{scratch}/metadata.json" "${{CMAKE_BINARY_DIR}}/evidence/actual.json")\n';(project/'CMakeLists.txt').write_text(cmake)
  results=[];metadata_extensions=[]
  for compiler in a.compilers:
   worker=scratch/compiler;worker.mkdir();environment=dict(os.environ,HALO2_TEST_SCRATCH=str(worker))
   for iteration in range(17):
    output=a.output/compiler/f'iteration-{iteration:02d}'
    command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(project),'--output',str(output),'--target','parent','--filter','^ordered_bound_parent$','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','2','--test-jobs','1','--retain-report','evidence/actual.json'];r=subprocess.run(command,env=environment);results.append(dict(compiler=compiler,iteration=iteration,exit_code=r.returncode,command=command))
    result_path=output/'evidence/evidence/actual.json'
    if r.returncode==0:break
    payload=json.loads(result_path.read_text())if result_path.is_file()else{}
    match=re.fullmatch(r'Original escaped authenticated closure pc=(\d+)',payload.get('error',''))
    if not match or iteration==16:break
    pc=int(match[1]);candidate.OWNERS=(*candidate.OWNERS,pc);updated=candidate.metadata(image)
    meta['sites']=updated['sites'];meta['source_listing_sha256']=updated['source_listing_sha256'];save(scratch/'metadata.json',meta)
    metadata_extensions.append(dict(compiler=compiler,iteration=iteration,original_only_pc=pc,listing_sha256=updated['source_listing_sha256'][hex(pc)]))
   assert r.returncode==0,'Ordered native304 helper differs; inspect retained diagnostics'
  report['results']=results;report['original_only_metadata_extensions']=metadata_extensions
  assert all(sha(ROOT/n)==pin for n,pin in pins.items())and sha(manifest)==stage_pin
  assert all(sha(a.stage/'staged'/n)==row['sha256']for n,row in stage['files'].items())
  report.update(passed=True,before=before.receipt,stage_manifest_sha256=stage_pin,source_inputs_unchanged=True,tool_pins=pins,original_image_sha256=NEW_PIN,source_listing_sha256=meta['source_listing_sha256'],generated_driver_sha256=sha(project/'driver.cpp'),scope='Same typed native304 helper, strict ordered retained DATA read and typed observer SourcePC/preimage/value/order passes separately, all bank bytes/FP, admission models. Original outcomes/control bytes never enter native. Wider domains/faults/mutants/peer/World/live remain separate requirements.')
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
if __name__=='__main__':main()

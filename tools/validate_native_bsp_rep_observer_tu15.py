#!/usr/bin/env python3
"""Check REP observer stop frontiers against physical TU1.5 retained prefixes."""
import argparse,copy,json,os,shutil,struct,subprocess,sys
from pathlib import Path
import validate_native_bsp_object_association_tu15 as graph
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from build_native_tu15_constraint_before_reference import sha,save

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);args=p.parse_args()
    args.output=args.output.resolve();args.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    files=[graph.STAGE/'src/physics/native_tu15_bsp_object_association.cpp',*graph.STAGE.rglob('*.hpp'),Path(__file__),ROOT/'tools/validate_native_bsp_object_association_tu15.py',ROOT/'tools/validate_native_array_growth_tu15.py',ROOT/'tools/observe_native_array_growth_tu15.cpp',ROOT/'tests/native_tu15_array_growth_tests.cpp']
    pins={str(f.relative_to(ROOT)):sha(f)for f in files}
    report=dict(passed=False,owned_scratch=str(scratch),source_hashes=pins,retained_large_artifacts=[],source_outcomes_supplied=False)
    try:
        assert sha(NEW_DEFAULT)==NEW_PIN;graph.base.OWNERS=graph.OWNERS
        banks={p:struct.pack('<I',v)for p,v in ((0x4ecbd0,graph.POOL),(0x4ecc18,graph.SCENARIO),(0x505e7c,graph.MASK))}
        seed=graph.cases()[0];rows=[]
        for stop in (1,2,16):
            row=copy.deepcopy(seed);row.update(name='rep_observer_stop_'+str(stop),observer_stop=stop,
              fault=dict(address=graph.MASK+stop*4 if stop<16 else 0x4ecbd0,bytes=4,occurrence=1,write=stop<16));rows.append(row)
        fixture,authority=graph.base.original(Xbe(NEW_DEFAULT),banks,rows,scratch)
        assert all(row['declared_fault']and len(row['writes'])==row['observer_stop']for row in rows)
        save(fixture/'original.json',dict(image_sha256=NEW_PIN,oracle='unchanged-tu15-physical-array-and-real-pool-v1',cases=rows))
        project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
        for f in files:
            if f.suffix=='.hpp'and graph.STAGE in f.parents:shutil.copyfile(f,project/'include/halo2'/f.name)
        shutil.copyfile(files[0],project/'native.cpp')
        text=graph.driver_text();text=text.replace('if(read_pass&&!c.value("mutations",J::array()).empty())continue;','if(read_pass)continue;')
        text=text.replace('memory.fault=c.value("fault",J{});','memory.fault=J{};')
        marker='++index;}};';assert text.count(marker)==1
        text=text.replace(marker,'++index;}if(event.instruction==0x2190cfu&&actual_stores.size()==c.at("observer_stop"))throw Fault{};};')
        text=text.replace('check(cases>20,','check(cases==3,')
        (project/'driver.cpp').write_text(text)
        (project/'CMakeLists.txt').write_text('cmake_minimum_required(VERSION 3.20)\nproject(repObserver LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json REQUIRED)\nadd_executable(rep driver.cpp native.cpp)\ntarget_include_directories(rep PRIVATE include)\ntarget_compile_features(rep PRIVATE cxx_std_20)\ntarget_link_libraries(rep PRIVATE nlohmann_json::nlohmann_json)\nadd_test(NAME rep_frontier COMMAND rep "'+str(fixture/'original.json')+'")\n')
        for compiler in ('g++','clang++'):
            command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(args.output/compiler),'--target','rep','--filter','^rep_frontier$','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1']
            r=subprocess.run(command,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True);(args.output/(compiler+'-console.log')).write_text(r.stdout[-32000:]);report.setdefault('results',[]).append(dict(compiler=compiler,command=command,exit_code=r.returncode));assert r.returncode==0,r.stdout[-4000:]
        assert all(sha(ROOT/f)==v for f,v in pins.items())
        report.update(passed=True,authority=authority,stop_frontiers=[1,2,16],
          physical_reference='Unchanged TU1.5 instruction prefixes stopped before the next retained operand; no callback result supplied',
          native_stop='Actual typed observer throws immediately after requested REP store event',
          comparison='Every retained bank byte, committed SourcePC/preimage/value event prefix and FP. Original private stack/control bytes are excluded.',
          live_result_claimed=False)
    except BaseException as e:report['error']=str(e)or repr(e);raise
    finally:
        report['temporary_allocated_bytes_before_cleanup']=sum(f.stat().st_blocks*512 for f in scratch.rglob('*')if f.is_file());save(args.output/'report.json',report)
if __name__=='__main__':main()

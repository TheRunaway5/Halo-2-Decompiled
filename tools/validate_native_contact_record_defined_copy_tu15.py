#!/usr/bin/env python3
"""Guarded private303BA0/303EE0 definedness; no unknown value is fabricated."""
import argparse,json,os,re,shutil,subprocess,sys
from pathlib import Path
from build_native_tu15_constraint_before_reference import sha,save
ROOT=Path(__file__).resolve().parents[1]
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.stage=a.stage.resolve();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False)
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[])
    try:
        project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include');shutil.copytree(a.stage/'staged/include',project/'include',dirs_exist_ok=True)
        units=['native_tu15_contact_records.cpp','native_tu15_contact_buffers.cpp']
        pins={}
        for name in units:
            source=a.stage/'staged/src/physics'/name;pins['src/physics/'+name]=sha(source);shutil.copyfile(source,project/name)
        source=ROOT/'src/physics/native_tu15_array_growth.cpp';shutil.copyfile(source,project/'native_tu15_array_growth.cpp');pins[str(source.relative_to(ROOT))]=sha(source)
        shutil.copyfile(ROOT/'src/physics/native_tu15_contact_unwind_internal.hpp',project/'native_tu15_contact_unwind_internal.hpp');shutil.copyfile(ROOT/'tools/probe_native_contact_record_defined_copy_tu15.cpp',project/'driver.cpp')
        record_source=(project/'native_tu15_contact_records.cpp').read_text()
        changes={
            'omit_defined_copy':('return copy&&copy->copy_record_data(destination,source,field,bytes);','return false;'),
            'wrong_copy_source_pc':('domain->copy_defined_data(site(field,Phase::copy),','domain->copy_defined_data(0x303ba0u,')}
        for name,(old,new)in changes.items():
            assert record_source.count(old)==1,name
            (project/(name+'.cpp')).write_text(record_source.replace(old,new))
        (project/'negative.py').write_text('import subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nassert r.returncode!=0 and any(message in r.stderr for message in ("Native candidate requires unprovided private DATA","Defined record SourcePC events differ")),(r.returncode,r.stdout,r.stderr)\nprint("Defined record mutation rejected:",r.stderr.strip())\n')
        cmake='cmake_minimum_required(VERSION 3.20)\nproject(definedRecordCopy LANGUAGES CXX)\ninclude(CTest)\n'
        targets=[]
        for name in ('records',*changes):
            targets.append(name)
            unit='native_tu15_contact_records.cpp'if name=='records'else name+'.cpp'
            cmake+='add_executable('+name+' driver.cpp '+unit+' native_tu15_contact_buffers.cpp native_tu15_array_growth.cpp)\ntarget_include_directories('+name+' PRIVATE include)\ntarget_compile_features('+name+' PRIVATE cxx_std_20)\ntarget_compile_options('+name+' PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n'
            command='$<TARGET_FILE:'+name+'>'
            if name!='records':command='"'+sys.executable+'" "'+str(project/'negative.py')+'" '+command
            cmake+='add_test(NAME defined_record_copy_'+name+' COMMAND '+command+')\n'
        (project/'CMakeLists.txt').write_text(cmake)
        queue=[project/name for name in ('driver.cpp',*units,'native_tu15_array_growth.cpp','native_tu15_contact_unwind_internal.hpp')];compiled_pins={}
        while queue:
            file=queue.pop();key=str(file.relative_to(project))
            if key in compiled_pins:continue
            compiled_pins[key]=sha(file)
            for include in re.findall(r'#include\s+"([^"]+)"',file.read_text()):
                queue.append(project/'include'/include if include.startswith('halo2/')else file.parent/include)
        stage_pin=sha(a.stage/'source-pins.json');driver_pin=sha(ROOT/'tools/probe_native_contact_record_defined_copy_tu15.cpp')
        results=[]
        for compiler in ('g++','clang++'):
            command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),*sum((['--target',name]for name in targets),[]),'--filter','^defined_record_copy_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','2','--test-jobs','1'];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command));assert r.returncode==0,'Native defined record copy differs'
        assert all(sha(project/name)==pin for name,pin in compiled_pins.items())and sha(a.stage/'source-pins.json')==stage_pin and sha(ROOT/'tools/probe_native_contact_record_defined_copy_tu15.cpp')==driver_pin
        report.update(passed=True,controls=5,negative_controls=list(changes),results=results,source_hashes=pins,compiled_dependency_pins=compiled_pins,compiled_sources_unchanged=True,driver_sha256=driver_pin,stage_manifest_sha256=stage_pin,scope='Typed private bitwise definedness only. Public/default proof is separate; six inherited buffer SEH fault-prefix failures and actual304 caller lifetime/World/live remain unqualified.')
    except BaseException as e:report['error']=str(e)or repr(e);raise
    finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
if __name__=='__main__':main()

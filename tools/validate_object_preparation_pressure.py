#!/usr/bin/env python3
"""Copied-source normal/sanitizer comparison against unchanged original bytes."""
import argparse,json,shutil,subprocess
from pathlib import Path
from oracle_mp_title_update_display import ROOT,sha,save,encoded

OWN=('include/halo2/object_preparation_pressure.hpp','src/object_preparation_pressure.cpp',
     'tests/object_preparation_pressure_tests.cpp')
DEP=('include/halo2/object_construction.hpp',)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--fixture',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve();out.mkdir(parents=True,exist_ok=False)
 for name in OWN+DEP:
  target=out/'source'/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/name,target)
 commands=[]
 for mode,flags in (('normal',['-O2']),('sanitizer',['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer','-no-pie'])):
  directory=out/mode;directory.mkdir();binary=directory/'object_preparation_pressure_tests'
  for label,argv in [('build',['g++','-std=c++20','-Wall','-Wextra','-Wpedantic','-Werror',*flags,'-I',str(out/'source/include'),str(out/'source/src/object_preparation_pressure.cpp'),str(out/'source/tests/object_preparation_pressure_tests.cpp'),'-o',str(binary)]),('test',[str(binary),str(a.fixture.resolve())])]:
   with(directory/(label+'.log')).open('w')as log:result=subprocess.run(argv,cwd=ROOT,stdout=log,stderr=subprocess.STDOUT)
   commands.append(dict(mode=mode,phase=label,argv=argv,exit_code=result.returncode))
   (out/'commands.json').write_bytes(encoded(commands))
   if result.returncode:raise RuntimeError((directory/(label+'.log')).read_text()[-5000:])
  assert '262 paired original controls, 4608 explicit test-only lower calls, 831 ordered stores, 71690 retained output bytes'in(directory/'test.log').read_text()
 result=dict(schema='object-preparation-pressure-validation-v1',tool_sha256=sha(__file__),owned_source_pins={n:sha(ROOT/n)for n in OWN},dependency_pins={n:sha(ROOT/n)for n in DEP},fixture_sha256=sha(a.fixture),commands_sha256=sha(out/'commands.json'),normal_and_asan_ubsan_pass=True,emulator_sticky_exception_status_compared=False,native_host_mxcsr_restoration_checked=True,scope='Complete preparation/pressure CPU owners; explicit lower callback controls remain test-only, no live Unit/raw-body restoration or collection success.')
 save(out/'report.json',encoded(result));print(json.dumps(dict(report_sha256=sha(out/'report.json'))))
if __name__=='__main__':main()

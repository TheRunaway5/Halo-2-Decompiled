#!/usr/bin/env python3
"""Record the registered production target without pinning the mutable repo."""
import argparse,json,shutil,subprocess
from pathlib import Path
from oracle_mp_title_update_display import ROOT,sha,save,encoded
from validate_object_preparation_pressure import OWN,DEP

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--build',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--sanitizer',action='store_true');a=p.parse_args();build=a.build.resolve();out=a.output.resolve();out.mkdir(parents=True,exist_ok=False)
 observed={}
 for name in OWN+DEP+('CMakeLists.txt',):
  target=out/'source-before'/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/name,target);observed[name]=sha(ROOT/name)
 target='object_preparation_pressure_tests';commands={}
 for label,argv in [('configure',['cmake','-S',str(ROOT),'-B',str(build)]),('build',['cmake','--build',str(build),'--target',target,'-j2']),('test',['ctest','--test-dir',str(build),'-R','^original_object_preparation_pressure$','-V','--output-on-failure'])]:
  with(out/(label+'.log')).open('w')as log:result=subprocess.run(argv,cwd=ROOT,stdout=log,stderr=subprocess.STDOUT)
  commands[label]=dict(argv=argv,exit_code=result.returncode);(out/'commands.json').write_bytes(encoded(commands))
  if result.returncode:raise RuntimeError((out/(label+'.log')).read_text()[-5000:])
 assert '262 paired original controls, 4608 explicit test-only lower calls, 831 ordered stores, 71690 retained output bytes'in(out/'test.log').read_text()
 assert '100% tests passed out of 1'in(out/'test.log').read_text()
 for name in OWN+DEP:assert sha(ROOT/name)==observed[name],name
 obj='CMakeFiles/halo2_systems.dir/src/object_preparation_pressure.cpp.o'
 files=[target,'libhalo2_systems.a','libhalo2_cache.a',obj]
 for name in(target,'halo2_systems','halo2_cache'):files.extend(f'CMakeFiles/{name}.dir/{item}'for item in('flags.make','link.txt'))
 for name in files:
  source=build/name;destination=out/'build'/name;destination.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(source,destination)
 if a.sanitizer:
  for name in(target,'halo2_systems','halo2_cache'):assert '-fsanitize=address,undefined'in(out/'build/CMakeFiles'/f'{name}.dir/flags.make').read_text(),name
 archive_object=subprocess.check_output(['ar','p',str(out/'build/libhalo2_systems.a'),'object_preparation_pressure.cpp.o']);assert archive_object==(out/'build'/obj).read_bytes()
 shutil.copy2(ROOT/'CMakeLists.txt',out/'CMakeLists.after.txt')
 result=dict(schema='object-preparation-pressure-production-v1',tool_sha256=sha(__file__),observed_source_pins=observed,cmake_after_sha256=sha(ROOT/'CMakeLists.txt'),cmake_changed_during_build=observed['CMakeLists.txt']!=sha(ROOT/'CMakeLists.txt'),commands_sha256=sha(out/'commands.json'),actual_sanitizer_flags_checked=a.sanitizer,owned_object_equals_observed_archive_member=True,scope='Only the new module and registered focused test are asserted. Other linked archive members are retained observed dependencies, not all claimed rebuilt/audited against this source snapshot. No live Unit/world reclamation result.')
 save(out/'report.json',encoded(result));print(json.dumps(dict(report_sha256=sha(out/'report.json'))))
if __name__=='__main__':main()

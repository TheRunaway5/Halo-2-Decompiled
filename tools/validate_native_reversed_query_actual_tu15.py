#!/usr/bin/env python3
"""Actual Elongation query parents with an explicit offline original solver.

All parents/geometry/collector/noop are native. Sole remaining30E930 executes
unchanged original instructions with typed current inputs, not expected results.
This is not a production-native-only or raw-private-control-stack proof.
"""
import argparse,hashlib,inspect,json,os,shutil,subprocess
from pathlib import Path
import validate_native_phantom_pair_create_frame_derived_tu15 as frame
from validate_native_reversed_query_process_tu15 import UNITS
ROOT=frame.ROOT
def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def save(p,j):p.write_text(json.dumps(j,indent=2,sort_keys=True)+'\n')
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--archive',required=True,type=Path);p.add_argument('--archive-sha256',required=True);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],supplied_original_outcomes=False,remaining_services_execute_original_cpu=True,production_native_only_closure=False,source_private_raw_ledger_claimed=False)
 try:
  source=inspect.getsource(frame.main);begin=source.index('  assert sha(a.archive)');end=source.index('  bridge=');environment=dict(frame.__dict__,a=a,scratch=scratch);exec('\n'.join(line[2:]if line.startswith('  ')else line for line in source[begin:end].splitlines()),environment)
  bridge=(ROOT/'tools/observe_native_contact_producer_tu15.cpp').read_text().split('struct Memory:SourcePhysicsContactMemory')[0]
  original=(ROOT/'tools/observe_native_contact_producer_tu15.cpp').read_text();begin=original.index('struct Memory:SourcePhysicsContactMemory');end=original.index('std::array<std::uint32_t,8>gp',begin)
  bridge+=original[begin:end]
  bridge='#include "halo2/native_tu15_reversed_query_process.hpp"\n#include "halo2/native_tu15_conditional_contact_process.hpp"\n#include "halo2/native_tu15_wrapped_contact_query.hpp"\n#include "halo2/native_tu15_convex_overlap_query.hpp"\n'+bridge+(ROOT/'tools/observe_native_reversed_query_actual_tu15.cpp').read_text();(scratch/'driver.cpp').write_text(bridge)
  results=[]
  for compiler in('g++','clang++'):
   executable=scratch/('actual-'+compiler);command=[compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),str(scratch/'driver.cpp'),*[str(ROOT/'src/physics'/name)for name in UNITS],'-o',str(executable)]
   r=subprocess.run(command,capture_output=True,text=True);(a.output/(compiler+'-build.log')).write_text(r.stdout+r.stderr);assert r.returncode==0,r.stderr[-4000:]
   result=a.output/(compiler+'-actual.json');r=subprocess.run([str(executable),str(environment['source_a'].output/'original-hardware'),str(environment['bankdir']/'capture.json'),str(result)]);results.append(dict(compiler=compiler,exit_code=r.returncode,result=str(result.relative_to(a.output))));assert r.returncode==0,'Actual query native composition differs'
  report.update(passed=True,archive_sha256=a.archive_sha256,after_banks_consumed=False,compiler_results=results,source_hashes={str(ROOT/'src/physics'/name):sha(ROOT/'src/physics'/name)for name in UNITS},scope=__doc__)
 except BaseException as error:report['error']=str(error)or repr(error);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())

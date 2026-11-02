#!/usr/bin/env python3
"""Follow authenticated304 typed BEFORE through genuine original2F services."""
from pathlib import Path
import argparse,json,os,subprocess
from native_contact_process_before_input_tu15 import NativeContactProcessBeforeInput,sha
from build_native_tu15_constraint_before_reference import build_constraint_reference
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve();out.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[])
 try:
  archive=ROOT/'analysis/multiplayer/physics-live-tu15-20261012/mcc-native-reorientation-joint-original-v1479-20261102/iteration-01-readonly-process-304350-driving-vehicle-camera/required-native-contact-process-304350-before/actual-native-before.tar.zst';source=NativeContactProcessBeforeInput(archive,'5ef6aba031fc184d390fcc65ad6da055e16a279212232da4252a36e0b69d5c9c',scratch/'before',0x304350)
  for row in source.regions:
   for first,size in [(0x70000000,4096),(0x71000000,4096),(0x72000000,65536)]:assert not(row['address']<first+size and first<row['address']+row['size'])
  reference=build_constraint_reference(source,scratch/'reference');reference['entry']='Declared original304350 ECX=agent and4ordinary stack DATA formals, no captured GP';(scratch/'reference/reference.json').write_text(json.dumps(reference,indent=2)+'\n');input=scratch/'input.json';input.write_text(json.dumps(source.input));exe=scratch/'study';command=['g++','-std=c++20','-O2',str(ROOT/'tools/observe_native_contact_candidate_entry_tu15.cpp'),'-o',str(exe)];r=subprocess.run(command,capture_output=True,text=True);(out/'compile.log').write_text(r.stdout+r.stderr);assert r.returncode==0,r.stderr[-2500:];r=subprocess.run([str(exe),reference['elf'],str(input),str(out/'original-entry.json')],capture_output=True,text=True,timeout=120);(out/'run.log').write_text(r.stdout+r.stderr);assert r.returncode==0,r.stderr[-2000:];report.update(passed=True,original_reference=reference,original_entry_report_sha256=sha(out/'original-entry.json'),source_sha256={n:sha(ROOT/n)for n in ['tools/native_contact_process_before_input_tu15.py','tools/observe_native_contact_candidate_entry_tu15.cpp','tools/study_native_contact_candidate_entry_tu15.py']},scope='Source-only genuine caller-derived candidate/query/resolver boundaries. No native service or current World qualification claimed.')
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());(out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
if __name__=='__main__':main()

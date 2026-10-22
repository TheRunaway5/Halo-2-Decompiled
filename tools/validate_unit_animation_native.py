#!/usr/bin/env python3
"""Focused original-fixture native manager and pose-delivery comparisons."""
import argparse,json,subprocess
from pathlib import Path
from oracle_unit_animation_manager_1_1 import ROOT,sha,save,encoded
BASE=ROOT/'analysis/multiplayer/integration/inventory-source/unit-animation-clock'
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--cxx',default='c++');a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True)
 rows=[]
 for name,source,test,fixture in[
  ('manager','src/gameplay/animation/unit_animation.cpp','tests/unit_animation_tests.cpp',BASE/'native-fixtures/fixtures.txt'),
  ('pose','src/gameplay/animation/unit_animation_pose.cpp','tests/unit_animation_pose_tests.cpp',BASE/'pose/native-fixtures/fixtures.txt')]:
  for sanitized in(False,True):
   binary=a.output/f'{name}-{"sanitized"if sanitized else"normal"}'
   flags=['-std=c++20','-Wall','-Wextra','-Werror','-ffp-contract=off','-O1'if sanitized else'-O2']
   if sanitized:flags+=['-g','-fno-omit-frame-pointer','-fsanitize=address,undefined']
   cmd=[a.cxx,*flags,'-Iinclude', '-Isrc',source,test,'-o',str(binary)]
   subprocess.run(cmd,cwd=ROOT,check=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
   r=subprocess.run([str(binary),str(fixture)],cwd=ROOT,check=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
   rows.append(dict(name=name,sanitized=sanitized,compile_command=cmd,run_command=[str(binary),str(fixture)],exit_code=r.returncode,stdout=r.stdout,stderr=r.stderr,fixture_sha256=sha(fixture),source_sha256=sha(ROOT/source),test_sha256=sha(ROOT/test)))
 report=dict(schema='source-unit-animation-native-validation-v1',tool_sha256=sha(__file__),compiler=subprocess.check_output([a.cxx,'--version'],text=True),comparisons=rows,scope='Exact original entry/return packets and callback delivery; decoder/world/variant callbacks preserve explicit original service boundaries. This does not establish a complete live unit pose or physical movement producer.')
 save(a.output/'report.json',encoded(report));print(json.dumps(dict(report_sha256=sha(a.output/'report.json'),comparisons=len(rows))))
if __name__=='__main__':main()

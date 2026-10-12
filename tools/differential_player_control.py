#!/usr/bin/env python3
"""Run native full-packet comparisons against independently repeated originals."""
import argparse,json,subprocess
from pathlib import Path
from export_player_control_fixtures import ROOT,sha,save,enc
SOURCE=ROOT/'analysis/multiplayer/integration/inventory-source/player-control-kernel/complete'
FIXTURES=ROOT/'analysis/multiplayer/integration/inventory-source/player-control-native/complete'
PINS={
 SOURCE/'report.json':'d1ccdfd8ddf3b08becd792aa691ffd0cfab5ac9545c3c7f8919962d33304af4c',
 SOURCE/'audit.json':'47ed3b9dc584b80449732979c884eb103829dda781c223aed38a326e0bcebdf0',
 FIXTURES/'fixtures.tsv':'9ab6c457b8f5f68f3eb79cd24ead60b19dbed67fc8721e1e59594d2243c57d57',
 FIXTURES/'export.json':'c450cd6c5e73912dc17fbfed729c57e08c6b5f4c1da43e114eb691271211fb83'}
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--binary',type=Path,required=True)
 ap.add_argument('--sanitizer-binary',type=Path);ap.add_argument('--output',type=Path,default=FIXTURES/'native-validation.json');a=ap.parse_args()
 for p,pin in PINS.items():assert sha(p)==pin,p
 report=json.loads((SOURCE/'report.json').read_text());assert report['oracle_sha256']==sha(ROOT/'tools/oracle_player_control_kernel.py')
 for path,pin in report['source_pins'].items():assert sha(Path(path))==pin,path
 repeated=[SOURCE/'report.json',SOURCE/'audit.json']+[SOURCE/q['file']for q in report['listings']]
 for p in repeated:assert p.read_bytes()==(SOURCE/'reproduction'/p.relative_to(SOURCE)).read_bytes(),p
 assert (FIXTURES/'fixtures.tsv').read_bytes()==(FIXTURES/'reproduction/fixtures.tsv').read_bytes()
 runs=[]
 for label,binary in [('normal',a.binary),('address_undefined_sanitizers',a.sanitizer_binary)]:
  if binary is None:continue
  for scope,args in [('preserved_neutral_cached',[ROOT/'analysis/multiplayer/integration/player-command-fixtures.tsv']),
                     ('full_producer',[ROOT/'analysis/multiplayer/integration/player-command-fixtures.tsv',FIXTURES/'fixtures.tsv'])]:
   p=subprocess.run([str(binary.resolve()),*map(str,args)],text=True,capture_output=True)
   print(p.stdout,end='');print(p.stderr,end='')
   assert p.returncode==0,(label,scope,p.returncode)
   runs.append(dict(kind=label,scope=scope,binary_sha256=sha(binary),returncode=p.returncode,stdout=p.stdout,stderr=p.stderr))
 result=dict(schema=1,driver_sha256=sha(__file__),passed=True,source_pins={str(p.relative_to(ROOT)):pin for p,pin in PINS.items()},
  native_sources={str(p.relative_to(ROOT)):sha(p)for p in [ROOT/'include/halo2/player_command.hpp',ROOT/'src/player_command.cpp',ROOT/'tests/player_command_tests.cpp']},
  independent_original_files_byte_identical=len(repeated),full_control_packet_comparisons=75,source_target134_and_admission_compared=True,
  normal_and_sanitizer_runs=runs,limits='Stored outputs in measured masked/nearest FP scope; unmeasured positive CCC60 seat transform unavailable. No live device/codec/deadzone/physical-request/animation/current-pose integration or Xbox hardware parity asserted.')
 save(a.output,enc(result));print(json.dumps(dict(output=str(a.output),sha256=sha(a.output))))
if __name__=='__main__':main()

#!/usr/bin/env python3
"""Compare native bounded locomotion with independent original packet/store fixtures."""
import argparse, json, subprocess
from pathlib import Path
from export_unit_locomotion_fixtures import ROOT,EXPORT,verify_authority,export,digest

def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--binary',type=Path,default=ROOT/'analysis/multiplayer/integration/unit-locomotion-native/unit_locomotion_tests')
 p.add_argument('--output',type=Path,default=ROOT/'analysis/multiplayer/integration/unit-locomotion-differential.json')
 a=p.parse_args();original=verify_authority();manifest=export(EXPORT)
 for name,q in manifest['payloads'].items():assert digest(EXPORT/name)==q['sha256']
 run=subprocess.run([str(a.binary.resolve()),str(EXPORT/'fixtures.tsv')],text=True,capture_output=True)
 print(run.stdout,end='');print(run.stderr,end='')
 report=dict(schema=1,passed=run.returncode==0,scope=__doc__,cases=len(original['cases']),vectors=196,
  ordered_stores=sum(len(c['stores'])for c in original['cases']),request_and_physical_readonly=True,complete_result70_comparison=True,
  original_manifest_sha256=digest(EXPORT/'manifest.json'),original_report_sha256=manifest['original_report_sha256'],
  fixture_sha256=digest(EXPORT/'fixtures.tsv'),native_binary_sha256=digest(a.binary),tool_sha256=digest(__file__),
  native_sources={name:digest(ROOT/name)for name in('include/halo2/unit_locomotion.hpp','src/gameplay/units/unit_locomotion.cpp','tests/unit_locomotion_tests.cpp')},
  stdout=run.stdout,stderr=run.stderr,returncode=run.returncode,
  limits='Explicit non-neutral request inputs on original source-owned state1, not live device admission, physics/motion/clip/pose/frame production or CPU/hardware FP exception parity. Other physical-state dispatches remain unavailable.')
 a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2)+'\n');raise SystemExit(run.returncode)
if __name__=='__main__':main()

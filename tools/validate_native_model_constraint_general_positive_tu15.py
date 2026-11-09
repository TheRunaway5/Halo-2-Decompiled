#!/usr/bin/env python3
"""Requalify the existing positive22D460 fixture with sole general exports."""
import argparse,hashlib,json,os,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
STAGE=ROOT/'analysis/physics/native-model-constraint-general-entries-stage-20261103/staged'
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);a=p.parse_args()
 output=a.output.resolve();output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
 original=ROOT/'tools/validate_native_body_constraint_actual_before_tu15.py';source=original.read_text()
 needle=" production_parent=units['parent']";assert source.count(needle)==1
 source=source.replace(needle,needle+"\n units['parent']=STAGE/'src/physics/native_tu15_body_constraint_update.cpp'\n units['lookup']=STAGE/'src/physics/native_tu15_model_constraint_lookup.cpp'")
 needle=' pins={str(f.relative_to(ROOT)):sha(f)for f in files}';assert source.count(needle)==1
 source=source.replace(needle," files.extend(STAGE.rglob('*.hpp'))\n"+needle)
 needle="project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')";assert source.count(needle)==1
 source=source.replace(needle,needle+"\n  for header in STAGE.rglob('*.hpp'):shutil.copyfile(header,project/'include/halo2'/header.name)")
 namespace=dict(__name__='native_general_positive_overlay',__file__=str(original),STAGE=STAGE)
 exec(compile(source,str(original),'exec'),namespace)
 capture=ROOT/'analysis/multiplayer/physics-live-tu15-20261012/mcc-native-phantom-owner-original-v1466-20261101/iteration-09-native-census-readonly-guard-alias-range6-frag-vehicle-camera/required-native-constraint-component-before/actual-native-before.tar.zst'
 argv=[str(original),'--capture',str(capture),'--archive-sha256','0fd26e268228c50ea01c07a5fea7b68a1e06c515eac2ec90562f303c5873caac','--output',str(output/'positive')]
 report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],source_runner_sha256=sha(original),overlay_runner_sha256=hashlib.sha256(source.encode()).hexdigest(),wrapper_sha256=sha(__file__),command=argv,scope=__doc__,native_admission_added=False)
 old=sys.argv
 try:
  sys.argv=argv;result=namespace['main']();assert result==0
  child=json.loads((output/'positive/report.json').read_text());assert child['passed']and child['current_sources_unchanged']
  report.update(passed=True,positive_report_sha256=sha(output/'positive/report.json'),before_bytes=child['reference']['before_bytes'],source_inputs_unchanged=True)
 finally:
  sys.argv=old;report['temporary_allocated_bytes_before_cleanup']=sum(f.stat().st_blocks*512 for f in scratch.rglob('*')if f.is_file());(output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
if __name__=='__main__':main()

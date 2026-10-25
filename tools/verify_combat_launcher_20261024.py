#!/usr/bin/env python3
"""Repeat requested actual launcher gameplay against one hash-pinned selected build.

Each finite capture has its own run_test.py child owner. Source results, GPU
admission and sound readiness remain actual runtime outputs. This suite does
not certify physical Xbox pixels/waveforms or campaign/joined gameplay.
"""
import argparse,hashlib,json,os,subprocess,sys
from pathlib import Path
from run_combat_presentation_audit_20261022 import keep_log
ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'analysis/combat-presentation-audit-20261022'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def write(p,d):p.write_text(json.dumps(d,indent=2)+'\n')
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--source',type=Path,required=True);ap.add_argument('--qualification',type=Path,required=True);ap.add_argument('--label');ap.add_argument('--cases',nargs='+',choices=['banshee','teleporter','extra-rocks','lockout-green-hum','lockout-wind','scope','fp-light']);a=ap.parse_args()
 owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);selection=ROOT/'gameplay-runtime-selection.json';saved=selection.read_bytes();selected=json.loads(saved)
 exe=ROOT/selected['runtime'];assert sha(exe)==selected['runtime_sha256'];label=a.label or exe.parent.name[:8]+'-native-light'
 qualification=json.loads(a.qualification.resolve(strict=True).read_text());assert qualification['passed'] and qualification['runtime']==selected['runtime'] and qualification['runtime_sha256']==selected['runtime_sha256'],'Selected runtime is not the qualified native-light build'
 out=BASE/'repair-installed-requalification'/label
 if out.exists():
  previous=json.loads((out/'report.json').read_text())
  requested=a.cases or ['banshee','teleporter','extra-rocks','lockout-green-hum','lockout-wind','scope','fp-light']
  assert previous['passed'] and previous['status']=='completed' and previous['runtime_sha256']==selected['runtime_sha256'] and previous['runtime']==selected['runtime']
  assert [row['case'] for row in previous['cases']]==requested and all(row['execution_passed'] for row in previous['cases'])
  for reference in previous['required_references']:assert sha(Path(reference['path']))==reference['sha256']
  print(json.dumps(dict(passed=True,reused_completed_exact_runtime_receipt=str(out/'report.json'))),flush=True);return 0
 out.mkdir(parents=True,exist_ok=False)
 env=dict(os.environ,HALO2_INSTALLED_LAYOUT_SOURCE=str(a.source.resolve(strict=True)));pipeline='/home/eric/.cache/halo2-native/vulkan/000010de-00002c02-99d38200-857fe26393daa3d76f9c57eeaf41849b.bin';assert Path(pipeline).is_file()
 env.update(HALO2_GAMEPLAY_PIPELINE_REFERENCE=pipeline)
 cases=[('banshee','probe_installed_banshee_gameplay_20261022.py',dict(HALO2_COMBAT_INSTALLED_LABEL=label,HALO2_COMBAT_INSTALLED_AUDIO='1',HALO2_COMBAT_INSTALLED_NATIVE_AUDIO='1',HALO2_COMBAT_INSTALLED_LAUNCHER='1',HALO2_COMBAT_INSTALLED_MANEUVER_BOOST='1',HALO2_COMBAT_INSTALLED_LEVEL_BOOST='1',HALO2_COMBAT_INSTALLED_TURN_PHASES='12',HALO2_COMBAT_INSTALLED_PIPELINE_REFERENCE=pipeline),'repair-installed-banshee'),
 ('teleporter','probe_installed_teleporter_20261023.py',dict(HALO2_TELEPORTER_PROBE_LABEL=label,HALO2_TELEPORTER_NATIVE_AUDIO='1'),'repair-installed-teleporter')]
 for case in ['extra-rocks','lockout-green-hum','lockout-wind']:
  cases.append((case,'probe_installed_object_ambience_20261023.py',dict(HALO2_OBJECT_AMBIENCE_LABEL=label+'-'+case,HALO2_OBJECT_AMBIENCE_CASE=case),'repair-installed-object-ambience'))
 cases += [('scope','probe_installed_scope_20261023.py',dict(HALO2_SCOPE_PROBE_LABEL=label),'repair-installed-scope'),
           ('fp-light','probe_installed_fp_light_20261024.py',dict(HALO2_FP_LIGHT_LABEL=label,HALO2_FP_LIGHT_SOURCE=str(a.source.resolve(strict=True))),'repair-installed-fp-light')]
 if a.cases:cases=[row for row in cases if row[0] in a.cases]
 report=dict(scope=__doc__,status='running',passed=False,qualification=str(a.qualification.resolve()),runtime=selected['runtime'],runtime_sha256=sha(exe),cases=[],visual_reviews_pending=['banshee','teleporter','scope','fp-light'],scratch=str(owner),required_references=[],raw_audio_retained=False)
 report['visual_reviews_pending']=[name for name in report['visual_reviews_pending'] if any(row[0]==name for row in cases)]
 write(out/'report.json',report)
 for name,script,extra,folder in cases:
  if selection.read_bytes()!=saved:raise RuntimeError('Launcher selection changed; do not combine evidence from different builds')
  command=[sys.executable,str(ROOT/'tools/run_test.py'),'--scratch-root',str(owner),'--',sys.executable,str(ROOT/'tools'/script)]
  log=owner/(name+'.log');child_env=dict(env,**extra)
  with log.open('w') as stream:code=subprocess.run(command,env=child_env,stdout=stream,stderr=subprocess.STDOUT).returncode
  keep_log(log,out/(name+'.log'));actual_label=extra.get('HALO2_OBJECT_AMBIENCE_LABEL',label);path=BASE/folder/actual_label/'report.json'
  actual=json.loads(path.read_text()) if path.exists() else {}
  matched=actual.get('runtime_sha256')==selected['runtime_sha256']
  row=dict(case=name,exit_code=code,report=str(path),same_runtime=matched,execution_passed=code==0 and matched)
  for key in ['retained_reference_image','retained_reference']:
   if actual.get(key):report['required_references'].append(actual[key])
  report['required_references'].extend(actual.get('retained_flight_images',[]))
  report['cases'].append(row);write(out/'report.json',report);print(json.dumps(row),flush=True)
 report.update(status='completed',passed=all(x['execution_passed'] for x in report['cases']) and selection.read_bytes()==saved,selection_unchanged=selection.read_bytes()==saved,visual_acceptance_complete=False)
 write(out/'report.json',report);print(json.dumps(dict(passed=report['passed'],report=str(out/'report.json'))),flush=True)
 return 0 if report['passed'] else 1
if __name__=='__main__':raise SystemExit(main())

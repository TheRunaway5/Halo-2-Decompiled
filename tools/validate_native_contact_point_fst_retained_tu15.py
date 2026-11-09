#!/usr/bin/env python3
"""Physical TU1.5 FST32 plus production borrowed-memory attribution/fault controls."""
from pathlib import Path
import argparse,hashlib,json,os,shutil,subprocess
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
ROOT=Path(__file__).resolve().parents[1]
N='src/physics/native_tu15_contact_point_create.cpp'
PIN='c14eb547e5ddfb253b59151019f0f1cfbe299532f6474612da18d0ceadca19bc'
BASE='8277160d'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--source-root',type=Path,required=True);p.add_argument('--candidate',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 out=a.output.resolve();out.mkdir(exist_ok=False,parents=True);owner=Path(os.environ['HALO2_TEST_SCRATCH'])
 assert sha(NEW_DEFAULT)==NEW_PIN and Xbe(NEW_DEFAULT).read(0x322f19,3)==bytes.fromhex('d95004')
 source=a.source_root.resolve();candidate=a.candidate.resolve()
 assert sha(candidate)==PIN and sha(source/N).startswith(BASE)
 driver=ROOT/'tools/observe_native_contact_point_fst_retained_tu15.cpp';shutil.copyfile(candidate,owner/'point.cpp')
 shutil.copyfile(driver,owner/'driver.cpp')
 r=dict(passed=False,scope=__doc__,owned_scratch=str(owner),source_xbe_sha256=NEW_PIN,source_instruction=0x322f19,source_opcode='d95004',candidate_sha256=PIN,baseline_sha256=sha(source/N),driver_sha256=sha(driver),results=[],retained_large_artifacts=[])
 try:
  for cc in ('g++','clang++'):
   exe=owner/('fst-'+cc)
   cmd=[cc,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-ffunction-sections','-fdata-sections','-I'+str(source/'include'),'-I'+str(source/'src/physics'),str(owner/'driver.cpp'),'-Wl,--gc-sections','-o',str(exe)]
   c=subprocess.run(cmd,capture_output=True,text=True);(out/(cc+'-build.log')).write_text(c.stdout+c.stderr);assert c.returncode==0,c.stderr
   c=subprocess.run([str(exe)],capture_output=True,text=True);(out/(cc+'-run.log')).write_text(c.stdout+c.stderr);assert c.returncode==0,c.stderr
   result=json.loads(c.stdout);assert result['passed'] and result['normal_profiles']==28 and result['hardware_write_fault_profiles']==28
   r['results'].append(dict(compiler=cc,command=cmd,**result))
  assert sha(candidate)==PIN and sha(owner/'point.cpp')==PIN
  r['passed']=True
 finally:
  r['allocated_bytes_before_cleanup']=sum(x.stat().st_blocks*512 for x in owner.rglob('*')if x.is_file());(out/'report.json').write_text(json.dumps(r,indent=2,sort_keys=True)+'\n')
if __name__=='__main__':main()

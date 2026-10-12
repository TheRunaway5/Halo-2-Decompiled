#!/usr/bin/env python3
"""Verify original frontend music selectors and complete native loop boundaries."""
import argparse,hashlib,json,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'analysis/frontend/profile-regression'
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--binary',type=Path,default=ROOT/'build-frontend/menu_music_tests')
 a=p.parse_args();original=json.loads((OUT/'menu-music-original.json').read_text())
 scheduler=json.loads((OUT/'menu-music-scheduler-original.json').read_text())
 assert sha(ROOT/'assets/disc/default.xbe')==original['xbe_sha256']
 assert sha(ROOT/'assets/disc/maps/mainmenu.map')==original['map_sha256']
 assert sha(OUT/'menu-music-original.tsv')==original['fixture_sha256']
 assert sha(OUT/'menu-music-scheduler-original.tsv')==scheduler['fixture_sha256']
 assert scheduler['xbe_sha256']==original['xbe_sha256']
 run=subprocess.run([str(a.binary.resolve()),str(ROOT/'assets/disc/maps'),str(OUT/'menu-music-original.tsv')],capture_output=True,text=True)
 (OUT/'menu-music-verified.log').write_text(run.stdout+run.stderr)
 report=dict(passed=run.returncode==0,native_binary_sha256=sha(a.binary),tool_sha256=sha(Path(__file__)),
   original_fixture_sha256=original['fixture_sha256'],original_selector_cases=original['cases'],
   original_lcg_stores=original['rng_stores'],original_seed_initializations=original['seed_initializations'],
   original_fade_cases=original['fade_cases'],full_intro_and_five_loop_boundaries_mixed=run.returncode==0,
   original_scheduler_cases=scheduler['cases'],
   original_scheduler_fixture_sha256=scheduler['fixture_sha256'],
   complete_loop_permutations_before_repeat=4 if run.returncode==0 else None,
   source_5001ms_fade_endpoint_verified=run.returncode==0,
   scope='Exact original bounded219110 selection and12956ordered stores,1462B0 integer initializer,12A810 fade gains/mutated times and235B9C music scheduling. Native full intro/four complete ASF loop permutations/five boundary transitions, fade continuity and unit-gain retirement endpoint checked through the mixer.',
   whole_frontend_parity_verified=False,original_decoder_waveform_parity_verified=False,
   global_secondary_rng_ordering_verified=False,original_driver_update_cadence_verified=False,
   stdout=run.stdout,stderr=run.stderr)
 (OUT/'menu-music-verified.json').write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps(report,indent=2));raise SystemExit(run.returncode)
if __name__=='__main__':main()

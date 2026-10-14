#!/usr/bin/env python3
"""Check every captured update of a drawn particle graph against untouched TU1.5.

This diagnostic invokes the MP Wraith charge callback manually, submits one
ordinary view, then retains that frame's genuine draw stamps while particles
age after the real stop request. It does not certify repeated rendering,
production Wraith charging or physical Xbox pixels.
"""
import argparse,json,subprocess
from pathlib import Path
from audit_rendering import file_sha
from verify_particle_frame_evidence_tu15 import equal_banks,pool_counts

ROOT=Path(__file__).resolve().parents[1]

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--case',type=Path,required=True)
    ap.add_argument('--particle-build',type=Path,required=True)
    ap.add_argument('--output',type=Path,required=True)
    a=ap.parse_args();case=a.case.resolve();build=a.particle_build.resolve();out=a.output.resolve()
    assert not out.exists();out.mkdir(parents=True)
    retained=out/'verifier-source.py';retained.write_bytes(Path(__file__).read_bytes())
    metadata=json.loads((case/'build.json').read_text());compiled=json.loads((build/'build.json').read_text())
    assert metadata['native_returncode']==0 and metadata['particle_build_sha256']==file_sha(build/'build.json')
    assert compiled['binary_sha256']==file_sha(build/'replay-native')
    lifetime=json.loads((case/'particle-lifetime.json').read_text());assert lifetime['passed']
    assert lifetime['stages'][0]['live_particles']>0 and lifetime['stages'][-1]['salt']==0
    count=len(lifetime['stages'])-2;assert 2<count<=128
    commands=[]
    def run(arguments,log):
        command=[str(ROOT/'.tools/python/bin/python'),*map(str,arguments)]
        with log.open('w') as stream:p=subprocess.run(command,cwd=ROOT,stdout=stream,stderr=subprocess.STDOUT)
        commands.append(dict(command=command,returncode=p.returncode,log=str(log.relative_to(out))))
        return p.returncode
    charge=[]
    for name,p in [('allocate',case),('stop',case/'stop')]:
        report=p/'original-comparison-v2.json'
        if not report.exists():
            rc=run([ROOT/'tools/oracle_charged_effect_lifetime_bank_tu15.py','--case',p,'--output',report],out/(name+'-original.log'))
            assert rc in (0,1)
        original=json.loads(report.read_text());assert original['original_pc']==0x60300000
        assert original['case_before_sha256']==file_sha(p/'before.bin')
        assert not original['services'] and not original['instruction_patches']
        assert original['matched_native_stores']==original['total_native_stores']
        if not (p/'hardware-verification/report.json').exists():
            assert not (p/'hardware-verification').exists()
            assert not run([ROOT/'tools/verify_charged_effect_hardware_tu15.py','--case',p],out/(name+'-hardware.log'))
        hardware=json.loads((p/'hardware-verification/report.json').read_text());assert hardware['passed']
        assert hardware['input_sha256']==file_sha(p/'before.bin') and hardware['native_exit_sha256']==file_sha(p/'native-exit.bin')
        charge.append(dict(case=name,original_sha256=file_sha(report),hardware_sha256=file_sha(p/'hardware-verification/report.json')))
    updates=[]
    for n in range(1,count+1):
        original=out/f'frame{n}-original';native=out/f'frame{n}-native'
        assert not run([ROOT/'tools/probe_original_charge_particle_tu15.py','--case',case,'--lifetime-frame',n,'--entry','0x179df0','--output',original],out/f'frame{n}-original.log')
        assert not run([ROOT/'tools/verify_original_particle_hardware_tu15.py','--case',original],out/f'frame{n}-hardware.log')
        command=[str(build/'replay-native'),str(original),'0x179df0',str(native)]
        with (out/f'frame{n}-native.log').open('w') as log:p=subprocess.run(command,cwd=ROOT,stdout=log,stderr=subprocess.STDOUT)
        assert not p.returncode
        native_api=case/f'particle-frame{n}'
        compared=equal_banks(native_api/'after.bin',original/'hardware/hardware-after.bin')
        original_report=json.loads((original/'report.json').read_text())
        hardware=json.loads((original/'hardware/report.json').read_text());assert hardware['emulator_retained_state_matches_hardware']
        before=pool_counts(native_api/'before.bin');after=pool_counts(native_api/'after.bin')
        assert after['particles']['count']<=before['particles']['count']
        updates.append(dict(frame=n,before=before,after=after,retained_bytes=compared,instructions=original_report['original_instructions'],writes=original_report['retained_stores'],original_report_sha256=file_sha(original/'report.json'),hardware_report_sha256=file_sha(original/'hardware/report.json'),native_files_sha256={f.name:file_sha(f) for f in native.iterdir() if f.is_file()}))
        print(json.dumps(dict(frame=n,frames=count,live_particles=after['particles']['count'],passed=True)),flush=True)
    assert updates[0]['before']['particles']['count']==lifetime['stages'][0]['live_particles']
    assert updates[-1]['after']['particles']['count']==0
    for pool in ['particle_system','particle_emitter','particle_location']:assert updates[-1]['after'][pool]['count']==0
    assert file_sha(Path(__file__))==file_sha(retained),'Verifier changed during execution'
    result=dict(scope=__doc__,verifier_sha256=file_sha(retained),case=str(case),case_build_sha256=file_sha(case/'build.json'),particle_build_sha256=file_sha(build/'build.json'),lifetime_sha256=file_sha(case/'particle-lifetime.json'),charge=charge,commands=commands,updates=updates,source_effect_retirement_tick=lifetime['stages'][-1]['source_tick'],one_draw_submission_only=True,repeated_rendering_verified=False,physical_xbox_verified=False,full_weapon_parity=False,passed=True)
    (out/'report.json').write_text(json.dumps(result,indent=2,sort_keys=True)+'\n')
    print(json.dumps(dict(passed=True,frames=count,particles_at_stop=lifetime['stages'][0]['live_particles'],retirement_tick=result['source_effect_retirement_tick'])))

if __name__=='__main__':main()

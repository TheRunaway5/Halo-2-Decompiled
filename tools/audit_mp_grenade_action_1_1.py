#!/usr/bin/env python3
"""Audit actual negative grenade requests and genuine texture-owner stop."""
import argparse
import json
import struct
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,PIN
from oracle_mp_grenade_action_1_1 import motion

BASE=ROOT/'analysis/multiplayer/integration/grenade-action'

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();assert not args.output.exists()
    image=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
    assert motion.digest(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')==PIN
    checked=0;files={}
    for left,right in [('exploratory','repeat'),('texture-context','texture-repeat')]:
        for path in (BASE/left).rglob('*'):
            if not path.is_file():continue
            paired=BASE/right/path.relative_to(BASE/left)
            assert path.read_bytes()==paired.read_bytes(),path
        for folder in [left,right]:
            report_path=BASE/folder/'report.json';report=json.loads(report_path.read_text())
            assert report['tool_sha256']==motion.digest(ROOT/('tools/oracle_mp_grenade_action_1_1.py' if left=='exploratory' else 'tools/oracle_mp_grenade_texture_context_1_1.py'))
            assert not report['new_supplied_gameplay_fields'] and not report['new_supplied_source_returns'] and not report['instruction_patches']
            for name,digest in report['source_pins'].items():assert motion.digest(ROOT/name)==digest,name
            for row in report['instructions']:
                path=BASE/folder/row['file'];assert motion.digest(path)==row['sha256']
                for line in path.read_text().splitlines():
                    address,raw,_=line.split(' ',2);data=bytes.fromhex(raw)
                    assert image.read(int(address,16),len(data))==data,(path,address)
                    checked+=1
            for path in (BASE/folder).rglob('*'):
                if path.is_file():files[str(path.relative_to(ROOT))]=motion.digest(path)
    initial=json.loads((BASE/'exploratory/report.json').read_text())
    assert len(initial['cases'])==2
    admission=[]
    for case in initial['cases']:
        begin,end=case['ranges']['calls'];calls=initial['calls'][begin:end]
        requests=[r for r in calls if r['entry']=='000e7f30']
        assert len(requests)==1
        raw=bytes.fromhex(requests[0]['packets'][0]['raw_hex'])
        assert raw==struct.pack('<I',22)+bytes(28)
        gates=[r for r in calls if r['entry']=='001055f0'];assert len(gates)==1
        gate=gates[0];state=initial['states'][gate['before_state']]
        weapon=next(o for o in state['objects'] if o['datum']=='e2700001')
        assert struct.unpack_from('<i',bytes.fromhex(weapon['body_hex']),0x178)[0]==9
        assert int(gate['return_cpu']['registers']['eax'],16)&255==1
        assert not any(r['entry'] in ['000e8d60','000e8f30','000efee0','00115650'] for r in calls if r['phase'].endswith('_player'))
        assert case['final']['counts']==[2,0] and case['final']['active_words']==['00000000','00000000']
        assert case['final']['held_object']=='ffffffff' and case['final']['held_type']==-1
        stop=case['stages'][-1]
        assert stop['completion']=='blocked_original_instruction'
        assert stop['invalid']['instruction']=='0013eafa' and stop['invalid']['address']=='00000034'
        admission.append(dict(case=case['name'],request_type=22,own_weapon_state=9,actual_gate_return_al=1,counts=case['final']['counts'],stop=stop['invalid']))
    assert initial['cases'][1]['source_count_scan_selector']==0
    assert initial['cases'][1]['final']['current']==initial['cases'][1]['final']['requested']==0
    texture=json.loads((BASE/'texture-context/report.json').read_text())
    assert texture['cases']==[]
    stages=texture['constructor_stages'];assert len(stages)==2
    assert stages[0]['completion']=='complete_original_return'
    assert stages[0]['after_cache']['texture_cache']=='0930a000'
    assert stages[1]['completion']=='blocked_original_instruction'
    stop=stages[1]['invalid']
    assert stop['instruction']=='800000b6' and stop['kernel_ordinal']==182
    assert stop['original_args']==['80061000','02ce6000','00000404']
    assert stop['original_return_site']=='0012d9c3' and stop['source_or_platform_result_supplied'] is False
    assert image.read(0x417ba8,4)==struct.pack('<I',0x800000b6)
    assert stages[1]['after_cache']['texture_storage']=='00000000'
    assert [int(v['original_args'][1],16) for v in texture['texture_raw_allocations']]==[0x508c,0x6a8,0x30fc]
    assert all(v['actual_host_mmap_storage'] and v['source_constructor_or_admission_result_supplied'] is False for v in texture['texture_raw_allocations'])
    prior_path=ROOT/'analysis/multiplayer/integration/scope-bitmap-geometry/manifest.json'
    assert motion.digest(prior_path)=='f76b0d90db47dbbd0d38e8c521ca08aab684a6d92a0c0e388f946fb459120535'
    prior=json.loads(prior_path.read_text());prior_pins={**prior['source_files'],**prior['verified_prior_pins']}
    for name,digest in prior_pins.items():assert motion.digest(ROOT/name)==digest,name
    for name in ['tools/oracle_mp_grenade_action_1_1.py','tools/oracle_mp_grenade_texture_context_1_1.py','tools/audit_mp_grenade_action_1_1.py','docs/MP_GRENADE_ACTION_SOURCE_RESEARCH.md']:
        files[name]=motion.digest(ROOT/name)
    report=dict(schema=1,original_xbe_sha256=PIN,source_instruction_rows=checked,source_files=files,verified_prior_pins=prior_pins,
                previous_geometry_manifest_sha256=motion.digest(prior_path),actual_admission=admission,texture_reset_stop=stop,
                scope='Negative actual-player grenade request and genuine texture-owner constructor/protection stop only. No positive throw/clock/count/trajectory/explosion/device/texture-readiness result or native integration is claimed.')
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(audit_sha256=motion.digest(args.output),instruction_rows=checked,source_files=len(files),prior_pins=len(prior_pins))))

if __name__=='__main__':main()

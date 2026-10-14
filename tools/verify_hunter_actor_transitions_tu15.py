#!/usr/bin/env python3
"""Verify TU1.5 cold Hunter creation, binding and declared no-squad birth.

The input world runs the original AI storage and campaign new-map owners,
creates the authored Hunter body/cannon and a nearby local player, then runs
60 neutral object ticks. Cold creation, binding and1E8410 birth are explicit
calls. Birth declares squad NONE and resolves a navigation location. These
comparisons do not establish production squad birth, AI decisions, firing
input, complete campaign boot, renderer integration or physical Xbox parity.
"""
import argparse,json,struct
from pathlib import Path
from audit_rendering import file_sha
from oracle_charged_effect_lifetime_bank_tu15 import stores,regions
from verify_particle_frame_evidence_tu15 import equal_banks
from validate_actor_constructor_bank_tu15 import validate

ROOT=Path(__file__).resolve().parents[1]

def compare(case,original):
    metadata=json.loads((case/'native.json').read_text())
    context=validate(case,metadata,list(regions(case/'before.bin')))
    report=json.loads((original/'report.json').read_text())
    hardware=json.loads((original/'hardware/report.json').read_text())
    assert report['completed'] and report['entry']==metadata['entry']
    assert not report['instruction_patches'] and not report['supplied_game_services']
    assert report['origin_case']==str(case) and report['origin_bank_sha256']==file_sha(case/'before.bin')
    assert hardware['original_report_sha256']==file_sha(original/'report.json')
    # Unicorn omits precision status on the birth continuation. Native must
    # match the complete hardware exit, including x87 status and MXCSR.
    assert hardware['emulator_retained_state_matches_hardware']
    assert (case/'native-trace.bin').read_bytes()==(original/'original-trace.bin').read_bytes()
    retained=bytearray()
    for pc,address,before,after in stores(case/'native-stores.bin'):
        retained.extend(struct.pack('<III',pc,address,len(after))+before+after)
    assert retained==(original/'original-stores.bin').read_bytes()
    exit_state=bytearray((original/'hardware/hardware-exit.bin').read_bytes())
    assert len(exit_state)==180 and exit_state[161]&2
    exit_state[161]&=0xfd
    assert (case/'native-exit.bin').read_bytes()==exit_state
    compared=equal_banks(case/'after.bin',original/'hardware/hardware-after.bin')
    assert compared==58970198
    rows=list(regions(case/'after.bin'))
    def u(address,n=4):
        for p,b in rows:
            if p<=address and address+n<=p+len(b):return int.from_bytes(b[address-p:address-p+n],'little')
        raise AssertionError(hex(address))
    actor=metadata['actor'];pool=u(0x5020c8);body=u(pool+0x44)+(actor&65535)*0x888
    assert actor!=0xffffffff and u(body,2)==actor>>16 and u(pool+0x3c)==1
    objects=u(0x4ecbd0);unit=u(u(objects+0x44)+(metadata['unit']&65535)*12+8)
    birth=metadata['entry']==0x1e8410
    bound=metadata['entry'] in (0x1e9000,0x1e8410)
    assert u(body+0x18)==(metadata['unit'] if bound else 0xffffffff)
    assert u(unit+0x12c)==(actor if bound else 0xffffffff)
    if birth:
        if metadata['declared_squad']==0xffffffff:
            assert u(u(0x5020a8)+0x14)==actor and u(body+0x30)==0xffffffff
        else:
            squad=context['authored_squad'];assert squad and u(body+0x30)==squad['index']
            assert u(squad['record']+0x68)==actor and u(u(0x5020a8)+0x14)==0xffffffff
            assert u(unit+0x138,2)==u(body+0x24,2)
        assert u(body+0x254,2)!=0xffff
    return dict(entry=metadata['entry'],source_case=str(case),context=context,
        source_build_sha256=file_sha(case/'build.json'),original_report_sha256=file_sha(original/'report.json'),
        hardware_report_sha256=file_sha(original/'hardware/report.json'),instructions=report['original_instructions'],
        retained_writes=report['retained_stores'],retained_bytes=compared,trace_exact=True,
        ordered_writes_exact=True,full_exit_exact_except_linux_IF=True,actor=actor,actor_bound=bound,
        birth_admitted=birth,declared_squad=metadata.get('declared_squad'),passed=True)

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--base',type=Path,default=ROOT/'analysis/weapons-tu15-goal-20261013')
    ap.add_argument('--output',type=Path,required=True);a=ap.parse_args()
    base=a.base.resolve();assert not a.output.exists()
    case=base/'hunter-actor-bind-native-v1'
    comparisons=[compare(case,base/'hunter-bound-actor-constructor-original-v1'),
                 compare(case/'actor-bind',base/'hunter-actor-unit-binding-original-v1'),
                 compare(base/'hunter-actor-birth-native-v4',base/'hunter-actor-birth-original-v2')]
    result=dict(scope=__doc__,tool_sha256=file_sha(Path(__file__)),comparisons=comparisons,
        actual_ai_storage_and_new_map_called=True,declared_no_squad_birth_admitted=True,
        full_production_actor_birth_verified=False,
        production_ai_fire_input_verified=False,complete_campaign_boot_verified=False,
        renderer_integration_verified=False,physical_xbox_verified=False,full_weapon_parity=False,passed=True)
    a.output.write_text(json.dumps(result,indent=2,sort_keys=True)+'\n')
    print(json.dumps(dict(passed=True,comparisons=len(comparisons),instructions=[r['instructions'] for r in comparisons],writes=[r['retained_writes'] for r in comparisons])))

if __name__=='__main__':main()

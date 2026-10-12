#!/usr/bin/env python3
"""Audit the independently repeated, bounded sandbox metadata/scalar evidence."""
import hashlib,json,struct
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,ROOT,PIN,_Code
from inspect_multiplayer import Cache
from inspect_mp_sandbox_source import sha
OUT=ROOT/'analysis/multiplayer/sandbox-source'
def main():
    suite=json.loads((OUT/'frozen/suite.json').read_text())
    for z in suite['artifacts']:assert sha(ROOT/z['path'])==z['sha256'],z['path']
    x=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');assert sha(x.path)==PIN
    inventory=json.loads((OUT/'asset-inventory.json').read_text());copy=json.loads((OUT/'asset-inventory-repeat.json').read_text());assert inventory==copy
    record_count=0
    def check_records(v,c):
        nonlocal record_count
        if isinstance(v,dict):
            if {'file_offset','original_va','raw_hex'}<=v.keys():
                p=v['file_offset'];b=bytes.fromhex(v['raw_hex']);assert b==c.data[p:p+len(b)]and int(v['original_va'],16)==p+c.magic;record_count+=1
            for z in v.values():check_records(z,c)
        elif isinstance(v,list):
            for z in v:check_records(z,c)
    for m in inventory['maps']:
        c=Cache(ROOT/m['path']);assert sha(c.path)==m['sha256'];check_records(m,c);c.data.close();c.stream.close()
    a=json.loads((OUT/'lift-scalar/measured/report.json').read_text());b=json.loads((OUT/'lift-scalar/reexecution/report.json').read_text());assert a==b
    assert a['oracle_sha256']==sha(ROOT/'tools/oracle_mp_lift_scalar_1_1.py')and a['xbe_sha256']==PIN
    assert len(a['cases'])==512 and a['original_epsilon_4650e0_hex']==x.read(0x4650e0,4).hex()=='6f12833a'
    for z in a['cases']:
        words=[int(v,16)for v in z['argument_words']];assert words[0]==0 and words[7]==words[8]==0
        assert z['allow_reverse_bl']==1 and z['clock_step_bits']=='3d088889'and z['stack_cleanup']==44
        rule=z['phantom_rule'];assert words[3]==int(rule['fields']['28'if z['component']=='axis'else'30']['bits'],16)
        assert words[5]==int(rule['fields']['2c'if z['component']=='axis'else'34']['bits'],16)
    assert not any(a['boundaries'][v]for v in ('contact_membership_and_source_activation_executed','Havok_body_mass_marker_direction_and_projected_velocity_producers_executed','complete_original_lift_or_collision_result_proven'))
    code=_Code(x,'.text');listed=set();rows=0
    for run in ('measured','reexecution'):
        for p in (OUT/f'lift-scalar/{run}/instructions').glob('*.asm'):
            for l in p.read_text().splitlines():
                at,h,mn,op=(l.split(' ',3)+[''])[:4];at=int(at,16);i=code.at(at)
                assert bytes.fromhex(h)==x.read(at,len(bytes.fromhex(h)))and i.mnemonic==mn and i.op_str==op
                listed.add(at);rows+=1
    assert set(int(v,16)for v in a['executed_addresses'])-{0x3000000}<=listed
    crate=struct.unpack('<32I',x.read(0x46f620,128));assert crate[1]==int.from_bytes(b'bloc','big')and crate[11]==0x11d1c0 and crate[16]==0x11d2b0 and crate[19]==0x11d330
    result={'suite_sha256':sha(OUT/'frozen/suite.json'),'authored_maps':len(inventory['maps']),'relevant_object_definitions':sum(len(m['objects'])for m in inventory['maps']),'raw_metadata_records_verified':record_count,'independent_scalar_cases':len(a['cases']),'original_instruction_rows_verified':rows,'source_contact_activation_world_or_complete_explosion_lift_proven':False,'native_gameplay_update_applied':False}
    (OUT/'frozen/audit.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
if __name__=='__main__':main()

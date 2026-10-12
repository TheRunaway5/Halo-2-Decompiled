#!/usr/bin/env python3
"""Audit the independently repeated, source-only grenade begin prefix."""
import argparse,json,struct
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,PIN
from oracle_mp_grenade_bounded_phases_1_1 import ROOT,OUT,motion

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output=a.output.resolve();assert not a.output.exists()
    image=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
    assert motion.digest(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')==PIN
    files={};checked=0
    for path in (OUT/'measured-complete').rglob('*'):
        if path.is_file():assert path.read_bytes()==(OUT/'repeat-complete'/path.relative_to(OUT/'measured-complete')).read_bytes(),path
    for directory in ('measured-complete','repeat-complete'):
        report=json.loads((OUT/directory/'report.json').read_text())
        assert report['tool_sha256']==motion.digest(ROOT/'tools/oracle_mp_grenade_bounded_phases_1_1.py')
        for name,digest in report['source_pins'].items():assert motion.digest(ROOT/name)==digest,name
        assert not report['new_supplied_gameplay_fields'] and not report['new_supplied_source_returns'] and not report['instruction_patches']
        for listing in report['instructions']:
            path=OUT/directory/listing['file'];assert motion.digest(path)==listing['sha256']
            for line in path.read_text().splitlines():
                address,raw,_=line.split(' ',2);data=bytes.fromhex(raw);assert image.read(int(address,16),len(data))==data
                checked+=1
        for path in (OUT/directory).rglob('*'):
            if path.is_file():files[str(path.relative_to(ROOT))]=motion.digest(path)
    report=json.loads((OUT/'measured-complete/report.json').read_text());case=report['cases'][0]
    assert len(report['cases'])==1 and case['neutral_phases_before_Q']==1 and len(case['rows'])==1
    assert [row['flags'] for row in case['inputs']]==['00000000','04000000']
    assert case['source_selector']==0 and case['actual_weapon_datum']=='e2700001'
    first=case['rows'][0]
    assert report['states'][first['before_state']]['carried_weapon_own_states'][0]['state_178']=='00000009'
    assert report['states'][first['after_weapon']]['carried_weapon_own_states'][0]['state_178']=='00000000'
    assert all(state['clock']==report['states'][0]['clock'] for state in report['states'])
    state_write=next(row for row in report['stores'] if row['address']=='82d897c8' and row['before_hex']=='09000000' and row['written_hex']=='00000000')
    assert state_write['instruction']=='00109b82'
    calls=[row for row in report['calls'] if row['phase'].endswith('phase_1_player')]
    gate=next(row for row in calls if row['entry']=='001055f0');assert int(gate['return_cpu']['registers']['eax'],16)&255==0
    select=next(row for row in calls if row['entry']=='00115650');assert int(select['return_cpu']['registers']['eax'],16)&255==1
    output=next(row for row in select['after_packets'] if row['field']=='stack3_selected_original_channel_output4')
    selected_pointer=struct.unpack('<I',bytes.fromhex(output['raw_hex']))[0]
    state=report['states'][select['after_state']];unit=next(row for row in state['objects'] if row['datum']=='e26f0000')
    body=bytes.fromhex(unit['body_hex']);offset=selected_pointer-int(unit['body_address'],16)
    graph,identifier,index=struct.unpack_from('<Ihh',body,offset)
    assert (graph,identifier,index)==(0xe9180793,-1,219)
    assert select['return_esp_delta']==20
    begin=next(row for row in calls if row['entry']=='000e95e0')
    assert 'return_cpu' not in begin and begin['id'] in case['unreturned_original_calls']
    assert not any(row['entry'] in ('000e8d60','000b8a30','000e99b0') for row in calls)
    final=case['final']['grenade']
    assert final['phase']==1 and final['timer']==0 and final['release_timer']==8
    assert final['counts']==[2,0] and final['active_words']==['00000000','00000000']
    assert final['held_object']=='ffffffff' and final['held_type']==-1
    stop=case['stages'][-1];assert stop['completion']=='blocked_original_instruction'
    assert stop['invalid']['instruction']=='0016d2c3' and stop['invalid']['address']=='00000034'
    assert stop['invalid']['registers']['edx']==stop['invalid']['registers']['ebp']=='00000000'
    assert stop['invalid']['registers']['edi']=='e7a60621'
    # These original bytes are static local authority for the precise observed
    # failing pool load/caller. They do not claim the whole routine returned.
    import capstone
    cs=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_32)
    for name,start,length in [('effect-pool-request-prefix',0x179b90,0x55),('datum-pool-stop-prefix',0x16d2c0,0x15)]:
        instructions=list(cs.disasm(image.read(start,length),start));text=''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n' for i in instructions)
        path=a.output.parent/'instructions'/(name+'.asm');motion.frozen_write(path,text.encode());files[str(path.relative_to(ROOT))]=motion.digest(path)
    previous=ROOT/'analysis/multiplayer/integration/grenade-action/manifest.json';assert motion.digest(previous)=='cb72c6ec1bfc4e5045c6bb537dc6f5e4fc91123616b18abbcfb5e5b0277493bf'
    prior=json.loads(previous.read_text());pins={**prior['source_files'],**prior['verified_prior_pins']}
    for name,digest in pins.items():assert motion.digest(ROOT/name)==digest,name
    metadata_path='analysis/multiplayer/integration/inventory-source/spartan-owned-pose-measured/report.json'
    metadata=json.loads((ROOT/metadata_path).read_text());clip=metadata['metadata']['animations'][219]
    assert clip['index']==219 and clip['name']=='combat:rifle:throw_grenade:var0';files[metadata_path]=motion.digest(ROOT/metadata_path)
    for name in ('tools/oracle_mp_grenade_bounded_phases_1_1.py','tools/audit_mp_grenade_bounded_phases_1_1.py','docs/MP_GRENADE_BOUNDED_PHASE_SOURCE_RESEARCH.md'):files[name]=motion.digest(ROOT/name)
    value=dict(schema=1,original_xbe_sha256=PIN,source_instruction_rows=checked,source_files=files,verified_prior_pins=pins,
        previous_grenade_manifest_sha256=motion.digest(previous),source_state_write=state_write,selected_channel=dict(graph=f'{graph:08x}',identifier=identifier,index=index,clip=clip),
        genuine_stop=stop,scope='Original grenade begin prefix after one explicitly composed original unit/weapon phase, with the unchanged fixed clock. Callback does not return at the unavailable effect pool. No admitted completed action, held projectile, count debit, release, continuous clock/world, graphics result or native gameplay integration is claimed.')
    motion.frozen_write(a.output,motion.encoded(value));print(json.dumps(dict(audit_sha256=motion.digest(a.output),rows=checked,source_files=len(files),prior_pins=len(pins))))

if __name__=='__main__':main()

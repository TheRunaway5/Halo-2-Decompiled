#!/usr/bin/env python3
"""Independently audit byte authority and unchanged prior HUD evidence."""
import argparse
import json
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT, Xbe, PIN as TU_PIN
from oracle_multiplayer_hud_positions import PIN as BASE_PIN
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from oracle_mp_scope_bitmap_geometry import sha

BASE=ROOT/'analysis/multiplayer/integration/scope-bitmap-geometry'
SOURCES=['tools/oracle_mp_scope_bitmap_geometry.py','tools/research_mp_scope_bitmap_updates.py',
         'tools/export_mp_scope_bitmap_fixtures.py','tools/audit_mp_scope_bitmap_geometry.py',
         'include/halo2/hud_bitmap_geometry.hpp','src/ui/hud/hud_bitmap_geometry.cpp','tests/hud_bitmap_geometry_tests.cpp',
         'docs/MP_SCOPE_BITMAP_GEOMETRY_SOURCE_RESEARCH.md']

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();assert not args.output.exists()
    paths=[ROOT/'assets/disc/default.xbe',ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03',NEW_DEFAULT]
    pins=[BASE_PIN,TU_PIN,NEW_PIN]
    assert [sha(p) for p in paths]==pins
    images=[Xbe(p) for p in paths]
    instruction_files={}
    checked=0
    def listing(path,image):
        nonlocal checked
        key=str(path.relative_to(ROOT))
        if key in instruction_files:return
        for line in path.read_text().splitlines():
            address,raw,_=line.split(' ',2)
            data=bytes.fromhex(raw)
            assert image.read(int(address,16),len(data))==data,(path,address)
            checked+=1
        instruction_files[key]=sha(path)
    for left,right in [('measured-complete','repeat-complete'),('update-measured','update-repeat')]:
        for path in (BASE/left).rglob('*'):
            if not path.is_file() or path.name=='fixtures.tsv':continue
            other=BASE/right/path.relative_to(BASE/left)
            assert path.read_bytes()==other.read_bytes(),path
    measured=json.loads((BASE/'measured-complete/report.json').read_text())
    assert measured['original_base_xbe_sha256']==BASE_PIN
    assert measured['tool_sha256']==sha(ROOT/'tools/oracle_mp_scope_bitmap_geometry.py')
    for path,digest in measured['preserved_dependencies'].items():assert sha(ROOT/path)==digest
    cases=measured['cases'];assert len(cases)==152
    authored=[c for c in cases if 'explicitly_modified_widget_flag_word' not in c]
    controls=[c for c in cases if 'explicitly_modified_widget_flag_word' in c]
    assert len(authored)==120 and len(controls)==32
    assert [c['explicitly_modified_widget_flag_word'] for c in controls]==list(range(32))
    for case in cases:
        flags=case.get('explicitly_modified_widget_flag_word',case['authored_widget']['flags'])
        assert len(case['original_draw_requests'])==1+bool(flags&4)+bool(flags&8)+((flags&12)==12)
        if case in authored and case['authored_widget']['name']=='binocular_mask':
            assert flags==0x1c
            assert bytes.fromhex(case['raw_widget_hex'])[0x1e:0x20]==bytes.fromhex('1c00')
            assert [r['caller_return_site'] for r in case['original_draw_requests']]==['0022b695','0022b701','0022b754','0022b7d3']
            assert len({r['sprite_bounds10_hex'] for r in case['original_draw_requests']})==1
    assert sum(len(c['original_draw_requests']) for c in cases)==312
    for folder in ['measured-complete','repeat-complete']:
        for row in measured['instructions']:
            path=BASE/folder/row['file'];assert sha(path)==row['sha256'];listing(path,images[0])
    comparison=json.loads((BASE/'update-measured/report.json').read_text())
    assert comparison['tool_sha256']==sha(ROOT/'tools/research_mp_scope_bitmap_updates.py')
    for side,pair in enumerate(comparison['updates'],1):
        assert pair['xbe_sha256']==pins[side] and len(pair['routines'])==29
        for row in pair['routines']:
            assert row['complete_instruction_and_internal_branch_shape_equal']
            assert all(t['mapped_contents_equal'] for t in row['switch_tables'])
            assert all(v['both_in_images'] for v in row['changed_nonbranch_immediates'])
            for folder in ['update-measured','update-repeat']:
                for label,image in [('base',images[0]),(pair['update'],images[side])]:
                    info=row[label+'_listing'];path=BASE/folder/info['file']
                    assert sha(path)==info['sha256'];listing(path,image)
        geometry=next(r for r in pair['routines'] if r['base_entry']=='0022aff1')
        values=[r for r in geometry['paired_image_operands'] if r['base_address'] in ['0045dc00','0045dc0c']]
        assert len(values)==5 and all(r['bytes_equal'] for r in values)
    prior_path=ROOT/'analysis/multiplayer/integration/scope-presentation/manifest.json'
    assert sha(prior_path)=='23b8c47b565ba0a87a85f4e628b0cecd85c894ab42fce863d6eb525952c19e5d'
    prior=json.loads(prior_path.read_text())
    prior_pins={**prior['source_files'],**prior['verified_prior_pins']}
    for name,digest in prior_pins.items():assert sha(ROOT/name)==digest,('prior_source_changed',name)
    report=dict(schema=1,original_xbe_pins=pins,source_instruction_rows=checked,
                instruction_files=instruction_files,source_files={p:sha(ROOT/p) for p in SOURCES},
                actual_authored_cases=120,explicit_flag_controls=32,ordered_draw_requests=312,
                previous_scope_manifest_sha256=sha(prior_path),verified_prior_pins=prior_pins,
                scope='Original CPU geometry only; actual original void graphics consumer excluded at checked ABI; no local-session, GPU/device/cache/shader completion or final Xbox pixels claimed.')
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(audit_sha256=sha(args.output),instruction_rows=checked,instruction_files=len(instruction_files),prior_pins=len(prior_pins))))

if __name__=='__main__':main()

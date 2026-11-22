#!/usr/bin/env python3
"""Compare integer-only Havok reclamation selection with unchanged TU1.5."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess

from audit_native_havok_statistics_tu15 import (NEW_DEFAULT, NEW_PIN, ROOT, STACK, STOP, WORLD,
                                                Xbe, observe_original, original_row, put, set_words)
from validate_native_memory_pressure_tu15 import POOL, cases as pressure_cases


NONE = 0xffffffff
BODY = WORLD + 0x3000
PROPERTY = WORLD + 0x3800
COMPONENT = WORLD + 0x1300
OBJECT_HEADER = WORLD + 0x900
OBJECT_BODY = WORLD + 0x2000
SELECTION = 0x1ccbe0
PREDICATE = 0x1ccad0
SELECTED = 0x1cccdc
OBJECT = 0xe3a00000
PRESSURE = 0x1496f0
CLEANUP = 0x1cb330


def build_case(name, index, seed, *, predicate=False, mode=0, aggressive=False,
               recent=False, final=False, excluded=NONE, game_mode=1, edits=(), bytes_=(),
               pressure_boundary=None):
    regions = {bank['address']: bytearray.fromhex(bank['before']) for bank in seed['banks']}
    regions.update({base: bytearray(size) for base, size in
                    ((0x4ec000, 4096), (0x4f3000, 4096), (0x52b000, 4096),
                     (WORLD, 16384), (STACK, 32768))})
    def words(address, *values):
        set_words(regions, address, *values)

    words(0x52be48, WORLD)
    words(0x52be5c, WORLD + 0x1200)
    words(0x4ecbd0, WORLD + 0x800)
    words(0x4f3218, WORLD + 0x780)
    words(WORLD + 0x78c, game_mode)
    words(WORLD + 0x844, OBJECT_HEADER)
    words(WORLD + 0x1244, COMPONENT)
    for n in range(6):
        words(COMPONENT+n*0xa0+8, OBJECT+n)
        words(OBJECT_HEADER+n*12, 0, 0, OBJECT_BODY+n*0x200)
        put(regions, OBJECT_HEADER+n*12+2, bytes((1, 11)))
        words(OBJECT_BODY+n*0x200+4, 0x4000)
        words(OBJECT_BODY+n*0x200+0xd4, NONE)
        words(OBJECT_BODY+n*0x200+0x13c, NONE)
        words(BODY+n*0x100+0x30, PROPERTY+n*0x20, 1)
        words(BODY+n*0x100+0x8c, n)
        words(PROPERTY+n*0x20, 0x2001, 0x12340000+n)
    words(WORLD+0x08, WORLD+0x100, 1)
    words(WORLD+0x14, WORLD+0x110, 2)
    words(WORLD+0x2c, WORLD+0x200)
    words(WORLD+0x100, WORLD+0x400)
    words(WORLD+0x110, WORLD+0x300, WORLD+0x500)
    for island, array, bodies in ((0x200,0x600,(0,1)), (0x300,0x620,()),
                                   (0x400,0x640,(4,5)), (0x500,0x660,(2,3))):
        words(WORLD+island+0x3c, WORLD+array, len(bodies))
        words(WORLD+array, *(BODY+n*0x100 for n in bodies))
    for address, values in edits:
        words(address, *values)
    for address, raw in bytes_:
        put(regions, address, raw)

    rounding=(index%4)<<10
    x87=('0000000000000080ff3f',) if index%2 else ()
    if pressure_boundary:
        # A live primary pool at level 2 selects reclamation without the
        # already qualified level-3 statistics phase. NONE is fresh cleanup.
        words(POOL+0xf0384,50000)
        words(0x486058,NONE)
    entry=PRESSURE if pressure_boundary else PREDICATE if predicate else SELECTION
    row=original_row(name,index,regions,entry,
                     [0x027f|rounding,((index%8)<<11)|0x0120,0x1f80|(rounding<<3)],
                     argument=int(aggressive),live_x87=x87)
    row.update(source=int(recent), stack_arguments=[excluded],
               request=dict(mode=mode,excluded=excluded,aggressive=aggressive,
                            recent_cleanup=recent,final_pass=final))
    if pressure_boundary:
        row['pressure_boundary']=pressure_boundary
        row['stop_before_entries']=[pressure_boundary]
    elif predicate:
        row['gp'][0]=row['body']=BODY
        row['gp'][3]=mode
    else:
        row['gp'][0]=(row['gp'][0]&~255)|int(final)
        row['gp'][1]=mode
        row['stop_before_entries']=[STOP,SELECTED]
    return row,tuple(sorted((base,len(bank)) for base,bank in regions.items()))


def cases(image):
    seed=next(case for case in pressure_cases(image) if case['name']=='frame_free_1')
    result=[]
    layouts=set()
    def add(name, **kw):
        row,layout=build_case(name,len(result),seed,**kw)
        result.append(row)
        layouts.add(layout)
    def predicate(name, **kw):
        add(name,predicate=True,**kw)

    predicate('missing_key_defaults_to_component_zero', edits=((PROPERTY,(0x2002,4)),))
    predicate('no_properties_defaults_to_component_zero', edits=((BODY+0x34,(0,)),))
    predicate('signed_negative_property_count_defaults_to_zero',
              edits=((BODY+0x34,(0x80000000,)),))
    predicate('present_none_is_rejected', edits=((PROPERTY,(0x2001,NONE)),))
    predicate('second_property_key_is_found', edits=((BODY+0x34,(2,)),
              (PROPERTY,(0x2002,3,0x2001,0x12340002))))
    predicate('first_duplicate_property_wins', edits=((BODY+0x34,(2,)),
              (PROPERTY,(0x2001,0x12340002,0x2001,0x12340003))))
    predicate('excluded_component_datum', excluded=0x12340000)
    predicate('same_index_different_salt_is_not_excluded', excluded=0x22340000)
    predicate('object_datum_is_not_an_excluded_component', excluded=OBJECT)
    for recent in (False,True):
        predicate(f'protected_campaign_recent_{recent}',recent=recent,game_mode=4,
                  edits=((OBJECT_BODY+0xd4,(1,)),))
    predicate('same_link_in_noncampaign_is_unprotected',edits=((OBJECT_BODY+0xd4,(1,)),))
    for aggressive in (False,True):
        predicate(f'inactive_object_aggressive_{aggressive}',aggressive=aggressive,
                  bytes_=((OBJECT_HEADER+2,b'\x00'),))
    for type_ in (0,1,7,11,43):
        for mode in (0,1,2,0xffffffff):
            predicate(f'type_{type_}_mode_{mode:x}',mode=mode,
                      edits=((OBJECT_BODY+4,(0,)),),bytes_=((OBJECT_HEADER+3,bytes([type_])),))
    predicate('linked_biped_is_ineligible',mode=2,
              edits=((OBJECT_BODY+4,(0,)),(OBJECT_BODY+0x13c,(1,))),
              bytes_=((OBJECT_HEADER+3,b'\x00'),))

    add('inactive_over_fixed_by_priority')
    add('active_pass_ignores_fixed_and_inactive',final=True)
    add('fixed_first_wins_priority_tie',
        edits=tuple((BODY+n*0x100+0x8c,(4,)) for n in range(4)))
    add('signed_comparison_prefers_positive_to_negative',
        edits=tuple((BODY+n*0x100+0x8c,(value,))
                    for n,value in enumerate((0xffffffff,0xfffffffe,0,0x80000000))))
    add('min_signed_priorities_are_never_selected',
        edits=tuple((BODY+n*0x100+0x8c,(0x80000000,)) for n in range(4)))
    add('no_candidate_after_full_visit',
        edits=tuple((PROPERTY+n*0x20,(0x2001,NONE)) for n in range(4)))
    add('highest_ineligible_does_not_mask_lower_candidate',
        edits=((PROPERTY+3*0x20,(0x2001,NONE)),))
    add('no_active_candidate_even_with_inactive_candidates',final=True,
        edits=((WORLD+0x0c,(0,)),))
    add('candidate_full_component_datum_exclusion',excluded=0x12340003)
    add('active_and_inactive_arrays_empty',
        edits=((WORLD+0x0c,(0,)),(WORLD+0x18,(0,)),(WORLD+0x240,(0,))))
    add('fresh_pressure_without_candidate_reaches_real_cleanup',pressure_boundary=CLEANUP,
        edits=tuple((PROPERTY+n*0x20,(0x2001,NONE)) for n in range(4)))
    add('pressure_with_candidate_stops_before_selected_object_mutation',pressure_boundary=SELECTED)
    if len(layouts)!=1:
        raise ValueError('Physical selection fixtures disagree on memory layout')
    return result,layouts.pop()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report',type=Path,required=True)
    args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    image=Xbe(NEW_DEFAULT)
    if hashlib.sha256(image.data).hexdigest()!=NEW_PIN:
        raise ValueError('Original TU1.5 executable identity differs')
    rows,regions=cases(image)
    proof,authority=observe_original(image,rows,scratch,
        entries=(PREDICATE,SELECTION,0xb1870,PRESSURE,0x149ad0,0x2335b0,0x1498a0),regions=regions)

    fixture=[]
    outcomes={}
    for row, observed in zip(rows,proof['cases'],strict=True):
        pressure_boundary=row.get('pressure_boundary')
        if not observed['completed'] or (not pressure_boundary and observed['after_fp']!=row['before_fp']):
            raise ValueError('The original selection did not reach its exact integer boundary')
        writes=[[event['address'],int(event['before'],16),int(event['written'],16)]
                for event in observed['writes'] if not STACK<=event['address']<STACK+32768]
        if writes!=([[0x48b873,0,1]] if pressure_boundary else []):
            raise ValueError('The original changed retained data outside its pressure guard')
        eax,_,_,_,_,_,_,edi=observed['boundary_gp']
        if pressure_boundary:
            if observed['stopped_at']!=pressure_boundary:
                raise ValueError('The original pressure parent reached the wrong owner')
            selected=edi if pressure_boundary==SELECTED else NONE
            outcome='pressure selected handoff' if pressure_boundary==SELECTED else 'real cleanup handoff'
        elif row['entry']==PREDICATE:
            selected=eax
            outcome='predicate accepted' if selected!=NONE else 'predicate rejected'
        elif observed['stopped_at']==SELECTED:
            selected=edi
            outcome='selected handoff'
        else:
            if observed['stopped_at']!=STOP or eax&255:
                raise ValueError('The original did not return AL=0 on its no-candidate branch')
            selected=NONE
            outcome='AL=0 return'
        outcomes[outcome]=outcomes.get(outcome,0)+1
        banks=[]
        for before,after in zip(row['banks'],observed['banks'],strict=True):
            if before['address']==STACK:
                continue
            if before['address']!=0x48b870 and before['before']!=after['after']:
                raise ValueError('The physical selector changed a retained bank')
            banks.append(before)
        reads=[[event['address'],event['bytes']] for event in observed['reads']
               if not STACK<=event['address']<STACK+32768]
        item=dict(name=row['name'],banks=banks,request=row['request'],selected=selected,reads=reads)
        if 'body' in row:
            item['body']=row['body']
        if pressure_boundary:
            item.update(pressure_boundary=pressure_boundary,before_fp=row['before_fp'],
                        after_fp=observed['after_fp'],writes=writes)
        fixture.append(item)
    path=scratch/'reclamation-selection-fixture.json'
    path.write_text(json.dumps(dict(image_sha256=NEW_PIN,cases=fixture),separators=(',',':')))
    sources=('src/physics/native_tu15_memory_pressure.cpp',
             'include/halo2/native_tu15_memory_pressure.hpp',
             'include/halo2/native_tu15_object_delete_protection.hpp',
             'src/physics/native_tu15_physics_frame_residuals.cpp',
             'src/gameplay/objects/native_object_factory_placement_tu15.cpp',
             'tests/native_tu15_havok_reclamation_selection_tests.cpp')
    command=['g++','-std=c++20','-O1','-ffunction-sections','-fdata-sections','-Wl,--gc-sections',
             '-fno-fast-math','-ffp-contract=off','-frounding-math','-Iinclude','-Isrc',
             *(source for source in sources if source.endswith('.cpp') and
               not source.endswith('native_tu15_physics_frame_residuals.cpp')),
             '-o',str(scratch/'reclamation-selection')]
    build=subprocess.run(command,cwd=ROOT,capture_output=True,text=True,check=False)
    result=build if build.returncode else subprocess.run(
        [str(scratch/'reclamation-selection'),str(path)],capture_output=True,text=True,check=False)
    evidence_sources=(*sources,str(Path(__file__).resolve().relative_to(ROOT)))
    report=dict(passed=result.returncode==0,original_xbe_sha256=NEW_PIN,authority=authority,
        original_outcomes=outcomes,retained_reads=sum(len(row['reads']) for row in fixture),
        retained_selector_writes=0,original_selector_floating_point_unchanged=True,
        original_instruction_patches=[],supplied_original_outcomes=False,
        source_sha256={source:hashlib.sha256((ROOT/source).read_bytes()).hexdigest()
                       for source in evidence_sources},
        compiler_command=command,detail=(result.stdout+result.stderr)[-4000:],
        owned_scratch=str(scratch),retained_large_artifacts=[])
    args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(report['detail'],end='')
    return result.returncode


if __name__=='__main__':
    raise SystemExit(main())

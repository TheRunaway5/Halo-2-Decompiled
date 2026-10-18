#!/usr/bin/env python3
"""Reject unauthenticated Cairo facts without needing donor game assets."""
import copy
import importlib.util
import json
from pathlib import Path
import unittest

ROOT=Path(__file__).resolve().parents[1]
SPEC=importlib.util.spec_from_file_location('cairo_generator',ROOT/'tools/generate_mcc_cairo_vanilla_authoring.py')
GENERATOR=importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(GENERATOR)
PROFILE=json.loads((ROOT/'tools/mcc-cairo-vanilla-authoring.json').read_text())


class CairoFactsTests(unittest.TestCase):
    def rejected(self,mutate):
        changed=copy.deepcopy(PROFILE)
        mutate(changed)
        with self.assertRaises(ValueError):
            GENERATOR.generated(changed)

    def test_current_generated_facts(self):
        self.assertEqual(GENERATOR.generated(PROFILE),(ROOT/'src/mcc_cairo_vanilla_authoring.inc').read_text())

    def test_sound_call_namespaces_and_owners(self):
        for i,sound in enumerate(PROFILE['sound_removals']):
            for field,value in sound.items():
                if field=='rows':
                    continue
                with self.subTest(call=i,field=field):
                    replacement=value+'custom' if isinstance(value,str) else value^1
                    self.rejected(lambda p:p['sound_removals'][i].update({field:replacement}))

    def test_every_typed_sound_row_field(self):
        for i,sound in enumerate(PROFILE['sound_removals']):
            for j,row in enumerate(sound['rows']):
                for field,value in row.items():
                    with self.subTest(call=i,row=j,field=field):
                        self.rejected(lambda p:p['sound_removals'][i]['rows'][j].update({field:value^1}))

    def test_merged_splices_and_complete_retirement(self):
        for i in range(13,20):
            for field,value in PROFILE['patches'][i].items():
                with self.subTest(patch=i,field=field):
                    self.rejected(lambda p:p['patches'][i].update({field:value^1}))
        self.rejected(lambda p:p['retired_nodes'].__setitem__(0,p['retired_nodes'][0]^1))
        self.rejected(lambda p:p['sound_removals'].pop())
        self.rejected(lambda p:p['sound_removals'][0]['rows'].pop())
        self.rejected(lambda p:p['sound_removals'].append(copy.deepcopy(p['sound_removals'][0])))
        self.rejected(lambda p:p['sound_removals'][0]['rows'].append(copy.deepcopy(p['sound_removals'][0]['rows'][0])))

    def test_original_timing_allocator_and_function_slots(self):
        block='final_holder_restoration'
        for i,restore in enumerate(PROFILE[block]['rewrites']):
            for side in ['before','after']:
                for field,value in restore[side].items():
                    with self.subTest(restore=i,side=side,field=field):
                        self.rejected(lambda p:p[block]['rewrites'][i][side].update({field:value^1}))
            self.rejected(lambda p:p[block]['rewrites'][i].update(allocated=not restore['allocated']))
        for i,slot in enumerate(PROFILE[block]['function_slots']):
            for field in ['offset','size','before_owner']:
                self.rejected(lambda p:p[block]['function_slots'][i].update({field:slot[field]^1}))
            for field in ['before','after_name']:
                self.rejected(lambda p:p[block]['function_slots'][i].update({field:slot[field]+'custom'}))
            for j,owner in enumerate(slot['after_owners']):
                self.rejected(lambda p:p[block]['function_slots'][i]['after_owners'].__setitem__(j,owner^1))
            for j,debug in enumerate(slot['preserved_debug_rows']):
                for field,value in debug['row'].items():
                    self.rejected(lambda p:p[block]['function_slots'][i]['preserved_debug_rows'][j]['row'].update({field:value^1}))
        for i,owner in enumerate(PROFILE[block]['incoming']):
            for field,value in owner.items():
                self.rejected(lambda p:p[block]['incoming'][i].update({field:not value if isinstance(value,bool) else value^1}))
        for name,value in PROFILE[block]['postimages'].items():
            self.rejected(lambda p:p[block]['postimages'].update({name:'0'+value[1:]}))

    def test_existing_authority_guards(self):
        mutations=[
            lambda p:p['removed_scripts'][0].update(name='custom'),
            lambda p:p.update(scenario_address=0),
            lambda p:p['script_references'].append(p['script_references'][0]),
            lambda p:p['patches'][4].update(next=0),
            lambda p:p['banks'][0].update(name=p['banks'][1]['name']),
            lambda p:p['literal_restores'][0].update(after=1201),
            lambda p:p['clock_restore'].update(literal_value=60),
            lambda p:p['clock_restore'].update(label_offset=24),
            lambda p:p['clock_literal_restores'][0].update(other_value=0),
            lambda p:p['effect_brand_restore'].update(new_object_index=25),
            lambda p:p['banks'][-1].update(token=0),
            lambda p:p['removed_final_global'].update(name='custom'),
            lambda p:p['removed_final_global'].update(type=8),
            lambda p:p['removed_final_global'].update(kind=1),
            lambda p:p['removed_final_global']['references'][0].update(opcode=7),
            lambda p:p['removed_final_global'].update(root=0),
        ]
        for i,mutate in enumerate(mutations):
            with self.subTest(case=i):
                self.rejected(mutate)


if __name__=='__main__':
    unittest.main()

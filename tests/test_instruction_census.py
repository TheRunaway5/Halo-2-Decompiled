#!/usr/bin/env python3
"""Regression tests for exact callback-PC census ownership and static edges."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools/perf_4k60'))
from census_common import CensusIndex, attribution_note, load_census


class InstructionCensusTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(dir=os.environ.get('HALO2_TEST_SCRATCH'))
        self.addCleanup(self.temporary.cleanup)
        self.directory = Path(self.temporary.name)

    def unit(self, name, text):
        path = self.directory / name
        path.write_text(text)
        return path

    def test_descriptive_owners_local_services_and_damage_mapping(self):
        path=self.unit('descriptive_generated.cpp','''
void producer(WorldState &s) {
if(s.services.instruction)s.services.instruction(0x1482a0u);
}
void run_status(WorldState &s) {
if(s.services.instruction)s.services.instruction(0x80023653u);
}
void source_run_tu15_power_dpc(M &m,const S &services) {
WorldState s{m,observe,services,SourcePhysicsContactImage::TitleUpdate15};
if(services.instruction)services.instruction(0x2e009du);
}
static void run_100(ObservedDamageState &s) {
if(s.services.instruction)s.services.instruction(damage_observation_pc(0x100u,0x200u,s));
}
''')
        index=CensusIndex.scan(paths=[path])
        for entry in (0x1482a0,0x80023653,0x2e009d,0x200):self.assertEqual(index.owner(entry),entry)

    def test_observed_image_addresses_and_cross_unit_calls(self):
        first = self.unit('world_generated.cpp', '''
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage image){if(image==SourcePhysicsContactImage::MapPack11)return p;switch(p){case 0x200u:return 0x300u;default:throw std::runtime_error("unbound");}}
static void run_100(ObservedWorldState &s) {
/* Braces in a multiline comment are not scopes: {
} */
if (!native_reference()) {
    native_kernel(s);
    return;
}
if(s.services.instruction){s.services.instruction(world_observation_pc(0x100u,0x400u,s));} run_200(s);
if(s.services.instruction){s.services.instruction(world_observation_pc(0x105u,0x409u,s));} {const auto target=selected(0x200u,s.image);dispatch(s,target,0x105u,false);}
if(s.services.instruction){s.services.instruction(world_observation_pc(0x10au,0x40eu,s));} dispatch(s,0x500u,0x10au,true);
if(s.services.instruction){s.services.instruction(world_observation_pc(0x10fu,0x413u,s));} s.required(0x10fu,0x600u,false);
}
static void run_200(VehicleState &s) {
if(s.services.instruction){s.services.instruction(selected(0x200u,s.image));} return;
}
''')
        second = self.unit('other_generated.cpp', '''
static void run_500(WorldState &s) {
if(s.services.instruction){s.services.instruction(0x500u);} return;
}
static void run_600(WorldState &s) {
if(s.services.instruction){s.services.instruction(0x600u);} return;
}
''')
        index = CensusIndex.scan(paths=[first, second])
        self.assertEqual(index.owner(0x409), 0x400)
        self.assertEqual(index.owner(0x300), 0x300)
        self.assertEqual(index.calls[0x400], {0x300, 0x500, 0x600})
        self.assertEqual(index.tree(0x400), {0x400, 0x300, 0x500, 0x600})
        self.assertEqual(index.tree(0x400, {0x400, 0x500}), {0x400, 0x500})
        canonical = CensusIndex.scan(paths=[first], image='map-pack11')
        self.assertEqual(canonical.owner(0x105), 0x100)
        self.assertEqual(canonical.calls[0x100], {0x200, 0x500, 0x600})

    def test_unknown_or_conflicting_pcs_are_never_assigned_by_proximity(self):
        first = self.unit('first_generated.cpp', '''
static void run_100(State &s) {
if(s.services.instruction){s.services.instruction(0x100u);} return;
if(s.services.instruction){s.services.instruction(0x110u);} return;
}
''')
        second = self.unit('second_generated.cpp', '''
static void run_108(WorldState &s) {
if(s.services.instruction){s.services.instruction(0x108u);} return;
if(s.services.instruction){s.services.instruction(0x110u);} return;
}
''')
        index = CensusIndex.scan(paths=[first, second])
        self.assertIsNone(index.owner(0x100000))
        self.assertIsNone(index.owner(0x110))
        self.assertEqual(index.attribute({0x100: 2, 0x100000: 7, 0x110: 3}), {0x100: 2, None: 10})
        self.assertIn('unknown instructions 7; ambiguous instructions 3',
                      attribution_note(index, {0x100000: 7, 0x110: 3}))

    def test_attributed_legacy_functions_require_matching_image(self):
        legacy=self.unit('legacy_generated.cpp','''
struct State : Base { State(M&m,O&o,S&shared):Base{m,o,shared,SourcePhysicsContactImage::MapPack11}{} };
[[gnu::noinline]] static void run_12f5e0(State &s) {
if(s.services.instruction){s.services.instruction(0x12f5e0u);} return;
}
'''.replace(':Base{',':State{'))
        current=self.unit('tu15_generated.cpp','''
void invoke(M&m,O&o,S&shared) { WorldState s{m,o,shared,SourcePhysicsContactImage::TitleUpdate15}; }
[[gnu::noinline]] static void run_130980(WorldState &s) {
if(s.services.instruction){s.services.instruction(0x130980u);} return;
}
''')
        tu15=CensusIndex.scan(paths=[legacy,current])
        self.assertIsNone(tu15.owner(0x12f5e0));self.assertEqual(tu15.owner(0x130980),0x130980)
        self.assertEqual(set(tu15.excluded_image_units.values()),{'map-pack11'})
        old=CensusIndex.scan(paths=[legacy,current],image='map-pack11')
        self.assertEqual(old.owner(0x12f5e0),0x12f5e0);self.assertIsNone(old.owner(0x130980))

    def test_actual_world_add_and_vehicle_routine_aliases(self):
        index = CensusIndex.scan(root=ROOT / 'src', paths=[
            ROOT / 'src/physics/runtime/physics_world_runtime_generated.cpp',
            ROOT / 'src/physics/vehicles/physics_vehicle_update_generated.cpp'])
        self.assertEqual(index.owner(0x2ec4b0), 0x2ec4b0)
        self.assertEqual(index.owner(0x2ec4b3), 0x2ec4b0)
        self.assertIn('physics_world_runtime_generated.cpp', index.units[0x2ec4b0])
        # Do not charge remapped Havok addObject instructions to nearby2EBC70.
        self.assertNotEqual(index.owner(0x2ec4b0), 0x2ebc70)
        self.assertEqual(index.owner(0xb9770), 0xb9770)
        self.assertIn('physics_vehicle_update_generated.cpp', index.units[0xb9770])

    def test_cli_keeps_unknown_counts_and_labels_static_estimates(self):
        self.unit('empty_generated.cpp', '')
        short = self.directory / 'short.txt'
        long = self.directory / 'long.txt'
        short.write_text('100 1\n')
        long.write_text('100 3\n100 2\n')
        self.assertEqual(load_census(long)[0x100], 5)
        for script, args in [('census_map.py', [str(long)]),
                             ('census_roots.py', [str(short), str(long), '2'])]:
            output = subprocess.check_output([sys.executable,
                str(ROOT / 'tools/perf_4k60' / script), *args,
                '--source-root', str(self.directory)], text=True)
            self.assertIn('unknown instructions', output)
            self.assertIn('static', output)
        output = subprocess.check_output([sys.executable,
            str(ROOT / 'tools/perf_4k60/census_roots.py'), str(short), str(long), '2',
            '--source-root', str(self.directory)], text=True)
        self.assertIn('unknown instructions 2', output)
        self.assertNotIn('native-entered roots', output)
        rejected = subprocess.run([sys.executable,
            str(ROOT / 'tools/perf_4k60/census_roots.py'), str(short), str(long), '0'],
            capture_output=True, text=True)
        self.assertEqual(rejected.returncode, 2)


if __name__ == '__main__':
    unittest.main()

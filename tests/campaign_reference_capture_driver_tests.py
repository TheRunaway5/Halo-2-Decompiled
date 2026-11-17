#!/usr/bin/env python3
"""Source-free parser rejection tests; these never qualify a live campaign."""
from pathlib import Path
import copy
import hashlib
import json
import os
import struct
import sys
import tempfile
from types import SimpleNamespace
import unittest
from unittest import mock

sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from run_campaign_skin_runtime import configuration_errors, construction_errors, log_errors, source_stage_errors, valid_state
from validate_path_tracing_campaign import (compare_traces, inspect_trace, motion_errors,
    POSE_FIELDS, POSE_DIAGNOSTIC_OWNER_LIMIT, POSE_DIAGNOSTIC_UNIT_LIMIT, selection_differences)
import validate_path_tracing_campaign as campaign_comparison


class CampaignCaptureDeclarations(unittest.TestCase):
    def setUp(self):
        self.args = SimpleNamespace(frames=60, interval=1 / 30, fraction=.5, auxiliary_seed=1,
                                    width=3840, height=2160, squad=None)
        bits = lambda value: struct.unpack('<I', struct.pack('<f', value))[0]
        self.record = dict(schema='H2_CAMPAIGN_SKIN_REFERENCE_V1', graphics_mode='Original', frames=60, neutral_input=True,
                           interval_bits=bits(1 / 30), render_fraction_bits=bits(.5), primary_seed=0x12345678,
                           auxiliary_seed=1, width=3840, height=2160, place_squad_fixture='',
                           natural_startup=True, scenario_start_requested=True)

    def test_exact_declaration(self):
        self.assertEqual(configuration_errors(self.record, self.args), [])

    def test_original_skin_proof_rejects_path_tracing(self):
        self.assertTrue(configuration_errors(dict(self.record, graphics_mode='Path Tracing'), self.args))

    def test_every_missing_field_rejected(self):
        for key in self.record:
            with self.subTest(key=key):
                copy = dict(self.record)
                del copy[key]
                self.assertIn(f'Capture configuration declaration differs: {key}', configuration_errors(copy, self.args))

    def test_every_forged_field_rejected(self):
        for key, value in self.record.items():
            with self.subTest(key=key):
                copy = dict(self.record)
                copy[key] = not value if type(value) is bool else value + 1 if type(value) is int else value + '-forged'
                self.assertIn(f'Capture configuration declaration differs: {key}', configuration_errors(copy, self.args))

    def test_integer_field_boolean_is_not_accepted(self):
        copy = dict(self.record, auxiliary_seed=True)
        self.assertTrue(configuration_errors(copy, self.args))

    def test_single_float32_bit_difference_rejected(self):
        for key in ('interval_bits', 'render_fraction_bits'):
            copy = dict(self.record)
            copy[key] ^= 1
            self.assertTrue(configuration_errors(copy, self.args))

    def test_fixture_never_becomes_natural_startup(self):
        self.args.squad = 'authored-squad'
        self.assertTrue(configuration_errors(self.record, self.args))
        copy = dict(self.record, place_squad_fixture=self.args.squad, natural_startup=False, scenario_start_requested=False)
        self.assertEqual(configuration_errors(copy, self.args), [])

    def test_loader_and_vulkan_diagnostics_are_hard_rejections(self):
        self.assertEqual(log_errors(b'normal output'), [])
        self.assertTrue(log_errors(b'cannot allocate memory in static TLS block\nprocess recovered and exit0'))
        self.assertTrue(log_errors(b'Vulkan validation: warning'))

    def test_original_startup_may_advance_primary_without_reseeding(self):
        actual = dict(game_options_seed=0x12345678, rng_words=[2715262136, 1])
        self.assertEqual(construction_errors([dict(state=actual)], self.args), [])
        actual['game_options_seed'] ^= 1
        self.assertTrue(construction_errors([dict(state=actual)], self.args))
        actual['game_options_seed'] ^= 1
        actual['rng_words'][1] ^= 1
        self.assertTrue(construction_errors([dict(state=actual)], self.args))

    def test_clock_tick_must_match_actual_clock_bytes(self):
        clock = bytearray(0x24)
        clock[8:12] = (12).to_bytes(4, 'little')
        state = dict(rng_words=[2715262136, 1], source_tick=12, clock_24_hex=clock.hex(), rng_address=100, clock_address=200)
        self.assertTrue(valid_state(state))
        state['source_tick'] = True
        self.assertFalse(valid_state(state))
        state['source_tick'] = 13
        self.assertFalse(valid_state(state))

    def test_construction_frame_stages_cannot_be_missing_or_reordered(self):
        self.args.frames = 1
        stages = [('before_scenario_factories', None), ('after_scenario_factories', None), ('before_frames', 0),
                  ('frame_begin', 0), ('simulation_end', 0), ('before_render', 0), ('after_render', 0)]
        events = [dict(event='source_state', stage=stage, **({} if frame is None else dict(frame=frame))) for stage, frame in stages]
        self.assertEqual(source_stage_errors(events, self.args), [])
        for index in range(len(events)):
            self.assertTrue(source_stage_errors(events[:index] + events[index + 1:], self.args))
        self.assertTrue(source_stage_errors(events + [dict(event='source_state', stage='unknown')], self.args))
        events[0], events[1] = events[1], events[0]
        self.assertTrue(source_stage_errors(events, self.args))


class CampaignGraphicsComparison(unittest.TestCase):
    def setUp(self):
        self.spec = dict(frames=2, width=640, height=480, interval=1/30, fraction=.5,
                         auxiliary_seed=1, mode='Original')
        bits = lambda value: struct.unpack('<I', struct.pack('<f', value))[0]
        self.events = [dict(event='configuration', schema='H2_CAMPAIGN_SKIN_REFERENCE_V1',
            graphics_mode='Original', frames=2, neutral_input=True, interval_bits=bits(1/30),
            render_fraction_bits=bits(.5), primary_seed=0x12345678, auxiliary_seed=1,
            width=640, height=480, place_squad_fixture='', natural_startup=True, scenario_start_requested=True)]

        def state(tick):
            clock = bytearray(0x24);clock[8:12] = tick.to_bytes(4, 'little')
            return dict(source_tick=tick, clock_24_hex=clock.hex(), rng_words=[123, 1],
                        rng_address=100, clock_address=200)

        for stage, frame in [('before_scenario_factories', None), ('after_scenario_factories', None), ('before_frames', 0)]:
            self.events.append(dict(event='source_state', stage=stage, frame=frame, state=state(0)))
        for frame in range(2):
            for stage in ('frame_begin', 'simulation_end', 'before_render', 'after_render'):
                self.events.append(dict(event='source_state', stage=stage, frame=frame, state=state(frame+1)))
            self.events.append(dict(event='main_view', frame=frame, tick=frame+1,
                eye_bits=[0, 0, 0], forward_bits=[bits(1), 0, 0], up_bits=[0, 0, bits(1)],
                projection_angle_bits=bits(1), viewport=[0, 0, 480, 640], main_commands_completed=True,
                state=state(frame+1)))
            node = [bits(1)] + [0]*12
            sampled = list(node);sampled[10] = bits(.5)
            self.events.append(dict(event='actor_pose', frame=frame, owner=0x12340001,
                definition=0x56780002, builder_rows=1, ordinary_main_units=[dict(lod=0, ordinal=1,
                    coverage=255, lighting_class=2, override_slot=255, shader_override=0xffffffff,
                    unit_words=[0x40000001, 0x56780002, 0, 0, 0x80001000, 0, 0x00ffffff, 0])],
                WORLD_node_bits=[node], sampled_node_bits=[sampled]))
            self.events.append(dict(event='actor_geometry', frame=frame, owner=0x12340001,
                vertices=3, complete_materialized_vertex_sha256='a'*64))
            self.events.append(dict(event='renderer', frame=frame, submitted=True, width=640, height=480))
        self.renumber()

    def renumber(self):
        for sequence, event in enumerate(self.events):
            event['sequence'] = sequence

    def test_exact_pair(self):
        original = inspect_trace(iter(self.events), self.spec)
        self.events[0]['graphics_mode'] = 'Path Tracing'
        traced = inspect_trace(iter(self.events), dict(self.spec, mode='PathTracing'))
        self.assertEqual(compare_traces(original, traced), [])

    def test_camera_one_bit_mismatch_rejected(self):
        original = inspect_trace(iter(self.events), self.spec)
        traced = copy.deepcopy(original)
        traced['cameras'][-1]['eye_bits'][1] ^= 1
        self.assertTrue(compare_traces(original, traced))

    def test_source_tick_mismatch_rejected(self):
        original = inspect_trace(iter(self.events), self.spec)
        traced = copy.deepcopy(original)
        traced['stages'][-1][2] += 1
        self.assertTrue(compare_traces(original, traced))

    def test_sampled_pose_one_bit_mismatch_rejected(self):
        original = inspect_trace(iter(self.events), self.spec)
        next(e for e in self.events if e['event'] == 'actor_pose')['sampled_node_bits'][0][10] ^= 1
        self.assertTrue(compare_traces(original, inspect_trace(iter(self.events), self.spec)))

    def test_materialized_geometry_mismatch_rejected(self):
        original = inspect_trace(iter(self.events), self.spec)
        next(e for e in self.events if e['event'] == 'actor_geometry')['complete_materialized_vertex_sha256'] = 'b'*64
        self.assertTrue(compare_traces(original, inspect_trace(iter(self.events), self.spec)))

    def test_pose_field_diagnostics_isolate_early_and_last_frame_changes(self):
        original = inspect_trace(iter(self.events), self.spec)
        for frame in (0, 1):
            for field in POSE_FIELDS:
                with self.subTest(frame=frame, field=field):
                    events = copy.deepcopy(self.events)
                    event = next(e for e in events if e['event'] == 'actor_pose' and e['frame'] == frame)
                    if field.endswith('_node_bits'):
                        event[field][0][10] ^= 1
                    elif field == 'ordinary_main_units':
                        event[field][0]['lod'] += 1
                    else:
                        event[field] += 1
                    changed = inspect_trace(iter(events), self.spec)
                    self.assertTrue(compare_traces(original, changed))
                    self.assertNotEqual(original['actors']['pose_sha256'], changed['actors']['pose_sha256'])
                    self.assertEqual(original['actors']['geometry_sha256'], changed['actors']['geometry_sha256'])
                    differs = lambda a, b: [name for name in POSE_FIELDS if a[name] != b[name]]
                    self.assertEqual(differs(original['actors']['pose_field_sha256'],
                                             changed['actors']['pose_field_sha256']), [field])
                    old_last, new_last = original['actors']['last_actors'][0], changed['actors']['last_actors'][0]
                    self.assertEqual(differs(old_last['pose_field_sha256'], new_last['pose_field_sha256']),
                                     [field] if frame == 1 else [])
                    if frame == 1 and field in ('definition', 'builder_rows', 'ordinary_main_units'):
                        self.assertEqual(new_last['native_selection'][field], event[field])

    def test_native_selection_diagnostics_are_bounded_without_weakening_hashes(self):
        pose = copy.deepcopy(next(e for e in self.events if e['event'] == 'actor_pose' and e['frame'] == 1))
        geometry = copy.deepcopy(next(e for e in self.events if e['event'] == 'actor_geometry' and e['frame'] == 1))
        pose['ordinary_main_units'] = [dict(pose['ordinary_main_units'][0], lod=i)
                                      for i in range(POSE_DIAGNOSTIC_UNIT_LIMIT+1)]
        self.events = [e for e in self.events if not (e.get('frame') == 1 and e['event'] in ('actor_pose', 'actor_geometry'))]
        at = next(i for i,e in enumerate(self.events) if e['event'] == 'renderer' and e['frame'] == 1)
        records = []
        for index in range(POSE_DIAGNOSTIC_OWNER_LIMIT+1):
            p, g = copy.deepcopy(pose), copy.deepcopy(geometry)
            p['owner'] += index;g['owner'] += index
            records.extend((p, g))
        self.events[at:at] = records
        self.renumber()
        original = inspect_trace(iter(self.events), self.spec)
        last = original['actors']['last_actors']
        self.assertEqual(sum('native_selection' in actor for actor in last), POSE_DIAGNOSTIC_OWNER_LIMIT)
        for actor in last[:POSE_DIAGNOSTIC_OWNER_LIMIT]:
            selection = actor['native_selection']
            self.assertEqual(len(selection['ordinary_main_units']), POSE_DIAGNOSTIC_UNIT_LIMIT)
            self.assertEqual(selection['ordinary_main_unit_count'], POSE_DIAGNOSTIC_UNIT_LIMIT+1)
            self.assertTrue(selection['ordinary_main_units_truncated'])
            self.assertNotIn('WORLD_node_bits', actor)
            self.assertNotIn('sampled_node_bits', actor)
        records[-2]['ordinary_main_units'][-1]['lod'] += 1
        self.assertTrue(compare_traces(original, inspect_trace(iter(self.events), self.spec)))

    def test_native_context_addresses_are_diagnostic_while_full_owner_is_compared(self):
        original = inspect_trace(iter(self.events), self.spec)
        for event in self.events:
            if event['event'] == 'actor_pose':
                event['ordinary_main_units'][0]['unit_words'][4] += 0x300
        changed = inspect_trace(iter(self.events), self.spec)
        self.assertEqual(compare_traces(original, changed), [])
        self.assertEqual(original['actors']['semantic_pose_sha256'], changed['actors']['semantic_pose_sha256'])
        self.assertNotEqual(original['actors']['pose_sha256'], changed['actors']['pose_sha256'])
        self.assertNotEqual(original['actors']['last_actors'][0]['native_selection'],
                            changed['actors']['last_actors'][0]['native_selection'])
        for event in self.events:
            if event['event'] in ('actor_pose', 'actor_geometry') and event['frame'] == 0:
                event['owner'] ^= 0x10000
        self.assertTrue(compare_traces(original, inspect_trace(iter(self.events), self.spec)))

    def test_every_non_address_native_word_and_selection_field_remains_strict(self):
        original = inspect_trace(iter(self.events), self.spec)
        fields = ('ordinal', 'lod', 'coverage', 'lighting_class', 'override_slot', 'shader_override')
        for frame in (0, 1):
            for field in (*fields, 0, 1, 2, 3, 5, 6, 7):
                with self.subTest(frame=frame, field=field):
                    events = copy.deepcopy(self.events)
                    event = next(e for e in events if e['event'] == 'actor_pose' and e['frame'] == frame)
                    unit = event['ordinary_main_units'][0]
                    if isinstance(field, int):
                        unit['unit_words'][field] ^= 1
                    else:
                        unit[field] ^= 1
                    changed = inspect_trace(iter(events), self.spec)
                    self.assertTrue(compare_traces(original, changed))
                    expected = f'unit_words[{field}]' if isinstance(field, int) else field
                    self.assertEqual([key for key in original['actors']['unit_field_sha256']
                                      if original['actors']['unit_field_sha256'][key] != changed['actors']['unit_field_sha256'][key]],
                                     [expected])

    def test_native_context_requires_complete_words_and_valid_address(self):
        for malformed in ([], [0]*7, [0]*9, [0]*8,
                          [0, 0, 0, 0, 0xffffffff, 0, 0, 0],
                          [0, 0, 0, 0, 0x80001000, 0, -1, 0]):
            with self.subTest(words=malformed):
                events = copy.deepcopy(self.events)
                next(e for e in events if e['event'] == 'actor_pose')['ordinary_main_units'][0]['unit_words'] = malformed
                with self.assertRaises(ValueError):
                    inspect_trace(iter(events), self.spec)

    def test_early_and_last_geometry_changes_remain_strict(self):
        original = inspect_trace(iter(self.events), self.spec)
        for frame in (0, 1):
            events = copy.deepcopy(self.events)
            next(e for e in events if e['event'] == 'actor_geometry' and e['frame'] == frame)[
                'complete_materialized_vertex_sha256'] = 'b'*64
            self.assertTrue(compare_traces(original, inspect_trace(iter(events), self.spec)))

    def test_raw_only_pose_receipts_cannot_pass_semantic_comparison(self):
        original = inspect_trace(iter(self.events), self.spec)
        for target in ('aggregate', 'last', 'contract'):
            with self.subTest(target=target):
                changed = copy.deepcopy(original)
                if target == 'aggregate':
                    del changed['actors']['semantic_pose_sha256']
                elif target == 'last':
                    del changed['actors']['last_actors'][0]['semantic_pose_sha256']
                else:
                    changed['actors']['pose_comparison'] = 'raw'
                self.assertTrue(compare_traces(original, changed))

    def test_selection_stream_finds_early_fields_without_retaining_nodes(self):
        with tempfile.TemporaryDirectory(dir=os.environ.get('HALO2_TEST_SCRATCH')) as directory:
            paths = [Path(directory)/name for name in ('original.jsonl', 'traced.jsonl')]
            for index, path in enumerate(paths):
                events = copy.deepcopy(self.events)
                if index:
                    next(e for e in events if e['event'] == 'actor_pose')['ordinary_main_units'][0]['unit_words'][2] ^= 1
                    for event in events:
                        if event['event'] == 'actor_pose':
                            event['ordinary_main_units'][0]['unit_words'][4] += 0x300
                with path.open('w') as output:
                    inspect_trace(iter(events), self.spec, lambda row: output.write(json.dumps(row)+'\n'))
            result = selection_differences(*paths)
            self.assertEqual(result['differing_records'], 1)
            first = result['first_differences'][0]
            self.assertEqual(first['Original']['frame'], 0)
            self.assertEqual(first['differing_fields'], [dict(unit=0, field='unit_words[2]', Original=0, PathTracing=1)])
            self.assertNotIn('WORLD_node_bits', paths[0].read_text())
            # Repeated failures beyond the report cap still contribute to the
            # exact count, while raw rows stay within the explicit bound.
            for path in paths:
                first_line = path.read_text().splitlines()[0]
                path.write_text((first_line+'\n')*(POSE_DIAGNOSTIC_OWNER_LIMIT+3))
            result = selection_differences(*paths)
            self.assertEqual(result['differing_records'], POSE_DIAGNOSTIC_OWNER_LIMIT+3)
            self.assertEqual(result['retained_records'], POSE_DIAGNOSTIC_OWNER_LIMIT)

    def test_coordinator_retains_failed_pairs_and_rejects_midrun_helper_change(self):
        from PIL import Image
        for mismatch, helper_changed in ((False, False), (True, False), (False, True), (True, True)):
            with self.subTest(mismatch=mismatch, helper_changed=helper_changed), \
                    tempfile.TemporaryDirectory(dir=os.environ.get('HALO2_TEST_SCRATCH')) as directory:
                root = Path(directory)
                scratch = root/'owned';scratch.mkdir()
                runtime = root/'halo2_native';runtime.write_bytes(b'controlled runtime')
                source_map = root/'00a_introduction.map';source_map.write_bytes(b'controlled map')
                (root/'gameplay-runtime-selection.json').write_text('{}')
                output, retained = root/'report.json', root/'references'
                def worker(command, **kwargs):
                    spec = json.loads(Path(command[command.index('--capture-worker')+1]).read_text())
                    result = Path(command[command.index('--result-directory')+1])
                    events = copy.deepcopy(self.events)
                    events[0]['graphics_mode'] = 'Path Tracing' if spec['mode'] == 'PathTracing' else 'Original'
                    if mismatch and spec['mode'] == 'PathTracing':
                        next(e for e in events if e['event'] == 'actor_pose')['ordinary_main_units'][0]['unit_words'][2] ^= 1
                    with (result/'native-selections.jsonl').open('w') as selections:
                        compact = inspect_trace(iter(events), spec, lambda row: selections.write(json.dumps(row)+'\n'))
                    (result/'trace.json').write_text(json.dumps(compact))
                    (result/'result.json').write_text(json.dumps(dict(passed=True, scratch=str(result/'removed-owner'))))
                    Image.new('RGB', (640, 480), (60, 70, 80)).save(result/'frame.png')
                    return SimpleNamespace(returncode=0)
                real_sha = campaign_comparison.sha
                helper_reads = 0
                def observed_sha(path):
                    nonlocal helper_reads
                    if Path(path) == Path(campaign_comparison.__file__):
                        helper_reads += 1
                        if helper_changed and helper_reads > 1:
                            return '0'*64
                    return real_sha(path)
                argv = ['validate_path_tracing_campaign.py', '--runtime', str(runtime), '--map', str(source_map),
                        '--frames', '2', '--report-output', str(output), '--retain-captures', str(retained)]
                with mock.patch.object(campaign_comparison, 'ROOT', root), \
                        mock.patch.dict(os.environ, {'HALO2_TEST_SCRATCH':str(scratch)}), \
                        mock.patch.object(sys, 'argv', argv), \
                        mock.patch.object(campaign_comparison.subprocess, 'run', side_effect=worker), \
                        mock.patch.object(campaign_comparison.subprocess, 'check_output', return_value='ext2/ext3\n'), \
                        mock.patch.object(campaign_comparison, 'sha', side_effect=observed_sha):
                    code = campaign_comparison.main()
                report = json.loads(output.read_text())
                self.assertEqual(code, int(mismatch or helper_changed))
                self.assertEqual(report['passed'], not mismatch and not helper_changed)
                shot = report['shots'][0]
                self.assertEqual(shot['comparison']['comparison_passed'], not mismatch)
                target = Path(shot['comparison']['retained_capture']['path'])
                self.assertTrue(target.exists())
                with Image.open(target) as image:
                    self.assertEqual(image.size, (1280, 522 if mismatch else 504))
                if mismatch:
                    self.assertFalse(shot['passed'])
                    self.assertEqual(shot['native_selection_differences']['first_differences'][0]['differing_fields'][0]['field'],
                                     'unit_words[2]')
                if helper_changed:
                    self.assertIn('Capture helper changed during the comparison', report['error'])

    def test_missing_actor_geometry_rejected(self):
        self.events = [e for e in self.events if e['event'] != 'actor_geometry']
        self.renumber()
        with self.assertRaises(ValueError):
            inspect_trace(iter(self.events), self.spec)

    def test_interpolation_is_observed(self):
        actors = inspect_trace(iter(self.events), self.spec)['actors']
        self.assertEqual(actors['pose_count'], 2)
        self.assertEqual(actors['interpolated_pose_count'], 2)
        self.assertEqual(actors['nonempty_geometry_count'], 2)
        self.assertEqual(actors['interpolated_geometry_count'], 2)
        self.assertEqual(actors['materialized_vertices'], 6)
        self.assertEqual(actors['last_frame'], 1)

    def test_empty_selected_geometry_is_not_motion_evidence(self):
        for event in self.events:
            if event['event'] == 'actor_geometry':
                event['vertices'] = 0
                event['complete_materialized_vertex_sha256'] = hashlib.sha256(b'').hexdigest()
        actors = inspect_trace(iter(self.events), self.spec)['actors']
        self.assertEqual(actors['interpolated_pose_count'], 2)
        self.assertEqual(actors['nonempty_geometry_count'], 0)
        self.assertEqual(actors['interpolated_geometry_count'], 0)
        self.assertEqual(actors['materialized_vertices'], 0)

    def test_empty_geometry_hash_must_match_empty_bank(self):
        next(e for e in self.events if e['event'] == 'actor_geometry')['vertices'] = 0
        with self.assertRaises(ValueError):
            inspect_trace(iter(self.events), self.spec)

    def test_motion_acceptance_requires_final_visible_interpolation(self):
        trace = inspect_trace(iter(self.events), self.spec)
        self.assertEqual(motion_errors(trace, 1), [])
        self.assertTrue(motion_errors(trace, 2))
        trace['actors']['last_actors'][0]['interpolated'] = False
        self.assertTrue(motion_errors(trace, 1))
        trace['actors']['last_actors'][0].update(interpolated=True, vertices=0)
        self.assertTrue(motion_errors(trace, 1))

    def test_cinematic_scene_models_are_motion_evidence(self):
        for event in self.events:
            if event['event'] in ('actor_pose', 'actor_geometry'):
                event['event'] = event['event'].replace('actor_', 'scene_')
        trace = inspect_trace(iter(self.events), self.spec)
        self.assertEqual(trace['actors']['scene_object_pose_count'], 2)
        self.assertEqual(trace['actors']['actor_pose_count'], 0)
        self.assertEqual(trace['actors']['interpolated_geometry_count'], 2)
        self.assertEqual(motion_errors(trace, 1), [])

    def test_pose_and_geometry_owner_kinds_must_match(self):
        next(e for e in self.events if e['event'] == 'actor_pose')['event'] = 'scene_pose'
        with self.assertRaises(ValueError):
            inspect_trace(iter(self.events), self.spec)

    def test_duplicate_full_owner_receipts_rejected(self):
        duplicate = [copy.deepcopy(e) for e in self.events if e['event'] in ('actor_pose', 'actor_geometry') and e['frame'] == 0]
        index = next(i for i,e in enumerate(self.events) if e['event'] == 'renderer')
        self.events[index:index] = duplicate
        self.renumber()
        with self.assertRaises(ValueError):
            inspect_trace(iter(self.events), self.spec)

    def test_missing_renderer_or_camera_rejected(self):
        for kind in ('renderer', 'main_view'):
            with self.subTest(kind=kind):
                events = [e for e in self.events if e['event'] != kind]
                for sequence, event in enumerate(events):
                    event['sequence'] = sequence
                with self.assertRaises(ValueError):
                    inspect_trace(iter(events), self.spec)
                self.renumber()

    def test_forged_clock_bytes_rejected(self):
        self.events[1]['state']['source_tick'] += 1
        with self.assertRaises(ValueError):
            inspect_trace(iter(self.events), self.spec)

    def test_reordered_sequence_rejected(self):
        self.events[1]['sequence'] = 0
        with self.assertRaises(ValueError):
            inspect_trace(iter(self.events), self.spec)

    def test_incomplete_source_commands_rejected(self):
        next(e for e in self.events if e['event'] == 'main_view')['main_commands_completed'] = False
        with self.assertRaises(ValueError):
            inspect_trace(iter(self.events), self.spec)

    def test_wrong_mode_label_rejected(self):
        with self.assertRaises(ValueError):
            inspect_trace(iter(self.events), dict(self.spec, mode='PathTracing'))


if __name__ == '__main__':
    unittest.main()

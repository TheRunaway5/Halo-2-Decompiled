#!/usr/bin/env python3
"""Source-free parser rejection tests; these never qualify a live campaign."""
from pathlib import Path
import struct
import sys
from types import SimpleNamespace
import unittest

sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from run_campaign_skin_runtime import configuration_errors, construction_errors, log_errors, source_stage_errors, valid_state


class CampaignCaptureDeclarations(unittest.TestCase):
    def setUp(self):
        self.args = SimpleNamespace(frames=60, interval=1 / 30, fraction=.5, auxiliary_seed=1,
                                    width=3840, height=2160, squad=None)
        bits = lambda value: struct.unpack('<I', struct.pack('<f', value))[0]
        self.record = dict(schema='H2_CAMPAIGN_SKIN_REFERENCE_V1', frames=60, neutral_input=True,
                           interval_bits=bits(1 / 30), render_fraction_bits=bits(.5), primary_seed=0x12345678,
                           auxiliary_seed=1, width=3840, height=2160, place_squad_fixture='',
                           natural_startup=True, scenario_start_requested=True)

    def test_exact_declaration(self):
        self.assertEqual(configuration_errors(self.record, self.args), [])

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


if __name__ == '__main__':
    unittest.main()

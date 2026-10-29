import ctypes
import json
from pathlib import Path
import unittest

from multiplayer.master_server.search_lifecycle import (
    GatheringClock, GatheringConfiguration, I32_MAX, JitterStream, PartyState, SearchProgress, SeenSessions,
    elapsed, jitter, native, native_birth_id_sessions, role, retail_configuration)


class OriginalLifecycleTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.fixture = json.loads((Path(__file__).parent/'fixtures/retail-search-lifecycle.json').read_text())
        if cls.fixture['original_runner'] != 'ad94f29e7afe6ea598c81f88fa3dabb35c0f4bbed7c92a94deed93b8985ed90b':
            raise AssertionError('Unpinned original-byte lifecycle controls')

    def test_original_signed_jitter_and_role_entry_controls(self):
        for case in self.fixture['jitter']:
            self.assertEqual(list(jitter(case['inputs']['seed'])[1:]),case['expected'],case['name'])
        for case in self.fixture['roles']:
            values = case['inputs']
            result = role(10000,values['search_start'],values['gather_start'],values['capacity'])
            self.assertEqual(result,'search' if case['expected']=='0x70c00' else 'gather',case['name'])

    def test_original_result_driven_radius_and_exhaustion_controls(self):
        for case in self.fixture['queries']:
            progress = SearchProgress(case['maximum'],case['ranked'],case['radius'],case['total'])
            again = progress.complete(case['count'])
            self.assertEqual([progress.radius,progress.total,int(again)],case['expected'],case['name'])
            if not again:
                self.assertFalse(progress.active)

    def test_each_live_handle_and_selection_blocks_query_completion(self):
        for gate in ('query_pending','full_pending','brief_pending','selected'):
            progress = SearchProgress(4,True)
            self.assertFalse(progress.complete(50,**{gate:True}))
            self.assertEqual((progress.radius,progress.total,progress.active),(0,0,True))

    def test_original_growth_clock_controls_and_rejoin_below_old_peak(self):
        for case in self.fixture['growth']:
            clock = GatheringClock(1000,5000,case['peak'])
            waited = clock.observe(10000,case['players'])
            self.assertEqual([clock.peak,clock.growth,waited],case['expected'],case['name'])
        clock = GatheringClock(1000,5000,3)
        clock.observe(10000,2)
        self.assertEqual(clock.observe(11000,3),6000)
        self.assertEqual(clock.growth,5000)

    def test_getter_maximum_override_invalid_count_and_exact_timeout_boundary(self):
        config = GatheringConfiguration(tuple(range(16)),120000,'Native boundary fixture')
        self.assertEqual(config.delay(0,2,8),I32_MAX)
        self.assertEqual(config.delay(9,2,8),I32_MAX)
        self.assertEqual(config.delay(8,2,8),0)
        self.assertEqual(config.delay(6,2,8),5)
        clock = GatheringClock(1000,1000,1)
        self.assertFalse(clock.expired(119456,config,-1543))
        self.assertTrue(clock.expired(119457,config,-1543))
        self.assertEqual(elapsed(4,0xfffffff0),20)

    def test_all_original_hopper_getters_and_signed_tick_wrap_controls(self):
        fixture = json.loads((Path(__file__).parent/'fixtures/retail-lifecycle-followup.json').read_text())
        self.assertEqual(fixture['executable_sha256'],self.fixture['original_xbe'])
        for row in fixture['hoppers']:
            minimum,maximum,config = retail_configuration(row['index'])
            self.assertEqual((minimum,maximum,config.timeout_ms),(row['minimum'],row['maximum'],row['timeout_ms']))
            for players,expected in enumerate(row['countdown_ms_by_players_0_through_17']):
                self.assertEqual(config.delay(players,minimum,maximum),expected,(row['name'],players))
        for row in fixture['tick_subtraction']:
            self.assertEqual(elapsed(row['now_u32'],row['start_u32']),row['elapsed_i32'])

    def test_original_seen_capacity_saturates_without_evicting_and_new_search_clears(self):
        fixture = json.loads((Path(__file__).parent/'fixtures/retail-lifecycle-followup.json').read_text())['seen_bank']
        seen = SeenSessions()
        for row in fixture['append_controls']:
            seen.reject(str(row['id']))
            self.assertEqual(len(seen),row['count'])
        self.assertIn('1',seen)
        self.assertNotIn('151',seen)
        seen.clear()
        self.assertEqual(len(seen),0)

    def test_rust_party_generation_owns_radius_seen_reset_and_stale_callback_rejection(self):
        state=PartyState(search_offset=-1307,gather_offset=-1543)
        state.start_search(10000,2,False)
        old_progress,old_seen=state.progress,state.seen
        old_seen.reject('session-a')
        self.assertTrue(state.complete_query(0))
        self.assertEqual((state.progress.radius,len(state.seen)),(1,1))
        state.start_gather(11000)
        self.assertEqual(state.gather_start,11000)
        state.start_search(12000,0,True)
        before=state.snapshot()
        self.assertEqual((state.progress.radius,len(state.seen)),(0,0))
        with self.assertRaises(ValueError):old_progress.complete(50)
        with self.assertRaises(ValueError):old_seen.reject('stale')
        with self.assertRaises(ValueError):state.progress.complete(51)
        self.assertEqual(state.snapshot(),before)
        self.assertFalse(state.complete_query(0))
        self.assertEqual(state.mode,'choose')

    def test_rust_clock_remaining_is_read_only_and_rejoin_preserves_old_peak(self):
        config=GatheringConfiguration((1000,)*16,120000,'Native clock projection fixture')
        clock=GatheringClock(100,100,1)
        before=clock.snapshot()
        self.assertEqual(clock.remaining(200,config,2,2,4),900)
        self.assertEqual(clock.snapshot(),before)
        self.assertFalse(clock.ready(200,config,2,2,4))
        self.assertEqual((clock.peak,clock.growth),(2,200))
        self.assertEqual(clock.observe(300,1),100)
        self.assertEqual(clock.observe(400,2),200)
        self.assertEqual(clock.growth,200)

    def test_failed_ffi_response_capacity_does_not_advance_rng_or_touch_output(self):
        stream=JitterStream(0)
        owner=stream._owner.handle
        source=ctypes.create_string_buffer(b'{"op":"jitter_next"}')
        output=(ctypes.c_uint8*64)(*([0xa5]*64))
        self.assertEqual(native().h2_lifecycle_command(owner,source,20,output,1),0)
        self.assertEqual(stream.seed,0)
        self.assertEqual(bytes(output),bytes([0xa5])*64)
        self.assertEqual(stream.next(),(-1307,-1543))
        stream.close()
        self.assertEqual(native().h2_lifecycle_command(owner,source,20,output,64),0)
        self.assertEqual(bytes(output),bytes([0xa5])*64)
        self.assertEqual(native().h2_lifecycle_new(source,262145),0)

    def test_native_provider_contract_is_explicit_bounded_and_rejects_duplicate_identities(self):
        rows=[(100+n%3,f'{n:03}')for n in range(60,0,-1)]
        expected=tuple(f'{n:03}'for n in (*range(3,61,3),*range(1,61,3),*range(2,31,3)))
        self.assertEqual(native_birth_id_sessions(rows),expected)
        with self.assertRaises(ValueError):native_birth_id_sessions([(1,'same'),(2,'same')])
        with self.assertRaises(ValueError):native_birth_id_sessions([(True,'a')])
        with self.assertRaises(ValueError):native_birth_id_sessions([(n,str(n))for n in range(1001)])

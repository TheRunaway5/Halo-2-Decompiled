"""Private Rust registry/IO lifecycle tests using declared transport facts."""
from dataclasses import FrozenInstanceError, replace
import os
import random
from types import SimpleNamespace
import unittest

from multiplayer.master_server.gathering_sessions import GatheringSessions, Gathering
from multiplayer.master_server.matchmaking import Coordinator, Player
from multiplayer.master_server.native_matchmaking import native
from multiplayer.master_server.protocol import Problem
from multiplayer.master_server.search_lifecycle import GatheringConfiguration
from multiplayer.master_server.search_qos import Measurement
from multiplayer.master_server.tests.test_matchmaking import policy, MAPS


class RustGatheringOwnerTests(unittest.TestCase):
    def setUp(self):
        self.now = 100.
        self.policy = replace(policy('native-gather', team_size=0, radius=6), maximum_players=2,
            candidate_selection='retail-candidates-native-relay-v1',
            gathering=GatheringConfiguration((0,)*16,150000,'Declared native registry timing'))
        self.core = Coordinator((self.policy,), clock=lambda:self.now, rng=random.Random(2))
        self.addCleanup(self.core.close)
        self.measured, self.requests = {}, []
        self.qos = SimpleNamespace(ready=lambda _:True,
            measurement=lambda a,b:self.measured.get(frozenset((a,b))),
            request=self.request, pending=lambda a,b:True)
        self.owner = GatheringSessions(SimpleNamespace(core=self.core,search_qos=self.qos), wall_clock=lambda:1000., seed=1)
        self.a, self.b = self.queue('a'), self.queue('b')

    def queue(self, name):
        self.now += .001
        return self.core.enqueue(name,self.policy.id,'declared-v8',(Player(name,'Pilot',2,MAPS,1),))

    def request(self, a, b):
        self.requests.append((a,b))
        return True

    def test_real_owner_retains_query_generation_and_consumes_only_admitted_snapshots(self):
        self.assertIsNone(self.owner.advance(self.policy.id))
        gathering = next(iter(self.owner.sessions.values()))
        query = next(iter(self.owner.queries.values()))
        with self.assertRaises(TypeError):
            self.owner.sessions[gathering.id] = gathering
        with self.assertRaises(TypeError):
            self.owner.queries[self.b.id] = query
        with self.assertRaises(FrozenInstanceError):
            gathering.tickets = (self.a.id,self.b.id)
        object.__setattr__(gathering,'host','outsider')
        self.assertEqual(next(iter(self.owner.sessions.values())).host,'a')
        self.assertIsNone(self.owner.advance(self.policy.id))
        current = next(iter(self.owner.queries.values()))
        self.assertEqual((current.generation,current.started),(query.generation,query.started))
        self.measured[frozenset(('a','b'))] = Measurement(10,9999.)
        result = self.owner.advance(self.policy.id)
        self.assertEqual(set(result.tickets),{self.a.id,self.b.id})
        self.assertEqual(result.gathering_host,'a')
        with self.assertRaises(Problem):
            _ = query.selector.bank

    def test_cancel_retires_saved_party_seen_and_progress_views_without_touching_requeue(self):
        state = self.owner.state(self.b)
        self.owner.start_search(self.policy,self.b,state,self.owner.tick(self.now))
        progress, seen = state.progress, state.seen
        old = self.b
        self.core.cancel(old.id)
        self.b = self.queue('b')
        current = self.owner.state(self.b)
        self.owner.start_search(self.policy,self.b,current,self.owner.tick(self.now))
        for action in (state.snapshot, lambda:progress.complete(50), lambda:seen.reject('old-session')):
            with self.assertRaises(Problem) as caught:
                action()
            self.assertEqual(caught.exception.status,404)
        self.assertEqual((current.progress.radius,current.progress.total,len(current.seen)),(0,0,0))
        self.assertNotEqual(old.id,self.b.id)

    def test_malformed_replayed_or_aborted_transport_results_do_not_advance_current_frame(self):
        event = self.core._call('gather_advance',playlist=self.policy.id,wall=1000.)
        self.assertEqual(event['action'],'facts')
        before = self.owner.seed
        for frame,result,status in ((event['frame']+1,dict(ready=[True],pairs=[]),409),
                                     (event['frame'],dict(ready=[],pairs=[]),400)):
            with self.assertRaises(Problem) as caught:
                self.core._call('gather_resume',frame=frame,result=result)
            self.assertEqual(caught.exception.status,status)
        self.assertEqual(self.owner.seed,before)
        next_event = self.core._call('gather_resume',frame=event['frame'],result=dict(ready=[True],pairs=[]))
        self.assertNotEqual(next_event['frame'],event['frame'])
        with self.assertRaises(Problem) as caught:
            self.core._call('gather_abort',frame=event['frame'])
        self.assertEqual(caught.exception.status,409)
        self.core._call('gather_abort',frame=next_event['frame'])
        self.assertIsNone(self.owner.advance(self.policy.id))

    def test_same_revision_profile_change_retires_gathering_query_and_saved_views(self):
        self.owner.advance(self.policy.id)
        saved = self.owner.states[self.b.id].progress
        changed = replace(self.policy,games=tuple(replace(g,weight=g.weight+1) for g in self.policy.games))
        self.core.playlists[changed.id] = changed
        self.assertFalse(self.owner.sessions)
        self.assertFalse(self.owner.queries)
        self.assertFalse(self.owner.states)
        with self.assertRaises(Problem):
            saved.complete(1)

    def test_transport_exception_aborts_only_the_pending_continuation(self):
        def failed(a,b): raise RuntimeError('Declared IO failure')
        self.qos.measurement = failed
        with self.assertRaisesRegex(RuntimeError,'Declared IO failure'):
            self.owner.advance(self.policy.id)
        self.qos.measurement = lambda a,b:None
        self.assertIsNone(self.owner.advance(self.policy.id))
        self.assertEqual(len(self.owner.sessions),1)
        self.assertEqual(len(self.owner.queries),1)

    def test_build_capability_and_default_abi_reject_diagnostic_restoration(self):
        enabled = os.environ.get('HALO2_MATCHMAKING_TEST_INPUTS') == '1'
        self.assertEqual(native().h2_matchmaking_test_inputs_enabled(),enabled)
        if not enabled:
            with self.assertRaises(Problem) as caught:
                self.owner._restore_fixture(Gathering('fixture',(self.a.id,),1000,'a',0))
            self.assertEqual(caught.exception.status,503)
            with self.assertRaises(Problem) as caught:
                self.core._call('gather_fixture',action='insert',value={})
            self.assertEqual(caught.exception.status,400)

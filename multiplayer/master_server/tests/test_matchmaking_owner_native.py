"""Actual service ABI authority/ownership tests; no original Live claim."""
import ctypes
from dataclasses import FrozenInstanceError, replace
import json
import random
import unittest

from multiplayer.master_server.matchmaking import Coordinator, Player
from multiplayer.master_server.native_matchmaking import native, value, _response
from multiplayer.master_server.protocol import Problem
from multiplayer.master_server.tests.test_matchmaking import policy, MAPS


class RustMatchmakingOwnerTests(unittest.TestCase):
    def setUp(self):
        self.policy = policy('native-owner-tests', team_size=0, radius=6)
        self.core = Coordinator((self.policy,), clock=lambda: 100., rng=random.Random(2))
        self.addCleanup(self.core.close)
        self.a, self.b = (self.core.enqueue('party-'+str(i), self.policy.id, 'same-build',
            (Player('player-'+str(i), 'Pilot', 10, MAPS),)) for i in range(2))
        self.ids = (self.a.id, self.b.id)

    def proposal(self):
        return self.core.form_proposal(self.policy.id, self.ids, self.policy.games[0], 'player-0')

    def test_snapshots_cannot_replace_admissions_or_ready_state(self):
        with self.assertRaises(TypeError):
            self.core.tickets[self.a.id] = self.a
        with self.assertRaises(FrozenInstanceError):
            self.a.compatibility = 'changed'
        # Even a trusted Python extension forcibly editing its local snapshot
        # cannot replace the admission held behind the private Rust handle.
        object.__setattr__(self.a, 'players', (Player('outsider', 'Other', 50, MAPS),))
        object.__setattr__(self.a, 'compatibility', 'changed')
        self.assertEqual(self.core.tickets[self.a.id].players[0].id, 'player-0')
        self.assertTrue(self.core._compatible(self.policy, self.a, self.b))
        proposal = self.proposal()
        self.assertEqual({p for team in proposal.teams for p in team}, {'player-0', 'player-1'})
        with self.assertRaises(TypeError):
            self.core.proposals[proposal.id] = proposal
        with self.assertRaises(AttributeError):
            self.core.ready[proposal.id].add('outsider')
        object.__setattr__(proposal, 'host', 'outsider')
        self.assertIsNone(self.core.proposals[proposal.id].host)
        with self.assertRaises(Problem) as caught:
            self.core._teams(self.policy, (replace(self.a, id='forged-ticket'), self.b))
        self.assertEqual(caught.exception.status, 404)

    def test_catalog_row_binding_reservation_and_duplicate_acknowledgment(self):
        before = self.core.hoppers.seed
        with self.assertRaises(Problem) as caught:
            self.core.form_proposal(self.policy.id, self.ids, replace(self.policy.games[0], variant='Forged settings'), 'player-0')
        self.assertEqual(caught.exception.status, 409)
        self.assertEqual(self.core.hoppers.seed, before)
        proposal = self.proposal()
        self.assertEqual(self.core.admission(proposal.id)['bound']['game']['id'], proposal.game.id)
        with self.assertRaises(Problem) as caught:
            self.proposal()
        self.assertEqual(caught.exception.status, 409)
        self.core.connection('player-0', 'player-1', 90)
        with self.assertRaises(Problem) as caught:
            self.core.acknowledge(proposal.id, 'outsider')
        self.assertEqual(caught.exception.status, 403)
        self.assertIsNone(self.core.proposals[proposal.id].host)
        self.assertEqual(self.core.ready[proposal.id], frozenset())
        self.assertIsNone(self.core.acknowledge(proposal.id, 'player-0'))
        self.assertIsNone(self.core.acknowledge(proposal.id, 'player-0'))
        self.assertEqual(self.core.ready[proposal.id], frozenset(('player-0',)))
        self.assertEqual(self.core.acknowledge(proposal.id, 'player-1').id, proposal.id)
        self.assertFalse(self.core.tickets)
        self.assertFalse(self.core.proposals)
        with self.assertRaises(Problem) as caught:
            self.core.acknowledge(proposal.id, 'player-1')
        self.assertEqual(caught.exception.status, 404)

    def test_same_revision_catalog_changes_require_new_admission(self):
        proposal = self.proposal()
        changed = replace(self.policy, games=(replace(self.policy.games[0], weight=4), *self.policy.games[1:]))
        self.core.playlists[changed.id] = changed
        self.assertEqual(self.core.playlists[changed.id], changed)
        for action in (lambda: self.core.heartbeat(self.a.id),
                       lambda: self.core.refresh_connections(proposal.id),
                       lambda: self.core.enqueue(self.a.party_id, self.a.playlist, self.a.compatibility, self.a.players)):
            with self.assertRaises(Problem) as caught:
                action()
            self.assertEqual(caught.exception.status, 409)
        self.core.cancel(self.a.id)
        self.assertFalse(self.core.proposals)
        self.assertFalse(self.core.ready)
        ticket = self.core.enqueue(self.a.party_id, self.a.playlist, self.a.compatibility, self.a.players)
        self.assertNotEqual(ticket.id, self.a.id)

    def test_success_error_malformed_and_retired_responses_release_buffers(self):
        lib = native()
        baseline = lib.h2_matchmaking_buffers_live()
        for _ in range(64):
            self.assertEqual(value('rank_radius', rank=10, high=None), 6)
            with self.assertRaises(Problem):
                value('rank_radius', rank=True, high=None)
            with self.assertRaises(Problem):
                self.core.status('missing-ticket')
        source = ctypes.create_string_buffer(b'{')
        length = ctypes.c_size_t()
        pointer = lib.h2_matchmaking_value(source, 1, ctypes.byref(length))
        self.assertTrue(pointer)
        try:
            response = json.loads(ctypes.string_at(pointer, length.value))
            self.assertFalse(response['ok'])
            self.assertEqual(response['status'], 400)
        finally:
            lib.h2_matchmaking_buffer_free(pointer, length.value)
        self.assertFalse(lib.h2_matchmaking_value(source, 8*1024*1024+1, ctypes.byref(length)))
        handle = self.core._handle
        self.core.close()
        with self.assertRaises(Problem) as caught:
            _response(lib.h2_matchmaking_owner_request, handle, request={'op':'now', 'now':100.})
        self.assertEqual(caught.exception.status, 503)
        self.assertEqual(lib.h2_matchmaking_buffers_live(), baseline)


class RustOwnerHttpLifecycleTests(unittest.IsolatedAsyncioTestCase):
    async def test_http_shutdown_retires_its_private_rust_owner(self):
        import tempfile
        from pathlib import Path
        from aiohttp.test_utils import TestClient, TestServer
        from multiplayer.master_server.service import Settings, create_app
        from multiplayer.master_server.tests.test_matchmaking_http import catalog
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)/'catalog.json'
            path.write_text(json.dumps(catalog()))
            app = create_app(Settings(state=Path(directory), public_url='http://127.0.0.1:8080',
                allow_http=True, matchmaking_catalog=str(path)))
            client = TestClient(TestServer(app))
            await client.start_server()
            core = next(iter(app.values())).matchmaking.core
            self.assertGreaterEqual(core.now(), 0)
            await client.close()
            with self.assertRaises(Problem) as caught:
                core.now()
            self.assertEqual(caught.exception.status, 503)

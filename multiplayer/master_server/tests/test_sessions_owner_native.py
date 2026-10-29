"""Session generation/IO authority through real HTTP and the private Rust ABI."""
import asyncio
from dataclasses import replace
import unittest
import uuid

from multiplayer.master_server.protocol import Problem
from multiplayer.master_server.native_matchmaking import native
from multiplayer.master_server.playlist_skill import POLICY
from . import test_matchmaking_http as baseline


class SessionOwnerHttpTests(unittest.IsolatedAsyncioTestCase):
    asyncSetUp = baseline.MatchmakingHttpTests.asyncSetUp
    session = baseline.MatchmakingHttpTests.session
    enqueue = baseline.MatchmakingHttpTests.enqueue

    async def test_session_request_and_failure_projections_cannot_replace_rust_admission(self):
        headers = await self.session()
        session = next(iter(self.api.sessions.values()))
        original = session.player
        for name, value in (('expires', 0), ('seen', 0), ('player', 'outsider')):
            with self.assertRaises(AttributeError): setattr(session, name, value)
        with self.assertRaises(TypeError): self.api.sessions[session.key] = session
        with self.assertRaises(AttributeError): self.api.sessions = {}
        with self.assertRaises(TypeError): self.api.requests[(original, str(uuid.uuid4()))] = 'forged'
        # Even a trusted Python extension mutating a projection cannot alter
        # the owner; authenticating/reading the same generation refreshes it.
        object.__setattr__(session, '_snapshot', replace(session._snapshot, player='outsider', expires=0))
        row = self.api.core._call('session_generation', generation=session.generation)
        self.assertEqual(row['player'], original)
        self.assertGreater(row['expires'], self.api.clock())
        self.assertIs(self.api.sessions[session.key], session)
        self.assertEqual(session.player, original)
        self.assertEqual((await self.enqueue(headers)).status, 201)
        self.assertEqual(self.api.ticket(original).players[0].id, original)

    async def test_all_external_local_grants_complete_before_any_profile_or_account_write(self):
        await self.session()
        db = self.api.directory.store.db
        before = db.execute('SELECT * FROM player_profiles').fetchall()
        response = await self.client.post('/v1/matchmaking/session', headers={'Authorization': 'Bearer one'}, json={
            'screen_name': 'Changed primary', 'maps': ['lockout'], 'compatibility': 'halo2-native-queue-v9', 'language': 0,
            'local_players': [{'screen_name': 'Valid local', 'identity_token': 'three'},
                              {'screen_name': 'Forged local', 'identity_token': 'forged'}]})
        self.assertEqual(response.status, 401, await response.text())
        self.assertIn('three', self.verified)
        self.assertEqual(db.execute('SELECT * FROM player_profiles').fetchall(), before)
        self.assertEqual(db.execute('SELECT count(*) FROM players').fetchone()[0], 1)
        self.assertEqual(len(self.api.sessions), 1)
        self.assertEqual(next(iter(self.api.sessions.values())).screen_name, 'Chosen Pilot')
        # Optional profile fields are absent or exact nine-byte inputs.
        response = await self.client.post('/v1/matchmaking/session', headers={'Authorization': 'Bearer one'}, json={
            'screen_name': 'Pilot', 'maps': ['lockout'], 'compatibility': 'test-v1', 'appearance': None})
        self.assertEqual(response.status, 400, await response.text())
        self.assertEqual(db.execute('SELECT * FROM player_profiles').fetchall(), before)

    async def test_retired_generation_cannot_requeue_or_overwrite_new_failure_state(self):
        headers = await self.session()
        old = next(iter(self.api.sessions.values()))
        request = str(uuid.uuid4())
        self.assertEqual((await self.enqueue(headers, request_id=request)).status, 201)
        self.api.failures[old.player] = old, 'party_changed'
        self.assertIs(self.api.failures[old.player][0], old)
        self.assertEqual((await self.client.delete('/v1/matchmaking/session', headers=headers)).status, 200)
        fresh_headers = await self.session()
        fresh = next(iter(self.api.sessions.values()))
        self.assertNotEqual(fresh.generation, old.generation)
        self.assertFalse(self.api.requests)
        self.assertFalse(self.api.failures)

        for operation in (lambda: self.api.core._call('session_queue_check', generation=old.generation,
                body=dict(playlist_id='ranked', revision='lab-1', request_id=request)),
                lambda: self.api.failures.__setitem__(old.player, (old, 'search_failed'))):
            with self.assertRaises(Problem) as caught: operation()
            self.assertEqual(caught.exception.status, 401)
        self.assertEqual((await self.enqueue(fresh_headers, request_id=request)).status, 201)
        self.assertEqual(len(self.api.requests), 1)
        self.assertFalse(self.api.failures)


    async def test_native_party_preflight_rejects_guests_and_missing_maps_before_skill_io(self):
        self.api.entries = tuple(replace(e, rank_configuration=POLICY) for e in self.api.entries)
        response = await self.client.post('/v1/matchmaking/session', headers={'Authorization': 'Bearer one'}, json={
            'screen_name': 'Pilot', 'maps': ['lockout'], 'compatibility': 'halo2-native-queue-v9', 'language': 0,
            'local_players': [{'screen_name': 'Guest'}]})
        self.assertEqual(response.status, 201, await response.text())
        headers = {'Authorization': 'Bearer ' + (await response.json())['token']}
        self.assertEqual((await self.enqueue(headers)).status, 409)
        headers = await self.session(maps=['ascension'])
        self.assertEqual((await self.enqueue(headers)).status, 400)
        self.assertEqual(self.api.directory.store.db.execute('SELECT count(*) FROM playlist_skill').fetchone()[0], 0)
        self.assertFalse(self.api.core.tickets)
        self.assertFalse(self.api.requests)

    async def test_revoking_while_http_body_waits_cannot_record_a_queue_request(self):
        headers = await self.session()
        entered, release = asyncio.Event(), asyncio.Event()
        original = self.api.directory.body
        async def body(request, *args, **kwargs):
            result = await original(request, *args, **kwargs)
            if request.path == '/v1/matchmaking/ticket':
                entered.set()
                await release.wait()
            return result
        self.api.directory.body = body
        pending = asyncio.create_task(self.enqueue(headers))
        try:
            await asyncio.wait_for(entered.wait(), 2)
            self.assertEqual((await self.client.delete('/v1/matchmaking/session', headers=headers)).status, 200)
        finally:
            release.set()
        response = await asyncio.wait_for(pending, 2)
        self.assertEqual(response.status, 401, await response.text())
        self.assertFalse(self.api.core.tickets)
        self.assertFalse(self.api.requests)

    async def test_default_library_has_no_session_expiry_restoration_entry(self):
        await self.session()
        session = next(iter(self.api.sessions.values()))
        if native().h2_matchmaking_test_inputs_enabled():
            # Explicit diagnostic builds can restore test operands; default
            # service libraries must reject this exact same operation.
            row = self.api.core._call('session_fixture_expires', generation=session.generation,
                                      expires=session.expires)
            self.assertEqual(row['expires'], session.expires)
        else:
            with self.assertRaises(Problem) as caught:
                self.api.core._call('session_fixture_expires', generation=session.generation, expires=0)
            self.assertEqual(caught.exception.status, 400)
        self.assertGreater(self.api.sessions[session.key].expires, self.api.clock())

"""Catalog/profile endpoint authority through the real Rust service ABI."""
from dataclasses import replace
import json
import unittest

from multiplayer.master_server.native_matchmaking import value
from multiplayer.master_server.playlist_skill import POLICY, LATER_POLICY
from multiplayer.master_server.protocol import Problem
from . import test_matchmaking_http as baseline


class CatalogWireTests(unittest.TestCase):
    def test_nested_duplicate_escaped_keys_and_nonfinite_values_are_rejected_by_rust(self):
        for raw in ('{"version":1,"playlists":[],"version":2}',
                    '{"version":1,"playlists":[{"id":"first","\\u0069d":"replacement"}]}',
                    '{"version":1,"playlists":[],"bad":NaN}',
                    '{"version":1,"playlists":[]} {}'):
            with self.subTest(raw=raw):
                with self.assertRaises(Problem) as caught:value('catalog_parse',raw=raw)
                self.assertEqual(caught.exception.status,400)


class CatalogOwnerHttpTests(unittest.IsolatedAsyncioTestCase):
    asyncSetUp=baseline.MatchmakingHttpTests.asyncSetUp
    session=baseline.MatchmakingHttpTests.session
    enqueue=baseline.MatchmakingHttpTests.enqueue

    async def test_python_catalog_snapshots_cannot_rewrite_public_rank_configuration_or_visibility(self):
        headers=await self.session()
        entries=tuple(replace(e,rank_configuration=POLICY) for e in self.api.entries)
        self.api.entries=entries
        who=next(iter(self.api.sessions.values())).player
        self.api.skills.snapshot(who,'ranked',POLICY)
        db=self.api.directory.store.db
        with db:db.execute("UPDATE playlist_skill SET raw_rank=15,xp=5500 WHERE player=?",(who,))
        projected=self.api.entries
        object.__setattr__(projected[0],'rank_configuration',LATER_POLICY)
        object.__setattr__(projected[0].policy,'ranked',False)
        response=await self.client.get('/v1/matchmaking/playlists',headers=headers)
        self.assertEqual(response.status,200,await response.text())
        body=await response.json()
        self.assertEqual(body['playlists'][0]['level'],16)
        self.assertTrue(body['playlists'][0]['ranked'])
        self.assertIsNone(body['playlists'][1]['level'])
        encoded=json.dumps(body)
        for private in (who,'raw_rank','xp','configuration','table_digest'):
            self.assertNotIn(private,encoded)
        self.assertEqual(self.api.core.entry('ranked').rank_configuration,POLICY)
        self.assertEqual(db.execute('SELECT configuration FROM playlist_skill').fetchone()[0],POLICY)

    async def test_metadata_only_change_invalidates_the_real_ticket_before_heartbeat_or_reuse(self):
        headers=await self.session()
        response=await self.enqueue(headers)
        self.assertEqual(response.status,201,await response.text())
        who=next(iter(self.api.sessions.values())).player
        ticket=self.api.ticket(who)
        self.api.entries=tuple(replace(e,rank_configuration=POLICY) for e in self.api.entries)
        with self.assertRaises(Problem) as caught:self.api.core.heartbeat(ticket.id)
        self.assertEqual(caught.exception.status,409)
        response=await self.client.get('/v1/matchmaking/ticket',headers=headers)
        self.assertEqual(response.status,409,await response.text())
        # Existing request state cannot revalidate the old admission silently.
        response=await self.enqueue(headers)
        self.assertEqual(response.status,409,await response.text())
        await self.client.delete('/v1/matchmaking/ticket',headers=headers)
        response=await self.enqueue(headers)
        self.assertEqual(response.status,201,await response.text())
        self.assertNotEqual(self.api.ticket(who).id,ticket.id)

    async def test_invalid_catalog_replacement_is_atomic_and_preserves_active_ticket(self):
        headers=await self.session()
        self.assertEqual((await self.enqueue(headers)).status,201)
        who=next(iter(self.api.sessions.values())).player
        ticket=self.api.ticket(who)
        before=self.api.entries
        invalid=tuple(replace(e,rank_configuration='unknown-policy') for e in before)
        with self.assertRaises(Problem):self.api.entries=invalid
        self.assertEqual(self.api.entries,before)
        self.assertEqual(self.api.ticket(who).id,ticket.id)
        self.api.core.heartbeat(ticket.id)

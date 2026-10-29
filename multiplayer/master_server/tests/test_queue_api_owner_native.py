"""Queue endpoint ownership and admission-bound terminal failures."""
import unittest
import uuid
from dataclasses import replace
from . import test_match_transport as baseline


class QueueApiOwnerTests(unittest.IsolatedAsyncioTestCase):
    asyncSetUp = baseline.MatchTransportTests.asyncSetUp
    session = baseline.MatchTransportTests.session
    enqueue = baseline.MatchTransportTests.enqueue
    pair = baseline.MatchTransportTests.pair

    async def test_all_signed_local_skill_reads_share_one_transaction_and_retry(self):
        from multiplayer.master_server.playlist_skill import POLICY
        self.api.entries = tuple(replace(e, rank_configuration=POLICY,
            policy=replace(e.policy, maximum_party=2)) for e in self.api.entries)
        response = await self.client.post('/v1/matchmaking/session', headers={'Authorization':'Bearer one'},
            json=dict(screen_name='Primary', maps=['lockout'], compatibility='halo2-native-queue-v9',
                      language=0, local_players=[dict(screen_name='Signed Local', discord_token='two')]))
        self.assertEqual(response.status, 201, await response.text())
        headers = {'Authorization':'Bearer '+(await response.json())['token']}
        session = next(iter(self.api.sessions.values()))
        local = session.seats()[1].player
        db = self.api.directory.store.db
        with db:
            db.execute('INSERT INTO playlist_ranks VALUES(?,?,40)', (local, 'casual'))
        request_id = str(uuid.uuid4())
        seed = self.api.core.hoppers.seed
        response = await self.enqueue(headers, 'casual', request_id=request_id)
        self.assertEqual(response.status, 409, await response.text())
        self.assertEqual(db.execute('SELECT count(*) FROM playlist_skill WHERE playlist=?', ('casual',)).fetchone()[0], 0)
        self.assertFalse(self.api.core.tickets)
        self.assertFalse(self.api.requests)
        self.assertEqual(self.api.core.hoppers.seed, seed)
        with db:
            db.execute('UPDATE playlist_ranks SET rank=1 WHERE player=? AND playlist=?', (local, 'casual'))
        response = await self.enqueue(headers, 'casual', request_id=request_id)
        self.assertEqual(response.status, 201, await response.text())
        self.assertEqual(db.execute('SELECT count(*) FROM playlist_skill WHERE playlist=?', ('casual',)).fetchone()[0], 2)

    async def test_old_destroyed_room_cannot_overwrite_newer_failure_on_same_session(self):
        headers, descriptors, _, _, _ = await self.pair()
        first = self.api.transport.rooms[descriptors[0]['id']]
        await first.close('First launch failed.')
        await self.api.transport.cleanup()
        for header in headers:
            response = await self.enqueue(header)
            self.assertEqual(response.status, 201, await response.text())
        response = await self.client.get('/v1/matchmaking/transport', headers=headers[0])
        self.assertEqual(response.status, 200, await response.text())
        second_id = (await response.json())['id']
        self.assertNotEqual(second_id, first.id)
        await self.api.transport.rooms[second_id].close('Newer launch failed.')
        await self.api.transport.cleanup()
        response = await self.client.post('/v1/matchmaking/failure', headers=headers[0],
            json=dict(match_id=first.id, kind='missing_content'))
        self.assertEqual(response.status, 409, await response.text())
        value = await (await self.client.get('/v1/matchmaking/ticket?failure=1', headers=headers[0])).json()
        self.assertEqual(value['failure'], 'search_failed')

"""Native account/queue/room population through authenticated HTTP fixtures."""
import asyncio
import json
import unittest
from multiplayer.master_server.player_presence import population
from . import test_matchmaking_http as http
from . import test_couch_online as couch
from . import test_match_transport as transport


class PopulationTests(unittest.IsolatedAsyncioTestCase):
    asyncSetUp = http.MatchmakingHttpTests.asyncSetUp
    catalog_fixture = staticmethod(couch.catalog)
    session = couch.CouchOnlineTests.session
    enqueue = couch.CouchOnlineTests.enqueue
    pair = transport.MatchTransportTests.pair

    async def test_unique_accounts_couch_guests_and_zero_playlists(self):
        headers = await self.session(local_players=[{"screen_name":"Signed","discord_token":"three"},
            {"screen_name":"Guest 3"},{"screen_name":"Guest 4"}])
        response = await self.client.post('/v1/account/session',headers={'Authorization':'Bearer one'},
            json={'screen_name':'Menu Owner'})
        self.assertEqual(response.status,201,await response.text())
        self.assertEqual((await self.enqueue(headers)).status,201)
        counts = population(self.api.directory)
        self.assertEqual(counts['online'],{'users':2,'guests':2})
        self.assertEqual(counts['queued'],counts['online'])
        self.assertEqual(counts['idle'],{'users':0,'guests':0})
        self.assertEqual(counts['playlists'][1]['online'],counts['online'])
        self.assertEqual(counts['playlists'][0]['online'],{'users':0,'guests':0})
        for private in ('private-subject','Menu Owner','Signed','Guest 3'):
            self.assertNotIn(private,json.dumps(counts))

    async def test_recent_activity_required_even_with_a_live_ticket(self):
        headers = await self.session()
        self.assertEqual((await self.enqueue(headers)).status,201)
        session = next(iter(self.api.sessions.values()))
        self.api.clock = lambda:session.seen + 30
        self.assertEqual(population(self.api.directory)['online'],{'users':0,'guests':0})
        self.assertTrue(self.api.core.tickets)

    async def test_proposal_and_room_handoff_count_once_and_retire_disconnected_seats(self):
        headers,_,sockets,queues,_ = await self.pair()
        for header in headers:
            self.assertEqual((await self.client.delete('/v1/matchmaking/session',headers=header)).status,200)
        counts = population(self.api.directory)
        self.assertEqual(counts['joining'],{'users':2,'guests':6})
        for socket in sockets:
            await socket.send_json({'loaded':'1234567890abcdef:000001a'})
        for queue in queues:
            self.assertEqual((await asyncio.wait_for(queue.get(),2))['phase'],'running')
        counts = population(self.api.directory)
        self.assertEqual(counts['online'],{'users':2,'guests':6})
        self.assertEqual(counts['playing'],counts['online'])
        self.assertEqual(counts['joining'],{'users':0,'guests':0})
        await sockets[0].close()
        for queue in queues:
            self.assertIsNone(await asyncio.wait_for(queue.get(),2))
        self.assertEqual(population(self.api.directory)['online'],{'users':0,'guests':0})

    async def test_disabled_matchmaking_still_counts_native_menu_users(self):
        response = await self.client.post('/v1/account/session',headers={'Authorization':'Bearer one'},
            json={'screen_name':'Menu Owner'})
        self.assertEqual(response.status,201,await response.text())
        core = self.api.core
        self.api.core = None
        try:
            counts = population(self.api.directory)
        finally:
            self.api.core = core
        self.assertEqual(counts['online'],counts['idle'])
        self.assertEqual(counts['online']['users'],1)
        self.assertEqual(counts['playlists'],[])


class OnlineStatusHttpTests(unittest.IsolatedAsyncioTestCase):
    asyncSetUp = http.MatchmakingHttpTests.asyncSetUp
    session = http.MatchmakingHttpTests.session
    enqueue = http.MatchmakingHttpTests.enqueue

    async def test_public_counts_exclude_private_rosters_and_do_not_renew_activity(self):
        headers = await self.session()
        self.assertEqual((await self.enqueue(headers)).status, 201)
        response = await self.client.get('/v1/online/status')
        self.assertEqual(response.status, 200)
        counts = await response.json()
        self.assertEqual(counts['online'], {'users': 1, 'guests': 0})
        self.assertEqual(counts['playlists'][0]['queued']['users'], 1)
        self.assertEqual((await (await self.client.get('/v1/health')).json())['matchmaking_players'], 1)
        self.assertNotIn('private-subject', json.dumps(counts))
        self.assertNotIn('Chosen Pilot', json.dumps(counts))
        self.assertEqual(response.headers['Cache-Control'], 'no-store')
        session = next(iter(self.api.sessions.values()))
        self.api.clock = lambda: session.seen + 30
        self.assertEqual((await (await self.client.get('/v1/online/status')).json())['online']['users'], 0)
        self.assertEqual((await self.client.get('/v1/online/status?player=any')).status, 400)

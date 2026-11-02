import json
import sqlite3
import unittest
from unittest.mock import patch
from multiplayer.master_server.couch_players import LocalPlayer
from multiplayer.master_server.player_presence import population, observed
from . import test_matchmaking_http as http
from . import test_match_transport as transport


class PresenceOwnerTests(unittest.IsolatedAsyncioTestCase):
    asyncSetUp = http.MatchmakingHttpTests.asyncSetUp
    session = http.MatchmakingHttpTests.session
    enqueue = http.MatchmakingHttpTests.enqueue

    async def test_python_seat_projection_cannot_invent_present_accounts(self):
        await self.session()
        session = next(iter(self.api.sessions.values()))
        seats = session.seats() + (LocalPlayer('forged-account', 'Forged', False),
                                   LocalPlayer('forged-guest', 'Guest', True))
        with patch.object(type(session), 'seats', return_value=seats):
            response = await self.client.get('/v1/online/status')
            self.assertEqual(response.status, 200, await response.text())
            counts = await response.json()
        self.assertEqual(counts['online'], {'users': 1, 'guests': 0})
        self.assertNotIn('forged', json.dumps(counts))

    async def test_status_read_is_sql_read_only_inside_an_outer_transaction(self):
        await self.session()
        db = self.api.directory.store.db
        db.execute('CREATE TEMP TABLE presence_outer_marker(value INTEGER)')
        db.execute('BEGIN')
        db.execute('INSERT INTO presence_outer_marker VALUES(117)')
        floor = db.execute('SELECT value FROM clock_floor WHERE id=1').fetchone()[0]
        writes = []
        def guard(action, table, column, database, trigger):
            if action in (sqlite3.SQLITE_INSERT, sqlite3.SQLITE_UPDATE, sqlite3.SQLITE_DELETE):
                writes.append((action, table))
                return sqlite3.SQLITE_DENY
            return sqlite3.SQLITE_OK
        db.set_authorizer(guard)
        try:
            response = await self.client.get('/v1/online/status')
            self.assertEqual(response.status, 200, await response.text())
            self.assertEqual(writes, [])
            self.assertTrue(db.in_transaction)
            self.assertEqual(db.execute('SELECT value FROM presence_outer_marker').fetchone()[0], 117)
            self.assertEqual(db.execute('SELECT value FROM clock_floor WHERE id=1').fetchone()[0], floor)
        finally:
            db.set_authorizer(None)
            db.rollback()

    async def test_clock_regression_does_not_revive_expired_presence(self):
        await self.session()
        seen = next(iter(self.api.sessions.values())).seen
        for elapsed, count in ((29.999, 1), (30, 0), (29, 0)):
            self.api.clock = lambda elapsed=elapsed: seen + elapsed
            response = await self.client.get('/v1/online/status')
            self.assertEqual(response.status, 200, await response.text())
            self.assertEqual((await response.json())['online']['users'], count, elapsed)

    async def test_wall_timestamp_is_monotonic_without_writing_the_registry(self):
        await self.session()
        directory = self.api.directory
        baseline = population(directory)['updated_at']
        with patch.object(directory.store, 'clock', return_value=baseline+100):
            self.assertEqual(population(directory)['updated_at'], baseline+100)
        with patch.object(directory.store, 'clock', return_value=baseline+1):
            self.assertEqual(population(directory)['updated_at'], baseline+100)

    async def test_native_owner_types_and_socket_banks_are_strict_and_bounded(self):
        from multiplayer.master_server.native_matchmaking import native, _response
        from multiplayer.master_server.protocol import Problem
        await self.session()
        directory = self.api.directory
        request = dict(op='presence',view='population',source=self.api.core._handle,
            now=directory.clans.clock(),queue_now=self.api.clock(),wall=int(directory.store.clock()),rooms=[])
        def call(handle, **changes):
            return _response(native().h2_matchmaking_owner_request,handle,request=request|changes)
        with self.assertRaises(Problem) as denied:
            call(self.api.core._handle)
        self.assertEqual(denied.exception.status,503)
        handle = directory.clans._sessions._handle
        for change in (
            {'source':handle},
            {'rooms':[dict(id=str(i),io_now=0,peers=[]) for i in range(65)]},
            {'rooms':[dict(id='unknown',io_now=0,peers=[],phase='running')]},
            {'rooms':[dict(id='unknown',io_now=0,peers=[dict(player=str(i),connection=1,closed=False,control_now=0) for i in range(17)])]},
        ):
            with self.assertRaises(Problem):call(handle,**change)
        self.assertEqual(call(handle)['online'],{'users':1,'guests':0})


class RoomPresenceOwnerTests(unittest.IsolatedAsyncioTestCase):
    asyncSetUp = http.MatchmakingHttpTests.asyncSetUp
    session = http.MatchmakingHttpTests.session
    enqueue = http.MatchmakingHttpTests.enqueue
    pair = transport.MatchTransportTests.pair

    async def test_room_seats_and_playlist_come_from_the_native_admission(self):
        headers, descriptors, _, _, _ = await self.pair()
        players = {session.player for session in self.api.sessions.values()}
        for header in headers:
            self.assertEqual((await self.client.delete('/v1/matchmaking/session', headers=header)).status, 200)
        room = self.api.transport.rooms[descriptors[0]['id']]
        with patch.object(room, 'connected_seats', return_value=[]):
            self.assertEqual(observed(self.api.directory), dict.fromkeys(players, 'online'))
            counts = population(self.api.directory)
        self.assertEqual(counts['online'], {'users': 2, 'guests': 0})
        self.assertEqual(counts['joining'], counts['online'])
        self.assertEqual(counts['playlists'][0]['joining'], counts['online'])

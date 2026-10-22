"""Linked providers must pass admission checks under their canonical account."""
import unittest
import uuid
from . import test_couch_online as couch
from . import test_matchmaking_http as baseline
from . import test_direct_peer_http as direct


class LinkedAdmissionTests(unittest.IsolatedAsyncioTestCase):
    asyncSetUp = baseline.MatchmakingHttpTests.asyncSetUp
    catalog_fixture = staticmethod(couch.catalog)
    session = couch.CouchOnlineTests.session

    async def link(self):
        await self.session(local_players=[{'screen_name': 'Signed seat', 'discord_token': 'three'}])
        db = self.api.directory.store.db
        player = db.execute("SELECT player FROM player_identities WHERE subject='private-subject-three'").fetchone()[0]
        with db:
            db.execute("INSERT INTO player_identities VALUES('steam','linked',?)", (player,))
        original = self.api.directory.verify_identity
        async def verify(token):
            return ('steam', 'linked') if token == 'alias' else await original(token)
        self.api.directory.verify_identity = verify
        return player

    async def request(self, owner, locals=()):
        return await self.client.post('/v1/matchmaking/session',
            headers={'Authorization': 'Bearer ' + owner}, json={
                'screen_name': 'Other PC', 'maps': ['lockout'],
                'compatibility': 'halo2-native-queue-v9', 'language': 0,
                'local_players': list(locals)})

    async def test_linked_primary_cannot_take_another_connections_seat(self):
        await self.link()
        response = await self.request('alias')
        self.assertEqual(response.status, 409, await response.text())
        self.assertEqual(len(self.api.sessions), 1)

    async def test_linked_couch_cannot_take_another_connections_seat(self):
        await self.link()
        response = await self.request('two', [{'screen_name': 'Alias', 'identity_token': 'alias'}])
        self.assertEqual(response.status, 409, await response.text())
        self.assertEqual(len(self.api.sessions), 1)

    async def test_aliases_cannot_occupy_two_local_slots(self):
        await self.link()
        response = await self.request('alias', [{'screen_name': 'Duplicate', 'discord_token': 'three'}])
        self.assertEqual(response.status, 400, await response.text())

    async def test_canonical_primary_can_replace_its_own_connection(self):
        await self.link()
        response = await self.request('one', [{'screen_name': 'Alias', 'identity_token': 'alias'}])
        self.assertEqual(response.status, 201, await response.text())
        self.assertEqual(len(self.api.sessions), 1)

    async def test_linked_couch_cannot_bypass_a_separate_persistent_party(self):
        player = await self.link()
        self.api.sessions.clear()
        db = self.api.directory.store.db
        party = str(uuid.uuid4())
        with db:
            db.execute('INSERT INTO parties VALUES(?,?,1)', (party, player))
            db.execute('INSERT INTO party_members(player,party,handle) VALUES(?,?,?)',
                       (player, party, str(uuid.uuid4())))
        response = await self.request('two', [{'screen_name': 'Alias', 'identity_token': 'alias'}])
        self.assertEqual(response.status, 409, await response.text())
        self.assertFalse(self.api.sessions)


class LinkedRunningRoomTests(unittest.IsolatedAsyncioTestCase):
    asyncSetUp = direct.DirectStarConnectivityTests.asyncSetUp
    session = direct.DirectStarConnectivityTests.session
    enqueue = direct.DirectStarConnectivityTests.enqueue
    connect = direct.DirectStarConnectivityTests.connect
    next = direct.DirectStarConnectivityTests.next
    catalog_fixture = direct.DirectStarConnectivityTests.catalog_fixture
    running_three = direct.DirectStarConnectivityTests.running_three
    request = LinkedAdmissionTests.request

    async def test_linked_couch_cannot_take_a_live_room_seat_after_session_loss(self):
        room, descriptors, connections, host = await self.running_three()
        other = next(i for i in range(3) if i != host)
        player = room.players[descriptors[other]['slot']][0]
        db = self.api.directory.store.db
        with db:
            db.execute("INSERT INTO player_identities VALUES('steam','linked',?)", (player,))
        original = self.api.directory.verify_identity
        async def verify(token):
            return ('steam', 'linked') if token == 'alias' else await original(token)
        self.api.directory.verify_identity = verify
        self.api.sessions = {key: session for key, session in self.api.sessions.items() if session.player != player}
        response = await self.request(('one', 'two', 'three')[host],
                                      [{'screen_name': 'Alias', 'identity_token': 'alias'}])
        self.assertEqual(response.status, 409, await response.text())
        self.assertEqual(room.phase, 'running')
        self.assertFalse(room.closed)
        self.assertEqual(len(room.tokens), 3)

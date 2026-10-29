"""Exercise result consent through authenticated signaling and the real ledger."""
import asyncio
import hashlib
import sqlite3
import time
import unittest
from dataclasses import replace
from types import SimpleNamespace

from multiplayer.master_server.match_results import MatchResults
from multiplayer.master_server.playlist_skill import POLICY
from multiplayer.master_server.protocol import Problem
from . import test_direct_peer_http as direct
from . import test_match_transport as transport


class ResultConsentHttpTests(unittest.IsolatedAsyncioTestCase):
    session = direct.DirectStarConnectivityTests.session
    enqueue = direct.DirectStarConnectivityTests.enqueue
    connect = direct.DirectStarConnectivityTests.connect
    next = direct.DirectStarConnectivityTests.next
    catalog_fixture = direct.DirectStarConnectivityTests.catalog_fixture
    running_three = direct.DirectStarConnectivityTests.running_three
    report = transport.MatchTransportTests.report

    async def asyncSetUp(self):
        await direct.DirectStarConnectivityTests.asyncSetUp(self)
        self.api.entries = tuple(replace(e, rank_configuration=POLICY) for e in self.api.entries)

    async def review(self, descriptors, connections, host):
        raw = self.report(descriptors)
        raw[0x370 + 0x94 + 0x90] = 1
        raw[0x370 + 2 * 0x94 + 0x90] = 2
        await connections[host][0].send_json({'postgame': raw.hex()})
        for socket, queue in connections:
            if not socket.closed:
                self.assertEqual((await self.next(queue, 'postgame'))['postgame'], raw.hex())
        return hashlib.sha256(raw).hexdigest()

    async def confirm(self, room, descriptor, connection, digest):
        owner = room.players[descriptor['slot']][0]
        await connection[0].send_json({'confirm_result': digest})
        async with asyncio.timeout(3):
            while owner not in room.confirmations:
                await asyncio.sleep(.005)
        return owner

    async def leave(self, connection):
        await connection[0].send_json({'leave': True})
        self.assertIsNone(await self.next(connection[1], 'closed'))

    def assert_no_result(self, room):
        db = self.api.directory.store.db
        for table in ('match_results', 'match_result_players', 'match_result_consensus',
                      'match_result_skill', 'carnage_reports'):
            self.assertEqual(db.execute('SELECT count(*) FROM ' + table).fetchone()[0], 0, table)
        self.assertEqual({r[0] for r in db.execute('SELECT xp FROM playlist_skill')}, {0})
        # PCR presentation may complete with unanimous surviving owners. The
        # immutable admitted electorate still guards every ledger/rank write.
        if room.phase == 'postgame':
            self.assertIs(room.result_recorded, False)
            self.assertNotEqual(room.confirmations, room.admitted_owners)

    async def test_sole_host_cannot_persist_rank_after_unconfirmed_peers_leave(self):
        room, descriptors, connections, host = await self.running_three()
        self.assertTrue(room.ranked)
        for i, connection in enumerate(connections):
            if i != host:
                await self.leave(connection)
        self.assertEqual(len(room.tokens), 1)
        self.assertEqual(len(room.players), 3)
        digest = await self.review(descriptors, connections, host)
        await self.confirm(room, descriptors[host], connections[host], digest)
        self.assert_no_result(room)

    async def test_unconfirmed_review_departure_does_not_supply_consent(self):
        room, descriptors, connections, host = await self.running_three()
        digest = await self.review(descriptors, connections, host)
        for i in range(3):
            if i != host:
                await self.confirm(room, descriptors[i], connections[i], digest)
        await self.leave(connections[host])
        self.assert_no_result(room)

    async def test_ledger_rejects_only_surviving_connections_confirming(self):
        room, descriptors, connections, host = await self.running_three()
        digest = await self.review(descriptors, connections, host)
        for i, connection in enumerate(connections):
            if i != host:
                await self.leave(connection)
        await self.confirm(room, descriptors[host], connections[host], digest)
        with self.assertRaises(Problem):
            self.api.results.commit(room)
        self.assert_no_result(room)

    async def test_confirmation_survives_departure_and_all_admitted_votes_commit(self):
        room, descriptors, connections, host = await self.running_three()
        digest = await self.review(descriptors, connections, host)
        leaving, last = [i for i in range(3) if i != host]
        owner = await self.confirm(room, descriptors[leaving], connections[leaving], digest)
        await self.leave(connections[leaving])
        self.assertIn(owner, room.confirmations)
        self.assertNotIn(owner, room.tokens)
        await self.confirm(room, descriptors[host], connections[host], digest)
        self.assert_no_result(room)
        await self.confirm(room, descriptors[last], connections[last], digest)
        for i in (host, last):
            self.assertEqual((await self.next(connections[i][1], 'completed'))['completed'], digest)
        self.assertEqual(room.phase, 'postgame')
        db = self.api.directory.store.db
        receipt = db.execute('SELECT * FROM match_result_consensus WHERE match=?', (room.id,)).fetchone()
        self.assertEqual(tuple(receipt), (room.id, 'native-unanimous-v1', 3, 3,
                                         1 << descriptors[leaving]['slot'], 0))
        for table in ('match_result_players', 'match_result_skill'):
            self.assertEqual(db.execute('SELECT count(*) FROM ' + table).fetchone()[0], 3)
        xp = db.execute('SELECT sum(xp) FROM playlist_skill').fetchone()[0]
        self.assertGreater(xp, 0)
        self.api.results.commit(room)
        self.assertEqual(db.execute('SELECT sum(xp) FROM playlist_skill').fetchone()[0], xp)
        self.assertEqual(db.execute('SELECT count(*) FROM carnage_reports').fetchone()[0], 1)

    async def test_timeout_does_not_turn_abstention_into_consent(self):
        room, descriptors, connections, host = await self.running_three()
        digest = await self.review(descriptors, connections, host)
        for i in range(3):
            if i != host:
                await self.confirm(room, descriptors[i], connections[i], digest)
        self.assert_no_result(room)
        expired = room.deadline + 1
        room.lifecycle_clock = lambda: expired
        await self.api.transport.cleanup()
        self.assertTrue(room.closed)
        self.assert_no_result(room)

    async def test_departed_confirmation_cannot_authorize_a_changed_report(self):
        room, descriptors, connections, host = await self.running_three()
        digest = await self.review(descriptors, connections, host)
        leaving = next(i for i in range(3) if i != host)
        await self.confirm(room, descriptors[leaving], connections[leaving], digest)
        await self.leave(connections[leaving])
        changed = bytearray(room.result)
        changed[0x370 + 0x92] ^= 1  # A different score still fits the admitted roster.
        await connections[host][0].send_json({'postgame': changed.hex()})
        self.assertIsNone(await self.next(connections[host][1], 'closed'))
        self.assertFalse(room.closed)
        self.assertNotIn(room.players[descriptors[host]['slot']][0], room.tokens)
        self.assert_no_result(room)

    async def test_no_connections_or_votes_cannot_satisfy_the_ledger(self):
        room, descriptors, connections, host = await self.running_three()
        await self.review(descriptors, connections, host)
        for i in [i for i in range(3) if i != host] + [host]:
            await self.leave(connections[i])
        self.assertTrue(room.closed)
        self.assertFalse(room.tokens)
        self.assertFalse(room.confirmations)
        with self.assertRaises(Problem):
            self.api.results.commit(room)
        self.assert_no_result(room)


class ResultConsentSchemaTests(unittest.TestCase):
    def test_legacy_consensus_receipts_are_preserved_across_upgrade_and_restart(self):
        db = sqlite3.connect(':memory:')
        self.addCleanup(db.close)
        db.execute('PRAGMA foreign_keys=ON')
        db.execute('CREATE TABLE players(id TEXT PRIMARY KEY)')
        store = SimpleNamespace(db=db)
        MatchResults(store, None, None)
        db.execute('DROP TABLE match_result_consensus')
        db.execute("""CREATE TABLE match_result_consensus(
            match TEXT PRIMARY KEY REFERENCES match_results(id),
            policy TEXT NOT NULL CHECK(policy='native-survivors-v1'),
            admitted_connections INTEGER NOT NULL, confirming_connections INTEGER NOT NULL,
            departed_slots INTEGER NOT NULL, authority_epoch INTEGER NOT NULL)""")
        with db:
            db.execute("INSERT INTO match_results VALUES('old','ranked','r1','lockout','Slayer',1,1,'digest','native-unanimous-v1')")
            db.execute("INSERT INTO match_result_consensus VALUES('old','native-survivors-v1',3,1,6,2)")
        MatchResults(store, None, None)
        self.assertEqual(db.execute('SELECT * FROM match_result_consensus').fetchone(),
                         ('old', 'native-survivors-v1', 3, 1, 6, 2))
        with db:
            db.execute("INSERT INTO match_results VALUES('new','ranked','r1','lockout','Slayer',1,2,'digest','native-unanimous-v1')")
            db.execute("INSERT INTO match_result_consensus VALUES('new','native-unanimous-v1',3,3,2,0)")
        MatchResults(store, None, None)
        self.assertEqual(db.execute('SELECT count(*) FROM match_result_consensus').fetchone()[0], 2)
        self.assertFalse(db.execute("SELECT name FROM sqlite_master WHERE name='match_result_consensus_v2'").fetchall())
        self.assertFalse(db.execute('PRAGMA foreign_key_check').fetchall())

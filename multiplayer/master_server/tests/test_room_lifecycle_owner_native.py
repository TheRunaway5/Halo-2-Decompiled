"""Production room/result authority over declared loaded-world fixtures."""
import unittest
import sqlite3
from multiplayer.master_server.protocol import Problem
from . import test_ranked_results as baseline


class RoomLifecycleOwnerTests(unittest.IsolatedAsyncioTestCase):
    asyncSetUp = baseline.RankedResultsTests.asyncSetUp
    session = baseline.RankedResultsTests.session
    enqueue = baseline.RankedResultsTests.enqueue
    pair = baseline.RankedResultsTests.pair
    report = baseline.RankedResultsTests.report
    running = baseline.RankedResultsTests.running

    async def test_nonhost_cannot_bypass_the_transport_report_guard_through_the_result_adapter(self):
        _, descriptors, _, _, host = await self.running()
        room = self.api.transport.rooms[descriptors[0]['id']]
        actor = next(room.players[d['slot']][0] for d in descriptors if d['slot'] != host)
        before = self.api.results.snapshot(room)
        with self.assertRaises(Problem) as caught:
            self.api.results.submit(room, actor, self.report(descriptors).hex())
        self.assertEqual(caught.exception.status, 403)
        self.assertEqual(self.api.results.snapshot(room), before)

    async def test_sql_failure_keeps_review_pending_and_retry_finalizes_once(self):
        _, descriptors, _, _, host = await self.running()
        room = self.api.transport.rooms[descriptors[0]['id']]
        actor = room.players[host][0]
        digest = self.api.results.submit(room, actor, self.report(descriptors).hex())
        for owner in room.tokens:
            self.api.results.confirm(room, owner, digest)
        db = self.api.directory.store.db
        def deny(action, table, *unused):
            return sqlite3.SQLITE_DENY if action == sqlite3.SQLITE_INSERT and table == 'match_result_admission' else sqlite3.SQLITE_OK
        db.set_authorizer(deny)
        try:
            with self.assertRaises(sqlite3.DatabaseError): room.finish_review()
        finally:
            db.set_authorizer(None)
        self.assertEqual(room.phase, 'reviewing')
        self.assertIsNone(room.result_recorded)
        self.assertEqual(db.execute('SELECT count(*) FROM match_results').fetchone()[0], 0)
        room.finish_review()
        self.assertEqual(room.phase, 'postgame')
        self.assertIs(room.result_recorded, True)
        room.finish_review()
        self.assertEqual(db.execute('SELECT count(*) FROM match_results').fetchone()[0], 1)

    async def test_phase_authority_deadline_and_departure_projections_are_read_only(self):
        _, descriptors, _, _, _ = await self.running()
        room = self.api.transport.rooms[descriptors[0]['id']]
        before = room.room_state()
        for field, value in (('phase','postgame'),('host',99),('deadline',0),('closed',True),
                             ('result_recorded',True),('departed_slots',65535),('setup_host_departed',True)):
            with self.assertRaises(AttributeError): setattr(room,field,value)
        self.assertEqual(room.room_state(),before)
        for op in ('room_state_restore','room_state_install'):
            with self.assertRaises(Problem): room.room_call(op,value=before)
        self.assertEqual(room.room_state(),before)

    async def test_expired_room_rejects_report_before_any_writer_cleanup(self):
        _, descriptors, _, _, host = await self.running()
        room = self.api.transport.rooms[descriptors[0]['id']]
        before = self.api.results.snapshot(room)
        expired = room.deadline + 1
        room.lifecycle_clock = lambda: expired
        with self.assertRaises(Problem) as caught:
            self.api.results.submit(room, room.players[host][0], self.report(descriptors).hex())
        self.assertEqual(caught.exception.status, 409)
        self.assertEqual(self.api.results.snapshot(room), before)
        self.assertTrue(room.expired())

    async def test_queue_clock_cannot_expire_transferred_match_capability(self):
        _, descriptors, _, _, _ = await self.running()
        room = self.api.transport.rooms[descriptors[0]['id']]
        before = room.room_state()
        queue_now = self.api.core.clock() + 10000
        self.api.core.clock = lambda: queue_now
        self.assertTrue(room.loaded)
        self.assertEqual(room.room_state(), before)
        self.assertFalse(room.expired())

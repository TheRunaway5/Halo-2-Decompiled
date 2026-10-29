"""Shared pregame publication must follow the original parameter writer."""
from dataclasses import replace
import unittest
import uuid
from . import test_parties_http as baseline


class CountdownOwnerTests(unittest.IsolatedAsyncioTestCase):
    asyncSetUp = baseline.PartiesHttpTests.asyncSetUp
    start = baseline.PartiesHttpTests.start
    login = baseline.PartiesHttpTests.login
    view = baseline.PartiesHttpTests.view
    command = baseline.PartiesHttpTests.command
    join = baseline.PartiesHttpTests.join
    queue_session = baseline.PartiesHttpTests.queue_session

    async def ready_three(self):
        self.queue.entries = tuple(replace(e, policy=replace(e.policy, team_size=0, minimum_players=4,
            maximum_players=4, maximum_party=3)) for e in self.queue.entries)
        await self.command(self.a, 'create')
        await self.join(self.a, self.b, 'pilot_two')
        await self.join(self.a, self.c, 'pilot_three')
        for owner in ('one', 'two', 'three'):
            await self.queue_session(owner)
        for account in (self.b, self.c):
            await self.command(account, 'ready', playlist_id='ranked', revision='lab-1')
        now = [0.0]
        self.parties.clock = lambda: now[0]
        return now

    async def countdown(self, account):
        response = await self.client.get('/v1/party/countdown', headers=account)
        self.assertEqual(response.status, 200, await response.text())
        return (await response.json())['countdown']

    async def test_identical_member_tuple_preserves_first_requester_serial_and_clock(self):
        now = await self.ready_three()
        await self.command(self.a, 'countdown_start', playlist_id='ranked', revision='lab-1')
        await self.command(self.b, 'countdown_request')
        before = await self.countdown(self.a)
        now[0] = .9
        await self.command(self.c, 'countdown_request')
        self.assertEqual(await self.countdown(self.a), before)
        now[0] = 1.0
        self.parties.advance_countdowns()
        current = await self.countdown(self.c)
        self.assertEqual((current['seconds'], current['mode'], current['requester']), (9, 0, None))

    def leader(self):
        return self.directory.store.db.execute('SELECT leader FROM parties').fetchone()[0]

    async def test_sql_failure_never_publishes_and_same_request_can_retry(self):
        import sqlite3
        now = await self.ready_three()
        db = self.directory.store.db
        body = dict(action='countdown_start', request_id=str(uuid.uuid4()),
                    version=(await self.view(self.a))['party']['version'], playlist_id='ranked', revision='lab-1')
        with db:
            db.execute("CREATE TRIGGER reject_countdown_request BEFORE INSERT ON party_requests "
                       "BEGIN SELECT RAISE(ABORT,'injected command failure'); END")
        for _ in range(2):
            with self.assertRaises(sqlite3.IntegrityError):
                self.parties.countdown_command(self.leader(), body)
            self.assertIsNone(await self.countdown(self.a))
            self.assertIsNone(db.execute('SELECT request FROM party_requests WHERE request=?', (body['request_id'],)).fetchone())
        with db: db.execute('DROP TRIGGER reject_countdown_request')
        self.parties.countdown_command(self.leader(), body)
        first = await self.countdown(self.a)
        self.assertEqual((first['seconds'], first['serial']), (10, 1))
        now[0] = .9
        self.parties.countdown_command(self.leader(), body)
        self.assertEqual(await self.countdown(self.a), first)
        now[0] = 1.0
        self.parties.advance_countdowns()
        self.assertEqual((await self.countdown(self.b))['seconds'], 9)

    async def test_full_profile_change_retires_countdown_and_requires_fresh_remote_consent(self):
        await self.ready_three()
        await self.command(self.a, 'countdown_start', playlist_id='ranked', revision='lab-1')
        self.queue.entries = tuple(replace(e, description=e.description+' Changed profile') for e in self.queue.entries)
        self.assertIsNone(await self.countdown(self.b))
        await self.command(self.a, 'countdown_start', 409, playlist_id='ranked', revision='lab-1')
        for account in (self.b, self.c):
            await self.command(account, 'ready', playlist_id='ranked', revision='lab-1')
        await self.command(self.a, 'countdown_start', playlist_id='ranked', revision='lab-1')
        self.assertEqual((await self.countdown(self.c))['seconds'], 10)

    async def test_connection_retirement_clears_live_countdown_before_directory_notification(self):
        from unittest.mock import patch
        await self.ready_three()
        await self.command(self.a, 'countdown_start', playlist_id='ranked', revision='lab-1')
        before = self.directory.store.db.execute('SELECT version FROM parties').fetchone()[0]
        with patch.object(self.parties, 'session_lost'):
            await self.queue_session('two')
        self.assertEqual(self.directory.store.db.execute('SELECT version FROM parties').fetchone()[0], before)
        self.assertIsNone(await self.countdown(self.c))
        await self.command(self.a, 'countdown_start', 409, playlist_id='ranked', revision='lab-1')

    async def test_native_publication_projections_cannot_be_mutated_or_replaced(self):
        from multiplayer.master_server.native_matchmaking import value
        from multiplayer.master_server.protocol import Problem
        await self.ready_three()
        await self.command(self.a, 'countdown_start', playlist_id='ranked', revision='lab-1')
        mapping = self.parties.countdowns
        key = next(iter(mapping))
        row = mapping[key]
        self.assertIs(row, mapping[key])
        copied = mapping.copy()
        self.assertEqual(copied, mapping)
        copied.clear()
        self.assertIn(key, mapping)
        with self.assertRaises(AttributeError): row.seconds = 0
        with self.assertRaises(TypeError): mapping[key] = row
        with self.assertRaises(AttributeError): self.parties.countdowns = {}
        with self.assertRaises(Problem) as raised:
            self.queue.core._call('party_countdown_restore', party=key, value={})
        self.assertEqual(raised.exception.status, 400)
        # A pure arithmetic sample is available without an installation path.
        sample = value('party_countdown_sample', value=dict(serial=9, seconds=0, clock_ms=0,
            playlist='ranked', revision='lab-1', party_version=1, mode=0, complete=True, requester=None))
        self.assertEqual(sample['seconds'], 0)
        self.assertEqual((await self.countdown(self.a))['seconds'], 10)

    async def test_idle_pump_needs_no_directory_database_reads(self):
        self.assertFalse(self.parties.countdowns)
        self.directory.store.db.close()
        self.parties.advance_countdowns()

    async def test_same_process_service_reinitialization_keeps_membership_and_resets_countdown(self):
        await self.ready_three()
        await self.command(self.a, 'countdown_start', playlist_id='ranked', revision='lab-1')
        db = self.directory.store.db
        before = db.execute('SELECT player,party,handle FROM party_members ORDER BY rowid').fetchall()
        await self.client.close()
        await self.start()
        self.directory = next(iter(self.app.values()))
        self.parties, self.queue = self.directory.parties, self.directory.matchmaking
        self.a, self.b = await self.login('one'), await self.login('two')
        self.assertEqual(self.directory.store.db.execute('SELECT player,party,handle FROM party_members ORDER BY rowid').fetchall(), before)
        self.assertIsNone(await self.countdown(self.b))
        self.assertFalse(self.parties.countdowns)

"""Accepted-party consent must bind the native profile and connection identity."""
from dataclasses import replace
import unittest
from . import test_parties_http as baseline


class PartyConsentOwnerTests(unittest.IsolatedAsyncioTestCase):
    asyncSetUp = baseline.PartiesHttpTests.asyncSetUp
    start = baseline.PartiesHttpTests.start
    login = baseline.PartiesHttpTests.login
    view = baseline.PartiesHttpTests.view
    command = baseline.PartiesHttpTests.command
    join = baseline.PartiesHttpTests.join
    queue_session = baseline.PartiesHttpTests.queue_session
    enqueue = baseline.PartiesHttpTests.enqueue
    team_policy = baseline.PartiesHttpTests.team_policy

    async def test_same_revision_catalog_change_requires_fresh_remote_consent(self):
        self.team_policy()
        await self.command(self.a, 'create')
        await self.join(self.a, self.b, 'pilot_two')
        headers = [await self.queue_session(owner) for owner in ('one', 'two')]
        await self.command(self.b, 'ready', playlist_id='ranked', revision='lab-1')
        self.queue.entries = tuple(replace(e, description=e.description+' Updated profile') for e in self.queue.entries)
        self.assertTrue(all(not row['ready_playlist'] for row in (await self.view(self.b))['party']['members']))
        await self.enqueue(headers[0], 409)
        self.assertFalse(self.queue.core.tickets)
        await self.command(self.b, 'ready', playlist_id='ranked', revision='lab-1')
        await self.enqueue(headers[0])

    async def accepted_pair(self):
        self.team_policy()
        await self.command(self.a, 'create')
        await self.join(self.a, self.b, 'pilot_two')
        headers = [await self.queue_session(owner) for owner in ('one', 'two')]
        await self.command(self.b, 'ready', playlist_id='ranked', revision='lab-1')
        return headers

    async def test_connection_replacement_invalidates_roster_consent_before_notification(self):
        from unittest.mock import patch
        headers = await self.accepted_pair()
        before = self.directory.store.db.execute('SELECT version FROM parties').fetchone()[0]
        with patch.object(self.parties, 'session_lost'):
            response = await self.client.delete('/v1/matchmaking/session', headers=headers[0])
            self.assertEqual(response.status, 200, await response.text())
            fresh = await self.queue_session('one')
        self.assertEqual(self.directory.store.db.execute('SELECT version FROM parties').fetchone()[0], before)
        await self.enqueue(fresh, 409)
        self.assertTrue(all(not row['ready_playlist'] for row in (await self.view(self.b))['party']['members']))
        await self.command(self.b, 'ready', playlist_id='ranked', revision='lab-1')
        await self.enqueue(fresh)

    async def test_sql_failure_rolls_back_legacy_and_native_consent_together(self):
        import sqlite3
        from multiplayer.master_server.native_database import apply
        self.team_policy()
        await self.command(self.a, 'create')
        await self.join(self.a, self.b, 'pilot_two')
        headers = [await self.queue_session(owner) for owner in ('one', 'two')]
        db = self.directory.store.db
        who = db.execute("SELECT player FROM player_identities WHERE subject='private-discord-subject-two'").fetchone()[0]
        facts = self.parties.matching_facts(who)
        plan = self.queue.core._call('party_ready', player=who, playlist='ranked', revision='lab-1',
            version=facts['party']['version'], facts=facts, rooms=self.queue.room_facts())
        with db:
            db.execute("CREATE TRIGGER reject_native_consent BEFORE INSERT ON matchmaking_party_consents "
                       "BEGIN SELECT RAISE(ABORT,'injected consent failure'); END")
        with self.assertRaises(sqlite3.IntegrityError):
            with db:
                apply(db, plan)
        self.assertEqual(db.execute('SELECT count(*) FROM matchmaking_party_consents').fetchone()[0], 0)
        self.assertTrue(all(row[0] == '' and row[1] == 0 for row in db.execute('SELECT ready_playlist,ready_version FROM party_members')))
        await self.enqueue(headers[0], 409)
        with db: db.execute('DROP TRIGGER reject_native_consent')
        await self.command(self.b, 'ready', playlist_id='ranked', revision='lab-1')
        await self.enqueue(headers[0])

    async def test_restart_keeps_membership_and_migrates_storage_but_resets_consent(self):
        await self.accepted_pair()
        db = self.directory.store.db
        before = db.execute('SELECT player,party,handle FROM party_members ORDER BY rowid').fetchall()
        # Exercise the prior development storage shape without whole-roster binding.
        with db:
            db.execute('DROP TABLE matchmaking_party_consents')
            db.execute('CREATE TABLE matchmaking_party_consents(player TEXT PRIMARY KEY,party TEXT,version INTEGER,handle TEXT,'
                       'playlist TEXT,revision TEXT,profile TEXT,session_key TEXT,generation INTEGER)')
        await self.client.close()
        await self.start()
        self.directory = next(iter(self.app.values()))
        self.parties, self.queue = self.directory.parties, self.directory.matchmaking
        self.a, self.b = await self.login('one'), await self.login('two')
        self.team_policy()
        db = self.directory.store.db
        self.assertEqual(db.execute('SELECT player,party,handle FROM party_members ORDER BY rowid').fetchall(), before)
        self.assertIn('roster', {row[1] for row in db.execute('PRAGMA table_info(matchmaking_party_consents)')})
        self.assertEqual(db.execute('SELECT count(*) FROM matchmaking_party_consents').fetchone()[0], 0)
        headers = [await self.queue_session(owner) for owner in ('one', 'two')]
        await self.enqueue(headers[0], 409)
        await self.command(self.b, 'ready', playlist_id='ranked', revision='lab-1')
        await self.enqueue(headers[0])

    async def test_match_admission_binds_each_accepted_connection_without_public_leak(self):
        import json
        headers = await self.accepted_pair()
        result = await self.enqueue(headers[0])
        for owner in ('three', 'four'):
            await self.enqueue(await self.queue_session(owner))
        proposals = tuple(self.queue.core.proposals.values())
        self.assertEqual(len(proposals), 1)
        bound = self.queue.core.admission(proposals[0].id)['bound']
        party = next(p for p in bound['party_bindings'] if p['version'] is not None)
        self.assertEqual(len(party['members']), 2)
        self.assertTrue(all(len(p['session_key']) == 64 and p['generation'] > 0 for p in party['members']))
        db = self.directory.store.db
        self.assertEqual(party['party'], db.execute('SELECT id FROM parties').fetchone()[0])
        self.assertEqual(party['profile'], bound['profile_sha256'])
        for private in ('session_key', 'party_bindings', 'profile_sha256', party['party']):
            self.assertNotIn(private, json.dumps(result))

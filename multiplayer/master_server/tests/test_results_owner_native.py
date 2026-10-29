"""Rust result authority through real IO; no Live/played-match parity claim."""
import asyncio
from contextlib import closing
from dataclasses import replace
import json
import sqlite3
import threading
import unittest

from multiplayer.master_server.native_database import run as database
from multiplayer.master_server.protocol import Problem
from . import test_ranked_results as ranked


class RustResultOwnerTests(unittest.IsolatedAsyncioTestCase):
    asyncSetUp = ranked.RankedResultsTests.asyncSetUp
    session = ranked.RankedResultsTests.session
    enqueue = ranked.RankedResultsTests.enqueue
    pair = ranked.RankedResultsTests.pair
    report = ranked.RankedResultsTests.report
    running = ranked.RankedResultsTests.running

    async def ready(self):
        _, descriptors, _, _, _ = await self.running()
        room = self.api.transport.rooms[descriptors[0]['id']]
        raw = self.report(descriptors)
        raw[0x370+0x94+0x90] = 1
        digest = self.api.results.submit(room, room.players[room.host][0], raw.hex())
        for owner in room.tokens:
            self.api.results.confirm(room, owner, digest)
        return room, raw

    async def test_python_room_projections_cannot_rewrite_admitted_results_skills_or_accuracy(self):
        room, raw = await self.ready()
        binding = self.api.results.snapshot(room)['admission_sha256']
        room.proposal = replace(room.proposal, game=replace(room.proposal.game, variant='King', map='ascension'))
        room.ranked, room.team_mode = False, True
        room.skills, room.clans, room.clan_skills = {}, {0:'forged'}, {}
        room.guests = frozenset(p for p,_,_ in room.players)
        room.supports_accuracy = True
        with self.assertRaises(AttributeError):
            room.confirmations = frozenset()
        with self.assertRaises(TypeError):
            room.identities[next(iter(room.identities))] = '00'*8
        self.assertEqual(self.api.results.validate(room, raw.hex())[0], raw)
        self.api.results.commit(room)
        db = self.api.directory.store.db
        self.assertEqual(tuple(db.execute('SELECT map,variant,ranked FROM match_results').fetchone()),('lockout','Slayer',1))
        self.assertEqual(db.execute('SELECT count(*) FROM match_result_skill').fetchone()[0],2)
        self.assertEqual(db.execute('SELECT sum(xp) FROM playlist_skill').fetchone()[0],100)
        self.assertEqual(db.execute('SELECT format FROM carnage_reports').fetchone()[0],'native-verified-summary-v2')
        self.assertEqual(db.execute('SELECT count(*) FROM carnage_player_accuracy').fetchone()[0],0)
        namespace, pin, metadata = db.execute('SELECT namespace,admission_sha256,metadata FROM match_result_admission').fetchone()
        self.assertEqual((namespace,pin),('native-matchmaking-v1',binding))
        admission=json.loads(metadata)['admission']
        self.assertEqual(admission['proposal']['game']['map'],'lockout')
        self.assertIn('original effective-to-result provenance unclosed',admission['comparison_skill_policy'])

    async def test_two_sqlite_connections_finalize_one_digest_exactly_once(self):
        room, raw = await self.ready()
        store = self.api.directory.store
        completed = store.now()
        store.db.commit()
        path = self.api.directory.settings.state/'registry.sqlite3'
        request = dict(source=self.api.core._handle, match=room.id, completed=completed)
        barrier = threading.Barrier(2, timeout=5)
        def finish():
            with closing(sqlite3.connect(path, timeout=10)) as db:
                db.execute('PRAGMA foreign_keys=ON')
                barrier.wait()
                return database(db, 'result_commit', **request)
        values = await asyncio.gather(asyncio.to_thread(finish),asyncio.to_thread(finish))
        self.assertEqual(sorted(v['already_committed'] for v in values),[False,True])
        for key in ('public','digest','admission_sha256','envelope_sha256'):
            self.assertEqual(values[0][key],values[1][key])
        db = store.db
        self.assertEqual(db.execute('SELECT count(*) FROM match_results').fetchone()[0],1)
        self.assertEqual(db.execute('SELECT count(*) FROM match_result_admission').fetchone()[0],1)
        self.assertEqual(db.execute('SELECT count(*) FROM match_result_skill').fetchone()[0],2)
        self.assertEqual(db.execute('SELECT sum(xp) FROM playlist_skill').fetchone()[0],100)

        self.assertEqual({r[0] for r in db.execute('SELECT revision FROM eos_profile_outbox')},{1})
        self.api.results.commit(room)
        self.assertEqual(db.execute('SELECT sum(xp) FROM playlist_skill').fetchone()[0],100)

    async def test_result_epoch_cannot_be_supplied_by_the_python_adapter(self):
        room, _ = await self.ready()
        store = self.api.directory.store
        with self.assertRaises(Problem) as caught:
            database(store.db, 'result_commit', source=self.api.core._handle,
                     match=room.id, completed=store.now(), epoch=1234)
        self.assertEqual(caught.exception.status, 400)
        self.assertEqual(store.db.execute('SELECT count(*) FROM match_results').fetchone()[0], 0)
        self.api.results.commit(room)
        metadata = json.loads(store.db.execute('SELECT metadata FROM match_result_admission').fetchone()[0])
        self.assertEqual(metadata['finalization']['authority_epoch'], 0)

    async def test_unknown_and_departed_owners_cannot_mutate_votes_or_quarantine(self):
        room, raw = await self.ready()
        before=self.api.results.snapshot(room)
        for owner in ('outsider',):
            with self.assertRaises(Problem) as caught:
                self.api.results.confirm(room,owner,'f'*64)
            self.assertEqual(caught.exception.status,403)
        self.assertEqual(self.api.results.snapshot(room),before)
        owner=next(iter(room.tokens))
        self.api.results.revoke(room,owner)
        with self.assertRaises(Problem) as caught:self.api.results.submit(room,owner,raw.hex())
        self.assertEqual(caught.exception.status,403)
        # Previously cast votes survive a transport departure.
        self.assertEqual(self.api.results.snapshot(room)['confirmations'],before['confirmations'])
        self.assertIsNone(self.api.results.snapshot(room)['quarantined'])


class RustSchemaAtomicityTests(unittest.TestCase):
    def test_failed_schema_creation_rolls_back_every_new_table_and_can_retry(self):
        with closing(sqlite3.connect(':memory:')) as db:
            db.execute('CREATE TABLE players(id TEXT PRIMARY KEY)')
            def deny(action, table, *unused):
                return sqlite3.SQLITE_DENY if action==sqlite3.SQLITE_CREATE_TABLE and table=='match_result_admission' else sqlite3.SQLITE_OK
            db.set_authorizer(deny)
            with self.assertRaises(sqlite3.DatabaseError):database(db,'result_schema')
            db.set_authorizer(None)
            tables={row[0] for row in db.execute("SELECT name FROM sqlite_master WHERE type='table'")}
            self.assertEqual(tables,{'players'})
            database(db,'result_schema')
            self.assertEqual(db.execute('SELECT count(*) FROM matchmaking_ledger_lock').fetchone()[0],1)
            self.assertFalse(db.execute('PRAGMA foreign_key_check').fetchall())

import json
import unittest
from types import SimpleNamespace as S
from unittest.mock import patch
from multiplayer.master_server.player_presence import population
from multiplayer.master_server.couch_players import LocalPlayer
from . import test_matchmaking_http as http


class PopulationTests(unittest.TestCase):
    def setUp(self):
        self.clock = patch('multiplayer.master_server.player_presence.time.monotonic', return_value=100)
        self.clock.start()
        self.addCleanup(self.clock.stop)
        self.api = S(clock=lambda: 100, sessions={}, entries=[S(policy=S(id=p), name=p.title())
            for p in ('ranked', 'casual')], core=S(tickets={}, proposals={}))
        self.directory = S(clans=S(clock=lambda: 100, sessions={}), matchmaking=self.api,
            store=S(now=lambda: 1700000000), match_transport=S(rooms={}))

    def session(self, player, *, seen=99, expires=200, seats=None):
        result = S(player=player, seen=seen, expires=expires)
        if seats is not None:
            result.seats = lambda: tuple(LocalPlayer(name, name, guest) for name, guest in seats)
        return result

    def ticket(self, key, playlist, players, expires=150):
        self.api.core.tickets[key] = S(id=key, playlist=playlist, expires_at=expires,
            players=[S(id=name) for name in players])

    def room(self, phase='running', *, deadline=150, closed=False, socket_closed=False,
             challenge='', sent=99):
        self.directory.match_transport.rooms['match'] = S(phase=phase, deadline=deadline, closed=closed,
            peers={'owner': S(socket=S(closed=socket_closed), challenge=challenge, sent=sent)},
            owners={'owner': 'owner', 'couch': 'owner', 'guest': 'owner'}, guests={'guest'},
            proposal=S(playlist='ranked'))

    def test_unique_accounts_couch_guests_and_zero_playlists(self):
        self.directory.clans.sessions['social'] = self.session('owner')
        self.api.sessions['queue'] = self.session('owner', seats=[('owner', False), ('couch', False), ('guest', True)])
        self.api.sessions['duplicate'] = self.session('couch')
        self.ticket('party', 'ranked', ['owner', 'couch', 'guest', 'unobserved'])
        counts = population(self.directory)
        self.assertEqual(counts['online'], {'users': 2, 'guests': 1})
        self.assertEqual(counts['queued'], counts['online'])
        self.assertEqual(counts['idle'], {'users': 0, 'guests': 0})
        self.assertEqual(counts['playlists'][0]['online'], counts['online'])
        self.assertEqual(counts['playlists'][1]['online'], {'users': 0, 'guests': 0})
        self.assertNotIn('owner', json.dumps(counts))
        self.assertNotIn('couch', json.dumps(counts))
        self.assertNotIn('guest"', json.dumps(counts))

    def test_recent_activity_required_even_with_a_live_ticket(self):
        self.api.sessions = {p: self.session(p, seen=seen, expires=expiry) for p, seen, expiry in
            [('recent', 99, 200), ('idle-login', 70, 200), ('expired', 99, 100), ('future', 101, 200)]}
        self.ticket('party', 'ranked', list(self.api.sessions))
        self.assertEqual(population(self.directory)['online'], {'users': 1, 'guests': 0})
        self.api.core.tickets['party'].expires_at = 100
        self.assertEqual(population(self.directory)['idle']['users'], 1)

    def test_proposal_and_room_handoff_count_once_and_retire_disconnected_seats(self):
        self.api.sessions['queue'] = self.session('owner', seats=[('owner', False), ('couch', False), ('guest', True)])
        self.ticket('party', 'ranked', ['owner', 'couch', 'guest'])
        self.api.core.proposals['match'] = S(tickets=['party'], expires_at=150)
        counts = population(self.directory)
        self.assertEqual(counts['joining'], counts['online'])
        self.assertEqual(counts['queued']['users'], 0)
        # Match leases and challenged connections take over from login activity.
        self.api.sessions.clear()
        for phase, state in [('loading', 'joining'), ('running', 'playing'), ('migrating', 'playing'),
                             ('reviewing', 'postgame'), ('postgame', 'postgame')]:
            if phase == 'running':
                self.api.core.tickets.clear()
                self.api.core.proposals.clear()
            self.room(phase)
            counts = population(self.directory)
            self.assertEqual(counts[state], {'users': 2, 'guests': 1}, phase)
            self.assertEqual(sum(counts[key]['users'] for key in ('idle', 'queued', 'joining', 'playing', 'postgame')), 2)
        for options in [dict(closed=True), dict(deadline=100), dict(socket_closed=True), dict(challenge='nonce', sent=84)]:
            self.room(**options)
            self.assertEqual(population(self.directory)['online'], {'users': 0, 'guests': 0}, options)

    def test_a_new_search_takes_priority_over_an_old_postgame_connection(self):
        self.room('postgame')
        self.ticket('new-search', 'casual', ['owner', 'couch', 'guest'])
        counts = population(self.directory)
        self.assertEqual(counts['queued'], counts['online'])
        self.assertEqual(counts['postgame']['users'], 0)
        self.assertEqual(counts['playlists'][1]['queued']['users'], 2)

    def test_disabled_matchmaking_still_counts_menu_users(self):
        self.api.core = None
        self.api.entries = []
        self.directory.clans.sessions['social'] = self.session('owner')
        counts = population(self.directory)
        self.assertEqual(counts['online'], counts['idle'])
        self.assertEqual(counts['online']['users'], 1)
        self.assertEqual(counts['playlists'], [])


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

import asyncio
from dataclasses import replace
from fractions import Fraction
import random
import struct
from types import SimpleNamespace
import unittest

from multiplayer.master_server.gathering_sessions import Gathering, GatheringSessions, mapped_minimum, rank_window, RETAIL_RANK_LIMITS
from multiplayer.master_server.matchmaking import Coordinator, Game, Player, Playlist, SearchStage
from multiplayer.master_server.search_qos import Measurement
from multiplayer.master_server.search_lifecycle import GatheringConfiguration, I32_MAX, SearchProgress
from . import test_matchmaking_http as http_fixture
from . import test_search_qos as qos_fixture


def gathering_catalog():
    result = http_fixture.catalog()
    for row in result["playlists"]:
        row["candidate_selection"] = "retail-candidates-native-relay-v1"
        row["rank_configuration"] = "retail-launch-2004"
        row["gathering"] = dict(retail_hopper=2)
    return result


class GatheringHttpTests(unittest.IsolatedAsyncioTestCase):
    catalog_fixture = staticmethod(gathering_catalog)
    asyncSetUp = http_fixture.MatchmakingHttpTests.asyncSetUp
    enqueue = http_fixture.MatchmakingHttpTests.enqueue
    session = qos_fixture.SearchQosTests.session
    connect = qos_fixture.SearchQosTests.connect
    receive = qos_fixture.SearchQosTests.receive
    pair = qos_fixture.SearchQosTests.pair

    async def test_full_match_waits_for_real_preflight_and_preserves_private_inputs(self):
        self.api.gatherings.wall_clock = lambda: 1234
        qos, headers, ids, sockets = await self.pair()
        self.assertFalse(self.api.core.proposals)
        response = await self.client.get("/v1/matchmaking/ticket",headers=headers[1])
        self.assertEqual(response.status,200)
        gathering = next(iter(self.api.gatherings.sessions.values()))
        self.assertEqual(len(gathering.tickets), 1)
        self.assertEqual(gathering.born, 1234)
        # Legacy v8 parsers require exactly four fields. Progress is opt-in
        # for the updated native UI, without splitting the gameplay pool.
        legacy = await (await self.client.get("/v1/matchmaking/ticket",headers=headers[0])).json()
        self.assertEqual(set(legacy),{"state","playlist_id","queued_players","match"})
        progress = await (await self.client.get("/v1/matchmaking/ticket?progress=1",headers=headers[0])).json()
        self.assertEqual(progress["progress"],{"players":1,"minimum_players":2,
                                              "maximum_players":2,"wait_ms":None,
                                              "roster":[{"name":"Private chosen name","self":True}]})
        for private in ("host", "language", "raw_rank", "radius", "private-subject", "ticket_id", "token"):
            self.assertNotIn(private,str(progress))
        self.api.gatherings.wall_clock = lambda: 1241
        response = await self.client.get("/v1/matchmaking/ticket", headers=headers[1])
        self.assertEqual((await response.json())["state"], "searching")
        query = next(iter(self.api.gatherings.queries.values()))
        self.assertEqual(struct.unpack_from("<i", query.selector.bank, 0x200)[0], 7)
        self.assertFalse(query.selector.bank[0x74])
        for sample in range(3):
            first = await self.receive(sockets[0])
            self.now += .01
            await sockets[0].send_json({"echo": first})
            second = await self.receive(sockets[1])
            self.now += .01
            await sockets[1].send_json({"echo": second})
            if sample < 2:
                self.assertFalse(self.api.core.proposals)
        async with asyncio.timeout(1):
            while qos.measurement(*ids) is None:
                await asyncio.sleep(.001)
        response = await self.client.get("/v1/matchmaking/ticket?progress=1", headers=headers[0])
        body = await response.json()
        self.assertEqual(body["state"], "checking_connections")
        self.assertEqual(len(body["match"]["players"]), 2)
        self.assertIsNone(body["progress"])
        self.assertFalse(self.api.gatherings.sessions)
        self.assertEqual(len(self.api.core.proposals), 1)
        for private in ("language", "raw_rank", "quality", "milliseconds", "private-subject", "ticket_id", "token"):
            self.assertNotIn(private, str(body))

    async def test_old_clients_cannot_enter_gathering_profile(self):
        header = await self.session(compatibility="halo2-native-queue-v7")
        rows = (await (await self.client.get("/v1/matchmaking/playlists",headers=header)).json())["playlists"]
        self.assertFalse(any(row["eligible"] for row in rows))
        self.assertEqual((await self.enqueue(header)).status, 409)
        self.assertFalse(self.api.core.tickets)

    def test_catalog_requires_explicit_valid_gathering_dimensions(self):
        import json
        from multiplayer.master_server.matchmaking_http import load_catalog
        from multiplayer.master_server.protocol import Problem
        for change in (lambda row: row.pop('gathering'),
                       lambda row: row.update(gathering={'retail_hopper':4}),
                       lambda row: row.update(gathering={'retail_hopper':7}),
                       lambda row: row.update(gathering={'countdown_ms':[0], 'timeout_ms':150000,
                                                        'provenance':'Invalid native fixture'})):
            value = gathering_catalog()
            change(value['playlists'][0])
            self.path.write_text(json.dumps(value))
            with self.assertRaises(Problem):
                load_catalog(self.path)


class GatheringInputsTests(unittest.TestCase):
    def setUp(self):
        self.now, self.wall = 10., 1000
        self.policy = Playlist("lab", "1", True, 0, 2, 2, 1,
            (Game("slayer", "lockout", "Slayer", 1),),
            (SearchStage(0, Fraction(1), 50),), "Native allocation fixture",
            custom_rank_radius=6, candidate_selection="retail-candidates-native-relay-v1",
            gathering=GatheringConfiguration((0,)*16,150000,"Native validation timing"))
        self.core = Coordinator((self.policy,), clock=lambda: self.now, rng=random.Random(1))
        self.measured, self.requests = {}, []
        self.qos = SimpleNamespace(ready=lambda _: True,
            measurement=lambda a,b: self.measured.get(frozenset((a,b))),
            request=lambda a,b: self.requests.append((a,b)), pending=lambda a,b:False)
        self.api = SimpleNamespace(core=self.core, search_qos=self.qos)
        self.gatherings = GatheringSessions(self.api, wall_clock=lambda: self.wall)

    def player(self, identity, raw=1):
        return Player(identity, "Chosen name", min(raw,49)+1, frozenset(("lockout",)), raw)

    def ticket(self, identity, raw=1, **kwargs):
        # Pin arrival order independently of random UUIDs at equal fake ticks.
        self.now += .001
        return self.core.enqueue(identity, "lab", "halo2-native-queue-v8", (self.player(identity,raw),), **kwargs)

    def gathering(self, ticket, *, born=None):
        import uuid
        g = Gathering(str(uuid.uuid4()), (ticket.id,), self.wall if born is None else born,
                      self.gatherings.leader(ticket), ticket.language)
        self.gatherings.sessions[g.id] = g
        return g

    def test_public_progress_reads_current_countdown_without_mutating_selection(self):
        self.policy = replace(self.policy,minimum_players=2,maximum_players=4,
            gathering=GatheringConfiguration((0,20000,10000)+(0,)*13,150000,"Native progress fixture"))
        self.core.playlists["lab"] = self.policy
        a,b = self.ticket("a"),self.ticket("b")
        self.now=20.0
        state=self.gatherings.state(a)
        g=self.gatherings.found(self.policy,a,state,self.now)
        self.assertEqual(self.gatherings.progress(a,"a"),{"players":1,"minimum_players":2,
            "maximum_players":4,"wait_ms":None,
            "roster":[{"name":"Chosen name","self":True}]})
        g.tickets+=(b.id,)
        g.clock.observe(self.gatherings.tick(self.now),2)
        self.assertEqual(self.gatherings.progress(b,"b")["wait_ms"],20000)
        before=(self.core.rng.getstate(),vars(g.clock).copy(),self.gatherings.seed,
                vars(state).copy(),len(self.requests))
        self.now+=5
        for _ in range(3):
            self.assertEqual(self.gatherings.progress(a,"a")["wait_ms"],15000)
        after=(self.core.rng.getstate(),vars(g.clock).copy(),self.gatherings.seed,
               vars(state).copy(),len(self.requests))
        self.assertEqual(before,after)
        c=self.ticket("c");g.tickets+=(c.id,);self.now=26.0
        g.clock.observe(self.gatherings.tick(self.now),3)
        self.assertEqual(self.gatherings.progress(c,"c")["wait_ms"],10000)
        g.tickets=(a.id,b.id) # New lower count selects its delay, without resetting peak.
        self.now+=5
        self.assertEqual(self.gatherings.progress(a,"a")["wait_ms"],15000)
        self.assertIsNone(self.gatherings.progress(c,"c"))
        self.now+=15
        self.assertIsNone(self.gatherings.progress(a,"a")["wait_ms"])
        self.gatherings.withdraw(g)
        self.assertIsNone(self.gatherings.progress(a,"a"))

    def test_advertisement_uses_birth_raw_skill_title_language_and_actual_route_median(self):
        host = self.ticket("host", 100, language=6)
        g = self.gathering(host)
        self.now += 45  # Ticket age differs from the gathering birth wall-clock.
        searcher = self.ticket("search", 100)
        self.wall += 7
        self.measured[frozenset(("host","search"))] = Measurement(499, 9999)
        row = self.gatherings.bank(self.policy, searcher, [g])
        for offset, value in ((0x90,499),(0xB8,6),(0xF8,0),(0xFC,-1),(0x10C,1),
                              (0x11C,7),(0x120,100),(0x124,100),(0x12C,100)):
            self.assertEqual(struct.unpack_from("<i",row,offset)[0],value)
        self.assertEqual(row[0x74],1)
        self.assertEqual(row[0x24:0x48],g.id.encode())
        self.assertEqual(self.gatherings.search(self.policy,searcher)[0].average_rank,100)

    def test_exact_selector_ranks_real_gatherings_instead_of_random_completed_subsets(self):
        a = self.ticket("host-a", language=6)
        b = self.ticket("host-b", language=0)
        ga, gb = self.gathering(a), self.gathering(b)
        searcher = self.ticket("search")
        self.measured[frozenset(("host-a","search"))] = Measurement(200,9999)
        self.measured[frozenset(("host-b","search"))] = Measurement(10,9999)
        proposal = self.gatherings.advance("lab")
        self.assertIsNotNone(proposal)
        self.assertEqual(set(proposal.tickets),{b.id,searcher.id})
        self.assertIn(ga.id,self.gatherings.sessions)
        self.assertNotIn(gb.id,self.gatherings.sessions)

    def test_partial_gathering_keeps_parties_and_birth_until_full_then_normal_handoff(self):
        self.policy = replace(self.policy, team_size=2, minimum_players=4, maximum_players=4, maximum_party=2)
        self.core.playlists["lab"] = self.policy
        a, b = self.ticket("a"), self.ticket("b")
        self.measured[frozenset(("a","b"))] = Measurement(10,9999)
        self.assertIsNone(self.gatherings.advance("lab"))
        g = next(iter(self.gatherings.sessions.values()))
        self.assertEqual(set(g.tickets),{a.id,b.id})
        original_id, original_birth = g.id,g.born
        self.now += 30
        self.wall += 20
        c = self.core.enqueue("party-c","lab","halo2-native-queue-v8",
                              (self.player("c1"),self.player("c2")),leader="c2",language=7)
        self.measured[frozenset(("a","c2"))] = Measurement(10,9999)
        proposal = self.gatherings.advance("lab")
        self.assertIsNotNone(proposal)
        self.assertEqual(set(proposal.tickets),{a.id,b.id,c.id})
        self.assertEqual(c.leader,"c2")
        self.assertFalse(self.requests)  # Current trusted measurement needs no redundant probe.
        self.assertEqual((g.id,g.born),(original_id,original_birth))
        self.assertIn(("c1","c2"),proposal.teams)

    def test_withdrawal_destroys_gathering_survivors_receive_a_new_session_birth(self):
        self.policy = replace(self.policy, team_size=2, minimum_players=4, maximum_players=4, maximum_party=2)
        self.core.playlists["lab"] = self.policy
        a,b = self.ticket("a"),self.ticket("b")
        self.measured[frozenset(("a","b"))] = Measurement(10,9999)
        self.gatherings.advance("lab")
        old = next(iter(self.gatherings.sessions.values()))
        self.core.cancel(a.id)
        self.wall += 50
        self.gatherings.advance("lab")
        new = next(iter(self.gatherings.sessions.values()))
        self.assertNotEqual(new.id,old.id)
        self.assertEqual((new.tickets,new.born),((b.id,),1050))

    def test_private_high_ranks_are_not_collapsed_to_the_shared_artwork_level(self):
        self.ticket("a",80)
        self.ticket("b",100)
        self.assertIsNone(self.gatherings.advance("lab"))
        self.assertEqual(len(self.gatherings.sessions),2)

    def test_advertisement_floor_is_derived_from_raw_extrema_not_adjusted_search_average(self):
        a,b = self.ticket("a",1),self.ticket("b",8)
        g = self.gathering(a)
        g.tickets += (b.id,)
        searcher = self.ticket("search",4)
        self.measured[frozenset(("a","search"))] = Measurement(10,9999)
        bank = self.gatherings.bank(self.policy,searcher,[g])
        self.assertEqual(struct.unpack_from("<i",bank,0x120)[0],4)
        self.assertEqual(struct.unpack_from("<i",bank,0x124)[0],8)
        self.assertEqual(struct.unpack_from("<i",bank,0x64)[0],3)
        self.assertEqual(struct.unpack_from("<i",bank,0x12C)[0],3)
        self.assertEqual(mapped_minimum(80,100),90)
        self.assertEqual(mapped_minimum(126,127),126)

    def test_original_rank_table_inverse_and_mixed_party_search_window_controls(self):
        import json
        from pathlib import Path
        fixture = json.loads((Path(__file__).parent/'fixtures/retail-gathering-rank-windows.json').read_text())
        self.assertEqual(tuple(fixture['table']),RETAIL_RANK_LIMITS)
        for maximum, expected in enumerate(fixture['inverse']):
            self.assertEqual(mapped_minimum(0,maximum),expected)
        for case in fixture['ranges']:
            self.assertEqual(mapped_minimum(case['raw_minimum'],case['raw_maximum']),case['mapped_minimum'])
            for bound in case['bounds']:
                self.assertEqual(rank_window(case['raw_minimum'],case['raw_maximum'],bound['radius']),
                    (case['maximum_radius'],bound['minimum'],bound['maximum']))

    def test_rank_window_changes_on_query_completion_and_ignores_elapsed_stage(self):
        ticket = self.ticket("search",20)
        progress = SearchProgress(7,True)
        self.assertEqual(self.gatherings.search(self.policy,ticket,progress)[0].minimum_rank,20)
        self.now += 3600
        initial = self.gatherings.search(self.policy,ticket,progress)[0]
        self.assertEqual((initial.minimum_rank,initial.maximum_rank),(20,20))
        progress.complete(0)
        next_search = self.gatherings.search(self.policy,ticket,progress)[0]
        self.assertEqual((next_search.minimum_rank,next_search.maximum_rank),(19,21))

    def test_live_probe_keeps_query_clock_and_radius_after_five_seconds(self):
        host,searcher = self.ticket("host"),self.ticket("search")
        self.gathering(host)
        self.qos.request = lambda a,b: True
        self.qos.pending = lambda a,b: True
        self.assertIsNone(self.gatherings.advance("lab"))
        query = self.gatherings.queries[searcher.id]
        state = self.gatherings.states[searcher.id]
        started = query.started
        self.now += 6
        self.wall += 6
        self.assertIsNone(self.gatherings.advance("lab"))
        self.assertIs(self.gatherings.queries[searcher.id],query)
        self.assertEqual(query.started,started)
        self.assertEqual((state.progress.radius,state.progress.total),(0,0))
        self.measured[frozenset(("host","search"))] = Measurement(10,9999)
        self.assertIsNotNone(self.gatherings.advance("lab"))

    def test_casual_unbounded_retry_is_driven_by_completed_queries(self):
        self.policy = replace(self.policy,ranked=False,stages=(SearchStage(0,Fraction(1,10),50),
            SearchStage(30,Fraction(1),50,True)))
        self.core.playlists['lab'] = self.policy
        host,searcher = self.ticket("host",100),self.ticket("search",1)
        self.gathering(host)
        self.measured[frozenset(("host","search"))] = Measurement(10,9999)
        before = self.now
        proposal = self.gatherings.advance("lab")
        self.assertIsNotNone(proposal)
        self.assertEqual(set(proposal.tickets),{host.id,searcher.id})
        self.assertEqual(self.now,before)
        self.assertEqual(self.gatherings.states[searcher.id].progress.radius,-1)

    def test_game_is_selected_at_gathering_creation_and_preserved_at_launch(self):
        self.policy = replace(self.policy,games=(Game('a','lockout','Slayer',1),Game('b','lockout','Team Slayer',1)))
        self.core.playlists['lab'] = self.policy
        host = self.ticket("host")
        self.gatherings.advance('lab')
        g = next(iter(self.gatherings.sessions.values()))
        self.assertIn(g.game,self.policy.games)
        game = g.game
        self.ticket("search")
        self.measured[frozenset(("host","search"))] = Measurement(10,9999)
        self.assertEqual(self.gatherings.advance('lab').game,game)

    def test_partial_countdown_waits_since_new_peak_before_launch(self):
        configuration = GatheringConfiguration((I32_MAX,5000,I32_MAX,I32_MAX)+(I32_MAX,)*12,
                                               150000,'Native countdown boundary fixture')
        self.policy = replace(self.policy,maximum_players=4,gathering=configuration)
        self.core.playlists['lab'] = self.policy
        self.ticket('host')
        self.ticket('search')
        self.measured[frozenset(('host','search'))] = Measurement(10,9999)
        self.assertIsNone(self.gatherings.advance('lab'))
        g = next(iter(self.gatherings.sessions.values()))
        self.assertEqual(g.clock.peak,2)
        self.now = (g.clock.growth+4999)/1000
        self.assertIsNone(self.gatherings.advance('lab'))
        self.now = (g.clock.growth+5000)/1000
        self.assertIsNotNone(self.gatherings.advance('lab'))

    def test_gather_timeout_withdraws_advertisement_and_renews_birth(self):
        self.core.lease = 300  # This clock fixture has no intervening HTTP heartbeats.
        self.ticket('host')
        self.gatherings.advance('lab')
        old = next(iter(self.gatherings.sessions.values()))
        state = self.gatherings.states[old.tickets[0]]
        self.now = (old.clock.growth+self.policy.gathering.timeout_ms+state.gather_offset+1)/1000
        self.wall += 150
        self.gatherings.advance('lab')
        new = next(iter(self.gatherings.sessions.values()))
        self.assertNotEqual(old.id,new.id)
        self.assertEqual(new.born,self.wall)
        self.assertEqual(state.mode,'gather')

    def test_failed_probe_is_marked_seen_and_not_retried_at_each_radius(self):
        host,searcher = self.ticket('host'),self.ticket('search')
        original = self.gathering(host)
        self.qos.request = lambda a,b: self.requests.append((a,b)) or True
        self.qos.pending = lambda a,b: False
        self.gatherings.advance('lab')
        self.assertEqual(len(self.requests),1)
        self.gatherings.advance('lab')
        self.assertEqual(len(self.requests),1)
        self.assertIn(original.id,self.gatherings.states[searcher.id].seen)
        self.assertEqual(len(self.gatherings.sessions),2)

    def test_gameplay_host_retains_gathering_owner_and_requires_real_connectivity(self):
        a,b = self.ticket('host'),self.ticket('search')
        self.measured[frozenset(('host','search'))] = Measurement(10,9999)
        proposal = self.gatherings.advance('lab')
        self.assertEqual(proposal.gathering_host,'host')
        self.assertIsNone(self.core.refresh_connections(proposal.id).host)
        self.core.connection('host','search',60)
        self.core.connection('search','host',100)
        self.assertEqual(self.core.refresh_connections(proposal.id).host,'host')
        self.core.connection('host','search',20)
        self.assertIsNone(self.core.refresh_connections(proposal.id).host)

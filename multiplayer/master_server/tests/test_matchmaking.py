"""Behavioral scenarios for reconstructed policy, not original-service parity."""
from dataclasses import replace
from fractions import Fraction
import hashlib
import json
from pathlib import Path
import random
import unittest

from multiplayer.master_server.matchmaking import (
    Coordinator, Game, Player, Playlist, SearchStage, PARTY_RULES_2007,
    party_rank, rank_radius_2007, effective_ranks,
)
from multiplayer.master_server.protocol import Problem


MAPS = frozenset(("lockout", "ascension"))


def policy(name="lab-ranked", *, team_size=4, ranked=True, radius=None):
    return Playlist(name, "lab-v1-not-retail", ranked, team_size,
                    team_size * 2 if team_size else 2, team_size * 2 if team_size else 8,
                    4 if team_size else 1,
                    (Game("slayer", "lockout", "Slayer", 3), Game("king", "ascension", "King", 1)),
                    (SearchStage(0, Fraction(1, 3), 80), SearchStage(30, Fraction(1), 60)),
                    "LAB ONLY: published mechanisms with unverified timing, game weights and scoring",
                    PARTY_RULES_2007["team_skirmish"] if team_size else None,
                    use_2007_rank_limits=radius is None, high_rank_radius=15 if radius is None else None,
                    custom_rank_radius=radius)


class MatchmakingTests(unittest.TestCase):
    def setUp(self):
        self.now = 100.
        self.serial = 0
        self.policy = policy()
        self.coordinator = Coordinator((self.policy,), clock=lambda: self.now, rng=random.Random(14))

    def party(self, ranks, *, coordinator=None, playlist=None, compatibility="native-v7-test", maps=MAPS):
        self.serial += 1
        players = tuple(Player(f"player-{self.serial}-{i}", "Chosen Name", rank, maps) for i, rank in enumerate(ranks))
        coordinator = coordinator or self.coordinator
        return coordinator.enqueue(f"party-{self.serial}", playlist or next(iter(coordinator.playlists)),
                                   compatibility, players)

    def qos(self, coordinator=None, *, score=90, hosts=None):
        coordinator = coordinator or self.coordinator
        players = [p.id for t in coordinator.tickets.values() for p in t.players]
        for host in players if hosts is None else hosts:
            for player in players:
                coordinator.connection(host, player, score)

    def test_published_bounds_and_mixed_party_example(self):
        expected = {1:10, 2:9, 3:8, 4:7, 5:6, 6:5, 7:6, 16:6, 17:7, 26:7,
                    27:8, 29:8, 30:9, 31:10, 32:11, 33:12, 34:13, 35:14}
        for rank, radius in expected.items():
            self.assertEqual(radius, rank_radius_2007(rank))
        with self.assertRaises(Problem):
            rank_radius_2007(36)
        self.assertEqual(15, rank_radius_2007(50, high_rank_radius=15))
        players = self.party((12, 1)).players
        self.assertEqual(6, party_rank(self.policy, players))
        adjusted = replace(self.policy, mixed_party_floor=True)
        self.assertEqual((12, 6), effective_ranks(adjusted, players))
        self.assertEqual(6, party_rank(adjusted, players))

    def test_raw_skill_is_preserved_above_visible_cap_and_units_do_not_mix(self):
        policy_value = replace(policy("raw", team_size=0, radius=6),
                               minimum_players=2, maximum_players=2)
        coordinator = Coordinator((policy_value,), clock=lambda: self.now, rng=random.Random(1))
        for i, raw in enumerate((60, 100)):
            coordinator.enqueue(f"party-{i}", policy_value.id, "native-test",
                                (Player(f"raw-{i}", "Chosen", 50, MAPS, raw),))
        self.qos(coordinator)
        self.now += 30
        self.qos(coordinator)
        self.assertIsNone(coordinator.advance(policy_value.id))
        self.assertEqual([60, 100], [party_rank(policy_value, t.players) for t in coordinator.tickets.values()])
        mixed = (Player("raw", "Chosen", 11, MAPS, 10), Player("legacy", "Chosen", 11, MAPS))
        with self.assertRaises(Problem):
            Coordinator((replace(policy_value, maximum_party=2),)).enqueue(
                "mixed", policy_value.id, "native-test", mixed)
        units = Coordinator((replace(policy_value, custom_rank_radius=49),), clock=lambda: self.now)
        units.enqueue("raw", policy_value.id, "native-test", (Player("raw", "Chosen", 50, MAPS, 60),))
        units.enqueue("legacy", policy_value.id, "native-test", (Player("legacy", "Chosen", 50, MAPS),))
        self.qos(units)
        self.assertIsNone(units.advance(policy_value.id))
        for raw, level in ((-1, 1), (128, 50), (60, 49), (0, 2)):
            with self.subTest(raw=raw, level=level), self.assertRaises(Problem):
                Player("invalid", "Chosen", level, MAPS, raw)

    def test_original_integer_unadjusted_party_average(self):
        fixture = json.loads((Path(__file__).parent / "fixtures/retail-party-search-ranks.json").read_bytes())
        self.assertEqual(fixture["xbe_sha256"],
                         "03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d")
        self.assertEqual(fixture["oracle_sha256"],
                         "30c4b0f21fa0c72a445e780ec6b7d9688d994689f5ac9d1f374d0563ec148506")
        runner = Path(__file__).resolve().parents[3] / "tools/oracle_party_search_ranks.py"
        if runner.is_file():
            self.assertEqual(fixture["oracle_sha256"], hashlib.sha256(runner.read_bytes()).hexdigest())
        controls = fixture["controls"]
        available = [r for r in controls if r["name"].startswith("integer_mean_")]
        self.assertEqual(len(available), 8)
        for row in available:
            players = tuple(Player(f"raw-{i}", "Chosen", min(raw,49)+1, MAPS, raw)
                            for i, raw in enumerate(row["inputs"]["ranks"]))
            with self.subTest(ranks=row["inputs"]["ranks"]):
                self.assertEqual(party_rank(self.policy, players), row["actual"][-1])
                self.assertIs(type(party_rank(self.policy, players)), int)
        recovered = next(r["actual"] for r in controls if r["name"] == "recover_raw_property_and_ignore_adjusted")
        players = tuple(Player(f"recovered-{i}", "Chosen", raw+1, MAPS, raw)
                        for i, raw in enumerate(recovered[-1]))
        self.assertEqual(party_rank(replace(self.policy, mixed_party_floor=True), players), recovered[2])

    def test_search_widens_but_never_crosses_hard_cap(self):
        for rank in (10, 10, 10, 10, 15, 15, 15, 15):
            self.party((rank,))
        self.qos()
        self.assertIsNone(self.coordinator.advance(self.policy.id))
        self.now += 30
        self.qos()
        proposal = self.coordinator.advance(self.policy.id)
        self.assertIsNotNone(proposal)
        self.assertEqual([4, 4], [len(team) for team in proposal.teams])
        ranks = {p.id: p.rank for t in self.coordinator.tickets.values() for p in t.players}
        self.assertEqual(0, abs(sum(ranks[p] for p in proposal.teams[0]) - sum(ranks[p] for p in proposal.teams[1])))
        self.assertTrue(all(self.coordinator.status(t)["state"] == "awaiting_transport" for t in proposal.tickets))
        other = Coordinator((self.policy,), clock=lambda: self.now, rng=random.Random(4))
        for rank in (10, 10, 10, 10, 17, 17, 17, 17):
            self.party((rank,), coordinator=other)
        self.now += 60
        self.qos(other)
        self.assertIsNone(other.advance(self.policy.id))

    def test_old_search_cannot_widen_new_party(self):
        for _ in range(4):
            self.party((10,))
        self.now += 30
        for _ in range(4):
            self.party((15,))
        self.qos()
        self.assertIsNone(self.coordinator.advance(self.policy.id))
        self.now += 30
        self.qos()
        self.assertIsNotNone(self.coordinator.advance(self.policy.id))

    def test_casual_final_pass_removes_skill_filter_but_retains_qos_and_new_search_bounds(self):
        casual = replace(policy("casual", team_size=0, ranked=False, radius=6),
                         minimum_players=2, maximum_players=2,
                         stages=(SearchStage(0, Fraction(1), 50),
                                 SearchStage(30, Fraction(1), 50, unbounded_skill=True)))
        coordinator = Coordinator((casual,), clock=lambda: self.now, rng=random.Random(22))
        a = self.party((1,), coordinator=coordinator)
        self.now += 30
        b = self.party((50,), coordinator=coordinator)
        self.qos(coordinator)
        self.assertIsNone(coordinator.advance(casual.id))  # New search still has its own bounds.
        self.now += 30
        self.qos(coordinator, score=49)
        self.assertIsNone(coordinator.advance(casual.id))  # Skill expansion never bypasses QoS.
        self.qos(coordinator, score=50)
        proposal = coordinator.advance(casual.id)
        self.assertEqual({a.id, b.id}, set(proposal.tickets))
        for stages, ranked in ((casual.stages, True),
                               ((casual.stages[1],), False),
                               ((casual.stages[0], replace(casual.stages[1], unbounded_skill=1)), False),
                               ((casual.stages[0], casual.stages[1], SearchStage(60, Fraction(1), 50)), False)):
            with self.subTest(stages=stages, ranked=ranked), self.assertRaises(Problem):
                replace(casual, stages=stages, ranked=ranked)

    def test_intact_significant_parties_and_small_parties(self):
        a, b = self.party((10, 10, 10)), self.party((10, 10, 10))
        self.party((10,)); self.party((10,)); self.qos()
        proposal = self.coordinator.advance(self.policy.id)
        self.assertIsNotNone(proposal)
        for ticket in (a, b):
            members = {p.id for p in ticket.players}
            self.assertEqual(1, sum(members <= set(team) for team in proposal.teams))
        unequal = Coordinator((self.policy,), clock=lambda: self.now)
        for ranks in ((10,) * 4, (10,) * 3, (10,)):
            self.party(ranks, coordinator=unequal)
        self.qos(unequal)
        self.assertIsNone(unequal.advance(self.policy.id))
        small = Coordinator((self.policy,), clock=lambda: self.now)
        for ranks in ((10, 10), (10, 10), (10,), (10,), (10,), (10,)):
            self.party(ranks, coordinator=small)
        self.qos(small)
        self.assertIsNotNone(small.advance(self.policy.id))

    def test_casual_uses_private_skill_and_separate_playlist_pool(self):
        casual = policy("lab-casual", team_size=0, ranked=False, radius=6)
        coordinator = Coordinator((self.policy, casual), clock=lambda: self.now, rng=random.Random(22))
        a = self.party((10,), coordinator=coordinator, playlist=casual.id)
        self.party((40,), coordinator=coordinator, playlist=casual.id)
        self.party((10,), coordinator=coordinator, playlist=self.policy.id)
        self.now += 30
        self.qos(coordinator)
        self.assertIsNone(coordinator.advance(casual.id))
        self.assertEqual({"state": "searching"}, coordinator.status(a.id))
        b = self.party((11,), coordinator=coordinator, playlist=casual.id)
        self.qos(coordinator)
        proposal = coordinator.advance(casual.id)
        self.assertEqual({a.id, b.id}, set(proposal.tickets))
        self.assertEqual(2, len(proposal.teams))

    def test_dated_btb_large_party_difference_and_multiple_party_inference(self):
        btb = replace(self.policy, id="lab-btb", team_size=8, minimum_players=16, maximum_players=16,
                      maximum_party=5, party_balance=PARTY_RULES_2007["big_team_battle"])
        coordinator = Coordinator((btb,), clock=lambda: self.now, rng=random.Random(4))
        a = self.party((10,) * 5, coordinator=coordinator)
        b = self.party((10,) * 3, coordinator=coordinator)
        for _ in range(8):
            self.party((10,), coordinator=coordinator)
        self.qos(coordinator)
        proposal = coordinator.advance(btb.id)
        self.assertIsNotNone(proposal)
        a_team = next(i for i, team in enumerate(proposal.teams) if a.players[0].id in team)
        b_team = next(i for i, team in enumerate(proposal.teams) if b.players[0].id in team)
        self.assertNotEqual(a_team, b_team)
        multiple = Coordinator((btb,), clock=lambda: self.now, rng=random.Random(3))
        for size in (3, 3, 2, 5, 1, 1, 1):
            self.party((10,) * size, coordinator=multiple)
        self.qos(multiple)
        self.assertIsNone(multiple.advance(btb.id))

    def test_readiness_requires_every_member_and_cancel_releases_others(self):
        for _ in range(8):
            self.party((10,))
        self.qos()
        proposal = self.coordinator.advance(self.policy.id)
        self.assertIsNone(self.coordinator.advance(self.policy.id))
        with self.assertRaises(Problem) as caught:
            self.coordinator.acknowledge(proposal.id, "outsider")
        self.assertEqual(403, caught.exception.status)
        members = [p for team in proposal.teams for p in team]
        for member in members[:-1]:
            self.assertIsNone(self.coordinator.acknowledge(proposal.id, member))
        self.assertEqual(proposal, self.coordinator.acknowledge(proposal.id, members[-1]))
        self.assertFalse(self.coordinator.tickets)
        self.assertFalse(self.coordinator.connections)
        for _ in range(8):
            self.party((10,))
        self.qos()
        proposal = self.coordinator.advance(self.policy.id)
        cancelled, survivor = proposal.tickets[:2]
        age = self.coordinator.tickets[survivor].joined_at
        self.coordinator.cancel(cancelled)
        self.assertEqual({"state": "searching"}, self.coordinator.status(survivor))
        self.assertEqual(age, self.coordinator.tickets[survivor].joined_at)
        with self.assertRaises(Problem):
            self.coordinator.acknowledge(proposal.id, members[0])

    def test_expiry_heartbeat_and_clock_rollback(self):
        for _ in range(8):
            self.party((10,))
        self.qos()
        proposal = self.coordinator.advance(self.policy.id)
        self.now += 30
        self.coordinator.cleanup()
        self.assertFalse(self.coordinator.proposals)
        self.assertFalse(self.coordinator.connections)
        self.assertEqual({"state": "searching"}, self.coordinator.status(proposal.tickets[0]))
        self.now += 59
        self.coordinator.heartbeat(proposal.tickets[0])
        self.now += 1
        self.coordinator.cleanup()
        self.assertEqual(1, len(self.coordinator.tickets))
        self.now -= 20
        self.coordinator.cleanup()
        self.assertEqual(1, len(self.coordinator.tickets))
        self.now += 120
        self.coordinator.cleanup()
        self.assertFalse(self.coordinator.tickets)
        self.now -= 200
        self.coordinator.cleanup()
        self.assertFalse(self.coordinator.tickets)

    def test_measured_connection_compatibility_and_map_eligibility(self):
        with self.assertRaises(Problem):
            self.party((10,), maps=frozenset(("lockout",)))
        with self.assertRaises(Problem):
            self.party((10,) * 5)
        for _ in range(7):
            self.party((10,))
        incompatible = self.party((10,), compatibility="another-build")
        self.qos()
        self.assertIsNone(self.coordinator.advance(self.policy.id))
        self.coordinator.cancel(incompatible.id)
        self.party((10,))
        self.assertIsNone(self.coordinator.advance(self.policy.id))
        self.qos(score=70)
        self.assertIsNone(self.coordinator.advance(self.policy.id))
        self.now += 30
        self.qos(score=70)
        self.assertIsNotNone(self.coordinator.advance(self.policy.id))

    def test_duplicate_idempotence_and_capacity(self):
        ticket = self.party((10,))
        self.assertEqual(ticket, self.coordinator.enqueue(ticket.party_id, ticket.playlist,
                                                         ticket.compatibility, ticket.players))
        with self.assertRaises(Problem):
            self.coordinator.enqueue("another-party", ticket.playlist, ticket.compatibility, ticket.players)
        with self.assertRaises(Problem):
            self.coordinator.enqueue(ticket.party_id, ticket.playlist, "changed-build", ticket.players)
        small = Coordinator((self.policy,), clock=lambda: self.now, capacity=1)
        self.party((10,), coordinator=small)
        with self.assertRaises(Problem) as caught:
            self.party((10,), coordinator=small)
        self.assertEqual(503, caught.exception.status)

    def test_measurements_expiring_during_search_are_not_used(self):
        for _ in range(8):
            self.party((10,))
        self.qos()
        moments = iter((self.now,))
        # Cleanup starts while QoS is live; search finishes after its deadline.
        self.coordinator.clock = lambda: next(moments, self.now + 31)
        self.assertIsNone(self.coordinator.advance(self.policy.id))

    def test_unreachable_old_anchor_does_not_starve_later_parties(self):
        ffa = policy("lab-ffa", team_size=0, radius=6)
        coordinator = Coordinator((ffa,), clock=lambda: self.now, rng=random.Random(3), search_budget=8)
        isolated = self.party((10,), coordinator=coordinator)
        self.now += 1
        later = [self.party((10,), coordinator=coordinator) for _ in range(60)]
        # Only later players can reach this potential host; isolated cannot.
        host = later[-1].players[0].id
        for ticket in later:
            coordinator.connection(host, ticket.players[0].id, 90)
        proposals = []
        for _ in range(200):
            proposal = coordinator.advance(ffa.id)
            if proposal:
                proposals.append(proposal)
                break
        self.assertTrue(proposals)
        self.assertNotIn(isolated.id, proposals[0].tickets)
        self.assertEqual({"state": "searching"}, coordinator.status(isolated.id))

    def test_weighted_games_and_candidate_variation_without_outcome_feedback(self):
        ffa = policy("lab-ffa", team_size=0, radius=6)
        coordinator = Coordinator((ffa,), clock=lambda: self.now, rng=random.Random(83))
        counts, opponents = {}, set()
        for _ in range(400):
            tickets = [self.party((rank,), coordinator=coordinator) for rank in (10, 11, 12)]
            self.qos(coordinator)
            proposal = coordinator.advance(ffa.id)
            counts[proposal.game.id] = counts.get(proposal.game.id, 0) + 1
            ranks = {p.id: p.rank for t in tickets for p in t.players}
            opponents.add(tuple(sorted(ranks[p] for team in proposal.teams for p in team)))
            for key in list(coordinator.tickets):
                coordinator.cancel(key)
        self.assertTrue(260 < counts["slayer"] < 340, counts)
        self.assertTrue(60 < counts["king"] < 140, counts)
        self.assertGreater(len(opponents), 1)

    def test_policy_validation_rejects_silent_tuning_errors(self):
        for changes in ({"stages": ()}, {"stages": (SearchStage(1, Fraction(1), 80),)},
                        {"stages": (SearchStage(0, Fraction(1), 50), SearchStage(20, Fraction(1, 2), 60))},
                        {"games": ()}, {"custom_rank_radius": 5}, {"maximum_party": 5},
                        {"provenance": ""}, {"high_rank_radius": True}):
            with self.subTest(changes=changes), self.assertRaises(Problem):
                replace(self.policy, **changes)


if __name__ == "__main__":
    unittest.main()

"""Recovered type5 lineup rules, with declared native coordinator plumbing."""
from dataclasses import replace
from fractions import Fraction
import random
import unittest

from multiplayer.master_server.matchmaking import (
    CLAN_RULES_RETAIL, Coordinator, Game, Player, Playlist, SearchStage,
)
from multiplayer.master_server.protocol import Problem


def clan_policy(kind="minor-retail-launch-2004"):
    rule = CLAN_RULES_RETAIL[kind]
    return Playlist("clan", "test-1", True, rule.maximum, rule.minimum*2,
                    rule.maximum*2, rule.maximum,
                    (Game("slayer", "lockout", "Team Slayer", 1),),
                    (SearchStage(0, Fraction(1), 50),),
                    "Recovered launch party rules; custom radius/timing/game pool",
                    custom_rank_radius=6, clan_match=rule)


class ClanMatchmakingCoreTests(unittest.TestCase):
    def setUp(self):
        self.serial = 0

    def queue(self, core, size, clan, rank=10, personal=10):
        self.serial += 1
        players = tuple(Player(f"p{self.serial}-{i}", "Same Pilot", personal,
                               frozenset(("lockout",))) for i in range(size))
        return core.enqueue(f"party-{self.serial}", "clan", "fixture", players,
                            clan=clan, clan_rank=rank)

    def proposal(self, core):
        players = [p.id for t in core.tickets.values() for p in t.players]
        for a in players:
            for b in players:
                core.connection(a, b, 100)
        return core.advance("clan")

    def test_minor_equal_intact_lineups_and_distinct_clans(self):
        for size in (3, 4):
            core = Coordinator((clan_policy(),), rng=random.Random(1))
            a, b = self.queue(core, size, "a"), self.queue(core, size, "b")
            match = self.proposal(core)
            self.assertEqual({frozenset(p.id for p in t.players) for t in (a, b)},
                             {frozenset(team) for team in match.teams})
        core = Coordinator((clan_policy(),))
        self.queue(core, 3, "a")
        self.queue(core, 4, "b")
        self.assertIsNone(self.proposal(core))

        # The final visible level is shared by raw ranks49..127. Its artwork
        # must not collapse those distinct private clan search ratings.
        core = Coordinator((clan_policy(),))
        self.queue(core, 3, "a", rank=49)
        self.queue(core, 3, "b", rank=127)
        self.assertIsNone(self.proposal(core))
        with self.assertRaises(Problem):
            self.queue(core, 3, "a")
        for size in (1, 2, 5):
            with self.assertRaises(Problem):
                self.queue(Coordinator((clan_policy(),)), size, "c")
        with self.assertRaises(Problem):
            self.queue(Coordinator((clan_policy(),)), 3, "")

    def test_major_permits_unequal_whole_parties_with_original_limit(self):
        for sizes in ((6, 6), (6, 8), (7, 8), (8, 8)):
            core = Coordinator((clan_policy("major-retail-launch-2004"),))
            for size, clan in zip(sizes, ("a", "b")):
                self.queue(core, size, clan)
            match = self.proposal(core)
            self.assertEqual(sorted(sizes), sorted(map(len, match.teams)))
        for size in (5, 9):
            with self.assertRaises(Problem):
                self.queue(Coordinator((clan_policy("major-retail-launch-2004"),)), size, "a")

    def test_clan_rank_drives_search_instead_of_personal_player_average(self):
        core = Coordinator((clan_policy(),))
        self.queue(core, 3, "a", rank=10, personal=50)
        self.queue(core, 3, "b", rank=10, personal=1)
        self.assertIsNotNone(self.proposal(core))
        core = Coordinator((clan_policy(),))
        self.queue(core, 3, "a", rank=10)
        self.queue(core, 3, "b", rank=40)
        self.assertIsNone(self.proposal(core))

    def test_clan_policy_cannot_be_casual_or_use_unrelated_party_limits(self):
        for changes in (dict(ranked=False), dict(minimum_players=8), dict(maximum_party=3),
                        dict(team_size=3), dict(mixed_party_floor=True), dict(use_2007_rank_limits=True)):
            with self.subTest(changes=changes), self.assertRaises(Problem):
                replace(clan_policy(), **changes)

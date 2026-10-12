"""Deterministic matchmaking demonstration. All playlist tuning is LAB ONLY."""
from fractions import Fraction
import json
import random

from .matchmaking import Coordinator, Game, Player, Playlist, SearchStage, PARTY_RULES_2007


def main():
    clock = [100.]
    playlist = Playlist(
        "lab-team", "lab-v1-not-retail", True, 4, 8, 8, 4,
        (Game("slayer", "lockout", "Slayer", 3), Game("king", "ascension", "King", 1)),
        (SearchStage(0, Fraction(1, 3), 80), SearchStage(30, Fraction(1), 60)),
        "LAB ONLY: 2007 Skirmish party constraints, synthetic game pool, unverified search/QoS tuning",
        party_balance=PARTY_RULES_2007["team_skirmish"], use_2007_rank_limits=True,
        high_rank_radius=15)
    coordinator = Coordinator((playlist,), clock=lambda: clock[0], rng=random.Random(7))
    for party, ranks in enumerate(((10, 10, 10), (15, 15, 15), (10,), (15,))):
        players = tuple(Player(f"opaque-{party}-{i}", f"Pilot {party + 1}.{i + 1}", rank,
                               frozenset(("lockout", "ascension"))) for i, rank in enumerate(ranks))
        coordinator.enqueue(f"party-{party}", playlist.id, "native-v7-demo", players)
    players = [p for t in coordinator.tickets.values() for p in t.players]

    def measure():
        # Controlled test measurements, not a real network probe.
        for host in players:
            for player in players:
                coordinator.connection(host.id, player.id, 90)

    measure()
    assert coordinator.advance(playlist.id) is None
    clock[0] += 30
    measure()
    proposal = coordinator.advance(playlist.id)
    assert proposal is not None
    names = {p.id: p.screen_name for p in players}
    for player in players[:-1]:
        assert coordinator.acknowledge(proposal.id, player.id) is None
    assert coordinator.acknowledge(proposal.id, players[-1].id) == proposal
    print(json.dumps({
        "policy": playlist.provenance,
        "initial_search": "no compatible full match",
        "after_30_lab_seconds": "match proposed; parties preserved",
        "teams": [[names[p] for p in team] for team in proposal.teams],
        "host": names[proposal.host], "map": proposal.game.map, "variant": proposal.game.variant,
        "readiness": "all eight simulated members acknowledged",
        "scope": "proposal handoff only; no server allocation, native queue UI or gameplay admission",
    }, indent=2))


if __name__ == "__main__":
    main()

import json
from pathlib import Path
import unittest

from multiplayer.master_server.playlist_skill import (
    TABLES, MAX_XP, RatingPlayer, pair_delta, player_delta, raw_rank, display_level)


class OriginalRankTests(unittest.TestCase):
    """Expected outputs come from unchanged XBE execution, not this port."""
    def setUp(self):
        self.original = json.loads((Path(__file__).parent / "fixtures/retail-rank-original.json").read_text())
        self.controls = {r["name"]: r["actual"] for r in self.original["controls"]}

    def check(self, name, actual):
        self.assertEqual(actual, self.controls[name], name)

    def test_literal_original_tables_and_pair_controls(self):
        self.assertEqual(TABLES, self.original["tables"])
        for name, args in (
            ("equal skill win", (15,15,0,1)), ("equal skill loss", (15,15,1,0)),
            ("equal standings", (15,15,0,0)), ("both no standing", (15,15,-1,-1)),
            ("opponent no standing", (15,15,0,-1)), ("own no standing", (15,15,-1,0)),
            ("higher skill win", (16,15,0,1)), ("higher skill loss", (16,15,1,0)),
            ("negative fraction truncates toward zero", (8,7,1,0)),
            ("low loss pair scaling", (7,10,1,0)),
            ("positive high rank fraction truncates toward zero", (41,40,0,1))):
            with self.subTest(name=name): self.check(name, pair_delta(*args))

    def test_original_collection_controls(self):
        def calculate(skills, places=None, teams=None, xp=None, **kwargs):
            players = [RatingPlayer(s, places[i] if places else i, teams[i] if teams else i,
                                    xp[i] if xp else 5000) for i,s in enumerate(skills)]
            return [player_delta(players, i, **kwargs) for i in range(len(players))]
        self.check("four player base averaging truncates signed result", calculate([20,17,18,15]))
        self.check("four player low mitigation applied pairwise", calculate([10,7,8,5]))
        self.check("tied opponent included in average denominator", calculate([15,15,15],[0,0,1]))
        self.check("same team ignored and team standings used", calculate(
            [15,15,15],[9,9,0],[0,0,1],team_places=[0,1]))
        self.check("bottom XP clamp", calculate([15,15],[1,0],xp=[3,5000])[0])
        self.check("top XP clamp", calculate([15,15],xp=[MAX_XP-3,5000])[0])
        self.check("mismatched playlist key excluded", player_delta(
            [RatingPlayer(15,0,0),RatingPlayer(15,1,1,key=7)],0))
        self.check("flagged guest excluded", player_delta(
            [RatingPlayer(15,0,0),RatingPlayer(15,1,1,flags=1)],0))
        custom = TABLES | {"delta_bounds": [7,-9]}
        self.check("configured maximum delta", calculate([15,15],tables=custom)[0])
        self.check("configured minimum delta", calculate([15,15],tables=custom)[1])

    def test_original_rank_hysteresis_and_display_boundary(self):
        for row in self.original["controls"]:
            if not row["name"].startswith("level xp="): continue
            _,xp,previous = row["name"].split()
            self.check(row["name"],raw_rank(int(xp.split('=')[1]),int(previous.split('=')[1])))
        self.assertEqual([display_level(i) for i in range(50)],list(range(1,51)))
        self.assertEqual(raw_rank(MAX_XP,0),127)
        self.assertEqual(display_level(127),50)  # Original last artwork range.

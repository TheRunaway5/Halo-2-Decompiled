"""Expected arithmetic is captured from unchanged retail instructions."""
import json
from pathlib import Path
import unittest

from multiplayer.master_server.clan_skill import RatingClan, clan_delta
from multiplayer.master_server.playlist_skill import MAX_XP, TABLES


class OriginalClanSkillTests(unittest.TestCase):
    def test_original_clan_collection_controls(self):
        original = json.loads((Path(__file__).parent / "fixtures/retail-clan-original.json").read_text())
        controls = {r["name"]: r["actual"] for r in original["controls"]}
        def ratings(skills=(15,15), places=(0,1), xp=(5500,5500)):
            return [RatingClan(s,p,x) for s,p,x in zip(skills,places,xp)]
        cases = (
            ("equal clan skill win/loss", ratings()),
            ("equal clan standing tie", ratings(places=(0,0))),
            ("clan higher skill differential", ratings(skills=(16,15))),
            ("clan bottom XP clamp", ratings(places=(1,0),xp=(3,5500))),
            ("clan top XP clamp", ratings(xp=(MAX_XP-3,5500))),
            ("unknown opposing clan skill ignored", ratings(skills=(15,-1))),
            ("clan pair loop includes zero identity without admission", ratings()),
            ("clan pair loop averages extra active entries", ratings((15,15,15),(0,0,1),(5500,5500,5500))),
        )
        for name, entries in cases:
            with self.subTest(name=name):
                self.assertEqual([clan_delta(entries,i) for i in range(len(controls[name]))], controls[name])
        self.assertEqual([clan_delta(ratings(),i,TABLES | {"delta_bounds":[7,-9]}) for i in (0,1)],
                         controls["clan configured delta limits"])

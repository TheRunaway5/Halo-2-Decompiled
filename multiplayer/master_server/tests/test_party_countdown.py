import json
from pathlib import Path
import unittest
from unittest.mock import patch

from multiplayer.master_server.party_countdown import Countdown, press, native
from multiplayer.master_server.protocol import Problem


class OriginalPartyCountdownTests(unittest.TestCase):
    def test_missing_native_kernel_fails_closed(self):
        for factory in (dict(return_value=object()),dict(side_effect=OSError("unavailable"))):
            with self.subTest(factory=tuple(factory)):
                native.cache_clear()
                try:
                    with patch("multiplayer.master_server.party_countdown.library",**factory):
                        with self.assertRaises(Problem) as raised:
                            press(None)
                    self.assertEqual(raised.exception.status,503)
                finally:
                    native.cache_clear()

    def test_unchanged_tu15_controls(self):
        source=Path(__file__).resolve().parents[3]/"tests/fixtures/tu15_party_countdown_original.json"
        fixture=json.loads(source.read_text())
        self.assertEqual(fixture["source_sha256"],"21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd")
        for row in fixture["cases"]:
            with self.subTest(kind=row["kind"],row=row):
                if row["kind"]=="timer":
                    c=Countdown(1,row["seconds"],0,"source","source",1)
                    c.update(row["elapsed"])
                    self.assertEqual(c.seconds,row["value"])
                    self.assertEqual(c.clock_ms,row["clock"])
                elif row["kind"]=="press":
                    self.assertEqual(press(row["seconds"]),row["value"])
                elif row["kind"]=="parameter" and row["mode"]==0 and row["oldmode"]==1:
                    c=Countdown(1,row["oldseconds"],0,"source","source",1,1,False,"private")
                    c.update(1000)
                    self.assertEqual((c.seconds,c.mode,c.requester),(row["value"],row["result_mode"],None))

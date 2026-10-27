import json
from pathlib import Path
import unittest
from unittest.mock import patch

from multiplayer.master_server.matchmaking_readiness import connected_teams, native, start_eligible
from multiplayer.master_server.protocol import Problem


class OriginalMatchmakingReadinessNativeTests(unittest.TestCase):
    def test_unchanged_tu15_controls_through_native_abi(self):
        fixture=json.loads((Path(__file__).resolve().parents[3]/"tests/fixtures/tu15_matchmaking_readiness_original.json").read_text())
        self.assertEqual(fixture["original_xbe_sha256"],"21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd")
        for row in fixture["readiness_controls"]:
            with self.subTest(name=row["name"]):
                inputs=row["inputs"];online=bool(inputs.get("online",1))
                peers=[3,3 if inputs.get("second_connected",True) else 0]
                connected=connected_teams(peers,inputs.get("current_peer",0),[0,1],inputs.get("teams",[0,1]))
                flags=inputs.get("debug_policy",0)
                ready=bool(native().h2_matchmaking_eligibility(bool(inputs.get("team_required",1)) and online,
                    connected,bool(inputs.get("local_check",1)) and online,inputs.get("logon",1)==1,
                    bool(inputs.get("local_flag",0)&0x80000000),bool(flags&255),bool(flags&0xff00)))
                self.assertEqual((ready,not ready),(row["ready"],row["defer"]))

    def test_actual_transport_inputs_do_not_invent_original_live_policy(self):
        self.assertTrue(start_eligible([3,3],0,[0,1],[0,1],True))
        self.assertFalse(start_eligible([3,0],0,[0,1],[0,1],True))
        self.assertTrue(start_eligible([3,0],1,[0,1],[0,1],True))
        self.assertTrue(start_eligible([3,3],0,[0,1],[0,0],False))
        self.assertFalse(connected_teams([3,3],0,[0,1],[0,1],occupied=1))

    def test_missing_native_kernel_fails_closed(self):
        for factory in (dict(return_value=object()),dict(side_effect=OSError("unavailable"))):
            with self.subTest(factory=tuple(factory)):
                native.cache_clear()
                try:
                    with patch("multiplayer.master_server.matchmaking_readiness.library",**factory):
                        with self.assertRaises(Problem) as raised:
                            start_eligible([3,3],0,[0,1],[0,1],True)
                    self.assertEqual(raised.exception.status,503)
                finally:
                    native.cache_clear()

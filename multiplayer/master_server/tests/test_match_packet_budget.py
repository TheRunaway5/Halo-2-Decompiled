import unittest

from multiplayer.master_server.match_transport import PacketBudget
from multiplayer.master_server.protocol import Problem


class MatchPacketBudgetTests(unittest.TestCase):
    def test_full_lobby_voice_fanout_and_game_traffic_have_independent_bounded_budgets(self):
        now = [0.]
        budget = PacketBudget(clock=lambda:now[0])
        # Native20ms audio,16 admitted players. A host forwards each speaker to
        # every other remote destination; maximum32KiB native snapshots split
        # into24 bounded datagrams, still30Hz per peer.
        for frame in range(150):
            now[0] = frame*.02
            for speaker in range(16):
                for target in range(1,16):
                    if speaker != target:
                        budget.take((speaker,target))
                    if frame%10 == 0:  # Native postgame membership/mute updates:5Hz.
                        budget.take((speaker,target))
            if frame%5 in (0,2,4):
                for target in range(1,16):
                    for fragment in range(24):
                        budget.take(game=target)
        # Exhausting one speaker/destination never borrows another stream's
        # credits or the game/control bucket.
        for _ in range(74):
            budget.take((0,1))
        with self.assertRaises(Problem):
            budget.take((0,1))
        budget.take((0,2))
        budget.take()

    def test_each_game_destination_is_bounded_without_borrowing_other_credits(self):
        budget = PacketBudget(clock=lambda:0.)
        for _ in range(512):
            budget.take(game=0)
        with self.assertRaises(Problem):
            budget.take(game=0)
        budget.take(game=1)
        budget.take((0,1))
        budget.take()

    def test_control_flood_cannot_borrow_audio_credits(self):
        budget = PacketBudget(clock=lambda:0.)
        for _ in range(512):
            budget.take()
        with self.assertRaises(Problem):
            budget.take()
        budget.take((0,1))

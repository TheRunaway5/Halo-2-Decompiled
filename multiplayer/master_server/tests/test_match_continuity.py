import unittest
from types import SimpleNamespace

from multiplayer.master_server.match_continuity import PlayerHostedContinuity
from multiplayer.master_server.protocol import Problem
from multiplayer.master_server.couch_players import same_continuity_protocol, supports_compatibility


class ContinuityTest(unittest.TestCase):
    def test_continuity_parties_cannot_silently_use_a_legacy_room(self):
        connection=lambda version: SimpleNamespace(compatibility="halo2-native-queue-v" + str(version))
        self.assertTrue(same_continuity_protocol([connection(11), connection(11)]))
        self.assertFalse(same_continuity_protocol([connection(10), connection(11)]))
        self.assertTrue(same_continuity_protocol([connection(9), connection(10)]))
        self.assertTrue(supports_compatibility("halo2-native-queue-v11", "halo2-native-queue-v10"))
        self.assertFalse(supports_compatibility("halo2-native-queue-v9", "halo2-native-queue-v10"))

    def world(self, owners=(0, 1, 2), host=0):
        now = [0.0]
        room = PlayerHostedContinuity(owners, host, clock=lambda: now[0])
        room.seal(host, 0, 100, "a" * 64)
        for peer in set(owners) - {host}:
            room.acknowledge(peer, 0, 100, "a" * 64)
        return room, now

    def test_nonhost_loss_keeps_authority_clock_and_world(self):
        room, _ = self.world()
        self.assertTrue(room.disconnect(2))
        self.assertFalse(room.disconnect(2))
        self.assertEqual((room.host, room.epoch, room.phase, room.members), (0, 0, "running", 3))
        self.assertEqual(room.seals[100].digest, "a" * 64)

    def test_four_seat_couch_is_one_departure_and_never_a_guest_host(self):
        room, _ = self.world((0, 0, 0, 0, 4, 4, 4, 4, 8), 0)
        room.disconnect(0)
        self.assertEqual(room.members, 0x1F0)
        self.assertIn(room.host, (4, 8))
        with self.assertRaises(Problem):
            room.accept(5, room.epoch, 100, "a" * 64)

    def test_host_loss_waits_for_world_and_all_surviving_rebinds(self):
        room, _ = self.world()
        room.disconnect(0)
        self.assertEqual(room.phase, "migrating")
        room.accept(1, 1, 100, "a" * 64)
        self.assertEqual(room.phase, "rebinding")
        room.rebind(2, 1, 100, "a" * 64)
        self.assertEqual((room.phase, room.epoch, room.members), ("running", 1, 6))
        with self.assertRaises(Problem):
            room.seal(0, 0, 101, "b" * 64)

    def test_newest_acknowledged_complete_world_wins(self):
        room, _ = self.world()
        room.seal(0, 0, 101, "b" * 64)
        room.acknowledge(2, 0, 101, "b" * 64)
        room.disconnect(0)
        self.assertEqual((room.host, room.selected.tick), (2, 101))

    def test_mismatched_world_never_promotes(self):
        room, _ = self.world()
        with self.assertRaises(Problem):
            room.acknowledge(1, 0, 100, "b" * 64)
        room.disconnect(0)
        with self.assertRaises(Problem):
            room.accept(room.host, 1, 100, "b" * 64)
        self.assertEqual(room.phase, "migrating")

    def test_host_loss_racing_latest_ack_waits_without_rollback(self):
        room, _ = self.world()
        room.seal(0, 0, 101, "b" * 64)
        room.disconnect(0)
        self.assertEqual(room.phase, "awaiting_world")
        room.acknowledge(2, 0, 101, "b" * 64)
        self.assertEqual((room.phase, room.host, room.epoch, room.selected.tick),
                         ("migrating", 2, 1, 101))

    def test_latest_world_never_arrives_times_out_explicitly(self):
        room, now = self.world()
        room.seal(0, 0, 101, "b" * 64)
        room.disconnect(0)
        now[0] = 16
        self.assertTrue(room.expire())
        self.assertEqual(room.phase, "unavailable")

    def test_candidate_loss_repeats_election_with_new_epoch(self):
        room, _ = self.world()
        room.disconnect(0)
        room.disconnect(1)
        self.assertEqual((room.host, room.epoch), (2, 2))
        room.accept(2, 2, 100, "a" * 64)
        self.assertEqual(room.phase, "running")

    def test_candidate_timeout_rejects_old_acceptance(self):
        room, now = self.world()
        room.disconnect(0)
        now[0] = 16
        self.assertTrue(room.expire())
        self.assertEqual((room.host, room.epoch), (2, 2))
        with self.assertRaises(Problem):
            room.accept(1, 1, 100, "a" * 64)

    def test_missing_complete_world_is_explicitly_unavailable(self):
        room = PlayerHostedContinuity((0, 1), 0)
        room.disconnect(0)
        self.assertEqual(room.phase, "unavailable")

    def test_rebind_timeout_removes_only_unrebound_peer(self):
        room, now = self.world()
        room.disconnect(0)
        room.accept(1, 1, 100, "a" * 64)
        now[0] = 16
        room.expire()
        self.assertEqual((room.phase, room.active, room.members), ("running", {1}, 2))

    def test_repeated_migration_preserves_generation_and_state(self):
        room, _ = self.world((0, 1, 2, 3))
        room.disconnect(0)
        room.accept(1, 1, 100, "a" * 64)
        room.rebind(2, 1, 100, "a" * 64)
        room.rebind(3, 1, 100, "a" * 64)
        room.seal(1, 1, 120, "b" * 64)
        room.acknowledge(2, 1, 120, "b" * 64)
        room.acknowledge(3, 1, 120, "b" * 64)
        room.disconnect(1)
        room.accept(2, 2, 120, "b" * 64)
        room.rebind(3, 2, 120, "b" * 64)
        self.assertEqual((room.phase, room.epoch, room.members), ("running", 2, 12))

    def test_observed_quality_is_native_tie_break_after_world_recency(self):
        room, _ = self.world()
        room.observe_quality(1, 50)
        room.observe_quality(2, 10)
        room.disconnect(0)
        self.assertEqual(room.host, 2)

    def test_metadata_is_bounded_and_sealed_ticks_are_immutable(self):
        room, _ = self.world()
        for tick in range(101, 200):
            room.seal(0, 0, tick, "a" * 64)
        self.assertEqual(len(room.seals), 64)
        with self.assertRaises(Problem):
            room.seal(0, 0, 199, "b" * 64)
        with self.assertRaises(Problem):
            room.seal(0, 0, 198, "a" * 64)


if __name__ == "__main__":
    unittest.main()

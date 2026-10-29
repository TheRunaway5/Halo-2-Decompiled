"""Actual HTTP rooms bind Rust continuity; world digests are declared fixtures."""
import unittest

from multiplayer.master_server.match_continuity import PlayerHostedContinuity
from multiplayer.master_server.protocol import Problem
from . import test_match_continuity_http as baseline


class ContinuityOwnerTests(unittest.IsolatedAsyncioTestCase):
    asyncSetUp = baseline.ContinuityHttpTests.asyncSetUp
    session = baseline.ContinuityHttpTests.session
    enqueue = baseline.ContinuityHttpTests.enqueue
    pair = baseline.ContinuityHttpTests.pair
    report = baseline.ContinuityHttpTests.report
    catalog_fixture = baseline.ContinuityHttpTests.__dict__['catalog_fixture']
    next = baseline.ContinuityHttpTests.next
    phase = baseline.ContinuityHttpTests.phase
    running = baseline.ContinuityHttpTests.running

    async def test_room_attaches_to_one_native_owner_and_cannot_install_mutable_state(self):
        _, _, _, _, room = await self.running()
        view = room.continuity
        owners = tuple(room.index(room.owners[p]) for p, _, _ in room.players)
        self.assertEqual(view.owners, owners)
        with self.assertRaises(AttributeError): room.continuity = None
        with self.assertRaises(AttributeError): view.epoch = 1234
        with self.assertRaises(AttributeError): view.deadline = 0
        with self.assertRaises(AttributeError): view.active.add(15)
        with self.assertRaises(TypeError): view.quality[0] = 0
        with self.assertRaises(TypeError): view.seals[0] = None
        before = view._snapshot()
        for operation in ('restore', 'open', 'install'):
            with self.assertRaises(Problem): view._call(operation, value=before)
        attached = PlayerHostedContinuity.attach(self.api.core, room.id)
        self.assertEqual(attached._snapshot(), before)
        await room.close()
        with self.assertRaises(Problem): attached._snapshot()
        with self.assertRaises(Problem): PlayerHostedContinuity.attach(self.api.core, room.id)

    async def test_late_seal_is_ignored_using_native_result_state(self):
        descriptors, _, _, host, room = await self.running()
        actor = room.players[host][0]
        raw = self.report(descriptors)
        # Submission updates the authoritative Rust phase and result together.
        self.api.results.submit(room, actor, raw.hex())
        self.assertEqual(room.phase, 'reviewing')
        before = room.continuity._snapshot()
        for action in ('world_seal', 'world_ack'):
            self.assertTrue(await room.world_control(actor, {
                action:dict(epoch=0, tick=100, digest='a'*64)}))
        self.assertEqual(room.continuity._snapshot(), before)

    async def test_native_message_validation_is_atomic_for_the_frozen_physical_actor(self):
        _, _, _, host, room = await self.running()
        actor = room.players[host][0]
        before = room.continuity._snapshot()
        invalid = [
            {'world_seal':dict(epoch=True, tick=100, digest='a'*64)},
            {'world_seal':dict(epoch=0, tick=2**64, digest='a'*64)},
            {'world_seal':dict(epoch=0, tick=100, digest='A'*64)},
            {'world_seal':dict(epoch=0, tick=100, digest='a'*64), 'extra':0},
        ]
        for data in invalid:
            with self.assertRaises(Problem): await room.world_control(actor, data)
            self.assertEqual(room.continuity._snapshot(), before)
        with self.assertRaises(Problem) as caught:
            await room.world_control('not-admitted', {'world_seal':dict(epoch=0, tick=100, digest='a'*64)})
        self.assertEqual(caught.exception.status, 403)
        self.assertEqual(room.continuity._snapshot(), before)

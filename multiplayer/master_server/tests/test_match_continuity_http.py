"""Actual relay/election controls; complete-world digests are declared fixtures.

These tests do not claim native simulation import or live host migration.
"""
import asyncio
import hashlib
import json
import time
import unittest

from aiohttp import WSServerHandshakeError
from . import test_matchmaking_http as baseline
from . import test_match_transport as transport


def catalog():
    value = baseline.catalog()
    for row in value["playlists"]:
        row.update(minimum_players=3, maximum_players=3)
    return value


class ContinuityHttpTests(unittest.IsolatedAsyncioTestCase):
    asyncSetUp = baseline.MatchmakingHttpTests.asyncSetUp
    enqueue = baseline.MatchmakingHttpTests.enqueue
    pair = transport.MatchTransportTests.pair
    report = transport.MatchTransportTests.report
    catalog_fixture = staticmethod(catalog)

    async def session(self, owner="one", name="Same Name", maps=None):
        response = await self.client.post("/v1/matchmaking/session",
            headers={"Authorization": "Bearer " + owner},
            json=dict(screen_name=name, maps=maps or ["lockout"],
                      compatibility="halo2-native-queue-v11", language=0))
        self.assertEqual(response.status, 201, await response.text())
        return {"Authorization": "Bearer " + (await response.json())["token"]}

    async def next(self, queue, predicate):
        async with asyncio.timeout(3):
            while True:
                value = await queue.get()
                if predicate(value):
                    return value

    async def phase(self, queue, expected):
        return await self.next(queue, lambda p: isinstance(p, dict) and p.get("phase") == expected)

    async def running(self):
        _, descriptors, sockets, queues, host = await self.pair(("one", "two", "three"))
        self.assertTrue(all(d["continuity"] == "complete-world-v1" for d in descriptors))
        for socket in sockets:
            await socket.send_json({"loaded": "1234567890abcdef:000003c"})
        for queue in queues:
            value = await self.phase(queue, "running")
            self.assertEqual(value["continuity"]["epoch"], 0)
        room = self.api.transport.rooms[descriptors[0]["id"]]
        return descriptors, sockets, queues, host, room

    async def sealed(self, descriptors, sockets, queues, host, room):
        h = next(i for i, d in enumerate(descriptors) if d["slot"] == host)
        value = dict(epoch=0, tick=100, digest="a" * 64)
        await sockets[h].send_json({"world_seal": value})
        for queue in queues:
            self.assertEqual(await self.next(queue, lambda p: isinstance(p, dict) and "world_seal" in p),
                             {"world_seal": value})
        for i, socket in enumerate(sockets):
            if i != h:
                await socket.send_json({"world_ack": value})
        async with asyncio.timeout(3):
            while len(room.continuity.proofs) != 3:
                await asyncio.sleep(.005)
        return h

    def packet(self, target, epoch, kind=1):
        return bytes([target]) + epoch.to_bytes(4, "little") + b"H2N2\x3c\x00" + bytes([kind]) + b"x" * 20

    async def test_nonhost_disconnect_keeps_routes_but_cannot_supply_result_consent(self):
        descriptors, sockets, queues, host, room = await self.running()
        h = next(i for i, d in enumerate(descriptors) if d["slot"] == host)
        lost = (h + 1) % 3
        survivor = (h + 2) % 3
        await sockets[lost].close()
        for i in (h, survivor):
            value = await self.phase(queues[i], "running")
            self.assertEqual((value["host"], value["continuity"]["epoch"]), (host, 0))
            self.assertEqual(value["continuity"]["members"], 7 ^ (1 << descriptors[lost]["slot"]))
        self.assertFalse(room.closed)
        packet = self.packet(host, 0)
        await sockets[survivor].send_bytes(packet)
        received = await self.next(queues[h], lambda p: isinstance(p, bytes))
        self.assertEqual(received, bytes([descriptors[survivor]["slot"]]) + packet[1:])
        # A final in-flight host packet for the departed destination is benign.
        await sockets[h].send_bytes(self.packet(descriptors[lost]["slot"], 0, 3))
        raw = self.report(descriptors)
        await sockets[h].send_json({"postgame": raw.hex()})
        for i in (h, survivor):
            await self.next(queues[i], lambda p: isinstance(p, dict) and "postgame" in p)
        for i in (h, survivor):
            await sockets[i].send_json({"confirm_result": hashlib.sha256(raw).hexdigest()})
        async with asyncio.timeout(3):
            while len(room.confirmations) != 2:
                await asyncio.sleep(.005)
        row = self.api.directory.store.db.execute(
            "SELECT policy,admitted_connections,confirming_connections,departed_slots,authority_epoch "
            "FROM match_result_consensus WHERE match=?", (room.id,)).fetchone()
        self.assertIsNone(row)
        self.assertEqual(room.phase, "reviewing")
        self.assertEqual(self.api.directory.store.db.execute(
            "SELECT count(*) FROM match_results WHERE id=?", (room.id,)).fetchone()[0], 0)
        self.assertEqual(self.api.directory.store.db.execute(
            "SELECT count(*) FROM match_result_players WHERE match=?", (room.id,)).fetchone()[0], 0)

    async def test_review_confirmation_survives_relay_disconnect(self):
        descriptors, sockets, queues, host, room = await self.running()
        h = next(i for i, d in enumerate(descriptors) if d["slot"] == host)
        leaving, last = [i for i in range(3) if i != h]
        raw = self.report(descriptors)
        digest = hashlib.sha256(raw).hexdigest()
        await sockets[h].send_json({"postgame": raw.hex()})
        for queue in queues:
            await self.next(queue, lambda p: isinstance(p, dict) and "postgame" in p)
        await sockets[leaving].send_json({"confirm_result": digest})
        owner = room.players[descriptors[leaving]["slot"]][0]
        async with asyncio.timeout(3):
            while owner not in room.confirmations:
                await asyncio.sleep(.005)
        await sockets[leaving].close()
        async with asyncio.timeout(3):
            while owner in room.tokens:
                await asyncio.sleep(.005)
        self.assertIn(owner, room.confirmations)
        for i in (h, last):
            await sockets[i].send_json({"confirm_result": digest})
        for i in (h, last):
            self.assertEqual(await self.next(queues[i], lambda p: isinstance(p, dict) and "completed" in p),
                             {"completed": digest})
        receipt = self.api.directory.store.db.execute(
            "SELECT policy,admitted_connections,confirming_connections FROM match_result_consensus WHERE match=?",
            (room.id,)).fetchone()
        self.assertEqual(tuple(receipt), ("native-unanimous-v1", 3, 3))

    async def test_host_handoff_preserves_room_and_rejects_old_generation(self):
        descriptors, sockets, queues, host, room = await self.running()
        h = await self.sealed(descriptors, sockets, queues, host, room)
        await sockets[h].close()
        survivors = [i for i in range(3) if i != h]
        controls = [await self.phase(queues[i], "migrating") for i in survivors]
        self.assertEqual(controls[0], controls[1])
        leader = controls[0]["host"]
        self.assertNotEqual(leader, host)
        proof = dict(epoch=1, tick=100, digest="a" * 64)
        self.assertEqual(controls[0]["continuity"]["tick"], 100)
        with self.assertRaises(WSServerHandshakeError):
            await self.client.ws_connect("/v1/matches/" + room.id + "/relay",
                headers={"Authorization": "Bearer " + descriptors[h]["token"]})
        candidate = next(i for i in survivors if descriptors[i]["slot"] == leader)
        other = next(i for i in survivors if i != candidate)
        await sockets[other].send_bytes(self.packet(host, 0))
        # A legitimate completed-tick acknowledgement may already be queued
        # when the former host disappears. It cannot evict a healthy survivor.
        await sockets[other].send_json({"world_ack": dict(epoch=0, tick=100, digest="a" * 64)})
        await sockets[candidate].send_json({"migration_accept": proof})
        for i in survivors:
            await self.phase(queues[i], "rebinding")
        await sockets[other].send_json({"migration_rebind": proof})
        for i in survivors:
            running = await self.phase(queues[i], "running")
            self.assertEqual(running["continuity"]["epoch"], 1)
        await sockets[other].send_bytes(self.packet(leader, 1))
        received = await self.next(queues[candidate], lambda p: isinstance(p, bytes))
        self.assertEqual(received[0], descriptors[other]["slot"])
        self.assertEqual(int.from_bytes(received[1:5], "little"), 1)
        self.assertFalse(room.closed)

    async def test_unacknowledged_world_never_becomes_a_new_host(self):
        descriptors, sockets, queues, host, room = await self.running()
        h = next(i for i, d in enumerate(descriptors) if d["slot"] == host)
        await sockets[h].close()
        for i in range(3):
            if i != h:
                self.assertIsNone(await self.next(queues[i], lambda p: p is None))
        self.assertTrue(room.closed)

    async def test_forged_authority_control_removes_only_offending_peer(self):
        descriptors, sockets, queues, host, room = await self.running()
        bad = next(i for i, d in enumerate(descriptors) if d["slot"] != host)
        await sockets[bad].send_json({"world_seal": dict(epoch=0, tick=100, digest="a" * 64)})
        self.assertIsNone(await self.next(queues[bad], lambda p: p is None))
        for i in range(3):
            if i != bad:
                await self.phase(queues[i], "running")
        self.assertFalse(room.closed)

    async def test_unadmitted_native_protocol_removes_only_sender(self):
        descriptors, sockets, queues, host, room = await self.running()
        bad = next(i for i, d in enumerate(descriptors) if d["slot"] != host)
        packet = bytearray(self.packet(host, 0))
        packet[9] += 1
        await sockets[bad].send_bytes(packet)
        self.assertIsNone(await self.next(queues[bad], lambda p: p is None))
        for i in range(3):
            if i != bad:
                await self.phase(queues[i], "running")
        self.assertFalse(room.closed)

    async def test_rebind_timeout_revokes_only_unrebound_connection(self):
        descriptors, sockets, queues, host, room = await self.running()
        h = await self.sealed(descriptors, sockets, queues, host, room)
        await sockets[h].close()
        survivors = [i for i in range(3) if i != h]
        controls = [await self.phase(queues[i], "migrating") for i in survivors]
        candidate = next(i for i in survivors if descriptors[i]["slot"] == controls[0]["host"])
        lost = next(i for i in survivors if i != candidate)
        await sockets[candidate].send_json({"migration_accept": dict(epoch=1, tick=100, digest="a" * 64)})
        for i in survivors:
            await self.phase(queues[i], "rebinding")
        room.continuity.deadline = time.monotonic() - 1
        await room.synchronize_continuity()
        value = await self.phase(queues[candidate], "running")
        self.assertEqual(value["continuity"]["members"], 1 << descriptors[candidate]["slot"])
        self.assertIsNone(await self.next(queues[lost], lambda p: p is None))
        self.assertFalse(room.closed)
        self.assertFalse(room.active_member(room.players[descriptors[lost]["slot"]][0]))
        with self.assertRaises(WSServerHandshakeError):
            await self.client.ws_connect("/v1/matches/" + room.id + "/relay",
                headers={"Authorization": "Bearer " + descriptors[lost]["token"]})
        # Neither the departed host nor a timed-out rebind supplied consent.
        # A complete-world acknowledgement is not a postgame result vote.
        raw = self.report(descriptors)
        await sockets[candidate].send_json({"postgame": raw.hex()})
        await self.next(queues[candidate], lambda p: isinstance(p, dict) and "postgame" in p)
        await sockets[candidate].send_json({"confirm_result": hashlib.sha256(raw).hexdigest()})
        owner = room.players[descriptors[candidate]["slot"]][0]
        async with asyncio.timeout(3):
            while owner not in room.confirmations:
                await asyncio.sleep(.005)
        self.assertEqual(room.phase, "reviewing")
        self.assertEqual(self.api.directory.store.db.execute(
            "SELECT count(*) FROM match_results WHERE id=?", (room.id,)).fetchone()[0], 0)


if __name__ == "__main__":
    unittest.main()

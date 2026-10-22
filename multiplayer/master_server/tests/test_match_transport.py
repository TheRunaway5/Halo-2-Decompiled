import asyncio
import contextlib
import hashlib
import json
import struct
from dataclasses import replace
import unittest
from aiohttp import WSMsgType, WSServerHandshakeError
from . import test_matchmaking_http as baseline


class MatchTransportTests(unittest.IsolatedAsyncioTestCase):
    asyncSetUp = baseline.MatchmakingHttpTests.asyncSetUp
    session = baseline.MatchmakingHttpTests.session
    enqueue = baseline.MatchmakingHttpTests.enqueue

    async def pair(self, owners=("one", "two"), leaders=None):
        headers = [await self.session(owner, "Same Name") for owner in owners]
        for i, header in enumerate(headers):
            if leaders is None or i in leaders:
                self.assertEqual((await self.enqueue(header)).status, 201)
        descriptors = []
        for header in headers:
            response = await self.client.get("/v1/matchmaking/transport", headers=header)
            self.assertEqual(response.status, 200, await response.text())
            descriptors.append(await response.json())
        self.assertEqual(descriptors[0]["id"], descriptors[1]["id"])
        self.assertNotEqual(descriptors[0]["token"], descriptors[1]["token"])
        self.assertNotEqual(descriptors[0]["slot"], descriptors[1]["slot"])
        sockets, queues = [], []
        async def reader(socket, queue):
            async for message in socket:
                if message.type == WSMsgType.TEXT:
                    data = json.loads(message.data)
                    if "ping" in data:
                        await socket.send_json({"pong": data["ping"]})
                    else:
                        await queue.put(data)
                elif message.type == WSMsgType.BINARY:
                    await queue.put(message.data)
            await queue.put(None)
        async def stop(task):
            task.cancel()
            with contextlib.suppress(asyncio.CancelledError):
                await task
        for descriptor in descriptors:
            socket = await self.client.ws_connect("/v1/matches/"+descriptor["id"]+"/relay",
                                                  headers={"Authorization": "Bearer "+descriptor["token"]})
            queue = asyncio.Queue()
            task = asyncio.create_task(reader(socket, queue))
            self.addAsyncCleanup(stop, task)
            self.addAsyncCleanup(socket.close)
            sockets.append(socket)
            queues.append(queue)
        for queue in queues:
            while True:
                value = await asyncio.wait_for(queue.get(), 4)
                if value and value.get("phase") == "loading":
                    host = value["host"]
                    break
        return headers, descriptors, sockets, queues, host

    async def test_admission_real_challenges_loading_and_authenticated_routes(self):
        headers, descriptors, sockets, queues, host = await self.pair()
        self.assertFalse(self.api.core.tickets)
        for socket in sockets:
            await socket.send_json({"loaded": "1234567890abcdef:000001a"})
        for queue in queues:
            self.assertEqual((await asyncio.wait_for(queue.get(), 2))["phase"], "running")
        h = next(i for i, row in enumerate(descriptors) if row["slot"] == host)
        c = 1-h
        # Gateway chooses the sender slot; duplicate screen names are harmless.
        packet = b"H2N2\x1a\x00\x01"+b"x"*20
        await sockets[c].send_bytes(bytes([host])+packet)
        self.assertEqual(await asyncio.wait_for(queues[h].get(), 2), bytes([descriptors[c]["slot"]])+packet)
        snapshot = b"H2N2\x1a\x00\x03"+b"y"*1393
        await sockets[h].send_bytes(bytes([descriptors[c]["slot"]])+snapshot)
        self.assertEqual(await asyncio.wait_for(queues[c].get(), 2), bytes([host])+snapshot)
        # Frontend queue teardown does not revoke the transferred match bearer.
        self.assertEqual((await self.client.delete("/v1/matchmaking/session", headers=headers[c])).status, 200)
        await sockets[c].send_bytes(bytes([host])+packet)
        self.assertEqual(await asyncio.wait_for(queues[h].get(), 2), bytes([descriptors[c]["slot"]])+packet)

    async def test_data_mismatch_closes_everyone(self):
        _, _, sockets, queues, _ = await self.pair()
        await sockets[0].send_json({"loaded": "1234567890abcdef:000001a"})
        await sockets[1].send_json({"loaded": "2234567890abcdef:000001a"})
        for queue in queues:
            self.assertIsNone(await asyncio.wait_for(queue.get(), 2))

    async def test_audio_flood_is_bounded_on_the_authenticated_stream(self):
        from multiplayer.master_server.match_transport import PacketBudget
        _, descriptors, sockets, queues, host = await self.pair()
        for socket in sockets:
            await socket.send_json({"loaded": "1234567890abcdef:000001c"})
        for queue in queues:
            await asyncio.wait_for(queue.get(), 2)
        c = next(i for i,d in enumerate(descriptors) if d["slot"] != host)
        room = self.api.transport.rooms[descriptors[0]["id"]]
        member = room.players[descriptors[c]["slot"]][0]
        # Freeze just the packet limiter's time so a single-stream burst is
        # deterministic; real socket admission/challenges remain unchanged.
        room.peers[member].budget = PacketBudget(clock=lambda:0.)
        audio = bytearray(23)
        audio[:7] = b"H2N2\x1c\x00\x06"
        audio[8] = descriptors[c]["slot"]
        from .voice_signing import sign_voice
        audio = sign_voice(room, bytes(audio))
        for _ in range(76):
            await sockets[c].send_bytes(bytes([host])+audio)
        for queue in queues:
            while await asyncio.wait_for(queue.get(),2) is not None:
                pass
        self.assertTrue(room.closed)

    async def test_game_flood_is_bounded_on_the_authenticated_destination(self):
        from multiplayer.master_server.match_transport import PacketBudget
        _, descriptors, sockets, queues, host = await self.pair()
        for socket in sockets:
            await socket.send_json({"loaded": "1234567890abcdef:000001c"})
        for queue in queues:
            await asyncio.wait_for(queue.get(), 2)
        c = next(i for i,d in enumerate(descriptors) if d["slot"] != host)
        room = self.api.transport.rooms[descriptors[0]["id"]]
        member = room.players[descriptors[c]["slot"]][0]
        room.peers[member].budget = PacketBudget(clock=lambda:0.)
        packet = bytes([host])+b"H2N2\x1c\x00\x02"+b"x"*20
        for i in range(513):
            await sockets[c].send_bytes(packet)
            if i%32 == 0:
                await asyncio.sleep(0)  # Let real socket writers/readers drain.
        for queue in queues:
            while await asyncio.wait_for(queue.get(),2) is not None:
                pass
        self.assertTrue(room.closed)

    async def test_voice_preferences_share_the_bounded_speaker_stream(self):
        from multiplayer.master_server.match_transport import PacketBudget
        _, descriptors, sockets, queues, host = await self.pair()
        for socket in sockets:
            await socket.send_json({"loaded": "1234567890abcdef:000001c"})
        for queue in queues:
            await asyncio.wait_for(queue.get(), 2)
        c = next(i for i,d in enumerate(descriptors) if d["slot"] != host)
        room = self.api.transport.rooms[descriptors[0]["id"]]
        member = room.players[descriptors[c]["slot"]][0]
        room.peers[member].budget = PacketBudget(clock=lambda:0.)
        preferences = bytes([host])+b"H2N2\x1c\x00\x06\x01\x01\x00\x00"
        for _ in range(76):
            await sockets[c].send_bytes(preferences)
        for queue in queues:
            while await asyncio.wait_for(queue.get(),2) is not None:
                pass
        self.assertTrue(room.closed)

    async def test_presence_follows_live_match_after_frontend_session_teardown(self):
        from multiplayer.master_server.player_presence import observed
        headers, _, sockets, queues, _ = await self.pair()
        directory = self.api.directory
        players = {s.player for s in self.api.sessions.values()}
        for header in headers:
            self.assertEqual((await self.client.delete("/v1/matchmaking/session", headers=header)).status, 200)
        self.assertFalse(self.api.sessions)
        self.assertEqual(observed(directory), dict.fromkeys(players, "online"))
        for socket in sockets:
            await socket.send_json({"loaded": "1234567890abcdef:000001a"})
        for queue in queues:
            self.assertEqual((await asyncio.wait_for(queue.get(), 2))["phase"], "running")
        self.assertEqual(observed(directory), dict.fromkeys(players, "playing"))
        await sockets[0].close()
        for queue in queues:
            self.assertIsNone(await asyncio.wait_for(queue.get(), 2))
        self.assertEqual(observed(directory), {})

    async def test_client_cannot_send_authoritative_snapshots(self):
        _, descriptors, sockets, queues, host = await self.pair()
        for socket in sockets:
            await socket.send_json({"loaded": "1234567890abcdef:000001a"})
        for queue in queues:
            await asyncio.wait_for(queue.get(), 2)
        client = next(i for i, row in enumerate(descriptors) if row["slot"] != host)
        await sockets[client].send_bytes(bytes([host])+b"H2N2\x1a\x00\x03"+b"x"*20)
        for queue in queues:
            self.assertIsNone(await asyncio.wait_for(queue.get(), 2))

    async def test_anonymous_and_unmatched_transport_denied(self):
        self.assertEqual((await self.client.get("/v1/matchmaking/transport")).status, 401)
        header = await self.session()
        self.assertEqual((await self.client.get("/v1/matchmaking/transport", headers=header)).status, 409)
        with self.assertRaises(WSServerHandshakeError):
            await self.client.ws_connect("/v1/matches/00000000-0000-0000-0000-000000000000/relay")

    def report(self, descriptors):
        raw = bytearray(0xdc78)
        raw[0:2] = raw[0x10:0x12] = b"\x01\x01"
        raw[0x64] = 2
        for slot, row in enumerate(descriptors[0]["players"]):
            at = 0x370 + slot * 0x94
            raw[at] = 1
            raw[at+0x88] = 255
            raw[at+2:at+10] = bytes.fromhex(row["identity"])
            name = row["name"].encode("utf-16-le")
            raw[at+12:at+12+len(name)] = name
        return raw

    async def test_completed_report_allows_independent_requeue(self):
        headers, descriptors, sockets, queues, host = await self.pair()
        for socket in sockets:
            await socket.send_json({"loaded": "1234567890abcdef:000001b"})
        for queue in queues:
            await asyncio.wait_for(queue.get(), 2)
        h = next(i for i, row in enumerate(descriptors) if row["slot"] == host)
        report = self.report(descriptors).hex()
        await sockets[h].send_json({"postgame": report})
        for queue in queues:
            self.assertEqual(await asyncio.wait_for(queue.get(), 2), {"postgame": report})
        self.assertEqual(self.api.directory.store.db.execute("SELECT count(*) FROM match_results").fetchone()[0], 0)
        digest = hashlib.sha256(bytes.fromhex(report)).hexdigest()
        await sockets[h].send_json({"confirm_result": digest})
        await asyncio.sleep(.05)
        self.assertEqual(self.api.directory.store.db.execute("SELECT count(*) FROM match_results").fetchone()[0], 0)
        await sockets[1-h].send_json({"confirm_result": digest})
        for queue in queues:
            self.assertEqual(await asyncio.wait_for(queue.get(), 2), {"completed": digest})
            self.assertEqual(await asyncio.wait_for(queue.get(), 2), {"voice_host": host, "voice_members": 3})
        db = self.api.directory.store.db
        self.assertEqual(db.execute("SELECT count(*) FROM match_results").fetchone()[0], 1)
        self.assertEqual(db.execute("SELECT count(*) FROM match_result_players").fetchone()[0], 2)
        self.assertEqual(db.execute("SELECT digest,validation FROM match_results").fetchone()[:],
                         (digest, "native-unanimous-v1"))
        # Idempotent confirmations cannot add results or advance skill twice.
        await sockets[h].send_json({"confirm_result": digest})
        await asyncio.sleep(.05)
        self.assertEqual(db.execute("SELECT count(*) FROM match_results").fetchone()[0], 1)
        self.assertEqual(set(r[0] for r in db.execute("SELECT rank FROM playlist_ranks")), {1})
        import sqlite3
        with sqlite3.connect(self.api.directory.settings.state / "registry.sqlite3") as persisted:
            self.assertEqual(persisted.execute("SELECT digest FROM match_results").fetchone()[0], digest)
        c = 1-h
        # Voice is admitted through the same match bearer after simulation;
        # obsolete gameplay never reaches the completed report screen.
        packet = b"H2N2\x1c\x00\x06\x01\x01\x00\x00"
        await sockets[c].send_bytes(bytes([host])+packet)
        self.assertEqual(await asyncio.wait_for(queues[h].get(), 2), bytes([descriptors[c]["slot"]])+packet)
        await sockets[c].send_bytes(bytes([host])+b"H2N2\x1c\x00\x02"+b"x"*20)
        with self.assertRaises(asyncio.TimeoutError):
            await asyncio.wait_for(queues[h].get(), .05)
        await sockets[h].close()
        self.assertEqual(await asyncio.wait_for(queues[c].get(), 2),
                         {"voice_host": descriptors[c]["slot"], "voice_members": 1 << descriptors[c]["slot"]})
        await sockets[c].send_bytes(bytes([host])+packet)
        await asyncio.sleep(.05)
        self.assertFalse(sockets[1-h].closed)
        self.assertEqual((await self.enqueue(headers[h])).status, 201)
        self.assertEqual((await self.enqueue(headers[1-h])).status, 201)
        self.assertEqual(len(set(p["identity"] for p in descriptors[0]["players"])), 2)

    async def test_disagreed_report_cannot_enter_persistent_results(self):
        _, descriptors, sockets, queues, host = await self.pair()
        for socket in sockets:
            await socket.send_json({"loaded": "1234567890abcdef:000001b"})
        for queue in queues:
            await asyncio.wait_for(queue.get(), 2)
        h = next(i for i, row in enumerate(descriptors) if row["slot"] == host)
        await sockets[h].send_json({"postgame": self.report(descriptors).hex()})
        for queue in queues:
            await asyncio.wait_for(queue.get(), 2)
        await sockets[1-h].send_json({"confirm_result": "0"*64})
        for queue in queues:
            self.assertIsNone(await asyncio.wait_for(queue.get(), 2))
        self.assertEqual(self.api.directory.store.db.execute("SELECT count(*) FROM match_results").fetchone()[0], 0)

    async def test_disconnect_during_review_records_no_result(self):
        _, descriptors, sockets, queues, host = await self.pair()
        for socket in sockets:
            await socket.send_json({"loaded": "1234567890abcdef:000001b"})
        for queue in queues:
            await asyncio.wait_for(queue.get(), 2)
        h = next(i for i, row in enumerate(descriptors) if row["slot"] == host)
        await sockets[h].send_json({"postgame": self.report(descriptors).hex()})
        for queue in queues:
            await asyncio.wait_for(queue.get(), 2)
        await sockets[1-h].close()
        self.assertIsNone(await asyncio.wait_for(queues[h].get(), 2))
        self.assertEqual(self.api.directory.store.db.execute("SELECT count(*) FROM match_results").fetchone()[0], 0)

    async def test_team_report_admission_signed_totals_and_tied_standings(self):
        self.api.core.playlists["ranked"] = replace(self.api.core.playlists["ranked"], team_size=1)
        _, descriptors, sockets, queues, host = await self.pair()
        for socket in sockets:
            await socket.send_json({"loaded": "1234567890abcdef:000001c"})
        for queue in queues:
            await asyncio.wait_for(queue.get(), 2)
        raw = self.report(descriptors)
        raw[0x13] = raw[0x68] = 1
        for slot, row in enumerate(descriptors[0]["players"]):
            at = 0x370 + slot*0x94
            raw[at+0x88], raw[at+0x90] = row["team"], slot
            struct.pack_into("<h", raw, at+0x92, slot+3)
            at = 0xcb0 + row["team"]*0x18
            raw[at] = 1
            struct.pack_into("<h", raw, at+2, -4)
        room = self.api.transport.rooms[descriptors[0]["id"]]
        from multiplayer.master_server.protocol import Problem
        for at, value in ((0x3f8, 7), (0xcb0+8*0x18, 1), (0xcb1, 2)):
            forged = raw.copy()
            forged[at] = value
            with self.assertRaises(Problem):
                self.api.results.validate(room, forged.hex())
        h = next(i for i, row in enumerate(descriptors) if row["slot"] == host)
        await sockets[h].send_json({"postgame": raw.hex()})
        for queue in queues:
            self.assertIn("postgame", await asyncio.wait_for(queue.get(), 2))
        digest = hashlib.sha256(raw).hexdigest()
        for socket in sockets:
            await socket.send_json({"confirm_result": digest})
        for queue in queues:
            self.assertEqual(await asyncio.wait_for(queue.get(), 2), {"completed": digest})
            self.assertEqual(await asyncio.wait_for(queue.get(), 2), {"voice_host": host, "voice_members": 3})
        db = self.api.directory.store.db
        self.assertEqual([tuple(r) for r in db.execute("SELECT team,placement,score FROM match_result_teams ORDER BY team")],
                         [(0, 0, -4), (1, 0, -4)])
        self.assertEqual({r[0] for r in db.execute("SELECT placement FROM match_result_players")}, {0})
        self.assertEqual({r[0] for r in db.execute("SELECT score FROM match_result_players")}, {3, 4})
        self.assertEqual({r[0] for r in db.execute("SELECT individual_placement FROM match_result_memberships")}, {0, 1})

    async def test_client_cannot_finalize_or_forge_postgame(self):
        _, descriptors, sockets, queues, host = await self.pair()
        for socket in sockets:
            await socket.send_json({"loaded": "1234567890abcdef:000001b"})
        for queue in queues:
            await asyncio.wait_for(queue.get(), 2)
        c = next(i for i, row in enumerate(descriptors) if row["slot"] != host)
        await sockets[c].send_json({"postgame": self.report(descriptors).hex()})
        for queue in queues:
            self.assertIsNone(await asyncio.wait_for(queue.get(), 2))

    async def test_forged_postgame_identity_is_rejected(self):
        _, descriptors, sockets, queues, host = await self.pair()
        for socket in sockets:
            await socket.send_json({"loaded": "1234567890abcdef:000001b"})
        for queue in queues:
            await asyncio.wait_for(queue.get(), 2)
        h = next(i for i, row in enumerate(descriptors) if row["slot"] == host)
        raw = self.report(descriptors)
        raw[0x372] ^= 1
        await sockets[h].send_json({"postgame": raw.hex()})
        for queue in queues:
            self.assertIsNone(await asyncio.wait_for(queue.get(), 2))

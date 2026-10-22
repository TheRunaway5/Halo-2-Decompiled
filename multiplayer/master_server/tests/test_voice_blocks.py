import asyncio
import unittest
from dataclasses import replace
from . import test_clans_http as clans
from . import test_match_transport as match
from .voice_signing import sign_voice


def numeric_owners(directory):
    owners = {name: str(i) * 18 for i, name in enumerate(("one", "two", "three", "four"), 1)}
    with directory.store.db:
        for name, subject in owners.items():
            directory.store.db.execute("UPDATE players SET subject=? WHERE subject LIKE ?", (subject, "%-" + name))
    async def verify(session, token, application):
        from multiplayer.master_server.protocol import require
        require(token in owners, "Invalid Discord authorization", 401)
        return "discord", owners[token]
    directory.verify_owner = verify
    directory.voice_blocks.subjects.clear() # Fixture subjects changed; real ones are immutable.
    return owners


class VoiceBlockTests(unittest.IsolatedAsyncioTestCase):
    start = clans.ClansHttpTests.start
    login = clans.ClansHttpTests.login
    asyncSetUp = clans.ClansHttpTests.asyncSetUp

    async def test_private_full_replacement_owner_binding_and_persistence(self):
        directory = next(iter(self.app.values()))
        owners = numeric_owners(directory)
        path = "/v1/account/voice-blocks"
        response = await self.client.put(path, json={"blocked": [owners["two"]]})
        self.assertEqual(response.status, 401)
        for body in ({"blocked": [owners["two"]], "player": "forged"}, {"blocked": [1]},
                     {"blocked": ["not-a-subject"]}, {"blocked": [owners["two"]] * 2},
                     {"blocked": [str(i).zfill(18) for i in range(2001)]}):
            response = await self.client.put(path, headers={"Authorization": "Bearer one"}, json=body)
            self.assertIn(response.status, (400, 413))
        who = dict((subject, player) for player, subject in directory.store.db.execute("SELECT id,subject FROM players"))
        response = await self.client.put(path, headers={"Authorization": "Bearer one"},
                                         json={"blocked": [owners["two"], "999999999999999999", owners["one"]]})
        self.assertEqual(response.status, 200, await response.text())
        self.assertEqual(await response.json(), {"state": "updated"})
        self.assertFalse(directory.voice_blocks.allowed(who[owners["one"]], who[owners["two"]]))
        self.assertFalse(directory.voice_blocks.allowed(who[owners["two"]], who[owners["one"]]))
        self.assertTrue(directory.voice_blocks.allowed(who[owners["one"]], who[owners["three"]]))
        from multiplayer.master_server.voice_blocks import VoiceBlocks
        restored = VoiceBlocks(directory)
        self.assertFalse(restored.allowed(who[owners["one"]], who[owners["two"]]))
        self.assertNotIn(owners["one"], restored.blocks[who[owners["one"]]])
        # Another account can replace only its own preferences, never A's.
        await self.client.put(path, headers={"Authorization": "Bearer two"}, json={"blocked": []})
        self.assertFalse(directory.voice_blocks.allowed(who[owners["one"]], who[owners["two"]]))
        for method in (self.client.get, self.client.delete):
            self.assertEqual((await method(path, headers={"Authorization": "Bearer one"})).status, 405)
        response = await self.client.put(path, headers={"Authorization": "Bearer one"}, json={"blocked": []})
        self.assertEqual(await response.json(), {"state": "updated"})
        self.assertTrue(directory.voice_blocks.allowed(who[owners["one"]], who[owners["two"]]))


class MatchVoiceBlockTests(unittest.IsolatedAsyncioTestCase):
    asyncSetUp = match.MatchTransportTests.asyncSetUp
    session = match.MatchTransportTests.session
    enqueue = match.MatchTransportTests.enqueue
    pair = match.MatchTransportTests.pair

    async def test_host_and_nonhost_blocks_preserve_third_party_relay_and_postgame(self):
        self.api.entries = tuple(replace(e, policy=replace(e.policy, minimum_players=3, maximum_players=3)) for e in self.api.entries)
        self.api.core.playlists = {e.policy.id: e.policy for e in self.api.entries}
        _, descriptors, sockets, queues, host = await self.pair(("one", "two", "three"))
        directory = self.api.directory
        owners = numeric_owners(directory)
        names = ("one", "two", "three")
        for socket in sockets:
            await socket.send_json({"loaded": "1234567890abcdef:000001c"})
        for queue in queues:
            self.assertEqual((await asyncio.wait_for(queue.get(), 2))["phase"], "running")
        h = next(i for i, d in enumerate(descriptors) if d["slot"] == host)
        c, d = [i for i in range(3) if i != h]
        room = self.api.transport.rooms[descriptors[0]['id']]
        async def blocks(actor, target):
            response = await self.client.put("/v1/account/voice-blocks", headers={"Authorization": "Bearer " + names[actor]},
                json={"blocked": [] if target is None else [owners[names[target]]]})
            self.assertEqual(response.status, 200, await response.text())
        async def mask(i, value):
            self.assertEqual(await asyncio.wait_for(queues[i].get(), 2), {"voice_blocked": value})
        def audio(source):
            return sign_voice(room, b"H2N2\x1c\0\x06\0" + bytes([descriptors[source]["slot"]]) + b"\0" * 14)
        await blocks(h, c)
        await mask(h, 1 << descriptors[c]["slot"])
        await mask(c, 1 << host)
        # A blocked microphone still reaches the host as relay data for D;
        # the separate authoritative mask prevents local host playback.
        packet = audio(c)
        await sockets[c].send_bytes(bytes([host]) + packet)
        self.assertEqual(await asyncio.wait_for(queues[h].get(), 2), bytes([descriptors[c]["slot"]]) + packet)
        await sockets[h].send_bytes(bytes([descriptors[d]["slot"]]) + packet)
        self.assertEqual(await asyncio.wait_for(queues[d].get(), 2), bytes([host]) + packet)
        await sockets[h].send_bytes(bytes([descriptors[c]["slot"]]) + audio(h))
        with self.assertRaises(asyncio.TimeoutError):
            await asyncio.wait_for(queues[c].get(), .1)
        # Manual native preferences cannot clear the server's relationship.
        preferences = b"H2N2\x1c\0\x06\x01\x01\0\0"
        await sockets[c].send_bytes(bytes([host]) + preferences)
        await asyncio.wait_for(queues[h].get(), 2)
        await sockets[h].send_bytes(bytes([descriptors[c]["slot"]]) + audio(h))
        with self.assertRaises(asyncio.TimeoutError):
            await asyncio.wait_for(queues[c].get(), .1)
        await blocks(h, None)
        await mask(h, 0); await mask(c, 0)
        await sockets[h].send_bytes(bytes([descriptors[c]["slot"]]) + audio(h))
        self.assertEqual(await asyncio.wait_for(queues[c].get(), 2), bytes([host]) + audio(h))
        # Postgame uses the same private pair filter; two nonhosts can block
        # each other without blocking the host or the simulation channel.
        room = directory.match_transport.rooms[descriptors[0]["id"]]
        room.phase = "postgame"
        await blocks(c, d)
        await mask(c, 1 << descriptors[d]["slot"]); await mask(d, 1 << descriptors[c]["slot"])
        for source, listener in ((c, d), (d, c)):
            await sockets[h].send_bytes(bytes([descriptors[listener]["slot"]]) + audio(source))
            with self.assertRaises(asyncio.TimeoutError):
                await asyncio.wait_for(queues[listener].get(), .1)

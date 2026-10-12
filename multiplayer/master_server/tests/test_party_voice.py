import asyncio
import unittest
import uuid
from aiohttp import WSMsgType, WSServerHandshakeError
from . import test_parties_http as parties


class PartyVoiceTests(unittest.IsolatedAsyncioTestCase):
    start = parties.PartiesHttpTests.start
    login = parties.PartiesHttpTests.login
    asyncSetUp = parties.PartiesHttpTests.asyncSetUp
    view = parties.PartiesHttpTests.view
    command = parties.PartiesHttpTests.command
    join = parties.PartiesHttpTests.join

    async def socket(self, headers):
        socket = await self.client.ws_connect("/v1/party/voice", headers=headers)
        self.addAsyncCleanup(socket.close)
        return socket

    async def test_accepted_membership_identity_scope_transfer_and_revocation(self):
        for headers, status in (({}, 401), (self.a, 403)):
            with self.assertRaises(WSServerHandshakeError) as denied:
                await self.socket(headers)
            self.assertEqual(denied.exception.status, status)
        await self.command(self.a, "create")
        await self.command(self.a, "invite", username="pilot_two")
        with self.assertRaises(WSServerHandshakeError) as denied:
            await self.socket(self.b)  # An invitation is not membership.
        self.assertEqual(denied.exception.status, 403)
        invitation = (await self.view(self.b))["invitations"][0]
        joined = await self.command(self.b, "accept", target=invitation["id"], version=invitation["version"])
        await self.command(self.c, "create")
        a, b, c = await self.socket(self.a), await self.socket(self.b), await self.socket(self.c)
        packet = b"\x01\0\0\0\x01\x78\0"
        await a.send_bytes(packet)
        received = await b.receive(timeout=2)
        self.assertEqual(received.type, WSMsgType.BINARY)
        leader = next(m for m in joined["party"]["members"] if m["leader"])
        self.assertEqual(received.data[1:17], uuid.UUID(leader["handle"]).bytes)
        self.assertEqual(received.data[17:], packet[1:])
        private = {r[0] for r in self.directory.store.db.execute("SELECT id FROM players")}
        self.assertNotIn(str(uuid.UUID(bytes=received.data[1:17])), private)
        with self.assertRaises(asyncio.TimeoutError):
            await c.receive(timeout=.05)
        member = next(m for m in joined["party"]["members"] if m["self"])
        await self.command(self.a, "transfer", target=member["handle"])
        await b.send_bytes(packet)
        self.assertEqual((await a.receive(timeout=2)).type, WSMsgType.BINARY)
        await self.command(self.b, "remove", target=leader["handle"])
        self.assertEqual((await a.receive(timeout=2)).type, WSMsgType.CLOSE)
        await self.client.delete("/v1/account/session", headers=self.b)
        self.assertEqual((await b.receive(timeout=2)).type, WSMsgType.CLOSE)

    async def test_active_gameplay_ends_passive_party_channel(self):
        await self.command(self.a, "create")
        a = await self.socket(self.a)
        who = self.parties.party(next(iter(self.directory.clans.sessions.values())).player)
        self.assertIsNotNone(who)
        from unittest.mock import patch
        with patch.object(self.parties, "playing", return_value=True):
            self.assertEqual((await a.receive(timeout=2)).type, WSMsgType.CLOSE)
            with self.assertRaises(WSServerHandshakeError) as denied:
                await self.socket(self.a)
            self.assertEqual(denied.exception.status, 403)

    async def test_discord_blocks_silence_both_directions_and_preserve_other_listeners(self):
        from .test_voice_blocks import numeric_owners
        await self.command(self.a, "create")
        await self.join(self.a, self.b, "pilot_two"); await self.join(self.a, self.c, "pilot_three")
        owners = numeric_owners(self.directory)
        a, b, c = await self.socket(self.a), await self.socket(self.b), await self.socket(self.c)
        response = await self.client.put("/v1/account/voice-blocks", headers={"Authorization": "Bearer one"}, json={"blocked": [owners["two"]]})
        self.assertEqual(response.status, 200, await response.text())
        packet = b"\x01\0\0\0\x01\x78\0"
        for speaker, silenced in ((a,b), (b,a)):
            await speaker.send_bytes(packet)
            self.assertEqual((await c.receive(timeout=2)).type, WSMsgType.BINARY)
            with self.assertRaises(asyncio.TimeoutError):
                await silenced.receive(timeout=.1)

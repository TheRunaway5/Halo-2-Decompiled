import asyncio
from aiohttp import WSMsgType, WSServerHandshakeError
from . import test_clans_http as social


class ClanVoiceTests(social.ClansHttpTests):
    # Reuse setup/helpers, not the inherited social test cases.
    async def socket(self, headers):
        ws = await self.client.ws_connect("/v1/clans/voice", headers=headers)
        self.addAsyncCleanup(ws.close)
        return ws

    async def test_voice_identity_scope_and_revocation(self):
        with self.assertRaises(WSServerHandshakeError) as denied:
            await self.socket({})
        self.assertEqual(denied.exception.status, 401)
        with self.assertRaises(WSServerHandshakeError) as denied:
            await self.socket(self.a)
        self.assertEqual(denied.exception.status, 403)
        await self.make_clan()
        view = await self.join(self.b, "pilot_two")
        await self.command(self.c, "create", name="Separate Clan")
        a, b, c = await self.socket(self.a), await self.socket(self.b), await self.socket(self.c)
        packet = bytes([1, 0, 0, 0, 1, 0x78, 0])
        await a.send_bytes(packet)
        message = await b.receive(timeout=2)
        self.assertEqual(message.type, WSMsgType.BINARY)
        self.assertEqual(message.data[:1], b"\x01")
        self.assertEqual(message.data[17:], packet[1:])
        self.assertNotIn(b"private-discord", message.data)
        with self.assertRaises(asyncio.TimeoutError):
            await c.receive(timeout=.05)
        member = next(m["id"] for m in view["clan"]["members"] if m["self"])
        await self.command(self.a, "remove", target=member)
        self.assertEqual((await b.receive(timeout=2)).type, WSMsgType.CLOSE)
        await self.client.delete("/v1/account/session", headers=self.a)
        self.assertEqual((await a.receive(timeout=2)).type, WSMsgType.CLOSE)

    async def test_voice_replacement_expiry_and_invalid_frames(self):
        await self.make_clan()
        old = await self.socket(self.a)
        current = await self.socket(self.a)
        self.assertEqual((await old.receive(timeout=2)).type, WSMsgType.CLOSE)
        self.assertFalse(current.closed)
        await current.send_str("identity spoof")
        self.assertEqual((await current.receive(timeout=2)).type, WSMsgType.CLOSE)
        current = await self.socket(self.a)
        self.api.sessions.clear()
        self.assertEqual((await current.receive(timeout=2)).type, WSMsgType.CLOSE)

    async def test_discord_blocks_silence_both_directions_without_affecting_third_member(self):
        from .test_voice_blocks import numeric_owners
        await self.make_clan()
        await self.join(self.b, "pilot_two"); await self.join(self.c, "pilot_three")
        directory = self.api.directory
        owners = numeric_owners(directory)
        a, b, c = await self.socket(self.a), await self.socket(self.b), await self.socket(self.c)
        response = await self.client.put("/v1/account/voice-blocks", headers={"Authorization": "Bearer one"}, json={"blocked": [owners["two"]]})
        self.assertEqual(response.status, 200, await response.text())
        packet = b"\x01\0\0\0\x01\x78\0"
        for speaker, silenced in ((a,b), (b,a)):
            await speaker.send_bytes(packet)
            self.assertEqual((await c.receive(timeout=2)).type, WSMsgType.BINARY)
            with self.assertRaises(asyncio.TimeoutError):
                await silenced.receive(timeout=.1)
        await self.client.put("/v1/account/voice-blocks", headers={"Authorization": "Bearer one"}, json={"blocked": []})
        await a.send_bytes(b"\x01\0\0\0\x02\x78\0")
        self.assertEqual((await b.receive(timeout=2)).type, WSMsgType.BINARY)

    async def test_voice_replay_oversize_and_rate(self):
        await self.make_clan()
        for packets in ((b"\x02\0\0\0\0a",), (b"x" * 262,),
                        (b"\x01\0\0\0\1a",) * 2,
                        tuple(b"\x01" + i.to_bytes(4, "big") + b"a" for i in range(20))):
            current = await self.socket(self.a)
            for packet in packets:
                await current.send_bytes(packet)
            self.assertEqual((await current.receive(timeout=2)).type, WSMsgType.CLOSE)


# The social suite is already run independently; this class adds only voice.
for _name in list(vars(social.ClansHttpTests)):
    if _name.startswith("test_"):
        setattr(ClanVoiceTests, _name, None)

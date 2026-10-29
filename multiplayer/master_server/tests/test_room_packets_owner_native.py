"""Actual relay admission and immutable Rust packet authority."""
import asyncio
import unittest
from . import test_ranked_results as baseline


class RoomPacketsOwnerTests(unittest.IsolatedAsyncioTestCase):
    asyncSetUp = baseline.RankedResultsTests.asyncSetUp
    session = baseline.RankedResultsTests.session
    enqueue = baseline.RankedResultsTests.enqueue
    pair = baseline.RankedResultsTests.pair
    running = baseline.RankedResultsTests.running

    async def test_loaded_native_protocol_is_bound_for_legacy_relay_too(self):
        _, descriptors, sockets, queues, host = await self.running()
        client = next(i for i,d in enumerate(descriptors) if d['slot'] != host)
        server = 1-client
        await sockets[client].send_bytes(bytes([host])+b'H2N2\x1d\0\x02'+b'x'*20)
        value = await asyncio.wait_for(queues[server].get(),2)
        self.assertIsNone(value,'Relay accepted a native protocol which differs from the admitted loaded world')

    async def test_python_roster_aliases_cannot_reassign_packet_or_voice_authority(self):
        from .voice_signing import sign_voice
        _, descriptors, sockets, queues, host = await self.running()
        client = next(i for i,d in enumerate(descriptors) if d['slot'] != host)
        server = 1-client
        room = self.api.transport.rooms[descriptors[0]['id']]
        source = descriptors[client]['slot']
        audio = sign_voice(room,b'H2N2\x1c\0\x06\0'+bytes([source])+b'\0'*14)
        room.players[:] = [('forged-player','Forged roster',0)]
        room.owners.clear()
        room.guests = frozenset({'forged-player'})
        packet = b'H2N2\x1c\0\x02'+b'x'*20
        await sockets[client].send_bytes(bytes([host])+packet)
        self.assertEqual(await asyncio.wait_for(queues[server].get(),2),bytes([source])+packet)
        await sockets[client].send_bytes(bytes([host])+audio)
        self.assertEqual(await asyncio.wait_for(queues[server].get(),2),bytes([source])+audio)


from . import test_couch_online as couch
from . import test_match_transport as transport


class CouchVoicePrivacyOwnerTests(unittest.IsolatedAsyncioTestCase):
    asyncSetUp = couch.CouchOnlineTests.asyncSetUp
    catalog_fixture = staticmethod(couch.catalog)
    session = couch.CouchOnlineTests.session
    enqueue = couch.CouchOnlineTests.enqueue
    pair = transport.MatchTransportTests.pair

    async def test_native_privacy_joins_signed_locals_and_inherits_guest_subjects(self):
        self.local_rosters = {'one':[
            {'screen_name':'Signed Local','discord_token':'three'},
            {'screen_name':'Guest 3'},{'screen_name':'Guest 4'}]}
        _, descriptors, _, _, _ = await self.pair()
        room = self.api.transport.rooms[descriptors[0]['id']]
        owner = room.players[descriptors[0]['slot']][0]
        other = room.players[descriptors[1]['slot']][0]
        signed = next(who for who,_,_ in room.players if who != owner and
                      room.owners[who] == owner and who not in room.guests)
        signed_slot = room.index(signed)
        seen = set()
        def allowed(a,b):
            seen.add((a,b))
            return signed not in (a,b)
        room.api.directory.voice_blocks.allowed = allowed
        expected = sum(1<<i for i,(who,_,_) in enumerate(room.players) if room.owners[who] == other)
        self.assertEqual(room.voice_block_mask(owner),expected)
        self.assertEqual(room.voice_block_mask(other),1<<signed_slot)
        self.assertEqual(seen,{(owner,other),(signed,other),(other,owner),(other,signed)})
        self.assertTrue(all(a not in room.guests and b not in room.guests for a,b in seen))

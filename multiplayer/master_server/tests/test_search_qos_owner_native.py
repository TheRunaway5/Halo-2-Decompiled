"""Native probe ownership through authenticated queue and socket admission."""
import asyncio
import unittest
from . import test_search_qos as baseline
from . import test_matchmaking_http as http


def catalog():
    value = http.catalog()
    value['playlists'][1].update(maximum_players=4,maximum_party=4)
    return value


class SearchQosOwnerTests(unittest.IsolatedAsyncioTestCase):
    catalog_fixture = staticmethod(catalog)
    asyncSetUp = baseline.SearchQosTests.asyncSetUp
    session = baseline.SearchQosTests.session
    enqueue = baseline.SearchQosTests.enqueue
    connect = baseline.SearchQosTests.connect
    receive = baseline.SearchQosTests.receive
    pair = baseline.SearchQosTests.pair

    async def test_probe_dispatch_wakes_each_actual_connection_without_polling(self):
        qos,_,ids,sockets=await self.pair()
        ordered=sorted(ids)
        peers=[qos.peers[who] for who in ordered]
        self.assertTrue(qos.request(*ids))
        self.assertTrue(peers[0].wakeup.is_set())
        self.assertFalse(peers[1].wakeup.is_set())
        first=await self.receive(sockets[0])
        qos.echo(ordered[0],first)
        # A synchronous native transition signals its actual IO destination;
        # delivery does not have to wait for the writer's periodic timer.
        self.assertTrue(peers[1].wakeup.is_set())
        second=await self.receive(sockets[1])
        qos.echo(ordered[1],second)
        self.assertTrue(peers[0].wakeup.is_set())

    async def test_guest_pending_uses_the_same_physical_pair_as_allocation(self):
        headers = [await self.session(compatibility='halo2-native-queue-v9',local_players=[dict(screen_name='Guest')]),
                   await self.session('two',compatibility='halo2-native-queue-v9')]
        qos = self.api.search_qos
        self.now = 10.
        qos.clock = lambda:self.now
        for header in headers:
            response = await self.enqueue(header,'casual')
            self.assertEqual(response.status,201,await response.text())
        opened=[await self.connect(header) for header in headers]
        owner = self.api.session(type('Request',(),{'headers':headers[0]})())[1]
        other = self.api.session(type('Request',(),{'headers':headers[1]})())[1]
        guest = next(seat.player for seat in owner.seats() if seat.guest)
        self.assertTrue(qos.request(guest,other.player))
        self.assertTrue(qos.pending(guest,other.player),'A real guest route probe disappeared when pending used logical seats')
        self.assertTrue(qos.pending(owner.player,other.player))
        self.assertTrue(qos.pending(other.player,guest))

        sockets=[socket for _,socket in sorted(zip((owner.player,other.player),opened))]
        for total in (30,90,50):
            first=await self.receive(sockets[0])
            start=qos.probes[tuple(sorted((owner.player,other.player)))].started
            self.now=start+.01
            await sockets[0].send_json({'echo':first})
            second=await self.receive(sockets[1])
            self.now=start+(total+.1)/1000
            await sockets[1].send_json({'echo':second})
        async with asyncio.timeout(1):
            while qos.measurement(guest,other.player) is None:await asyncio.sleep(.001)
        self.assertEqual(qos.measurement(guest,other.player).milliseconds,50)
        self.assertEqual(qos.measurement(other.player,guest),qos.measurement(owner.player,other.player))

    async def test_late_retirement_and_reader_detach_cannot_revoke_a_fresh_connection(self):
        header=await self.session()
        first=await self.connect(header)
        qos=self.api.search_qos
        old=next(iter(self.api.sessions.values()))
        connection=qos.peers[old.player].connection
        self.assertEqual((await self.client.delete('/v1/matchmaking/session',headers=header)).status,200)
        replacement=await self.session()
        second=await self.connect(replacement)
        qos.discard(old.player,generation=old.generation)
        self.assertFalse(qos._call('detach',player=old.player,connection=connection))
        self.assertTrue(qos.ready(old.player))
        self.assertNotEqual(qos.peers[old.player].connection,connection)
        self.assertFalse(second.closed)

    async def test_expired_heartbeat_cannot_be_renewed_by_a_late_pong(self):
        header=await self.session()
        qos=self.api.search_qos
        self.now=10.
        qos.clock=lambda:self.now
        socket=await self.connect(header)
        value=await asyncio.wait_for(socket.receive_json(),1)
        self.assertEqual(set(value),{'ping'})
        self.now=25.001
        await socket.send_json({'pong':value['ping']})
        async with asyncio.timeout(1):
            async for _ in socket:pass
        self.assertFalse(qos.peers)

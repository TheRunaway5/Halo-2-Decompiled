import asyncio
import contextlib
import hashlib
import json
import unittest
from aiohttp import WSMsgType, WSServerHandshakeError
from . import test_matchmaking_http as baseline


class DirectPeerHttpTests(unittest.IsolatedAsyncioTestCase):
    asyncSetUp = baseline.MatchmakingHttpTests.asyncSetUp
    enqueue = baseline.MatchmakingHttpTests.enqueue

    async def session(self, owner):
        response = await self.client.post('/v1/matchmaking/session',
            headers={'Authorization': 'Bearer ' + owner},
            json=dict(screen_name='Same chosen name', maps=['lockout'], language=0,
                      compatibility='halo2-native-queue-v12'))
        self.assertEqual(response.status, 201, await response.text())
        return {'Authorization': 'Bearer ' + (await response.json())['token']}

    async def descriptors(self):
        headers = [await self.session(owner) for owner in ('one', 'two')]
        for header in headers:
            self.assertEqual((await self.enqueue(header)).status, 201)
        rows = []
        for header in headers:
            response = await self.client.get('/v1/matchmaking/transport', headers=header)
            self.assertEqual(response.status, 200, await response.text())
            rows.append(await response.json())
        return rows

    async def connect(self, descriptor):
        socket = await self.client.ws_connect('/v1/matches/' + descriptor['id'] + '/signal',
            headers={'Authorization': 'Bearer ' + descriptor['token']})
        queue = asyncio.Queue()
        async def read():
            async for message in socket:
                if message.type == WSMsgType.TEXT:
                    value = json.loads(message.data)
                    if 'ping' in value:
                        await socket.send_json({'pong': value['ping']})
                    else:
                        await queue.put(value)
                else:
                    self.assertNotEqual(message.type, WSMsgType.BINARY, 'Coordinator relayed media')
            await queue.put(None)
        task = asyncio.create_task(read())
        async def close():
            await socket.close()
            task.cancel()
            with contextlib.suppress(asyncio.CancelledError):
                await task
        self.addAsyncCleanup(close)
        return socket, queue

    async def next(self, queue, key, value=None):
        async with asyncio.timeout(4):
            while True:
                result = await queue.get()
                if result is None or (key in result and (value is None or result[key] == value)):
                    return result

    async def test_authenticated_signaling_buffers_and_direct_admission(self):
        descriptors = await self.descriptors()
        self.assertTrue(all(d['transport'] == dict(kind='direct-udp-v1', stun_servers=[]) for d in descriptors))
        self.assertNotIn('private-subject', json.dumps(descriptors))
        with self.assertRaises(WSServerHandshakeError) as failure:
            await self.client.ws_connect('/v1/matches/' + descriptors[0]['id'] + '/relay',
                headers={'Authorization': 'Bearer ' + descriptors[0]['token']})
        self.assertEqual(failure.exception.status, 409)
        low, high = sorted(descriptors, key=lambda d: d['slot'])
        a, qa = await self.connect(low)
        signal = dict(peer=high['slot'], type='offer', payload='fixture SDP only', mid='')
        await a.send_json({'peer_signal': signal})
        b, qb = await self.connect(high)
        value = await self.next(qb, 'peer_signal')
        self.assertEqual(value['peer_signal'], dict(signal, peer=low['slot']))
        room = self.api.transport.rooms[low['id']]
        async with asyncio.timeout(3):
            while any(len(room.samples.get(p, ())) < 3 for p in room.tokens):
                await asyncio.sleep(.01)
        self.assertEqual(room.phase, 'connecting', 'Master RTT alone admitted direct gameplay')
        await a.send_json({'peer_ready': [dict(peer=high['slot'], rtt_ms=30)]})
        await b.send_json({'peer_ready': [dict(peer=low['slot'], rtt_ms=40)]})
        for queue in (qa, qb):
            self.assertEqual((await self.next(queue, 'phase', 'loading'))['phase'], 'loading')
        for socket in (a, b):
            await socket.send_json({'loaded': '1234567890abcdef:0000042'})
        for queue in (qa, qb):
            self.assertEqual((await self.next(queue, 'phase', 'running'))['phase'], 'running')
        # A late ICE candidate is normal even after the fastest path connected.
        await a.send_json({'peer_signal': dict(peer=high['slot'], type='candidate',
            payload='candidate:1 1 UDP 1 127.0.0.1 2000 typ host', mid='0')})
        self.assertEqual((await self.next(qb, 'peer_signal'))['peer_signal']['peer'], low['slot'])
        self.assertFalse(room.closed)
        # Neither native game packets nor Opus frames have a master relay path.
        await a.send_bytes(bytes([high['slot']]) + b'H2N2\x42\0\x06' + b'x' * 32)
        self.assertIsNone(await self.next(qb, 'closed'))

    async def test_readiness_cannot_substitute_an_unadmitted_endpoint(self):
        descriptors = await self.descriptors()
        a, qa = await self.connect(descriptors[0])
        b, qb = await self.connect(descriptors[1])
        await a.send_json({'peer_ready': [dict(peer=descriptors[0]['slot'], rtt_ms=0)]})
        self.assertIsNone(await self.next(qb, 'closed'))

    async def test_signaling_cannot_assert_a_source_identity(self):
        descriptors = await self.descriptors()
        a, qa = await self.connect(descriptors[0])
        b, qb = await self.connect(descriptors[1])
        await a.send_json({'peer_signal': dict(peer=descriptors[1]['slot'], source=descriptors[1]['slot'],
                                              type='candidate', payload='forged', mid='0')})
        self.assertIsNone(await self.next(qb, 'peer_signal'))

    async def test_master_reconnect_preserves_room_and_result_retry_is_idempotent(self):
        from .test_match_transport import MatchTransportTests
        descriptors = await self.descriptors()
        connections = [await self.connect(d) for d in descriptors]
        for i, (socket, queue) in enumerate(connections):
            await socket.send_json({'peer_ready': [dict(peer=descriptors[1-i]['slot'], rtt_ms=20)]})
        controls = [await self.next(q, 'phase', 'loading') for _, q in connections]
        for socket, queue in connections:
            await socket.send_json({'loaded': '1234567890abcdef:0000042'})
        for socket, queue in connections:
            await self.next(queue, 'phase', 'running')
        room = self.api.transport.rooms[descriptors[0]['id']]
        await connections[0][0].close(code=1001)
        async with asyncio.timeout(3):
            while len(room.peers) != 1:
                await asyncio.sleep(.01)
        self.assertFalse(room.closed)
        self.assertEqual(len(room.tokens), 2)
        connections[0] = await self.connect(descriptors[0])
        await self.next(connections[0][1], 'phase', 'running')
        host = next(i for i, d in enumerate(descriptors) if d['slot'] == controls[0]['host'])
        raw = MatchTransportTests.report(self, descriptors)
        await connections[host][0].send_json({'postgame': raw.hex()})
        for socket, queue in connections:
            self.assertEqual((await self.next(queue, 'postgame'))['postgame'], raw.hex())
        digest = hashlib.sha256(raw).hexdigest()
        for socket, queue in connections:
            await socket.send_json({'confirm_result': digest})
        for socket, queue in connections:
            await self.next(queue, 'completed')
        await connections[0][0].close(code=1001)
        connections[0] = await self.connect(descriptors[0])
        self.assertEqual((await self.next(connections[0][1], 'postgame'))['postgame'], raw.hex())
        await connections[0][0].send_json({'confirm_result': digest})
        self.assertEqual((await self.next(connections[0][1], 'completed'))['completed'], digest)
        self.assertEqual(self.api.directory.store.db.execute('SELECT count(*) FROM match_results').fetchone()[0], 1)


class DirectStarConnectivityTests(unittest.IsolatedAsyncioTestCase):
    asyncSetUp = baseline.MatchmakingHttpTests.asyncSetUp
    session = DirectPeerHttpTests.session
    enqueue = baseline.MatchmakingHttpTests.enqueue
    connect = DirectPeerHttpTests.connect
    next = DirectPeerHttpTests.next

    def catalog_fixture(self):
        value = baseline.catalog()
        for row in value['playlists']:
            row.update(minimum_players=3, maximum_players=3)
        return value

    async def test_nonhosts_need_not_reach_each_other_when_one_host_reaches_all(self):
        headers = [await self.session(owner) for owner in ('one', 'two', 'three')]
        for header in headers:
            self.assertEqual((await self.enqueue(header)).status, 201)
        descriptors = []
        for header in headers:
            response = await self.client.get('/v1/matchmaking/transport', headers=header)
            self.assertEqual(response.status, 200)
            descriptors.append(await response.json())
        sockets = [await self.connect(d) for d in descriptors]
        chosen = descriptors[0]['slot']
        for d, (socket, queue) in zip(descriptors, sockets):
            rows = [dict(peer=other['slot'], rtt_ms=20 if chosen in (d['slot'], other['slot']) else None)
                    for other in descriptors if other['slot'] != d['slot']]
            await socket.send_json({'peer_ready': rows})
        for socket, queue in sockets:
            self.assertEqual((await self.next(queue, 'phase', 'loading'))['host'], chosen)

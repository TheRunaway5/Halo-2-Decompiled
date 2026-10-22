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
        offender, rejected, survivor = (a, qa, b) if low['slot'] != room.host else (b, qb, a)
        await offender.send_bytes(bytes([room.host]) + b'H2N2\x42\0\x06' + b'x' * 32)
        self.assertIsNone(await self.next(rejected, 'closed'))
        self.assertFalse(room.closed)
        self.assertFalse(survivor.closed)
        self.assertEqual(len(room.tokens), 1)

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

    async def test_measured_upload_changes_physical_host_selection(self):
        descriptors = await self.descriptors()
        connections = [await self.connect(d) for d in descriptors]
        preferred = descriptors[1]['slot']
        for d, (socket, _) in zip(descriptors, connections):
            await socket.send_json({'peer_ready': [dict(peer=other['slot'], rtt_ms=30,
                upload_bytes_per_second=2_000_000 if d['slot'] == preferred else 100_000,
                loss_per_mille=0) for other in descriptors if other != d]})
        for _, queue in connections:
            self.assertEqual((await self.next(queue, 'phase', 'loading'))['host'], preferred)
        room = self.api.transport.rooms[descriptors[0]['id']]
        self.assertEqual(len(room.measured_hosts()), 2)

    async def test_missing_reciprocal_measurement_cannot_downgrade_host_policy(self):
        descriptors = await self.descriptors()
        connections = [await self.connect(d) for d in descriptors]
        await connections[0][0].send_json({'peer_ready': [dict(peer=descriptors[1]['slot'], rtt_ms=30,
            upload_bytes_per_second=100_000, loss_per_mille=0)]})
        await connections[1][0].send_json({'peer_ready': [dict(peer=descriptors[0]['slot'], rtt_ms=20)]})
        self.assertIsNone(await self.next(connections[0][1], 'phase', 'loading'))

    async def test_invalid_upload_measurement_is_rejected(self):
        descriptors = await self.descriptors()
        connections = [await self.connect(d) for d in descriptors]
        await connections[0][0].send_json({'peer_ready': [dict(peer=descriptors[1]['slot'], rtt_ms=30,
            upload_bytes_per_second=64 * 1024 * 1024 + 1, loss_per_mille=0)]})
        self.assertIsNone(await self.next(connections[1][1], 'phase', 'loading'))

    async def test_boolean_upload_measurement_is_rejected(self):
        descriptors = await self.descriptors()
        connections = [await self.connect(d) for d in descriptors]
        await connections[0][0].send_json({'peer_ready': [dict(peer=descriptors[1]['slot'], rtt_ms=30,
            upload_bytes_per_second=True, loss_per_mille=0)]})
        self.assertIsNone(await self.next(connections[1][1], 'phase', 'loading'))

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

    async def running_three(self):
        headers = [await self.session(owner) for owner in ('one', 'two', 'three')]
        for header in headers:
            self.assertEqual((await self.enqueue(header)).status, 201)
        descriptors = []
        for header in headers:
            response = await self.client.get('/v1/matchmaking/transport', headers=header)
            self.assertEqual(response.status, 200)
            descriptors.append(await response.json())
        connections = [await self.connect(d) for d in descriptors]
        chosen = descriptors[0]['slot']
        for d, (socket, queue) in zip(descriptors, connections):
            await socket.send_json({'peer_ready': [dict(peer=other['slot'],
                rtt_ms=20 if chosen in (d['slot'], other['slot']) else None)
                for other in descriptors if other['slot'] != d['slot']]})
        for socket, queue in connections:
            self.assertEqual((await self.next(queue, 'phase', 'loading'))['host'], chosen)
            await socket.send_json({'loaded': '1234567890abcdef:0000042'})
        for socket, queue in connections:
            await self.next(queue, 'phase', 'running')
        room = self.api.transport.rooms[descriptors[0]['id']]
        host = next(i for i, d in enumerate(descriptors) if d['slot'] == chosen)
        return room, descriptors, connections, host

    async def test_nonhost_explicit_leave_preserves_play_but_cannot_supply_consent(self):
        from .test_match_transport import MatchTransportTests
        room, descriptors, connections, host = await self.running_three()
        leaving = next(i for i in range(3) if i != host)
        survivors = [i for i in range(3) if i != leaving]
        await connections[leaving][0].send_json({'leave': True})
        self.assertIsNone(await self.next(connections[leaving][1], 'closed'))
        self.assertFalse(room.closed)
        self.assertEqual(room.phase, 'running')
        self.assertEqual(room.host, descriptors[host]['slot'])
        self.assertEqual(len(room.tokens), 2)
        self.assertEqual(len(room.players), 3)
        self.assertEqual(room.departed_slots, 1 << descriptors[leaving]['slot'])
        await connections[host][0].send_json({'peer_signal': dict(peer=descriptors[leaving]['slot'],
            type='candidate', payload='candidate:1 1 UDP 1 127.0.0.1 2000 typ host', mid='0')})
        with self.assertRaises(WSServerHandshakeError) as failure:
            await self.client.ws_connect('/v1/matches/' + room.id + '/signal',
                headers={'Authorization': 'Bearer ' + descriptors[leaving]['token']})
        self.assertEqual(failure.exception.status, 401)
        raw = MatchTransportTests.report(self, descriptors)
        await connections[host][0].send_json({'postgame': raw.hex()})
        for i in survivors:
            self.assertEqual((await self.next(connections[i][1], 'postgame'))['postgame'], raw.hex())
            await connections[i][0].send_json({'confirm_result': hashlib.sha256(raw).hexdigest()})
        async with asyncio.timeout(3):
            while len(room.confirmations) != len(survivors):
                await asyncio.sleep(.005)
        departed_owner = room.players[descriptors[leaving]['slot']][0]
        self.assertNotIn(departed_owner, room.pending_signals)
        db = self.api.directory.store.db
        self.assertEqual(room.phase, 'reviewing')
        self.assertIsNone(db.execute('SELECT * FROM match_result_consensus WHERE match=?', (room.id,)).fetchone())
        self.assertEqual(db.execute('SELECT count(*) FROM match_results WHERE id=?', (room.id,)).fetchone()[0], 0)
        self.assertEqual(db.execute('SELECT count(*) FROM match_result_players WHERE match=?',
                                   (room.id,)).fetchone()[0], 0)

    async def test_completed_world_host_can_leave_during_result_review(self):
        from .test_match_transport import MatchTransportTests
        room, descriptors, connections, host = await self.running_three()
        survivors = [i for i in range(3) if i != host]
        raw = MatchTransportTests.report(self, descriptors)
        await connections[host][0].send_json({'postgame': raw.hex()})
        for socket, queue in connections:
            await self.next(queue, 'postgame')
        # Host departure may preserve a vote it actually cast for this digest.
        await connections[host][0].send_json({'confirm_result': hashlib.sha256(raw).hexdigest()})
        await connections[survivors[0]][0].send_json({'confirm_result': hashlib.sha256(raw).hexdigest()})
        await connections[host][0].send_json({'leave': True})
        self.assertIsNone(await self.next(connections[host][1], 'closed'))
        self.assertFalse(room.closed)
        self.assertEqual(room.phase, 'reviewing')
        for i in survivors:
            membership = await self.next(connections[i][1], 'voice_members')
            self.assertEqual(membership['voice_host'], min(descriptors[j]['slot'] for j in survivors))
            self.assertFalse(membership['voice_members'] & (1 << descriptors[host]['slot']))
        await connections[survivors[1]][0].send_json({'confirm_result': hashlib.sha256(raw).hexdigest()})
        for i in survivors:
            await self.next(connections[i][1], 'completed')
        self.assertEqual(room.phase, 'postgame')

    async def test_explicit_departure_revokes_an_inflight_master_reconnect(self):
        from unittest.mock import patch
        from aiohttp import web
        room, descriptors, connections, host = await self.running_three()
        leaving = next(i for i in range(3) if i != host)
        prepared, resume = asyncio.Event(), asyncio.Event()
        original = web.WebSocketResponse.prepare
        async def delayed_prepare(socket, request):
            first = not socket.prepared
            value = await original(socket, request)
            if first:
                prepared.set()
                await resume.wait()
            return value
        with patch.object(web.WebSocketResponse, 'prepare', delayed_prepare):
            pending = asyncio.create_task(self.client.ws_connect('/v1/matches/' + room.id + '/signal',
                headers={'Authorization': 'Bearer ' + descriptors[leaving]['token']}))
            await asyncio.wait_for(prepared.wait(), 3)
            replacement = await asyncio.wait_for(pending, 3)
            self.addAsyncCleanup(replacement.close)
            await connections[leaving][0].send_json({'leave': True})
            self.assertIsNone(await self.next(connections[leaving][1], 'closed'))
            resume.set()
            response = await replacement.receive(timeout=3)
            self.assertEqual(response.type, WSMsgType.CLOSE)
        owner = room.players[descriptors[leaving]['slot']][0]
        self.assertNotIn(owner, room.tokens)
        self.assertNotIn(owner, room.peers)
        self.assertFalse(room.closed)
        self.assertEqual(room.phase, 'running')

    async def test_running_host_departure_requires_complete_authority_handoff(self):
        room, descriptors, connections, host = await self.running_three()
        await connections[host][0].send_json({'leave': True})
        for socket, queue in connections:
            self.assertIsNone(await self.next(queue, 'closed'))
        self.assertTrue(room.closed)

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

"""EOS setup is control-only; pair keys bind EOS identities to admitted PCs."""
import unittest
from .test_direct_peer_http import DirectPeerHttpTests
from . import test_matchmaking_http as baseline


class EosPeerHttpTests(unittest.IsolatedAsyncioTestCase):
    asyncSetUp = baseline.MatchmakingHttpTests.asyncSetUp
    enqueue = baseline.MatchmakingHttpTests.enqueue
    descriptors = DirectPeerHttpTests.descriptors
    connect = DirectPeerHttpTests.connect
    next = DirectPeerHttpTests.next

    async def session(self, owner, compatibility='halo2-native-queue-v13'):
        response = await self.client.post('/v1/matchmaking/session',
            headers={'Authorization': 'Bearer ' + owner},
            json=dict(screen_name='EOS Pilot', maps=['lockout'], language=0, compatibility=compatibility))
        self.assertEqual(response.status, 201, await response.text())
        return {'Authorization': 'Bearer ' + (await response.json())['token']}

    async def test_pair_keys_are_shared_only_between_admitted_physical_endpoints(self):
        rows = await self.descriptors()
        a, b = rows
        self.assertEqual(a['transport']['kind'], 'eos-p2p-v1')
        self.assertEqual(a['transport']['socket'], b['transport']['socket'])
        self.assertEqual(len(a['transport']['socket']), 32)
        self.assertEqual(a['transport']['session'],dict(playlist='ranked', revision='lab-1',map='lockout',variant='Slayer'))
        self.assertEqual(a['transport']['peers'], [dict(slot=b['slot'], key=b['transport']['peers'][0]['key'])])
        self.assertEqual(len(a['transport']['peers'][0]['key']), 64)
        self.assertNotIn('secret', str(rows))
        room = self.api.transport.rooms[a['id']]
        self.assertGreater(room.deadline - room.created, 55)
        sa, qa = await self.connect(a)
        sb, qb = await self.connect(b)
        for row, socket in ((a, sa), (b, sb)):
            other = b if row is a else a
            await socket.send_json({'peer_signal': dict(peer=other['slot'], type='eos', payload=str(row['slot'])*32, mid='')})
        self.assertEqual((await self.next(qb, 'peer_signal'))['peer_signal']['peer'], a['slot'])
        self.assertEqual((await self.next(qa, 'peer_signal'))['peer_signal']['peer'], b['slot'])
        for row, socket in ((a, sa), (b, sb)):
            other = b if row is a else a
            await socket.send_json({'peer_ready': [dict(peer=other['slot'], rtt_ms=25)]})
        self.assertEqual((await self.next(qa, 'phase', 'loading'))['phase'], 'loading')
        self.assertEqual((await self.next(qb, 'phase', 'loading'))['phase'], 'loading')
        # The coordinator still refuses game/media payloads for EOS rooms.
        await sa.send_bytes(b'H2N2\0\0\6')
        self.assertIsNone(await self.next(qb, 'closed'))

    async def test_eos_room_rejects_legacy_ice_signals(self):
        rows = await self.descriptors()
        a, _ = await self.connect(rows[0])
        _, qb = await self.connect(rows[1])
        await a.send_json({'peer_signal': dict(peer=rows[1]['slot'], type='offer', payload='SDP', mid='')})
        self.assertIsNone(await self.next(qb, 'peer_signal'))

    async def test_eos_identity_cannot_change_after_publication(self):
        rows = await self.descriptors()
        a, _ = await self.connect(rows[0])
        _, qb = await self.connect(rows[1])
        signal = dict(peer=rows[1]['slot'], type='eos', payload='a'*32, mid='')
        await a.send_json({'peer_signal': signal})
        self.assertEqual((await self.next(qb, 'peer_signal'))['peer_signal']['payload'], 'a'*32)
        await a.send_json({'peer_signal': signal | dict(payload='b'*32)})
        self.assertIsNone(await self.next(qb, 'peer_signal'))

    async def test_eos_and_legacy_players_do_not_share_a_match(self):
        a = await self.session('one')
        b = await self.session('two', 'halo2-native-queue-v12')
        for header in (a, b): self.assertEqual((await self.enqueue(header)).status, 201)
        for header in (a, b):
            response = await self.client.get('/v1/matchmaking/transport', headers=header)
            self.assertNotEqual(response.status, 200)

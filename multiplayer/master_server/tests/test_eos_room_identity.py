"""Room capabilities authenticate EOS Connect without replacing social sessions.

The official SDK verifier remains the explicit boundary; these HTTP regressions
exercise actual room bearers, immutable binding, revocation and signal delivery.
"""
import base64
import json
import time
import unittest
from unittest.mock import AsyncMock, Mock, patch
from . import test_matchmaking_http as baseline
from . import test_direct_peer_http as direct
from . import test_eos_identity as identity_tests
from . import test_player_profile as profiles

decode = identity_tests.decode


def proof(puid, **claims):
    payload = base64.urlsafe_b64encode(json.dumps({'sub': puid} | claims).encode()).decode().rstrip('=')
    return 'header.' + payload + '.signature'


class EosRoomIdentityTests(unittest.IsolatedAsyncioTestCase):
    asyncSetUp = baseline.MatchmakingHttpTests.asyncSetUp
    enqueue = baseline.MatchmakingHttpTests.enqueue
    configure = identity_tests.EosIdentityTests.configure
    connect = direct.DirectPeerHttpTests.connect
    next = direct.DirectPeerHttpTests.next
    login = profiles.PlayerProfileTests.login

    async def session(self, owner, compatibility='halo2-native-queue-v14', **extra):
        response = await self.client.post('/v1/matchmaking/session', headers={'Authorization': 'Bearer ' + owner},
            json=dict(screen_name='Room Pilot ' + owner, maps=['lockout'], language=0,
                      compatibility=compatibility) | extra)
        self.assertEqual(response.status, 201, await response.text())
        return {'Authorization': 'Bearer ' + (await response.json())['token']}

    async def room(self, compatibility='halo2-native-queue-v14', playlist='ranked', first_extra=None):
        headers = [await self.session(owner, compatibility, **((first_extra or {}) if owner == 'one' else {}))
                   for owner in ('one', 'two')]
        for header in headers:
            self.assertEqual((await self.enqueue(header, playlist)).status, 201)
        rows = []
        for header in headers:
            response = await self.client.get('/v1/matchmaking/transport', headers=header)
            self.assertEqual(response.status, 200, await response.text())
            rows.append(await response.json())
        room = self.api.transport.rooms[rows[0]['id']]
        self.assertEqual(rows[0]['id'], rows[1]['id'])
        return room, rows

    @staticmethod
    def headers(row):
        return {'Authorization': 'Bearer ' + row['token']}

    @staticmethod
    def route(row, suffix):
        return '/v1/matches/' + row['id'] + '/' + suffix

    async def bind(self, identity, room, row, puid):
        who = room.players[row['slot']][0]
        identity.verify = AsyncMock(return_value=who)
        response = await self.client.post(self.route(row, 'eos-proof'), headers=self.headers(row), json={'token': proof(puid)})
        self.assertEqual(response.status, 200, await response.text())
        self.assertEqual(await response.json(), {'eos_product_user_id': puid})
        identity.verify.assert_awaited_once_with(puid, proof(puid), principal=who)
        return who

    async def test_v13_routes_refuse_account_transport_without_changing_legacy_signals(self):
        room, rows = await self.room('halo2-native-queue-v13')
        self.assertGreater(room.deadline - room.created, 59)
        self.assertLess(room.deadline - room.created, 61)
        self.configure()
        for suffix, body in [('eos-token', {}), ('eos-proof', {'token': proof('1' * 32)})]:
            response = await self.client.post(self.route(rows[0], suffix), headers=self.headers(rows[0]), json=body)
            self.assertEqual(response.status, 409)
        sa, _ = await self.connect(rows[0]); _, qb = await self.connect(rows[1])
        await sa.send_json({'peer_signal': dict(peer=rows[1]['slot'], type='eos', payload='a' * 32, mid='')})
        self.assertEqual((await self.next(qb, 'peer_signal'))['peer_signal']['payload'], 'a' * 32)

    async def test_room_issuer_uses_physical_owner_and_preserves_social_sessions(self):
        room, rows = await self.room(); self.configure()
        self.assertGreater(room.deadline - room.created, 119)
        self.assertLess(room.deadline - room.created, 121)
        social = [await self.login(owner) for owner in ('one', 'two')]
        before = dict(self.api.directory.clans.sessions)
        for row in rows:
            for _ in range(2):
                response = await self.client.post(self.route(row, 'eos-token'), headers=self.headers(row), json={})
                self.assertEqual(response.status, 200, await response.text())
                value = await response.json(); self.assertEqual(set(value), {'token', 'expires_in'})
                claims = json.loads(decode(value['token'].split('.')[1]))
                self.assertEqual(claims['sub'], room.players[row['slot']][0])
                self.assertEqual(claims['name'], room.players[row['slot']][1])
                self.assertEqual(value['expires_in'], 120); self.assertEqual(claims['exp'] - claims['iat'], 120)
        self.assertEqual(self.api.directory.clans.sessions, before)
        for headers in social:
            self.assertEqual((await self.client.get('/v1/account/profile', headers=headers)).status, 200)
        self.assertEqual(self.api.directory.store.db.execute('SELECT count(*) FROM eos_players').fetchone()[0], 0)

    async def test_room_bearer_and_strict_body_are_required(self):
        room, rows = await self.room(); identity = self.configure(); identity.verify = AsyncMock()
        a = rows[0]
        for headers in ({}, {'Authorization': 'Bearer wrong'}, {'Authorization': 'Bearer ' + 'x' * 128}):
            for suffix, body in [('eos-token', {}), ('eos-proof', {'token': proof('1' * 32)})]:
                self.assertEqual((await self.client.post(self.route(a, suffix), headers=headers, json=body)).status, 401)
        for body in ({'player': room.players[a['slot']][0]}, {'puid': '1' * 32}, [], {'token': 'unused'}):
            self.assertEqual((await self.client.post(self.route(a, 'eos-token'), headers=self.headers(a), json=body)).status, 400)
        for body in ({'puid': '1' * 32, 'token': proof('1' * 32)}, {'token': proof('1' * 32), 'player': room.players[a['slot']][0]}):
            self.assertEqual((await self.client.post(self.route(a, 'eos-proof'), headers=self.headers(a), json=body)).status, 400)
        identity.verify.assert_not_awaited()

    async def test_body_read_revalidates_revoked_capability_before_issuer_or_sdk(self):
        room, rows = await self.room(); identity = self.configure(); row = rows[0]
        who = room.players[row['slot']][0]; saved = room.tokens[who]
        original = self.api.directory.body
        async def revoke_after_body(request):
            body = await original(request)
            room.tokens.pop(who, None)
            return body
        identity.token = Mock(wraps=identity.token); identity.verify = AsyncMock()
        with patch.object(self.api.directory, 'body', revoke_after_body):
            response = await self.client.post(self.route(row, 'eos-token'), headers=self.headers(row), json={})
            self.assertEqual(response.status, 401)
            room.tokens[who] = saved
            response = await self.client.post(self.route(row, 'eos-proof'), headers=self.headers(row), json={'token': proof('1' * 32)})
            self.assertEqual(response.status, 401)
        identity.token.assert_not_called(); identity.verify.assert_not_awaited()
        room.tokens[who] = saved

    async def test_owner_rate_limit_bounds_signing_and_survives_room_token_rotation(self):
        room, rows = await self.room(); identity = self.configure(); row = rows[0]
        who = room.players[row['slot']][0]
        # Exhaust only the real owner bucket, leaving the IP bucket unaffected.
        for _ in range(120): self.api.directory.rate('eos-room:' + who, 120)
        identity.token = Mock(wraps=identity.token); identity.verify = AsyncMock()
        for suffix, body in [('eos-token', {}), ('eos-proof', {'token': proof('1' * 32)})]:
            response = await self.client.post(self.route(row, suffix), headers=self.headers(row), json=body)
            self.assertEqual(response.status, 429)
        room.tokens[who] = 'rotated-room-bearer'; rotated = row | {'token': 'rotated-room-bearer'}
        response = await self.client.post(self.route(rotated, 'eos-token'), headers=self.headers(rotated), json={})
        self.assertEqual(response.status, 429)
        identity.token.assert_not_called(); identity.verify.assert_not_awaited()

    async def test_proof_verifies_account_then_preserves_immutable_binding_and_revision(self):
        room, rows = await self.room(); identity = self.configure(); a = rows[0]
        who = room.players[a['slot']][0]; other = room.players[rows[1]['slot']][0]
        identity.verify = AsyncMock(return_value=other)
        response = await self.client.post(self.route(a, 'eos-proof'), headers=self.headers(a), json={'token': proof('1' * 32)})
        self.assertEqual(response.status, 403)
        db = self.api.directory.store.db
        self.assertEqual(db.execute('SELECT count(*) FROM eos_players').fetchone()[0], 0)
        await self.bind(identity, room, a, '1' * 32)
        revision = db.execute('SELECT revision FROM eos_profile_outbox WHERE player=?', (who,)).fetchone()[0]
        response = await self.client.post(self.route(a, 'eos-proof'), headers=self.headers(a), json={'token': proof('1' * 32)})
        self.assertEqual(response.status, 200)
        self.assertEqual(db.execute('SELECT revision FROM eos_profile_outbox WHERE player=?', (who,)).fetchone()[0], revision)
        self.assertEqual((await self.client.post(self.route(a, 'eos-proof'), headers=self.headers(a), json={'token': proof('2' * 32)})).status, 409)
        identity.verify.return_value = other
        self.assertEqual((await self.client.post(self.route(rows[1], 'eos-proof'), headers=self.headers(rows[1]), json={'token': proof('1' * 32)})).status, 409)
        self.assertEqual(db.execute('SELECT puid FROM eos_players WHERE player=?', (who,)).fetchone()[0], '1' * 32)

    async def test_malformed_or_duplicate_unverified_subject_never_reaches_sdk(self):
        _, rows = await self.room(); identity = self.configure(); identity.verify = AsyncMock()
        duplicate = base64.urlsafe_b64encode(b'{"sub":"11111111111111111111111111111111","sub":"22222222222222222222222222222222"}').decode().rstrip('=')
        for token in ('opaque', 'a..b', 'a.%%%.b', proof('bad'), proof(4), 'a.' + duplicate + '.b', 'a.' + 'a' * 8192 + '.b'):
            response = await self.client.post(self.route(rows[0], 'eos-proof'), headers=self.headers(rows[0]), json={'token': token})
            self.assertEqual(response.status, 400)
        identity.verify.assert_not_awaited()

    async def test_revoked_closed_expired_or_replaced_room_cannot_complete_late_proof(self):
        room, rows = await self.room(); identity = self.configure(); a = rows[0]
        who = room.players[a['slot']][0]; saved = room.tokens[who]
        async def revoke(*_, **kwargs):
            room.tokens.pop(who)
            return who
        identity.verify = revoke
        self.assertEqual((await self.client.post(self.route(a, 'eos-proof'), headers=self.headers(a), json={'token': proof('1' * 32)})).status, 401)
        room.tokens[who] = saved
        async def expire(*_, **kwargs):
            room.deadline = time.monotonic() - 1
            return who
        identity.verify = expire
        self.assertEqual((await self.client.post(self.route(a, 'eos-proof'), headers=self.headers(a), json={'token': proof('1' * 32)})).status, 404)
        room.deadline = time.monotonic() + 60
        async def close(*_, **kwargs):
            room.closed = True
            return who
        identity.verify = close
        self.assertEqual((await self.client.post(self.route(a, 'eos-proof'), headers=self.headers(a), json={'token': proof('1' * 32)})).status, 404)
        room.closed = False
        async def replace(*_, **kwargs):
            self.api.transport.rooms.pop(room.id)
            return who
        identity.verify = replace
        self.assertEqual((await self.client.post(self.route(a, 'eos-proof'), headers=self.headers(a), json={'token': proof('1' * 32)})).status, 404)
        self.api.transport.rooms[room.id] = room
        self.assertEqual(self.api.directory.store.db.execute('SELECT count(*) FROM eos_players').fetchone()[0], 0)

    async def test_v14_signals_bind_verified_account_puid_and_never_peer_supplied_identity(self):
        room, rows = await self.room(); identity = self.configure()
        for row in rows: await self.bind(identity, room, row, str(row['slot'] + 1) * 32)
        connections = [await self.connect(row) for row in rows]
        for i, row in enumerate(rows):
            await connections[i][0].send_json({'peer_signal': dict(peer=rows[1-i]['slot'], type='eos', payload=str(row['slot'] + 1) * 32, mid='')})
            value = (await self.next(connections[1-i][1], 'peer_signal'))['peer_signal']
            self.assertEqual(value['peer'], row['slot']); self.assertEqual(value['payload'], str(row['slot'] + 1) * 32)
        await connections[0][0].send_json({'peer_signal': dict(peer=rows[1]['slot'], type='eos', payload='f' * 32, mid='')})
        self.assertIsNone(await self.next(connections[1][1], 'peer_signal'))

    async def test_v14_signal_without_verified_binding_is_rejected(self):
        room, rows = await self.room(); self.configure()
        sa, _ = await self.connect(rows[0]); _, qb = await self.connect(rows[1])
        await sa.send_json({'peer_signal': dict(peer=rows[1]['slot'], type='eos', payload='a' * 32, mid='')})
        self.assertIsNone(await self.next(qb, 'peer_signal'))
        self.assertFalse(room.eos_users)

    async def test_v14_never_mixes_with_v13(self):
        headers = [await self.session('one'), await self.session('two', 'halo2-native-queue-v13')]
        for header in headers: self.assertEqual((await self.enqueue(header)).status, 201)
        for header in headers:
            self.assertEqual((await self.client.get('/v1/matchmaking/transport', headers=header)).status, 409)


class EosRoomGuestTests(unittest.IsolatedAsyncioTestCase):
    asyncSetUp = EosRoomIdentityTests.asyncSetUp
    enqueue = EosRoomIdentityTests.enqueue
    session = EosRoomIdentityTests.session
    room = EosRoomIdentityTests.room
    headers = staticmethod(EosRoomIdentityTests.headers)
    route = staticmethod(EosRoomIdentityTests.route)
    configure = EosRoomIdentityTests.configure

    def catalog_fixture(self):
        value = baseline.catalog()
        for row in value['playlists']:
            row.update(ranked=False, minimum_players=3, maximum_players=3, maximum_party=2)
        return value

    # This fixture admits a real couch guest alongside two physical owners.
    async def test_guests_have_no_room_credential_and_cannot_supply_player_identity(self):
        room, rows = await self.room(first_extra={'local_players': [{'screen_name': 'Couch Guest'}]})
        self.configure(); self.assertEqual(len(room.guests), 1)
        guest = next(iter(room.guests)); self.assertNotIn(guest, room.tokens)
        response = await self.client.post(self.route(rows[0], 'eos-token'), headers={'Authorization': 'Bearer ' + guest}, json={})
        self.assertEqual(response.status, 401)
        response = await self.client.post(self.route(rows[0], 'eos-token'), headers=self.headers(rows[0]), json={'player': guest})
        self.assertEqual(response.status, 400)
        response = await self.client.post(self.route(rows[0], 'eos-token'), headers=self.headers(rows[0]), json={})
        self.assertEqual(response.status, 200)
        claims = json.loads(decode((await response.json())['token'].split('.')[1]))
        self.assertNotEqual(claims['sub'], guest)

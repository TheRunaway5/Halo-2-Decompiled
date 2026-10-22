"""Signed account identity and strict binding; SDK verification is a test boundary."""
import base64
import json
import os
import unittest
from unittest.mock import AsyncMock, patch
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.asymmetric import padding, rsa
from multiplayer.master_server.eos_identity import EosIdentity
from . import test_player_profile as profiles


def decode(value):
    return base64.urlsafe_b64decode(value+'='*(-len(value)%4))


class EosIdentityTests(unittest.IsolatedAsyncioTestCase):
    asyncSetUp=profiles.PlayerProfileTests.asyncSetUp
    catalog_fixture=staticmethod(profiles.PlayerProfileTests.catalog_fixture)
    login=profiles.PlayerProfileTests.login

    def configure(self):
        directory=self.api.directory
        directory.settings.allow_http=False
        directory.settings.behind_tls_proxy=True
        with patch.dict(os.environ,{'H2_EOS_OPENID_AUDIENCE':'h2forever-profile-v1'}):
            identity=EosIdentity(directory)
        directory.eos_identity.key=identity.key
        directory.eos_identity.kid=identity.kid
        directory.eos_identity.audience=identity.audience
        self.addAsyncCleanup(identity.close,None)
        return directory.eos_identity

    async def test_signed_token_jwks_expiry_and_persistent_key(self):
        identity=self.configure()
        headers=await self.login()
        response=await self.client.post('/v1/account/eos-token',headers=headers,json={})
        self.assertEqual(response.status,200)
        data=await response.json()
        header,payload,signature=data['token'].split('.')
        claims=json.loads(decode(payload))
        who=self.api.directory.store.db.execute('SELECT player FROM player_profiles').fetchone()[0]
        self.assertEqual(claims['sub'],who)
        self.assertEqual(claims['aud'],'h2forever-profile-v1')
        self.assertEqual(claims['name'],'Pilot one')
        self.assertEqual(claims['exp']-claims['iat'],120)
        self.assertEqual(data['expires_in'],120)
        key=(await (await self.client.get('/v1/auth/eos/jwks')).json())['keys'][0]
        self.assertEqual(key['kid'],json.loads(decode(header))['kid'])
        public=rsa.RSAPublicNumbers(int.from_bytes(decode(key['e'])),int.from_bytes(decode(key['n']))).public_key()
        public.verify(decode(signature),(header+'.'+payload).encode(),padding.PKCS1v15(),hashes.SHA256())
        self.assertEqual(self.configure().kid,key['kid'])
        path=self.api.directory.settings.state/'eos-openid-key.pem'
        self.assertEqual(path.stat().st_mode&0o777,0o600)
        self.assertEqual((await self.client.post('/v1/account/eos-token',json={})).status,401)

    async def test_binding_requires_verified_account_and_cannot_reassign(self):
        identity=self.configure()
        headers=await self.login()
        db=self.api.directory.store.db
        who=db.execute('SELECT player FROM player_profiles').fetchone()[0]
        identity.verify=AsyncMock(return_value='another-account')
        body={'puid':'1'*32,'token':'opaque-sdk-proof'}
        self.assertEqual((await self.client.post('/v1/account/eos-proof',headers=headers,json=body)).status,403)
        self.assertEqual(db.execute('SELECT count(*) FROM eos_players').fetchone()[0],0)
        identity.verify.return_value=who
        self.assertEqual((await self.client.post('/v1/account/eos-proof',headers=headers,json=body)).status,200)
        revision=db.execute('SELECT revision FROM eos_profile_outbox WHERE player=?',(who,)).fetchone()[0]
        self.assertEqual((await self.client.post('/v1/account/eos-proof',headers=headers,json=body)).status,200)
        self.assertEqual(db.execute('SELECT revision FROM eos_profile_outbox WHERE player=?',(who,)).fetchone()[0],revision)
        self.assertEqual((await self.client.post('/v1/account/eos-proof',headers=headers,json=body|{'puid':'2'*32})).status,409)
        self.assertEqual((await self.client.post('/v1/account/eos-proof',headers=headers,json=body|{'player':who})).status,400)
        other=await self.login('two')
        second=db.execute("SELECT player FROM player_profiles WHERE screen_name='Pilot two'").fetchone()[0]
        identity.verify.return_value=second
        self.assertEqual((await self.client.post('/v1/account/eos-proof',headers=other,json=body)).status,409)

    async def test_session_revocation_during_verification_cannot_bind(self):
        identity=self.configure();headers=await self.login()
        who=self.api.directory.store.db.execute('SELECT player FROM player_profiles').fetchone()[0]
        async def revoke(*_):
            self.api.directory.clans.sessions.clear()
            return who
        identity.verify=revoke
        self.assertEqual((await self.client.post('/v1/account/eos-proof',headers=headers,json={'puid':'1'*32,'token':'proof'})).status,401)
        self.assertEqual(self.api.directory.store.db.execute('SELECT count(*) FROM eos_players').fetchone()[0],0)

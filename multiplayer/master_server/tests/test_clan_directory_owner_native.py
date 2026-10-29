"""Native account/clan ownership through authenticated service entry paths."""
import unittest
import hashlib
from types import SimpleNamespace
from unittest.mock import patch
from multiplayer.master_server.player_identity import VerifiedOwner
from . import test_clans_http as baseline


class ClanDirectoryOwnerTests(unittest.IsolatedAsyncioTestCase):
    asyncSetUp=baseline.ClansHttpTests.asyncSetUp
    start=baseline.ClansHttpTests.start
    login=baseline.ClansHttpTests.login
    command=baseline.ClansHttpTests.command
    view=baseline.ClansHttpTests.view

    async def test_python_session_bank_cannot_authorize_an_unadmitted_bearer(self):
        token='declared-unadmitted-account-bearer'
        key=hashlib.sha256(token.encode()).hexdigest()
        who=next(iter(self.api.sessions.values())).player
        try:
            self.api.sessions[key]=SimpleNamespace(player=who,expires=self.api.clock()+900,seen=self.api.clock())
        except TypeError:pass
        response=await self.client.get('/v1/clans',headers={'Authorization':'Bearer '+token})
        self.assertEqual(response.status,401,await response.text())

    async def test_account_capacity_rejection_precedes_persistent_identity_and_history_io(self):
        directory=self.api.directory
        async def verifier(session,token,application):
            if token.startswith('bulk'):
                return VerifiedOwner('discord','declared-capacity-subject-'+token,'pilot_'+token)
            return await self.verifier(session,token,application)
        # This is a declared capacity fixture. Disable outer IP/provider rate
        # limits while exercising1000 actual verified account admissions;
        # native account/session capacity and authorization stay enabled.
        with patch.object(directory,'verify_owner',new=verifier),patch.object(directory,'rate',return_value=None):
            first=None
            for i in range(996):
                headers=await self.login('bulk'+str(i))
                if i==0:first=headers
            self.assertEqual(len(self.api.sessions),1000)
            tables=('players','player_profiles','player_identities','history_players','player_friend_tags','friend_cloud_outbox')
            before={table:self.api.db.execute('SELECT count(*) FROM '+table).fetchone()[0] for table in tables}
            response=await self.client.post('/v1/account/session',headers={'Authorization':'Bearer bulk996'},json={'screen_name':'Denied capacity fixture'})
            self.assertEqual(response.status,503,await response.text())
            self.assertEqual({table:self.api.db.execute('SELECT count(*) FROM '+table).fetchone()[0] for table in tables},before)
            replacement=await self.login('bulk0')
            self.assertEqual(len(self.api.sessions),1000)
            self.assertEqual((await self.client.get('/v1/clans',headers=first)).status,401)
            self.assertEqual((await self.client.get('/v1/clans',headers=replacement)).status,200)

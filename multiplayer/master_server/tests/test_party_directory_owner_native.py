"""Accepted party operations through the authenticated service boundary."""
import unittest
from . import test_parties_http as baseline


class PartyDirectoryOwnerTests(unittest.IsolatedAsyncioTestCase):
    asyncSetUp=baseline.PartiesHttpTests.asyncSetUp
    start=baseline.PartiesHttpTests.start
    login=baseline.PartiesHttpTests.login
    clan_command=baseline.PartiesHttpTests.clan_command
    view=baseline.PartiesHttpTests.view
    command=baseline.PartiesHttpTests.command
    join=baseline.PartiesHttpTests.join
    queue_session=baseline.PartiesHttpTests.queue_session

    async def test_activity_capacity_uses_admitted_local_seats(self):
        await self.command(self.a,'create')
        for headers,name in ((self.b,'pilot_two'),(self.c,'pilot_three'),(self.d,'pilot_four')):
            await self.join(self.a,headers,name)
        for owner in ('one','two','three','four'):
            await self.queue_session(owner,compatibility='halo2-native-queue-v9',language=0,
                local_players=[dict(screen_name=f'Guest {i}') for i in range(3)])
        version=(await self.view(self.a))['party']['version']
        response=await self.client.post('/v1/party/activity',headers=self.a,json=dict(version=version))
        self.assertEqual(response.status,409,await response.text())
        self.assertEqual(self.parties.db.execute('SELECT count(*) FROM party_activity_links').fetchone()[0],0)

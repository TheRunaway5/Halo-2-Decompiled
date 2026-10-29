"""Accepted-report accuracy/profile regression with explicit native fixtures.

The local relay fixture supplies identity and completed-PCR inputs. Its trusted
room capability models a new native client that compares the counters against
its terminal snapshot. This is not a played match or native counter oracle.
Legacy clients retain their separate summary-only tests.
"""
import struct
import unittest

from . import test_carnage_history as history
from . import test_match_transport as transport


class AccuracyReportingTests(unittest.IsolatedAsyncioTestCase):
    asyncSetUp=history.CarnageHistoryTests.asyncSetUp
    catalog_fixture=staticmethod(history.CarnageHistoryTests.catalog_fixture)
    session=history.CarnageHistoryTests.session
    enqueue=history.CarnageHistoryTests.enqueue
    completed=history.CarnageHistoryTests.completed

    async def pair(self):
        value=await transport.MatchTransportTests.pair(self)
        room=self.api.transport.rooms[value[1][0]['id']]
        # Internal fixture capability only. No HTTP caller can set this flag.
        self.api.results._fixture_accuracy(room)
        return value

    def report(self,descriptors):
        raw=transport.MatchTransportTests.report(self,descriptors)
        for slot in range(len(descriptors[0]['players'])):
            # Exact original PC consumer: word78, not the old word77 fixture.
            at=0xe30+slot*874+156+5*16
            struct.pack_into('<HH',raw,at+8,getattr(self,'shots',20),getattr(self,'hits',8))
            # An adjacent non-accuracy category must never enter the totals.
            struct.pack_into('<H',raw,at+6,30000)
        return raw

    async def profile(self,room):
        db=self.api.directory.store.db
        player=next(p for p,_,_ in room.players if p not in room.guests)
        public=db.execute('SELECT public_id FROM history_players WHERE player=?',(player,)).fetchone()[0]
        response=await self.client.get('/v1/history/players/'+public+'/profile')
        self.assertEqual(response.status,200,await response.text())
        return player,(await response.json())['statistics']

    async def test_verified_shot_counts_reach_public_profile_and_exclude_guests(self):
        room,_=await self.completed()
        _,stats=await self.profile(room)
        self.assertEqual((stats['shots'],stats['hits'],stats['accuracy_matches']),(20,8,1))
        self.assertTrue(stats['accuracy_complete'])
        db=self.api.directory.store.db
        self.assertEqual(db.execute('SELECT count(*) FROM carnage_player_accuracy').fetchone()[0],2)
        self.assertEqual(db.execute('SELECT count(*) FROM carnage_weapon_accuracy').fetchone()[0],2)
        for guest in room.guests:
            self.assertEqual(db.execute('SELECT count(*) FROM carnage_player_accuracy WHERE player=?',(guest,)).fetchone()[0],0)
        self.api.results.commit(room)
        self.assertEqual((await self.profile(room))[1],stats)

    async def test_career_accuracy_uses_total_shots_instead_of_averaging_percentages(self):
        self.shots,self.hits=1,1
        await self.completed()
        self.shots,self.hits=99,0
        room,_=await self.completed()
        _,stats=await self.profile(room)
        self.assertEqual((stats['matches'],stats['shots'],stats['hits'],stats['accuracy_matches']),(2,100,1,2))

    async def test_zero_shot_result_has_known_coverage(self):
        self.shots,self.hits=0,0
        room,_=await self.completed()
        _,stats=await self.profile(room)
        self.assertEqual((stats['shots'],stats['hits'],stats['accuracy_matches']),(0,0,1))
        self.assertTrue(stats['accuracy_complete'])


if __name__=='__main__':unittest.main()

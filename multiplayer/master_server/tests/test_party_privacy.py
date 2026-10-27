import json
from pathlib import Path
import unittest
import uuid
from unittest.mock import patch

from . import test_parties_http as parties
from multiplayer.master_server import party_privacy
from multiplayer.master_server.protocol import Problem
from multiplayer.master_server.player_identity import VerifiedOwner


class PartyPrivacyHttpTests(unittest.IsolatedAsyncioTestCase):
    start=parties.PartiesHttpTests.start
    login=parties.PartiesHttpTests.login
    asyncSetUp=parties.PartiesHttpTests.asyncSetUp
    view=parties.PartiesHttpTests.view
    join=parties.PartiesHttpTests.join
    queue_session=parties.PartiesHttpTests.queue_session
    enqueue=parties.PartiesHttpTests.enqueue
    team_policy=parties.PartiesHttpTests.team_policy

    async def command(self,headers,action,status=200,**kw):
        if action=="join_friend" and "target_context" not in kw:
            response=await self.client.get("/v1/friends?party=1",headers=headers)
            rows=(await response.json())["friends"]
            target=next((r for r in rows if r["id"]==kw["target"]),None)
            kw["target_context"]=target.get("party_context") if target else None
            if kw["target_context"] is None:kw["target_context"]=str(uuid.uuid4())
        return await parties.PartiesHttpTests.command(self,headers,action,status,**kw)

    async def private_view(self, headers):
        response=await self.client.get("/v1/party?privacy=1&countdown=1",headers=headers)
        self.assertEqual(response.status,200,await response.text())
        return await response.json()

    async def set_privacy(self, headers, mode, status=200, **overrides):
        party=(await self.private_view(headers))["party"]
        body=dict(action="privacy",request_id=str(uuid.uuid4()),version=party["version"],
                  privacy=mode,privacy_revision=party["privacy"]["revision"],
                  privacy_context=party["privacy"]["context"])|overrides
        response=await self.client.post("/v1/party/commands?privacy=1&countdown=1",headers=headers,json=body)
        value=await response.json()
        self.assertEqual(response.status,status,value)
        return value

    async def friendship(self, first, second):
        response=await self.client.get("/v1/friends",headers=second)
        tag=(await response.json())["friend_tag"]
        async def command(headers,action,**kw):
            response=await self.client.post("/v1/friends/commands",headers=headers,
                json=dict(action=action,request_id=str(uuid.uuid4()))|kw)
            self.assertEqual(response.status,200,await response.text())
            return await response.json()
        found=await command(first,"search",tag=tag)
        target=found["search_result"]["id"]
        await command(first,"request",target=target)
        response=await self.client.get("/v1/friends",headers=second)
        incoming=next(r for r in (await response.json())["friends"] if r["state"]=="incoming")
        await command(second,"accept",target=incoming["id"])
        return target

    async def test_open_is_real_authenticated_friend_admission_and_opt_in(self):
        await self.command(self.a,"create")
        raw=(await self.view(self.a))["party"]
        self.assertNotIn("privacy",raw)
        current=(await self.private_view(self.a))["party"]
        self.assertEqual(current["privacy"]["mode"],0)
        self.assertEqual(current["privacy"]["context"],next(m["handle"] for m in current["members"] if m["self"]))
        target=await self.friendship(self.b,self.a)
        response=await self.client.get("/v1/friends?party=1",headers=self.b)
        row=next(r for r in (await response.json())["friends"] if r["id"]==target)
        self.assertTrue(row["party_joinable"])
        response=await self.client.get("/v1/friends",headers=self.b)
        self.assertNotIn("party_joinable",(await response.json())["friends"][0])
        await self.command(self.c,"join_friend",403,target=target) # Public identity is not friendship proof.
        joined=await self.command(self.b,"join_friend",target=target)
        self.assertEqual(len(joined["party"]["members"]),2)
        self.assertTrue(all(not m["ready_playlist"] for m in joined["party"]["members"]))
        self.assertNotIn("private-discord",json.dumps(joined))

    async def test_invitation_only_and_closed_even_with_valid_invitation(self):
        await self.command(self.a,"create")
        target=await self.friendship(self.b,self.a)
        await self.set_privacy(self.a,1)
        await self.command(self.b,"join_friend",403,target=target)
        await self.join(self.a,self.b,"pilot_two")
        await self.command(self.b,"leave")
        await self.command(self.a,"invite",username="pilot_two")
        invite=(await self.view(self.b))["invitations"][0]
        await self.set_privacy(self.a,2)
        await self.command(self.b,"accept",403,target=invite["id"],version=invite["version"])
        await self.command(self.b,"join_friend",403,target=target)
        self.assertIsNone((await self.view(self.b))["party"])
        # Reopening changes metadata only; the separately targeted invitation
        # still has its real roster version and may then be accepted.
        await self.set_privacy(self.a,1)
        await self.command(self.b,"accept",target=invite["id"],version=invite["version"])

    async def test_leader_authority_context_and_revision_are_independent_of_consent_clock(self):
        self.team_policy()
        await self.command(self.a,"create")
        await self.join(self.a,self.b,"pilot_two")
        queues=[await self.queue_session(who) for who in ("one","two")]
        for account in (self.a,self.b):
            await self.command(account,"ready",playlist_id="ranked",revision="lab-1")
        self.parties.clock=lambda:0.0
        await self.command(self.a,"countdown_start",playlist_id="ranked",revision="lab-1")
        await self.enqueue(queues[0])
        before=(await self.private_view(self.a))["party"]
        clock=self.parties.countdowns.copy()
        tickets=set(self.queue.core.tickets)
        await self.set_privacy(self.b,2,403)
        after=(await self.set_privacy(self.a,2))["party"]
        self.assertEqual(after["version"],before["version"])
        self.assertEqual(after["members"],before["members"])
        self.assertEqual(after["countdown"],before["countdown"])
        self.assertEqual(self.parties.countdowns,clock)
        self.assertEqual(set(self.queue.core.tickets),tickets)
        self.assertGreater(after["privacy"]["revision"],before["privacy"]["revision"])
        await self.set_privacy(self.a,0,409,privacy_revision=before["privacy"]["revision"])
        await self.set_privacy(self.a,0,409,privacy_context=str(uuid.uuid4()))
        # A delayed command from an old accepted party cannot target a new
        # party whose independent privacy revision happens to be the same.
        await self.command(self.a,"leave")
        await self.command(self.a,"create")
        fresh=(await self.private_view(self.a))["party"]
        await self.set_privacy(self.a,2,409,privacy_context=before["privacy"]["context"],
                               privacy_revision=fresh["privacy"]["revision"])
        self.assertEqual((await self.private_view(self.a))["party"]["privacy"]["mode"],0)

    async def test_strict_modes_authenticated_context_and_idempotency(self):
        await self.command(self.a,"create")
        for invalid in (-1,3,True,"0",None):
            await self.set_privacy(self.a,invalid,400)
        for invalid in (0,True,"1"):
            await self.set_privacy(self.a,1,400,privacy_revision=invalid)
        request=str(uuid.uuid4())
        first=(await self.set_privacy(self.a,1,request_id=request))["party"]
        response=await self.client.post("/v1/party/commands?privacy=1",headers=self.a,json=dict(
            action="privacy",request_id=request,version=first["version"],privacy=1,
            privacy_revision=first["privacy"]["revision"]-1,privacy_context=first["privacy"]["context"]))
        self.assertEqual(response.status,200,await response.text())
        self.assertEqual((await response.json())["party"]["privacy"],first["privacy"])
        response=await self.client.post("/v1/party/commands",json=dict(action="privacy",request_id=str(uuid.uuid4()),
            version=1,privacy=0,privacy_revision=1,privacy_context=str(uuid.uuid4())))
        self.assertEqual(response.status,401)

    async def test_legacy_party_rows_remain_invitation_only_and_blocks_remain_effective(self):
        await self.command(self.a,"create")
        target=await self.friendship(self.b,self.a)
        db=self.directory.store.db
        with db:db.execute("DELETE FROM party_privacy")
        self.assertEqual((await self.private_view(self.a))["party"]["privacy"]["mode"],1)
        await self.command(self.b,"join_friend",403,target=target)
        await self.set_privacy(self.a,0)
        # An explicit private party block is not bypassed by public Open state.
        await self.command(self.a,"invite",username="pilot_two")
        invite=(await self.view(self.b))["invitations"][0]
        await self.command(self.b,"block",target=invite["id"])
        await self.command(self.b,"join_friend",403,target=target)

    async def test_capacity_counts_actual_connection_owned_couch_seats(self):
        previous=self.directory.verify_owner
        async def verifier(session,token,application):
            if token=="five":return VerifiedOwner("discord","private-discord-subject-five","pilot_five")
            return await previous(session,token,application)
        self.directory.verify_owner=verifier
        fifth=await self.login("five")
        for owner in ("one","two","three","four"):
            await self.queue_session(owner,compatibility="halo2-native-queue-v9",language=0,
                local_players=[dict(screen_name="Guest "+str(n)) for n in (2,3,4)])
        await self.command(self.a,"create")
        for headers,name in ((self.b,"pilot_two"),(self.c,"pilot_three"),(self.d,"pilot_four")):
            await self.join(self.a,headers,name)
        target=await self.friendship(fifth,self.a)
        await self.command(fifth,"join_friend",409,target=target)
        await self.command(self.a,"invite",username="pilot_five")
        invite=(await self.view(fifth))["invitations"][0]
        await self.command(fifth,"accept",409,target=invite["id"],version=invite["version"])
        self.assertEqual(len((await self.view(self.a))["party"]["members"]),4)
        self.assertEqual(self.directory.store.db.execute("SELECT count(*) FROM players").fetchone()[0],5)

    async def test_open_friend_join_does_not_cancel_actual_matchmaking(self):
        self.team_policy()
        await self.command(self.a,"create")
        await self.join(self.a,self.b,"pilot_two")
        target=await self.friendship(self.c,self.a)
        queues=[await self.queue_session(owner) for owner in ("one","two")]
        for headers in (self.a,self.b):
            await self.command(headers,"ready",playlist_id="ranked",revision="lab-1")
        await self.enqueue(queues[0])
        before=(await self.view(self.a))["party"]
        tickets=set(self.queue.core.tickets)
        response=await self.client.get("/v1/friends?party=1",headers=self.c)
        self.assertFalse(next(r for r in (await response.json())["friends"] if r["id"]==target)["party_joinable"])
        await self.command(self.c,"join_friend",409,target=target)
        self.assertEqual((await self.view(self.a))["party"],before)
        self.assertEqual(set(self.queue.core.tickets),tickets)
        self.assertIsNone((await self.view(self.c))["party"])

    async def test_selected_friend_context_cannot_follow_a_different_party(self):
        await self.command(self.a,"create")
        target=await self.friendship(self.b,self.a)
        response=await self.client.get("/v1/friends?party=1",headers=self.b)
        selected=next(r for r in (await response.json())["friends"] if r["id"]==target)
        old_context=selected["party_context"]
        self.assertTrue(selected["party_joinable"])
        await self.command(self.a,"leave")
        await self.command(self.a,"create")
        await self.command(self.b,"join_friend",409,target=target,target_context=old_context)
        self.assertIsNone((await self.view(self.b))["party"])
        response=await self.client.get("/v1/friends?party=1",headers=self.b)
        current=next(r for r in (await response.json())["friends"] if r["id"]==target)
        self.assertNotEqual(current["party_context"],old_context)
        await self.command(self.b,"join_friend",target=target,target_context=current["party_context"])


class PartyPrivacyKernelTests(unittest.TestCase):
    def test_missing_native_kernel_fails_closed(self):
        for factory in (dict(return_value=object()),dict(side_effect=OSError("unavailable"))):
            party_privacy.native.cache_clear()
            try:
                with patch("multiplayer.master_server.party_privacy.library",**factory):
                    with self.assertRaises(Problem) as raised:party_privacy.mode(0)
                self.assertEqual(raised.exception.status,503)
            finally:party_privacy.native.cache_clear()

    def test_actual_unchanged_tu15_privacy_capacity_controls_through_ffi(self):
        path=Path(__file__).resolve().parents[3]/"tests/fixtures/tu15_party_privacy_original.json"
        source=json.loads(path.read_text())
        self.assertEqual(source["source_sha256"],"21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd")
        native=party_privacy.native()
        for row in source["cases"]:
            with self.subTest(row=row):
                self.assertEqual(native.h2_party_privacy_admission(row["mode"],row["invited"],row["count"],
                    row["public_open"],row["private_open"]),row["error"])

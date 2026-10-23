from dataclasses import replace
import json
import unittest
import uuid

from . import test_clans_http as clans


class PartiesHttpTests(unittest.IsolatedAsyncioTestCase):
    start = clans.ClansHttpTests.start
    login = clans.ClansHttpTests.login
    clan_command = clans.ClansHttpTests.command

    async def asyncSetUp(self):
        await clans.ClansHttpTests.asyncSetUp(self)
        self.directory = next(iter(self.app.values()))
        self.parties = self.directory.parties
        self.queue = self.directory.matchmaking

    async def view(self, headers):
        response = await self.client.get("/v1/party",headers=headers)
        self.assertEqual(response.status,200,await response.text())
        return await response.json()

    async def command(self, headers, action, status=200, **kw):
        if action in ("invite","invite_discord","invite_steam","invite_member","invite_clan","cancel","leave","remove","transfer","ready","unready") and "version" not in kw:
            kw["version"] = (await self.view(headers))["party"]["version"]
        body = dict(action=action,request_id=str(uuid.uuid4())) | kw
        response = await self.client.post("/v1/party/commands",headers=headers,json=body)
        value = await response.json()
        self.assertEqual(response.status,status,value)
        return value

    async def join(self, leader, target, username):
        await self.command(leader,"invite",username=username)
        invite = (await self.view(target))["invitations"][0]
        return await self.command(target,"accept",target=invite["id"],version=invite["version"])

    async def queue_session(self, owner, **kw):
        body = dict(screen_name="Same Pilot",maps=["lockout"],compatibility="test-v1") | kw
        response = await self.client.post("/v1/matchmaking/session",headers={"Authorization":"Bearer "+owner},json=body)
        self.assertEqual(response.status,201,await response.text())
        return {"Authorization":"Bearer "+(await response.json())["token"]}

    async def enqueue(self, headers, status=201):
        response = await self.client.post("/v1/matchmaking/ticket",headers=headers,
            json=dict(playlist_id="ranked",revision="lab-1",request_id=str(uuid.uuid4())))
        body = await response.json()
        self.assertEqual(response.status,status,body)
        return body

    def team_policy(self):
        self.queue.entries = tuple(replace(e,policy=replace(e.policy,team_size=2,minimum_players=4,
                                  maximum_players=4,maximum_party=2)) for e in self.queue.entries)
        self.queue.core.playlists = {e.policy.id:e.policy for e in self.queue.entries}

    async def test_identity_invite_accept_decline_block_cancel_and_privacy(self):
        self.assertEqual((await self.client.get("/v1/party")).status,401)
        first = await self.command(self.a,"create")
        await self.command(self.a,"create",409)
        await self.command(self.a,"invite",400,username="Chosen Pilot")
        await self.command(self.a,"invite",404,username="unknown_pilot")
        await self.command(self.a,"invite",username="pilot_two")
        invitation = (await self.view(self.b))["invitations"][0]
        await self.command(self.c,"accept",404,target=invitation["id"],version=invitation["version"])
        await self.command(self.b,"decline",target=invitation["id"])
        await self.command(self.a,"invite",username="pilot_two")
        invitation = (await self.view(self.b))["invitations"][0]
        await self.command(self.b,"block",target=invitation["id"])
        await self.command(self.a,"invite",403,username="pilot_two")
        await self.command(self.a,"invite",username="pilot_three")
        outgoing = (await self.view(self.a))["party"]["outgoing"][0]
        await self.command(self.a,"cancel",target=outgoing["id"])
        self.assertEqual((await self.view(self.c))["invitations"],[])
        joined = await self.join(self.a,self.c,"pilot_three")
        self.assertEqual([r["self"] for r in joined["party"]["members"]],[False,True])
        self.assertEqual({r["name"] for r in joined["party"]["members"]},{"Chosen Pilot"})
        private_ids = {r[0] for r in self.directory.store.db.execute("SELECT id FROM players")}
        for private in (*private_ids,"private-discord","pilot_one","subject","provider","token"):
            self.assertNotIn(private,json.dumps(joined))
        self.assertNotEqual(first["party"]["members"][0]["handle"],next(iter(private_ids)))
        await self.command(self.c,"invite",403,username="pilot_four")
        await self.command(self.a,"invite",409,version=1,username="pilot_four")

    async def test_selected_clan_member_requires_current_membership_and_presence(self):
        await self.clan_command(self.a,"create",name="Clan")
        await self.clan_command(self.a,"invite",username="pilot_two")
        invite=(await clans.ClansHttpTests.view(self,self.b))["invitations"][0]["id"]
        await self.clan_command(self.b,"accept",target=invite)
        roster=(await clans.ClansHttpTests.view(self,self.b))["clan"]["members"]
        peer=next(row["id"] for row in roster if row["self"])
        await self.command(self.a,"create")
        await self.command(self.a,"invite_member",target=peer)
        self.assertEqual(len((await self.view(self.b))["invitations"]),1)
        await self.clan_command(self.b,"leave")
        await self.command(self.a,"invite_member",403,target=peer)

    async def test_roster_appearance_uses_current_accepted_member_profiles(self):
        await self.command(self.a, "create")
        await self.join(self.a, self.b, "pilot_two")
        raw = "080c0901011a098100"
        response = await self.client.put("/v1/account/profile", headers=self.b,
                                       json={"appearance": raw})
        self.assertEqual(response.status, 200, await response.text())
        response = await self.client.get("/v1/party?local_players=1&appearance=1", headers=self.a)
        self.assertEqual(response.status, 200)
        members = (await response.json())["party"]["members"]
        self.assertEqual([m["appearance"] for m in members], [None, raw])
        # Display names are deliberately identical. Only accepted identity may
        # select a profile; invitees and unrelated accounts cannot supply it.
        await self.command(self.a, "invite", username="pilot_three")
        self.assertEqual(len(members), 2)
        legacy = (await self.view(self.a))["party"]["members"]
        self.assertTrue(all("appearance" not in m for m in legacy))
        self.assertEqual((await self.client.get("/v1/party?appearance=1")).status, 401)

    async def test_sdk_subject_invite_survives_recycled_username_and_preserves_privacy(self):
        db=self.directory.store.db
        with db:
            db.execute("UPDATE players SET subject=? WHERE subject=?",("222222222222222222","private-discord-subject-two"))
            db.execute("UPDATE player_profiles SET discord_username='' WHERE discord_username='pilot_two'")
            db.execute("UPDATE player_profiles SET discord_username='pilot_two' WHERE discord_username='pilot_three'")
        await self.command(self.a,"create")
        await self.command(self.a,"invite_discord",400,discord_id="not-an-account")
        await self.command(self.a,"invite_discord",404,discord_id="999999999999999999")
        rid=str(uuid.uuid4())
        sent=await self.command(self.a,"invite_discord",discord_id="222222222222222222",request_id=rid)
        self.assertEqual(sent,await self.command(self.a,"invite_discord",discord_id="222222222222222222",request_id=rid))
        incoming=(await self.view(self.b))["invitations"]
        self.assertEqual(len(incoming),1)
        self.assertEqual((await self.view(self.c))["invitations"],[])
        self.assertNotIn("222222222222222222",json.dumps(sent))
        self.assertNotIn("discord_id",json.dumps(sent))
        await self.command(self.b,"block",target=incoming[0]["id"])
        await self.command(self.a,"invite_discord",403,discord_id="222222222222222222")
        # The same native leader/block/accept policy owns both entry points.
        await self.join(self.a,self.c,"pilot_two")
        await self.command(self.c,"invite_discord",403,discord_id="222222222222222222")

    async def test_leader_transfer_leave_remove_idempotence_and_persistence(self):
        rid = str(uuid.uuid4())
        first = await self.command(self.a,"create",request_id=rid)
        self.assertEqual(first,await self.command(self.a,"create",request_id=rid))
        await self.command(self.a,"leave",409,request_id=rid)
        await self.join(self.a,self.b,"pilot_two")
        await self.join(self.a,self.c,"pilot_three")
        roster = (await self.view(self.b))["party"]["members"]
        b = next(r["handle"] for r in roster if r["self"])
        await self.command(self.a,"transfer",target=b)
        self.assertTrue((await self.view(self.b))["party"]["leader"])
        await self.command(self.a,"remove",403,target=b)
        await self.command(self.b,"leave")
        self.assertTrue((await self.view(self.a))["party"]["leader"])
        await self.client.close()
        await self.start()
        self.directory = next(iter(self.app.values())); self.parties = self.directory.parties
        self.a,self.c = await self.login("one"),await self.login("three")
        roster = (await self.view(self.a))["party"]["members"]
        self.assertEqual(len(roster),2)
        self.assertTrue(all(not r["ready_playlist"] for r in roster))
        c = next(r["handle"] for r in roster if not r["self"])
        await self.command(self.a,"remove",target=c)
        self.assertIsNone((await self.view(self.c))["party"])
        await self.command(self.a,"leave")
        self.assertEqual(self.directory.store.db.execute("SELECT count(*) FROM parties").fetchone()[0],0)

    async def test_group_queue_consent_whole_teams_member_cancel_and_session_loss(self):
        self.team_policy()
        await self.command(self.a,"create"); await self.join(self.a,self.b,"pilot_two")
        await self.command(self.c,"create"); await self.join(self.c,self.d,"pilot_four")
        q = [await self.queue_session(owner) for owner in ("one","two","three","four")]
        await self.enqueue(q[0],409)  # Acceptance alone is not search consent.
        await self.enqueue(q[1],403)
        for account in (self.b,self.d):
            await self.command(account,"ready",playlist_id="ranked",revision="lab-1")
        self.assertEqual((await self.enqueue(q[0]))["queued_players"],2)
        matched = await self.enqueue(q[2])
        self.assertEqual(len(matched["match"]["players"]),4)
        proposal = next(iter(self.queue.core.proposals.values()))
        parties = [{p.id for p in t.players} for t in self.queue.core.tickets.values()]
        self.assertEqual({frozenset(t) for t in proposal.teams},{frozenset(p) for p in parties})
        self.assertEqual((await self.client.delete("/v1/matchmaking/ticket",headers=q[1])).status,200)
        self.assertFalse(self.queue.core.proposals)
        self.assertEqual(len(self.queue.core.tickets),1)
        await self.enqueue(q[0])
        self.assertEqual((await self.client.delete("/v1/matchmaking/session",headers=q[1])).status,200)
        self.assertEqual(len(self.queue.core.tickets),1)
        party = (await self.view(self.a))["party"]
        self.assertEqual(len(party["members"]),2)
        self.assertTrue(all(not m["ready_playlist"] for m in party["members"]))
        await self.enqueue(q[0],409)
        await self.queue_session("two")
        await self.command(self.b,"ready",playlist_id="ranked",revision="lab-1")
        await self.enqueue(q[0])
        await self.command(self.b,"leave")
        self.assertEqual(len(self.queue.core.tickets),1)
        self.assertFalse(self.queue.core.proposals)

    async def test_maps_compatibility_revision_and_new_consent_cancel(self):
        self.team_policy()
        await self.command(self.a,"create"); await self.join(self.a,self.b,"pilot_two")
        a = await self.queue_session("one")
        await self.queue_session("two",maps=["ascension"])
        await self.command(self.b,"ready",409,playlist_id="ranked",revision="lab-1")
        await self.queue_session("two",compatibility="different")
        await self.command(self.b,"ready",409,playlist_id="ranked",revision="stale")
        await self.command(self.b,"ready",playlist_id="ranked",revision="lab-1")
        await self.enqueue(a,409)
        await self.queue_session("two")
        await self.command(self.b,"ready",playlist_id="ranked",revision="lab-1")
        await self.enqueue(a)
        await self.command(self.b,"ready",playlist_id="casual",revision="lab-1")
        self.assertFalse(self.queue.core.tickets)
        await self.enqueue(a,409)

    async def test_clan_batch_invites_are_not_membership_and_respect_source_roles(self):
        await self.clan_command(self.a,"create",name="Clan")
        await self.clan_command(self.a,"invite",username="pilot_two")
        invite = (await clans.ClansHttpTests.view(self,self.b))["invitations"][0]["id"]
        await self.clan_command(self.b,"accept",target=invite)
        roster = (await clans.ClansHttpTests.view(self,self.b))["clan"]["members"]
        b = next(m["id"] for m in roster if m["self"])
        await self.clan_command(self.a,"role",target=b,role=0,confirmed=True)
        await self.command(self.b,"create")
        await self.command(self.b,"invite_clan",403)
        await self.command(self.b,"leave")
        await self.command(self.a,"create")
        await self.command(self.a,"invite_clan")
        self.assertEqual(len((await self.view(self.a))["party"]["members"]),1)
        invite = (await self.view(self.b))["invitations"][0]
        await self.command(self.b,"accept",target=invite["id"],version=invite["version"])
        self.assertEqual(len((await self.view(self.a))["party"]["members"]),2)

    async def test_invitation_expiry_and_no_client_roster_injection(self):
        await self.command(self.a,"create")
        await self.command(self.a,"invite",username="pilot_two")
        invite = (await self.view(self.b))["invitations"][0]
        now = self.parties.wall(); self.parties.wall = lambda: now+601
        await self.command(self.b,"accept",404,target=invite["id"],version=invite["version"])
        response = await self.client.post("/v1/party/commands",headers=self.a,
            json=dict(action="create",request_id=str(uuid.uuid4()),players=["forged"]))
        self.assertEqual(response.status,400)

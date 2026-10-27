from dataclasses import replace
import hashlib
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
        if action in ("invite","invite_discord","invite_steam","invite_member","invite_clan","cancel","leave","leave_lobby","remove","transfer","ready","unready","countdown_start","countdown_cancel","countdown_request") and "version" not in kw:
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

    async def test_shared_original_pregame_countdown_start_shorten_cancel_and_late_member(self):
        self.team_policy()
        await self.command(self.a,"create")
        await self.join(self.a,self.b,"pilot_two")
        for owner,account in (("one",self.a),("two",self.b)):
            await self.queue_session(owner)
            await self.command(account,"ready",playlist_id="ranked",revision="lab-1")
        before=(await self.view(self.a))["party"]
        now=[0.0];self.parties.clock=lambda:now[0]
        async def countdown(headers):
            response=await self.client.get("/v1/party/countdown",headers=headers)
            self.assertEqual(response.status,200,await response.text())
            return (await response.json())["countdown"]
        async def start():
            await self.command(self.a,"countdown_start",playlist_id="ranked",revision="lab-1")
        self.assertEqual((await self.client.get("/v1/party/countdown")).status,401)
        await self.command(self.b,"countdown_start",403,playlist_id="ranked",revision="lab-1")
        await start()
        initial=await countdown(self.a)
        self.assertEqual(initial,await countdown(self.b))
        self.assertEqual(initial["seconds"],10)
        self.assertEqual((await self.view(self.a))["party"],before)
        now[0]=.999;self.parties.advance_countdowns()
        self.assertEqual((await countdown(self.b))["seconds"],10)
        now[0]=1.0;self.parties.advance_countdowns()
        self.assertEqual((await countdown(self.b))["seconds"],9)
        now[0]=20.0;self.parties.advance_countdowns()
        self.assertEqual((await countdown(self.b))["seconds"],8) # one source update, no catchup
        await start()
        self.assertEqual((await countdown(self.b))["seconds"],7)
        for _ in range(8):
            await start()
        minimum=await countdown(self.a)
        self.assertEqual(minimum["seconds"],3)
        await start()
        self.assertEqual(await countdown(self.a),minimum)
        await self.command(self.b,"countdown_cancel",403)
        await self.command(self.b,"countdown_request")
        requested=await countdown(self.a)
        self.assertEqual((requested["seconds"],requested["mode"]),(10,1))
        stored=next(iter(self.parties.countdowns.values()))
        self.assertIsNotNone(stored.requester)
        now[0]+=1;self.parties.advance_countdowns()
        self.assertEqual(((await countdown(self.a))["seconds"],stored.mode,stored.requester),(9,0,None))
        await self.command(self.a,"countdown_cancel")
        self.assertIsNone(await countdown(self.b))
        await start()
        for _ in range(5):
            now[0]+=1;self.parties.advance_countdowns()
        late=await countdown(self.b)
        self.assertEqual(late,await countdown(self.a))
        self.assertEqual(late["seconds"],5)
        for _ in range(5):
            now[0]+=1;self.parties.advance_countdowns()
        zero=await countdown(self.a)
        self.assertEqual((zero["seconds"],zero["complete"]),(0,True))
        await start()
        self.assertEqual(await countdown(self.b),zero) # state10 at zero cannot restart itself
        leader=self.directory.store.db.execute("SELECT leader FROM parties WHERE id=?",(next(iter(self.parties.countdowns)),)).fetchone()[0]
        self.parties.queue_members(leader,self.queue.core.playlists["ranked"])
        consumed=await countdown(self.b)
        self.assertEqual((consumed["seconds"],consumed["complete"],consumed["serial"]),(None,True,zero["serial"]))
        self.assertEqual((await self.view(self.b))["party"]["version"],before["version"])
        self.assertTrue(all(m["ready_playlist"]=="ranked" for m in (await self.view(self.b))["party"]["members"]))

    async def test_original_lobby_confirm_leaves_accepted_party_and_invalidates_countdown(self):
        self.team_policy()
        await self.command(self.a,"create")
        await self.join(self.a,self.b,"pilot_two")
        for owner,account in (("one",self.a),("two",self.b)):
            await self.queue_session(owner)
            await self.command(account,"ready",playlist_id="ranked",revision="lab-1")
        await self.command(self.a,"countdown_start",playlist_id="ranked",revision="lab-1")
        version=(await self.view(self.a))["party"]["version"]
        await self.command(self.b,"leave_lobby")
        self.assertIsNone((await self.view(self.b))["party"])
        remaining=(await self.view(self.a))["party"]
        self.assertEqual(len(remaining["members"]),1)
        self.assertGreater(remaining["version"],version)
        self.assertEqual(remaining["members"][0]["ready_playlist"],"")
        self.assertFalse(self.parties.countdowns)
        self.assertFalse(self.queue.core.tickets)
        # Rejoin is a fresh accepted membership and requires fresh consent.
        await self.join(self.a,self.b,"pilot_two")
        await self.command(self.a,"countdown_start",409,playlist_id="ranked",revision="lab-1")
        # 251345's two-member leader confirm kicks its sole remote first.
        await self.command(self.a,"leave_lobby")
        self.assertIsNone((await self.view(self.a))["party"])
        self.assertIsNone((await self.view(self.b))["party"])

    async def test_verified_lease_renewal_preserves_party_consent_and_live_ticket(self):
        self.team_policy()
        await self.command(self.a, "create")
        await self.join(self.a, self.b, "pilot_two")
        headers = [await self.queue_session(owner) for owner in ("one", "two")]
        for account in (self.a, self.b):
            await self.command(account, "ready", playlist_id="ranked", revision="lab-1")
        await self.enqueue(headers[0])
        before = (await self.view(self.a))["party"]
        old_token = headers[0]["Authorization"][7:]
        session = self.queue.sessions[hashlib.sha256(old_token.encode()).hexdigest()]
        session.expires = self.queue.clock() + 1
        ticket = self.queue.ticket(session.player)
        response = await self.client.post("/v1/matchmaking/session", headers={"Authorization": "Bearer one"},
            json=dict(screen_name="Same Pilot", maps=["lockout"], compatibility="test-v1", renew_session=old_token))
        self.assertEqual(response.status, 201, await response.text())
        self.assertEqual((await response.json())["token"], old_token)
        self.assertIs(self.queue.ticket(session.player), ticket)
        self.assertIn(session, self.queue.sessions.values())
        self.assertGreater(session.expires, self.queue.clock() + 899)
        self.assertEqual((await self.view(self.a))["party"], before)

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

    async def test_postgame_presence_is_opt_in_and_follows_report_peers(self):
        await self.command(self.a, "create")
        await self.join(self.a, self.b, "pilot_two")
        leader = self.directory.store.db.execute("SELECT leader FROM parties").fetchone()[0]

        class Socket:
            closed = False

        class Peer:
            challenge, sent = None, 0.0

            def __init__(self):
                self.socket = Socket()

        class Room:
            closed, phase, deadline, guests = False, "postgame", float("inf"), ()

            def __init__(self, peers):
                self.peers, self.owners = peers, {who: who for who in peers}

        class Transport:
            rooms = {}

        transport = Transport()
        transport.rooms = {"room": Room({leader: Peer()})}
        self.directory.match_transport = transport

        async def flags():
            response = await self.client.get("/v1/party?postgame=1", headers=self.b)
            self.assertEqual(response.status, 200, await response.text())
            return {m["leader"]: m["postgame"] for m in (await response.json())["party"]["members"]}

        # The leader is still on the carnage report; the member is not.
        self.assertEqual(await flags(), {True: True, False: False})
        # Older clients and command responses keep the exact legacy fields.
        self.assertTrue(all("postgame" not in m for m in (await self.view(self.b))["party"]["members"]))
        # Leaving the report (Done closes the room connection) clears it.
        transport.rooms["room"].peers[leader].socket.closed = True
        self.assertEqual(await flags(), {True: False, False: False})
        transport.rooms["room"].peers[leader].socket.closed = False
        transport.rooms["room"].phase = "running"
        self.assertEqual(await flags(), {True: False, False: False})

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

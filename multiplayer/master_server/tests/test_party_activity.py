import asyncio
import json
import unittest
import uuid
from unittest.mock import patch

from multiplayer.master_server.player_identity import player
from . import test_parties_http as parties


class PartyActivityTests(unittest.IsolatedAsyncioTestCase):
    asyncSetUp = parties.PartiesHttpTests.asyncSetUp
    start = parties.PartiesHttpTests.start
    login = parties.PartiesHttpTests.login
    clan_command = parties.PartiesHttpTests.clan_command
    view = parties.PartiesHttpTests.view
    command = parties.PartiesHttpTests.command
    join = parties.PartiesHttpTests.join
    queue_session = parties.PartiesHttpTests.queue_session
    enqueue = parties.PartiesHttpTests.enqueue

    async def activity(self, headers, status=200, version=None, previous=None):
        if version is None:
            version = (await self.view(headers))["party"]["version"]
        body = {"version": version}
        if previous is not None:
            body["join_secret"] = previous
        response = await self.client.post("/v1/party/activity", headers=headers, json=body)
        value = await response.json()
        self.assertEqual(response.status, status, value)
        return value

    async def test_issue_auth_leader_version_privacy_and_retry(self):
        await self.activity({}, 401, 1)
        await self.activity(self.a, 409, 1)
        await self.command(self.a, "create")
        first = await self.activity(self.a)
        self.assertEqual(set(first), {"party_id", "version", "current_size", "max_size", "join_secret", "expires_in"})
        self.assertEqual((first["current_size"], first["max_size"], first["expires_in"]), (1, 16, 600))
        self.assertRegex(first["join_secret"], r"^h2p1_[A-Za-z0-9_-]{43}$")
        self.assertEqual(str(uuid.UUID(first["party_id"])), first["party_id"])
        again = await self.activity(self.a)
        self.assertEqual(first["join_secret"], again["join_secret"])
        self.assertEqual(first["party_id"], again["party_id"])
        leader = self.parties.db.execute("SELECT player FROM player_profiles WHERE discord_username='pilot_one'").fetchone()[0]
        private_party = self.parties.party(leader)[0]
        self.assertNotEqual(first["party_id"], private_party)
        public_view = json.dumps(await self.view(self.a))
        self.assertNotIn(first["join_secret"], public_view)
        self.assertNotIn(first["party_id"], public_view)
        for private in ("private-discord", "pilot_one", self.a["Authorization"], private_party):
            self.assertNotIn(private, json.dumps(first))
        await self.activity(self.a, 409, first["version"]+1)
        await self.join(self.a, self.b, "pilot_two")
        await self.activity(self.b, 403)
        changed = await self.activity(self.a)
        self.assertEqual(changed["join_secret"], first["join_secret"])
        self.assertEqual(changed["party_id"], first["party_id"])
        self.assertEqual(changed["version"], first["version"]+1)
        self.assertEqual(changed["current_size"], 2)

    async def test_target_bound_join_uses_shared_membership_and_retry_gate(self):
        await self.command(self.a, "create")
        await self.command(self.a, "invite", username="pilot_two")
        link = await self.activity(self.a)
        await self.command(self.c, "join_activity", 404, join_secret=link["join_secret"])
        queue = await self.queue_session("two")
        await self.enqueue(queue)
        self.assertTrue(self.queue.core.tickets)
        rid = str(uuid.uuid4())
        joined = await self.command(self.b, "join_activity", join_secret=link["join_secret"], request_id=rid)
        self.assertEqual(len(joined["party"]["members"]), 2)
        self.assertTrue(any(m["self"] and m["name"] == "Same Pilot" for m in joined["party"]["members"]))
        self.assertFalse(self.queue.core.tickets)
        self.assertEqual(joined, await self.command(self.b, "join_activity", join_secret=link["join_secret"], request_id=rid))
        await self.command(self.b, "join_activity", 409, join_secret="h2p1_"+"x"*43, request_id=rid)
        await self.command(self.b, "leave")
        replay = await self.command(self.b, "join_activity", join_secret=link["join_secret"], request_id=rid)
        self.assertIsNone(replay["party"])
        await self.command(self.b, "join_activity", 404, join_secret=link["join_secret"])

    async def test_expiry_native_invite_cancel_decline_and_block(self):
        await self.command(self.a, "create")
        now = 100000
        self.parties.wall = lambda: now
        await self.command(self.a, "invite", username="pilot_two")
        link = await self.activity(self.a)
        invitation = (await self.view(self.b))["invitations"][0]
        await self.command(self.b, "decline", target=invitation["id"])
        await self.command(self.b, "join_activity", 404, join_secret=link["join_secret"])
        await self.command(self.a, "invite", username="pilot_two")
        invitation = (await self.view(self.b))["invitations"][0]
        await self.command(self.a, "cancel", target=invitation["id"])
        await self.command(self.b, "join_activity", 404, join_secret=link["join_secret"])
        await self.command(self.a, "invite", username="pilot_two")
        invitation = (await self.view(self.b))["invitations"][0]
        await self.command(self.b, "block", target=invitation["id"])
        await self.command(self.b, "join_activity", 404, join_secret=link["join_secret"])
        await self.command(self.a, "invite", username="pilot_three")
        now += 600
        await self.command(self.c, "join_activity", 404, join_secret=link["join_secret"])
        renewed = await self.activity(self.a)
        self.assertNotEqual(renewed["join_secret"], link["join_secret"])
        self.assertEqual(renewed["expires_in"], 600)
        await self.command(self.c, "join_activity", 404, join_secret=renewed["join_secret"])
        await self.command(self.a, "invite", username="pilot_three")
        self.assertEqual(len((await self.command(self.c, "join_activity", join_secret=renewed["join_secret"]))["party"]["members"]), 2)

    async def test_current_party_matches_and_discord_blocks_reject_without_mutation(self):
        await self.command(self.a, "create")
        await self.command(self.a, "invite", username="pilot_two")
        link = await self.activity(self.a)
        await self.command(self.b, "create")
        await self.command(self.b, "join_activity", 409, join_secret=link["join_secret"])
        await self.command(self.b, "leave")
        who = self.parties.db.execute("SELECT player FROM player_profiles WHERE discord_username='pilot_two'").fetchone()[0]
        # Declared room observations feed the native authority predicate;
        # overriding a former Python decision cannot control Rust admission.
        rooms=lambda player:[dict(closed=False,phase='running',owners={player:player},active=[player],playlist='ranked',clans=[])]
        with patch.object(self.queue, "room_facts", return_value=rooms(who)):
            await self.command(self.b, "join_activity", 409, join_secret=link["join_secret"])
        a = self.parties.db.execute("SELECT player FROM player_profiles WHERE discord_username='pilot_one'").fetchone()[0]
        self.directory.voice_blocks.blocks[who] = {self.directory.voice_blocks.subject(a)}
        await self.command(self.b, "join_activity", 403, join_secret=link["join_secret"])
        self.directory.voice_blocks.blocks.clear()
        self.directory.voice_blocks.blocks[a] = {self.directory.voice_blocks.subject(who)}
        await self.command(self.b, "join_activity", 403, join_secret=link["join_secret"])
        self.assertIsNone((await self.view(self.b))["party"])
        self.assertEqual(len((await self.view(self.a))["party"]["members"]), 1)
        self.directory.voice_blocks.blocks.clear()
        with patch.object(self.queue, "room_facts", return_value=rooms(a)):
            await self.activity(self.a, 409)
            await self.command(self.b, "join_activity", 409, join_secret=link["join_secret"])
        self.assertEqual(len((await self.command(self.b, "join_activity", join_secret=link["join_secret"]))["party"]["members"]), 2)

    async def test_full_party_and_atomic_competing_acceptances(self):
        await self.command(self.a, "create")
        await self.command(self.a, "invite", username="pilot_two")
        await self.command(self.a, "invite", username="pilot_three")
        link = await self.activity(self.a)
        pid = self.parties.db.execute("SELECT id FROM parties").fetchone()[0]
        with self.parties.db:
            for i in range(15):
                who = player(self.parties.db, ("fixture", str(i)), "Extra Pilot")
                self.parties.db.execute("INSERT INTO party_members(player,party,handle) VALUES(?,?,?)", (who,pid,str(uuid.uuid4())))
        await self.activity(self.a, 409)
        await self.command(self.b, "join_activity", 404, join_secret=link["join_secret"])
        with self.parties.db:
            self.parties.db.execute("DELETE FROM party_members WHERE handle=(SELECT handle FROM party_members WHERE party=? ORDER BY rowid DESC LIMIT 1)", (pid,))
        link=await self.activity(self.a)
        responses = await asyncio.gather(*[self.client.post("/v1/party/commands", headers=h,
            json={"action": "join_activity", "join_secret": link["join_secret"], "request_id": str(uuid.uuid4())}) for h in (self.b,self.c)])
        self.assertEqual(sorted(r.status for r in responses), [200,404])
        self.assertEqual(len(self.parties.members(pid)), 16)

    async def test_schema_invalid_secret_and_session_revocation(self):
        await self.command(self.a, "create")
        await self.command(self.a, "invite", username="pilot_two")
        link = await self.activity(self.a)
        for secret in ("", "h2p1_"+"x"*42, "h2p1_"+"x"*44, "https://other.example/join", "h2p1_"+"/"*43, True, None):
            await self.command(self.b, "join_activity", 400, join_secret=secret)
        await self.command(self.b, "join_activity", 400, join_secret=link["join_secret"], version=link["version"])
        response = await self.client.post("/v1/party/activity", headers=self.a, json={"version": link["version"], "join_secret": link["join_secret"], "extra": True})
        self.assertEqual(response.status, 400)
        await self.command({}, "join_activity", 401, join_secret=link["join_secret"])
        await self.client.delete("/v1/account/session", headers=self.b)
        await self.command(self.b, "join_activity", 401, join_secret=link["join_secret"])
        self.assertEqual(len((await self.view(self.a))["party"]["members"]), 1)

    async def test_links_persist_but_party_change_revokes_them(self):
        await self.command(self.a, "create")
        await self.command(self.a, "invite", username="pilot_two")
        link = await self.activity(self.a)
        await self.client.close()
        await self.start()
        self.directory = next(iter(self.app.values()))
        self.parties = self.directory.parties
        self.queue = self.directory.matchmaking
        self.a, self.b = await self.login("one"), await self.login("two")
        self.assertEqual((await self.activity(self.a))["join_secret"], link["join_secret"])
        leader = self.parties.db.execute("SELECT player FROM player_profiles WHERE discord_username='pilot_one'").fetchone()[0]
        self.parties.session_lost(leader)
        await self.command(self.b, "join_activity", 404, join_secret=link["join_secret"])

    async def test_multiple_pending_discord_invites_keep_one_bounded_party_link(self):
        await self.command(self.a, "create")
        for name in ("pilot_two", "pilot_three", "pilot_four"):
            await self.command(self.a, "invite", username=name)
        link = await self.activity(self.a)
        responses = await asyncio.gather(*[self.client.post("/v1/party/commands", headers=h,
            json={"action": "join_activity", "join_secret": link["join_secret"], "request_id": str(uuid.uuid4())})
            for h in (self.b,self.c,self.d)])
        self.assertEqual([r.status for r in responses], [200,200,200])
        refreshed = await self.activity(self.a, previous=link["join_secret"])
        self.assertEqual(refreshed["party_id"], link["party_id"])
        self.assertEqual(refreshed["join_secret"], link["join_secret"])
        self.assertEqual((refreshed["current_size"],refreshed["version"]), (4,4))
        self.assertEqual((await self.view(self.a))["party"]["outgoing"], [])
        self.assertLessEqual(refreshed["expires_in"], link["expires_in"])

    async def test_native_accept_also_preserves_other_discord_invites(self):
        await self.command(self.a, "create")
        await self.command(self.a, "invite", username="pilot_two")
        await self.command(self.a, "invite", username="pilot_three")
        link = await self.activity(self.a)
        invitation = (await self.view(self.b))["invitations"][0]
        await self.command(self.b, "accept", target=invitation["id"], version=invitation["version"])
        await self.command(self.d, "join_activity", 404, join_secret=link["join_secret"])
        joined = await self.command(self.c, "join_activity", join_secret=link["join_secret"])
        self.assertEqual(len(joined["party"]["members"]),3)

    async def test_conditional_refresh_cannot_renew_or_create_a_capability(self):
        await self.command(self.a, "create")
        now = 100000
        self.parties.wall = lambda: now
        await self.command(self.a, "invite", username="pilot_two")
        link = await self.activity(self.a)
        await self.activity(self.a, 404, previous="h2p1_"+"x"*43)
        now += 590
        await self.command(self.b, "join_activity", join_secret=link["join_secret"])
        refreshed = await self.activity(self.a, previous=link["join_secret"])
        self.assertEqual(refreshed["expires_in"],10)
        await self.activity(self.a, 409, version=link["version"], previous=link["join_secret"])
        now += 10
        await self.activity(self.a, 404, previous=link["join_secret"])
        self.assertEqual(self.parties.db.execute("SELECT count(*) FROM party_activity_links").fetchone()[0],0)
        renewed = await self.activity(self.a)
        self.assertNotEqual(renewed["join_secret"],link["join_secret"])

    async def test_departure_leadership_and_consent_changes_revoke_preserved_links(self):
        for action in ("leave","remove","transfer","unready","session_lost"):
            with self.subTest(action=action):
                with self.parties.db:
                    self.parties.db.execute("DELETE FROM parties")
                await self.command(self.a,"create")
                await self.command(self.a,"invite",username="pilot_two")
                await self.command(self.a,"invite",username="pilot_three")
                link = await self.activity(self.a)
                await self.command(self.b,"join_activity",join_secret=link["join_secret"])
                if action == "session_lost":
                    leader = self.parties.db.execute("SELECT player FROM player_profiles WHERE discord_username='pilot_one'").fetchone()[0]
                    self.parties.session_lost(leader)
                elif action in ("remove","transfer"):
                    target = next(m["handle"] for m in (await self.view(self.a))["party"]["members"] if not m["self"])
                    await self.command(self.a,action,target=target)
                else:
                    await self.command(self.b if action=="leave" else self.a,action)
                await self.command(self.c,"join_activity",404,join_secret=link["join_secret"])
                leader_headers = self.b if action=="transfer" else self.a
                await self.activity(leader_headers,404,previous=link["join_secret"])
                self.assertEqual(self.parties.db.execute("SELECT count(*) FROM party_activity_links").fetchone()[0],0)

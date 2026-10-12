import asyncio
from copy import deepcopy
import json
from pathlib import Path
import sqlite3
import tempfile
import unittest
import uuid

from aiohttp.test_utils import TestClient, TestServer
from multiplayer.master_server.player_identity import VerifiedOwner
from multiplayer.master_server.protocol import Problem
from multiplayer.master_server.service import Settings, create_app
from multiplayer.master_server.playlist_skill import POLICY, TABLE_DIGEST
from .test_matchmaking_http import catalog
from . import test_match_transport as transport
from . import test_ranked_results as ranked
from . import test_clans_http as clans


OWNERS = ("one", "two", "three", "four", "five", "six")


def clan_catalog(major=False):
    row = deepcopy(catalog()["playlists"][0])
    row.update(id="clan", quickmatch=False, team_size=4, minimum_players=6,
               maximum_players=8, maximum_party=4, clan_match="minor-retail-launch-2004",
               rank_configuration=POLICY)
    row["skill"] = dict(kind="custom", radius=6)
    if major:
        row.update(team_size=8, minimum_players=12, maximum_players=16, maximum_party=8,
                   clan_match="major-retail-launch-2004")
    row["games"][0]["variant"] = "Team Slayer"
    return dict(version=1, playlists=[row])


class ClanMatchmakingHttpTests(unittest.IsolatedAsyncioTestCase):
    owners = OWNERS
    sizes = (3,3)
    clan_command = clans.ClansHttpTests.command
    login = clans.ClansHttpTests.login
    running = ranked.RankedResultsTests.running
    review = ranked.RankedResultsTests.review

    async def asyncSetUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.path = Path(self.temp.name)
        (self.path/"catalog.json").write_text(json.dumps(clan_catalog(self.sizes == (6,8))))
        async def verifier(session, token, application):
            if token not in self.owners:
                raise Problem(401, "Invalid fixture Discord authorization")
            return VerifiedOwner("discord", "private-subject-"+token, "pilot_"+token)
        app = create_app(Settings(state=self.path, public_url="http://127.0.0.1:8080", allow_http=True,
                                  rate_per_minute=100000, matchmaking_catalog=str(self.path/"catalog.json")), verifier)
        self.client = TestClient(TestServer(app))
        await self.client.start_server()
        self.addAsyncCleanup(self.client.close)
        self.directory = next(iter(app.values()))
        self.addCleanup(self.directory.store.db.close)
        self.api = self.directory.matchmaking
        self.accounts = [await self.login(owner) for owner in self.owners]
        # Fourteen users share one loopback source in this fixture. Keep the
        # real per-owner limit; start queue exchanges with a fresh IP bucket.
        self.directory.buckets.pop("owner:127.0.0.1", None)
        self.queue_headers = {}

    async def session(self, owner="one", name="Same Name"):
        if owner not in self.queue_headers:
            self.queue_headers[owner] = await transport.MatchTransportTests.session(self, owner, name)
        return self.queue_headers[owner]

    async def party_view(self, header):
        return await (await self.client.get("/v1/party", headers=header)).json()

    async def party(self, header, action, **kw):
        if action != "create" and "version" not in kw:
            kw["version"] = (await self.party_view(header))["party"]["version"]
        response = await self.client.post("/v1/party/commands", headers=header,
            json=dict(action=action, request_id=str(uuid.uuid4())) | kw)
        self.assertEqual(response.status, 200, await response.text())
        return await response.json()

    async def prepare(self):
        for base, size, name in ((0, self.sizes[0], "First Clan"), (self.sizes[0], self.sizes[1], "Second Clan")):
            leader = self.accounts[base]
            await self.clan_command(leader, "create", name=name)
            await self.party(leader, "create")
            for i in range(base+1, base+size):
                await self.clan_command(leader, "invite", username="pilot_"+self.owners[i])
                incoming = await (await self.client.get("/v1/clans", headers=self.accounts[i])).json()
                await self.clan_command(self.accounts[i], "accept", target=incoming["invitations"][0]["id"])
                await self.party(leader, "invite", username="pilot_"+self.owners[i])
                invite = (await self.party_view(self.accounts[i]))["invitations"][0]
                await self.party(self.accounts[i], "accept", target=invite["id"], version=invite["version"])
        for owner in self.owners:
            await self.session(owner)
        # A Peon can lead the accepted party into a clan search; clan recruitment
        # privilege is unrelated to original type5 admission.
        # Private IDs are fixture lookups, never displayed or accepted as queue input.
        player_two = self.directory.store.db.execute("SELECT id FROM players WHERE subject=?", ("private-subject-two",)).fetchone()[0]
        await self.clan_command(self.accounts[0], "role", target=player_two, role=0, confirmed=True)
        roster = (await self.party_view(self.accounts[1]))["party"]["members"]
        handle = next(m["handle"] for m in roster if m["self"])
        await self.party(self.accounts[0], "transfer", target=handle)
        for i in range(len(self.owners)):
            if i not in (1, self.sizes[0]):
                await self.party(self.accounts[i], "ready", playlist_id="clan", revision="lab-1")

    async def enqueue(self, headers, **extra):
        return await transport.MatchTransportTests.enqueue(self, headers, "clan", **extra)

    async def pair(self):
        await self.prepare()
        return await transport.MatchTransportTests.pair(self, self.owners, leaders=(1, self.sizes[0]))

    def report(self, descriptors):
        raw = transport.MatchTransportTests.report(self, descriptors)
        raw[0x13] = raw[0x68] = 1
        for slot, player in enumerate(descriptors[0]["players"]):
            raw[0x370+slot*0x94+0x88] = player["team"]
            raw[0x370+slot*0x94+0x90] = 1-player["team"]
        for team in (0, 1):
            raw[0xcb0+team*0x18] = 1
            raw[0xcb0+team*0x18+1] = team
        return raw

    async def test_accepted_clans_peon_leader_private_rank_and_unanimous_clan_progression(self):
        before = await (await self.client.get("/v1/matchmaking/playlists", headers=await self.session())).json()
        self.assertFalse(before["playlists"][0]["eligible"])
        self.assertIsNone(before["playlists"][0]["level"])
        headers, descriptors, sockets, queues, host = await self.running()
        room = self.api.transport.rooms[descriptors[0]["id"]]
        self.assertEqual(sorted(map(len, room.proposal.teams)), list(self.sizes))
        self.assertEqual(len(set(room.clans.values())), 2)
        db = self.directory.store.db
        for owner in self.owners:
            self.assertEqual((await self.enqueue(await self.session(owner), clan="forged")).status, 400)
        # Removing or leaving an admitted clan match cannot invalidate its owner.
        player_two = db.execute("SELECT id FROM players WHERE subject=?", ("private-subject-two",)).fetchone()[0]
        await self.clan_command(self.accounts[0], "remove", 409, target=player_two)
        await self.clan_command(self.accounts[1], "leave", 409)
        digest = await self.review(descriptors, sockets, queues, host, self.report(descriptors))
        for socket in sockets[:-1]:
            await socket.send_json({"confirm_result": digest})
        await asyncio.sleep(.05)
        self.assertEqual(db.execute("SELECT count(*) FROM match_results").fetchone()[0], 0)
        await sockets[-1].send_json({"confirm_result": digest})
        for queue in queues:
            self.assertEqual(await asyncio.wait_for(queue.get(), 2), {"completed": digest})
            await asyncio.wait_for(queue.get(), 2)
        self.api.results.commit(room)
        rows = db.execute("SELECT configuration,table_digest,delta,raw_rank,xp FROM match_result_clan_skill ORDER BY delta DESC").fetchall()
        self.assertEqual([tuple(r) for r in rows], [(POLICY,TABLE_DIGEST,100,1,100),(POLICY,TABLE_DIGEST,0,0,0)])
        self.assertEqual(db.execute("SELECT count(*) FROM playlist_skill").fetchone()[0], 0)
        self.assertEqual(db.execute("SELECT count(*) FROM match_result_skill").fetchone()[0], 0)
        for header, descriptor in zip(headers, descriptors):
            body = await (await self.client.get("/v1/matchmaking/playlists", headers=header)).json()
            self.assertEqual(body["playlists"][0]["level"], 2 if descriptor["players"][descriptor["slot"]]["team"] == 0 else 1)
            for private in ("private-subject", "clan_id", '"xp"', '"raw_rank"'):
                self.assertNotIn(private, json.dumps(body))
        with sqlite3.connect(self.path/"registry.sqlite3") as reopened:
            self.assertEqual(reopened.execute("SELECT sum(xp) FROM clan_playlist_skill").fetchone()[0], 100)
        for index in (1, self.sizes[0]):
            response = await self.enqueue(headers[index])
            self.assertEqual(response.status, 201, await response.text())
        self.assertEqual({t.clan_rank for t in self.api.core.tickets.values()}, {0, 1})
        for base, size in ((0,self.sizes[0]),(self.sizes[0],self.sizes[1])):
            for i in range(base+1,base+size):
                await self.clan_command(self.accounts[i], "leave")
            await self.clan_command(self.accounts[base], "leave")
        self.assertEqual(db.execute("SELECT count(*) FROM clan_playlist_skill").fetchone()[0], 0)
        self.assertEqual(db.execute("SELECT count(*) FROM match_result_clan_skill").fetchone()[0], 2)
        self.assertEqual(db.execute("SELECT count(*) FROM match_results").fetchone()[0], 1)

    async def test_membership_change_cancels_entire_queued_lineup(self):
        await self.prepare()
        response = await self.enqueue(await self.session("two"))
        self.assertEqual(response.status, 201, await response.text())
        player_three = self.directory.store.db.execute("SELECT id FROM players WHERE subject=?", ("private-subject-three",)).fetchone()[0]
        await self.clan_command(self.accounts[0], "remove", target=player_three)
        self.assertFalse(self.api.core.tickets)
        self.assertEqual((await self.enqueue(await self.session("two"))).status, 409)

    async def test_changed_clan_skill_aborts_whole_result_transaction(self):
        _, descriptors, sockets, queues, host = await self.running()
        room = self.api.transport.rooms[descriptors[0]["id"]]
        digest = await self.review(descriptors, sockets, queues, host, self.report(descriptors))
        db = self.directory.store.db
        with db:
            db.execute("UPDATE clan_playlist_skill SET xp=1 WHERE clan=?", (room.clans[0],))
        for socket in sockets:
            await socket.send_json({"confirm_result": digest})
        for queue in queues:
            self.assertIsNone(await asyncio.wait_for(queue.get(), 2))
        for table in ("match_results", "match_result_players", "match_result_teams", "match_result_clan_skill"):
            self.assertEqual(db.execute("SELECT count(*) FROM "+table).fetchone()[0], 0)

    async def test_mixed_clan_membership_and_pending_recruits_cannot_queue(self):
        await self.prepare()
        db = self.directory.store.db
        player_three = db.execute("SELECT id FROM players WHERE subject=?", ("private-subject-three",)).fetchone()[0]
        await self.clan_command(self.accounts[0], "remove", target=player_three)
        # A pending invitation carries no accepted membership.
        await self.clan_command(self.accounts[0], "invite", username="pilot_three")
        response = await self.enqueue(await self.session("two"))
        self.assertEqual(response.status, 409)
        self.assertFalse(self.api.core.tickets)
        await self.clan_command(self.accounts[self.sizes[0]], "invite", username="pilot_three")
        incoming = (await (await self.client.get("/v1/clans", headers=self.accounts[2])).json())["invitations"]
        other = next(i for i in incoming if i["name"] == "Second Clan")
        await self.clan_command(self.accounts[2], "accept", target=other["id"])
        self.assertEqual((await self.enqueue(await self.session("two"))).status, 409)
        self.assertFalse(self.api.core.tickets)


class MajorClanMatchmakingHttpTests(ClanMatchmakingHttpTests):
    owners = OWNERS + ("seven","eight","nine","ten","eleven","twelve","thirteen","fourteen")
    sizes = (6,8)

"""Real HTTP/relay couch admission; guest publication is a server decision."""
import asyncio
import hashlib
import json
import struct
import unittest
from dataclasses import replace

from . import test_matchmaking_http as baseline
from . import test_match_transport as transport
from multiplayer.master_server.playlist_skill import POLICY


def catalog():
    value = baseline.catalog()
    for row in value["playlists"]:
        row.update(team_size=4, minimum_players=8, maximum_players=8, maximum_party=4)
        row["games"][0]["variant"] = "Team Slayer"
    return value


class CouchOnlineTests(unittest.IsolatedAsyncioTestCase):
    asyncSetUp = baseline.MatchmakingHttpTests.asyncSetUp
    catalog_fixture = staticmethod(catalog)
    report = transport.MatchTransportTests.report
    pair = transport.MatchTransportTests.pair

    async def session(self, owner="one", name="Same Name", *, local_players=None):
        if local_players is None:
            local_players = getattr(self, "local_rosters", {}).get(owner,
                [{"screen_name": "Guest " + str(i)} for i in range(2, 5)])
        response = await self.client.post("/v1/matchmaking/session",
            headers={"Authorization": "Bearer " + owner},
            json=dict(screen_name=name, maps=["lockout"], compatibility="halo2-native-queue-v9",
                      language=0, local_players=local_players))
        self.assertEqual(response.status, 201, await response.text())
        return {"Authorization": "Bearer " + (await response.json())["token"]}

    async def enqueue(self, headers, playlist="casual"):
        return await baseline.MatchmakingHttpTests.enqueue(self, headers, playlist)

    async def test_four_slots_and_guest_policy_are_server_owned(self):
        headers = await self.session()
        session = next(iter(self.api.sessions.values()))
        self.assertEqual(len(session.local_players), 4)
        self.assertEqual([p.guest for p in session.local_players], [False, True, True, True])
        self.assertEqual(self.api.directory.store.db.execute("SELECT count(*) FROM players").fetchone()[0], 1)
        playlists = await (await self.client.get("/v1/matchmaking/playlists", headers=headers)).json()
        self.assertFalse(playlists["playlists"][0]["eligible"])
        self.assertTrue(playlists["playlists"][1]["eligible"])
        self.assertEqual((await self.enqueue(headers, "ranked")).status, 409)
        self.assertEqual((await self.enqueue(headers)).status, 201)
        ticket = next(iter(self.api.core.tickets.values()))
        self.assertEqual(len(ticket.players), 4)
        self.assertEqual({p.connection_owner for p in ticket.players}, {session.player})

    async def test_extra_identity_requires_its_own_valid_discord_authorization(self):
        headers = await self.session(local_players=[{"screen_name": "Tethered", "discord_token": "three"},
                                                  {"screen_name": "Guest Two"}, {"screen_name": "Guest Three"}])
        session = next(iter(self.api.sessions.values()))
        self.assertEqual([p.guest for p in session.local_players], [False, False, True, True])
        self.assertEqual(self.verified, ["one", "three"])
        self.assertEqual(self.api.directory.store.db.execute("SELECT count(*) FROM players").fetchone()[0], 2)
        self.assertEqual((await self.enqueue(headers)).status, 201)
        # An unrelated PC cannot silently take an account already seated here.
        response = await self.client.post("/v1/matchmaking/session", headers={"Authorization": "Bearer three"},
            json=dict(screen_name="Other PC", maps=["lockout"], compatibility="halo2-native-queue-v9", language=0))
        self.assertEqual(response.status, 409)

    async def test_invalid_or_forged_local_rosters_are_rejected(self):
        for rows, status in [
            ([{"screen_name": "G", "guest": False}], 400),
            ([{"screen_name": "G", "publish_stats": True}], 400),
            ([{"screen_name": "G", "discord_token": "bad"}], 401),
            ([{"screen_name": "G", "discord_token": "one"}], 400),
            ([{"screen_name": "G"}] * 4, 400),
        ]:
            response = await self.client.post("/v1/matchmaking/session", headers={"Authorization": "Bearer one"},
                json=dict(screen_name="Host", maps=["lockout"], compatibility="halo2-native-queue-v9",
                          language=0, local_players=rows))
            self.assertEqual(response.status, status, await response.text())
        self.assertFalse(self.api.sessions)
        self.assertEqual(self.api.directory.store.db.execute("SELECT count(*) FROM players").fetchone()[0], 0)

    async def test_ranked_requires_each_local_identity_but_not_a_particular_name(self):
        headers = await self.session(local_players=[{"screen_name": "Guest 2", "discord_token": "three"}])
        playlists = await (await self.client.get("/v1/matchmaking/playlists", headers=headers)).json()
        self.assertTrue(playlists["playlists"][0]["eligible"])
        self.assertEqual((await self.enqueue(headers, "ranked")).status, 201)
        self.assertFalse(any(p.guest for t in self.api.core.tickets.values() for p in t.players))

    async def test_tethered_local_player_is_saved_even_with_a_guest_screen_name(self):
        self.local_rosters = {"one": [{"screen_name": "Guest 2", "discord_token": "three"},
                                     {"screen_name": "Guest 3"}, {"screen_name": "Guest 4"}]}
        await self.run_two_couches(expected=3)

    async def test_two_couches_eight_players_gameplay_routes_and_no_guest_stats(self):
        await self.run_two_couches(expected=2)

    async def run_two_couches(self, expected):
        self.api.entries = tuple(replace(e, rank_configuration=POLICY) for e in self.api.entries)
        headers, descriptors, sockets, queues, host = await self.pair()
        room = self.api.transport.rooms[descriptors[0]["id"]]
        self.assertEqual(len(room.players), 8)
        self.assertEqual(len(room.tokens), 2)
        for descriptor in descriptors:
            self.assertEqual(len(descriptor["local_slots"]), 4)
            self.assertEqual(len({descriptor["players"][i]["team"] for i in descriptor["local_slots"]}), 1)
            for private in ("private-subject", "discord_token", "connection_owner"):
                self.assertNotIn(private, json.dumps(descriptor))
        for socket in sockets:
            await socket.send_json({"loaded": "1234567890abcdef:0000035"})
        for queue in queues:
            self.assertEqual((await asyncio.wait_for(queue.get(), 2))["phase"], "running")
        h = next(i for i, d in enumerate(descriptors) if d["slot"] == host)
        other = 1 - h
        packet = b"H2N2\x35\x00\x02" + b"x" * 20
        await sockets[other].send_bytes(bytes([host]) + packet)
        self.assertEqual(await asyncio.wait_for(queues[h].get(), 2), bytes([descriptors[other]["slot"]]) + packet)
        raw = self.report(descriptors)
        raw[0x13] = raw[0x68] = 1
        for slot, (_, _, team) in enumerate(room.players):
            raw[0x370+slot*0x94+0x88] = team
            raw[0x370+slot*0x94+0x90] = slot
            # Distinct guest statistics must remain local/volatile, never in SQLite.
            struct.pack_into("<H", raw, 0xe30+2*(slot*0x1b5+7), 30000 if room.players[slot][0] in room.guests else 7)
            raw[0xcb0+team*0x18] = 1
            raw[0xcb0+team*0x18+1] = team
        await sockets[h].send_json({"postgame": raw.hex()})
        for queue in queues:
            self.assertEqual(await asyncio.wait_for(queue.get(), 2), {"postgame": raw.hex()})
        digest = hashlib.sha256(raw).hexdigest()
        for socket in sockets:
            await socket.send_json({"confirm_result": digest})
        for queue in queues:
            self.assertEqual(await asyncio.wait_for(queue.get(), 2), {"completed": digest})
            await asyncio.wait_for(queue.get(), 2)
        db = self.api.directory.store.db
        for table in ["players", "player_profiles", "match_result_players", "match_result_memberships",
                      "match_result_skill", "playlist_skill"]:
            self.assertEqual(db.execute("SELECT count(*) FROM " + table).fetchone()[0], expected, table)
        self.assertEqual({r[0] for r in db.execute("SELECT kills FROM match_result_players")}, {7})
        self.api.results.commit(room)
        self.assertEqual(db.execute("SELECT count(*) FROM match_result_players").fetchone()[0], expected)

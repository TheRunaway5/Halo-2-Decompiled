"""Private, idempotent ledger of unanimously confirmed native Slayer results.

Agreement is a consistency check, not server simulation or cheat detection.
The retired Live skill service is not inferred from a host's report.
"""
import hashlib
import struct
import json
import logging

from .protocol import require


class MatchResults:
    def __init__(self, store, skills, clan_skills):
        self.store, self.skills, self.clan_skills = store, skills, clan_skills
        store.db.executescript("""
        CREATE TABLE IF NOT EXISTS match_results(
          id TEXT PRIMARY KEY, playlist TEXT NOT NULL, revision TEXT NOT NULL,
          map TEXT NOT NULL, variant TEXT NOT NULL, ranked INTEGER NOT NULL,
          completed INTEGER NOT NULL, digest TEXT NOT NULL,
          validation TEXT NOT NULL CHECK(validation='native-unanimous-v1'));
        CREATE TABLE IF NOT EXISTS match_result_players(
          match TEXT NOT NULL REFERENCES match_results(id),
          player TEXT NOT NULL REFERENCES players(id), placement INTEGER NOT NULL,
          score INTEGER NOT NULL, kills INTEGER NOT NULL, deaths INTEGER NOT NULL,
          assists INTEGER NOT NULL, suicides INTEGER NOT NULL,
          PRIMARY KEY(match,player));
        CREATE INDEX IF NOT EXISTS player_match_results ON match_result_players(player,match);
        CREATE TABLE IF NOT EXISTS match_result_teams(
          match TEXT NOT NULL REFERENCES match_results(id), team INTEGER NOT NULL CHECK(team BETWEEN 0 AND 7),
          placement INTEGER NOT NULL, score INTEGER NOT NULL, PRIMARY KEY(match,team));
        CREATE TABLE IF NOT EXISTS match_result_memberships(
          match TEXT NOT NULL, player TEXT NOT NULL, team INTEGER NOT NULL CHECK(team BETWEEN 0 AND 7),
          individual_placement INTEGER NOT NULL, PRIMARY KEY(match,player),
          FOREIGN KEY(match,player) REFERENCES match_result_players(match,player),
          FOREIGN KEY(match,team) REFERENCES match_result_teams(match,team));
        CREATE TABLE IF NOT EXISTS match_result_consensus(
          match TEXT PRIMARY KEY REFERENCES match_results(id),
          policy TEXT NOT NULL CHECK(policy='native-survivors-v1'),
          admitted_connections INTEGER NOT NULL, confirming_connections INTEGER NOT NULL,
          departed_slots INTEGER NOT NULL, authority_epoch INTEGER NOT NULL);
        """)
        from .carnage_history import ReportHistory
        self.history = ReportHistory(store)

    @staticmethod
    def validate(room, report):
        require(isinstance(report, str) and len(report) == 2 * 0xdc78
                and all(c in "0123456789abcdef" for c in report), "Invalid postgame report")
        raw = bytes.fromhex(report)
        require(raw[0:2] == b"\x01\x01" and raw[0x10:0x12] == b"\x01\x01"
                and raw[0x13] == int(room.team_mode) and raw[0x64:0x68] == b"\x02\x00\x00\x00"
                and (raw[0x68] & 1) == int(room.team_mode), "Unfinished admitted Slayer postgame report")
        teams = {team for _, _, team in room.players} if room.team_mode else set()
        require(all(0 <= team < 8 for team in teams), "Invalid admitted teams")
        for slot, (member, name, team) in enumerate(room.players):
            at = 0x370 + slot * 0x94
            require(raw[at] == 1 and raw[at+2:at+10].hex() == room.identities[member],
                    "Postgame roster differs from admission")
            expected = name.encode("utf-16-le")
            require(len(expected) <= 62 and raw[at+12:at+76] == expected.ljust(64, b"\0"),
                    "Postgame name differs from admission")
            require(raw[at+0x90] < len(room.players), "Postgame placement outside roster")
            require(raw[at+0x88] == (team if room.team_mode else 255), "Postgame team differs from admission")
        require(all(raw[0x370+i*0x94] == 0 for i in range(len(room.players), 16)),
                "Postgame contains unadmitted players")
        for i in range(16):
            at = 0xcb0 + i*0x18
            require(raw[at] == int(i in teams), "Postgame team rows differ from admission")
            if i in teams:
                require(raw[at+1] < len(teams), "Team placement outside admission")
        return raw, hashlib.sha256(raw).hexdigest()

    def commit(self, room):
        require(set(room.confirmations) == set(room.tokens), "All required physical connections must confirm the result")
        db = self.store.db
        with db:
            old = db.execute("SELECT digest FROM match_results WHERE id=?", (room.id,)).fetchone()
            if old:
                require(old[0] == room.result_digest, "Completed result is immutable", 409)
                return
            proposal = room.proposal
            db.execute("INSERT INTO match_results VALUES(?,?,?,?,?,?,?,?,?)",
                       (room.id, proposal.playlist, proposal.revision, proposal.game.map,
                        proposal.game.variant, int(room.ranked), self.store.now(),
                        room.result_digest, "native-unanimous-v1"))
            if getattr(room, "supports_continuity", False):
                db.execute("INSERT INTO match_result_consensus VALUES(?,?,?,?,?,?)",
                           (room.id, "native-survivors-v1", len(room.admitted_owners),
                            len(room.tokens), room.departed_slots, room.continuity.epoch))
            raw = room.result
            if room.team_mode:
                for team in sorted({team for _, _, team in room.players}):
                    at = 0xcb0 + team*0x18
                    db.execute("INSERT INTO match_result_teams VALUES(?,?,?,?)",
                               (room.id, team, raw[at+1], struct.unpack_from("<h",raw,at+2)[0]))
            for slot, (member, _, team) in enumerate(room.players):
                if member in getattr(room, "guests", ()):
                    continue
                at = 0x370 + slot * 0x94
                def stat(index):
                    return struct.unpack_from("<H", raw, 0xe30 + 2 * (slot * 0x1b5 + index))[0] & 0x7fff
                db.execute("INSERT INTO match_result_players VALUES(?,?,?,?,?,?,?,?)",
                           (room.id, member, raw[0xcb0+team*0x18+1] if room.team_mode else raw[at+0x90],
                            struct.unpack_from("<h", raw, at+0x92)[0],
                            stat(7), stat(9), stat(8), stat(11)))
                if room.team_mode:
                    db.execute("INSERT INTO match_result_memberships VALUES(?,?,?,?)",
                               (room.id, member, team, raw[at+0x90]))
            self.skills.commit(room)
            self.clan_skills.commit(room)
            public = self.history.capture(room)
        logging.getLogger("halo2.results").info(json.dumps({"event": "carnage_report_committed",
            "match": public, "format": "vista-pcr-consumed-v1",
            "published_players": len(room.players)-len(getattr(room, "guests", ())) }))


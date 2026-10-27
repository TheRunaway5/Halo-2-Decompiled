"""Private ledger of unanimously confirmed original native multiplayer results.

Agreement is a consistency check, not server simulation or cheat detection.
The retired Live skill service is not inferred from a host's report.
"""
import hashlib
import struct
import json
import logging

from .protocol import require

# Exact original mainmenu presets, not English substring classification.
# tools/oracle_multiplayer_engines_tu15.py independently executes original
# init/writes/normalization and retains each304-byte option buffer in
# tests/fixtures/multiplayer_engines_original_tu15.json (SHA-256
# c84eed61c3fcdac7788329917f764cb0f43523c9d800c68ee471fde21b171d19).
# Tuple fields are the original LE engine at44 and team flag48 bit0.
ORIGINAL_PRESETS = {
    "Slayer": (2, False), "Team Slayer": (2, True), "Rockets": (2, False),
    "Swords": (2, False), "Snipers": (2, False), "Phantoms": (2, False),
    "Team Phantoms": (2, True), "Elimination": (2, False), "Phantom Elim": (2, False),
    "Oddball": (3, False), "Rocketball": (3, False), "Swordball": (3, False),
    "Team Ball": (3, True), "Low Ball": (3, True), "King": (4, False),
    "Team King": (4, True), "Phantom King": (4, False), "Crazy King": (4, False),
    "Team Crazy King": (4, True), "2 on 1": (7, False), "Multi Flag CTF": (1, True),
    "CTF Classic": (1, True), "1 Flag CTF": (1, True), "1 Flag CTF Fast": (1, True),
    "Multi Bomb": (9, True), "Single Bomb": (9, True), "Single Bomb Fast": (9, True),
    "Neutral Bomb": (9, True), "3 Plots": (8, True), "Land Grab": (8, True),
    "3 on 1": (7, False), "Ninjanaut": (7, False), "Gold Rush": (8, True),
    "Fiesta": (3, False), "Blast Resort": (9, True), "Phantom Fodder": (7, False),
    "Dreadnaut": (7, False), "Control Issues": (8, True), "Contention": (8, True),
}


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
        """)
        consensus_schema = """CREATE TABLE IF NOT EXISTS match_result_consensus(
          match TEXT PRIMARY KEY REFERENCES match_results(id),
          policy TEXT NOT NULL CHECK(policy IN ('native-survivors-v1','native-unanimous-v1')),
          admitted_connections INTEGER NOT NULL, confirming_connections INTEGER NOT NULL,
          departed_slots INTEGER NOT NULL, authority_epoch INTEGER NOT NULL)"""
        old_schema = store.db.execute(
            "SELECT sql FROM sqlite_master WHERE type='table' AND name='match_result_consensus'").fetchone()
        if old_schema and "native-unanimous-v1" not in old_schema[0]:
            # Preserve historical survivor-policy receipts without relabeling
            # them as consent from every originally admitted connection.
            with store.db:
                store.db.execute(consensus_schema.replace('match_result_consensus', 'match_result_consensus_v2'))
                store.db.execute("INSERT INTO match_result_consensus_v2 SELECT * FROM match_result_consensus")
                store.db.execute("DROP TABLE match_result_consensus")
                store.db.execute("ALTER TABLE match_result_consensus_v2 RENAME TO match_result_consensus")
        else:
            store.db.execute(consensus_schema)
        from .carnage_history import ReportHistory
        self.history = ReportHistory(store)
        from .player_statistics import initialize
        initialize(store.db)

    @staticmethod
    def validate(room, report):
        require(isinstance(report, str) and len(report) == 2 * 0xdc78
                and all(c in "0123456789abcdef" for c in report), "Invalid postgame report")
        raw = bytes.fromhex(report)
        engine = struct.unpack_from("<I", raw, 0x64)[0]
        preset = ORIGINAL_PRESETS.get(room.proposal.game.variant)
        # Preserve the existing explicitly Slayer-only custom-name policy and
        # its actual admitted team setting. Unknown names gain no other engine.
        expected_engine = preset[0] if preset else 2
        expected_team = preset[1] if preset and expected_engine != 2 else None
        require(raw[0:2] == b"\x01\x01" and raw[0x10:0x12] == b"\x01\x01"
                and raw[0x13] == int(room.team_mode) and engine == expected_engine
                and (expected_team is None or room.team_mode == expected_team)
                and (raw[0x68] & 1) == int(room.team_mode), "Unfinished admitted multiplayer postgame report")
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
        require(room.confirmations == room.admitted_owners,
                "All admitted physical connections must confirm the result")
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
            if getattr(room, "supports_continuity", False) or (
                    getattr(room, "direct_udp", False) and room.departed_slots):
                db.execute("INSERT INTO match_result_consensus VALUES(?,?,?,?,?,?)",
                           (room.id, "native-unanimous-v1", len(room.admitted_owners),
                            len(room.confirmations), room.departed_slots, room.continuity.epoch if room.continuity else 0))
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
            from .player_statistics import enqueue
            for member,_,_ in room.players:
                if member not in getattr(room,"guests",()):
                    enqueue(db,member)
        logging.getLogger("halo2.results").info(json.dumps({"event": "carnage_report_committed",
            "match": public, "format": "native-verified-summary-v2",
            "published_players": len(room.players)-len(getattr(room, "guests", ())) }))


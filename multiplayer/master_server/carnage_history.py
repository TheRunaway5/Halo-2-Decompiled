"""Normalized, guest-filtered PCR history and read-only website API.

Offsets/index ranges are the original Vista PCR consumers in
src/vista_pcr_results.cpp and src/vista_pcr_publication.cpp. Unknown statistic
semantics retain their source indices; summaries are never expanded into fake
medal, weapon or opponent detail. Discord subjects and credentials stay private.
"""
import base64
import binascii
import json
import re

from aiohttp import web
from .native_database import run as database
from .protocol import identifier, integer, parse, require

FORMAT = "native-verified-summary-v2"
ACCURACY_FORMAT = "native-verified-accuracy-v3"


class ReportHistory:
    """Public history reads stay here; Rust owns writes and schema migration."""
    def __init__(self, store):
        self.store, self.db = store, store.db
        database(self.db, 'history_schema')

    def identity(self, player):
        return database(self.db, 'history_identity', player=player)


def query(request, allowed):
    require(set(request.query) <= set(allowed) and len(request.query) == len(set(request.query)), "Unexpected query fields")


def cursor(completed, public):
    return base64.urlsafe_b64encode(json.dumps([completed, public], separators=(",", ":")).encode()).decode().rstrip("=")


class HistoryApi:
    def __init__(self, directory):
        self.directory, self.db = directory, directory.store.db

    def player(self, public):
        identifier(public)
        row = self.db.execute("SELECT h.player,p.discord_username,p.screen_name FROM history_players h "
                              "JOIN player_profiles p ON p.player=h.player WHERE h.public_id=?", (public,)).fetchone()
        require(row is not None, "Player history not found", 404)
        return row

    async def search(self, request):
        query(request, ("discord_username",))
        username = request.query.get("discord_username", "").lower()
        require(re.fullmatch(r"[a-z0-9_.]{2,32}", username), "Use an exact Discord username")
        rows = self.db.execute("SELECT h.public_id,p.discord_username,p.screen_name FROM history_players h "
            "JOIN player_profiles p ON p.player=h.player WHERE p.discord_username!='' AND p.discord_username=?", (username,)).fetchall()
        return web.json_response({"players": [dict(id=r[0], discord_username=r[1], screen_name=r[2]) for r in rows]})

    async def matches(self, request):
        query(request, ("limit", "before"))
        who, username, name = self.player(request.match_info["player"])
        limit = request.query.get("limit", "20")
        require(re.fullmatch(r"[0-9]{1,2}", limit), "Invalid page size")
        limit = integer(int(limit), 1, 50)
        cutoff, parameters = "", [who]
        if "before" in request.query:
            value = request.query["before"]
            require(re.fullmatch(r"[A-Za-z0-9_-]{1,128}", value), "Invalid history cursor")
            try:
                decoded = parse(base64.urlsafe_b64decode(value + "="*(-len(value) % 4)))
            except (binascii.Error, ValueError):
                require(False, "Invalid history cursor")
            require(type(decoded) is list and len(decoded) == 2, "Invalid history cursor")
            when, public = integer(decoded[0], 0, 9007199254740991), identifier(decoded[1])
            cutoff = " AND (m.completed<? OR (m.completed=? AND h.public_id<?))"
            parameters += [when, when, public]
        parameters.append(limit+1)
        rows = self.db.execute("SELECT h.public_id,m.completed,m.playlist,m.revision,m.map,m.variant,m.ranked,"
            "r.placement,r.score,r.kills,r.deaths,r.assists,r.suicides,h.format FROM match_result_players r "
            "JOIN match_results m ON m.id=r.match JOIN carnage_reports h ON h.match=m.id "
            "WHERE r.player=?"+cutoff+" ORDER BY m.completed DESC,h.public_id DESC LIMIT ?", parameters).fetchall()
        fields = ("id", "completed", "playlist", "revision", "map", "variant", "ranked", "placement", "score", "kills", "deaths", "assists", "suicides", "format")
        return web.json_response({"player": dict(id=request.match_info["player"], discord_username=username, **self.directory.profiles.view(who)),
            "matches": [dict(zip(fields, row)) for row in rows[:limit]],
            "next": cursor(rows[limit-1][1], rows[limit-1][0]) if len(rows)>limit else None})

    async def report(self, request):
        query(request, ())
        public = identifier(request.match_info["match"])
        row = self.db.execute("SELECT h.match,m.completed,m.playlist,m.revision,m.map,m.variant,m.ranked,m.validation,"
            "h.format,h.admitted_at,h.admitted_players,h.published_players,h.variant_bytes FROM carnage_reports h "
            "JOIN match_results m ON m.id=h.match WHERE h.public_id=?", (public,)).fetchone()
        require(row is not None, "Carnage report not found", 404)
        match = row[0]
        fields = ("completed", "playlist", "revision", "map", "variant", "ranked", "validation", "format", "admitted_at", "admitted_players", "published_players")
        result = dict(id=public, **dict(zip(fields, row[1:12])))
        result["variant_bytes"] = row[12].hex() if row[12] is not None else None
        result["guest_details"] = "excluded"
        result["players"] = []
        for r in self.db.execute("SELECT a.player,h.public_id,p.discord_username,p.screen_name,coalesce(c.screen_name,s.screen_name),coalesce(c.discord_username_at_match,s.discord_username_at_match),"
            "coalesce(c.slot,s.slot),c.appearance,a.placement,a.score,a.kills,a.deaths,a.assists,a.suicides,t.team,t.individual_placement "
            "FROM match_result_players a JOIN history_players h ON h.player=a.player "
            "JOIN player_profiles p ON p.player=a.player LEFT JOIN carnage_players c ON c.match=a.match AND c.player=a.player "
            "LEFT JOIN carnage_player_summaries s ON s.match=a.match AND s.player=a.player "
            "LEFT JOIN match_result_memberships t ON t.match=a.match AND t.player=a.player WHERE a.match=? ORDER BY coalesce(c.slot,s.slot),h.public_id", (match,)):
            who = r[0]
            player = dict(zip(("id", "discord_username", "screen_name", "screen_name_at_match", "discord_username_at_match", "slot"), r[1:7]))
            player["appearance"] = r[7].hex() if r[7] is not None else None
            player.update(zip(("placement", "score", "kills", "deaths", "assists", "suicides", "team", "individual_placement"), r[8:]))
            player["details_available"] = r[7] is not None
            for name, table, columns in (("general", "carnage_general_stats", "stat,value"),
                ("medals", "carnage_medals", "medal,count"), ("weapons", "carnage_weapon_stats", "weapon,field,value")):
                player[name] = [list(v) for v in self.db.execute("SELECT "+columns+" FROM "+table+" WHERE match=? AND player=? ORDER BY "+columns, (match, who))]
            player["opponents"] = [dict(player=v[0], values=[v[1],v[2]]) for v in self.db.execute(
                "SELECT h.public_id,o.field0,o.field1 FROM carnage_opponents o JOIN history_players h ON h.player=o.opponent "
                "WHERE o.match=? AND o.player=? ORDER BY h.public_id", (match,who))]
            result["players"].append(player)
        result["teams"] = [dict(team=t[0], placement=t[1], score=t[2]) for t in self.db.execute(
            "SELECT team,placement,score FROM match_result_teams WHERE match=? ORDER BY team", (match,))]
        for team in result["teams"]:
            team["general"] = [list(s) for s in self.db.execute("SELECT stat,value FROM carnage_team_stats WHERE match=? AND team=? ORDER BY stat", (match,team["team"]))]
        return web.json_response(result)

    def routes(self):
        return [web.get("/v1/history/players", self.search),
                web.get("/v1/history/players/{player}/matches", self.matches),
                web.get("/v1/history/matches/{match}", self.report)]

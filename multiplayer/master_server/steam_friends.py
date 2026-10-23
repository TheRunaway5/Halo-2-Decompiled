"""Private, bounded Steam friends lookup for the currently authenticated account."""
import asyncio
import re
import time
from aiohttp import web
from .protocol import require


class SteamFriends:
    def __init__(self, directory):
        self.directory = directory
        self.cache = {}
        self.lock = asyncio.Lock()

    def subject(self, player):
        row = self.directory.store.db.execute(
            "SELECT subject FROM player_identities WHERE player=? AND provider='steam'", (player,)).fetchone()
        require(row, "Sign in with Steam to view Steam friends", 403)
        return row[0]

    async def lookup(self, player):
        from .service import bounded_json
        subject = self.subject(player)
        key = self.directory.settings.steam_key
        require(key, "Steam friends are unavailable", 503)
        async with self.lock:
            now = time.monotonic()
            self.cache = {k:v for k,v in self.cache.items() if v[0]>now}
            if subject in self.cache:
                return self.cache[subject][1]
            require(len(self.cache)<1000, "Steam friends are busy; try again", 503)
            async with self.directory.session.get(
                "https://api.steampowered.com/ISteamUser/GetFriendList/v1/",
                params={"key":key,"steamid":subject,"relationship":"friend"},allow_redirects=False) as response:
                if response.status in (401,403):
                    result = {"state":"private","friends":[]}
                    self.cache[subject]=(now+60,result)
                    return result
                require(response.status==200, "Steam friends are temporarily unavailable",502)
                body = await bounded_json(response)
            rows = body.get("friendslist",{}).get("friends",[])
            require(type(rows) is list and len(rows)<=10000, "Invalid Steam friends response",502)
            subjects = {r.get("steamid", "") for r in rows if type(r) is dict and r.get("relationship")=="friend"}
            require(all(type(s) is str and re.fullmatch(r"[1-9][0-9]{16}",s) for s in subjects),"Invalid Steam friends response",502)
            # Web API summaries are limited to 100 identities per call. A
            # bounded first page avoids an unbounded upstream fanout.
            selected = sorted(subjects)[:1000]
            summaries = []
            for offset in range(0,len(selected),100):
                async with self.directory.session.get(
                    "https://api.steampowered.com/ISteamUser/GetPlayerSummaries/v2/",
                    params={"key":key,"steamids":",".join(selected[offset:offset+100])},allow_redirects=False) as response:
                    require(response.status==200,"Steam friends are temporarily unavailable",502)
                    body=await bounded_json(response)
                batch=body.get("response",{}).get("players",[])
                require(type(batch) is list and len(batch)<=100,"Invalid Steam profiles response",502)
                summaries.extend(batch)
            friends=[]; seen=set()
            for row in summaries:
                if type(row) is not dict: continue
                sid=row.get("steamid"); name=row.get("personaname"); online=row.get("personastate")
                if sid not in subjects or sid in seen: continue
                seen.add(sid)
                if type(name) is not str or not name or len(name.encode())>128 or any(ord(c)<32 or ord(c)==127 for c in name): continue
                if type(online) is not int or not 1<=online<=6: continue
                friends.append({"id":sid,"name":name})
            result={"state":"ready","friends":friends,"truncated":len(subjects)>len(selected)}
            self.cache[subject]=(now+60,result)
            return result

    async def get(self, request):
        self.directory.rate("steam-friends:"+self.directory.source(request),30)
        _,who=self.directory.clans.session(request)
        result=await self.lookup(who)
        from .player_presence import observed
        present=observed(self.directory); db=self.directory.store.db
        friends=[]
        for row in result["friends"]:
            target=db.execute("SELECT player FROM player_identities WHERE provider='steam' AND subject=?",(row["id"],)).fetchone()
            if target and db.execute("SELECT 1 FROM party_blocks WHERE (player=? AND inviter=?) OR (player=? AND inviter=?)",(target[0],who,who,target[0])).fetchone(): continue
            friends.append({**row,"registered":bool(target),"halo_online":bool(target and target[0] in present)})
        return web.json_response({**result,"friends":sorted(friends,key=lambda r:(not r["halo_online"],r["name"].casefold(),r["id"]))},headers={"Cache-Control":"no-store"})

    def routes(self):
        return [web.get("/v1/account/steam-friends",self.get)]

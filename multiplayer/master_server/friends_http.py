"""Game-owned friendships with explicit acceptance and bilateral blocking."""
import time

from aiohttp import web
from . import friend_tags
from .protocol import canonical, fields, identifier, require, text

MAX_PENDING = 32


class FriendsApi:
    def __init__(self, directory, *, wall=time.time):
        self.directory, self.db, self.wall = directory, directory.store.db, wall
        self.db.executescript("""
        CREATE TABLE IF NOT EXISTS player_friendships(
          low TEXT NOT NULL REFERENCES players(id), high TEXT NOT NULL REFERENCES players(id),
          requester TEXT NOT NULL REFERENCES players(id), accepted INTEGER NOT NULL CHECK(accepted IN (0,1)),
          CHECK(low<high AND requester IN (low,high)), PRIMARY KEY(low,high));
        CREATE INDEX IF NOT EXISTS player_friendships_high ON player_friendships(high);
        CREATE INDEX IF NOT EXISTS player_friend_pending_requester ON player_friendships(requester) WHERE accepted=0;
        CREATE TABLE IF NOT EXISTS player_friend_blocks(
          player TEXT NOT NULL REFERENCES players(id), target TEXT NOT NULL REFERENCES players(id),
          CHECK(player!=target), PRIMARY KEY(player,target));
        CREATE INDEX IF NOT EXISTS player_friend_blocks_target ON player_friend_blocks(target);
        CREATE TABLE IF NOT EXISTS player_friend_commands(
          player TEXT NOT NULL REFERENCES players(id), request TEXT NOT NULL,
          payload TEXT NOT NULL, expires INTEGER NOT NULL, PRIMARY KEY(player,request));
        CREATE INDEX IF NOT EXISTS player_friend_commands_expiry ON player_friend_commands(expires);
        """)
        self.blocks = {(row[0],row[1]) for row in self.db.execute("SELECT player,target FROM player_friend_blocks")}

    def blocked(self, a, b):
        # Voice checks run for every listener on every packet. Keep this bounded
        # persisted preference in memory and publish changes only after commit.
        return (a,b) in self.blocks or (b,a) in self.blocks

    def accepted(self, a, b):
        return not self.blocked(a,b) and bool(self.db.execute(
            "SELECT 1 FROM player_friendships WHERE low=? AND high=? AND accepted=1", tuple(sorted((a,b)))).fetchone())

    def public_player(self, who, *, presence="offline", state=None):
        row = self.db.execute("SELECT h.public_id,p.screen_name FROM history_players h "
            "JOIN player_profiles p ON p.player=h.player WHERE h.player=?", (who,)).fetchone()
        require(row, "Player not found", 404)
        return dict(id=row[0],name=row[1],friend_tag=friend_tags.tag(self.db,who),presence=presence,
                    **({"state":state} if state else {}))

    def target(self, public):
        row = self.db.execute("SELECT player FROM history_players WHERE public_id=?", (identifier(public),)).fetchone()
        require(row, "Player not found", 404)
        return row[0]

    def view(self, who, *, party=False):
        from .player_presence import observed
        present = observed(self.directory)
        # One indexed projection, rather than per-friend name/tag/block queries.
        stored = self.db.execute("""WITH relationships(target,state) AS (
            SELECT CASE WHEN low=:who THEN high ELSE low END,
              CASE WHEN accepted=1 THEN 'friend' WHEN requester=:who THEN 'outgoing' ELSE 'incoming' END
            FROM player_friendships WHERE low=:who OR high=:who
            UNION ALL SELECT target,'blocked' FROM player_friend_blocks WHERE player=:who)
          SELECT r.target,r.state,h.public_id,p.screen_name,t.discriminator
          FROM relationships r JOIN player_profiles p ON p.player=r.target
          JOIN history_players h ON h.player=r.target
          LEFT JOIN player_friend_tags t ON t.player=r.target
          WHERE r.state='blocked' OR NOT EXISTS(SELECT 1 FROM player_friend_blocks b
            WHERE (b.player=:who AND b.target=r.target) OR (b.player=r.target AND b.target=:who))""",{"who":who})
        rows = [dict(id=public,name=name,friend_tag=f"{name}#{suffix}" if suffix else None,state=state,
                     presence=present.get(target,"offline") if state=="friend" else "offline",
                     **({"party_joinable":state=="friend" and self.directory.parties.friend_joinable(who,target),
                         "party_context":self.directory.parties.friend_context(who,target) if state=="friend" else None} if party else {}))
                for target,state,public,name,suffix in stored]
        rows.sort(key=lambda r: (r["state"]!="incoming",r["state"]=="blocked",r["presence"]=="offline",friend_tags.fold(r["name"]),r["id"]))
        cloud=getattr(self.directory,'friend_cloud',None)
        return dict(friend_tag=friend_tags.tag(self.db,who),friends=rows,
                    **({'cloud':cloud.status(who)} if cloud else {}))

    async def get(self, request):
        _,who = self.directory.clans.session(request)
        return web.json_response(self.view(who,party=request.query.get("party")=="1"),headers={"Cache-Control":"no-store"})

    def capacity(self, who):
        total = self.db.execute("SELECT count(*) FROM player_friendships WHERE low=? OR high=?", (who,who)).fetchone()[0]
        total += self.db.execute("SELECT count(*) FROM player_friend_blocks WHERE player=?", (who,)).fetchone()[0]
        require(total < 250, "Friend list is full", 409)

    async def command(self, request):
        # Reject unauthenticated uploads before reading their bodies, then
        # revalidate after the await before consulting or changing social data.
        self.directory.clans.session(request)
        body = await self.directory.body(request)
        fields(body,("request_id","action"),("tag","target"))
        _,who = self.directory.clans.session(request)
        rid,action = identifier(body["request_id"]),text(body["action"])
        require(action in ("search","request","accept","decline","cancel","remove","block","unblock"),"Unknown friend action")
        fields(body,("request_id","action","tag" if action=="search" else "target"))
        if action=="search":
            target = friend_tags.resolve(self.db,body["tag"])
            require(target!=who and not self.blocked(who,target),"Player not found",404)
            return web.json_response(dict(**self.view(who,party=request.query.get("party")=="1"),search_result=self.public_player(target)),headers={"Cache-Control":"no-store"})
        payload = canonical(body).decode()
        with self.db:
            self.db.execute("DELETE FROM player_friend_commands WHERE expires<=?",(int(self.wall()),))
            previous = self.db.execute("SELECT payload FROM player_friend_commands WHERE player=? AND request=?",(who,rid)).fetchone()
            if previous:
                require(previous[0]==payload,"Friend command ID reused differently",409)
                return web.json_response(self.view(who,party=request.query.get("party")=="1"),headers={"Cache-Control":"no-store"})
            require(self.db.execute("SELECT count(*) FROM player_friend_commands WHERE player=?",(who,)).fetchone()[0]<256,
                    "Friend command limit reached; try again later",429)
            target = self.target(body["target"])
            require(target!=who,"Choose another player")
            pair = tuple(sorted((who,target)))
            relation = self.db.execute("SELECT requester,accepted FROM player_friendships WHERE low=? AND high=?",pair).fetchone()
            if action=="block":
                self.db.execute("DELETE FROM player_friendships WHERE low=? AND high=?",pair)
                if not self.db.execute("SELECT 1 FROM player_friend_blocks WHERE player=? AND target=?",(who,target)).fetchone():
                    self.capacity(who)
                    require(self.db.execute("SELECT count(*) FROM player_friend_blocks").fetchone()[0]<100000,"Friend service is full",503)
                    self.db.execute("INSERT INTO player_friend_blocks VALUES(?,?)",(who,target))
                self.db.execute("DELETE FROM party_invites WHERE (inviter=? AND target=?) OR (inviter=? AND target=?)",(who,target,target,who))
                self.db.execute("DELETE FROM clan_invites WHERE (inviter=? AND target=?) OR (inviter=? AND target=?)",(who,target,target,who))
            elif action=="unblock":
                self.db.execute("DELETE FROM player_friend_blocks WHERE player=? AND target=?",(who,target))
            else:
                require(not self.blocked(who,target),"Friend action unavailable",404)
                if action=="request":
                    require(not relation,"Friend request or friendship already exists",409)
                    require(self.db.execute("SELECT count(*) FROM player_friendships WHERE requester=? AND accepted=0",(who,)).fetchone()[0]<MAX_PENDING,
                            "Too many outgoing friend requests",429)
                    require(self.db.execute("SELECT count(*) FROM player_friendships WHERE (low=? OR high=?) AND requester!=? AND accepted=0",(target,target,target)).fetchone()[0]<MAX_PENDING,
                            "Player is not accepting more friend requests",429)
                    self.capacity(who);self.capacity(target)
                    require(self.db.execute("SELECT count(*) FROM player_friendships").fetchone()[0]<100000,"Friend service is full",503)
                    self.db.execute("INSERT INTO player_friendships VALUES(?,?,?,0)",(*pair,who))
                else:
                    require(relation,"Friend request or friendship no longer available",404)
                    requester,accepted = relation
                    require((action=="remove" and accepted) or
                            (action in ("accept","decline") and not accepted and requester!=who) or
                            (action=="cancel" and not accepted and requester==who),"Friend action unavailable",409)
                    if action=="accept":
                        self.db.execute("UPDATE player_friendships SET accepted=1 WHERE low=? AND high=?",pair)
                    else:
                        self.db.execute("DELETE FROM player_friendships WHERE low=? AND high=?",pair)
            self.db.execute("INSERT INTO player_friend_commands VALUES(?,?,?,?)",(who,rid,payload,int(self.wall())+86400))
            cloud=getattr(self.directory,'friend_cloud',None)
            if cloud:
                cloud.enqueue(who);cloud.enqueue(target)
        if action=="block":
            self.blocks.add((who,target))
        elif action=="unblock":
            self.blocks.discard((who,target))
        return web.json_response(self.view(who,party=request.query.get("party")=="1"),headers={"Cache-Control":"no-store"})

    def routes(self):
        return [web.get("/v1/friends",self.get),web.post("/v1/friends/commands",self.command)]

"""Real native Vista UI -> HTTP clan service -> independent second player."""
import asyncio
import contextlib
import os
from pathlib import Path
import sys
import tempfile
import uuid
from frontend_test_ports import callback_port
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT))
try:
    from aiohttp.test_utils import TestClient, TestServer
    from multiplayer.master_server.service import Settings, create_app
    from multiplayer.master_server.protocol import Problem
    from multiplayer.master_server.player_identity import VerifiedOwner
except ImportError:
    sys.exit(77)

async def main():
    if "--capture" in sys.argv:
        capture_path=Path(sys.argv[sys.argv.index("--capture")+1])
        for marker in ("request-full-roster","full-roster-complete"):
            (capture_path/marker).unlink(missing_ok=True)
    with tempfile.TemporaryDirectory(prefix="halo2-clan-ui-") as temp:
        verified=[]
        async def verify(session,token,app):
            if token not in ("t","peer-token"): raise Problem(401,"Fixture authorization denied")
            verified.append(token)
            return VerifiedOwner("discord","123456789012345678" if token=="t" else "999999999999999999",
                                 "verified_user" if token=="t" else "peer_pilot")
        app=create_app(Settings(state=Path(temp),public_url="http://127.0.0.1:8080",allow_http=True,rate_per_minute=100000),verify)
        api=next(iter(app.values())).clans
        async with TestClient(TestServer(app)) as client:
            response=await client.post("/v1/account/session",headers={"Authorization":"Bearer peer-token"},json={"screen_name":"Peer Pilot"})
            peer={"Authorization":"Bearer "+(await response.json())["token"]}
            async def command(action,**kw):
                response=await client.post("/v1/clans/commands",headers=peer,json=dict(request_id=str(uuid.uuid4()),action=action)|kw)
                assert response.status==200,await response.text()
            async def partner():
                joined=False; reinvited=False
                capture_path=Path(sys.argv[sys.argv.index("--capture")+1]) if "--capture" in sys.argv else None
                seeded=[];stress_done=False
                while True:
                    response=await client.get("/v1/clans",headers=peer)
                    assert response.status==200,await response.text()
                    view=await response.json()
                    if capture_path and not stress_done and (capture_path/"request-full-roster").exists() and not seeded and view["clan"]:
                        from multiplayer.master_server.player_identity import player
                        cid=api.db.execute("SELECT m.clan FROM clan_members m JOIN player_profiles p ON p.player=m.player WHERE p.discord_username='peer_pilot'").fetchone()[0]
                        with api.db:
                            for index in range(98):
                                name="W"*31 if index>=96 else "Clan Member "+str(index).zfill(3)
                                who=player(api.db,("discord","layout-fixture-"+str(index)),name)
                                api.db.execute("INSERT INTO clan_members VALUES(?,?,0,?)",(who,cid,100000+index));seeded.append(who)
                    if seeded and (capture_path/"full-roster-complete").exists():
                        with api.db:
                            api.db.executemany("DELETE FROM clan_members WHERE player=?",((who,) for who in seeded))
                        seeded=[];stress_done=True
                    if not joined and view["invitations"]:
                        await command("accept",target=view["invitations"][0]["id"]); joined=True
                    if joined and not reinvited and view["clan"] and len(view["clan"]["members"])==1:
                        assert view["clan"]["role"]==3
                        await command("invite",username="verified_user"); reinvited=True
                    await asyncio.sleep(.5)
            task=asyncio.create_task(partner())
            env=os.environ|dict(http_proxy="http://127.0.0.1:1",HTTP_PROXY="http://127.0.0.1:1",NO_PROXY="",no_proxy="",SDL_AUDIODRIVER="dummy")
            process=await asyncio.create_subprocess_exec(str(Path(sys.argv[1]).resolve()),str(ROOT/"assets/disc/maps"),
                "--asset-sources",str(ROOT/"assets/asset-sources.tsv"),"--account-url",str(client.make_url("/")).rstrip("/"),"--callback-port",callback_port(),*sys.argv[2:],env=env,
                stdout=asyncio.subprocess.PIPE,stderr=asyncio.subprocess.PIPE)
            try:
                out,err=await asyncio.wait_for(process.communicate(),300)
                print(out.decode(),end=""); print(err.decode(),end="",file=sys.stderr)
                assert process.returncode in (0,77),"Native clan flow failed"
                if process.returncode==77:return 77
                assert verified.count("t")>=2,"Backend did not verify login/name change"
                assert len(api.sessions)==1,"Native account bearer survived sign-out"
                await command("leave")
                assert api.db.execute("SELECT count(*) FROM clans").fetchone()[0]==0
                print("PASS native Discord account gate, clan create/invite/join/roles/Overlord confirmation/messages/leave, chosen screen name, sign-out, privacy and Vista HD assets")
            finally:
                task.cancel()
                with contextlib.suppress(asyncio.CancelledError):await task
                if process.returncode is None:process.kill();await process.wait()
    return 0
if __name__=="__main__":sys.exit(asyncio.run(main()))

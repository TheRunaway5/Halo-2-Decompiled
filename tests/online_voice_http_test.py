"""Two native Opus clients through the real authenticated aiohttp relay."""
import asyncio
import argparse
from contextlib import asynccontextmanager
import json
import os
import socket
from pathlib import Path
import sys
import tempfile
import uuid
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT))
try:
    from aiohttp.test_utils import TestClient, TestServer
    from multiplayer.master_server.service import Settings, create_app
    from multiplayer.master_server.player_identity import VerifiedOwner
except ImportError:
    sys.exit(77)


@asynccontextmanager
async def voice_origin(client, args, temp):
    upstream = str(client.make_url("/")).rstrip("/")
    if not args.caddy:
        yield upstream
        return
    with socket.socket() as reservation:
        reservation.bind(("127.0.0.1", 0))
        port = reservation.getsockname()[1]
    origin = f"http://127.0.0.1:{port}"
    config = Path(temp)/"Caddyfile"
    config.write_text("{\n admin off\n auto_https off\n}\n" +
        args.proxy_config.read_text().replace("directory:8080", upstream.removeprefix("http://")))
    with (Path(temp)/"caddy.log").open("w+") as log:
        proxy = await asyncio.create_subprocess_exec(str(args.caddy.resolve()), "run", "--config", str(config),
            "--adapter", "caddyfile", env=os.environ|{"MASTER_DOMAIN":origin},
            stdout=log, stderr=asyncio.subprocess.STDOUT)
        try:
            for attempt in range(100):
                if proxy.returncode is not None:
                    raise RuntimeError("Caddy stopped during startup")
                try:
                    async with client.session.get(origin+"/v1/health") as response:
                        if response.status == 200: break
                except OSError: pass
                await asyncio.sleep(.02)
            else: raise RuntimeError("Caddy proxy did not become ready")
            yield origin
        finally:
            if proxy.returncode is None: proxy.terminate()
            await proxy.wait()


async def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("native")
    parser.add_argument("group", nargs="?", choices=("clan", "party"), default="clan")
    parser.add_argument("--caddy", type=Path, help="Exercise the production reverse proxy too")
    parser.add_argument("--proxy-config", type=Path, default=ROOT/"multiplayer/master_server/Caddyfile")
    parser.add_argument("--idle-seconds", type=int, default=32)
    args = parser.parse_args()
    party = args.group == "party"
    with tempfile.TemporaryDirectory(prefix="halo2-voice-") as temp:
        async def verify(session,token,app):return VerifiedOwner("discord",token,"voice_"+token)
        app=create_app(Settings(state=Path(temp)/"state",public_url="http://127.0.0.1:8080",allow_http=True),verify)
        async with TestClient(TestServer(app)) as client, voice_origin(client,args,temp) as origin:
            credentials={}
            for who in ("a","b"):
                response=await client.post("/v1/account/session",headers={"Authorization":"Bearer "+who},json={"screen_name":"Pilot "+who})
                assert response.status==201,await response.text()
                credentials[who]=(await response.json())["token"]
            async def command(who,action,**kw):
                response=await client.post("/v1/party/commands" if party else "/v1/clans/commands",headers={"Authorization":"Bearer "+credentials[who]},json=dict(request_id=str(uuid.uuid4()),action=action)|kw)
                assert response.status==200,await response.text()
                return await response.json()
            await command("a","create",**({} if party else dict(name="Voice Test")))
            await command("a","invite",username="voice_b",**(dict(version=1) if party else {}))
            response=await client.get("/v1/party" if party else "/v1/clans",headers={"Authorization":"Bearer "+credentials["b"]})
            invitation=(await response.json())["invitations"][0]
            view=await command("b","accept",target=invitation["id"],**(dict(version=invitation["version"]) if party else {}))
            group=view["party" if party else "clan"]
            config={"origin":origin,"clan":"party" if party else group["id"],"idle_seconds":args.idle_seconds,"party":party}
            for who in ("a","b"):
                config[who]={"token":credentials[who],"player":next(m["handle" if party else "id"] for m in group["members"] if m["name"]=="Pilot "+who)}
            path=Path(temp)/"voice.json";path.write_text(json.dumps(config));path.chmod(0o600)
            env=os.environ|dict(http_proxy="http://127.0.0.1:1",HTTP_PROXY="http://127.0.0.1:1",ALL_PROXY="http://127.0.0.1:1")
            process=await asyncio.create_subprocess_exec(args.native,str(path),env=env,stdout=asyncio.subprocess.PIPE,stderr=asyncio.subprocess.PIPE)
            try:
                out,err=await asyncio.wait_for(process.communicate(),args.idle_seconds+28)
                print(out.decode(),end="");print(err.decode(),end="",file=sys.stderr)
                assert process.returncode==0,"Native voice relay check failed"
            finally:
                if process.returncode is None:process.kill();await process.wait()
    return 0
if __name__=="__main__":sys.exit(asyncio.run(main()))

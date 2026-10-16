"""Native HTTPS-owner clients over real loopback HTTP; Steam alone is a fixture.

The independent HMAC fixture validates the exact signed OpenID fields forwarded
by the service. This is wire integration evidence, never a real Steam login.
"""
import asyncio
import base64
from datetime import datetime, timezone
import hashlib
import hmac
import json
import os
from pathlib import Path
import sys
import tempfile
import time
from urllib.parse import parse_qs, urlsplit
from unittest.mock import patch

from aiohttp import ClientSession
from aiohttp.test_utils import TestServer
from multiplayer.master_server.service import Settings, create_app
from multiplayer.master_server.steam_login import OP, NS
from multiplayer.master_server.tests.test_steam_login import Response
from multiplayer.master_server.tests.test_matchmaking_http import catalog


async def main(binary):
    with tempfile.TemporaryDirectory() as temp:
        root=Path(temp); (root/'catalog.json').write_text(json.dumps(catalog()))
        app=create_app(Settings(state=root, allow_http=True, public_url='http://127.0.0.1:1',
                                steam_enabled=True, matchmaking_catalog=str(root/'catalog.json')))
        server=TestServer(app);await server.start_server()
        directory=next(iter(app.values()));directory.settings.public_url=str(server.make_url('')).rstrip('/')
        key=b'independent-native-wire-fixture'
        def signature(values):
            signed=''.join(k+':'+values['openid.'+k]+'\n' for k in values['openid.signed'].split(','))
            return base64.b64encode(hmac.new(key,signed.encode(),hashlib.sha256).digest()).decode()
        def upstream(url,*,data,allow_redirects):
            assert url==OP and allow_redirects is False and data['openid.mode']=='check_authentication'
            valid=hmac.compare_digest(data['openid.sig'],signature(data))
            return Response(('ns:'+NS+'\nis_valid:'+('true' if valid else 'false')+'\n').encode())
        task=None
        try:
            with patch.object(directory.session,'post',upstream):
                output=root/'authorization.txt'
                task=await asyncio.create_subprocess_exec(str(Path(binary).resolve()),str(output),'Chosen Steam Pilot',
                    stdout=asyncio.subprocess.PIPE,stderr=asyncio.subprocess.STDOUT,
                    env=dict(os.environ,H2_ACCOUNT_URL=directory.settings.public_url,H2_ACCOUNT_ALLOW_LOOPBACK_HTTP='1'))
                for _ in range(100):
                    if output.exists() and output.stat().st_size:break
                    if task.returncode is not None:raise RuntimeError('Native sign-in terminated before browser handoff')
                    await asyncio.sleep(.05)
                values=parse_qs(urlsplit(output.read_text()).query)
                callback=values['openid.return_to'][0]
                nonce=datetime.fromtimestamp(time.time(),timezone.utc).strftime('%Y-%m-%dT%H:%M:%SZ')+'native-wire'
                assertion={'openid.ns':NS,'openid.mode':'id_res','openid.op_endpoint':OP,
                    'openid.claimed_id':'https://steamcommunity.com/openid/id/76561197960287930',
                    'openid.identity':'https://steamcommunity.com/openid/id/76561197960287930',
                    'openid.return_to':callback,'openid.response_nonce':nonce,'openid.assoc_handle':'fixture',
                    'openid.signed':'op_endpoint,claimed_id,identity,return_to,response_nonce,assoc_handle'}
                assertion['openid.sig']=signature(assertion)
                async with ClientSession() as client:
                    async with client.get(callback,params=assertion) as response:
                        assert response.status==200, await response.text()
                stdout=await asyncio.wait_for(task.communicate(),10)
                if task.returncode:raise RuntimeError(stdout[0].decode())
                assert 'PASS' in stdout[0].decode()
                assert tuple(directory.store.db.execute('SELECT screen_name,steam_name FROM player_profiles').fetchone())==('Chosen Steam Pilot','Steam Player')
                assert not directory.clans.sessions, 'Native sign-out retained its account session'
                print('PASS native Steam proof-bound login, real HTTP account session, chosen name, clan read, sign-out; fixture Steam provider')
        finally:
            if task and task.returncode is None:task.kill();await task.wait()
            await server.close()

if __name__=='__main__':asyncio.run(main(sys.argv[1]))

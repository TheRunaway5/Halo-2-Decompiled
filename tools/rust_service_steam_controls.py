"""Owned Rust Steam browser/device/renewal controls; upstream Steam is a fixture.

All production parsing, authorization, pending state and SQL execute in the
unchanged-default Rust executable. Test provider IO is never a real Steam grant.
"""
import base64
import asyncio
from datetime import datetime,timezone
import hashlib
import hmac
import json
import os
from pathlib import Path
import sqlite3
import subprocess
import sys
import time
import uuid
from urllib.parse import urlencode,urlsplit,parse_qs

OP='https://steamcommunity.com/openid/login'
NS='http://specs.openid.net/auth/2.0'
KEY=b'independent-rust-service-openid-fixture'


def signature(values):
    text=''.join(k+':'+values['openid.'+k]+'\n' for k in values['openid.signed'].split(','))
    return base64.b64encode(hmac.new(KEY,text.encode(),hashlib.sha256).digest()).decode()


def qualify(binary,scratch,catalog,Server,native_probe=None):
    scratch.mkdir();state=scratch/'state';checks=[]
    if not sys.platform.startswith('linux'):
        return dict(qualified=False,checks=[],scope='The private provider interposer requires Linux; no Steam flow acceptance on this platform.')
    shim=scratch/'provider.so';source=Path(__file__).with_name('rust_service_steam_provider_fixture.cpp')
    command=['g++','-std=c++20','-O1','-shared','-fPIC',str(source),'-o',str(shim),'-ldl','-lcrypto']
    run=subprocess.run(command,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
    assert run.returncode==0,run.stdout.decode(errors='replace')[-3000:]
    dependencies=subprocess.run(['ldd',str(binary)],stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
    assert dependencies.returncode==0 and b'libcurl.so' in dependencies.stdout,'Default executable lacks the admitted dynamic provider IO seam'
    control=scratch/'provider.json';control.write_text(json.dumps({'name':'Steam Fixture'}));calls=scratch/'provider-calls.jsonl'
    environment=dict(LD_PRELOAD=str(shim),H2_STEAM_KEY='1'*32,H2_TEST_STEAM_PROVIDER_DATA=str(control),H2_TEST_STEAM_PROVIDER_CALLS=str(calls))
    server=Server(binary,state,scratch,catalog,environment=environment,proxy=True)
    try:
        observed=asyncio.run(core(server,state,control,calls,checks))
        server.close();server=Server(binary,state,scratch,catalog,environment=environment,proxy=True)
        capacity=asyncio.run(capacity_controls(server,checks))
        server.close();server=Server(binary,state,scratch,catalog,environment=environment,proxy=True)
        restart=asyncio.run(restart_controls(server,state,observed,checks))
        native=None
        if native_probe:native=asyncio.run(native_controls(server,state,scratch,native_probe,checks))
        return dict(qualified=True,checks=checks,observed={k:v for k,v in observed.items() if k not in ('refresh_a','refresh_b')},capacity=capacity,restart=restart,native=native,
            fixture=dict(compiler=command,sha256=hashlib.sha256(source.read_bytes()).hexdigest(),default_production_binary=True,policy_in_rust=True,upstream_interposed=True),
            real_steam=False,scope='Actual Rust browser/device/SQL/renewal HTTP with fixed-provider curl IO replaced by an independent private HMAC fixture; no real Steam grant, original or delivered/gameplay acceptance.')
    finally:server.close()


def configure(path,**values):
    temp=path.with_suffix('.tmp');temp.write_text(json.dumps({'name':'Steam Fixture',**values}));temp.replace(path)


def sql(state,statement,params=()):
    with sqlite3.connect(state/'registry.sqlite3') as db:return db.execute(statement,params).fetchall()


async def core(server,state,control,calls,checks):
    import aiohttp
    base=f'http://{server.host}:{server.port}';verifier='v'*43;code='A1B2C3D4E5F6';serial=0
    digest=lambda s:base64.urlsafe_b64encode(hashlib.sha256(s.encode()).digest()).decode().rstrip('=')
    def count_calls():return len(calls.read_text().splitlines()) if calls.exists() else 0
    async with aiohttp.ClientSession(cookie_jar=aiohttp.DummyCookieJar()) as client:
        async def http(method,path,status,*,body=None,form=None,token=None,headers=None):
            fields=dict(headers or {})
            if token:fields['Authorization']='Bearer '+token
            async with client.request(method,base+path,json=body,data=form,headers=fields,allow_redirects=False) as response:
                text=await response.text();value=json.loads(text) if response.headers.get('Content-Type','').startswith('application/json') else text
                assert response.status==status,(method,path.split('?')[0],response.status,status,value.get('error') if isinstance(value,dict) else text[:100])
                assert response.headers.get('Cache-Control')=='no-store'
                return value,response.headers.copy()
        async def start(source=None):
            nonlocal serial
            serial+=1
            value,_=await http('POST','/v1/auth/steam/start',201,body={'challenge':digest(verifier),'confirmation':digest(code)},headers={'X-Forwarded-For':source or f'2001:db8:{serial:x}::1'})
            assert value['expires_in']==300 and value['interval']==1 and len(value['device'])==43
            expected={'openid.ns':[NS],'openid.mode':['checkid_setup'],'openid.realm':[base+'/'],
                'openid.return_to':[base+'/v1/auth/steam/callback/'+value['device']],
                'openid.identity':[NS+'/identifier_select'],'openid.claimed_id':[NS+'/identifier_select']}
            assert parse_qs(urlsplit(value['url']).query)==expected
            return value['device']
        def assertion(device,subject='76561197960287930'):
            values={'openid.ns':NS,'openid.mode':'id_res','openid.op_endpoint':OP,
                'openid.claimed_id':'https://steamcommunity.com/openid/id/'+subject,'openid.identity':'https://steamcommunity.com/openid/id/'+subject,
                'openid.return_to':base+'/v1/auth/steam/callback/'+device,
                'openid.response_nonce':datetime.now(timezone.utc).strftime('%Y-%m-%dT%H:%M:%SZ')+device,
                'openid.assoc_handle':'fixture','openid.signed':'op_endpoint,claimed_id,identity,return_to,response_nonce,assoc_handle'}
            values['openid.sig']=signature(values);return values
        async def callback(device,values=None,status=303):
            return await http('GET','/v1/auth/steam/callback/'+device+'?'+urlencode(values or assertion(device)),status)
        async def confirm(device,headers,entered=code,status=303):
            cookie=headers['Set-Cookie'].split(';',1)[0]
            return await http('POST','/v1/auth/steam/confirm/'+device,status,form={'code':entered},headers={'Cookie':cookie,'Origin':base})
        async def complete(subject='76561197960287930'):
            device=await start();_,headers=await callback(device,assertion(device,subject));await confirm(device,headers)
            value,_=await http('POST','/v1/auth/steam/poll',200,body={'device':device,'verifier':verifier});assert value['state']=='signed_in'
            return device,value
        device=await start();pending,_=await http('POST','/v1/auth/steam/poll',200,body={'device':device,'verifier':verifier});assert pending=={'state':'pending'}
        configure(control,name='<b>Steam & "Pilot"</b>')
        _,headers=await callback(device)
        assert headers['Location']=='/v1/auth/steam/confirm/'+device and 'HttpOnly' in headers['Set-Cookie'] and 'SameSite=Strict' in headers['Set-Cookie']
        assert not sql(state,'SELECT token_hash FROM steam_login_sessions')
        value,_=await http('POST','/v1/auth/steam/poll',200,body={'device':device,'verifier':verifier});assert value=={'state':'awaiting_confirmation'}
        await http('GET','/v1/auth/steam/confirm/'+device,401)
        page,page_headers=await http('GET','/v1/auth/steam/confirm/'+device,200,headers={'Cookie':headers['Set-Cookie'].split(';',1)[0]})
        assert '&lt;b&gt;Steam &amp; &quot;Pilot&quot;&lt;/b&gt;' in page and '<b>Steam' not in page and 'frame-ancestors' in page_headers['Content-Security-Policy']
        await http('POST','/v1/auth/steam/confirm/'+device,403,form={'code':code},headers={'Cookie':headers['Set-Cookie'].split(';',1)[0],'Origin':'https://different.invalid'})
        await confirm(device,headers)
        await http('POST','/v1/auth/steam/poll',401,body={'device':device,'verifier':'x'*43})
        before=sql(state,'SELECT count(*) FROM steam_login_refresh')[0][0]
        with sqlite3.connect(state/'registry.sqlite3') as db:
            db.executescript("CREATE TABLE steam_parent(id TEXT PRIMARY KEY);CREATE TABLE steam_failure(id TEXT REFERENCES steam_parent(id) DEFERRABLE INITIALLY DEFERRED);CREATE TRIGGER fail_steam_poll AFTER INSERT ON steam_login_sessions BEGIN INSERT INTO steam_failure VALUES('missing');END;")
        await http('POST','/v1/auth/steam/poll',500,body={'device':device,'verifier':verifier})
        assert not sql(state,'SELECT token_hash FROM steam_login_sessions') and sql(state,'SELECT count(*) FROM steam_login_refresh')[0][0]==before
        sql(state,'DROP TRIGGER fail_steam_poll')
        a,_=await http('POST','/v1/auth/steam/poll',200,body={'device':device,'verifier':verifier});assert a['identity']['id']=='76561197960287930' and a['refresh_token'].startswith('h2r_')
        await http('POST','/v1/auth/steam/poll',401,body={'device':device,'verifier':verifier})
        assert a['token'] not in repr(sql(state,'SELECT * FROM steam_login_sessions')) and a['refresh_token'] not in repr(sql(state,'SELECT * FROM steam_login_refresh'))
        checks.append('real Rust native URL/device/PKCE and browser-cookie/code authorization, escaped confirmation HTML, wrong-origin/proof rejection and one-time handoff')
        checks.append('actual failed credential COMMIT rolls back active/remembered/mapping writes and preserves confirmed pending state; retry publishes once')
        configure(control)
        _,b=await complete('76561197960287931')
        account,_=await http('POST','/v1/account/session',201,token=a['token'],body={'screen_name':'Chosen Halo Name'})
        await http('GET','/v1/clans',200,token=account['token'])
        await http('POST','/v1/account/session',401,token=a['refresh_token'],body={'screen_name':'Forged refresh'})
        checks.append('Rust-issued Steam grant creates the existing canonical account/clan/profile namespace; saved renewal credentials cannot authorize account APIs')
        expiry=sql(state,'SELECT expires FROM steam_login_refresh WHERE token_hash=?',[hashlib.sha256(a['refresh_token'].encode()).hexdigest()])[0][0]
        sql(state,'UPDATE steam_login_sessions SET expires=? WHERE subject=?',[int(time.time())-1,a['identity']['id']])
        calls_before=count_calls()
        resumed,_=await http('POST','/v1/auth/steam/resume',200,token=a['refresh_token'],body={})
        assert resumed['identity']==a['identity'] and resumed['token']!=a['token'] and 'refresh_token' not in resumed and count_calls()==calls_before
        assert sql(state,'SELECT expires FROM steam_login_refresh WHERE token_hash=?',[hashlib.sha256(a['refresh_token'].encode()).hexdigest()])[0][0]==expiry
        with sqlite3.connect(state/'registry.sqlite3') as db:
            db.executescript("CREATE TRIGGER fail_steam_forget AFTER DELETE ON steam_login_refresh BEGIN INSERT INTO steam_failure VALUES('missing');END;")
        await http('POST','/v1/auth/steam/forget',500,token=a['refresh_token'],body={})
        assert sql(state,'SELECT count(*) FROM steam_login_sessions WHERE subject=?',[a['identity']['id']])[0][0]==1
        sql(state,'DROP TRIGGER fail_steam_forget')
        await http('POST','/v1/auth/steam/forget',200,token=a['refresh_token'],body={})
        await http('POST','/v1/auth/steam/forget',200,token=a['refresh_token'],body={})
        await http('POST','/v1/auth/steam/resume',401,token=a['refresh_token'],body={})
        await http('POST','/v1/account/session',401,token=resumed['token'],body={'screen_name':'Retired source'})
        await http('POST','/v1/auth/steam/resume',200,token=b['refresh_token'],body={})
        checks.append('renewal preserves the fixed thirty-day expiry without upstream IO; failed forget COMMIT rolls back, retry revokes only its issued sessions and is idempotent')
        retained,_=await http('POST','/v1/account/session',201,token='discord-fixture-token',body={'screen_name':'Retained Pilot'})
        await http('POST','/v1/clans/commands',200,token=retained['token'],body={'request_id':str(uuid.uuid4()),'action':'create','name':'Retained Clan'})
        target=sql(state,"SELECT player FROM player_identities WHERE provider='discord' AND subject='123456789012345678'")[0][0]
        bare,_=await http('POST','/v1/account/session',201,token=b['token'],body={'screen_name':'Bare Steam'})
        old=sql(state,"SELECT player FROM player_identities WHERE provider='steam' AND subject=?",[b['identity']['id']])[0][0]
        assert target!=old
        configure(control,discord_bad_app=True)
        await http('POST','/v1/account/link-discord',403,token=b['token'],body={'discord_token':'discord-fixture-token'})
        configure(control)
        with sqlite3.connect(state/'registry.sqlite3') as db:
            db.executescript("CREATE TRIGGER fail_steam_link AFTER INSERT ON player_identities WHEN NEW.provider='steam' BEGIN INSERT INTO steam_failure VALUES('missing');END;")
        await http('POST','/v1/account/link-discord',500,token=b['token'],body={'discord_token':'discord-fixture-token'})
        assert sql(state,"SELECT player FROM player_identities WHERE provider='steam' AND subject=?",[b['identity']['id']])[0][0]==old
        await http('GET','/v1/clans',200,token=bare['token'])
        sql(state,'DROP TRIGGER fail_steam_link')
        await http('POST','/v1/account/link-discord',200,token=b['token'],body={'discord_token':'discord-fixture-token'})
        await http('POST','/v1/account/link-discord',200,token=b['token'],body={'discord_token':'discord-fixture-token'})
        assert sql(state,"SELECT player FROM player_identities WHERE provider='steam' AND subject=?",[b['identity']['id']])[0][0]==target
        await http('GET','/v1/clans',401,token=bare['token'])
        retained_clan,_=await http('GET','/v1/clans',200,token=retained['token'])
        assert retained_clan['clan']['name']=='Retained Clan' and retained_clan['clan']['role']==3
        assert sql(state,'SELECT screen_name FROM player_profiles WHERE player=?',[target])[0][0]=='Retained Pilot'
        checks.append('real Rust dual-provider linking uses the shared canonical owner: wrong application is rejected, failed COMMIT retains both identities/grants, retry preserves the existing clan and revokes only the adopted bare account')
        _,late=await complete('76561197960287939')
        configure(control,discord_delay_ms=350)
        before=count_calls()
        pending=asyncio.create_task(http('POST','/v1/account/link-discord',401,token=late['token'],body={'discord_token':'discord-fixture-token'}))
        deadline=time.monotonic()+3
        while count_calls()==before and time.monotonic()<deadline:await asyncio.sleep(.01)
        assert count_calls()>before
        await http('POST','/v1/auth/steam/forget',200,token=late['refresh_token'],body={})
        await pending;configure(control)
        assert not sql(state,"SELECT player FROM player_identities WHERE provider='steam' AND subject=?",[late['identity']['id']])
        checks.append('Steam credential revocation during actual Discord provider IO is revalidated before canonical linking and creates no alias')
        _,waiting=await complete('76561197960287938')
        configure(control,discord_delay_ms=150,discord_subject='223456789012345678',discord_name='expired_pilot')
        before=count_calls()
        async def link_waiting():
            async with client.post(base+'/v1/account/link-discord',headers={'Authorization':'Bearer '+waiting['token']},json={'discord_token':'discord-fixture-token'}) as response:
                return response.status,await response.json()
        pending=asyncio.create_task(link_waiting());deadline=time.monotonic()+3
        while count_calls()==before and time.monotonic()<deadline:await asyncio.sleep(.01)
        assert count_calls()>before
        lock=sqlite3.connect(state/'registry.sqlite3',isolation_level=None)
        try:
            lock.execute('BEGIN IMMEDIATE');expires=int(time.time())+2
            lock.execute('UPDATE steam_login_sessions SET expires=? WHERE subject=?',[expires,waiting['identity']['id']])
            await asyncio.sleep(max(0,expires+.2-time.time()))
            lock.commit()
        finally:lock.close()
        status,result=await pending
        written=sql(state,"SELECT player FROM player_identities WHERE provider='steam' AND subject=?",[waiting['identity']['id']])
        assert status==401 and not written,('expired Steam credential after actual SQL wait',status,len(written),result.get('error'))
        configure(control)
        checks.append('actual BEGIN IMMEDIATE wait crosses Steam expiry and rejects the late canonical transaction without writing an alias')
        device=await start();values=assertion(device);before=count_calls()
        for patch in ({'openid.return_to':'https://different.invalid/callback'},{'openid.op_endpoint':'https://different.invalid'},
                      {'openid.identity':'https://steamcommunity.com/openid/id/76561197960287931'},
                      {'openid.signed':'op_endpoint,claimed_id,identity,return_to,assoc_handle'},
                      {'openid.response_nonce':'2026-02-31T12:00:00Zbad'}, {'openid.response_nonce':'1970-01-01T00:00:00Zexpired'}):
            await callback(device,values|patch,400)
        await http('GET','/v1/auth/steam/callback/'+device+'?'+urlencode(list(values.items())+[('openid.identity',values['openid.identity'])]),400)
        assert count_calls()==before
        _,headers=await callback(device,values)
        for _ in range(5):await confirm(device,headers,'0'*12,401)
        await confirm(device,headers,code,401)
        value,_=await http('POST','/v1/auth/steam/poll',200,body={'device':device,'verifier':verifier});assert value['state']=='failed'
        checks.append('wrong destination/identity/signed fields, duplicate query and invalid/stale nonce are rejected before provider IO; five wrong game codes retire confirmation')
        device=await start();values=assertion(device);values['openid.claimed_id']=values['openid.identity']='https://steamcommunity.com/openid/id/76561197960287939'
        await callback(device,values,401)
        assert not sql(state,'SELECT token_hash FROM steam_login_sessions WHERE subject=?',['76561197960287939'])
        checks.append('independent fixture HMAC rejects a modified signed Steam identity before any credential is issued')
        for mode,status in [('duplicate',502),('oversize',502),('encoded',502),('redirect',502),('wrong-profile',502)]:
            configure(control,mode=mode);device=await start();await callback(device,status=status)
        checks.append('actual curl boundary disables ambient proxy/redirects, verifies TLS, bounds and parses replies, rejects compression/duplicate fields/oversize and checks profile identity')
        configure(control);device=await start();values=assertion(device)
        with sqlite3.connect(state/'registry.sqlite3') as db:db.executescript("CREATE TRIGGER fail_steam_nonce AFTER INSERT ON steam_login_nonces BEGIN INSERT INTO steam_failure VALUES('missing');END;")
        await callback(device,values,500);assert not sql(state,'SELECT nonce FROM steam_login_nonces WHERE nonce=?',[values['openid.response_nonce']])
        sql(state,'DROP TRIGGER fail_steam_nonce')
        checks.append('failed nonce COMMIT leaves no replay record or browser authorization publication')
        device=await start();value,_=await http('GET','/v1/auth/steam/callback/'+device+'?openid.mode=cancel',303)
        value,_=await http('POST','/v1/auth/steam/poll',200,body={'device':device,'verifier':verifier});assert value['state']=='declined'
        checks.append('provider cancellation remains a proof-bound declined pending state without credential writes')
        configure(control)
        return dict(refresh_a=a['refresh_token'],refresh_b=b['refresh_token'],subject_b=b['identity']['id'],nonce_rows=sql(state,'SELECT count(*) FROM steam_login_nonces')[0][0],provider_calls=count_calls())


async def capacity_controls(server,checks):
    import aiohttp
    base=f'http://{server.host}:{server.port}';digest=base64.urlsafe_b64encode(hashlib.sha256(b'v'*43).digest()).decode().rstrip('=')
    async with aiohttp.ClientSession() as client:
        async def start(source,status):
            async with client.post(base+'/v1/auth/steam/start',json={'challenge':digest,'confirmation':digest},headers={'X-Forwarded-For':source}) as response:
                await response.read();assert response.status==status,('source capacity',source,response.status,status)
        for i in range(4):await start('2001:db8:1234::1',201)
        await start('2001:db8:1234::1',429)
        for i in range(4):await start('2001:db8:1234::2',201)
        await start('2001:db8:1234::3',429)
        for i in range(8):await start(f'2001:db8:{i:x}::1',201)
        await start('2001:db8:ff::1',429)
    checks.append('real trusted-proxy source/prefix occupancy caps4/8 preserve fresh sources; rejected occupancy leaves the independent sixteen-start global burst intact')
    return dict(per_source=4,per_prefix=8,accepted_global=16,forwarded_sources_are_private_proxy_fixtures=True)


async def restart_controls(server,state,observed,checks):
    import aiohttp
    async with aiohttp.ClientSession() as client:
        async with client.post(f'http://{server.host}:{server.port}/v1/auth/steam/resume',json={},headers={'Authorization':'Bearer '+observed['refresh_b']}) as response:
            value=await response.json();assert response.status==200 and value['identity']['id']==observed['subject_b']
        async with client.post(f'http://{server.host}:{server.port}/v1/auth/steam/resume',json={},headers={'Authorization':'Bearer '+observed['refresh_a']}) as response:assert response.status==401
    checks.append('real Rust process restart retains valid remembered grants and revocations while dropping private pending browser flows')
    return dict(persisted_refresh=True,revocation_persisted=True)


async def native_controls(server,state,scratch,probe,checks,expected_friends=None,completion_timeout=15):
    import aiohttp
    output=scratch/'native-authorization.txt';confirmation=scratch/'native-confirmation.txt'
    output.unlink(missing_ok=True);confirmation.unlink(missing_ok=True)
    env=dict(os.environ,H2_ACCOUNT_URL=f'http://{server.host}:{server.port}',H2_ACCOUNT_ALLOW_LOOPBACK_HTTP='1',H2_EOS_ENABLED='0',H2_EOS_PROFILES='0',H2_ONLINE_TRANSPORT='direct')
    command=[str(probe),str(output),'Native Steam Rust','--confirmation-file',str(confirmation)]
    if expected_friends is not None:command.extend(['--steam-friends',str(expected_friends)])
    task=await asyncio.create_subprocess_exec(*command,stdout=asyncio.subprocess.PIPE,stderr=asyncio.subprocess.STDOUT,env=env)
    try:
        deadline=time.monotonic()+12
        while not output.exists() and time.monotonic()<deadline:
            assert task.returncode is None,'Native client stopped before real Rust browser handoff';await asyncio.sleep(.02)
        values=parse_qs(urlsplit(output.read_text()).query);callback=values['openid.return_to'][0]
        assertion={'openid.ns':NS,'openid.mode':'id_res','openid.op_endpoint':OP,'openid.claimed_id':'https://steamcommunity.com/openid/id/76561197960287937',
            'openid.identity':'https://steamcommunity.com/openid/id/76561197960287937','openid.return_to':callback,
            'openid.response_nonce':datetime.now(timezone.utc).strftime('%Y-%m-%dT%H:%M:%SZ')+'native-rust-'+values['openid.return_to'][0].rsplit('/',1)[-1],
            'openid.assoc_handle':'native-fixture','openid.signed':'op_endpoint,claimed_id,identity,return_to,response_nonce,assoc_handle'}
        assertion['openid.sig']=signature(assertion)
        async with aiohttp.ClientSession(cookie_jar=aiohttp.CookieJar(unsafe=True)) as client:
            async with client.get(callback,params=assertion) as response:assert response.status==200;path=str(response.url);await response.read()
            while (not confirmation.exists() or confirmation.stat().st_size!=12) and time.monotonic()<deadline:await asyncio.sleep(.02)
            code=confirmation.read_text();confirmation.unlink()
            async with client.post(path,data={'code':code},headers={'Origin':f'http://{server.host}:{server.port}'}) as response:assert response.status==200;await response.read()
        stdout,_=await asyncio.wait_for(task.communicate(),completion_timeout);assert task.returncode==0,stdout.decode(errors='replace')[-2000:]
        assert b'PASS native Steam OpenID handoff' in stdout
        if expected_friends is not None:
            assert f'PASS native Steam friend reply count {expected_friends}.'.encode() in stdout
            checks.append('actual native Steam browser/account clients admit the complete bounded thousand-friend response from Rust')
        assert sql(state,'SELECT screen_name FROM player_profiles WHERE steam_name=? AND screen_name=?',['Steam Fixture','Native Steam Rust'])
        checks.append('actual native SteamLogin/browser proof/code/poll and OnlineAccount/clan/sign-out use real Rust HTTP; only upstream Steam verification/profile IO is a fixture')
        return dict(passed=True,scope='Real native Steam/account clients over Rust HTTP, independent upstream fixture; no real Steam grant or played-game/delivered acceptance.')
    finally:
        if task.returncode is None:task.kill();await task.wait()

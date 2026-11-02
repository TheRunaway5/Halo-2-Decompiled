"""Rust Steam relationship IO controls; fixed upstream calls are private fixtures."""
import concurrent.futures
import asyncio
import importlib.util
import hashlib
import json
import os
from pathlib import Path
import sqlite3
import subprocess
import sys
import time
import uuid


def qualify(binary, scratch, catalog, Server, native_probe=None):
    scratch.mkdir()
    if not sys.platform.startswith('linux'):
        return dict(qualified=False, real_steam=False, scope='Private curl interposer requires Linux; no relationship IO qualification here.')
    state = scratch / 'state'
    shim = scratch / 'provider.so'
    source = Path(__file__).with_name('rust_service_steam_provider_fixture.cpp')
    command = ['g++','-std=c++20','-O1','-shared','-fPIC',str(source),'-o',str(shim),'-ldl','-lcrypto']
    compiled = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    assert compiled.returncode == 0, compiled.stdout.decode(errors='replace')[-2000:]
    control = scratch / 'provider.json'
    calls = scratch / 'calls.jsonl'
    environment = dict(LD_PRELOAD=str(shim), H2_STEAM_KEY='1'*32,
                       H2_TEST_STEAM_PROVIDER_DATA=str(control), H2_TEST_STEAM_PROVIDER_CALLS=str(calls))
    actor, peer = '76561197960287930', '76561197960287931'
    tokens = ['h2s_' + c*43 for c in 'HK']
    sessions = []
    checks = []
    server = None

    def configure(**values):
        p = control.with_suffix('.tmp')
        p.write_text(json.dumps(dict(relationship_fixture=True, friend_ids=[peer]) | values))
        p.replace(control)

    def sql(statement, params=()):
        with sqlite3.connect(state/'registry.sqlite3') as db:
            return db.execute(statement, params).fetchall()

    def expect(method, path, status=200, **values):
        actual, value = server.request(method, path, **values)
        assert actual == status, (method, path, actual, status, value.get('error'))
        return value

    def events():
        return [json.loads(line) for line in calls.read_text().splitlines()] if calls.exists() else []

    def view(i=0, status=200, **values):
        return expect('GET','/v1/account/steam-friends',status,token=sessions[i],timeout=90,**values)

    def start(**config):
        nonlocal server, sessions
        if server: server.close()
        configure(**config)
        server = Server(binary,state,scratch,catalog,environment=environment)
        if not sessions:
            with sqlite3.connect(state/'registry.sqlite3') as db:
                with db: db.executemany('INSERT INTO steam_login_sessions VALUES(?,?,?,?)',[
                    (hashlib.sha256(t.encode()).hexdigest(),s,'Private Steam Friend Fixture',int(time.time())+3600)
                    for t,s in zip(tokens,[actor,peer])])
        sessions = [expect('POST','/v1/account/session',201,token=t,body={'screen_name':name})['token']
                    for t,name in zip(tokens,['Steam Actor','Steam Peer'])]

    def party():
        return expect('GET','/v1/party?privacy=1',token=sessions[0])['party']

    def invitation():
        return dict(action='invite_steam', request_id=str(uuid.uuid4()),version=party()['version'],steam_id=peer)

    def invite(body,status=200):
        return expect('POST','/v1/party/commands?privacy=1',status,token=sessions[0],body=body,timeout=90)

    try:
        start()
        expect('GET','/v1/account/steam-friends',401)
        value = view()
        assert value['state']=='ready' and value['friends']==[dict(id=peer,name='Relationship Fixture',registered=True,halo_online=True)]
        before=len(events()); assert view()==value and len(events())==before
        assert all(v.get('safe_options') for v in events())
        checks.append('actual Rust Steam lookup, canonical registration/observed presence, private cache hit and fixed curl security options')
        expect('POST','/v1/party/commands',token=sessions[0],body=dict(action='create',request_id=str(uuid.uuid4())))
        body=invitation()
        failed=dict(body,claim=str(uuid.uuid4()))
        invite(failed,400)
        sql("CREATE TABLE steam_invite_commit_failure(player TEXT REFERENCES players(id) DEFERRABLE INITIALLY DEFERRED)")
        sql("CREATE TRIGGER steam_invite_fail AFTER INSERT ON party_invites BEGIN INSERT INTO steam_invite_commit_failure VALUES('private-missing'); END")
        invite(body,500)
        assert sql('SELECT count(*) FROM party_invites')[0][0]==0
        assert sql('SELECT count(*) FROM party_requests WHERE request=?',(body['request_id'],))[0][0]==0
        sql('DROP TRIGGER steam_invite_fail')
        reply=invite(body)
        assert reply['party']['privacy']['mode']==party()['privacy']['mode']
        assert len(reply['party']['outgoing'])==1
        before=len(events());invite(body);assert len(events())==before
        assert sql('SELECT count(*) FROM party_requests WHERE request=?',(body['request_id'],))[0][0]==1
        assert sql('SELECT count(*) FROM party_invites')[0][0]==1
        checks.append('actual sealed Steam invitation proof, forged-claim rejection, deferred COMMIT rollback/retry, command replay and native privacy projection')

        start(friends_status=403)
        assert view()==dict(state='private',friends=[])
        invite(invitation(),403)
        checks.append('private upstream relationships cannot be replaced by public identities or cached invitation proof')
        for config in [dict(friends_raw='{"friendslist":{"friends":[],"fri\\u0065nds":[]}}'),
                       dict(mode='encoded'),dict(mode='redirect'),dict(mode='unavailable'),
                       dict(friends_raw='x'*1048577),dict(summary_raw='{"response":{"players":null}}')]:
            start(**config);view(status=502)
        checks.append('actual curl/JSON boundary rejects duplicate keys, compressed/redirected/oversized/unavailable responses and malformed profile rows')

        start(friends_delay_ms=1000)
        before=len(events())
        with concurrent.futures.ThreadPoolExecutor(max_workers=2) as pool:
            pending=pool.submit(view)
            deadline=time.monotonic()+3
            while len(events())==before:
                assert time.monotonic()<deadline;time.sleep(.01)
            began=time.monotonic();expect('GET','/health');sql('UPDATE clock_floor SET value=value WHERE id=1')
            assert time.monotonic()-began<1
            expect('DELETE','/v1/account/session',token=sessions[0])
            try:pending.result(timeout=6)
            except AssertionError as error:assert error.args[0][2]==401
            else:raise AssertionError('Revoked account accepted a provider reply')
        checks.append('actual provider IO holds neither runtime mutex nor SQL; account revocation during IO rejects the late reply')

        start(friends_delay_ms=500)
        who=sql("SELECT player FROM player_identities WHERE provider='steam' AND subject=?",(actor,))[0][0]
        before=len(events())
        with concurrent.futures.ThreadPoolExecutor(max_workers=1) as pool:
            pending=pool.submit(view)
            deadline=time.monotonic()+3
            while len(events())==before:
                assert time.monotonic()<deadline;time.sleep(.01)
            sql("DELETE FROM player_identities WHERE player=? AND provider='steam'",(who,))
            try:pending.result(timeout=5)
            except AssertionError as error:assert error.args[0][2]==403
            else:raise AssertionError('Removed Steam alias accepted a provider reply')
        checks.append('current canonical Steam link is rechecked after actual provider IO before any projection or invitation')
        # Login restores only the verified private fixture alias through Rust.
        start()
        large=[peer]+[str(76561197960300000+i) for i in range(1002)]
        escaped='\\"'*64
        start(friend_ids=list(reversed(large)),friends_name=escaped)
        before=len(events());value=view();rows=value['friends'];large_bytes=len(json.dumps(value,separators=(',',':')).encode())
        assert len(rows)==1000 and value['truncated'] is True and large_bytes>256*1024
        batches=[e['subject'].split(',') for e in events()[before:] if e['phase']=='profile']
        assert [len(b) for b in batches]==[100]*10 and sum(batches,[])==sorted(large)[:1000]
        assert rows[0]['id']==peer and rows[0]['halo_online']
        checks.append('actual thousand-row response exceeds the former256KiB native limit, preserves deterministic first1000 selection and ten exact100-ID provider batches')
        start(friend_ids=list(reversed(large)),friends_name=escaped,friends_delay_ms=3000,
              friend_names={'76561197960287937':'Steam Fixture'})
        began=time.monotonic();slow=view();slow_seconds=time.monotonic()-began
        assert slow_seconds>30 and len(slow['friends'])==1000
        checks.append('real33-second eleven-call lookup survives the former30-second HTTP connection cutoff; native75-second owner deadline remains unchanged')
        native=None
        if native_probe:
            configure(friend_ids=list(reversed(large)),friends_name=escaped,
                      friend_names={'76561197960287937':'Steam Fixture'})
            spec=importlib.util.spec_from_file_location('steam_native_relationships',Path(__file__).with_name('rust_service_steam_controls.py'))
            module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
            native=asyncio.run(module.native_controls(server,state,scratch,native_probe,checks,expected_friends=1000))
        start()
        began=time.monotonic();assert view()['state']=='ready'
        configure(friends_status=403)
        assert view()['state']=='ready'
        while time.monotonic()-began<60.2:time.sleep(1)
        assert view()==dict(state='private',friends=[])
        cache_seconds=time.monotonic()-began
        checks.append('actual unshortened60-second cache expiry observes a new private upstream rather than retaining stale invitation authority')
        assert expect('GET','/health')['ready'] is False
        return dict(qualified=True, checks=checks, rows=1000, public_reply_bytes=large_bytes,
                    real_long_lookup_seconds=slow_seconds,real_cache_expiry_seconds=cache_seconds,
                    fixture=dict(compiler=command,sha256=hashlib.sha256(source.read_bytes()).hexdigest(),default_binary=True,upstream_interposed=True),
                    real_steam=False, native_played_world=False, native=native,
                    native_slow_lookup_qualification=False,native_existing_http_timeout_seconds=3)
    finally:
        if server:server.close()

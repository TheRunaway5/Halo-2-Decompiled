"""Actual native default HTTP timeout controls against the Rust server.

Only fixed upstream provider IO is interposed. No real Steam authorization,
relationship mutation, rendered world or installed-client acceptance.
"""
import asyncio
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import sqlite3
import subprocess
import sys
import time


def qualify(binary,scratch,catalog,Server,login_probe,invite_probe=None,cases='all'):
    scratch.mkdir()
    if not sys.platform.startswith('linux'):
        return dict(qualified=False,real_provider=False,scope='Private upstream interposer requires Linux.')
    source=Path(__file__).with_name('rust_service_steam_provider_fixture.cpp');shim=scratch/'provider.so'
    compile=['g++','-std=c++20','-O1','-shared','-fPIC',str(source),'-o',str(shim),'-ldl','-lcrypto']
    run=subprocess.run(compile,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
    assert run.returncode==0,run.stdout.decode(errors='replace')[-2000:]
    control=scratch/'provider.json';calls=scratch/'calls.jsonl';state=scratch/'state';server=None
    env=dict(LD_PRELOAD=str(shim),H2_STEAM_KEY='1'*32,H2_TEST_STEAM_PROVIDER_DATA=str(control),H2_TEST_STEAM_PROVIDER_CALLS=str(calls))
    spec=importlib.util.spec_from_file_location('native_steam_budget_auth',Path(__file__).with_name('rust_service_steam_controls.py'))
    auth=importlib.util.module_from_spec(spec);spec.loader.exec_module(auth)
    checks=[];timings={};large=[str(76561197960300000+i) for i in range(1000)]
    def configure(**values):
        temp=control.with_suffix('.tmp');temp.write_text(json.dumps(dict(relationship_fixture=True,friend_ids=large,
            friends_name='\\"'*64,friend_names={'76561197960287937':'Steam Fixture'})|values));temp.replace(control)
    def start(**values):
        nonlocal server
        if server:server.close()
        configure(**values);server=Server(binary,state,scratch,catalog,environment=env)
    def events():return [json.loads(s) for s in calls.read_text().splitlines()] if calls.exists() else []
    async def login():return await auth.native_controls(server,state,scratch,login_probe,checks,expected_friends=1000,completion_timeout=110)
    try:
        start(friends_delay_ms=3000)
        began=time.monotonic();native=asyncio.run(login());timings['slow_lookup_seconds']=time.monotonic()-began
        assert native['passed'] and timings['slow_lookup_seconds']>30
        checks.append('actual native default Curl accepts the full thousand-row lookup after eleven real three-second provider calls')
        if cases=='lookup':return dict(qualified=True,checks=checks,timings=timings,cases=cases,real_provider=False)
        start(friends_delay_ms=3000)
        queue_token='h2s_'+'Q'*43
        with sqlite3.connect(state/'registry.sqlite3') as db:
            with db:db.execute('INSERT INTO steam_login_sessions VALUES(?,?,?,?)',(hashlib.sha256(queue_token.encode()).hexdigest(),'76561197960287932','Private queued owner',int(time.time())+3600))
        status,value=server.request('POST','/v1/account/session',token=queue_token,body={'screen_name':'Queued Owner'});assert status==201
        grant=value['token'];before=len(events())
        async def queued():
            pending=asyncio.create_task(asyncio.to_thread(server.request,'GET','/v1/account/steam-friends',token=grant,timeout=100))
            try:
                deadline=time.monotonic()+5
                while len(events())==before:
                    assert time.monotonic()<deadline;await asyncio.sleep(.02)
                began=time.monotonic();result=await login();timings['queued_lookup_seconds']=time.monotonic()-began
                status,value=await pending;assert status==200 and len(value['friends'])==1000
                assert result['passed'] and timings['queued_lookup_seconds']>55
            finally:
                if not pending.done():await pending
        asyncio.run(queued())
        checks.append('actual native client waits behind an owned provider lookup and completes its own full thousand-row request within the native owner deadline')
        start(friend_ids=['76561197960287931'],friends_name='Native Peer',friends_delay_ms=0)
        actor_token='h2s_'+'I'*43;peer_token='h2s_'+'J'*43
        with sqlite3.connect(state/'registry.sqlite3') as db:
            with db:db.executemany('INSERT INTO steam_login_sessions VALUES(?,?,?,?)',[(hashlib.sha256(t.encode()).hexdigest(),s,'Private invite fixture',int(time.time())+3600) for t,s in [(actor_token,'76561197960287938'),(peer_token,'76561197960287931')]])
        status,value=server.request('POST','/v1/account/session',token=peer_token,body={'screen_name':'Native Peer'});assert status==201
        peer_grant=value['token'];ready=scratch/'invite-ready.txt';go=scratch/'invite-go.txt'
        native_env=dict(os.environ,H2_ACCOUNT_URL=f'http://{server.host}:{server.port}',H2_ACCOUNT_ALLOW_LOOPBACK_HTTP='1',
            H2_TEST_STEAM_TOKEN=actor_token,H2_TEST_STEAM_SUBJECT='76561197960287938',H2_TEST_STEAM_TARGET='76561197960287931',
            H2_EOS_ENABLED='0',H2_EOS_PROFILES='0',H2_ONLINE_TRANSPORT='direct')
        probe=subprocess.Popen([str(invite_probe),str(ready),str(go)],env=native_env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
        try:
            deadline=time.monotonic()+15
            while (not ready.exists() or ready.stat().st_size==0) and time.monotonic()<deadline:
                assert probe.poll() is None,'Native invitation probe stopped before cache priming';time.sleep(.02)
            assert ready.exists()
            began=time.monotonic()
            while time.monotonic()-began<60.2:
                status,_=server.request('GET','/v1/clans',token=peer_grant);assert status==200;time.sleep(1)
            timings['cold_cache_wait_seconds']=time.monotonic()-began
            configure(friend_ids=['76561197960287931'],friends_name='Native Peer',friends_delay_ms=2500)
            go.write_text('invite\n');began=time.monotonic()
            while probe.poll() is None:
                assert time.monotonic()-began<105
                status,_=server.request('GET','/v1/clans',token=peer_grant);assert status==200;time.sleep(.2)
            stdout=probe.communicate(timeout=1)[0];assert probe.returncode==0,stdout.decode(errors='replace')[-2000:]
            assert b'PASS native cold Steam invitation seconds ' in stdout
            timings['cold_invitation_seconds']=time.monotonic()-began;assert timings['cold_invitation_seconds']>3
            with sqlite3.connect(state/'registry.sqlite3') as db:
                rows=db.execute('SELECT payload FROM party_requests').fetchall()
                assert sum(json.loads(r[0]).get('action')=='invite_steam' for r in rows)==1
                assert db.execute('SELECT count(*) FROM party_invites').fetchone()[0]==1
            checks.append('actual native cold invitation survives a five-second upstream lookup after the unshortened sixty-second cache expiry and commits exactly one command/invitation')
        finally:
            if probe.poll() is None:probe.kill();probe.wait()
            probe.stdout.close()
        start(friends_delay_ms=3000)
        cancel_token='h2s_'+'C'*43
        with sqlite3.connect(state/'registry.sqlite3') as db:
            with db:db.execute('INSERT INTO steam_login_sessions VALUES(?,?,?,?)',(hashlib.sha256(cancel_token.encode()).hexdigest(),'76561197960287942','Private cancel fixture',int(time.time())+3600))
        cancel_go=scratch/'cancel-go.txt';before=len(events())
        native_env.update(H2_ACCOUNT_URL=f'http://{server.host}:{server.port}',
            H2_TEST_STEAM_TOKEN=cancel_token,H2_TEST_STEAM_SUBJECT='76561197960287942')
        probe=subprocess.Popen([str(invite_probe),str(scratch/'cancel-ready.txt'),str(cancel_go),'--cancel-lookup'],env=native_env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
        try:
            deadline=time.monotonic()+12
            while not any(e.get('phase')=='friends' for e in events()[before:]):
                assert probe.poll() is None and time.monotonic()<deadline,'Native cancellation lookup did not reach the provider'
                time.sleep(.02)
            time.sleep(.2);began=time.monotonic();cancel_go.write_text('cancel\n')
            stdout=probe.communicate(timeout=3)[0];timings['native_cancel_seconds']=time.monotonic()-began
            assert probe.returncode==0 and b'PASS native cancelled Steam lookup seconds ' in stdout,stdout.decode(errors='replace')[-2000:]
            assert timings['native_cancel_seconds']<2.5
            checks.append('native sign-out and destruction cancel an active extended-budget lookup within2.5 seconds through the existing Curl stop-token progress callback')
        finally:
            if probe.poll() is None:probe.kill();probe.wait()
            probe.stdout.close()
        return dict(qualified=True,checks=checks,timings=timings,cases=cases,real_provider=False,
                    native_world=False,fixture=dict(source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),upstream_only=True))
    finally:
        if server:server.close()

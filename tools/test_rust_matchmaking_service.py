#!/usr/bin/env python3
"""Build and qualify real Rust HTTP/SQLite inside run_test.py-owned storage."""
import argparse
import hashlib
import http.client
import importlib.util
import json
import os
from pathlib import Path
import selectors
import shutil
import socket
import sqlite3
import subprocess
import sys
import time
import uuid

ROOT=Path(__file__).resolve().parents[1]


def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()


class Server:
    def __init__(self,binary,state,scratch,catalog=None,*,environment=None,tls=False,proxy=False):
        self.log=scratch/'server.log'
        self.stream=self.log.open('ab')
        env=dict(os.environ,H2_EOS_FRIEND_STORAGE='0')
        for name in ('H2_EOS_OPENID_AUDIENCE','H2_EOS_WORKER','H2_PUBLIC_URL','H2_MASTER_PUBLIC_URL',
                     'H2_STEAM_KEY_FILE','H2_STEAM_KEY','STEAM_KEY'):
            env.pop(name,None)
        env.update(environment or {})
        command=[str(binary),'--state',str(state),'--listen','127.0.0.1:0','--steam-enabled',
                 '--behind-tls-proxy' if tls else '--allow-http']
        if proxy and not tls:command.append('--behind-tls-proxy')
        if catalog:command.extend(['--matchmaking-catalog',str(catalog)])
        self.process=subprocess.Popen(command,stdout=subprocess.PIPE,stderr=self.stream,env=env)
        selector=selectors.DefaultSelector();selector.register(self.process.stdout,selectors.EVENT_READ)
        try:
            if not selector.select(15):raise RuntimeError('Rust service did not publish its listener')
            line=self.process.stdout.readline().decode().strip()
            if not line.startswith('LISTENING '):raise RuntimeError('Rust service failed before listener: '+self.log.read_text()[-3000:])
            self.host,self.port=line.split()[1].rsplit(':',1);self.port=int(self.port)
        except BaseException:self.close();raise
        finally:selector.close()
    def close(self):
        if self.process.poll() is None:
            self.process.terminate()
            try:self.process.wait(timeout=5)
            except subprocess.TimeoutExpired:self.process.kill();self.process.wait()
        self.process.stdout.close();self.stream.close()
    def request(self,method,path,*,body=None,token=None,raw=None,headers=None,timeout=5):
        connection=http.client.HTTPConnection(self.host,self.port,timeout=timeout)
        fields={'Content-Type':'application/json',**(headers or {})}
        if token:fields['Authorization']='Bearer '+token
        data=raw if raw is not None else json.dumps(body).encode() if body is not None else None
        try:
            connection.request(method,path,body=data,headers=fields)
            reply=connection.getresponse();data=reply.read(2*1024*1024+1)
            assert len(data)<=2*1024*1024
            value=json.loads(data)
            assert reply.getheader('Cache-Control')=='no-store'
            return reply.status,value
        finally:connection.close()


def qualify(binary,scratch,native_client=None):
    state=scratch/'state';checks=[];server=Server(binary,state,scratch)
    def expect(method,path,status,**kwargs):
        actual,value=server.request(method,path,**kwargs)
        assert actual==status,(method,path,actual,{'error':value.get('error'),'fields':sorted(value)})
        return value
    try:
        assert binary.read_bytes()[:4]==b'\x7fELF' if sys.platform.startswith('linux') else binary.is_file()
        health=expect('GET','/health',200)
        assert health['component_ready'] and health['ready'] is False
        versioned=expect('GET','/v1/health',200)
        for key in ('status','component_ready','ready','capabilities','catalog_sha256'):
            assert versioned[key]==health[key],key
        checks.append('real Rust executable and HTTP listener; full matchmaking readiness remains false')
        db=sqlite3.connect(state/'registry.sqlite3')
        try:
            tokens=['h2s_'+'A'*43,'h2s_'+'B'*43]
            subjects=['76561197960287930','76561197960287931']
            with db:
                db.executemany('INSERT INTO steam_login_sessions VALUES(?,?,?,?)',[
                    (hashlib.sha256(token.encode()).hexdigest(),subject,'Steam Fixture',int(time.time())+3600)
                    for token,subject in zip(tokens,subjects)])
            expect('POST','/v1/account/session',400,token=tokens[0],raw=b'{"screen_name":"a","screen_name":"b"}')
            expect('POST','/v1/account/session',400,token=tokens[0],body={'screen_name':'a','player':'forged'})
            expect('POST','/v1/account/session',413,token=tokens[0],raw=b' '*16385)
            expect('POST','/v1/account/session',415,token=tokens[0],body={'screen_name':'a'},headers={'Content-Encoding':'gzip'})
            expect('POST','/v1/account/session',401,token='h2r_'+'C'*43,body={'screen_name':'a'})
            expect('POST','/v1/account/session',401,token='h2s_'+'C'*43,body={'screen_name':'a'})
            assert db.execute('SELECT count(*) FROM players').fetchone()[0]==0
            with socket.create_connection((server.host,server.port),timeout=3) as wire:
                wire.sendall(b'GET /v1/clans HTTP/1.1\r\nHost: localhost\r\nAuthorization: Bearer one\r\nAuthorization: Bearer two\r\n\r\n')
                assert b' 400 ' in wire.recv(4096).split(b'\r\n')[0]
            checks.append('strict duplicate/field/body/encoding checks and expired/wrong-kind grants leave no players')
            with socket.create_connection((server.host,server.port),timeout=7) as wire:
                wire.sendall(b'GET /health HTTP/1.1\r\n')
                reply=wire.recv(4096)
                assert reply==b'' or b' 408 ' in reply.split(b'\r\n')[0]
            with socket.create_connection((server.host,server.port),timeout=7) as wire:
                wire.sendall(b'POST /v1/account/session HTTP/1.1\r\nHost: localhost\r\nContent-Type: application/json\r\nContent-Length: 100\r\n\r\n{')
                assert b' 408 ' in wire.recv(4096).split(b'\r\n')[0]
            expect('GET','/health',200)
            checks.append('incomplete headers/body expire at the real HTTP parser/reader; request capacity is released')
            sessions=[expect('POST','/v1/account/session',201,token=token,body={'screen_name':name})['token'] for token,name in zip(tokens,['Rust Pilot','Rust Peer'])]
            first_profile=expect('GET','/v1/account/profile',200,token=sessions[0])
            assert first_profile['joined_at'] is not None and first_profile['appearance'] is None
            who=db.execute('SELECT player FROM player_identities WHERE provider=? AND subject=?',('steam',subjects[0])).fetchone()[0]
            initial_tag=first_profile['friend_tag'] if 'friend_tag' in first_profile else db.execute('SELECT discriminator FROM player_friend_tags WHERE player=?',(who,)).fetchone()[0]
            checks.append('private Steam grant SQL, native account reservation/publication, canonical profile and public history identity')
            body={'request_id':str(uuid.uuid4()),'action':'create','name':'Rust Clan'}
            created=expect('POST','/v1/clans/commands',200,token=sessions[0],body=body)
            assert created['clan']['role']==3 and len(created['clan']['members'])==1
            expect('POST','/v1/clans/commands',200,token=sessions[0],body=body)
            expect('POST','/v1/clans/commands',409,token=sessions[0],body=body|{'name':'Other Clan'})
            with db:
                db.execute("UPDATE player_profiles SET discord_username='fixture_peer' WHERE player=(SELECT player FROM player_identities WHERE provider='steam' AND subject=?)",(subjects[1],))
            expect('POST','/v1/clans/commands',200,token=sessions[0],body={'request_id':str(uuid.uuid4()),'action':'invite','username':'fixture_peer'})
            invited=expect('GET','/v1/clans',200,token=sessions[1])['invitations'][0]['id']
            expect('POST','/v1/clans/commands',200,token=sessions[1],body={'request_id':str(uuid.uuid4()),'action':'accept','target':invited})
            roster=expect('GET','/v1/clans?presence=1',200,token=sessions[0])
            assert len(roster['clan']['members'])==2
            assert all(row['presence']=='online' for row in roster['clan']['members'])
            assert subjects[0] not in json.dumps(roster) and tokens[0] not in json.dumps(roster)
            checks.append('real clan create/invite/accept, command replay/conflict, native presence and private identifiers')
            customization={'appearance':'ff020304013f1f81a5','service_tag':'A117'}
            with db:
                db.executescript("CREATE TABLE rust_commit_failure(player TEXT REFERENCES players(id) DEFERRABLE INITIALLY DEFERRED);"
                    "CREATE TRIGGER rust_fail_profile AFTER INSERT ON player_customization BEGIN INSERT INTO rust_commit_failure VALUES('declared-missing'); END;")
            expect('PUT','/v1/account/profile',500,token=sessions[0],body=customization)
            assert db.execute('SELECT count(*) FROM player_customization').fetchone()[0]==0
            assert expect('GET','/v1/account/profile',200,token=sessions[0])['appearance'] is None
            with db:db.execute('DROP TRIGGER rust_fail_profile')
            profile=expect('PUT','/v1/account/profile',200,token=sessions[0],body=customization)
            assert profile['service_tag']=='A117' and profile['appearance']['species']=='elite'
            assert profile['joined_at']==first_profile['joined_at']
            checks.append('actual deferred SQL COMMIT failure rolls back profile and native cache plan; retry succeeds')
            public=expect('GET','/v1/history/players?discord_username=fixture_peer',200)['players'][0]['id']
            assert expect('GET','/v1/history/players/'+public+'/profile',200)['joined_at'] is not None
            expect('GET','/v1/history/players?limit=1&limit=2',400)
            checks.append('Rust public history/profile SQL and duplicate-query rejection')
            expect('DELETE','/v1/account/session',200,token=sessions[1])
            expect('GET','/v1/clans',401,token=sessions[1])
            checks.append('native account logout retires its grant')
            if native_client:
                env=dict(os.environ,H2_ACCOUNT_URL=f'http://{server.host}:{server.port}',H2_ACCOUNT_ALLOW_LOOPBACK_HTTP='1',
                    H2_TEST_STEAM_TOKEN=tokens[0],H2_ONLINE_TRANSPORT='direct',H2_EOS_ENABLED='0',H2_EOS_PROFILES='0')
                run=subprocess.run([str(native_client)],env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=15)
                assert run.returncode==0,run.stdout.decode(errors='replace')
                assert 'PASS native account/profile/clan client over Rust HTTP' in run.stdout.decode()
                stored=db.execute('SELECT appearance FROM player_customization WHERE player=?',(who,)).fetchone()[0]
                assert bytes(stored).hex()=='0a00000a0000000000','Native profile write did not reach Rust-owned SQLite'
                checks.append('real native account/profile/clan client over Rust HTTP; only provider handoff is a fixture')
            old=sessions[0];server.close();server=Server(binary,state,scratch)
            expect('GET','/v1/clans',401,token=old)
            fresh=expect('POST','/v1/account/session',201,token=tokens[0],body={'screen_name':'After Restart'})['token']
            assert expect('GET','/v1/account/profile',200,token=fresh)['service_tag']=='A117'
            assert len(expect('GET','/v1/clans',200,token=fresh)['clan']['members'])==2
            assert db.execute('SELECT player FROM player_identities WHERE provider=? AND subject=?',('steam',subjects[0])).fetchone()[0]==who
            assert db.execute('SELECT discriminator FROM player_friend_tags WHERE player=?',(who,)).fetchone()[0]==initial_tag.rsplit('#',1)[-1]
            assert db.execute('SELECT count(*) FROM players').fetchone()[0]==2
            checks.append('Rust process restart retains canonical/profile/clan/tag data and does not revive old native grants')
            with db:db.execute('DELETE FROM steam_login_sessions WHERE token_hash=?',(hashlib.sha256(tokens[0].encode()).hexdigest(),))
            expect('POST','/v1/account/session',401,token=tokens[0],body={'screen_name':'Rejected'})
            checks.append('revoked persisted Steam credential cannot create a new account grant')
            expect('POST','/v1/matchmaking/queue',503,token=fresh,body={})
            assert expect('GET','/health',200)['ready'] is False
            checks.append('unported matchmaking capabilities fail explicitly and never advertise full readiness')
        finally:db.close()
    finally:server.close()
    return checks


def qualify_matchmaking(binary,scratch,catalog,native_client=None):
    """Real Rust HTTP and storage, with private fixture provider grants only."""
    scratch.mkdir();state=scratch/'state';checks=[];server=Server(binary,state,scratch,catalog)
    def expect(method,path,status,**kwargs):
        actual,value=server.request(method,path,**kwargs)
        assert actual==status,(method,path,actual,{'error':value.get('error'),'fields':sorted(value)})
        return value
    def playlists(token):return expect('GET','/v1/matchmaking/playlists',200,token=token)['playlists']
    def party(token):return expect('GET','/v1/party?local_players=1&appearance=1&privacy=1',200,token=token)
    def command(token,action,status=200,**extra):
        if action not in ('create','accept') and 'version' not in extra:
            extra['version']=party(token)['party']['version']
        return expect('POST','/v1/party/commands',status,token=token,body=dict(action=action,request_id=str(uuid.uuid4()))|extra)
    def countdown(token):return expect('GET','/v1/party/countdown',200,token=token)['countdown']
    try:
        health=expect('GET','/health',200)
        assert health['catalog_sha256']==digest(catalog) and health['ready'] is False
        assert {'matchmaking_sessions','playlist_eligibility','party_countdown'}<=set(health['capabilities'])
        entries=json.loads(catalog.read_text())['playlists'];maps=sorted({game['map'] for entry in entries for game in entry['games']})
        assert len(entries)==14
        db=sqlite3.connect(state/'registry.sqlite3')
        try:
            tokens=['h2s_'+letter*43 for letter in 'DEF']
            subjects=['76561197960287930','76561197960287931','76561197960287932']
            with db:db.executemany('INSERT INTO steam_login_sessions VALUES(?,?,?,?)',[
                (hashlib.sha256(token.encode()).hexdigest(),subject,'Steam Fixture',int(time.time())+3600)
                for token,subject in zip(tokens,subjects)])
            body=dict(screen_name='Rust Match Pilot',maps=maps,compatibility='halo2-native-queue-v16',language=0)
            expect('POST','/v1/matchmaking/session',400,token=tokens[0],body=body|{'player_id':'forged'})
            expect('POST','/v1/matchmaking/session',400,token=tokens[0],body=body|{'maps':[maps[0],maps[0]]})
            expect('POST','/v1/matchmaking/session',400,token=tokens[0],body=body|{'local_players':[{'screen_name':'Duplicate','identity_token':tokens[0]}]})
            assert db.execute('SELECT count(*) FROM players').fetchone()[0]==0
            checks.append('session parser and verified couch preflight reject forged/duplicate inputs before canonical writes')
            old=expect('POST','/v1/matchmaking/session',201,token=tokens[0],body=body|{'compatibility':'halo2-native-queue-v12'})['token']
            legacy=playlists(old)
            assert len(legacy)==14 and not any(row['eligible'] for row in legacy)
            a=expect('POST','/v1/matchmaking/session',201,token=tokens[0],body=body)['token']
            expect('GET','/v1/matchmaking/playlists',401,token=old)
            rows=playlists(a)
            assert sum(row['eligible'] for row in rows)==12
            assert all(row['eligible'] is not row.get('clan_match',False) for row in rows)
            assert all(set(row)<=set(('id','revision','name','description','ranked','quickmatch','minimum_players','maximum_players','eligible','level','clan_match')) for row in rows)
            checks.append('actual 14-row catalog over Rust HTTP: v12 admits zero; v16 admits all twelve non-clan playlists')
            b=expect('POST','/v1/matchmaking/session',201,token=tokens[1],body=body|{'screen_name':'Rust Match Peer'})['token']
            idle=expect('GET','/v1/matchmaking/ticket?progress=1&ranks=1&start=1&failure=1&appearance=1',200,token=a)
            assert idle['state']=='idle' and idle['match'] is None and idle['start_countdown_ms'] is None
            lab=next(row for row in rows if row['id']=='lab-test')
            ticket=dict(playlist_id=lab['id'],revision=lab['revision'],request_id=str(uuid.uuid4()))
            expect('POST','/v1/matchmaking/ticket',400,token=a,body=ticket|{'rank':50})
            with db:
                db.executescript("CREATE TABLE rust_queue_commit_failure(player TEXT REFERENCES players(id) DEFERRABLE INITIALLY DEFERRED);"
                    "CREATE TRIGGER rust_fail_ticket AFTER INSERT ON playlist_ranks WHEN NEW.playlist='lab-test' BEGIN INSERT INTO rust_queue_commit_failure VALUES('declared-missing'); END;")
            expect('POST','/v1/matchmaking/ticket',500,token=a,body=ticket)
            assert expect('GET','/v1/matchmaking/ticket',200,token=a)['state']=='idle'
            assert db.execute("SELECT count(*) FROM playlist_ranks WHERE playlist='lab-test'").fetchone()[0]==0
            with db:db.execute('DROP TRIGGER rust_fail_ticket')
            searching=expect('POST','/v1/matchmaking/ticket',201,token=a,body=ticket)
            assert searching['state']=='searching' and searching['queued_players']==1 and searching['match'] is None
            assert expect('POST','/v1/matchmaking/ticket',201,token=a,body=ticket)==searching
            assert expect('GET','/v1/matchmaking/ticket?progress=1&ranks=1&start=1&failure=1&appearance=1',200,token=a)['state']=='searching'
            assert expect('DELETE','/v1/matchmaking/ticket?cancel=1',200,token=a)['state']=='idle'
            expect('POST','/v1/matchmaking/ticket',409,token=a,body=ticket)
            checks.append('real queue API/SQL/gathering continuation rolls back ticket and rank writes on failed COMMIT, retries once, waits for actual QoS and cancels without fabricated matches')
            accounts=[expect('POST','/v1/account/session',201,token=token,body={'screen_name':name})['token']
                for token,name in zip(tokens,['Rust Match Pilot','Rust Match Peer'])]
            with db:db.execute("UPDATE player_profiles SET discord_username='fixture_peer' WHERE player=(SELECT player FROM player_identities WHERE provider='steam' AND subject=?)",(subjects[1],))
            created=command(accounts[0],'create')
            assert created['party']['leader'] and len(created['party']['members'])==1
            command(accounts[0],'invite',username='fixture_peer')
            invitation=party(accounts[1])['invitations'][0]
            accepted=command(accounts[1],'accept',target=invitation['id'],version=invitation['version'])
            assert len(accepted['party']['members'])==2 and all(row['online'] for row in accepted['party']['members'])
            raw=json.dumps(accepted)
            for private in (*tokens,*subjects,*[r[0] for r in db.execute('SELECT id FROM players')]):assert private not in raw
            assert all('local_players' not in row for row in accepted['party']['members'])
            checks.append('accepted-party create/invite/accept follows the same Rust queue source and hides private identities')
            lab=next(row for row in rows if row['id']=='lab-test');version=party(accounts[0])['party']['version']
            command(accounts[0],'countdown_start',409,playlist_id=lab['id'],revision=lab['revision'])
            command(accounts[1],'ready',playlist_id=lab['id'],revision=lab['revision'])
            command(accounts[1],'countdown_start',403,playlist_id=lab['id'],revision=lab['revision'])
            start=dict(action='countdown_start',request_id=str(uuid.uuid4()),version=version,playlist_id=lab['id'],revision=lab['revision'])
            expect('POST','/v1/party/commands',200,token=accounts[0],body=start)
            initial=countdown(accounts[0]);assert initial['seconds']==10 and initial==countdown(accounts[1])
            expect('POST','/v1/party/commands',200,token=accounts[0],body=start)
            assert countdown(accounts[0])['serial']==initial['serial']
            expect('POST','/v1/party/commands',409,token=accounts[0],body=start|{'revision':'changed'})
            deadline=time.monotonic()+3
            while countdown(accounts[1])['seconds']==10 and time.monotonic()<deadline:time.sleep(.05)
            assert countdown(accounts[1])['seconds']==9
            command(accounts[0],'countdown_start',playlist_id=lab['id'],revision=lab['revision'])
            assert countdown(accounts[1])['seconds']==8
            checks.append('real shared pregame countdown requires remote consent, enforces leader authority, replays once and ticks without request clocks')
            with db:
                db.execute("CREATE TRIGGER rust_fail_session AFTER UPDATE ON player_profiles WHEN NEW.screen_name='Failed Replacement' BEGIN INSERT INTO rust_queue_commit_failure VALUES('declared-missing'); END;")
            before=countdown(accounts[0]);members=db.execute('SELECT player,party,handle FROM party_members ORDER BY rowid').fetchall()
            expect('POST','/v1/matchmaking/session',500,token=tokens[0],body=body|{'screen_name':'Failed Replacement'})
            assert len(playlists(a))==14
            assert db.execute('SELECT player,party,handle FROM party_members ORDER BY rowid').fetchall()==members
            assert countdown(accounts[1])['serial']==before['serial']
            assert party(accounts[1])['party']['version']==version
            with db:db.execute('DROP TRIGGER rust_fail_session')
            checks.append('actual failed session COMMIT restores the prior native grant, accepted-party version, consent and active countdown')
            cancel=dict(action='countdown_cancel',request_id=str(uuid.uuid4()),version=version)
            with db:db.execute("CREATE TRIGGER rust_fail_countdown AFTER INSERT ON party_requests WHEN NEW.request='"+cancel['request_id']+"' BEGIN INSERT INTO rust_queue_commit_failure VALUES('declared-missing'); END")
            expect('POST','/v1/party/commands',500,token=accounts[0],body=cancel)
            assert countdown(accounts[0]) is not None
            assert db.execute('SELECT 1 FROM party_requests WHERE request=?',(cancel['request_id'],)).fetchone() is None
            with db:db.execute('DROP TRIGGER rust_fail_countdown')
            expect('POST','/v1/party/commands',200,token=accounts[0],body=cancel)
            assert countdown(accounts[1]) is None
            checks.append('failed countdown COMMIT preserves publication and request retry cancels exactly once')
            expect('POST','/v1/matchmaking/session',409,token=tokens[0],body=body|{'local_players':[{'screen_name':'Separate Party','identity_token':tokens[1]}]})
            locals_body=body|{'local_players':[{'screen_name':'New Signed Local','identity_token':tokens[2]},
                {'screen_name':'Guest One','appearance':'0a00000a0000000000'},{'screen_name':'Guest Two'}]}
            with db:db.execute("CREATE TRIGGER rust_fail_local AFTER INSERT ON player_profiles WHEN NEW.screen_name='New Signed Local' BEGIN INSERT INTO rust_queue_commit_failure VALUES('declared-missing'); END")
            expect('POST','/v1/matchmaking/session',500,token=tokens[0],body=locals_body)
            assert db.execute('SELECT count(*) FROM players').fetchone()[0]==2
            assert db.execute('SELECT count(*) FROM player_friend_tags').fetchone()[0]==2
            assert len(playlists(a))==14
            with db:db.execute('DROP TRIGGER rust_fail_local')
            local=expect('POST','/v1/matchmaking/session',201,token=tokens[0],body=locals_body)['token']
            expect('GET','/v1/matchmaking/playlists',401,token=a)
            assert db.execute('SELECT count(*) FROM players').fetchone()[0]==3
            assert db.execute('SELECT count(*) FROM player_friend_tags').fetchone()[0]==3
            assert db.execute("SELECT count(*) FROM players WHERE provider='guest'").fetchone()[0]==0
            renewed=expect('POST','/v1/matchmaking/session',201,token=tokens[0],body=locals_body|{'renew_session':local})['token']
            assert renewed==local
            guests=playlists(local)
            assert all(not row['eligible'] for row in guests if row['ranked'] or row.get('clan_match'))
            assert next(row for row in guests if row['id']=='lab-test')['eligible']
            expect('POST','/v1/matchmaking/session',409,token=tokens[2],body=body)
            viewed=party(accounts[0])['party']['members']
            leader=next(row for row in viewed if row['leader'])
            assert [row['guest'] for row in leader['local_players']]==[False,True,True]
            assert leader['local_players'][1]['appearance']=='0a00000a0000000000'
            checks.append('four-seat canonical transaction rolls back new signed locals on failed COMMIT; guests remain transient, renewal preserves the grant and duplicate physical seating is rejected')
            command(accounts[1],'ready',playlist_id=lab['id'],revision=lab['revision'])
            command(accounts[0],'countdown_start',playlist_id=lab['id'],revision=lab['revision'])
            assert countdown(accounts[1])['seconds']==10
            expect('DELETE','/v1/matchmaking/session',200,token=b)
            expect('GET','/v1/matchmaking/playlists',401,token=b)
            assert countdown(accounts[0]) is None
            assert party(accounts[0])['party']['version']>version
            checks.append('actual session retirement clears shared countdown and stale consent through Rust directory effects')
            retained_members=db.execute('SELECT player,party,handle FROM party_members ORDER BY rowid').fetchall()
            server.close();server=Server(binary,state,scratch,catalog)
            expect('GET','/v1/matchmaking/playlists',401,token=local)
            expect('GET','/v1/party',401,token=accounts[0])
            fresh=expect('POST','/v1/account/session',201,token=tokens[0],body={'screen_name':'After Restart'})['token']
            assert countdown(fresh) is None
            assert db.execute('SELECT player,party,handle FROM party_members ORDER BY rowid').fetchall()==retained_members
            assert db.execute('SELECT count(*) FROM players').fetchone()[0]==3
            checks.append('real Rust restart keeps accepted membership and canonical identities, while retiring session grants and countdown state')
            for _ in range(130):expect('GET','/v1/party/countdown',200,token=fresh)
            expect('GET','/v1/clans',200,token=fresh)
            checks.append('high-frequency countdown reads use the separate native countdown allowance and preserve ordinary social request capacity')
            if native_client:
                native_token='h2s_'+'G'*43;native_subject='76561197960287933'
                with db:db.execute('INSERT INTO steam_login_sessions VALUES(?,?,?,?)',
                    (hashlib.sha256(native_token.encode()).hexdigest(),native_subject,'Steam Fixture',int(time.time())+3600))
                env=dict(os.environ,H2_MATCHMAKING_URL=f'http://{server.host}:{server.port}',H2_MATCHMAKING_ALLOW_LOOPBACK_HTTP='1',
                    H2_ACCOUNT_URL=f'http://{server.host}:{server.port}',H2_ACCOUNT_ALLOW_LOOPBACK_HTTP='1',
                    H2_TEST_STEAM_TOKEN=native_token,H2_TEST_STEAM_SUBJECT=native_subject,H2_TEST_MATCHMAKING_MAPS=','.join(maps),H2_ONLINE_TRANSPORT='direct',H2_EOS_ENABLED='0',H2_EOS_PROFILES='0')
                for compatibility,eligible in (('v12',0),('v16',12)):
                    run=subprocess.run([str(native_client),compatibility],env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=20)
                    assert run.returncode==0,run.stdout.decode(errors='replace')
                    assert f'PASS native matchmaking catalog 14 eligible {eligible}' in run.stdout.decode()
                    if compatibility=='v16':assert 'PASS native party Start/cancel observes the Rust shared countdown' in run.stdout.decode()
                checks.append('actual native matchmaking client reads all fourteen Rust HTTP playlists with the observed v12/v16 eligibility differential; the native party client sends Start/cancel and observes Rust countdown; only provider handoff is a fixture')
            assert expect('GET','/health',200)['ready'] is False
            expect('GET','/v1/matchmaking/transport',401,token=fresh)
            expect('POST','/v1/matches/unavailable/eos-token',503,token=fresh,body={})
            checks.append('room descriptors reject an account-only grant; unported EOS issuance stays unavailable and full readiness stays false')
        finally:db.close()
    finally:server.close()
    return checks


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source',type=Path,default=ROOT)
    parser.add_argument('--output',type=Path,default=ROOT/'build/test-results/rust-matchmaking-service')
    parser.add_argument('--native-client',type=Path)
    parser.add_argument('--native-matchmaking-client',type=Path)
    parser.add_argument('--native-qos-probe',type=Path)
    parser.add_argument('--native-room-probe',type=Path)
    parser.add_argument('--native-steam-probe',type=Path)
    parser.add_argument('--native-social-probe',type=Path)
    parser.add_argument('--native-budget-only',action='store_true')
    parser.add_argument('--native-budget-cases',choices=('lookup','all'),default='all')
    parser.add_argument('--native-budget-probe',type=Path)
    parser.add_argument('--friend-cloud-only',action='store_true')
    parser.add_argument('--friend-cloud-room-only',action='store_true')
    parser.add_argument('--voice-only',action='store_true')
    parser.add_argument('--native-voice-probe',type=Path)
    parser.add_argument('--clock-only',action='store_true')
    parser.add_argument('--native-handoff-failclosed-only',action='store_true')
    parser.add_argument('--native-handoff-probe',type=Path)
    parser.add_argument('--frontend-buttons-only',action='store_true')
    parser.add_argument('--native-frontend-probe',type=Path)
    parser.add_argument('--mcc-root',type=Path)
    parser.add_argument('--frontend-case',choices=('blocked','complete','rendered'),default='complete')
    args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();source=scratch/'source'
    source.mkdir();args.output=args.output.resolve();args.output.mkdir(parents=True,exist_ok=True)
    for name in ('online_protocol','matchmaking_service'):
        shutil.copytree(args.source/'rust'/name,source/'rust'/name,ignore=shutil.ignore_patterns('target','__pycache__'))
    catalog=Path('multiplayer/master_server/examples/matchmaking-online.json')
    (source/catalog).parent.mkdir(parents=True,exist_ok=True)
    shutil.copyfile(args.source/catalog,source/catalog)
    helper=Path('tools/test_rust_matchmaking_service.py')
    (source/helper).parent.mkdir(parents=True,exist_ok=True)
    shutil.copyfile(args.source/helper,source/helper)
    frontend_controls=Path('tools/rust_service_frontend_controls.py')
    shutil.copyfile(args.source/frontend_controls,source/frontend_controls)
    frontend_io=Path('tools/rust_service_frontend_io_delay.cpp')
    shutil.copyfile(args.source/frontend_io,source/frontend_io)
    if args.frontend_buttons_only:
        for name in ('tests/helpers/mcc_online_ui.py','tools/data/gameplay-font-reference.json','resources/frontend/mcc-native-layout.json'):
            target=source/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(args.source/name,target)
    controls=Path('tools/rust_service_qos_controls.py')
    shutil.copyfile(args.source/controls,source/controls)
    room_controls=Path('tools/rust_service_room_controls.py')
    shutil.copyfile(args.source/room_controls,source/room_controls)
    eos_controls=Path('tools/rust_service_eos_controls.py')
    shutil.copyfile(args.source/eos_controls,source/eos_controls)
    steam_controls=Path('tools/rust_service_steam_controls.py')
    shutil.copyfile(args.source/steam_controls,source/steam_controls)
    steam_fixture=Path('tools/rust_service_steam_provider_fixture.cpp')
    shutil.copyfile(args.source/steam_fixture,source/steam_fixture)
    social_controls=Path('tools/rust_service_social_controls.py')
    shutil.copyfile(args.source/social_controls,source/social_controls)
    steam_friends_controls=Path('tools/rust_service_steam_friends_controls.py')
    shutil.copyfile(args.source/steam_friends_controls,source/steam_friends_controls)
    native_steam_controls=Path('tools/rust_service_native_steam_controls.py')
    shutil.copyfile(args.source/native_steam_controls,source/native_steam_controls)
    friend_cloud_controls=Path('tools/rust_service_friend_cloud_controls.py')
    shutil.copyfile(args.source/friend_cloud_controls,source/friend_cloud_controls)
    voice_controls=Path('tools/rust_service_voice_controls.py')
    shutil.copyfile(args.source/voice_controls,source/voice_controls)
    clock_controls=Path('tools/rust_service_clock_controls.py')
    shutil.copyfile(args.source/clock_controls,source/clock_controls)
    handoff_controls=Path('tools/rust_service_native_handoff_controls.py')
    shutil.copyfile(args.source/handoff_controls,source/handoff_controls)
    files={str(p.relative_to(source)):digest(p) for p in source.rglob('*') if p.is_file()}
    assert all(digest(args.source/p)==h for p,h in files.items())
    env=dict(os.environ,CARGO_TARGET_DIR=str(scratch/'cargo'),RUSTUP_TOOLCHAIN='1.96.0',RUSTUP_AUTO_INSTALL='0',CARGO_INCREMENTAL='0')
    report=dict(passed=False,source=str(source),scratch=str(scratch),commands=[],retained_large_artifacts=[])
    (args.output/'source-manifest.json').write_text(json.dumps(dict(files=files,source=str(source),drift_during_freeze=[]),indent=2)+'\n')
    try:
        for index,kind in enumerate(('test','build')):
            command=['cargo',kind,'--release','--locked','--offline','--jobs','2','--manifest-path',str(source/'rust/matchmaking_service/Cargo.toml')]
            run=subprocess.run(command,env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
            log=run.stdout[-1024*1024:];(args.output/f'step-{index}.log').write_bytes(log)
            report['commands'].append(dict(argv=command,returncode=run.returncode))
            if run.returncode:
                print(log.decode(errors='replace'));return run.returncode
        binary=scratch/'cargo/release/halo2-matchmaking-service'
        report['binary']=dict(path=str(binary),bytes=binary.stat().st_size,sha256=digest(binary),retained=False)
        if args.frontend_buttons_only:
            spec=importlib.util.spec_from_file_location('rust_frontend_controls',source/frontend_controls)
            module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
            report['mode']='frontend-buttons-only'
            assert args.native_frontend_probe and args.mcc_root,'Native frontend and admitted MCC inputs are required'
            report['frontend']=module.qualify(binary,scratch/'frontend',source/catalog,Server,args.native_frontend_probe,args.mcc_root,args.frontend_case)
            report['passed']=report['frontend']['passed']
            assert all(digest(source/p)==h for p,h in files.items())
            return 0 if report['passed'] else 1
        if args.native_handoff_failclosed_only:
            spec=importlib.util.spec_from_file_location('rust_native_handoff',source/handoff_controls)
            module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
            report['mode']='native-handoff-failclosed-only'
            assert args.native_handoff_probe, '--native-handoff-probe is required'
            report['native_handoff_probe']=dict(path=str(args.native_handoff_probe),sha256=digest(args.native_handoff_probe),borrowed_read_only=True)
            report['native_handoff']=module.qualify(binary,scratch/'native-handoff',source/catalog,Server,args.native_handoff_probe)
            report['passed']=report['native_handoff']['negative_control_passed']
            assert all(digest(source/p)==h for p,h in files.items())
            return 0 if report['passed'] else 1
        if args.clock_only:
            spec=importlib.util.spec_from_file_location('rust_registration_clock',source/clock_controls)
            module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
            report['mode']='clock-only'
            report['clock']=module.qualify(binary,scratch/'clock',source/catalog,Server)
            report['passed']=report['clock']['qualified']
            assert all(digest(source/p)==h for p,h in files.items())
            return 0 if report['passed'] else 1
        if args.voice_only:
            spec=importlib.util.spec_from_file_location('rust_social_voice',source/voice_controls)
            module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
            report['mode']='voice-only'
            if args.native_voice_probe:report['native_voice_probe']=dict(path=str(args.native_voice_probe),sha256=digest(args.native_voice_probe),borrowed_read_only=True)
            report['voice']=module.qualify(binary,scratch/'voice',source/catalog,Server,args.native_voice_probe)
            report['passed']=report['voice']['qualified']
            assert all(digest(source/p)==h for p,h in files.items())
            return 0 if report['passed'] else 1
        if args.friend_cloud_only or args.friend_cloud_room_only:
            spec=importlib.util.spec_from_file_location('rust_friend_cloud',source/friend_cloud_controls)
            module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
            report['mode']='friend-cloud-room-only' if args.friend_cloud_room_only else 'friend-cloud-only'
            report['friend_cloud']=module.qualify(binary,scratch/'friend-cloud',source/catalog,Server,room_only=args.friend_cloud_room_only)
            report['passed']=report['friend_cloud']['qualified']
            assert all(digest(source/p)==h for p,h in files.items())
            return 0 if report['passed'] else 1
        if args.native_budget_only:
            spec=importlib.util.spec_from_file_location('native_steam_budget',source/native_steam_controls)
            module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
            report['mode']='native-steam-budget-only'
            report['native_budget']=module.qualify(binary,scratch/'native-budget',source/catalog,Server,args.native_steam_probe,args.native_budget_probe,args.native_budget_cases)
            report['passed']=report['native_budget']['qualified']
            return 0 if report['passed'] else 1
        if args.native_client:report['native_client']=dict(path=str(args.native_client),sha256=digest(args.native_client),borrowed_read_only=True)
        report['checks']=qualify(binary,scratch,args.native_client)
        if args.native_matchmaking_client:report['native_matchmaking_client']=dict(path=str(args.native_matchmaking_client),sha256=digest(args.native_matchmaking_client),borrowed_read_only=True)
        report['matchmaking_checks']=qualify_matchmaking(binary,scratch/'matchmaking',source/catalog,args.native_matchmaking_client)
        spec=importlib.util.spec_from_file_location('rust_qos_controls',source/controls)
        module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
        if args.native_qos_probe:report['native_qos_probe']=dict(path=str(args.native_qos_probe),sha256=digest(args.native_qos_probe),borrowed_read_only=True)
        report['qos']=module.qualify(binary,scratch/'qos',source/catalog,Server,args.native_qos_probe)
        spec=importlib.util.spec_from_file_location('rust_room_controls',source/room_controls)
        module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
        if args.native_room_probe:report['native_room_probe']=dict(path=str(args.native_room_probe),sha256=digest(args.native_room_probe),borrowed_read_only=True)
        report['rooms']=module.qualify(binary,scratch/'rooms',source/catalog,Server,args.native_room_probe)
        spec=importlib.util.spec_from_file_location('rust_eos_controls',source/eos_controls)
        module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
        report['eos']=module.qualify(binary,scratch/'eos',source/catalog,Server)
        spec=importlib.util.spec_from_file_location('rust_steam_controls',source/steam_controls)
        module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
        if args.native_steam_probe:report['native_steam_probe']=dict(path=str(args.native_steam_probe),sha256=digest(args.native_steam_probe),borrowed_read_only=True)
        report['steam']=module.qualify(binary,scratch/'steam',source/catalog,Server,args.native_steam_probe)
        spec=importlib.util.spec_from_file_location('rust_social_controls',source/social_controls)
        module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
        if args.native_social_probe:report['native_social_probe']=dict(path=str(args.native_social_probe),sha256=digest(args.native_social_probe),borrowed_read_only=True)
        report['social']=module.qualify(binary,scratch/'social',source/catalog,Server,args.native_social_probe)
        spec=importlib.util.spec_from_file_location('rust_steam_friends_controls',source/steam_friends_controls)
        module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
        report['steam_friends']=module.qualify(binary,scratch/'steam-friends',source/catalog,Server,args.native_steam_probe)
        assert all(digest(source/p)==h for p,h in files.items())
        report['passed']=True
        print(json.dumps(dict(passed=True,checks=report['checks'],matchmaking_checks=report['matchmaking_checks'],qos=report['qos'],rooms=report['rooms'],eos=report['eos'],steam=report['steam'],social=report['social'],report=str(args.output/'report.json')),indent=2))
        return 0
    except BaseException as error:
        report['error']=str(error);raise
    finally:
        (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')


if __name__=='__main__':raise SystemExit(main())

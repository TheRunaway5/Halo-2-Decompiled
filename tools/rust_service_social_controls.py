"""Real Rust friends/presence HTTP controls; private provider grants only."""
import hashlib
import json
import os
from pathlib import Path
import socket
import sqlite3
import subprocess
import time
import uuid


def qualify(binary, scratch, catalog, Server, native_probe=None):
    scratch.mkdir()
    state = scratch / 'state'
    checks = []
    server = Server(binary, state, scratch, catalog)

    def expect(method, path, status, **values):
        actual, value = server.request(method, path, **values)
        assert actual == status, (method, path, actual, status, value.get('error'))
        return value

    try:
        expect('GET', '/v1/friends', 401)
        counts = expect('GET', '/v1/online/status', 200)
        assert counts['online'] == {'users': 0, 'guests': 0}
        checks.append('real Rust friends authentication and anonymous population routes')
        expect('GET', '/v1/online/status?player=forged', 400)
        expect('GET', '/v1/online/status?party=0&party=1', 400)
        with sqlite3.connect(state / 'registry.sqlite3') as db:
            tokens = ['h2s_' + letter * 43 for letter in 'ABC']
            subjects = [str(76561197960287930 + i) for i in range(3)]
            with db:
                db.executemany('INSERT INTO steam_login_sessions VALUES(?,?,?,?)', [
                    (hashlib.sha256(token.encode()).hexdigest(), subject, 'Private Fixture', int(time.time()) + 3600)
                    for token, subject in zip(tokens, subjects)])
            sessions = [expect('POST', '/v1/account/session', 201, token=token,
                               body={'screen_name': name})['token']
                        for token, name in zip(tokens, ['Café', 'Second', 'Third'])]

            def view(i, party=False):
                return expect('GET', '/v1/friends' + ('?party=1' if party else ''), 200, token=sessions[i])

            def command(i, action, status=200, **body):
                body = dict(request_id=str(uuid.uuid4()), action=action) | body
                return expect('POST', '/v1/friends/commands?party=1', status, token=sessions[i], body=body)

            def party(i):
                return expect('GET', '/v1/party?privacy=1', 200, token=sessions[i])['party']

            def pc(i, action, **extra):
                body = dict(action=action, request_id=str(uuid.uuid4())) | extra
                if action != 'create' and 'version' not in body:
                    body['version'] = party(i)['version']
                return expect('POST', '/v1/party/commands', 200, token=sessions[i], body=body)

            tags = [view(i)['friend_tag'] for i in range(3)]
            public = [command((i + 1) % 3, 'search', tag=tag)['search_result']['id'] for i, tag in enumerate(tags)]
            players = [db.execute('SELECT player FROM player_identities WHERE provider=\'steam\' AND subject=?', (subject,)).fetchone()[0] for subject in subjects]
            assert expect('GET', '/v1/online/status', 200)['online'] == {'users': 3, 'guests': 0}
            assert command(1, 'search', tag='CAFE\u0301#' + tags[0].rsplit('#', 1)[1])['search_result']['id'] == public[0]
            assert command(0, 'search', tag=tags[1])['search_result']['presence'] == 'offline'
            expect('GET', '/v1/friends?party=0&party=1', 400, token=sessions[0])
            expect('GET', '/v1/friends?player=forged', 400, token=sessions[0])
            command(0, 'request', 400, target=public[1], player='forged')
            checks.append('real account observations and anonymous identity-free counts; native Unicode tags and nonfriend presence privacy; duplicate/forged inputs rejected')

            rid = str(uuid.uuid4())
            command(0, 'request', target=public[1], request_id=rid)
            command(0, 'request', target=public[1], request_id=rid)
            command(0, 'remove', 409, target=public[1], request_id=rid)
            command(0, 'accept', 409, target=public[1])
            command(1, 'request', 409, target=public[0])
            assert view(0)['friends'][0]['presence'] == 'offline'
            assert view(1)['friends'][0]['state'] == 'incoming'
            command(1, 'accept', target=public[0])
            assert all(view(i, True)['friends'][0]['presence'] == 'online' for i in (0, 1))
            assert db.execute('SELECT count(*) FROM player_friend_commands WHERE request=?', (rid,)).fetchone()[0] == 1
            checks.append('actual sender/recipient consent, request replay/conflict, no automatic acceptance and accepted-friend presence')

            pc(0, 'create')
            joined = view(1, True)['friends'][0]
            assert joined['party_joinable'] and len(joined['party_context']) == 36
            private = party(0)['privacy']
            pc(0, 'privacy', privacy=2, privacy_revision=private['revision'], privacy_context=private['context'])
            assert view(1, True)['friends'][0]['party_joinable'] is False
            private = party(0)['privacy']
            pc(0, 'privacy', privacy=0, privacy_revision=private['revision'], privacy_context=private['context'])
            pc(0, 'invite_friend', target=public[1])
            before = db.execute('SELECT revision FROM player_friend_block_epoch').fetchone()[0]
            with db:
                db.executescript("CREATE TABLE social_commit_failure(player TEXT REFERENCES players(id) DEFERRABLE INITIALLY DEFERRED);"
                                 "CREATE TRIGGER social_fail_block AFTER INSERT ON player_friend_blocks BEGIN INSERT INTO social_commit_failure VALUES('private-missing'); END;")
            rid = str(uuid.uuid4())
            command(1, 'block', 500, target=public[0], request_id=rid)
            assert db.execute('SELECT revision FROM player_friend_block_epoch').fetchone()[0] == before
            assert view(1, True)['friends'][0]['state'] == 'friend'
            assert view(1, True)['friends'][0]['party_joinable']
            assert db.execute('SELECT count(*) FROM party_invites').fetchone()[0] == 1
            with db:
                db.execute('DROP TRIGGER social_fail_block')
            blocked = command(1, 'block', target=public[0], request_id=rid)
            assert blocked['friends'][0]['state'] == 'blocked' and blocked['friends'][0]['presence'] == 'offline'
            assert view(0)['friends'] == []
            assert db.execute('SELECT count(*) FROM party_invites').fetchone()[0] == 0
            assert db.execute('SELECT revision FROM player_friend_block_epoch').fetchone()[0] == before + 1
            command(0, 'request', 404, target=public[1])
            assert command(0, 'search', tag=tags[1])['search_result']['presence'] == 'offline'
            command(1, 'unblock', target=public[0])
            assert view(0)['friends'] == view(1)['friends'] == []
            checks.append('real friend party privacy/context and invitation policy; failed block COMMIT restores relationship/cache epoch/invites, retry revokes both directions without auto-accepting on unblock')

            command(0, 'request', target=public[1]); command(0, 'cancel', target=public[1])
            command(0, 'request', target=public[1]); command(1, 'decline', target=public[0])
            command(0, 'request', target=public[1]); command(1, 'accept', target=public[0])
            renamed = expect('POST', '/v1/account/session', 201, token=tokens[1], body={'screen_name': 'Renamed'})['token']
            sessions[1] = renamed
            row = view(0)['friends'][0]
            assert row['id'] == public[1] and row['name'] == 'Renamed'
            assert row['friend_tag'].rsplit('#', 1)[1] == tags[1].rsplit('#', 1)[1]
            command(0, 'search', 404, tag=tags[1])
            command(0, 'remove', target=public[1])
            checks.append('actual cancel, decline, remove and canonical rename preserve the public identifier/tag discriminator')

            payload = json.dumps(dict(action='request', request_id=str(uuid.uuid4()), target=public[0])).encode()
            with socket.create_connection((server.host, server.port), timeout=5) as wire:
                wire.sendall((f'POST /v1/friends/commands HTTP/1.1\r\nHost: localhost\r\nContent-Type: application/json\r\nAuthorization: Bearer {sessions[2]}\r\nContent-Length: {len(payload)}\r\n\r\n').encode() + payload[:1])
                expect('DELETE', '/v1/account/session', 200, token=sessions[2])
                wire.sendall(payload[1:])
                assert b' 401 ' in wire.recv(8192).split(b'\r\n')[0]
            assert not db.execute('SELECT 1 FROM player_friendships WHERE requester=?', (players[2],)).fetchall()
            checks.append('revocation during actual body upload is rechecked before friend SQL, with no provisional relationship')

            # Declared private SQL roster fixtures exercise the real 128-row IO
            # continuation and all 250 public rows, without claiming providers.
            with db:
                for i in range(250):
                    who = 'private-social-roster-' + str(i)
                    db.execute('INSERT INTO players VALUES(?,?,?)', (who, 'steam', who))
                    db.execute('INSERT INTO player_profiles(player,screen_name) VALUES(?,?)', (who, 'Roster ' + str(i)))
                    db.execute('INSERT INTO history_players VALUES(?,?)', (who, str(uuid.uuid4())))
                    db.execute('INSERT INTO player_friendships VALUES(?,?,?,1)', (*sorted((players[0], who)), players[0]))
            roster = view(0, True)['friends']
            assert len(roster) == 250 and len({r['id'] for r in roster}) == 250
            assert all(r['presence'] == 'offline' and r['party_joinable'] is False for r in roster)
            command(0, 'request', 409, target=public[1])
            checks.append('private 250-row SQL roster crosses the actual 128-row continuation, preserves all unique private public projections and rejects the next friendship at capacity')

            maps = sorted({g['map'] for entry in json.loads(catalog.read_text())['playlists'] for g in entry['games']})
            match = expect('POST', '/v1/matchmaking/session', 201, token=tokens[1], body=dict(screen_name='Renamed', maps=maps, language=0, compatibility='halo2-native-queue-v16', local_players=[{'screen_name':'Guest'}]))['token']
            online = expect('GET', '/v1/online/status', 200)
            assert online['online'] == {'users': 2, 'guests': 1}
            lab = next(p for p in expect('GET', '/v1/matchmaking/playlists', 200, token=match)['playlists'] if p['id'] == 'lab-test')
            expect('POST', '/v1/matchmaking/ticket', 201, token=match, body=dict(playlist_id=lab['id'], revision=lab['revision'], request_id=str(uuid.uuid4())))
            counts = expect('GET', '/v1/online/status', 200)
            assert counts['queued'] == {'users': 1, 'guests': 1}
            assert next(p for p in counts['playlists'] if p['id'] == 'lab-test')['queued'] == counts['queued']
            encoded = json.dumps(counts)
            assert all(secret not in encoded for secret in players + subjects + public + tokens + sessions)
            expect('DELETE', '/v1/matchmaking/ticket?cancel=1', 200, token=match)
            expect('DELETE', '/v1/matchmaking/session', 200, token=match)
            assert expect('GET', '/v1/online/status', 200)['online'] == {'users': 2, 'guests': 0}
            checks.append('actual account/matching-owner deduplication, transient guest and queued playlist counts, cancellation and anonymous response privacy')

            old = sessions[0]
            server.close()
            server = Server(binary, state, scratch, catalog)
            expect('GET', '/v1/friends', 401, token=old)
            assert expect('GET', '/v1/online/status', 200)['online'] == {'users': 0, 'guests': 0}
            sessions[0] = expect('POST', '/v1/account/session', 201, token=tokens[0], body={'screen_name':'Café'})['token']
            assert view(0)['friend_tag'] == tags[0] and len(view(0)['friends']) == 250
            checks.append('real process restart retains friendships/tags and retires presence/account grants without reviving pending capabilities')
            started = time.monotonic()
            while time.monotonic() - started < 30.2:
                expect('GET', '/v1/online/status', 200)
                time.sleep(1)
            assert expect('GET', '/v1/online/status', 200)['online'] == {'users': 0, 'guests': 0}
            assert view(0)['friend_tag'] == tags[0]
            assert expect('GET', '/v1/online/status', 200)['online'] == {'users': 1, 'guests': 0}
            checks.append('actual unshortened thirty-second activity window expires despite repeated anonymous population reads; using the still-valid account grant restores observed activity')
        native = None
        if native_probe:
            with sqlite3.connect(state / 'registry.sqlite3') as db:
                native_tokens = ['h2s_' + c * 43 for c in 'YZ']
                native_subjects = ['76561197960287988', '76561197960287989']
                with db:
                    db.executemany('INSERT INTO steam_login_sessions VALUES(?,?,?,?)', [
                        (hashlib.sha256(t.encode()).hexdigest(), s, 'Private Native Fixture', int(time.time()) + 3600)
                        for t, s in zip(native_tokens, native_subjects)])
            env = dict(os.environ, H2_ACCOUNT_URL=f'http://{server.host}:{server.port}',
                       H2_ACCOUNT_ALLOW_LOOPBACK_HTTP='1', H2_TEST_SOCIAL_TOKEN_A=native_tokens[0],
                       H2_TEST_SOCIAL_TOKEN_B=native_tokens[1], H2_TEST_SOCIAL_SUBJECT_A=native_subjects[0],
                       H2_TEST_SOCIAL_SUBJECT_B=native_subjects[1], H2_ONLINE_TRANSPORT='direct',
                       H2_EOS_ENABLED='0', H2_EOS_PROFILES='0')
            run = subprocess.run([str(native_probe)], env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=40)
            assert run.returncode == 0, run.stdout.decode(errors='replace')[-2000:]
            assert b'PASS native friend consent' in run.stdout
            native = dict(passed=True, scope='Actual native OnlineAccount friend commands/views and party join against Rust HTTP; private provider handoff, no played world or real Steam grant.')
            checks.append('real native friend search/request/recipient acceptance, private presence, party context/join and bilateral block over Rust HTTP')
        assert expect('GET', '/health', 200)['ready'] is False
        return dict(qualified=True, checks=checks, native=native, real_provider=False,
                    native_played_world=False, scope='Real Rust HTTP/SQLite; private provider fixtures, no original or delivered acceptance.')
    finally:
        server.close()

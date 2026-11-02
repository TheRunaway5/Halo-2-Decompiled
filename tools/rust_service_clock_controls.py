"""Separate credential and registration clocks in private Rust HTTP processes."""
import hashlib
import asyncio
import sqlite3
import time


def preflight(binary, scratch, catalog, Server):
    scratch.mkdir()
    state = scratch / 'state'
    server = Server(binary, state, scratch, catalog)
    try:
        observed = int(time.time())
        floor = observed + 600
        tokens = ['h2s_' + letter * 43 for letter in 'KLM']
        subjects = ['76561197960287960', '76561197960287961', '76561197960287962']
        with sqlite3.connect(state / 'registry.sqlite3') as db:
            with db:
                db.executemany('INSERT INTO steam_login_sessions VALUES(?,?,?,?)', [
                    (hashlib.sha256(token.encode()).hexdigest(), subject,
                     'Private clock fixture', observed + lifetime)
                    for token, subject, lifetime in zip(tokens, subjects, (300, 900, -1))
                ])
                db.execute('UPDATE clock_floor SET value=? WHERE id=1', (floor,))
        # SteamLogin.owner uses its own wall clock. ClansAPI.login passes
        # Store.now to identity resolution, clamping registration independently.
        statuses = [server.request('POST', '/v1/account/session', token=token,
                                   body={'screen_name': 'Clock pilot ' + str(i)})[0]
                    for i, token in enumerate(tokens)]
        with sqlite3.connect(state / 'registry.sqlite3') as db:
            allocated = db.execute('SELECT count(*) FROM player_identities WHERE subject=?',
                                   (subjects[2],)).fetchone()[0]
            retained = db.execute('SELECT value FROM clock_floor WHERE id=1').fetchone()[0]
            joined = [db.execute('SELECT p.joined_at FROM player_profiles p JOIN player_identities i ON i.player=p.player WHERE i.subject=?',
                                 (subject,)).fetchone()[0] for subject in subjects[:2]]
        return dict(passed=statuses == [201, 201, 401] and joined == [floor, floor] and allocated == 0,
                    contract='Steam expiry uses credential wall time; social account registration uses Store.now with the persisted floor.',
                    statuses=statuses, expected_statuses=[201, 201, 401],
                    registration_dates=joined, expected_registration_dates=[floor, floor],
                    expired_identity_allocations=allocated,
                    floor_retained=retained >= floor,
                    fixture='Private provider grants and a later registration clock floor; no system-clock mutation.',
                    real_provider=False, original_parity=False)
    finally:
        server.close()


def qualify(binary, scratch, catalog, Server):
    scratch.mkdir()
    checks = []
    initial = preflight(binary, scratch / 'preflight', catalog, Server)
    assert initial['passed'], initial
    checks.append('real HTTP keeps Steam expiry on credential wall time and clamps social registration to its persisted floor')
    state = scratch / 'lifecycle-state'
    server = Server(binary, state, scratch, catalog)

    def sql(query, params=()):
        with sqlite3.connect(state / 'registry.sqlite3') as db:
            return db.execute(query, params).fetchall()

    def expect(method, path, status=200, **kwargs):
        actual, value = server.request(method, path, **kwargs)
        assert actual == status, (path, actual, status, value.get('error'))
        return value

    tokens = ['h2s_' + letter * 43 for letter in 'NPQRST']
    subjects = [str(76561197960287970 + i) for i in range(len(tokens))]

    def seed():
        with sqlite3.connect(state / 'registry.sqlite3') as db:
            with db:
                db.executemany('INSERT INTO steam_login_sessions VALUES(?,?,?,?)', [
                    (hashlib.sha256(token.encode()).hexdigest(), subject,
                     'Private clock fixture', int(time.time()) + 300)
                    for token, subject in zip(tokens, subjects)
                ])

    def login(i, name=None, status=201):
        return expect('POST', '/v1/account/session', status, token=tokens[i],
                      body={'screen_name': name or 'Clock pilot ' + str(i)})

    def joined(i):
        rows = sql('SELECT p.joined_at FROM player_profiles p JOIN player_identities i ON i.player=p.player WHERE i.subject=?',
                   (subjects[i],))
        return rows[0][0] if rows else None

    try:
        seed()
        before = int(time.time())
        first = login(0)['token']
        date = joined(0)
        assert before <= date <= int(time.time())
        assert sql('SELECT value FROM clock_floor WHERE id=1') == [(date,)]
        floor = date + 900
        sql('UPDATE clock_floor SET value=? WHERE id=1', (floor,))
        login(0, 'Clock renamed')
        assert joined(0) == date
        login(1)
        assert joined(1) == floor
        checks.append('new registration advances the SQL floor, returning login preserves its original date, and a later profile uses the retained floor')
        sql('UPDATE player_profiles SET joined_at=NULL WHERE player IN (SELECT player FROM player_identities WHERE subject=?)', (subjects[0],))
        login(0)
        assert joined(0) is None
        checks.append('returning legacy profiles keep unknown registration dates without fabricated backfill')
        server.close()
        server = Server(binary, state, scratch, catalog)
        expect('GET', '/v1/account/profile', 401, token=first)
        login(2)
        assert joined(0) is None and joined(1) == joined(2) == floor
        assert sql('SELECT value FROM clock_floor WHERE id=1') == [(floor,)]
        checks.append('actual process restart retains registration dates and the persisted floor while retiring old account grants')

        async def wait_case(i, *, expire=False):
            import aiohttp
            with sqlite3.connect(state / 'registry.sqlite3') as db:
                db.execute('BEGIN IMMEDIATE')
                async with aiohttp.ClientSession() as client:
                    async def send():
                        async with client.post(f'http://{server.host}:{server.port}/v1/account/session',
                                               headers={'Authorization': 'Bearer ' + tokens[i]},
                                               json={'screen_name': 'Clock pilot ' + str(i)}) as response:
                            await response.read()
                            assert response.status == (401 if expire else 201), response.status
                    task = asyncio.create_task(send())
                    await asyncio.sleep(.25)
                    if expire:
                        db.execute('UPDATE steam_login_sessions SET expires=? WHERE token_hash=?',
                                   (int(time.time()) - 1, hashlib.sha256(tokens[i].encode()).hexdigest()))
                    else:
                        db.execute('UPDATE clock_floor SET value=? WHERE id=1', (floor + 600,))
                    db.commit()
                    await task

        asyncio.run(wait_case(3))
        assert joined(3) == floor + 600
        checks.append('a newer floor committed during the actual external SQL write lock is read before registration')
        asyncio.run(wait_case(4, expire=True))
        assert joined(4) is None
        assert sql('SELECT count(*) FROM player_identities WHERE subject=?', (subjects[4],)) == [(0,)]
        checks.append('a provider credential that expires during the SQL lock wait cannot allocate an identity despite the independent future registration floor')

        server.close()
        state = scratch / 'failure-state'
        server = Server(binary, state, scratch, catalog)
        seed()
        sql('CREATE TABLE clock_failure(player TEXT REFERENCES players(id) DEFERRABLE INITIALLY DEFERRED)')
        sql("CREATE TRIGGER fail_registration AFTER INSERT ON player_profiles BEGIN INSERT INTO clock_failure VALUES('missing-private-player'); END")
        login(5, status=500)
        assert sql('SELECT value FROM clock_floor WHERE id=1') == [(0,)]
        assert sql('SELECT count(*) FROM players') == [(0,)]
        assert sql('SELECT count(*) FROM player_profiles') == [(0,)]
        assert sql('SELECT count(*) FROM clock_failure') == [(0,)]
        sql('DROP TRIGGER fail_registration')
        reply = login(5)
        expect('GET', '/v1/clans', token=reply['token'])
        assert sql('SELECT count(*) FROM players') == [(1,)]
        assert sql('SELECT value FROM clock_floor WHERE id=1') == [(joined(5),)]
        checks.append('real deferred COMMIT failure rolls back the floor, canonical identity and profile; retry publishes one usable account')
        return dict(qualified=True, checks=checks, preflight=initial,
                    real_provider=False, original_parity=False,
                    native_played_world=False, system_clock_mutated=False)
    except BaseException as error:
        raise AssertionError(dict(error=repr(error), completed_checks=checks)) from error
    finally:
        server.close()

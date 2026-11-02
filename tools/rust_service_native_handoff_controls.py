"""Native v16 settings and failed-launch isolation; no played-world claim.

The intentionally missing client configuration must refuse the admitted EOS
transport. Passing this control never qualifies the positive handoff.
"""
import hashlib
import json
import os
import sqlite3
import subprocess
import time


def qualify(binary, scratch, catalog, Server, native_probe):
    assert native_probe, 'An actual native handoff probe is required'
    scratch.mkdir()
    state = scratch / 'state'
    server = Server(binary, state, scratch, catalog)
    processes = []
    streams = []
    try:
        entries = json.loads(catalog.read_text())['playlists']
        lab = next(row for row in entries if row['id'] == 'lab-test')
        maps = sorted({game['map'] for game in lab['games']})
        tokens = ['h2s_' + letter * 43 for letter in 'XY']
        subjects = ['76561197960287980', '76561197960287981']
        with sqlite3.connect(state / 'registry.sqlite3') as db:
            with db:
                db.executemany('INSERT INTO steam_login_sessions VALUES(?,?,?,?)', [
                    (hashlib.sha256(token.encode()).hexdigest(), subject,
                     'Private native handoff', int(time.time()) + 3600)
                    for token, subject in zip(tokens, subjects)
                ])
        config = scratch / 'eos-disabled.env'
        config.write_text('H2_EOS_ENABLED=0\nH2_EOS_PROFILES=0\n')
        config.chmod(0o600)
        def publication_snapshot():
            tables = ('match_results', 'match_result_players', 'match_result_teams',
                      'match_result_memberships', 'match_result_skill',
                      'match_result_clan_skill', 'carnage_reports', 'carnage_players',
                      'carnage_weapon_stats', 'carnage_player_accuracy',
                      'carnage_weapon_accuracy')
            with sqlite3.connect(state / 'registry.sqlite3') as db:
                counts = {name: db.execute('SELECT count(*) FROM ' + name).fetchone()[0]
                          for name in tables}
                skill = {name: dict(rows=row[0], xp=row[1])
                         for name in ('playlist_skill', 'clan_playlist_skill')
                         for row in [db.execute('SELECT count(*),coalesce(sum(xp),0) FROM ' + name).fetchone()]}
            return dict(result_rows=counts, skill=skill)

        baseline = publication_snapshot()
        assert not any(baseline['result_rows'].values())
        attempts = []
        # Reuse the canonical identities and grants in the same Rust process.
        # New native processes distinguish server cleanup from native in-memory
        # retry, which remains a separate acceptance requirement.
        for attempt in range(2):
            current = []
            for i, (token, subject) in enumerate(zip(tokens, subjects)):
                environment = {key: value for key, value in os.environ.items()
                               if not key.startswith('H2_EOS_')}
                environment.update(H2_EOS_CONFIG=str(config),
                                   H2_EOS_CACHE_DIR=str(scratch / f'eos-cache-{attempt}-{i}'),
                                   H2_ACCOUNT_URL=f'http://{server.host}:{server.port}',
                                   H2_ACCOUNT_ALLOW_LOOPBACK_HTTP='1',
                                   H2_HANDOFF_URL=f'http://{server.host}:{server.port}',
                                   H2_HANDOFF_TOKEN=token, H2_HANDOFF_SUBJECT=subject,
                                   H2_HANDOFF_NAME='Native Handoff ' + str(i),
                                   H2_HANDOFF_MAPS=','.join(maps),
                                   H2_HANDOFF_PLAYLIST=lab['id'],
                                   H2_HANDOFF_GAMES=json.dumps(lab['games']))
                path = scratch / f'native-{attempt}-{i}.log'
                stream = path.open('wb')
                streams.append(stream)
                process = subprocess.Popen([str(native_probe)], env=environment,
                                           stdout=stream, stderr=subprocess.STDOUT)
                processes.append(process)
                current.append((process, stream, path))
            deadline = time.monotonic() + 85
            while any(p.poll() is None for p, _, _ in current) and time.monotonic() < deadline:
                time.sleep(.02)
            rows = []
            for process, stream, path in current:
                timed_out = process.poll() is None
                if timed_out:
                    process.terminate()
                    try:
                        process.wait(timeout=3)
                    except subprocess.TimeoutExpired:
                        process.kill()
                        process.wait()
                stream.flush()
                text = path.read_text(errors='replace')
                parsed = [json.loads(line) for line in text.splitlines() if line.startswith('{')]
                row = dict(returncode=process.returncode, timed_out=timed_out,
                           **(parsed[-1] if parsed else dict(error='Native receipt missing')))
                rows.append(row)
            sdk = [row.get('sdk_available') for row in rows]
            expected = ('Missing H2_EOS_PRODUCT_ID' if sdk == [True, True]
                        else 'This build does not include the EOS SDK')
            errors = [row.get('native_error') for row in rows]
            settings = all(row.get('full_settings_validated') is True and
                           row.get('settings_bytes') == 304 for row in rows)
            same_match = settings and all(rows[0].get(key) == rows[1].get(key)
                                         for key in ('game_id', 'map', 'variant'))
            refused = (sdk in ([True, True], [False, False]) and expected in errors and
                       all(error in (expected, 'Matchmaking failed. Try again.') for error in errors) and
                       all(row['returncode'] == 1 and not row['timed_out'] and
                           row.get('queue_status') == 9 and
                           row.get('native_session_taken') is False and
                           row.get('native_loading_reached') is False for row in rows))
            published = publication_snapshot()
            no_results = published['result_rows'] == baseline['result_rows']
            no_xp = all(row['xp'] == 0 for row in published['skill'].values())
            no_sdk_cache = not any(scratch.glob('eos-cache-*'))
            passed = bool(same_match and refused and no_results and no_xp and no_sdk_cache)
            attempts.append(dict(negative_control_passed=passed, native_clients=rows,
                                 full_settings_validated=settings, same_admitted_match=same_match,
                                 expected_refusal=expected, publication=published,
                                 no_result_publication=no_results, no_xp_awarded=no_xp,
                                 no_sdk_cache_created=no_sdk_cache))
            if not passed:
                break
        return dict(qualified=False, positive_handoff_qualified=False,
                    negative_control_passed=len(attempts) == 2 and all(
                        row['negative_control_passed'] for row in attempts),
                    attempts=attempts, configured_client=False,
                    negative_control='Missing EOS client configuration must refuse v16 without changing its transport.',
                    native_clients_restarted_between_attempts=True,
                    same_process_native_retry_verified=False,
                    same_service_and_provider_grants=True,
                    source_catalog_edited=False,
                    source_catalog_sha256=hashlib.sha256(catalog.read_bytes()).hexdigest(),
                    declared_map_inventory=maps, installed_map_admission_verified=False,
                    real_provider=False, played_world=False, original_parity=False,
                    full_goal_complete=False)
    finally:
        for process in processes:
            if process.poll() is None:
                process.terminate()
                try:
                    process.wait(timeout=3)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait()
        for stream in streams:
            stream.close()
        server.close()

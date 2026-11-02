"""Actual native menus/SDL renderer against the production Rust service.

Only the external Steam handoff is a private fixture. The source catalog is
unchanged; no Python API, coordinator, countdown or ready result is supplied.
Run under run_test.py. All processes, private state and raw frames are owned.
"""
import contextlib
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import selectors
import shutil
import sqlite3
import subprocess
import time


def sha(path):
    with path.open('rb') as stream:return hashlib.file_digest(stream,'sha256').hexdigest()


@contextlib.contextmanager
def display(scratch,enabled):
    if not enabled:
        yield None;return
    executable=shutil.which('Xvfb')
    if not executable:raise RuntimeError('Rendered Rust frontend acceptance requires Xvfb')
    read_fd,write_fd=os.pipe();process=None
    try:
        with (scratch/'xvfb.log').open('wb') as log:
            process=subprocess.Popen([executable,'-displayfd',str(write_fd),'-screen','0','1280x1024x24','-nolisten','tcp'],
                pass_fds=(write_fd,),stdout=log,stderr=subprocess.STDOUT)
            os.close(write_fd);write_fd=None
            selector=selectors.DefaultSelector();selector.register(read_fd,selectors.EVENT_READ)
            try:
                if not selector.select(10):raise RuntimeError('Owned Xvfb did not publish its display')
                value=os.read(read_fd,64).decode().strip()
                if not value.isdecimal() or process.poll() is not None:raise RuntimeError('Owned Xvfb failed before native rendering')
                yield ':'+value
            finally:selector.close()
    finally:
        os.close(read_fd)
        if write_fd is not None:os.close(write_fd)
        if process is not None:
            if process.poll() is None:process.terminate()
            try:process.wait(timeout=3)
            except subprocess.TimeoutExpired:process.kill();process.wait()


def frames(scratch):
    from PIL import Image
    folder=scratch/'lobby-buttons-gpu';rows=json.loads((folder/'observations.json').read_text())
    mouse=json.loads((folder/'mouse-inputs.json').read_text())
    assert [(r['phase'],r['screen']) for r in mouse]==[
        ('choose-matchmaking-playlist',23),('playlist-row',237),('start-game',14)]
    assert all(r['actual_sdl_events'] and not r['direct_menu_activation'] and r['font_material'].startswith('font/')
        and 0<=r['x']<r['width'] and 0<=r['y']<r['height'] for r in mouse)
    expected={'online':186,'pregame':14,'setup':23,'privacy-open':25,'privacy-invite-only':14,'privacy-setup':23,
        'privacy-current':25,'privacy-same-mode-return':14,'privacy-setup-reset':23,'privacy-reset':25,
        'privacy-open-restored':14,'setup-restored':23,'playlist':237,'selected-lobby':14,'countdown-start':14,
        'countdown-cancel':14,'countdown-restart':14,'countdown-3':14,'countdown-2':14,'countdown-1':14,
        'search':210,'leave-confirm':7,'returned-lobby':14}
    assert len(rows)==len(expected) and {r['phase']:r['screen'] for r in rows}==expected
    hashes=set()
    for row in rows:
        path=folder/row['file'];assert path.is_file() and not path.is_symlink()
        with Image.open(path) as image:
            assert image.size==(960,720);rgb=image.convert('RGB').tobytes()
        # Actual rendered output must contain more than a uniform clear image.
        assert len(set(rgb[::3]))>32
        pin=hashlib.sha256(rgb).hexdigest();hashes.add(pin)
        row.update(raw_bytes=path.stat().st_size,raw_sha256=sha(path),decoded_rgb_sha256=pin,raw_retained=False)
    assert len(hashes)>12
    stages={r['phase']:r for r in rows}
    assert stages['privacy-open']['privacy']['mode']==0 and stages['privacy-invite-only']['privacy']['mode']==1
    assert stages['privacy-current']['selected']==1
    assert stages['privacy-same-mode-return']['privacy']['revision']>stages['privacy-current']['privacy']['revision']
    assert stages['privacy-open-restored']['privacy']['mode']==0
    assert [stages['countdown-'+str(i)]['lobby_countdown'] for i in [3,2,1]]==[3,2,1]
    return dict(passed=True,observations=rows,mouse_inputs=mouse,retained_frames=[],actual_sdl_events=True,
        actual_sdl_mouse_events=True,actual_vulkan_frames=True,
        xbox_framebuffer_parity=False,physical_installed_input=False)


def qualify(binary,scratch,catalog,Server,native_probe,mcc_root,case):
    assert case in ('blocked','complete','rendered') and native_probe and mcc_root
    scratch.mkdir();source=Path(__file__).resolve().parents[1]
    spec=importlib.util.spec_from_file_location('rust_frontend_mcc',source/'tests/helpers/mcc_online_ui.py')
    helper=importlib.util.module_from_spec(spec)
    import sys
    sys.modules[spec.name]=helper;spec.loader.exec_module(helper)
    cache=helper.validate_cache(source,mcc_root);maps=cache/'halo2/h2_maps_win64_dx11'
    state=scratch/'state';server=Server(binary,state,scratch,catalog);process=None
    token='h2s_'+'F'*43;subject='76561197960287991'
    result_tables=('match_results','match_result_players','match_result_teams','match_result_memberships','match_result_skill',
        'match_result_clan_skill','carnage_reports','carnage_players','carnage_weapon_stats','carnage_player_accuracy','carnage_weapon_accuracy')
    def publications():
        with sqlite3.connect(state/'registry.sqlite3') as db:
            return dict(results={name:db.execute('SELECT count(*) FROM '+name).fetchone()[0] for name in result_tables},
                xp={name:db.execute('SELECT coalesce(sum(xp),0) FROM '+name).fetchone()[0] for name in ('playlist_skill','clan_playlist_skill')})
    try:
        shim=scratch/'native-party-io-delay.so'
        shim_source=Path(__file__).with_name('rust_service_frontend_io_delay.cpp')
        compile_command=['g++','-std=c++20','-O1','-shared','-fPIC',str(shim_source),'-o',str(shim),'-ldl','-pthread']
        compiled=subprocess.run(compile_command,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
        assert compiled.returncode==0,compiled.stdout.decode(errors='replace')[-3000:]
        with sqlite3.connect(state/'registry.sqlite3') as db:
            with db:db.execute('INSERT INTO steam_login_sessions VALUES(?,?,?,?)',(hashlib.sha256(token.encode()).hexdigest(),subject,'Steam Fixture',int(time.time())+3600))
        before=publications();assert not any(before['results'].values()) and not any(before['xp'].values())
        config=scratch/'eos-disabled.env';config.write_text('H2_EOS_ENABLED=0\nH2_EOS_PROFILES=0\n');config.chmod(0o600)
        environment={k:v for k,v in os.environ.items() if not k.startswith(('H2_EOS_','H2_TEST_'))}
        endpoint=f'http://{server.host}:{server.port}'
        environment.update(H2_EOS_CONFIG=str(config),H2_EOS_CACHE_DIR=str(scratch/'unused-eos-cache'),
            H2_ONLINE_TRANSPORT='direct' if case=='blocked' else 'eos',H2_ACCOUNT_URL=endpoint,H2_ACCOUNT_ALLOW_LOOPBACK_HTTP='1',
            H2_TEST_FRONTEND_STEAM='1',H2_TEST_STEAM_TOKEN=token,H2_TEST_STEAM_SUBJECT=subject,
            H2_TEST_BLOCKED_PLAYLIST='1' if case=='blocked' else '0',H2_TEST_COMPLETE_PLAYLIST='0' if case=='blocked' else '1',
            H2_TEST_LOBBY_BUTTONS_GPU='1' if case=='rendered' else '0',SDL_AUDIODRIVER='dummy',
            http_proxy='http://127.0.0.1:1',HTTP_PROXY='http://127.0.0.1:1',NO_PROXY='',no_proxy='',
            XDG_CONFIG_HOME=str(scratch/'config'),XDG_DATA_HOME=str(scratch/'data'))
        environment.update(LD_PRELOAD=str(shim),HALO2_TEST_SCRATCH=str(scratch),XDG_CACHE_HOME=str(scratch/'cache'),
            MESA_SHADER_CACHE_DIR=str(scratch/'mesa-cache'),__GL_SHADER_DISK_CACHE_PATH=str(scratch/'shader-cache'))
        runtime=scratch/'runtime';runtime.mkdir(mode=0o700);environment['XDG_RUNTIME_DIR']=str(runtime)
        command=[str(native_probe),str(maps),'--matchmaking-url',endpoint,'--callback-port','0']
        log=scratch/'native.log';started=time.monotonic()
        with display(scratch,case=='rendered') as screen:
            if screen:environment['DISPLAY']=screen
            with log.open('wb') as stream:
                process=subprocess.Popen(command,cwd=scratch,env=environment,stdout=stream,stderr=subprocess.STDOUT)
                while process.poll() is None:
                    if time.monotonic()-started>160:raise RuntimeError('Native Rust frontend case timed out: '+case)
                    if log.stat().st_size>8*1024*1024:raise RuntimeError('Native Rust frontend output exceeded its bound')
                    time.sleep(.02)
        output=log.read_text(errors='replace')
        if process.returncode!=0:
            print(output[-12000:]);raise RuntimeError('Actual native Rust frontend failed: '+case)
        prefix='NATIVE_UI_CATALOG '
        catalog_rows=[json.loads(line[len(prefix):]) for line in output.splitlines() if line.startswith(prefix)]
        assert len(catalog_rows)==1 and catalog_rows[0]['steam_handoff_fixture']
        view=catalog_rows[0];assert len(view['playlists'])==14
        assert view['compatibility']==('halo2-native-queue-v12' if case=='blocked' else 'halo2-native-queue-v16')
        selected=next(r for r in view['playlists'] if r['id']=='rumble-pit');assert selected['eligible']==(case!='blocked')
        if case=='blocked':assert not any(r['eligible'] for r in view['playlists']) and 'never enqueue' in output
        elif case=='complete':assert 'authoritative shared countdown -> queue -> cancel' in output
        else:assert 'PASS actual SDL/Vulkan source Setup23' in output
        after=publications();assert after==before
        status,health=server.request('GET','/health');assert status==200 and health['ready'] is False
        assert not (scratch/'unused-eos-cache').exists()
        result=dict(passed=True,case=case,catalog_sha256=sha(catalog),source_catalog_unchanged=True,catalog=view,
            native_binary_sha256=sha(native_probe),native_log_bytes=log.stat().st_size,native_log_sha256=sha(log),
            seconds=round(time.monotonic()-started,3),publications_before=before,publications_after=after,
            actual_native_menu=True,rust_http_policy=True,python_api_or_coordinator=False,steam_provider_fixture=True,
            native_party_reply_io_delay_ms=150,io_delay_source_sha256=sha(shim_source),
            genuine_steam=False,genuine_eos=False,gameplay_played=False,installed_input_verified=False,
            raw_outputs_owned=True,retained_large_artifacts=[],scope='Unchanged full source catalog, actual native menu handlers and Rust service; private external Steam handoff. No played world, positive EOS, original Live or installed delivery claim.')
        if case=='rendered':result['rendered']=frames(scratch)
        return result
    finally:
        if process and process.poll() is None:
            process.terminate()
            try:process.wait(timeout=3)
            except subprocess.TimeoutExpired:process.kill();process.wait()
        server.close()

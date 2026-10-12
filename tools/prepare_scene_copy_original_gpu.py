#!/usr/bin/env python3
"""Create a fresh private research session; never launch or touch an old VM."""
from pathlib import Path
from datetime import datetime, timezone
import hashlib, json, resource, socket, subprocess

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'analysis/multiplayer-shading/original-weather-20261009/session1'
PARENT = ROOT / 'analysis/multiplayer/game-variants/scene-copy-original-gpu-20261012'

def digest(path):
    h = hashlib.sha256()
    with path.open('rb') as f:
        for block in iter(lambda: f.read(8 << 20), b''):
            h.update(block)
    return h.hexdigest()

def child_limit():
    resource.setrlimit(resource.RLIMIT_MEMLOCK, (0, 0))

def run(args, **kwargs):
    p = subprocess.run(args, capture_output=True, text=True, **kwargs)
    return {'argv': args, 'returncode': p.returncode,
            'stdout': p.stdout, 'stderr': p.stderr}

def free_port():
    with socket.socket() as s:
        s.bind(('127.0.0.1', 0))
        return s.getsockname()[1]

def main():
    out = PARENT / ('attempt-' + datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%S.%fZ'))
    out.mkdir(parents=True, exist_ok=False)
    report = {'directory': str(out), 'source_files': [],
              'parent_memlock': list(resource.getrlimit(resource.RLIMIT_MEMLOCK)),
              'qemu_img_child_memlock': [0, 0], 'guest_instruction_patches': [],
              'supplied_returns': [], 'snapshot': 'lockout-weather-outdoor'}
    for name in ['base-hdd.qcow2', 'session-hdd.qcow2', 'eeprom.bin']:
        source, target = SOURCE / name, out / name
        before = digest(source)
        result = run(['cp', '--reflink=auto', '--sparse=always', str(source), str(target)])
        assert result['returncode'] == 0, result
        copied, after = digest(target), digest(source)
        assert before == copied == after
        report['source_files'].append({'source': str(source), 'copy': str(target),
                                      'sha256': before, 'source_unchanged': True})
    # A null *old* backing node prevents opening the retained source chain.
    # The new backing metadata is an ordinary private filename compatible with xemu.
    image = 'json:' + json.dumps({'driver': 'qcow2', 'file': {'driver': 'file',
             'filename': str(out / 'session-hdd.qcow2'), 'aio': 'threads'}, 'backing': None})
    rebase = run(['qemu-img', 'rebase', '-u', '-F', 'qcow2', '-b',
                  str(out / 'base-hdd.qcow2'), image], preexec_fn=child_limit)
    (out / 'rebase-result.json').write_text(json.dumps(rebase, indent=2) + '\n')
    assert rebase['returncode'] == 0, rebase
    chain = run(['qemu-img', 'info', '--backing-chain', '--output=json',
                 str(out / 'session-hdd.qcow2')], preexec_fn=child_limit)
    (out / 'backing-chain.json').write_text(json.dumps(chain, indent=2) + '\n')
    assert chain['returncode'] == 0, chain
    rows = json.loads(chain['stdout'])
    assert len(rows) == 2
    assert rows[0]['full-backing-filename'] == str(out / 'base-hdd.qcow2')
    assert all(Path(row['filename']).parent == out for row in rows)
    for name in ['config', 'data']:
        (out / name).mkdir()
    config = (SOURCE / 'xemu.toml').read_text().replace(str(SOURCE), str(out))
    (out / 'xemu.toml').write_text(config)
    compile_result = run(['gcc', '-shared', '-fPIC', '-g', '-I',
        str(ROOT / '.tools/renderdoc/usr/include'), str(ROOT / 'tools/xemu_gl_capture_control.c'),
        '-o', str(out / 'gl-capture-manual.so'), '-ldl', '-pthread'])
    (out / 'capture-control-build.json').write_text(json.dumps(compile_result, indent=2) + '\n')
    assert compile_result['returncode'] == 0
    display_args = ['Xvfb', '-displayfd', '1', '-screen', '0', '800x600x24', '-nolisten', 'tcp']
    display = subprocess.Popen(display_args, stdout=subprocess.PIPE,
        stderr=(out / 'xvfb.log').open('w'), start_new_session=True, text=True)
    display_number = display.stdout.readline().strip()
    assert display_number.isdigit()
    report.update({'display': ':' + display_number, 'xvfb_pid': display.pid,
        'xvfb_argv': display_args, 'qmp_port': free_port(), 'guest_gdb_port': free_port()})
    report['argv'] = [str(ROOT / '.tools/xemu-app/squashfs-root/usr/bin/xemu'),
        '-config_path', str(out / 'xemu.toml'), '-S', '-qmp',
        f"tcp:127.0.0.1:{report['qmp_port']},server=on,wait=off", '-gdb',
        f"tcp:127.0.0.1:{report['guest_gdb_port']}"]
    script = ['set pagination off', 'set confirm off', 'set debuginfod enabled off',
        'set print thread-events off', 'set startup-with-shell off',
        f'set logging file {out}/host-gdb.log', 'set logging overwrite off',
        'set logging enabled on', 'unset environment WAYLAND_DISPLAY',
        'unset environment VK_LAYER_PATH', 'unset environment VK_INSTANCE_LAYERS',
        'unset environment ENABLE_VULKAN_RENDERDOC_CAPTURE']
    env = {'DISPLAY': report['display'], 'SDL_VIDEODRIVER': 'x11',
        'SDL_AUDIODRIVER': 'dummy', 'LIBGL_ALWAYS_SOFTWARE': '1', 'LP_NUM_THREADS': '4',
        'XDG_CONFIG_HOME': str(out / 'config'), 'XDG_DATA_HOME': str(out / 'data'),
        'LD_PRELOAD': f'{ROOT}/.tools/renderdoc/usr/lib/librenderdoc.so:{out}/gl-capture-manual.so',
        'HALO2_RENDERDOC_LIBRARY': f'{ROOT}/.tools/renderdoc/usr/lib/librenderdoc.so',
        'HALO2_GL_CAPTURE_REQUEST': str(out / 'capture-request'),
        'HALO2_CAPTURE_PREFIX': str(out / 'scene-copy')}
    script += [f'set environment {key}={value}' for key, value in env.items()]
    (out / 'host-start.gdb').write_text('\n'.join(script + ['run']) + '\n')
    report['pinned_inputs'] = {str(path): digest(path) for path in [
        ROOT / 'tools/xemu_gl_capture_control.c',
        ROOT / '.tools/xemu-app/squashfs-root/usr/bin/xemu',
        ROOT / '.tools/renderdoc/usr/lib/librenderdoc.so']}
    (out / 'preparation.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({'directory': str(out), 'argv': report['argv'],
                      'xvfb_pid': display.pid, 'display': report['display']}))

if __name__ == '__main__':
    main()

#!/usr/bin/env python3
"""Run the real launcher from a read-only signed app with isolated data.

Setup, UI and engine children are explicit argument/storage recorders. The
Python shim runs the test host interpreter; it is not a packaged Python SDK.
"""
import hashlib
import json
import os
from pathlib import Path
import plistlib
import shutil
import stat
import subprocess
import sys
import tempfile

REQUIRED = ('halo2/h2_maps_win64_dx11/mainmenu.map', 'halo2/h2_maps_win64_dx11/shared.map',
    'halo2/h2_maps_win64_dx11/single_player_shared.map', 'halo2/h2_maps_win64_dx11/lockout.map',
    'halo2/h2_maps_win64_dx11/textures.dat', 'halo2/h2_maps_win64_dx11/sounds_neutral.dat',
    'halo2/h2_fonts/font_table.txt', 'bitmap-resources.json', 'tag-conversion.json',
    'loading/mcc-loading-assets.json')


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def hashes(app):
    return {str(p.relative_to(app)): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in app.rglob('*') if p.is_file()}


def writable(app):
    for path in (app, *app.rglob('*')):
        path.chmod(path.stat().st_mode | stat.S_IWUSR)


def make_app(executable, runtime_recorder, repo, license_path, app, arches):
    macos, resources = app / 'Contents/MacOS', app / 'Contents/Resources'
    macos.mkdir(parents=True)
    tools = resources / 'tools'
    tools.mkdir(parents=True)
    shutil.copy2(executable, macos / 'Halo2-Launcher')
    (app / 'Contents/Info.plist').write_bytes(plistlib.dumps(dict(CFBundleExecutable='Halo2-Launcher',
        CFBundleIdentifier='net.halo2forever.storage-test', CFBundlePackageType='APPL')))
    shutil.copy2(repo / 'tools/installation_paths.py', tools / 'installation_paths.py')
    (resources / 'resources').mkdir()
    (resources / 'resources/static-fixture.txt').write_text('read-only resource owner')
    python = resources / 'python/bin/python3'
    python.parent.mkdir(parents=True)
    python.write_text('#!' + sys.executable + '\nimport os, sys\nos.execv(' + repr(sys.executable) +
        ', [' + repr(sys.executable) + ', *sys.argv[1:]])\n')
    python.chmod(0o755)
    (tools / 'setup_assets.py').write_text('''import json, pathlib, sys
from installation_paths import resource_root
root=pathlib.Path(sys.argv[sys.argv.index('--root')+1]); resources=resource_root(root)
assert (resources/'resources/static-fixture.txt').read_text()=='read-only resource owner'
root.mkdir(parents=True,exist_ok=True)
(root/'setup-call.json').write_text(json.dumps(dict(argv=sys.argv[1:],root=str(root),resources=str(resources))))
cache=root/'assets/imported/mcc-fixture'
for name in ''' + repr(REQUIRED) + ''':
 p=cache/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_text('synthetic routing fixture')
(cache/'provenance.json').write_text(json.dumps(dict(schema='H2_MCC_TU15_PREPARATION_V1',
 external_installation_requirement='Halo MCC only',runtime_active=False,full_tu15_parity=False)))
(root/'assets/mcc-source.tsv').write_text('H2_MCC_ASSET_SOURCE_V1\\ncache\\tassets/imported/mcc-fixture\\n')
''')
    (tools / 'native_gameplay_test.py').write_text('raise SystemExit(1) # No synthetic gameplay/frontend admission.\n')
    # Admission is a controlled child status in this storage/argv fixture.
    # Real Vulkan package bytes and rejection cases have their own tests.
    # It must also remain read-only while the application is sealed.
    (tools / 'native_renderer_shaders.py').write_text('''import pathlib,sys
resources=pathlib.Path(__file__).resolve().parents[1]
assert '--verify-aot-package' in sys.argv and '--quiet' in sys.argv
assert pathlib.Path(sys.argv[sys.argv.index('--directory')+1]) == resources/'shaders'
assert pathlib.Path(sys.argv[sys.argv.index('--executable')+1]) == resources.parent/'MacOS/halo2_native'
raise SystemExit(0)
''')
    (tools / 'launcher_ui.py').write_text('''import json,pathlib,sys
root=pathlib.Path(sys.argv[sys.argv.index('--root')+1])
(root/'home-call.json').write_text(json.dumps(sys.argv[1:]))
raise SystemExit(20)
''')
    (tools / 'repair_mcc_shaders.py').write_text('''import json,pathlib,sys
root=pathlib.Path(sys.argv[sys.argv.index('--root')+1])
(root/'repair-call.json').write_text(json.dumps(sys.argv[1:]))
''')
    (tools / 'build_runtime.py').write_text("raise RuntimeError('Packaged launchers must not build source')\n")
    engine = macos / 'halo2_native'
    shutil.copy2(runtime_recorder, engine)
    subprocess.run(['codesign', '--force', '--sign', '-', str(engine)], check=True)
    import re
    source = (repo / 'src/app/launcher.cpp').read_text()
    block = re.search(r'shader_names\{(.*?)\};', source, re.S).group(1)
    (resources / 'shaders').mkdir()
    for name in re.findall(r'"([^"\n]+\.spv)"', block):
        (resources / 'shaders' / name).write_bytes(b'synthetic shader presence fixture')
    subprocess.run([sys.executable, '-B', str(repo / 'tools/macos/bundle.py'), '--app', str(app),
        *[arg for arch in arches for arg in ('--arch', arch)], '--license', 'SDL=' + str(license_path)], check=True)
    for path in (app, *app.rglob('*')):
        path.chmod(path.stat().st_mode & ~0o222)


def main():
    executable, runtime_recorder, repo, license_path = (Path(x).resolve(strict=True) for x in sys.argv[1:5])
    report_path, arches = Path(sys.argv[5]), sys.argv[6:]
    report = dict(schema='H2_MACOS_LAUNCHER_STORAGE_V1', runs=[],
        child_runtime='synthetic setup/UI/engine recorders; Python uses test host interpreter',
        retained_large_artifacts=[])
    apps = []
    try:
        with tempfile.TemporaryDirectory(prefix='macos-launcher-storage-', dir=os.environ['HALO2_TEST_SCRATCH']) as directory:
            work = Path(directory)
            try:
                for arch in arches:
                    row = dict(arch=arch, checks=[])
                    report['runs'].append(row)
                    app = work / ('Relocated Δ ' + arch) / 'Halo 2 Launcher.app'
                    make_app(executable, runtime_recorder, repo, license_path, app, arches)
                    apps.append(app)
                    data = work / ('User data Ω ' + arch)
                    launcher = app / 'Contents/MacOS/Halo2-Launcher'
                    resources = app / 'Contents/Resources'
                    original = hashes(app)
                    environment = dict(os.environ, H2_USER_DATA_DIR=str(data),
                        SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy')
                    environment.pop('H2_INSTALLATION_LAYOUT', None)
                    environment.pop('PYTHONDONTWRITEBYTECODE', None)
                    def run(*args, env=environment):
                        result = subprocess.run(['/usr/bin/arch', '-' + arch, str(launcher), *args],
                            cwd=repo, env=env, capture_output=True, text=True, timeout=20)
                        row.setdefault('commands', []).append(dict(argv=list(args), exit=result.returncode,
                            stdout=result.stdout[-4096:], stderr=result.stderr[-4096:]))
                        return result
                    info = run('--launcher-info')
                    require(info.returncode == 0, info.stderr)
                    require('Project: ' + str(data) + '\n' in info.stdout, 'Packaged launcher selected its resource directory as mutable storage')
                    require(not data.exists(), 'Read-only launcher info created user storage')
                    row['checks'].append('info selects separate data without creating it')
                    system_environment = dict(environment)
                    system_environment.pop('H2_USER_DATA_DIR')
                    system_info = run('--launcher-info', env=system_environment)
                    require(system_info.returncode == 0, system_info.stderr)
                    system_root = next(line[9:] for line in system_info.stdout.splitlines() if line.startswith('Project: '))
                    require(Path(system_root).is_absolute() and Path(system_root).name == 'net.halo2forever', 'Foundation did not select the application support owner')
                    row['default_application_support'] = system_root
                    row['checks'].append('Foundation resolves default user-domain Application Support')
                    source = work / ('MCC source ' + arch)
                    source.mkdir(); (source / 'original.txt').write_text('original input retained')
                    setup = run('--setup', '--mcc-source', str(source), '--setup-only')
                    require(setup.returncode == 0, setup.stderr)
                    call = json.loads((data / 'setup-call.json').read_text())
                    require(call['resources'] == str(resources) and call['root'] == str(data), 'Setup lost one of its distinct owners')
                    require(call['argv'][call['argv'].index('--mcc-root') + 1] == str(source), 'Manual MCC source selection was lost')
                    require((source / 'original.txt').read_text() == 'original input retained', 'Original source was changed')
                    row['checks'].append('manual setup reads app resources and writes only user data')
                    home = run()
                    require(home.returncode == 0, home.stderr)
                    home_args = json.loads((data / 'home-call.json').read_text())
                    require(home_args[home_args.index('--launcher-executable') + 1] == str(launcher), 'Home relaunch escaped the app')
                    row['checks'].append('home receives the actual packaged launcher')
                    preview = run('--preview-mcc', '--hidden', '--frames', '1')
                    require(preview.returncode == 0, preview.stderr)
                    runtime = json.loads((data / 'runtime-call.json').read_text())
                    require(runtime['cwd'] == str(data) and runtime['launcher'] == str(launcher), 'Runtime lost writable storage or relaunch owner')
                    require('--mcc-preview' in runtime['argv'], 'Preview changed into a gameplay claim')
                    require(str(data / 'assets/imported/mcc-fixture') in ' '.join(runtime['argv']), 'Runtime maps came from outside user storage')
                    row['checks'].append('runtime uses separate data cwd, imported maps and app relaunch')
                    blocked = dict(environment, H2_USER_DATA_DIR=str(resources / 'should-not-exist'))
                    bad = run('--launcher-info', env=blocked)
                    require(bad.returncode != 0 and 'separate from the application bundle' in bad.stderr, 'Bundle-local data override was accepted')
                    require(not (resources / 'should-not-exist').exists(), 'Rejected override wrote to the app')
                    row['checks'].append('bundle-local data is rejected before writes')
                    require(hashes(app) == original, 'The signed read-only application changed')
                    subprocess.run(['codesign', '--verify', '--deep', '--strict', str(app)], check=True)
                    row['checks'].append('resource hashes and complete signature remain unchanged')
                    # Installed apps can be writable by their owner. Resource
                    # integrity must not depend on an EROFS error hiding pyc
                    # writes from the interpreter.
                    writable(app)
                    repeated = run('--setup', '--mcc-source', str(source), '--setup-only')
                    require(repeated.returncode == 0, repeated.stderr)
                    after = hashes(app)
                    row['changed_resources'] = sorted(name for name in set(after) | set(original)
                        if after.get(name) != original.get(name))
                    require(not row['changed_resources'], 'Python wrote bytecode or other files into the writable signed app')
                    subprocess.run(['codesign', '--verify', '--deep', '--strict', str(app)], check=True)
                    row['checks'].append('writable app also retains its exact sealed resources')
                    row['passed'] = True
                report['passed'] = True
            finally:
                for app in apps:
                    writable(app)
    finally:
        report_path.write_text(json.dumps(report, indent=2) + '\n')


if __name__ == '__main__':
    main()

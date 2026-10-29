#!/usr/bin/env python3
"""Exercise the complete native Windows MCC import in run_test.py scratch.

This is an opt-in, large-input check. It reads the installed MCC source and
copies all generated data into HALO2_TEST_SCRATCH; the enclosing supervisor
removes it on success, failure and cancellation. Only bounded reports remain.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import select
import shutil
import subprocess
import time
import traceback


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def windows_path(path):
    return 'Z:' + str(Path(path).resolve()).replace('/', chr(92))


def source_snapshot(source):
    rows = []
    for base, directories, names in os.walk(source):
        directories.sort()
        for name in sorted(names):
            path = Path(base) / name
            info = path.lstat()
            rows.append((path.relative_to(source).as_posix(), info.st_dev,
                         info.st_ino, info.st_size, info.st_mtime_ns))
    return rows


def freeze_package(package, destination, expected=None):
    manifest = package / 'windows-package.json'
    raw = manifest.read_bytes()
    pin = hashlib.sha256(raw).hexdigest()
    if expected is not None and pin != expected:
        raise ValueError('Requested package manifest changed')
    records = json.loads(raw)['files']
    destination.mkdir()
    copies, logical_bytes, unique_bytes = {}, 0, 0
    for name, row in records.items():
        relative = PurePosixPath(name)
        if relative.is_absolute() or '..' in relative.parts:
            raise ValueError('Invalid managed package member')
        source, target = package / relative, destination / relative
        if source.is_symlink() or not source.is_file():
            raise ValueError('Package input is not a direct file: ' + name)
        target.parent.mkdir(parents=True, exist_ok=True)
        key = (row['sha256'], row['bytes'])
        logical_bytes += row['bytes']
        if key in copies:
            # Links are confined to this owned copy. Original resource inode
            # link counts/change times remain untouched.
            os.link(copies[key], target)
        else:
            shutil.copy2(source, target)
            if target.stat().st_size != row['bytes'] or digest(target) != row['sha256']:
                raise ValueError('Package input changed: ' + name)
            copies[key] = target
            unique_bytes += row['bytes']
    if manifest.read_bytes() != raw:
        raise ValueError('Package changed while freezing its resources')
    (destination / manifest.name).write_bytes(raw)
    return dict(manifest_sha256=pin, files=len(records), logical_bytes=logical_bytes,
                unique_payload_bytes=unique_bytes)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--package', type=Path, required=True)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--wait-for-package', action='store_true')
    parser.add_argument('--package-sha256', help='Require this exact managed package manifest')
    parser.add_argument('--verify-reuse', action='store_true',
                        help='Repeat setup to verify all cached/source files and reuse publication')
    parser.add_argument('--timeout', type=int, default=7200)
    args = parser.parse_args()
    owner = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    package, source = args.package.resolve(strict=True), args.source.resolve(strict=True)
    data, prefix, jobs = owner / 'port', owner / 'wine', owner / 'jobs'
    data.mkdir(); jobs.mkdir()
    report = dict(passed=False, status='booting', scratch=str(owner),
                  wine_prefix=str(prefix), data=str(data), source=str(source),
                  jobs=str(jobs), milestones=[], retained_large_artifacts=[])
    args.report.parent.mkdir(parents=True, exist_ok=True)

    def save():
        args.report.write_text(json.dumps(report, indent=2) + '\n')

    base = os.environ.copy()
    base.update(WINEPREFIX=str(prefix), WINEARCH='win64', WINEDEBUG='-all',
                WINEDLLOVERRIDES='winemenubuilder.exe=d;winewayland.drv=d',
                DISPLAY='', WAYLAND_DISPLAY='',
                PYTHONDONTWRITEBYTECODE='1', PYTHONIOENCODING='utf-8', PYTHONUNBUFFERED='1')

    def environment(resources):
        result = base.copy()
        result.update(PYTHONHOME=windows_path(resources / 'python'),
                      TCL_LIBRARY=windows_path(resources / 'python/lib/tcl8.6'),
                      TK_LIBRARY=windows_path(resources / 'python/lib/tk8.6'),
                      H2_INSTALLATION_LAYOUT=json.dumps(dict(schema='H2_INSTALLATION_LAYOUT_V1',
                          data=windows_path(data), resources=windows_path(resources))),
                      TMP=windows_path(owner), TEMP=windows_path(owner),
                      HALO2_TEST_SCRATCH=windows_path(owner))
        return result

    child = None
    save()
    print(json.dumps(dict(status=report['status'], scratch=str(owner), jobs=str(jobs))), flush=True)
    try:
        started = time.monotonic()
        boot = subprocess.run(['wineboot', '--init'], env=base | {'DISPLAY':'', 'WAYLAND_DISPLAY':''},
                              stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=300)
        report['wine_boot'] = dict(exit_code=boot.returncode, seconds=time.monotonic() - started)
        if boot.returncode:
            raise RuntimeError('Owned Wine prefix initialization failed')
        # Keep the initialized services warm for the immediately following
        # preflight. Waiting for server shutdown here creates another cold boot.
        preflight = owner / 'preflight.py'
        preflight.write_text("""import json, sys
from pathlib import Path
root, resources, source = map(Path, sys.argv[1:])
sys.path.insert(0, str(resources / 'tools'))
from installation_paths import resource_root
from mcc_loading_presentation import preflight_v3
assert resource_root(root) == resources.resolve()
value = preflight_v3(root, source)
print(json.dumps(dict(passed=True, resources=str(resource_root(root)), profile=value['profile'],
                     converter=value['converter'], codebook=value['codebook'])))
""", encoding='utf-8')
        command = ['wine', str(package / 'python/python.exe'), '-u', '-B', windows_path(preflight),
                   windows_path(data), windows_path(package), windows_path(source)]
        started = time.monotonic()
        result = subprocess.run(command, env=environment(package), cwd=data, stdin=subprocess.DEVNULL,
                                capture_output=True, text=True, errors='replace', timeout=360)
        report['preflight'] = dict(command=command, exit_code=result.returncode,
                                  seconds=time.monotonic() - started,
                                  stdout=result.stdout[-16384:], stderr=result.stderr[-16384:])
        if result.returncode:
            raise RuntimeError('Native Windows resource binding or V3 preflight failed')
        report['status'] = 'ready_for_fixed_package'; save()
        print(json.dumps(dict(ready=True, scratch=str(owner), jobs=str(jobs))), flush=True)
        expected = args.package_sha256
        if args.wait_for_package:
            deadline = time.monotonic() + 3600
            while not (jobs / 'start.json').is_file():
                if (jobs / 'quit.json').is_file():
                    report['status'] = 'closed_without_import'; save()
                    return 0
                if time.monotonic() >= deadline:
                    raise TimeoutError('Fixed package readiness wait expired')
                time.sleep(1)
            expected = json.loads((jobs / 'start.json').read_text()).get('package_manifest_sha256')
        resources = owner / 'resources'
        report['status'] = 'freezing_package'; save()
        report['package_snapshot'] = freeze_package(package, resources, expected)
        executable = resources / 'halo2_mcc_loading_audio.exe'
        report['loading_converter'] = dict(bytes=executable.stat().st_size, sha256=digest(executable))
        helpers = ['tools/prepare_mcc_gameplay.py', 'tools/audit_rendering.py', 'tools/mcc_content.py',
                   'tools/setup_assets.py', 'tools/mcc_onboarding.py']
        report['helper_sha256'] = {name:digest(resources / name) for name in helpers}
        original = source_snapshot(source)
        report['original_source_metadata_sha256'] = hashlib.sha256(json.dumps(original).encode()).hexdigest()
        report['original_source_files'] = len(original)
        report['original_game_cfg_sha256'] = digest(source / 'game.cfg')
        command = ['wine', str(resources / 'python/python.exe'), '-u', '-B',
                   windows_path(resources / 'tools/setup_assets.py'), '--root', windows_path(data),
                   '--mcc-root', windows_path(source)]
        report.update(command=command, resources=str(resources), status='importing'); save()
        print(json.dumps(dict(import_started=True, package=report['package_snapshot'], scratch=str(owner))), flush=True)
        log = args.report.with_suffix('.log')
        started, last_save, last_progress = time.monotonic(), 0, ''
        pending, tail = b'', b''
        with log.open('w+b') as stream:
            child = subprocess.Popen(command, env=environment(resources), cwd=data,
                                     stdin=subprocess.DEVNULL, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            report['native_pid'] = child.pid; save()
            while True:
                if time.monotonic() - started > args.timeout:
                    child.terminate()
                    raise TimeoutError('Native Windows import deadline expired')
                readable = select.select([child.stdout], [], [], 1)[0]
                if readable:
                    chunk = os.read(child.stdout.fileno(), 65536)
                    if chunk:
                        tail = (tail + chunk)[-65536:]
                        stream.write(chunk); stream.flush(); pending += chunk
                        while b'\n' in pending:
                            line, pending = pending.split(b'\n', 1)
                            text = line.decode('utf-8', 'replace').strip()
                            if text:
                                last_progress = text[:1000]
                                if len(report['milestones']) < 2000:
                                    report['milestones'].append(dict(seconds=time.monotonic()-started, line=last_progress))
                                print(text, flush=True)
                        if stream.tell() > 8 * 1024**2:
                            stream.seek(-4 * 1024**2, os.SEEK_END)
                            keep = stream.read(); stream.seek(0); stream.truncate(); stream.write(keep)
                    elif child.poll() is not None:
                        break
                if time.monotonic() - last_save > 10:
                    report.update(last_progress=last_progress, seconds=time.monotonic()-started); save()
                    last_save = time.monotonic()
                if child.poll() is not None and not readable:
                    break
            code = child.wait(timeout=30)
            child.stdout.close(); child = None
        report.update(exit_code=code, seconds=time.monotonic()-started, last_progress=last_progress,
                      stdout_tail=tail.decode('utf-8', 'replace'),
                      original_source_unchanged=original == source_snapshot(source),
                      original_game_cfg_unchanged=report['original_game_cfg_sha256'] == digest(source/'game.cfg'))
        if code == 0:
            registry = data / 'assets/mcc-source.tsv'
            lines = registry.read_text().splitlines()
            selected = lines[1].split('\t', 1)[1]
            cache = (Path(selected[2:].replace(chr(92), '/')) if selected.startswith('Z:') else
                     data / selected.replace(chr(92), '/'))
            provenance = cache / 'provenance.json'; value = json.loads(provenance.read_text())
            report['published'] = dict(cache=str(cache), provenance_sha256=digest(provenance),
                                       schema=value.get('schema'), source_files=len(value.get('source_files', {})),
                                       summary=value.get('summary'), runtime_active=value.get('runtime_active'),
                                       full_tu15_parity=value.get('full_tu15_parity'))
            loading = cache / 'presentation/loading-v3/manifest.json'
            if loading.is_file():
                report['published']['loading_manifest'] = json.loads(loading.read_text())
                report['published']['loading_manifest_sha256'] = digest(loading)
            report['map_preflight_count'] = sum(row['line'].startswith('Checking aggregate MCC bitmap budget:')
                                               for row in report['milestones'])
            report['map_conversion_count'] = sum(row['line'].startswith('Reading MCC Classic tags, textures and bump resources:')
                                                for row in report['milestones'])
            if args.verify_reuse:
                report['status'] = 'verifying_cache_reuse'; save()
                print('Verifying the completed cache and source through a second native Windows setup run…', flush=True)
                registry_before, provenance_before = registry.read_bytes(), digest(provenance)
                caches_before = sorted(path.name for path in (data/'assets/imported').iterdir())
                reuse_log = args.report.with_suffix('.reuse.log')
                started_reuse = time.monotonic()
                with reuse_log.open('wb') as stream:
                    result = subprocess.run(command, env=environment(resources), cwd=data,
                                            stdin=subprocess.DEVNULL, stdout=stream, stderr=subprocess.STDOUT,
                                            timeout=args.timeout)
                output = reuse_log.read_bytes()
                report['reuse'] = dict(exit_code=result.returncode, seconds=time.monotonic()-started_reuse,
                                       stdout_tail=output[-32768:].decode('utf-8', 'replace'),
                                       registry_unchanged=registry.read_bytes() == registry_before,
                                       provenance_unchanged=digest(provenance) == provenance_before,
                                       cache_directories_unchanged=caches_before == sorted(
                                           path.name for path in (data/'assets/imported').iterdir()),
                                       verified_cache='Checking the existing MCC cache (' in output.decode('utf-8', 'replace'),
                                       recopied_assets=b'Copying ' in output)
                reuse = report['reuse']
                if (reuse['exit_code'] or not reuse['registry_unchanged'] or
                        not reuse['cache_directories_unchanged'] or not reuse['verified_cache'] or reuse['recopied_assets']):
                    raise RuntimeError('Second native Windows setup failed to verify and reuse the completed cache')
                report['original_source_unchanged'] = original == source_snapshot(source)
                report['original_game_cfg_unchanged'] = report['original_game_cfg_sha256'] == digest(source/'game.cfg')
        report['passed'] = code == 0 and report['original_source_unchanged'] and report['original_game_cfg_unchanged']
        report['status'] = 'passed' if report['passed'] else 'failed'
    except BaseException as error:
        report.update(status='failed', passed=False, error=repr(error), traceback=traceback.format_exc()[-12000:])
        print(json.dumps(dict(error=repr(error), report=str(args.report))), flush=True)
    finally:
        if child is not None:
            try:
                child.terminate(); child.wait(timeout=15)
            except (OSError, subprocess.TimeoutExpired):
                pass
        for command in (['wineserver', '-k'], ['wineserver', '-w']):
            try:
                subprocess.run(command, env=base, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=30)
            except (OSError, subprocess.TimeoutExpired) as error:
                report.setdefault('cleanup_errors', []).append(str(error))
        total = files = 0
        for base_dir, directories, names in os.walk(owner):
            for name in names:
                try:
                    total += (Path(base_dir) / name).lstat().st_size; files += 1
                except OSError:
                    pass
        report['disposable_outputs'] = dict(logical_bytes_before_cleanup=total, files=files)
        save()
        print(json.dumps(dict(finished=True, passed=report['passed'], report=str(args.report), scratch=str(owner))), flush=True)
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())

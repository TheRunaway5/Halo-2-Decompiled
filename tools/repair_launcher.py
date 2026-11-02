#!/usr/bin/env python3
"""Verify an installation, repair its private MCC cache, and recheck startup."""
import argparse
import json
import os
from pathlib import Path, PurePosixPath
import tempfile

from launcher_health import inspect_launcher
from setup_progress import WorkProgress, write_progress

WINDOWS = os.name == 'nt'

def verify_windows_package(root, progress):
    from native_gameplay_test import _file_digest, _digest
    manifest = root/'windows-package.json'
    if manifest.is_symlink() or not manifest.is_file() or manifest.stat().st_size > 16*1024**2:
        raise ValueError('Windows package inventory is missing. Re-extract the Windows package and retry Repair.')
    pin = _file_digest(manifest)
    report = json.loads(manifest.read_bytes())
    files = report.get('files')
    if (report.get('schema') != 'H2_WINDOWS_NATIVE_PACKAGE_V1' or
            report.get('platform') != 'windows-x86_64' or not isinstance(files, dict) or
            not 1 <= len(files) <= 50000 or 'halo2_native.exe' not in files):
        raise ValueError('Windows package inventory is invalid. Re-extract the Windows package and retry Repair.')
    rows = []
    for name, row in files.items():
        path = PurePosixPath(name)
        if (not path.parts or path.is_absolute() or str(path) != name or '\\' in name or
                ':' in name or any(p in ('', '.', '..') for p in name.split('/')) or
                not isinstance(row, dict) or type(row.get('bytes')) is not int or row['bytes'] < 0):
            raise ValueError('Windows package inventory has an invalid file owner')
        _digest(row.get('sha256'))
        rows.append((root/path, name, row))
    counter = WorkProgress('repair-package', 'Verifying installed game files',
                           sum(row['bytes'] for _, _, row in rows), progress, unit='bytes')
    for path, name, row in rows:
        if (path.is_symlink() or path.resolve() != path or not path.is_file() or
                path.stat().st_size != row['bytes'] or _file_digest(path) != row['sha256']):
            raise ValueError('Installed package file is missing or changed: ' + name +
                             '. Re-extract the Windows package and retry Repair.')
        counter.advance(row['bytes'])
    if _file_digest(manifest) != pin:
        raise ValueError('Windows package inventory changed during Repair; retry after the other operation finishes')
    counter.finish()
    return pin


def repair(root, runtime=None, *, mcc_root=None, progress=print):
    import mcc_onboarding
    from native_gameplay_test import _file_digest

    root = Path(root).resolve(strict=True)
    runtime = Path(runtime).resolve() if runtime is not None else None
    windows = WINDOWS
    package_pin = verify_windows_package(root, progress) if windows else None
    progress('Checking the private MCC cache…')
    try:
        source = mcc_onboarding.selected(root)
        if source is None:
            raise ValueError('No prepared MCC cache is selected')
        cache, _ = mcc_onboarding.select(root, source[0], verify=True, progress=progress)
    except (OSError, ValueError, KeyError, TypeError) as error:
        progress('Rebuilding the private MCC cache from your MCC installation…')
        try:
            cache, _ = mcc_onboarding.setup(root, mcc_root, progress=progress)
        except (OSError, ValueError, KeyError, TypeError) as failure:
            raise ValueError('MCC cache repair failed: ' + str(failure) +
                             '. Choose your MCC folder and retry Repair.') from error
    health = inspect_launcher(root, runtime)
    if health:
        progress('Repair complete. Ready to play.')
        return
    if windows and health.category == 'validation':
        runtime = runtime or root/'halo2_native.exe'
        if runtime != root/'halo2_native.exe':
            raise ValueError('The selected runtime is outside this Windows package. Reinstall that runtime and retry Repair.')
        sidecar = runtime.with_name(runtime.name+'.frontend-test.json')
        if sidecar.exists() or sidecar.is_symlink():
            raise ValueError('The existing startup check is invalid. Re-extract the Windows package and retry Repair.')
        from qualify_frontend_runtime import qualify
        # Captures, native state, and logs have one owner and retire on every
        # outcome. Only bounded passed validation records remain permanently.
        with tempfile.TemporaryDirectory(prefix='forever-repair-') as temporary:
            scratch = Path(temporary)
            try:
                if _file_digest(root/'windows-package.json') != package_pin:
                    raise ValueError('Package inventory changed during Repair')
                include_movie = (cache/'presentation/movies').exists() or (root/'resources/movies').exists()
                qualify(root, runtime, cache, scratch/'reports', scratch, progress=progress,
                        include_optional_movie=include_movie)
                # Retain the bounded receipt, not native captures or state.
                diagnostics = root/'diagnostics'
                if diagnostics.is_symlink():
                    raise ValueError('Repair diagnostics directory has no direct owner')
                diagnostics.mkdir(exist_ok=True)
                receipt = (scratch/'reports/report.json').read_bytes()
                if len(receipt) > 1024**2:
                    raise ValueError('Startup check report exceeded its size budget')
                target = diagnostics/'frontend-startup-check.json'
                if target.is_symlink():
                    raise ValueError('Repair diagnostics file has no direct owner')
                with tempfile.NamedTemporaryFile(prefix='.frontend-check-', dir=diagnostics, delete=False) as stream:
                    pending = Path(stream.name)
                    try:
                        stream.write(receipt)
                    except BaseException:
                        pending.unlink(missing_ok=True)
                        raise
                try:
                    os.replace(pending, target)
                finally:
                    pending.unlink(missing_ok=True)
            except (OSError, ValueError, RuntimeError) as error:
                report_path = scratch/'reports/report.json'
                if report_path.is_file():
                    # The failed row gives the useful reason even when native
                    # startup logged success before failing to save/close.
                    progress('Startup check details: '+report_path.read_text()[-4000:])
                logs = sorted((scratch/'reports').glob('*.log'))
                detail = logs[-1].read_text(errors='replace')[-1400:] if logs else str(error)
                raise ValueError('Game startup check failed: ' + str(error) + '\n' + detail) from error
        health = inspect_launcher(root, runtime)
    if not health:
        raise ValueError(health.message + ' ' + health.detail +
                         ' Install a validated game package, then retry Repair.')
    progress('Repair complete. Ready to play.')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, required=True)
    parser.add_argument('--runtime', type=Path)
    parser.add_argument('--mcc-root', type=Path)
    parser.add_argument('--gui-owner', action='store_true', help=argparse.SUPPRESS)
    args = parser.parse_args()
    from mcc_movies import _cancellation
    from setup_assets import _gui_owner
    try:
        with _cancellation(), _gui_owner(args.gui_owner):
            repair(args.root, args.runtime, mcc_root=args.mcc_root,
                   progress=write_progress if args.gui_owner else print)
        return 0
    except (OSError, ValueError, KeyError, TypeError, RuntimeError) as error:
        print('Repair failed: ' + str(error), flush=True)
        return 1


if __name__ == '__main__':
    raise SystemExit(main())

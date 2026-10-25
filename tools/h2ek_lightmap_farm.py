#!/usr/bin/env python3
"""Bounded Classic H2 lightmap workers with private tags and Wine prefixes.

Call from an existing tools/run_test.py owner. Installed command descriptors
and raster partition/save paths are pinned in the Deserted lightmap audit.
Every worker can save the common structure-lightmap tag, so workers must never
share writable tags. Only worker 0's successfully merged final tags are copied
back to the caller's already-private kit. This module does not build a cache.
"""
from __future__ import annotations

import concurrent.futures
import contextlib
import hashlib
import json
import os
from pathlib import Path, PureWindowsPath
import shutil
import subprocess
import tempfile
import threading
import time


FARM_QUALITIES = frozenset(('checkerboard', 'draft_low', 'draft_medium',
                          'draft_high', 'draft_super', 'low', 'medium', 'high', 'super'))


def _sha(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


@contextlib.contextmanager
def _owned_worker_root(scratch, environments):
    with tempfile.TemporaryDirectory(prefix='h2ek-farm-', dir=scratch) as temporary:
        try:
            yield Path(temporary)
        finally:
            # Stop only the created prefixes while their paths still exist.
            for env in environments:
                subprocess.run(['wineserver', '-k'], env=env, stdout=subprocess.DEVNULL,
                               stderr=subprocess.DEVNULL, timeout=30)


def bake_lightmaps_farm(kit, environment, scenario, bsp, quality, workers, timeout,
                       evidence, *, accepted_error=None, progress=None, inspect_workers=None):
    """Bake/merge into a caller-owned kit, returning bounded command/tag evidence.

    kit and environment['WINEPREFIX'] must belong to HALO2_TEST_SCRATCH. Workers
    are created sequentially; at most two run concurrently. `timeout` bounds
    each compiler invocation. The enclosing owner must also bound total time.
    accepted_error(line), if supplied, admits only explicitly known stock tag
    diagnostics. Every other compiler/merge error fails before publication.
    progress(event) is optional and must be thread-safe. Evidence keeps bounded
    logs/JSON only; all copied tags, prefixes and intermediates are disposable.
    inspect_workers(kits) may read the completed independent kits before merge
    and return a bounded diagnostic receipt. It must not mutate their inputs.
    """
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    kit, evidence = Path(kit).resolve(strict=True), Path(evidence).resolve()
    prefix = Path(environment['WINEPREFIX']).resolve(strict=True)
    assert kit.is_relative_to(scratch) and prefix.is_relative_to(scratch)
    assert quality in FARM_QUALITIES, 'Classic farm workers do not accept direct_only'
    assert isinstance(workers, int) and workers >= 2 and timeout > 0
    logical = PureWindowsPath(scenario)
    assert not logical.is_absolute() and not logical.drive and '..' not in logical.parts
    assert logical.suffix == '' and bsp and '/' not in bsp and '\\' not in bsp
    relative = Path(*logical.parts)
    tag_directory = kit/'tags'/relative.parent
    assert tag_directory.resolve(strict=True).is_relative_to(kit), 'Final map tags must be private'
    stem = relative.name+'_'+bsp+'_lightmap'
    required = (relative.name+'.scenario', bsp+'.scenario_structure_bsp',
                stem+'.scenario_structure_lightmap', stem+'_truecolor_bitmaps.bitmap')
    allowed = set(required) | {stem+'_bitmaps.bitmap'}
    original = {name: _sha(tag_directory/name) for name in allowed if (tag_directory/name).exists()}
    evidence.mkdir(parents=True, exist_ok=False)
    state = dict(schema='H2EK_PRIVATE_LIGHTMAP_FARM_V1', scenario=str(logical), bsp=bsp,
                 quality=quality, workers=workers, concurrent_workers=min(2, workers),
                 per_command_timeout=timeout, scratch=str(scratch), passed=False,
                 commands=[], worker_bitmaps=[], input_final_tags=original,
                 retained_large_artifacts=[])
    lock = threading.RLock()
    def save():
        with lock:
            temporary = evidence/'farm.json.tmp'
            temporary.write_text(json.dumps(state, indent=2)+'\n')
            temporary.replace(evidence/'farm.json')
    def event(name):
        if progress:
            progress(name)
    environments = []
    def compiler(name, working, env, args, temporary):
        command = ['wine', 'tool_fast.exe', '-batch', *args]
        log = temporary/(name+'.log')
        started = time.monotonic()
        priority_restorations = []
        restored = set()
        expected_executable = str(working/'tool_fast.exe').replace('/', '\\').encode()
        caller_priority = os.getpriority(os.PRIO_PROCESS, 0)
        try:
            with log.open('w') as output:
                process = subprocess.Popen(command, cwd=working, env=env,
                                           stdout=output, stderr=subprocess.STDOUT)
                while process.poll() is None:
                    if time.monotonic()-started > timeout:
                        process.kill()
                        process.wait()
                        raise TimeoutError(name+' exceeded its declared timeout')
                    # Wine can reset the original tool main-thread priority.
                    # Restore only an exact private executable, never a global
                    # wine/tool name or an unrelated compiler process.
                    for candidate in Path('/proc').iterdir():
                        if not candidate.name.isdecimal() or int(candidate.name) in restored:
                            continue
                        try:
                            argv = (candidate/'cmdline').read_bytes()
                            if expected_executable in argv and b'-batch' in argv:
                                pid = int(candidate.name)
                                before = os.getpriority(os.PRIO_PROCESS, pid)
                                os.setpriority(os.PRIO_PROCESS, pid, caller_priority)
                                restored.add(pid)
                                priority_restorations.append(dict(pid=pid, previous_nice=before,
                                                                  restored_nice=caller_priority))
                        except (FileNotFoundError, ProcessLookupError, PermissionError):
                            pass
                    time.sleep(.25)
        finally:
            text = log.read_text(errors='replace').replace('\r', '\n') if log.exists() else ''
            # Raw progress can contain megabytes of carriage-return redraws.
            lines = [line for line in text.splitlines() if line.strip() and '%' not in line]
            (evidence/(name+'.log')).write_text(('\n'.join(lines)+'\n')[-65536:])
        errors = [line for line in text.splitlines() if '### ERROR' in line]
        unexpected = [line for line in errors if accepted_error is None or not accepted_error(line)]
        # Dimension/count/load errors in merge are not all prefixed ### ERROR.
        merge_errors = [line for line in text.splitlines()
                        if 'missing radiance bitmaps' in line or
                        ('lightmap worker #' in line and ('!=' in line or 'failed to load' in line))]
        row = dict(name=name, command=command, cwd=str(working), exit_code=process.returncode,
                   elapsed_seconds=round(time.monotonic()-started, 3),
                   known_diagnostics=errors, unexpected_errors=unexpected+merge_errors,
                   owned_priority_restorations=priority_restorations)
        with lock:
            state['commands'].append(row)
            save()
        assert process.returncode == 0 and not unexpected and not merge_errors, row
        if name == 'merge':
            assert 'lightmap merge succeeded, palettized and truecolor bitmaps outputted' in text
        event(name+' completed')
    try:
        save()
        with _owned_worker_root(scratch, environments) as temporary:
            state['private_worker_root'] = str(temporary)
            kits = []
            registry = {p.name: _sha(p) for p in prefix.glob('*.reg')}
            assert {'system.reg', 'user.reg', 'userdef.reg'} <= registry.keys()
            for index in range(workers):
                working = temporary/f'worker-{index}'
                working.mkdir()
                (working/'reports').mkdir()
                for name in ('tags', 'data'):
                    # Dereference the staged kit's stock overlays into private
                    # CoW copies. No writable worker tag points at installation.
                    subprocess.run(['cp', '-aL', '--reflink=auto', '--', str(kit/name),
                                    str(working/name)], check=True, timeout=timeout)
                for name in ('tool_fast.exe', 'libfbxsdk.dll'):
                    shutil.copy2(kit/name, working/name)
                private_prefix = temporary/f'prefix-{index}'
                subprocess.run(['cp', '-a', '--reflink=auto', '--', str(prefix), str(private_prefix)],
                               check=True, timeout=timeout)
                assert registry == {p.name: _sha(p) for p in prefix.glob('*.reg')}, 'Prefix changed during snapshot'
                env = dict(environment, WINEPREFIX=str(private_prefix))
                environments.append(env)
                kits.append(working)
                for name in allowed:
                    source, copy = tag_directory/name, working/'tags'/relative.parent/name
                    if source.exists():
                        assert source.stat().st_ino != copy.stat().st_ino and _sha(copy) == original[name]
            event('Independent worker kits and prefixes prepared')
            save()
            with concurrent.futures.ThreadPoolExecutor(max_workers=2) as pool:
                tasks = [pool.submit(compiler, f'worker-{i}', kits[i], environments[i],
                                     ['lightmaps-farm-worker', str(logical), bsp, quality, str(i), str(workers)], temporary)
                         for i in range(workers)]
                try:
                    for task in concurrent.futures.as_completed(tasks):
                        task.result()
                except BaseException:
                    # Executor.__exit__ waits for running futures. Stop only
                    # these owned Wine prefixes before entering that wait.
                    for task in tasks:
                        task.cancel()
                    for env in environments:
                        subprocess.run(['wineserver', '-k'], env=env, stdout=subprocess.DEVNULL,
                                       stderr=subprocess.DEVNULL, timeout=30)
                    raise
            if inspect_workers:
                state['worker_common_inspection'] = inspect_workers(tuple(kits))
                assert len(json.dumps(state['worker_common_inspection'])) <= 1024*1024
                save()
            merged = kits[0]/'tags'/relative.parent
            for index in range(workers):
                name = stem+f'_radiance_bitmaps_{index}.bitmap'
                source = kits[index]/'tags'/relative.parent/name
                assert source.is_file() and source.stat().st_size > 64, 'Missing completed worker bitmap'
                state['worker_bitmaps'].append(dict(worker=index, filename=name,
                                                     bytes=source.stat().st_size, sha256=_sha(source)))
                if index:
                    shutil.copy2(source, merged/name)
            save()
            compiler('merge', kits[0], environments[0],
                     ['lightmaps-farm-merge', str(logical), bsp, str(workers)], temporary)
            assert all((merged/name).is_file() for name in required), 'Incomplete merged final tag set'
            # Validate every source before publishing any final output.
            snapshots = [dict(filename=name, bytes=(merged/name).stat().st_size,
                              sha256=_sha(merged/name)) for name in sorted(allowed) if (merged/name).is_file()]
            assert all(row['bytes'] > 64 for row in snapshots)
            for row in snapshots:
                source, destination = merged/row['filename'], tag_directory/row['filename']
                # Atomic replacement also breaks any pre-existing hardlink.
                with tempfile.NamedTemporaryFile(dir=tag_directory, prefix='.farm-', delete=False) as file:
                    staged = Path(file.name)
                try:
                    shutil.copy2(source, staged)
                    assert _sha(staged) == row['sha256']
                    staged.replace(destination)
                finally:
                    staged.unlink(missing_ok=True)
            assert all(_sha(tag_directory/row['filename']) == row['sha256'] for row in snapshots)
            state['merged_final_tags'] = snapshots
            state['passed'] = True
            save()
            event('Verified merged tags published into private caller kit')
    except BaseException as error:
        state['error'] = repr(error)
        save()
        raise
    finally:
        state['private_workers_removed'] = not Path(state.get('private_worker_root', scratch/'absent')).exists()
        save()
    return state

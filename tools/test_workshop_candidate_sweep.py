#!/usr/bin/env python3
"""Sweep an unpublished frozen engine without changing launcher selections.

Qualified prepared candidates use their exact packaged executable and shaders.
For a failed launcher, the unchanged compiled shader owners are restaged in
HALO2_TEST_SCRATCH and a temporary fallback link is removed on every exit.
Qualification failures remain failures; bounded mod checks cannot publish or
qualify a candidate or accept complete mod support.
Run inside the active build owner's run_test.py command session.
"""
import argparse
from contextlib import contextmanager, nullcontext
from functools import lru_cache
import hashlib
import json
import os
from pathlib import Path
import signal
import subprocess
import sys


def qualified_base_cache(root,package,qualification):
    """Use the same sealed candidate/cache admission as the normal launcher."""
    from gameplay_runtime_selection import selected_gameplay_runtime
    selection=package/'selection.json'
    declared=json.loads(selection.read_text())
    if (declared.get('runtime')!=qualification['runtime'] or
        declared.get('runtime_sha256')!=qualification['runtime_sha256']):
        raise ValueError('Sweep candidate selection differs from its qualification')
    record=selected_gameplay_runtime(root,selection_name=selection.relative_to(root).as_posix())
    if record!=declared:
        raise ValueError('Sweep candidate selection changed during admission')
    return root/record['cache'],dict(path=str(selection),
        sha256=hashlib.sha256(selection.read_bytes()).hexdigest(),cache=record['cache'])


@contextmanager
def temporary_fallback_link(fallback,package):
    def cancel(signum,frame):raise SystemExit(128+signum)
    handlers={kind:signal.getsignal(kind) for kind in (signal.SIGINT,signal.SIGTERM)}
    identity=None
    for kind in handlers:signal.signal(kind,cancel)
    try:
        blocked=signal.pthread_sigmask(signal.SIG_BLOCK,set(handlers)) if hasattr(signal,'pthread_sigmask') else None
        try:
            fallback.symlink_to(package,target_is_directory=True)
            stat=fallback.lstat();identity=(stat.st_dev,stat.st_ino)
        finally:
            if blocked is not None:signal.pthread_sigmask(signal.SIG_SETMASK,blocked)
        yield
    finally:
        if identity is not None and fallback.is_symlink():
            stat=fallback.lstat()
            if identity==(stat.st_dev,stat.st_ino) and fallback.readlink()==package:fallback.unlink()
        for kind,previous in handlers.items():signal.signal(kind,previous)


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--qualification',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    a=p.parse_args();root=Path.cwd();scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    source=scratch/'source';build=scratch/'build';built_runtime=build/'halo2_native'
    assert source.is_dir() and built_runtime.is_file() and not a.output.resolve().is_relative_to(scratch)
    q=json.loads(a.qualification.read_text())
    qualified=q['status']=='prepared'
    assert (qualified and q['passed'] is True and q['qualified_candidate'] is True)or(q['status']=='failed'and q['passed']is False)
    fallback=root/Path(q['runtime']).parent
    assert fallback.parent==root/'bin/engine-builds' and not fallback.is_symlink()
    assert fallback.is_dir() if qualified else not fallback.exists()
    runtime=root/q['runtime'] if qualified else built_runtime
    sys.path.insert(0,str(source/'tools'))
    import steam_workshop as workshop
    import test_installed_workshop_mods as sweep
    from stage_native_renderer_shaders import stage_production_shaders
    from native_renderer_shaders import validate_native_package
    from build_launcher_engine import source_hashes
    frozen_pins=source_hashes(source)
    if qualified:
        assert frozen_pins==json.loads((fallback/'source-hashes.json').read_text())
        assert workshop.pin(runtime)==workshop.pin(built_runtime) and workshop.pin(runtime)['sha256']==q['runtime_sha256']
        base_cache,base_selection=qualified_base_cache(root,fallback,q)
    else:
        base_cache=root/'assets/imported/mcc-sky-water-tu15-20261025-v7'
        base_selection=None
    a.output.mkdir(parents=True,exist_ok=True)
    argv=[sys.executable,str(root/'tools/run_tests.py'),'--build-worker','--source',str(source),
        '--output',str(a.output/'probe-build'),'--target','workshop_mod_probe','--build-only','--jobs','2']
    with (a.output/'probe-worker.log').open('w') as log:
        subprocess.run(argv,stdout=log,stderr=subprocess.STDOUT,check=True)
    expected=q['native_renderer_shader_package']
    package=fallback if qualified else scratch/'sweep-render-runtime'
    staged=None if qualified else stage_production_shaders(build,package/'shaders')
    proof=validate_native_package(package/'shaders',executable=runtime)
    inventory=hashlib.sha256(('\n'.join(sorted(name for name in proof['files']
        if name!=proof['package']['path']))+'\n').encode()).hexdigest()
    assert proof['module_count']==expected['module_count'] and inventory==expected['inventory_sha256']
    if staged is not None:
        assert staged['inventory_sha256']==expected['inventory_sha256'] and staged['module_count']==expected['module_count']
    if qualified:assert proof['package']['sha256']==expected['report_sha256']
    direct_pin=workshop.pin
    @lru_cache(maxsize=512)
    def cached(path,identity):
        result=direct_pin(Path(path));stat=Path(path).stat()
        assert identity==(stat.st_dev,stat.st_ino,stat.st_size,stat.st_mtime_ns,stat.st_ctime_ns)
        return tuple(sorted(result.items()))
    def preparation_pin(path):
        path=Path(path);stat=path.stat()
        return dict(cached(str(path),(stat.st_dev,stat.st_ino,stat.st_size,stat.st_mtime_ns,stat.st_ctime_ns)))
    original_prepare=workshop.prepare_cached
    def prepared(*args,**kwargs):
        # Runtime/map before-after hashes and resource hashes after every map
        # remain direct reads. Memoization is confined to preparation calls.
        assert workshop.pin is direct_pin
        workshop.pin=preparation_pin
        try:return original_prepare(*args,**kwargs)
        finally:workshop.pin=direct_pin
    control=scratch/'preparation-pin-control';control.write_bytes(b'first')
    initial=preparation_pin(control);mtime=control.stat().st_mtime_ns
    control.write_bytes(b'other');os.utime(control,ns=(mtime,mtime));assert preparation_pin(control)!=initial
    replacement=scratch/'replacement-pin-control';replacement.write_bytes(b'third')
    os.utime(replacement,ns=(mtime,mtime));replacement.replace(control);assert preparation_pin(control)!=initial
    workshop.prepare_cached=prepared
    receipt=dict(scope=__doc__,qualification=str(a.qualification),qualification_passed=q['passed'],
        uses_qualified_package=qualified,complete_mod_support_qualified=False,
        coordinator_source_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        runtime=dict(path=str(runtime),**direct_pin(runtime)),shader_package_sha256=proof['package']['sha256'],
        exact_prior_shader_package=proof['package']['sha256']==expected['report_sha256'],
        module_count=proof['module_count'],inventory_sha256=inventory,
        base_cache=str(base_cache),base_cache_selection=base_selection,
        base_cache_matches_candidate_selection=qualified,
        temporary_fallback=None if qualified else str(fallback),temporary_shader_owner=None if qualified else str(package),
        canonical_package=str(package)if qualified else None,preparation_cache_controls_passed=True,
        runtime_unpublished=True,retained_large_artifacts=[])
    try:
        with nullcontext()if qualified else temporary_fallback_link(fallback,package):
            sys.argv=[str(source/'tools/test_installed_workshop_mods.py'),'--runtime',str(runtime),
                '--probe',str(build/'workshop_mod_probe'),'--base-cache',str(base_cache),
                '--project-root',str(root),'--output',str(a.output/'runtime')]
            result=sweep.main();receipt['sweep_exit_code']=result
            receipt['runtime_unchanged']=direct_pin(runtime)=={k:v for k,v in receipt['runtime'].items() if k!='path'}
            receipt['shader_package_unchanged']=validate_native_package(package/'shaders',executable=runtime)==proof
            receipt['source_cohort_unchanged']=source_hashes(source)==frozen_pins
            assert receipt['runtime_unchanged']and receipt['shader_package_unchanged']and receipt['source_cohort_unchanged']
        return result
    finally:
        workshop.pin=direct_pin;workshop.prepare_cached=original_prepare
        receipt['temporary_fallback_removed']=None if qualified else not fallback.exists() and not fallback.is_symlink()
        if qualified:receipt['qualified_package_preserved']=runtime.is_file()and(package/'shaders').is_dir()
        receipt['preparation_cache']=cached.cache_info()._asdict()
        receipt['original_map_runtime_and_resource_verification']='direct uncached reads outside preparation'
        (a.output/'candidate-route.json').write_text(json.dumps(receipt,indent=2)+'\n')


if __name__=='__main__':raise SystemExit(main())

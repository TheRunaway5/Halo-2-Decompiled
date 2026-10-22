"""Select MCC-only presentation producers from an authenticated runtime owner.

This chooses import requirements only. Cache/resource admission and runtime
availability remain the responsibility of the existing capability readers.
Without a capability, the existing V3 preparation stays explicitly unqualified.
"""
from pathlib import Path, PurePosixPath

from audit_rendering import FormatError
from installation_paths import resource_root
from native_gameplay_test import (FRONTEND_MAP, _digest, _file_digest,
                                  _frontend_contract, _runtime_owner)
from frontend_runtime_selection import (SELECTION, check_owner_unchanged,
                                        selected_frontend_owner)

AUTHORED = 'authored-v2'
V3 = 'mcc-v3'
UPDATE_REQUIRED = ('MCC-only setup requires an MCC-capable runtime update with '
                   'separately validated V3 loading and native MCC prompt fonts; '
                   'existing reference assets and proofs were preserved')


def _family(report):
    contract = _frontend_contract(report)
    if not contract['native_fonts']:
        raise FormatError(UPDATE_REQUIRED)
    resources = report['resources']
    if report['map'].get('path') != FRONTEND_MAP:
        raise FormatError('Presentation capability covers a different MCC map')
    _digest(report['map']['sha256'])
    if not isinstance(resources, dict) or len(resources) > 1024:
        raise FormatError('Invalid frontend presentation resources')
    for name, digest in resources.items():
        relative = PurePosixPath(name)
        if (relative.is_absolute() or str(relative) != name or '\\' in name or
                any(p in ('', '.', '..') for p in name.split('/')) or
                (relative.parts[0] not in ('halo2', 'ui') and
                 name != 'presentation/ui/native-layout.json' and
                 name not in contract['allowed_presentation'])):
            raise FormatError('Unsupported frontend presentation resource owner')
        _digest(digest)
    if not contract['loading_files'] <= resources.keys():
        raise FormatError('Explicit V3 capability omits its loading resources')
    return V3


def _require_v3(value):
    """Reject prior dispatch values too; reference helpers are separate APIs."""
    try:
        if (value['family'] != V3 or type(value['qualified_owner']) is not bool or
                not isinstance(value['owner_root'], Path) or
                not isinstance(value['selections'], dict) or
                (value['absent_capability'] is not None and
                 not isinstance(value['absent_capability'], Path))):
            raise FormatError(UPDATE_REQUIRED)
        owner = value['owner']
        if (owner is None) != (not value['qualified_owner']):
            raise FormatError(UPDATE_REQUIRED)
        if owner is not None:
            _family(owner['report'])
    except (KeyError, TypeError, AttributeError) as error:
        raise FormatError(UPDATE_REQUIRED) from error


def requirements(root, *, runtime=None):
    """Choose once before import; invalid explicit choices never use a default."""
    try:
        return _requirements(root, runtime=runtime)
    except (KeyError, TypeError, AttributeError) as error:
        raise FormatError('Invalid frontend presentation capability') from error


def _requirements(root, *, runtime=None):
    root = Path(root).resolve()
    explicit = runtime is not None
    owner_root, owner = root, None
    sidecar = None
    selection = root / SELECTION
    release = root / 'runtime-selection.json'
    if runtime is not None:
        runtime = Path(runtime)
        sidecar = runtime.with_name(runtime.name + '.frontend-test.json')
        if sidecar.exists() or sidecar.is_symlink():
            owner = _runtime_owner(root, runtime, frontend=True)
    elif selection.exists() or selection.is_symlink():
        owner = selected_frontend_owner(root)
    elif release.exists() or release.is_symlink():
        from update_runtime import selected_runtime
        runtime = selected_runtime(root)
        if runtime is None:
            raise FormatError('Selected release runtime is invalid; presentation import stopped')
        sidecar = runtime.with_name(runtime.name + '.frontend-test.json')
        if sidecar.exists() or sidecar.is_symlink():
            owner = _runtime_owner(root, runtime, frontend=True)
    else:
        owner_root = resource_root(root)
        runtime = owner_root / 'bin/halo2_native'
        sidecar = runtime.with_name(runtime.name + '.frontend-test.json')
        if sidecar.exists() or sidecar.is_symlink():
            owner = _runtime_owner(owner_root, runtime, frontend=True)
    return dict(family=_family(owner['report']) if owner else V3,
                qualified_owner=owner is not None, owner=owner, owner_root=owner_root,
                # Pin absence too: setup must not publish for a newly selected owner.
                selections={} if explicit else {
                    p: _file_digest(p) if p.exists() or p.is_symlink() else None
                    for p in (selection, release)},
                absent_capability=sidecar if owner is None else None)


def recheck(value):
    _require_v3(value)
    absent = value['absent_capability']
    if absent is not None and (absent.exists() or absent.is_symlink()):
        raise FormatError('Runtime capability appeared during presentation import')
    for path, digest in value['selections'].items():
        actual = _file_digest(path) if path.exists() or path.is_symlink() else None
        if actual != digest:
            raise FormatError('Runtime selection changed during presentation import')
    if value['owner'] is not None:
        check_owner_unchanged(value['owner'])
        current = _runtime_owner(value['owner_root'], value['owner']['runtime'], frontend=True)
        if current != {k: v for k, v in value['owner'].items()
                       if k not in ('selection', 'selection_sha256')}:
            raise FormatError('Runtime presentation capability changed during import')


def required_bytes(root, value):
    _require_v3(value)
    return 0


def preflight(root, source, value):
    _require_v3(value)
    from mcc_loading_presentation import preflight_v3
    preflight_v3(root, source)
    return 0


def prepare(root, stage, source, value, *, progress=print):
    _require_v3(value)
    import mcc_loading_presentation as loading
    return dict(native_loading_presentation=loading.prepare_v3(root, stage, source, progress=progress))


def compatible(report, value):
    _require_v3(value)
    from mcc_loading_presentation import PROFILE, V3_PROFILE
    previous = report.get('native_loading_presentation')
    if previous is None:
        return True
    if not isinstance(previous, dict):
        raise FormatError('Invalid existing loading presentation; cache retained')
    actual = previous.get('profile')
    if actual not in (PROFILE, V3_PROFILE):
        raise FormatError('Customized loading presentation; cache retained')
    return actual == V3_PROFILE


def repair(root, cache, source, value, *, progress=print):
    _require_v3(value)
    import mcc_loading_presentation as loading
    return loading.repair_v3(root, cache, source, progress=progress)

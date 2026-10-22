"""Select an independently qualified local MCC frontend without changing gameplay."""
import json
from pathlib import Path, PurePosixPath

from native_gameplay_test import _check, _digest, _file_digest, _runtime_owner

SCHEMA = 'H2_LOCAL_FRONTEND_RUNTIME_SELECTION_V1'
SELECTION = 'frontend-runtime-selection.json'


def selected_frontend_owner(root, fallback_runtime=None):
    """Authenticate a selected owner before cache import; grant no availability.

    Unlike the public availability reader, this strict reader distinguishes a
    missing selection from a damaged explicit selection. Import must not hide
    the latter by choosing another presentation family.
    """
    root = Path(root).resolve(strict=True)
    selection = root / SELECTION
    if not selection.exists() and not selection.is_symlink():
        if fallback_runtime is None:
            return None
        return _runtime_owner(root, fallback_runtime, frontend=True)
    if selection.is_symlink() or not selection.is_file() or selection.stat().st_size > 65536:
        raise ValueError('Local frontend selection lost its bounded direct owner')
    selection_sha = _file_digest(selection)
    report = json.loads(selection.read_text())
    if report.get('schema') != SCHEMA:
        raise ValueError('Unsupported local frontend selection')
    name = report['runtime']
    if not isinstance(name, str):
        raise ValueError('Local frontend requires a relative runtime owner')
    relative = PurePosixPath(name)
    if (relative.is_absolute() or str(relative) != name or '\\' in name or
            not relative.parts or any(p in ('', '.', '..') for p in name.split('/'))):
        raise ValueError('Local frontend runtime escaped its project owner')
    runtime = root
    for part in relative.parts:
        runtime /= part
        if runtime.is_symlink():
            raise ValueError('Local frontend runtime has a symlink component')
    if runtime.resolve(strict=True) != runtime or not runtime.is_file():
        raise ValueError('Local frontend runtime has no direct owner')
    capability = runtime.with_name(runtime.name + '.frontend-test.json')
    expected_runtime = _digest(report['runtime_sha256'])
    expected_capability = _digest(report['capability_sha256'])
    if (_file_digest(runtime) != expected_runtime or
            _file_digest(capability) != expected_capability):
        raise ValueError('Selected frontend runtime or capability changed')
    owner = _runtime_owner(root, runtime, frontend=True)
    owner.update(selection=selection, selection_sha256=selection_sha)
    check_owner_unchanged(owner)
    return owner


def check_owner_unchanged(owner):
    runtime = owner['runtime']
    capability = runtime.with_name(runtime.name + '.frontend-test.json')
    if ((_file_digest(owner['selection']) != owner['selection_sha256'] if 'selection' in owner else False) or
            _file_digest(capability) != owner['capability_sha256'] or
            _file_digest(runtime) != owner['runtime_sha256']):
        raise ValueError('Local frontend owner changed during admission')


def selected_frontend_runtime(root, gameplay_runtime, cache):
    """Return one admitted owner, or None; an invalid explicit choice never falls back."""
    if cache is None:
        return None
    try:
        owner = selected_frontend_owner(root, gameplay_runtime)
        if owner is None:
            return None
        runtime = owner['runtime']
        _check(root, runtime, cache, frontend=True)
        check_owner_unchanged(owner)
        return runtime
    except (OSError, ValueError, KeyError, TypeError, AttributeError, RuntimeError):
        return None

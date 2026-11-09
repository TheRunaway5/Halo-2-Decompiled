#!/usr/bin/env python3
"""Reimport MCC and the complete custom bundle into a fresh private cache."""
import argparse
from pathlib import Path
import shutil
import tempfile
import uuid

from mcc_movies import _cancellation
from setup_progress import write_progress


def rebuild(root, *, mcc_root=None, runtime=None, progress=print):
    import mcc_onboarding
    from frontend_runtime_selection import SELECTION, selected_frontend_owner, check_owner_unchanged
    from mcc_content import directory
    root = Path(root).resolve(strict=True)
    registry = root / mcc_onboarding.REGISTRY
    before = registry.read_bytes() if registry.exists() else None
    selection = root/SELECTION
    sidecar = Path(runtime).with_name(Path(runtime).name+'.frontend-test.json') if runtime is not None else None
    # An absent proof permits asset preparation only. Invalid declared owners
    # still fail closed; rebuilding assets never upgrades an executable proof.
    unqualified = (sidecar is not None and not sidecar.exists() and not sidecar.is_symlink() and
                   not selection.exists() and not selection.is_symlink())
    owner = None if unqualified else selected_frontend_owner(root, runtime)
    # Recover the original installation when library auto-discovery is no
    # longer applicable. A manually chosen folder always wins.
    if mcc_root is None:
        try:
            current = mcc_onboarding.selected(root)
            if current and current[1].get('source'):
                mcc_root = directory(current[1]['source'])
        except (OSError, ValueError, KeyError, TypeError):
            pass
    progress('Rebuilding game assets, custom maps, modes, button prompts and celestial ranks…')
    cache, report = mcc_onboarding.setup(root, mcc_root, force_rebuild=True,
                                        select_cache=False, progress=progress,
                                        runtime=owner['runtime'] if owner else runtime)
    try:
        if owner is not None:
            from qualify_frontend_runtime import qualify
            diagnostics = root / 'diagnostics' / ('cache-rebuild-' + uuid.uuid4().hex)
            from mcc_movies import _owned
            _owned(diagnostics, root, directory=True).mkdir(parents=True)
            with tempfile.TemporaryDirectory(prefix='.cache-check-', dir=cache.parent) as temporary:
                qualify(root, owner['runtime'], cache, diagnostics, Path(temporary),
                        progress=progress, cache_scoped=True,
                        executable_owner=owner)
            check_owner_unchanged(owner)
        if (registry.read_bytes() if registry.exists() else None) != before:
            raise ValueError('Another operation changed the cache selection; its choice was preserved')
        result = mcc_onboarding.select(root, cache, verify=False, progress=progress)
    except BaseException:
        # Only this operation's fresh, unselected cache belongs to this cleanup.
        try:
            current = mcc_onboarding.selected(root)
        except (OSError, ValueError, KeyError, TypeError):
            current = None
        if cache.parent == root/'assets/imported' and (current is None or current[0] != cache):
            shutil.rmtree(cache)
        raise
    progress('Cache rebuild complete. Previous cache retained.')
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', required=True, type=Path)
    parser.add_argument('--runtime', type=Path, help='Fallback executable when no independent frontend is selected')
    parser.add_argument('--mcc-root', type=Path)
    parser.add_argument('--gui-owner', action='store_true', help=argparse.SUPPRESS)
    args = parser.parse_args()
    from setup_assets import _gui_owner
    try:
        with _cancellation(), _gui_owner(args.gui_owner):
            rebuild(args.root, mcc_root=args.mcc_root, runtime=args.runtime,
                    progress=write_progress if args.gui_owner else print)
        return 0
    except (OSError, ValueError, KeyError, TypeError, RuntimeError) as error:
        print('Cache rebuild failed: ' + str(error) + '. Choose your MCC folder and retry.', flush=True)
        return 1


if __name__ == '__main__':
    raise SystemExit(main())

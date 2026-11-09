"""Read-only launcher readiness with actionable reasons; never creates admission."""
from dataclasses import dataclass
import os
from pathlib import Path


@dataclass(frozen=True)
class LauncherHealth:
    ready: bool
    message: str
    category: str
    detail: str = ''

    def __bool__(self):
        return self.ready


def inspect_launcher(root, runtime=None):
    from mcc_onboarding import selected
    from frontend_runtime_selection import selected_frontend_owner, check_owner_unchanged
    from native_gameplay_test import _check

    root = Path(root).resolve()
    if runtime is None:
        candidates = (root / ('halo2_native.exe' if os.name == 'nt' else 'halo2_native'),
                      root / 'bin/halo2_native')
        runtime = next((path for path in candidates if path.is_file()), None)
    try:
        source = selected(root)
    except (OSError, ValueError, KeyError, TypeError) as error:
        return LauncherHealth(False, 'The imported game files need repair.', 'assets', str(error))
    if source is None:
        return LauncherHealth(False, 'MCC game files have not been prepared.', 'assets')
    try:
        from custom_asset_cache import verify as verify_custom_bundle
        verify_custom_bundle(root, *source)
    except (OSError, ValueError, KeyError, TypeError) as error:
        return LauncherHealth(False, 'Custom assets need Rebuild Cache.', 'assets', str(error))
    try:
        owner = selected_frontend_owner(root, runtime)
        if owner is None:
            return LauncherHealth(False, 'The menu runtime is missing. Select Repair to check the installation.', 'runtime')
        _check(root, owner['runtime'], source[0], frontend=True, owner=owner)
        check_owner_unchanged(owner)
        return LauncherHealth(True, 'Ready to play', 'ready')
    except (OSError, ValueError, KeyError, TypeError, AttributeError, RuntimeError) as error:
        detail = str(error)
        if 'sidecar' in detail:
            return LauncherHealth(False, 'This game build is missing its menu startup check.', 'validation', detail)
        return LauncherHealth(False, 'The menu runtime or its game files need repair.', 'runtime', detail)

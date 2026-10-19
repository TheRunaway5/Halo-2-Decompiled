"""MCC-only input admission for weapon runtime validation.

This checks the actual cache header, rather than trusting a filename. It never
substitutes Xbox/Vista metadata or performs an import/conversion.
"""
from pathlib import Path
import struct

from mcc_content import directory
from audit_rendering import file_sha as sha


def require_mcc_map(path):
    path = Path(path).expanduser().resolve(strict=True)
    with path.open('rb') as stream:
        header = stream.read(8)
    if len(header) != 8 or struct.unpack('<II', header) != (0x68656164, 13):
        raise ValueError(f'Weapon runtime testing requires an MCC version13 cache: {path}')
    return path


def mcc_runtime_inputs(root):
    halo2 = directory(root)
    maps = halo2 / 'h2_maps_win64_dx11'
    files = [require_mcc_map(maps / name) for name in (
        '01b_spacestation.map', 'lockout.map', 'mainmenu.map',
        'shared.map', 'single_player_shared.map')]
    return maps, dict(
        runtime_cache_source='MCC', mcc_halo2_directory=str(halo2),
        maps_directory=str(maps), cache_version=13,
        files=[dict(path=str(path), bytes=path.stat().st_size, sha256=sha(path)) for path in files],
        xbox_and_vista_cache_fallback_allowed=False,
        tag_conversion_performed_by_test=False,
        authority='Xbox TU1.5 behavior and visuals; MCC supplies runtime content')

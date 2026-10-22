"""Run actual Ghost firing on a private display with disposable captures."""
import os
from pathlib import Path
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from validate_weapon_runtime_tu15 import private_display
from weapon_test_cache import require_mcc_map


def main():
    executable, map_, report, *mode = sys.argv[1:]
    map_ = Path(map_)
    if not map_.is_file():
        return 77
    require_mcc_map(map_)
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    root = Path(__file__).resolve().parents[1]
    with private_display(scratch) as display:
        environment = dict(os.environ, DISPLAY=display, SDL_VIDEODRIVER='x11',
                           SDL_AUDIODRIVER='dummy', XDG_CACHE_HOME=str(scratch / 'cache'),
                           HALO2_TEST_VEHICLE_WEAPON_HUD_CASE='ghost',
                           HALO2_VEHICLE_HUD_GPU_NAME='ghost',
                           HALO2_TEST_GHOST_PROJECTILE_RENDER='1',
                           HALO2_TEST_VEHICLE_WEAPON_HUD_REPORT=report)
        if mode == ['rocket-explosion']:
            environment.pop('HALO2_TEST_GHOST_PROJECTILE_RENDER')
            environment['HALO2_TEST_ROCKET_LOCK_ON'] = '1'
            environment['HALO2_TEST_ROCKET_EXPLOSION_RENDER'] = '1'
        layers = root / '.tools/validation/usr/share/vulkan/explicit_layer.d'
        if layers.is_dir():
            environment['VK_LAYER_PATH'] = str(layers)
            environment['LD_LIBRARY_PATH'] = ':'.join([
                str(root / '.tools/validation/usr/lib'),
                str(root / '.tools/validation/usr/lib/x86_64-linux-gnu'),
                environment.get('LD_LIBRARY_PATH', '')])
        return subprocess.run([executable, str(map_), str(map_.parent), 'native-mcc',
                               str(scratch / 'ghost.ppm')], env=environment,
                              timeout=160, check=False).returncode


if __name__ == '__main__':
    raise SystemExit(main())

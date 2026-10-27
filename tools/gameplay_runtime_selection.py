"""Admit a locally qualified MCC multiplayer runtime independently of full parity."""
import argparse
import json
import os
from pathlib import Path, PurePosixPath
import sys

from native_gameplay_test import _digest, _file_digest, validate_eos_worker

SCHEMA = 'H2_LOCAL_GAMEPLAY_RUNTIME_SELECTION_V1'
SELECTION = 'gameplay-runtime-selection.json'


def _file(root, name):
    if not isinstance(name, str):
        raise ValueError('Gameplay selection needs a relative file')
    relative = PurePosixPath(name)
    if (relative.is_absolute() or str(relative) != name or '\\' in name or
            not relative.parts or any(p in ('', '.', '..') for p in name.split('/'))):
        raise ValueError('Gameplay selection escaped its project owner')
    path = root
    for part in relative.parts:
        path /= part
        if path.is_symlink():
            raise ValueError('Gameplay selection has a symlink component')
    if not path.is_file() or path.resolve(strict=True) != path:
        raise ValueError('Gameplay selection has no direct file owner')
    return path


def selected_gameplay_runtime(root, *, selection_name=SELECTION, cache_candidate=None):
    """Absent means no override; an invalid explicit choice raises and never falls back.

    Builders may check a project-owned candidate selection and an explicitly
    verified private cache without changing either ROOT selection.
    """
    root = Path(root).resolve(strict=True)
    selection = root / selection_name
    if not selection.exists() and not selection.is_symlink():
        return None
    if selection.is_symlink() or not selection.is_file() or selection.stat().st_size > 65536:
        raise ValueError('Gameplay selection lost its bounded direct owner')
    selection = _file(root, selection_name)
    selection_pin = _file_digest(selection)
    record = json.loads(selection.read_text())
    if record.get('schema') != SCHEMA:
        raise ValueError('Unsupported local gameplay selection')
    runtime = _file(root, record['runtime'])
    proof = _file(root, record['qualification'])
    if _file_digest(proof) != _digest(record['qualification_sha256']):
        raise ValueError('Gameplay qualification changed')
    qualification = json.loads(proof.read_text())
    pin = _digest(record['runtime_sha256'])
    if (qualification.get('passed') is not True or
            qualification.get('runtime_sha256') != pin or
            qualification.get('runtime') != record['runtime'] or
            qualification.get('unresolved_case_failures') != []):
        raise ValueError('Gameplay runtime has no passed qualification')
    inventory = _file(root, record['inventory'])
    if inventory.parent != runtime.parent or _file_digest(inventory) != _digest(record['inventory_sha256']):
        raise ValueError('Gameplay runtime inventory changed')
    files = json.loads(inventory.read_text())
    if 'eos_worker' in qualification or 'eos_worker' in record:
        declaration = qualification.get('eos_worker')
        if declaration != record.get('eos_worker'):
            raise ValueError('Gameplay EOS worker qualification changed')
        validate_eos_worker(runtime, declaration)
        item = files.get(declaration['name'], {})
        if (item.get('sha256') != declaration['sha256'] or
                item.get('bytes') != declaration['bytes']):
            raise ValueError('Gameplay EOS worker inventory changed')
    shaders = runtime.parent / 'shaders'
    actual = {p.relative_to(shaders).as_posix() for p in shaders.rglob('*') if p.is_file() or p.is_symlink()}
    expected = {name[len('shaders/'):] for name in files if name.startswith('shaders/')}
    baseline = {'world.vert.spv', 'world.frag.spv', 'world_emblem.geom.spv', 'world_emblem.frag.spv',
                'fog.vert.spv', 'fog.frag.spv', 'water_copy.frag.spv', 'bsp_patchy_noise.frag.spv',
                'bsp_patchy_composite.frag.spv', 'patchy_noise.frag.spv', 'patchy_composite.frag.spv',
                'bloom.vert.spv', 'bloom.frag.spv', 'crt_oled.frag.spv', 'display.frag.spv',
                'overlay.vert.spv', 'overlay.frag.spv', 'ui.vert.spv', 'ui.frag.spv', 'hud.frag.spv'}
    from native_renderer_shaders import AOT_PACKAGE_NAME, validate_native_package
    package = shaders/AOT_PACKAGE_NAME
    if package.exists() or package.is_symlink():
        declaration = qualification.get('native_renderer_shader_package', {})
        item = files.get('shaders/'+AOT_PACKAGE_NAME, {})
        if (declaration.get('passed') is not True or
                declaration.get('report') != str(package.relative_to(root)) or
                declaration.get('report_sha256') != item.get('sha256')):
            raise ValueError('Gameplay qualification lacks its native shader package identity')
        package_proof = validate_native_package(shaders, expected_sha256=_digest(item['sha256']),
                                                executable=runtime)
        if (set(package_proof['files']) != expected or
                package_proof['module_count'] != declaration.get('module_count')):
            raise ValueError('Gameplay native shader catalog differs from qualification')
    elif not baseline <= expected:
        raise ValueError('Gameplay shader inventory omits its declared legacy family')
    if actual != expected:
        raise ValueError('Gameplay shader inventory changed')
    for name, value in files.items():
        if name.startswith(('shaders/', 'data/', 'upscalers/')):
            path = _file(root, str(runtime.parent.relative_to(root) / name))
            if _file_digest(path) != _digest(value['sha256']):
                raise ValueError('Gameplay runtime resource changed: ' + name)
    if not os.access(runtime, os.X_OK) or _file_digest(runtime, embedded=os.fsencode(shaders)) != pin:
        raise ValueError('Gameplay executable or compiled shader binding changed')
    if cache_candidate is None:
        from mcc_onboarding import selected
        source = selected(root)
    else:
        from mcc_onboarding import private_cache
        candidate = Path(cache_candidate)
        source = private_cache(root, candidate if candidate.is_absolute() else root / candidate,
                               verify=True)
    if source is None or str(source[0].relative_to(root)) != record['cache']:
        raise ValueError('Gameplay selection requires its qualified prepared MCC cache')
    maps = source[0] / 'halo2/h2_maps_win64_dx11'
    for name, expected in record['maps'].items():
        if Path(name).name != name or not name.endswith('.map'):
            raise ValueError('Gameplay map escaped its MCC directory')
        if _file_digest(maps / name) != _digest(expected):
            raise ValueError('Qualified MCC map changed: ' + name)
    if (record['default_map'] not in record['maps'] or
            type(record['local_players']) is not int or not 1 <= record['local_players'] <= 4):
        raise ValueError('Gameplay defaults are outside their qualified inputs')
    if record.get('video_driver', '') not in ('', 'x11'):
        raise ValueError('Unsupported qualified gameplay video driver')
    if _file_digest(selection) != selection_pin or _file_digest(proof) != record['qualification_sha256']:
        raise ValueError('Gameplay selection changed during admission')
    return record


def validate_arguments(root, record, arguments):
    maps = Path(root) / record['cache'] / 'halo2/h2_maps_win64_dx11'
    forbidden = {'--campaign', '--play', '--simulate', '--gameplay-xbe', '--mp-source-object-world',
                 '--menu-ui-assets', '--hud-ui-assets', '--pause-ui-assets',
                 '--menu', '--menu-screen', '--menu-maps', '--menu-audit', '--menu-sweep', '--menu-movie'}
    for i, option in enumerate(arguments):
        if option in forbidden or option.startswith('--reference-'):
            raise ValueError('Selected gameplay qualification covers ordinary MCC multiplayer')
        if option in ('--map', '--shared-maps'):
            if arguments.count(option) != 1 or i + 1 == len(arguments):
                raise ValueError('Specify one complete ' + option)
            path = Path(arguments[i + 1])
            if not path.is_absolute():
                path = maps / path
            path = path.resolve(strict=True)
            if option == '--shared-maps':
                if path != maps:
                    raise ValueError('Shared maps must use the qualified MCC cache')
            elif path.parent != maps or path.name not in record['maps']:
                raise ValueError('Select a qualified multiplayer map from the MCC cache')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, required=True)
    parser.add_argument('--runtime', type=Path)
    parser.add_argument('arguments', nargs=argparse.REMAINDER)
    args = parser.parse_args()
    try:
        record = selected_gameplay_runtime(args.root)
        if record is None:
            return 1
        if args.runtime is not None and (args.root.resolve() / record['runtime']) != args.runtime:
            raise ValueError('Selected gameplay owner changed before launch')
        arguments = args.arguments[1:] if args.arguments[:1] == ['--'] else args.arguments
        validate_arguments(args.root, record, arguments)
        return 0
    except (OSError, ValueError, KeyError, TypeError, RuntimeError) as error:
        print(str(error), file=sys.stderr)
        return 1


if __name__ == '__main__':
    raise SystemExit(main())

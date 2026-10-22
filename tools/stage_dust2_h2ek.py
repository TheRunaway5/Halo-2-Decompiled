#!/usr/bin/env python3
"""Stage a private Classic compiler overlay without changing the installed kit.

Run through tools/run_test.py. Stock assets are read through symlinks; every
map tag, shader collection, generated cache and compiler log is privately owned.
"""
import argparse
import json
import os
from pathlib import Path
import shutil


def stage(installed, package, output):
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    output = output.resolve()
    assert output.is_relative_to(scratch) and not output.exists()
    output.mkdir()

    def overlay(source, destination, private):
        destination.mkdir()
        for child in source.iterdir():
            if child.name in private:
                overlay(child, destination / child.name, private[child.name])
            else:
                (destination / child.name).symlink_to(child.resolve(), target_is_directory=child.is_dir())

    # These directory chains are the only mutable portions of the stock tree.
    overlay(installed / 'tags', output / 'tags', {'scenarios': {
        'multi': {'dust2_mombasa': {}}, 'shaders': {}}})
    map_tags = output / 'tags/scenarios/multi/dust2_mombasa'
    for child in list(map_tags.iterdir()):
        child.unlink()
    for child in (installed / 'tags/scenarios/multi/dust2_mombasa').iterdir():
        if child.suffix in ('.scenario_structure_bsp', '.scenario_structure_lightmap', '.bitmap'):
            continue
        destination = map_tags / child.name
        if child.is_dir():
            shutil.copytree(child, destination)
        else:
            shutil.copyfile(child, destination)
    collections = output / 'tags/scenarios/shaders/shader_collections.shader_collections'
    collections.unlink()
    text = (installed / 'tags/scenarios/shaders/shader_collections.shader_collections').read_text()
    collections.write_text(text.rstrip() + '\nhog objects\\vehicles\\warthog\\shaders\n')

    overlay(installed / 'data', output / 'data', {'scenarios': {'multi': {'dust2_mombasa': {}}}})
    map_data = output / 'data/scenarios/multi/dust2_mombasa'
    for child in list(map_data.iterdir()):
        child.unlink()
    for child in (package / 'data/scenarios/multi/dust2_mombasa').iterdir():
        if child.is_dir():
            shutil.copytree(child, map_data / child.name)
        else:
            shutil.copyfile(child, map_data / child.name)
    for name in ('tool_fast.exe', 'libfbxsdk.dll'):
        (output / name).symlink_to((installed / name).resolve())
    (output / 'overlay.json').write_text(json.dumps({
        'schema': 'H2_DUST2_PRIVATE_COMPILER_OVERLAY_V1',
        'installed_reference': str(installed.resolve()), 'package': str(package.resolve()),
        'stock_assets_are_read_only_references': True,
        'private_map_tags': str(map_tags), 'private_map_data': str(map_data),
    }, indent=2) + '\n')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('installed', 'package', 'output'):
        parser.add_argument('--' + name, type=Path, required=True)
    stage(**vars(parser.parse_args()))

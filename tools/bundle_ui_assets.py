#!/usr/bin/env python3
"""Bundle, verify, or install the source-owned MCC artwork and Vista redraws."""
import argparse
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import shutil
import tempfile
import uuid

ROOT = Path(__file__).resolve().parents[1]
PACKS = ('mcc-classic-hud', 'mcc-classic-menus', 'vista-prompts-svg-4k')
REGISTRIES = {'mcc-classic-hud': ('hud-source.tsv', 'H2_HUD_SOURCE_V1'),
              'mcc-classic-menus': ('menu-source.tsv', 'H2_MENU_SOURCE_V1')}
RUNTIME_BINDINGS = {'menu-ui-source.tsv', 'prompt-source.tsv'}
SCHEMA = 'halo2-bundled-ui-v1'


def sha(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def contained(root, name):
    relative = PurePosixPath(name)
    if relative.is_absolute() or not relative.parts or any(p in ('.', '..') for p in relative.parts) or '\\' in name or ':' in name:
        raise ValueError('Invalid bundled UI path: ' + name)
    path = root.joinpath(*relative.parts)
    if not path.resolve().is_relative_to(root.resolve()):
        raise ValueError('Bundled UI path escapes its directory: ' + name)
    return path


def verify(bundle):
    bundle = bundle.resolve()
    manifest = json.loads((bundle/'bundle.json').read_text())
    if manifest.get('schema') != SCHEMA or set(manifest.get('packs', {})) != set(PACKS):
        raise ValueError('Invalid bundled UI manifest')
    total = 0
    expected = {'bundle.json'}
    for name, pack in manifest['packs'].items():
        expected.update(name+'/'+relative for relative in pack['files'])
        for relative, record in pack['files'].items():
            path = contained(bundle, name+'/'+relative)
            if not path.is_file() or path.stat().st_size != record['bytes'] or sha(path) != record['sha256']:
                raise ValueError('Bundled UI asset changed or missing: '+name+'/'+relative)
            total += record['bytes']
    actual = {p.relative_to(bundle).as_posix() for p in bundle.rglob('*') if p.is_file()}
    if actual != expected:
        raise ValueError('Bundled UI inventory differs from its manifest')
    # Check every source-owned payload against its original import record too.
    for name in PACKS[:2]:
        pack = bundle/name
        source = json.loads((pack/'manifest.json').read_text())
        for bank in source['bitmaps']:
            for frame in bank['frames']:
                if sha(contained(pack, frame['dds'])) != frame['dds_sha256']:
                    raise ValueError('MCC bitmap provenance differs: '+frame['dds'])
        for filename, record in source['fonts'].items():
            path = contained(pack, 'fonts/'+filename)
            if path.stat().st_size != record['bytes'] or sha(path) != record['sha256']:
                raise ValueError('MCC font provenance differs: '+filename)
    prompts = bundle/PACKS[2]
    source = json.loads((prompts/'reconstruction.json').read_text())
    if sha(contained(prompts, source['source_table'])) != source['source_table_sha256']:
        raise ValueError('Vista reference table differs')
    for row in source['records']:
        for field in ('source_font', 'vector', 'atlas'):
            if sha(contained(prompts, row[field])) != row[field+'_sha256']:
                raise ValueError('Vista prompt provenance differs: '+row[field])
    return manifest, dict(passed=True, files=len(expected)-1, bytes=total,
        mcc_hud_pages=manifest['packs'][PACKS[0]]['pages'],
        mcc_menu_pages=manifest['packs'][PACKS[1]]['pages'],
        prompt_bindings=len(source['records']), prompt_vectors=source['unique_vectors'])


def export(root, output):
    root, output = root.resolve(), output.resolve()
    if output.exists():
        raise ValueError('Bundle output already exists; use --verify or a new --output')
    output.parent.mkdir(parents=True, exist_ok=True)
    stage = Path(tempfile.mkdtemp(prefix='.ui-bundle-', dir=output.parent))
    try:
        records = {}
        for name in PACKS:
            source, destination = root/'assets'/name, stage/name
            shutil.copytree(source, destination,
                ignore=shutil.ignore_patterns(*RUNTIME_BINDINGS) if name in REGISTRIES else None)
            if name == PACKS[2]:
                provenance = json.loads((source/'reconstruction.json').read_text())
                reference = destination/'reference/fonts'; reference.mkdir(parents=True, exist_ok=True)
                for row in provenance['records']:
                    original = Path(row['source_font'])
                    if not original.is_absolute():original = source/original
                    shutil.copy2(original, reference/original.name)
                    row['source_font'] = 'reference/fonts/'+original.name
                table = Path(provenance['source_table'])
                if not table.is_absolute():table = source/table
                shutil.copy2(table, reference/'font_table.txt')
                provenance['source_table'] = 'reference/fonts/font_table.txt'
                (destination/'reconstruction.json').write_text(json.dumps(provenance, indent=2)+'\n')
                summary = dict(bindings=len(provenance['records']), vectors=provenance['unique_vectors'],
                               atlases=provenance['atlas_count'], density=provenance['density'])
            else:
                source_manifest = json.loads((source/'manifest.json').read_text())
                summary = dict(pages=source_manifest['summary']['imported_pages'],
                    font_files=source_manifest['summary']['font_files'], font_tables=source_manifest['summary']['font_tables'])
            summary['files'] = {p.relative_to(destination).as_posix():
                dict(bytes=p.stat().st_size, sha256=sha(p)) for p in sorted(destination.rglob('*')) if p.is_file()}
            records[name] = summary
        tools = ('bundle_ui_assets.py', 'recreate_vista_prompts.py', 'vista_prompt_svg.py',
                 'vista_prompt_source.py', 'verify_vista_prompts.py')
        manifest = dict(schema=SCHEMA, packs=records,
            generator_sources={'tools/'+name:sha(root/'tools'/name) for name in tools},
            activation='tools/bundle_ui_assets.py --install; original Vista metadata selected through assets/asset-sources.tsv',
            provenance='MCC payloads/mips/fonts retain original import hashes; Vista SVGs and bitmaps retain redraw hashes. Original installation paths in MCC manifests are historical provenance, not runtime dependencies.')
        (stage/'bundle.json').write_text(json.dumps(manifest, indent=2, sort_keys=True)+'\n')
        _, result = verify(stage)
        stage.rename(output)
        return result
    finally:
        if stage.exists():shutil.rmtree(stage)


def selected_vista(root):
    rows = (root/'assets/asset-sources.tsv').read_text().splitlines()
    if not rows or rows[0] != 'H2_ASSET_SOURCES_V1':
        raise ValueError('Set up the original Xbox/Vista game files first')
    fields = dict(row.split('\t', 1) for row in rows[1:])
    vista = Path(fields['vista'])
    if not vista.is_absolute():vista = root/vista
    if not (vista/'maps/01a_tutorial.map').is_file():
        raise ValueError('Selected original Vista menu metadata is unavailable')
    return vista.resolve()


def install(root, bundle, only_unselected=False):
    root, bundle = root.resolve(), bundle.resolve()
    selected = [name for name, (registry, _) in REGISTRIES.items()
                if not only_unselected or not (root/'assets'/registry).exists()]
    if not selected:return dict(passed=True, installed=False, reason='Existing UI selections preserved')
    manifest, result = verify(bundle)
    vista = selected_vista(root)
    if any(c in str(vista) for c in '\t\r\n\0'):
        raise ValueError('Vista source path contains control characters')
    identifier = sha(bundle/'bundle.json')[:16]
    destination = root/'assets/bundled-ui'/identifier
    if not destination.exists():
        destination.parent.mkdir(parents=True, exist_ok=True)
        stage = Path(tempfile.mkdtemp(prefix='.ui-install-', dir=destination.parent))
        try:
            for name in PACKS:shutil.copytree(bundle/name, stage/name)
            shutil.copy2(bundle/'bundle.json', stage/'bundle.json')
            verify(stage)
            stage.rename(destination)
        finally:
            if stage.exists():shutil.rmtree(stage)
    else:
        # Bindings are generated locally and excluded from the source inventory.
        for name, pack in manifest['packs'].items():
            for relative, record in pack['files'].items():
                if sha(contained(destination, name+'/'+relative)) != record['sha256']:
                    raise ValueError('Installed UI asset changed: '+name+'/'+relative)
    for name in REGISTRIES:
        pack = destination/name
        (pack/'menu-ui-source.tsv').write_text('H2_HUD_MENU_SOURCE_V1\nvista\t'+str(vista)+'\n')
        (pack/'prompt-source.tsv').write_text('H2_VISTA_PROMPTS_V1\n'+str(destination/PACKS[2])+'\n')
    # All pack copies and bindings are ready before changing either selection.
    for name in selected:
        registry, header = REGISTRIES[name]
        temporary = root/'assets'/('.ui-source-'+uuid.uuid4().hex)
        temporary.write_text(header+'\nmcc\t'+(destination/name).relative_to(root).as_posix()+'\n')
        os.replace(temporary, root/'assets'/registry)
    return dict(result, installed=True, selected=selected, destination=str(destination))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=ROOT)
    parser.add_argument('--output', type=Path, help='New source bundle destination')
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument('--verify', action='store_true')
    mode.add_argument('--install', action='store_true')
    args = parser.parse_args()
    bundle = args.output or args.root/'resources/ui'
    if args.verify:_, result = verify(bundle)
    elif args.install:result = install(args.root, bundle)
    else:result = export(args.root, bundle)
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()

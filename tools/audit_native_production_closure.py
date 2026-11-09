#!/usr/bin/env python3
"""Export the configured halo2_native CMake File API dependency/source closure.

Use tools/native_inventory_fileapi.cmake as a private top-level include through
run_tests.py. Source aliases are prepared with the existing generators, without
compiling game libraries. All replies/aliases stay in owned test scratch; only
the bounded final JSON is retained. Configured linkage is not execution proof.
"""
import argparse
from collections import defaultdict
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import subprocess
import sys
from generated_routine_ownership import literal_source_paths

TOOLS = Path(__file__).resolve().parent
SCHEMA = 'native-production-cmake-closure-v1'


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def json_sha(value):
    return hashlib.sha256(json.dumps(value, sort_keys=True, separators=(',', ':')).encode()).hexdigest()

def retained_json(value,limit=1_048_576):
    """Keep all graph evidence while enforcing the existing retention cap."""
    content=json.dumps(value,indent=2,sort_keys=True)+'\n'
    if len(content.encode())>limit:
        content=json.dumps(value,sort_keys=True,separators=(',',':'))+'\n'
    if len(content.encode())>limit:
        raise ValueError('Final configured-closure report exceeds1MiB retention budget')
    return content


def read_json(path):
    return json.loads(Path(path).read_text())


def native_binding_groups(receipt):
    """Keep every source-qualified specialization category, without a closure claim."""
    groups = {}
    for name, entries in sorted(receipt.items()):
        if name != 'native_leaves' and not re.fullmatch(r'native_[a-z0-9_]+_(?:entries|leaves)', name):
            continue
        if not isinstance(entries, list) or any(not isinstance(value, str) or
                not re.fullmatch(r'(?:0x)?[0-9a-fA-F]{1,8}', value) for value in entries):
            raise ValueError('Malformed native specialization entry group: ' + name)
        groups[name] = entries
    return groups


def resolved_source(value, root, build, generated=False):
    path = Path(value)
    if path.is_absolute():
        return path.resolve()
    source = (root / path).resolve()
    if source.exists() or source.is_relative_to(build):
        return source
    return (build / path).resolve() if generated else source


def printable(path, root, build):
    path = Path(path).resolve()
    if path.is_relative_to(build):
        return '<build>/' + str(path.relative_to(build))
    if path.is_relative_to(root):
        return str(path.relative_to(root))
    return str(path)


def execution_alias(owner,source,root,build):
    variant=hashlib.sha256(str(source).encode()).hexdigest()[:16]
    return build/'native-execution'/variant/owner.relative_to(root)


def execution_owner(actual,canonical,root,build):
    """Authenticate both the canonical suffix and the input-path variant."""
    relative=actual.relative_to(build)
    if len(relative.parts)<4 or relative.parts[0]!='native-execution':return None
    owner=(root/Path(*relative.parts[2:])).resolve()
    if str(owner) not in canonical:return None
    for source in (owner,build/'native-operands'/owner.relative_to(root/'src')):
        if execution_alias(owner,source,root,build)==actual:return owner,source
    return None


def configured_graph(reply, configuration, root_target):
    indices = list(reply.glob('index-*.json'))
    if not indices:
        raise ValueError('Missing CMake File API index; configure with the private query include')
    index_path = max(indices, key=lambda path: (path.stat().st_mtime_ns, path.name))
    index = read_json(index_path)
    objects = {value['kind']: value for value in index['objects']}
    model_path = reply / objects['codemodel']['jsonFile']
    model = read_json(model_path)
    configurations = model['configurations']
    candidates = [value for value in configurations if value['name'] == configuration]
    if not candidates and len(configurations) == 1 and configurations[0]['name'] == '':
        candidates = configurations
    if len(candidates) != 1:
        raise ValueError(f'Ambiguous/missing configuration {configuration!r}')
    selected = candidates[0]
    references = {value['id']: value for value in selected['targets']}
    roots = [key for key, value in references.items() if value['name'] == root_target]
    if len(roots) != 1:
        raise ValueError(f'Ambiguous/missing production target {root_target}')
    targets, missing, pending = {}, set(), roots.copy()
    while pending:
        identity = pending.pop()
        if identity in targets or identity in missing:
            continue
        if identity not in references:
            missing.add(identity)
            continue
        path = reply / references[identity]['jsonFile']
        target = read_json(path)
        targets[identity] = (target, path)
        pending.extend(value['id'] for value in target.get('dependencies', []))
    auxiliary = {}
    for kind in ('cache', 'toolchains', 'cmakeFiles'):
        if kind in objects:
            path = reply / objects[kind]['jsonFile']
            auxiliary[kind] = (read_json(path), path)
    return index, index_path, model, model_path, selected['name'], targets, sorted(missing), auxiliary


def export(root, build, configuration='Release', root_target='halo2_native', prepare=False, context=None,
           collection_context=None):
    root, build = Path(root).resolve(), Path(build).resolve()
    index, index_path, model, model_path, config, targets, missing, auxiliary = configured_graph(
        build / '.cmake/api/v1/reply', configuration, root_target)
    if Path(model['paths']['source']).resolve() != root or Path(model['paths']['build']).resolve() != build:
        raise ValueError('File API source/build owner differs from the declared cohort')
    if prepare:
        scratch = os.environ.get('HALO2_TEST_SCRATCH')
        if not scratch or not build.is_relative_to(Path(scratch).resolve()):
            raise ValueError('Preparing aliases requires a build under HALO2_TEST_SCRATCH')
    canonical = {str(path.resolve()): path for path in literal_source_paths(root)}
    by_stem = defaultdict(list)
    for path in canonical.values():
        by_stem[path.stem].append(path)
    before_hashes = {key: sha(path) for key, path in canonical.items()}
    units, partition_paths, unresolved = {}, defaultdict(set), []
    execution_inputs={}
    target_rows = []
    for identity, (target, path) in sorted(targets.items()):
        source_paths = []
        for source in target.get('sources', []):
            actual = resolved_source(source['path'], root, build, source.get('isGenerated', False))
            source_paths.append(printable(actual, root, build))
            owner, kind = None, None
            if str(actual) in canonical:
                owner, kind = actual, 'canonical'
            elif actual.is_relative_to(build):
                relative = actual.relative_to(build)
                if relative.parts[0] == 'native-operands':
                    candidate = (root / 'src' / Path(*relative.parts[1:])).resolve()
                    if str(candidate) in canonical:
                        owner, kind = candidate, 'specialized'
                elif relative.parts[0]=='native-execution':
                    identified=execution_owner(actual,canonical,root,build)
                    if identified:
                        owner,input_source=identified;kind='accounted';execution_inputs[actual]=input_source
                elif len(relative.parts) >= 3 and relative.parts[0] == 'native-translations':
                    candidates = by_stem.get(relative.parts[1], [])
                    if len(candidates) == 1 and (actual.name == 'router.cpp' or re.fullmatch(r'body_\d+\.cpp', actual.name)):
                        owner, kind = candidates[0], 'partitioned'
                        partition_paths[owner].add(actual)
                if owner is None and actual.name.endswith('_generated.cpp'):
                    unresolved.append({'source': printable(actual, root, build), 'target': identity})
            if owner is None:
                continue
            row = units.setdefault(owner, {'source': str(owner.relative_to(root)),
                'source_sha256': before_hashes[str(owner)], 'targets': set(), 'aliases': {}})
            row['targets'].add(identity)
            row['aliases'][actual] = kind
        target_rows.append({'id': identity, 'name': target['name'], 'type': target['type'],
            'dependencies': sorted(value['id'] for value in target.get('dependencies', [])),
            'source_entries': len(source_paths), 'source_paths_sha256': json_sha(sorted(source_paths)),
            'file_api_target_sha256': sha(path)})
    cache = {value['name']: value.get('value') for value in auxiliary.get('cache', ({'entries': []}, None))[0]['entries']}
    specialized = str(cache.get('HALO2_SPECIALIZE_NATIVE_OPERANDS', 'ON')).upper() not in ('OFF', 'FALSE', '0', 'NO')
    accounting = str(cache.get('HALO2_NATIVE_EXECUTION_ACCOUNTING', 'OFF')).upper() not in ('OFF', 'FALSE', '0', 'NO')
    prepared = set()
    commands = []
    def specialize_source(owner):
        output = build / 'native-operands' / owner.relative_to(root / 'src')
        if prepare and output not in prepared:
            command = [sys.executable, str(TOOLS / 'specialize_native_operands.py'),
                       '--source', str(owner), '--output', str(output)]
            subprocess.run(command, check=True, stdout=subprocess.DEVNULL)
            commands.append([printable(Path(value), root, build) if value.startswith('/') else value for value in command])
            prepared.add(output)
        return output
    def account_source(owner,source):
        output=execution_alias(owner,source,root,build)
        if prepare and output not in prepared:
            command=[sys.executable,'-B',str(TOOLS/'instrument_native_execution.py'),
                '--root',str(root),'--canonical',str(owner),'--source',str(source),
                '--output',str(output),'--manifest',str(output.with_suffix('.execution.json'))]
            subprocess.run(command,check=True,stdout=subprocess.DEVNULL)
            commands.append([printable(Path(value),root,build) if value.startswith('/') else value for value in command])
            prepared.add(output)
        return output
    for owner, aliases in partition_paths.items():
        indices = {int(re.fullmatch(r'body_(\d+)\.cpp', value.name)[1]) for value in aliases if value.name != 'router.cpp'}
        if not indices or indices != set(range(max(indices) + 1)):
            raise ValueError(f'Incomplete configured partition body list: {owner}')
        directory = next(iter(aliases)).parent
        partition_source = specialize_source(owner) if specialized else owner
        if accounting:partition_source=account_source(owner,partition_source)
        if prepare:
            command = [sys.executable, str(TOOLS / 'split_native_translation_unit.py'),
                       '--source', str(partition_source), '--output', str(directory), '--count', str(len(indices))]
            subprocess.run(command, check=True, stdout=subprocess.DEVNULL)
            commands.append([printable(Path(value), root, build) if value.startswith('/') else value for value in command])
    unit_rows = [];embedded_units={}
    for owner, row in sorted(units.items(), key=lambda item: item[1]['source']):
        aliases = []
        receipts = {}
        for path, kind in sorted(row['aliases'].items()):
            if kind == 'specialized':
                specialize_source(owner)
            elif kind=='accounted':
                source=execution_inputs[path]
                if source!=owner:specialize_source(owner)
                account_source(owner,source)
            if not path.exists():
                raise ValueError(f'Configured source alias has no generated bytes/receipt: {path}')
            aliases.append({'path': printable(path, root, build), 'kind': kind, 'sha256': sha(path)})
            if kind=='accounted':
                receipt_path=path.with_suffix('.execution.json');receipt=read_json(receipt_path)
                if (receipt['canonical_sha256']!=sha(owner) or receipt['output_sha256']!=sha(path)
                        or receipt['input_sha256']!=sha(execution_inputs[path])):
                    raise ValueError('Execution accounting receipt differs from canonical/output source')
                receipts[receipt_path]={'path':printable(receipt_path,root,build),'sha256':sha(receipt_path),
                    'schema':receipt['schema'],'canonical_sha256':receipt['canonical_sha256'],
                    'input_sha256':receipt['input_sha256'],'output_sha256':receipt['output_sha256'],
                    'function_count':receipt['function_count'],'literal_sites':receipt['literal_sites']}
                for header in receipt.get('embedded_units',[]):
                    original=root/header['canonical_source']
                    if str(original.resolve()) not in canonical or sha(original)!=header['canonical_sha256']:
                        raise ValueError('Embedded header canonical source differs from accounting receipt')
                    marker=hashlib.sha256(header['canonical_source'].encode()).hexdigest()[:24]
                    content=path.read_text().split('// HALO2_ACCOUNTED_HEADER_BEGIN '+marker+'\n',1)
                    if len(content)!=2:raise ValueError('Embedded header marker missing from configured CPP')
                    body=content[1].split('\n// HALO2_ACCOUNTED_HEADER_END '+marker,1)[0]
                    if hashlib.sha256(body.encode()).hexdigest()!=header['output_sha256']:
                        raise ValueError('Embedded header bytes differ from accounting receipt')
                    item=embedded_units.setdefault(header['canonical_source'],dict(source=header['canonical_source'],
                        source_sha256=header['canonical_sha256'],targets=set(),configured_sources=[],generator_receipts=[],
                        source_relationship='textually embedded in configured CPP; not a direct CMake SOURCES registration'))
                    item['targets'].update(row['targets'])
                    item['configured_sources'].append(dict(path=printable(path,root,build),kind='embedded-accounted-header',
                        container_sha256=sha(path),embedded_body_sha256=header['output_sha256']))
                    item['generator_receipts'].append(dict(path=printable(receipt_path,root,build),sha256=sha(receipt_path),
                        schema=receipt['schema'],canonical_sha256=header['canonical_sha256'],input_sha256=header['input_sha256'],
                        output_sha256=header['output_sha256'],function_count=header['function_count'],literal_sites=header['literal_sites']))
            if kind in ('specialized', 'partitioned','accounted'):
                if kind=='accounted':
                    source=execution_inputs[path];receipt_path=source.with_suffix('.operands.json')
                    if source==owner:continue
                    receipt_kind='specialized'
                else:receipt_kind=kind
                if kind!='accounted':receipt_path = path.with_suffix('.operands.json') if kind == 'specialized' else path.parent / 'manifest.json'
                if receipt_path not in receipts:
                    receipt = read_json(receipt_path)
                    receipt_row = {'path': printable(receipt_path, root, build), 'sha256': sha(receipt_path),
                                   'schema': receipt['schema']}
                    if receipt_kind == 'specialized':
                        if receipt['canonical_sha256'] != hashlib.sha256(owner.read_text().encode()).hexdigest():
                            raise ValueError('Specialization receipt no longer matches canonical source')
                        specialized_path=execution_inputs[path] if kind=='accounted' else path
                        if receipt['specialized_sha256']!=sha(specialized_path):
                            raise ValueError('Specialization receipt differs from actual prepared bytes')
                        receipt_row.update(canonical_sha256=receipt['canonical_sha256'],
                            specialized_sha256=receipt['specialized_sha256'], native_leaves=receipt['native_leaves'],
                            native_binding_groups=native_binding_groups(receipt))
                    else:
                        if not receipt['body_bytes_unchanged']:
                            raise ValueError('Partition receipt does not preserve instruction bodies')
                        if receipt['source_sha256']!=sha(Path(receipt['source'])):
                            raise ValueError('Partition receipt differs from actual input bytes')
                        receipt_row.update(source=printable(Path(receipt['source']), root, build),
                            source_sha256=receipt['source_sha256'], body_count=receipt['body_count'],
                            chunk_count=receipt['chunk_count'], body_bytes_unchanged=True)
                        # Partitioning normally consumes a specialized source;
                        # retain its canonical binding too, not only the split.
                        input_path=Path(receipt['source'])
                        execution=input_path.with_suffix('.execution.json')
                        if execution.exists():
                            execution_row=read_json(execution)
                            if execution_row['canonical_sha256']!=sha(owner) or execution_row['output_sha256']!=sha(input_path):
                                raise ValueError('Partition accounting receipt differs from canonical/input source')
                            receipts[execution]={'path':printable(execution,root,build),'sha256':sha(execution),
                                'schema':execution_row['schema'],'canonical_sha256':execution_row['canonical_sha256'],
                                'input_sha256':execution_row['input_sha256'],'output_sha256':execution_row['output_sha256'],
                                'function_count':execution_row['function_count'],'literal_sites':execution_row['literal_sites']}
                            identified=execution_owner(input_path,canonical,root,build)
                            if not identified:raise ValueError('Unowned partition execution alias')
                            input_path=identified[1]
                            if execution_row['input_sha256']!=sha(input_path):
                                raise ValueError('Partition accounting receipt differs from actual input bytes')
                        operand = input_path.with_suffix('.operands.json')
                        if operand.exists():
                            operand_row = read_json(operand)
                            if operand_row['canonical_sha256'] != hashlib.sha256(owner.read_text().encode()).hexdigest():
                                raise ValueError('Partition specialization receipt differs from canonical source')
                            if operand_row['specialized_sha256']!=sha(input_path):
                                raise ValueError('Partition specialization receipt differs from prepared input bytes')
                            receipts[operand] = {'path': printable(operand, root, build), 'sha256': sha(operand),
                                'schema': operand_row['schema'], 'canonical_sha256': operand_row['canonical_sha256'],
                                'specialized_sha256': operand_row['specialized_sha256'], 'native_leaves': operand_row['native_leaves'],
                                'native_binding_groups': native_binding_groups(operand_row)}
                    receipts[receipt_path] = receipt_row
        unit_rows.append({'source': row['source'], 'source_sha256': row['source_sha256'],
            'targets': sorted(row['targets']), 'configured_sources': aliases,
            'generator_receipts': sorted(receipts.values(), key=lambda value: value['path'])})
    for header in embedded_units.values():
        header['targets']=sorted(header['targets']);unit_rows.append(header)
    unit_rows.sort(key=lambda value:value['source'])
    changed = [str(path.relative_to(root)) for path in canonical.values() if sha(path) != before_hashes[str(path)]]
    if changed:
        raise ValueError('Canonical generated sources changed during export: ' + ', '.join(changed))
    collection_context = collection_context or {}
    configuration_drift = [name for name, value in collection_context.get('cmake_source_hashes', {}).items()
                           if not (root / name).is_file() or sha(root / name) != value]
    if configuration_drift:
        raise ValueError('CMake configuration inputs changed during collection: ' + ', '.join(configuration_drift))
    cmake_inputs = []
    if 'cmakeFiles' in auxiliary:
        for item in auxiliary['cmakeFiles'][0]['inputs']:
            path = resolved_source(item['path'], root, build, item.get('isGenerated', False))
            if path.is_file() and path.is_relative_to(root) and not path.is_relative_to(build):
                scratch = os.environ.get('HALO2_TEST_SCRATCH')
                disposable = bool(scratch and path.is_relative_to(Path(scratch).resolve()))
                cmake_inputs.append({'path': str(path.relative_to(root)), 'sha256_at_export': sha(path),
                                     'disposable_input': disposable})
    report = {'schema': SCHEMA, 'generator': 'tools/audit_native_production_closure.py',
        'generator_sha256': sha(Path(__file__)), 'root_target': root_target,
        'configuration': config, 'host': {'system': platform.system(), 'machine': platform.machine()},
        'requested_context': context or {}, 'cmake': index['cmake'],
        'collection_context': collection_context,
        'configuration_input_drift': configuration_drift,
        'source_root': str(root), 'build_was_disposable': bool(os.environ.get('HALO2_TEST_SCRATCH')),
        'file_api_reply_hashes': {'index': sha(index_path), 'codemodel': sha(model_path),
            **{kind: sha(value[1]) for kind, value in auxiliary.items()}},
        'toolchains': auxiliary.get('toolchains', ({}, None))[0].get('toolchains', []),
        'configuration_options': {name: cache[name] for name in sorted(cache) if name.startswith('HALO2_') or name in (
            'CMAKE_BUILD_TYPE', 'CMAKE_SYSTEM_NAME', 'CMAKE_SYSTEM_PROCESSOR', 'CMAKE_CXX_COMPILER', 'CMAKE_CXX_COMPILER_ID', 'BUILD_TESTING')},
        'target_count': len(target_rows), 'targets': target_rows,
        'configured_generated_units': sum(value['source'].endswith('_generated.cpp')for value in unit_rows),
        'configured_literal_instruction_source_units':len(unit_rows),'canonical_units': unit_rows,
        'canonical_unit_scope':'generated filenames plus all source/header owners of recognized literal instruction calls',
        'generated_units_not_in_configured_closure': sorted(str(path.relative_to(root)) for path in canonical.values() if path not in units and path.name.endswith('_generated.cpp')),
        'nongenerated_literal_sources_not_in_configured_closure':sorted(str(path.relative_to(root))for path in canonical.values()
            if not path.name.endswith('_generated.cpp') and str(path.relative_to(root))not in {value['source']for value in unit_rows}),
        'unresolved_dependency_ids': missing, 'unresolved_generated_sources': unresolved,
        'cmake_inputs_at_export': sorted(cmake_inputs, key=lambda value: value['path']),
        'source_preparation_commands': commands,
        'scope': {'configured_target_dependency_closure': not missing, 'canonical_alias_bindings_complete': not unresolved,
            'game_compiled': False,
            'generated_aliases_prepared': prepare, 'runtime_execution_proved': False,
            'native_only_complete': False, 'other_platforms_or_configurations_qualified': False},
        'limitations': ['Target/source closure is authoritative only for this host, generator, configuration and evaluated CMake options.',
            'CMake dynamic conditions are evaluated for this configure; other platforms/options need their own codemodel.',
            'Configured dependencies/sources do not prove linked object retention, function reachability or runtime execution.',
            'Native specialization entry groups retain authenticated receipt categories; they do not prove admission, child closure or execution.',
            'CMake input hashes are captured at export; source code and the final inventory must be refreshed after concurrent edits.',
            'All raw File API replies, source aliases and build products are disposable; their bounded receipts/hashes are retained.']}
    content=retained_json(report)
    return report, content


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-root', type=Path, required=True)
    parser.add_argument('--build', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--configuration', default='Release')
    parser.add_argument('--root-target', default='halo2_native')
    parser.add_argument('--prepare-aliases', action='store_true')
    parser.add_argument('--target-system', default='')
    parser.add_argument('--target-processor', default='')
    parser.add_argument('--collection-context', type=Path)
    args = parser.parse_args()
    report, content = export(args.source_root, args.build, args.configuration, args.root_target,
        args.prepare_aliases, {'target_system': args.target_system, 'target_processor': args.target_processor},
        read_json(args.collection_context) if args.collection_context else None)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(content)
    print(json.dumps({'configured_generated_units': report['configured_generated_units'],
        'target_count': report['target_count'], 'unresolved_generated_sources': report['unresolved_generated_sources'],
        'output_bytes': len(content.encode())}))


if __name__ == '__main__':
    main()

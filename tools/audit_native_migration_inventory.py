#!/usr/bin/env python3
"""Build a bounded, source-backed inventory of all generated runtime routines.

This is a migration worklist, not native-only or runtime acceptance. Current
CMake registrations and a conservative static target closure identify production
candidates; conditions/variables are not executed. Each retained translation,
literal call and native admission hint is recorded with its source hash.
"""
import argparse
from collections import Counter, defaultdict
import hashlib
import json
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools/perf_4k60'))
from census_common import (DIRECT, DISPATCH, INSTRUCTION, MAPPING,
    REMAPPED_VEHICLE_UNITS, REQUIRED, TARGET, address, callback_image_scope, vehicle_observations)
from specialize_native_operands import LEAVES, digest
from generated_routine_ownership import function_bodies,literal_source_paths,CANONICAL_ONLY_CALLBACK_SOURCES,qualified_dispatch_entries
from native_service_observations import native_service_observation
from native_source_prefixes import authenticated_prefixes,FAMILIES as SOURCE_PREFIX_FAMILIES

SCHEMA = 'native-migration-source-inventory-v1'
PRODUCTION_EVIDENCE = 'docs/native_replacement/evidence/linux-release-production-closure-20261108.json'
MAX_FILE_BYTES = 900_000
INCLUDE = re.compile(r'^#include "([^"]+)"', re.M)
GATE = re.compile(r'\bHALO2_(?:REFERENCE|VERIFY|NATIVE)_[A-Z0-9_]+\b')
CALL = re.compile(r'\b([A-Za-z_]\w*(?:::[A-Za-z_]\w*)*)\s*(?:<[^;{}]*>)?\(')
CM_COMMAND = re.compile(r'(?mi)^[\t ]*(add_library|add_executable|target_sources|target_link_libraries|halo2_partition_native_translation)\s*\(')
CM_TOKEN = re.compile(r'#\[[=]*\[.*?\][=]*\]|#[^\n]*|"(?:\\.|[^"\\])*"|[()]', re.S)
SRC_PATH = re.compile(r'\bsrc/[A-Za-z0-9_./+-]+\.(?:cpp|hpp|h|c)\b')


def sha(data):
    return hashlib.sha256(data).hexdigest()


def json_text(value):
    return json.dumps(value, indent=2, sort_keys=True) + '\n'


def cmake_evidence(root):
    graph, registrations, hashes = defaultdict(set), defaultdict(list), {}
    files = [root / 'CMakeLists.txt', *sorted((root / 'cmake').rglob('*.cmake'))]
    for path in files:
        data = path.read_bytes()
        hashes[str(path.relative_to(root))] = sha(data)
        text = data.decode(errors='replace')
        for match in CM_COMMAND.finditer(text):
            depth, end = 1, None
            for token in CM_TOKEN.finditer(text, match.end()):
                if token[0] == '(':
                    depth += 1
                elif token[0] == ')':
                    depth -= 1
                    if not depth:
                        end = token.start()
                        break
            if end is None:
                continue
            body = re.sub(r'#[^\n]*', '', text[match.end():end])
            words = re.findall(r'[A-Za-z_][A-Za-z0-9_:+.-]*', body)
            if not words or body.lstrip().startswith('${'):
                continue
            target = words[0]
            site = {'file': str(path.relative_to(root)),
                    'line': text.count('\n', 0, match.start()) + 1,
                    'command': match[1].lower(), 'target': target}
            if match[1].lower() == 'target_link_libraries':
                graph[target].update(word for word in words[1:] if word.startswith('halo2_'))
            else:
                for source in SRC_PATH.findall(body):
                    registrations[source].append(site)
    reachable, pending = set(), ['halo2_native']
    while pending:
        target = pending.pop()
        if target not in reachable:
            reachable.add(target)
            pending.extend(graph.get(target, ()))
    return registrations, reachable, hashes


def resolved_include(root, source, name):
    for candidate in (source.parent / name, root / 'src' / name, root / 'include' / name):
        if candidate.is_file():
            return candidate.resolve()
    return None


def unit_inventory(root, path, registrations, reachable, remap):
    data = path.read_bytes()
    text = data.decode()
    relative = str(path.relative_to(root))
    image_scope=callback_image_scope(text)
    if path.name in REMAPPED_VEHICLE_UNITS and remap:image_scope='tu15-observation-remap-or-map-pack11'
    observed_image='map-pack11' if image_scope=='map-pack11' else 'tu15'
    dispatch_bindings=({image:qualified_dispatch_entries(text,image)for image in ('tu15','map-pack11')}
        if relative not in CANONICAL_ONLY_CALLBACK_SOURCES else {})
    dispatch_evidence={image:{key:value for key,value in next(iter(entries.values())).items()if key!='entry'}
        for image,entries in dispatch_bindings.items()if entries}
    mapping = {}
    for line in text.splitlines():
        if line.startswith('std::uint32_t selected('):
            mapping.update((int(a, 16), int(b, 16)) for a, b in MAPPING.findall(line))
    admission_sources = {}
    gates = set(GATE.findall(text))
    for include in INCLUDE.findall(text):
        include_path = resolved_include(root, path, include)
        if include_path:
            include_data = include_path.read_bytes()
            include_gates = sorted(set(GATE.findall(include_data.decode(errors='replace'))))
            if include_gates:
                admission_sources[str(include_path.relative_to(root))] = {
                    'sha256': sha(include_data), 'gates': include_gates}
                gates.update(include_gates)
    sites = registrations.get(relative, [])
    production_targets = sorted({site['target'] for site in sites if site['target'] in reachable})
    classification = ('static_production_candidate' if production_targets else
                      'registered_outside_static_production_closure' if sites else
                      'no_literal_cmake_registration_found_requires_audit')
    source_prefixes=authenticated_prefixes(root,path,text)
    routines = []
    for match, body in function_bodies(text):
        symbol, state_type = match[1], match[2]
        line = text.count('\n', 0, match.start()) + 1
        observed = []
        for callback in INSTRUCTION.finditer(body):
            value = address(callback[1], mapping, observed_image)
            if value is not None and path.name in REMAPPED_VEHICLE_UNITS:
                value = remap.get(value)
            if value is not None:
                observed.append(value)
        direct = sorted(set(DIRECT.findall(body)))
        targets = set()
        for statement in body.splitlines():
            expressions = DISPATCH.findall(statement) + REQUIRED.findall(statement)
            if 'dispatch(s,target,' in statement:
                expressions.extend(TARGET.findall(statement))
            for expression in expressions:
                target = address(expression, mapping, observed_image)
                if target is not None:
                    targets.add(target)
        prefix = body.split('s.services.instruction', 1)[0]
        prefix_calls = sorted({name for name in CALL.findall(prefix)
                              if not name.startswith('run_') and name not in {'if', 'while', 'switch', 'for', 'return'}})
        hand_written = '\n'.join(statement for statement in body.splitlines()
                                if 's.services.instruction' not in statement and 's.instruction_literal' not in statement)
        native_hints = sorted({name for name in CALL.findall(hand_written)
                              if any(part in name.lower() for part in ('native', 'bridge', 'handoff', 'kernel', '::reference'))})
        specialization = None
        if symbol in LEAVES:
            expected, kernel, final_pc = LEAVES[symbol]
            if digest(body.replace(state_type + ' &s', 'STATE &s')) == expected:
                specialization = {'kernel': kernel, 'authenticated_body_sha256': expected,
                                  'condition': 'build-time literal-cursor admission; reference route retained'}
        status = ('retained_translation_with_hash_bound_native_leaf' if specialization else
                  'retained_translation_with_native_admission_hint_requires_audit' if observed and native_hints else
                  'retained_translation_no_native_admission_proven' if observed else
                  'no_callback_body_requires_audit')
        if not path.name.endswith('_generated.cpp') and observed:
            bookkeeping=bool(re.search(r'\b\w+\.(?:gp\s*\[|reg\s*\(|set\s*\(|flags\.)|iterator_flags\(',body))
            status=('literal_instruction_shape_candidate_requires_native_ownership_review' if len(observed)>=3 and bookkeeping
                else 'literal_observer_boundary_requires_semantic_review')
        source_prefix=source_prefixes.get(symbol)
        if source_prefix and source_prefix['authenticated']:status='hash_authenticated_source_native_prefix_with_retained_reference'
        if native_service_observation(relative,sha(data),symbol):status='hash_qualified_native_service_observation_boundary'
        routine = {'symbol': symbol, 'line': line, 'body_sha256': sha(body.encode()),
                   'function_name':match.function_name,
                   'symbol_kind':'address-named' if match.function_name.startswith('run_') and re.fullmatch('[0-9a-f]+',symbol) else 'descriptive',
                   'state_type': state_type, 'callback_image_scope': image_scope,
                   'observed_entry': f'{observed[0]:x}' if observed else None,
                   'observed_entry_semantics': 'first literal callback; function-entry identity requires independent qualification',
                   'observed_tu15_entry': f'{observed[0]:x}' if observed and observed_image=='tu15' and relative not in CANONICAL_ONLY_CALLBACK_SOURCES else None,
                   'observed_map_pack11_entry': f'{observed[0]:x}' if observed and observed_image=='map-pack11' else None,
                   'instruction_callback_sites': len(observed), 'route_status': status,
                   'direct_run_symbols': direct,
                   'literal_dispatch_required_targets': [f'{target:x}' for target in sorted(targets)]}
        owners={image:f'{entries[symbol]["entry"]:x}'for image,entries in dispatch_bindings.items()if symbol in entries}
        if owners:routine['internal_dispatch_owner_entries']=owners
        if relative in CANONICAL_ONLY_CALLBACK_SOURCES:
            routine.update(canonical_callback_entry=f'{observed[0]:x}'if observed else None,
                callback_address_namespace='canonical source PCs; owning production remapper not authenticated',
                actual_tu15_function_entry=None,actual_tu15_identity_qualified=False)
        if prefix_calls:
            routine['admission_prefix_calls'] = prefix_calls
        if native_hints:
            routine['native_admission_hints'] = native_hints
        if source_prefix:
            routine['source_native_prefix_evidence']=source_prefix
        if specialization:
            routine['hash_bound_specialization'] = specialization
        routines.append(routine)
    return {'schema': SCHEMA, 'source': relative, 'source_sha256': sha(data),
            'source_bytes': len(data), 'production_scope': classification,
            'callback_image_scope':image_scope,
            'internal_dispatch_identity_evidence':dispatch_evidence,
            'production_candidate_targets': production_targets, 'cmake_registration_sites': sites,
            'reference_verification_gate_names': sorted(gates), 'gate_sources': admission_sources,
            'routine_count': len(routines), 'routines': routines}


def configured_production_evidence(root, path=None):
    if path is None:
        saved = root / 'docs/native_replacement/INVENTORY.json'
        if saved.is_file():
            prior = json.loads(saved.read_text()).get('configured_production_evidence')
            if prior and prior.get('path'):
                path = root / prior['path']
    path = Path(path) if path is not None else root / PRODUCTION_EVIDENCE
    if not path.is_file():
        return None, {}
    evidence = json.loads(path.read_text())
    if evidence.get('schema') != 'native-production-cmake-closure-v1':
        raise ValueError('Unsupported configured production closure evidence')
    if Path(evidence['source_root']).resolve() != root.resolve():
        raise ValueError('Configured production evidence belongs to another source root')
    configuration = dict(evidence.get('collection_context', {}).get('cmake_source_hashes', {}))
    configuration.update((value['path'], value['sha256_at_export']) for value in evidence.get('cmake_inputs_at_export', [])
                    if not value.get('disposable_input'))
    canonical = {value['source']: value['source_sha256'] for value in evidence['canonical_units']}
    tools = evidence.get('collection_context', {}).get('tool_hashes', {})
    def changed(expected):
        return sorted(name for name, value in expected.items()
                      if not (root / name).is_file() or sha((root / name).read_bytes()) != value)
    config_drift, source_drift, tool_drift = changed(configuration), changed(canonical), changed(tools)
    drift = sorted(set(config_drift + source_drift + tool_drift))
    return {'path': str(path.relative_to(root)) if path.is_relative_to(root) else str(path),
        'sha256': sha(path.read_bytes()), 'current_source_and_configuration': not drift,
        'configured_graph_current': not config_drift,
        'canonical_alias_bindings_current': not source_drift and not tool_drift,
        'configuration_drift': config_drift, 'canonical_source_drift': source_drift,
        'source_preparation_tool_drift': tool_drift,
        'source_or_configuration_drift': drift, 'host': evidence['host'],
        'configuration': evidence['configuration'], 'root_target': evidence['root_target'],
        'target_count': evidence['target_count'], 'configured_generated_units': evidence['configured_generated_units'],
        'canonical_unit_scope':evidence.get('canonical_unit_scope','generated filenames only'),
        'configured_literal_instruction_source_units':evidence.get('configured_literal_instruction_source_units',evidence['configured_generated_units']),
        'scope': evidence['scope']}, {value['source']: value for value in evidence['canonical_units']}


def inventory(root, production_closure=None):
    registrations, reachable, cmake_hashes = cmake_evidence(root)
    production, configured_units = configured_production_evidence(root, production_closure)
    remap = vehicle_observations(root / 'src')
    outputs, units = {}, []
    totals, subsystems, statuses = Counter(), defaultdict(Counter), Counter()
    source_paths=list(literal_source_paths(root))
    include_owners=defaultdict(list)
    for parent in source_paths:
        targets=sorted({site['target']for site in registrations.get(str(parent.relative_to(root)),[])if site['target']in reachable})
        if not targets:continue
        pending=[parent];seen=set()
        while pending:
            source=pending.pop()
            if source in seen:continue
            seen.add(source);text=source.read_text()
            for include in INCLUDE.finditer(text):
                child=resolved_include(root,source,include[1])
                if not child or not child.is_relative_to(root/'src'):continue
                include_owners[str(child.relative_to(root))].append(dict(including_source=str(source.relative_to(root)),
                    line=text.count('\n',0,include.start())+1,registered_source=str(parent.relative_to(root)),
                    production_candidate_targets=targets,preprocessor_admission_qualified=False))
                pending.append(child)
    for path in source_paths:
        unit = unit_inventory(root, path, registrations, reachable, remap)
        inclusion=include_owners.get(unit['source'],[])
        if inclusion:
            unit['source_inclusion_candidates']=inclusion
            if not unit['production_candidate_targets']:
                unit['production_candidate_targets']=sorted({target for row in inclusion for target in row['production_candidate_targets']})
                unit['production_scope']='static_production_include_candidate'
        filename_scope='generated' if path.name.endswith('_generated.cpp') else 'nongenerated'
        unit['source_filename_scope']=filename_scope
        if production:
            unit['configured_production_scope'] = ('included' if unit['source'] in configured_units else
                ('not_in_generated_only_evidence_scope_requires_audit' if production['canonical_unit_scope']=='generated filenames only' else
                 'not_in_configured_prepared_source_or_include_closure_requires_audit') if filename_scope=='nongenerated' else 'excluded')
            unit['configured_production_target_ids'] = configured_units.get(unit['source'], {}).get('targets', [])
        rows = unit.pop('routines')
        parts, chunk = [], []
        for row in rows:
            if chunk and len(json_text({**unit, 'routines': chunk + [row]}).encode()) > MAX_FILE_BYTES:
                parts.append(chunk)
                chunk = []
            chunk.append(row)
        parts.append(chunk)
        tables = []
        stem = str(path.relative_to(root / 'src')).removesuffix('.cpp').replace('/', '__')
        for number, part in enumerate(parts):
            name = f'inventory/{stem}.{number:02d}.json'
            content = json_text({**unit, 'part': number, 'routines': part})
            outputs[name] = content
            tables.append({'path': name, 'sha256': sha(content.encode()), 'bytes': len(content.encode()), 'routines': len(part)})
        subsystem = str(path.relative_to(root / 'src')).split('/')[0]
        counts = Counter(row['route_status'] for row in rows)
        totals[filename_scope+'_units'] += 1
        totals['source_units'] += 1
        totals['function_bodies'] += len(rows)
        totals['address_named_function_bodies'] += sum(row['symbol_kind']=='address-named' for row in rows)
        totals['descriptive_function_bodies'] += sum(row['symbol_kind']=='descriptive' for row in rows)
        totals[unit['production_scope']] += 1
        subsystems[subsystem][filename_scope+'_units'] += 1
        subsystems[subsystem]['function_bodies'] += len(rows)
        if unit['production_scope'] in {'static_production_candidate','static_production_include_candidate'}:
            totals['production_candidate_function_bodies'] += len(rows)
            subsystems[subsystem]['production_candidate_function_bodies'] += len(rows)
        if unit['source'] in configured_units:
            totals['configured_production_units'] += 1
            totals['configured_production_function_bodies'] += len(rows)
            subsystems[subsystem]['configured_production_function_bodies'] += len(rows)
        statuses.update(counts)
        units.append({key: unit[key] for key in ['source', 'source_sha256', 'source_bytes', 'production_scope','source_filename_scope',
                                                'production_candidate_targets', 'routine_count','callback_image_scope']} |
                     {'route_counts': dict(counts), 'tables': tables} |
                     ({'configured_production_scope': unit['configured_production_scope'],
                       'configured_production_target_ids': unit['configured_production_target_ids']} if production else {}))
    specializer = ROOT / 'tools/specialize_native_operands.py'
    specialization_sources = {'tools/specialize_native_operands.py': {'sha256': sha(specializer.read_bytes())}}
    for name in sorted(set(re.findall(r'#include "([^"]+)"', specializer.read_text()))):
        header = ROOT / 'src' / name
        if header.is_file():
            header_data = header.read_bytes()
            specialization_sources[str(header.relative_to(ROOT))] = {
                'sha256': sha(header_data),
                'reference_verification_gate_names': sorted(set(GATE.findall(header_data.decode())))}
    manifest = {'schema': SCHEMA, 'generator': 'tools/audit_native_migration_inventory.py',
        'generator_sha256': sha(Path(__file__).read_bytes()),
        'source_native_prefix_catalog': {'path':'tools/native_source_prefixes.py','sha256':sha((ROOT/'tools/native_source_prefixes.py').read_bytes()),
            'entry_symbols':sorted({row[0]for row in SOURCE_PREFIX_FAMILIES}),
            'families':[{'entry_symbol':entry,'binder':'tools/'+name+'.py'}for entry,name,_ in SOURCE_PREFIX_FAMILIES],
            'scope':'Listed exact source-level admission prefixes only; uncatalogued hooks retain audit hints. Runtime admission and dependency closure are not proved.'},
        'scope': 'All src/**/*_generated.cpp units plus literal-instruction owners in src C/C++ files and headers, regardless of filename; unresolved/nonproduction registrations remain visible.',
        'production_root': 'halo2_native', 'production_static_target_candidates': sorted(reachable),
        'configured_production_evidence': production,
        'totals': dict(totals), 'route_status_totals': dict(statuses),
        'subsystem_totals': {name: dict(counts) for name, counts in sorted(subsystems.items())},
        'acceptance': {'native_only_complete': False, 'runtime_execution_census_recorded': False,
                       'frame_parity_recorded': False, 'launcher_runtime_promoted': False},
        'limitations': [
            'Current CMake source/target registration is static evidence; option conditions, generator expressions and variable-produced sources are not evaluated.',
            'Retained translated body existence is not evidence that it executes in production.',
            'Native hook/admission hints do not prove full function or dependency closure and are not counted as completed migrations.',
            'Hash-bound build-time native leaves retain their original oracle and explicit admission gates.',
            'Authenticated source-native prefixes supplement build-local receipts and preserve retained-reference bodies; owning binder checks are static admission evidence, not runtime or child-closure proof.',
            'Routine identities are source-qualified; duplicate observed addresses across units are not deduplicated into an assumed runtime owner.',
            'Observed entries are callback tags, not independently qualified function entries; internal_dispatch_owner_entries use complete simple dispatcher/resolver grammars and establish conditional static targets only.',
            'Explicit state-constructor image evidence distinguishes fixed MapPack11 callbacks from TU1.5 callbacks. Unknown or image-selected owners remain unresolved; image labels do not exclude compiled production variants.',
            'Literal dispatch/required targets and direct calls are static references; indirect runtime targets require live coverage.',
            'Nongenerated literal sites can be semantic observer boundaries; bookkeeping/native-admission hints do not by themselves classify them as translated execution.',
        ], 'cmake_source_hashes': cmake_hashes,
        'build_time_native_specialization_sources': specialization_sources, 'units': units}
    outputs['INVENTORY.json'] = json_text(manifest)
    for name, content in outputs.items():
        if len(content.encode()) > 1_048_576:
            raise ValueError(f'Inventory output exceeds bounded1MiB contract: {name}')
    return manifest, outputs


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=ROOT)
    parser.add_argument('--output', type=Path, default=ROOT / 'docs/native_replacement')
    parser.add_argument('--check', action='store_true', help='verify saved inventory is current without changing it')
    parser.add_argument('--production-closure', type=Path,
                        help='bounded CMake File API configured-production closure evidence')
    args = parser.parse_args()
    manifest, outputs = inventory(args.root.resolve(), args.production_closure)
    stale = []
    for relative, content in outputs.items():
        path = args.output / relative
        if args.check:
            if not path.exists() or path.read_text() != content:
                stale.append(relative)
        else:
            path.parent.mkdir(parents=True, exist_ok=True)
            if path.exists() and json.loads(path.read_text()).get('schema') != SCHEMA:
                raise ValueError(f'Refusing to overwrite unowned document: {path}')
            path.write_text(content)
    summary = {'totals': manifest['totals'], 'subsystems': manifest['subsystem_totals'],
               'route_status_totals': manifest['route_status_totals'],
               'output_files': len(outputs), 'output_bytes': sum(len(value.encode()) for value in outputs.values()),
               'stale_files': stale, 'native_only_complete': False}
    print(json.dumps(summary, indent=2))
    return 1 if stale else 0


if __name__ == '__main__':
    raise SystemExit(main())

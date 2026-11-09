#!/usr/bin/env python3
"""Build a bounded source-qualified work queue from existing accounted captures.

Counts are literal statement observations, including startup/teardown. They are
neither timings nor dynamic caller attribution, and do not prove native closure.
"""
import argparse
from collections import Counter,defaultdict
import hashlib
import json
from pathlib import Path
import re
import sys

ROOT=Path(__file__).resolve().parents[1]
KNOWN_UNINSTRUMENTED=[
    'src/gameplay/animation/unit_animation_selection_runtime.cpp',
    'src/gameplay/weapons/unit_pickup_runtime.cpp',
    'src/gameplay/weapons/unit_weapon_routing.cpp',
]
sys.path.insert(0,str(ROOT/'tools'))
from generated_routine_ownership import function_bodies,CANONICAL_ONLY_CALLBACK_SOURCES,qualified_dispatch_entries
from native_execution_report import analyze
from native_service_observations import native_service_observation
from perf_4k60.census_common import (DIRECT,DISPATCH,INSTRUCTION,MAPPING,REQUIRED,
    REMAPPED_VEHICLE_UNITS,address,vehicle_observations)


def digest(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def metadata(root,source,symbols,image,frozen_sha):
    path=root/source
    if not path.is_file():return {}
    data=path.read_bytes();text=data.decode();current_sha=hashlib.sha256(data).hexdigest()
    mapping={int(a,16):int(b,16)for line in text.splitlines()if line.startswith('std::uint32_t selected(')for a,b in MAPPING.findall(line)}
    remap=vehicle_observations(root/'src')if path.name in REMAPPED_VEHICLE_UNITS and image=='tu15'else None
    qualified=qualified_dispatch_entries(text,image)
    result={}
    for signature,body in function_bodies(text):
        if signature[1]not in symbols:continue
        first=next(INSTRUCTION.finditer(body),None)
        entry=address(first[1],mapping,image)if first and image in ('tu15','map-pack11')else None
        if remap and entry is not None:entry=remap.get(entry)
        canonical_entry=entry;identity=qualified.get(signature[1])
        explicit=bool(first and any(name in first[1]for name in ('world_observation_pc','damage_observation_pc')))
        if identity:entry=identity['entry']
        elif not explicit and remap is None:entry=None
        if source in CANONICAL_ONLY_CALLBACK_SOURCES:entry=None;identity=None;explicit=False
        targets=[]
        for expression in DISPATCH.findall(body)+REQUIRED.findall(body):
            value=address(expression,mapping,image)
            if value is not None:targets.append(f'{value:x}')
        direct=sorted(set(DIRECT.findall(body)))
        hints=any(part in body for part in ('::bridge(', '::bridge<','::reference()', 'native_main_heap_','native_pool_allocator_','native_array_allocator_'))
        result[signature[1]]=dict(function=signature.function_name,line=text.count('\n',0,signature.start())+1,
            current_unit_sha256=current_sha,current_body_sha256=hashlib.sha256(body.encode()).hexdigest(),
            unit_matches_frozen=current_sha==frozen_sha,
            actual_entry=f'{entry:x}'if entry is not None else None,
            canonical_callback_entry=f'{canonical_entry:x}'if canonical_entry is not None else None,
            entry_identity_qualified=entry is not None,
            entry_identity_evidence=identity or ('explicit callback remapping interface'if explicit or remap is not None else None),
            identity_evidence='canonical-only source PCs; owning production remapper not authenticated'if source in CANONICAL_ONLY_CALLBACK_SOURCES else
                'frozen-unit hash confirmed'if current_sha==frozen_sha else 'current-source audit; unit differs from frozen capture',
            body_lines=len(body.splitlines()),literal_sites=len(list(INSTRUCTION.finditer(body))),
            static_direct_run_symbols=direct,static_dispatch_required_targets=sorted(set(targets)),
            unresolved_dispatch=(len(DISPATCH.findall(body))+len(REQUIRED.findall(body))<
                len(re.findall(r'\bdispatch\(s,|\bs\.required\(',body))),
            current_native_admission_hint=hints,native_closure_proved=False)
    return result


def aggregate(cohort_path,root,top=80):
    cohort_path=cohort_path.resolve();directory=cohort_path.parent;root=root.resolve()
    cohort=json.loads(cohort_path.read_text())
    if not(cohort.get('passed')and cohort.get('execution_accounting_enabled')and cohort.get('bounded_runtime_accounting_recorded')):
        raise ValueError('A passed runtime accounting cohort is required')
    pins_path=directory/'source-hashes.json';pins=json.loads(pins_path.read_text())
    if digest(pins_path)!=cohort['source_inventory_sha256']:raise ValueError('Frozen source manifest hash differs')
    coverage_path=directory/'cmake/evidence/native-execution-coverage.json';coverage=json.loads(coverage_path.read_text())
    if coverage.get('canonical_source_drift'):raise ValueError('Captured alias coverage has canonical source drift')
    allowed={(row['source'],row['canonical_sha256'],row['input_sha256'])for row in coverage['aliases']}
    missing_known=[source for source in KNOWN_UNINSTRUMENTED if source not in {row['source']for row in coverage['aliases']}]
    runs=cohort['accounting_runs'];labels=[run['label']for run in runs]
    if len(labels)!=len(set(labels))or not labels:raise ValueError('Distinct nonempty workload labels are required')
    records={};units={};run_rows=[];totals=Counter();input_variants=defaultdict(set)
    for index,run in enumerate(runs):
        if not run.get('capture',{}).get('valid'):raise ValueError('Invalid capture in accounting cohort')
        path=directory/Path(run['runtime_report']).name
        if path.stat().st_size>1_048_576 or digest(path)!=run['runtime_report_sha256']:raise ValueError('Runtime report size/hash differs')
        raw=json.loads(path.read_text());analysis=analyze(raw)
        if raw['active_scopes']:raise ValueError('Completed cohort contains active function scopes')
        run_total=Counter()
        for values in raw['rows']:
            row=dict(zip(raw['row_fields'],values));unit=raw['units'][row['unit']];image=raw['image_names'][row['image']]
            identity=tuple(unit[:3])
            if identity not in allowed or pins.get(unit[0])!=unit[1]:raise ValueError('Runtime identity has no frozen alias/source binding')
            unit_key=(unit[0],unit[1]);units[unit_key]=None
            key=(*unit_key,row['symbol'],image)
            record=records.setdefault(key,dict(counts=Counter(),runs=[[0,0,0,0,0]for _ in runs],literal_sites=set(),forwarders=set()))
            record['literal_sites'].add(row['literal_sites']);record['forwarders'].add(row['dynamic_forwarders'])
            input_variants[key].add(unit[2])
            values={name:row[name]for name in ('entries','body_entries','literal_statements','returns','unwinds','returns_without_body','unwinds_without_body')}
            record['counts'].update(values);run_total.update(values)
            for n,name in enumerate(('entries','body_entries','literal_statements','returns_without_body','unwinds')):record['runs'][index][n]+=row[name]
        if run_total!=Counter(analysis['totals'])-Counter({'active':0}):
            # Counter equality treats missing zeros equally; all nonzero sums must agree.
            if any(run_total[k]!=v for k,v in analysis['totals'].items()):raise ValueError('Aggregated counts differ from validated runtime totals')
        totals.update(run_total)
        run_rows.append(dict(label=run['label'],runtime_report=path.name,sha256=digest(path),
            input_script_sha256=run.get('input_script_sha256'),**dict(run_total),
            fallback_rows=analysis['literal_fallback_rows'],entered_rows=analysis['source_qualified_rows']))
    audit={}
    for source,unit_sha in units:
        for image in {key[3]for key in records if key[:2]==(source,unit_sha)}:
            symbols={key[2]for key in records if key[:2]==(source,unit_sha)and key[3]==image}
            for symbol,row in metadata(root,source,symbols,image,unit_sha).items():audit[source,unit_sha,symbol,image]=row
    order=sorted(records,key=lambda key:(-records[key]['counts']['literal_statements'],key))
    queue=[]
    for key in order:
        record=records[key]
        if not record['counts']['body_entries']or native_service_observation(key[0],key[1],key[2]):continue
        source,unit_sha,symbol,image=key;details=audit.get(key,{})
        queue.append(dict(rank=len(queue)+1,source=source,canonical_unit_sha256=unit_sha,
            canonical_symbol=symbol,image=image,input_sha256_variants=sorted(input_variants[key]),
            counts=dict(record['counts']),per_run_literal_statements=[row[2]for row in record['runs']],
            observed_literal_sites=sorted(record['literal_sites']),dynamic_forwarders=sorted(record['forwarders']),
            source_audit=details,share_of_accounted_statements=record['counts']['literal_statements']/totals['literal_statements']))
        if len(queue)>=top:break
    unit_keys=sorted(units);unit_numbers={key:n for n,key in enumerate(unit_keys)}
    table=[]
    for key in order:
        r=records[key];c=r['counts'];table.append([unit_numbers[key[:2]],key[2],key[3],
            *[c[name]for name in ('entries','body_entries','literal_statements','returns','unwinds','returns_without_body','unwinds_without_body')],
            r['runs'],sorted(input_variants[key])])
    small=[]
    for key in order:
        r=records[key];a=audit.get(key,{})
        if r['counts']['body_entries']and not native_service_observation(key[0],key[1],key[2])and a and a['body_lines']<=130 and a['literal_sites']<=96:
            small.append(dict(source=key[0],canonical_symbol=key[2],actual_entry=a['actual_entry'],
                literal_statements=r['counts']['literal_statements'],body_lines=a['body_lines'],
                static_direct_run_symbols=a['static_direct_run_symbols'],static_dispatch_required_targets=a['static_dispatch_required_targets'],
                identity_evidence=a['identity_evidence'],current_native_admission_hint=a['current_native_admission_hint']))
        if len(small)>=30:break
    return dict(schema='native-execution-hot-queue-v1',generator_sha256=digest(Path(__file__)),
        cohort_report_sha256=digest(cohort_path),source_manifest_sha256=digest(pins_path),coverage_sha256=digest(coverage_path),
        runtime_sha256=cohort['runtime_sha256'],workloads=run_rows,totals=dict(totals),
        source_qualified_functions=len(records),input_qualified_function_variants=sum(len(x)for x in input_variants.values()),
        fallback_functions=sum(bool(r['counts']['body_entries'])and not native_service_observation(key[0],key[1],key[2])for key,r in records.items()),
        native_service_observation_functions=sum(bool(r['counts']['body_entries'])and native_service_observation(key[0],key[1],key[2])for key,r in records.items()),
        no_literal_body_functions=sum(not r['counts']['body_entries']for r in records.values()),
        mixed_body_and_no_body_functions=sum(bool(r['counts']['body_entries']and r['counts']['returns_without_body'])for r in records.values()),
        units=[[source,unit_sha]for source,unit_sha in unit_keys],
        row_fields=['unit','canonical_symbol','image','entries','body_entries','literal_statements','returns','unwinds','returns_without_body','unwinds_without_body','per_run','input_sha256_variants'],
        per_run_fields=['entries','body_entries','literal_statements','returns_without_body','unwinds'],rows=table,
        hot_queue=queue,small_body_candidates=small,
        known_uninstrumented_production_sources=missing_known,
        current_source_drift=sorted({key[0]for key,row in audit.items()if not row['unit_matches_frozen']}),
        native_closure_proved=False,performance_qualified=False,
        limitations=['Counts include startup/teardown and are literal statement observations, not elapsed time or steady-state tick costs.',
            'Ranks sum eight workloads with equal workload weights; no dynamic caller/root cost is inferred.',
            'Input-alias variants remain recorded even when combined into one source/image/function row.',
            'A completed return without literal body execution does not prove native function/dependency closure.',
            'Exact-hash native service observation bodies remain in raw rows/totals but are separated from the migration queue; they do not imply zero callbacks or full closure.',
            'Source audits distinguish matching frozen units from later current-source edits; current-only identities/call edges are not retroactive frozen-source proof.',
            'First callback labels are not actual function entries; actual_entry requires an exact simple internal dispatcher or an explicit observation remapping interface.',
            'Small bodies are implementation leads, not proven leaves; indirect and nongenerated services require separate audits.',
            'Unentered production variants including MapPack11 and nongenerated Effect emulation remain outside this accounted runtime coverage.',
            *(['Compiled nongenerated animation selection, unit pickup or weapon routing owners were not instrumented; see known_uninstrumented_production_sources for the exact omissions.']if missing_known else []),
            'The captured runtime predates subsequent native ports; this queue does not establish current production fallback or launcher promotion.'])


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--cohort',type=Path,required=True)
    p.add_argument('--source-root',type=Path,default=ROOT);p.add_argument('--output',type=Path,required=True)
    p.add_argument('--top',type=int,default=80);args=p.parse_args()
    report=aggregate(args.cohort,args.source_root,args.top)
    encoded=json.dumps(report,separators=(',',':'),sort_keys=True)+'\n'
    if len(encoded.encode())>1_048_576:raise ValueError('Hot queue exceeds bounded1MiB report limit')
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(encoded)
    print(json.dumps({key:report[key]for key in ('source_qualified_functions','input_qualified_function_variants','fallback_functions','no_literal_body_functions','mixed_body_and_no_body_functions','totals','current_source_drift')}))
    return 0


if __name__=='__main__':raise SystemExit(main())

#!/usr/bin/env python3
"""Validate bounded runtime counts or summarize disposable instrumentation coverage."""
import argparse
from collections import Counter,defaultdict
import hashlib
import json
from pathlib import Path
from native_service_observations import native_service_observation
from native_metadata import digest as metadata_digest,load as load_metadata


def digest(path):return metadata_digest(path)


def analyze(data):
    if data.get('schema')!='native-execution-accounting-v1':raise ValueError('Unsupported execution report')
    if data.get('omitted_records'):raise ValueError('Runtime report omitted function records')
    if data.get('registration_failures'):raise ValueError('Runtime function registration failed')
    if data.get('orphan_literal_events'):raise ValueError('Literal instruction event has no live owning activation')
    fields=data['row_fields'];units=data['units'];images=data['image_names']
    totals=Counter();by_image=defaultdict(Counter);fallbacks=[];unresolved=[];native_observers=[];observer_totals=Counter()
    for values in data['rows']:
        if len(values)!=len(fields):raise ValueError('Execution row has wrong field count')
        row=dict(zip(fields,values));unit=units[row['unit']];image=images[row['image']]
        counts={key:row[key] for key in ('entries','body_entries','literal_statements','returns','unwinds',
                                        'returns_without_body','unwinds_without_body','active')}
        if any(not isinstance(value,int) or value<0 for value in counts.values()):raise ValueError('Invalid execution counter')
        if row['entries']!=row['returns']+row['unwinds']+row['active']:
            raise ValueError('Inconsistent or concurrently sampled activation counts')
        if not (row['body_entries']<=row['entries'] and row['literal_statements']>=row['body_entries'] and
                row['returns_without_body']<=row['returns'] and row['unwinds_without_body']<=row['unwinds']):
            raise ValueError('Inconsistent literal/return counter attribution')
        completed_body=row['returns']-row['returns_without_body']+row['unwinds']-row['unwinds_without_body']
        if not completed_body<=row['body_entries']<=completed_body+row['active']:
            raise ValueError('Body-entry ownership does not agree with completed activations')
        totals.update(counts);by_image[image].update(counts)
        identity=dict(source=unit[0],canonical_sha256=unit[1],input_sha256=unit[2],symbol=row['symbol'],image=image)
        if row['body_entries']:
            if native_service_observation(unit[0],unit[1],row['symbol']):native_observers.append({**identity,**counts});observer_totals.update(counts)
            else:fallbacks.append({**identity,**counts})
        if row['dynamic_forwarders'] or not row['literal_sites'] or image=='unknown' or row['active']:
            unresolved.append({**identity,'dynamic_forwarders':row['dynamic_forwarders'],'literal_sites':row['literal_sites'],'active':row['active']})
    if totals['active']!=data['active_scopes']:raise ValueError('Active scope total differs from report')
    return dict(schema='native-execution-analysis-v1',totals=dict(totals),
        image_totals={key:dict(value) for key,value in sorted(by_image.items())},
        source_qualified_rows=len(data['rows']),units_entered=len(units),
        literal_fallback_rows=len(fallbacks),unresolved_rows=len(unresolved),
        native_service_observation_rows=len(native_observers),native_service_observation_totals=dict(observer_totals),
        native_service_observation_examples=native_observers[:30],
        top_literal_fallbacks=sorted(fallbacks,key=lambda row:-row['literal_statements'])[:60],
        unresolved_examples=unresolved[:30],native_closure_proved=False,
        limitations=['Completed returns without literal body sites prove that bounded route only, not whole native dependency closure.',
                     'Functions not entered, nonliteral native services and uninstrumented includes remain outside execution coverage.',
                     'body_entries records literal observation-site execution; nongenerated semantic observer boundaries must not be labeled translated emulation without source review.',
                     'Hash-qualified native service observation rows are reported separately; original counters/totals are unchanged and no dependency closure is inferred.',
                     'Dynamic instruction forwarders and bodies with no literal sites require separate ownership audit.',
                     'Accounting changes timing; use a separate non-accounted cohort for performance acceptance.'])


def coverage(directory,root):
    root=root.resolve();rows=[];units={};drift=[]
    for path in sorted(directory.rglob('*.execution.json')):
        data=json.loads(path.read_text())
        if data.get('schema')!='native-execution-instrumentation-v1':raise ValueError('Unsupported alias receipt')
        for unit in (data,*data.get('embedded_units',[])):
            canonical=root/unit['canonical_source']
            if not canonical.is_file() or digest(canonical)!=unit['canonical_sha256']:drift.append(unit['canonical_source'])
            row=dict(source=unit['canonical_source'],canonical_sha256=unit['canonical_sha256'],input_sha256=unit['input_sha256'],
                     output_sha256=unit['output_sha256'],function_count=unit['function_count'],literal_sites=unit['literal_sites'],
                     callback_image_scope=unit['callback_image_scope'],manifest_sha256=digest(path),
                     source_filename_scope=unit.get('source_filename_scope','generated'),
                     embedded_in=unit.get('embedded_in'),
                     dynamic_instruction_forwarders=sum(len(item['dynamic_instruction_forwarders']) for item in unit['functions']))
            units.setdefault(row['source'],row['function_count']);rows.append(row)
    if not rows:raise ValueError('No instrumentation manifests found')
    return dict(schema='native-execution-coverage-v1',canonical_units=len(units),
        instrumented_unit_scope='generated units plus configured nongenerated CPP literal owners and their embedded local headers',
        generated_filename_units=sum(name.endswith('_generated.cpp')for name in units),
        nongenerated_filename_units=sum(not name.endswith('_generated.cpp')for name in units),
        source_qualified_functions=sum(units.values()),alias_variants=len(rows),aliases=rows,
        canonical_source_drift=sorted(set(drift)),service_callbacks_replaced=False,
        native_admission_state_modified=False,game_compiled=False,runtime_execution_proved=False,native_closure_proved=False)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    group=parser.add_mutually_exclusive_group(required=True)
    group.add_argument('--runtime',type=Path);group.add_argument('--manifests',type=Path)
    parser.add_argument('--source-root',type=Path,default=Path(__file__).resolve().parents[1])
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    if args.runtime:
        report=analyze(load_metadata(args.runtime,max_bytes=1_048_576));report['runtime_report_sha256']=digest(args.runtime)
    else:report=coverage(args.manifests,args.source_root)
    encoded=json.dumps(report,indent=2,sort_keys=True)+'\n'
    if len(encoded.encode())>1_048_576:raise ValueError('Bounded accounting summary exceeds1MiB')
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(encoded)
    print(json.dumps({key:report[key] for key in ('schema','canonical_units','source_qualified_functions','alias_variants',
        'literal_fallback_rows','native_service_observation_rows','unresolved_rows','canonical_source_drift') if key in report}))
    return int(bool(report.get('canonical_source_drift')))


if __name__=='__main__':raise SystemExit(main())

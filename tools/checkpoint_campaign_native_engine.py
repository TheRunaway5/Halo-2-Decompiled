#!/usr/bin/env python3
"""Record bounded component evidence, current pins, reference inputs and cleanup.

This aggregates existing reports. It runs no engine or original instructions and
does not turn component success into full campaign or native-only acceptance.
"""
import argparse,datetime,hashlib,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def walk(value):
    if isinstance(value,dict):
        for key,child in value.items():
            yield key,child
            yield from walk(child)
    elif isinstance(value,list):
        for index,child in enumerate(value):
            # Cleanup/reference paths often appear as bare list entries.
            # Preserve those values as well as dictionaries nested in lists.
            yield str(index),child
            yield from walk(child)
def relative(value):
    path=Path(value)
    if path.is_absolute():
        try:path=path.relative_to(ROOT)
        except ValueError:return None
    if not path.parts or path.parts[0]not in ('src','include','cmake','tools','tests','docs'):return None
    return str(path)
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--previous',type=Path,required=True)
    parser.add_argument('--migration',type=Path,required=True)
    parser.add_argument('--component',type=Path,action='append',required=True)
    args=parser.parse_args();previous=json.loads(args.previous.read_text());migration=json.loads(args.migration.read_text())
    components=[];pins={};reference_paths={row['path']for row in previous['required_reference_inputs']};scratch=set()
    for path in args.component:
        data=json.loads(path.read_text());assert data.get('passed')is True,(str(path),'component did not pass')
        name=str(path.resolve().relative_to(ROOT));components.append(dict(path=name,sha256=sha(path),passed=True))
        for key,value in walk(data):
            if isinstance(value,str):
                if len(value)==64 and all(c in '0123456789abcdef' for c in value):
                    source=relative(key)
                    if source and (ROOT/source).is_file():pins.setdefault(source,[]).append(dict(report=name,expected=value))
                source=relative(value)
                if source and source.startswith('tests/fixtures/')and source.endswith(('.json','.json.gz')):reference_paths.add(source)
                if value.startswith('/tmp/halo2-test-')or value.startswith(str(ROOT/'.local/test-scratch/halo2-test-')):scratch.add(value)
            if isinstance(key,str)and(key.startswith('/tmp/halo2-test-')or key.startswith(str(ROOT/'.local/test-scratch/halo2-test-'))):scratch.add(key)
    current={};drift=[]
    for source,records in pins.items():
        actual=sha(ROOT/source)
        if any(row['expected']==actual for row in records):current[source]=actual
        for row in records:
            if row['expected']!=actual:drift.append(dict(path=source,current=actual,**row,current_matches_another_component=source in current))
        if source.startswith('tests/fixtures/')and source.endswith(('.json','.json.gz')):reference_paths.add(source)
    references=[]
    for name in sorted(reference_paths):
        path=ROOT/name;assert path.is_file(),name
        references.append(dict(path=name,bytes=path.stat().st_size,sha256=sha(path)))
    for row in previous['required_reference_inputs']:
        assert sha(ROOT/row['path'])==row['sha256'],(row['path'],'previous reference changed')
    assert all(not Path(path).exists()for path in scratch),'A component-owned scratch path remains'
    assert migration['client_built']and migration['source_snapshot_unchanged']
    assert {row['next_missing_owner']for row in migration['rows']if row['next_missing_owner']}=={'278460','1c9f10'}
    metadata=['CMakeLists.txt','.gitignore','docs/CAMPAIGN_NATIVE_ENGINE_GOAL_CHECKLIST.md','tools/checkpoint_campaign_native_engine.py','tools/test_campaign_ai_runtime.py','tools/test_campaign_current_cohort.py']
    pending=sorted({row['path']for row in drift if not row['current_matches_another_component']})
    result=dict(checkpoint_validated=True,goal_complete=False,created_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),previous_checkpoint=dict(path=str(args.previous),sha256=sha(args.previous)),component_reports=components,matching_current_component_source_sha256=current,historical_component_source_drift=drift,current_component_source_pins_all_matched=not pending,unqualified_current_changed_component_sources=pending,metadata_sha256={name:sha(ROOT/name)for name in metadata},migration_report=dict(path=str(args.migration),sha256=sha(args.migration),scope=migration['scope'],original_ctest_passed=False,retrospective_lifetime_criterion_preserves_original_reports=True,next_missing_owners=['278460','1c9f10']),required_reference_inputs=references,required_reference_bytes=sum(row['bytes']for row in references),component_scratch_removed=sorted(scratch),retained_binaries_raw_banks_or_captures=[],existing_engine_lifted_paths_remain=True,full_native_only_campaign_qualified=False,launcher_runtime_qualified_or_promoted=False,next_work=['Qualify current New Game menu, authored Heretic completion and fresh Armory startup with native camera/lifecycle consumers','Finish/requalify explicitly listed current changed component sources','Complete actual278460 getter/start/cancellation dependencies','Complete actual1C9F10 cached-position/filter/workspace/arena/search closure','Finish authored orders/action begin/HS scheduling and full remaining audit gates'],limitations=['Component results are bounded by linked proof scopes','The coherent v5 client predates subsequent components; current metadata pins do not qualify a newly linked runtime','Historical changed pins remain explicit; matching newer component pins are recorded separately; unmatched current files remain unqualified','Required original observations/site metadata are retained data inputs, not executable gameplay'])
    result['previous_component_report_refreshes']=[dict(path=row['path'],previous_sha256=row['sha256'],current_sha256=sha(ROOT/row['path']))for row in previous['component_reports']if sha(ROOT/row['path'])!=row['sha256']]
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(dict(checkpoint_validated=True,goal_complete=False,components=len(components),matching_current_pins=len(current),historical_drift=len(drift),required_references=len(references),required_reference_bytes=result['required_reference_bytes'],removed_scratch_paths=len(scratch))))
if __name__=='__main__':main()

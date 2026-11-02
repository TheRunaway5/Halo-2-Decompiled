"""Compare a complete packaged mod sweep with its previous recorded cohort.

Validate native progress/frame evidence and immutable input/owner receipts.
Passing these bounded comparisons does not qualify complete mod gameplay.
Only a bounded JSON summary is written; no runtime or selection is changed.
"""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import re


KINDS=('inspection','simulation','gameplay_render','preview_render')


def digest(path):
    with path.open('rb')as stream:return hashlib.file_digest(stream,'sha256').hexdigest()


def evidence(sweep_path):
    sweep=json.loads(sweep_path.read_text())
    assert sweep['completed'] and sweep['schema']=='H2_ALL_INSTALLED_WORKSHOP_SWEEP_V1'
    assert not sweep['catalog_errors']
    rows={};passes={kind:set()for kind in KINDS};packages=[]
    for mod in sweep['mods']:
        assert not mod['metadata_error']
        counts=Counter()
        for compact in mod['maps']:
            key=(mod['id'],compact['map'])
            assert key not in rows and compact['status']!='running'
            detail=Path(compact['detail_report'])
            assert detail.is_file() and detail.stat().st_size<=2*1024*1024
            row=json.loads(detail.read_text());assert row['map']==compact['map'] and row['status']==compact['status']
            rows[key]=row
            if row['status']=='not_declared_in_catalog':
                assert not row['checks'];continue
            assert set(row['checks'])==set(KINDS)
            assert row['source_unchanged'] and row['resource_sources_unchanged'] and row['scratch_removed']
            counts['declared']+=1
            for kind in KINDS:
                check=row['checks'][kind]
                if not check['passed']:continue
                assert check['exit_code']==0
                if kind=='inspection':assert row['inspection']['inspection_completed']
                elif kind=='simulation':
                    ticks=re.findall(r'^(?:tick=|Multiplayer ticks=)([0-9]+)\b',check.get('stdout',''),re.M)
                    assert ticks and int(ticks[-1])>=90
                    assert 'Script fault:'not in check.get('stdout','')+check.get('stderr','')
                    if 'completed_ticks'in check:assert int(ticks[-1])==check['completed_ticks']
                else:
                    assert re.search(r'(?:Rendered 4 frames|Multiplayer frames=4\b)',check.get('stdout',''))
                    assert 'Script fault:'not in check.get('stdout','')+check.get('stderr','')
                passes[kind].add(key);counts[kind]+=1
        packages.append(dict(id=mod['id'],title=mod['title'],map_files=len(mod['maps']),**counts))
    assert sweep['runtime_unchanged']
    return sweep,rows,passes,packages


def content_evidence(rows):
    """Inspection completion and successful BSP content reads are distinct."""
    loaded=set();failed={};all_bsps=set()
    for (item,map_name),row in rows.items():
        if row['status']=='not_declared_in_catalog':continue
        inspection=row['inspection'];content=inspection['content'];count=inspection['bsp_count']
        assert len(content)==count and {v['bsp']for v in content}==set(range(count))
        for entry in content:
            key=(item,map_name,entry['bsp']);assert key not in all_bsps;all_bsps.add(key)
            assert type(entry['loaded'])is bool
            if entry['loaded']:
                assert 'description'in entry;loaded.add(key)
            else:
                assert isinstance(entry['error'],str)and entry['error'];failed[key]=entry['error']
    return all_bsps,loaded,failed


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--qualification',type=Path,required=True)
    parser.add_argument('--candidate-route',type=Path,required=True)
    parser.add_argument('--sweep',type=Path,required=True)
    parser.add_argument('--baseline',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    root=Path(__file__).resolve().parents[1]
    q=json.loads(args.qualification.read_text());route=json.loads(args.candidate_route.read_text())
    assert q['passed'] and q['status']=='prepared' and q['qualified_candidate']
    assert not q['accepted_case_failures'] and not q['unresolved_case_failures'] and q['wiring']['passed']
    assert route['uses_qualified_package'] and route['qualification_passed']
    for field in ('runtime_unchanged','shader_package_unchanged','source_cohort_unchanged',
                  'preparation_cache_controls_passed','base_cache_matches_candidate_selection','qualified_package_preserved'):
        assert route[field],field
    assert route['sweep_exit_code']in (0,1)
    current,rows,passes,packages=evidence(args.sweep)
    old,before,old_passes,_=evidence(args.baseline)
    assert set(rows)==set(before),'Installed file identities changed; require a new comparison cohort'
    declared={key for key,row in rows.items()if row['status']!='not_declared_in_catalog'}
    assert declared=={key for key,row in before.items()if row['status']!='not_declared_in_catalog'}
    assert all(rows[key]['source']==before[key]['source']for key in declared),'Installed map bytes changed'
    assert current['runtime']['sha256']==q['runtime_sha256']==route['runtime']['sha256']
    assert digest(root/q['runtime'])==q['runtime_sha256']
    assert route['module_count']==q['native_renderer_shader_package']['module_count']
    assert route['inventory_sha256']==q['native_renderer_shader_package']['inventory_sha256']
    deltas={kind:dict(gained=[dict(id=k[0],map=k[1])for k in sorted(passes[kind]-old_passes[kind])],
        lost=[dict(id=k[0],map=k[1])for k in sorted(old_passes[kind]-passes[kind])])for kind in KINDS}
    losses=any(row['lost']for row in deltas.values())
    bsps,loaded,failed=content_evidence(rows)
    old_bsps,old_loaded,old_failed=content_evidence(before)
    assert bsps==old_bsps,'Authored BSP identities changed; require a new comparison cohort'
    content_key=lambda k:dict(id=k[0],map=k[1],bsp=k[2])
    content_comparison=dict(bsp_count=len(bsps),before_loaded=len(old_loaded),after_loaded=len(loaded),
        before_failed=len(old_failed),after_failed=len(failed),
        gained=[content_key(k)for k in sorted(loaded-old_loaded)],
        lost=[content_key(k)for k in sorted(old_loaded-loaded)],
        remaining_failures=[dict(**content_key(k),error=failed[k])for k in sorted(failed)])
    losses|=bool(content_comparison['lost'])
    assert len(passes['inspection'])==len(declared)
    changes=[]
    for key in sorted(declared):
        was=set(before[key]['inspection']['script_requirements']);now=set(rows[key]['inspection']['script_requirements'])
        if was!=now:changes.append(dict(id=key[0],map=key[1],removed=sorted(was-now),added=sorted(now-was)))
    report=dict(scope=__doc__,completed=True,no_passing_check_losses=not losses,
        runtime=current['runtime'],qualification=str(args.qualification),candidate_route=str(args.candidate_route),
        baseline_runtime=old['runtime'],baseline=str(args.baseline),baseline_sha256=digest(args.baseline),
        files=len(rows),declared=len(declared),packages=packages,totals={kind:len(passes[kind])for kind in KINDS},
        comparisons=deltas,content_reads=content_comparison,
        summary_coordinator_sha256=digest(Path(__file__)),
        all_map_identities_equal=True,all_source_resource_scratch_checks_passed=True,
        script_requirements=current['summary']['script_requirements'],script_requirement_changes=changes,
        material_templates_with_gaps=current['summary']['material_templates_with_gaps'],
        shader_modules=route['module_count'],complete_mod_support=False,mission_flow_qualified=False,
        publication_performed=False,retained_large_artifacts=[])
    raw=json.dumps(report,indent=2)+'\n';assert len(raw.encode())<1024*1024
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(raw)
    print(json.dumps(dict(files=len(rows),declared=len(declared),totals=report['totals'],
        no_passing_check_losses=not losses,distinct_script_requirements=len(report['script_requirements']))))
    return 0 if not losses else 1


if __name__=='__main__':
    raise SystemExit(main())

"""Sweep every installed Halo2 package/map with bounded, private per-map owners.

Declaration and shader inventories are evidence of gaps, never gameplay proof.
Each declared mission runs native inspection,90 simulation ticks, four Original
gameplay frames and four preview frames. All rows survive individual failures.
Undeclared map files and preparation failures remain explicit untested entries.
Run through tools/run_test.py. Only bounded JSON/Markdown reports are retained.
"""
import argparse
import copy
from collections import Counter
import hashlib
import json
import os
import re
from pathlib import Path
import subprocess
import tempfile
import time
import steam_workshop as workshop


def run(command, env, timeout):
    started=time.monotonic()
    try:
        result=subprocess.run(list(map(str,command)),env=env,capture_output=True,text=True,timeout=timeout)
        return dict(command=list(map(str,command)),exit_code=result.returncode,passed=result.returncode==0,
                    seconds=round(time.monotonic()-started,3),stdout=result.stdout[-16000:],stderr=result.stderr[-20000:])
    except subprocess.TimeoutExpired as e:
        return dict(command=list(map(str,command)),exit_code=None,passed=False,
                    seconds=round(time.monotonic()-started,3),error=f'{timeout} second timeout',
                    stdout=(e.stdout or b'').decode(errors='replace')[-16000:],
                    stderr=(e.stderr or b'').decode(errors='replace')[-20000:])


def completed_simulation(check, requested_ticks):
    """Require the final native progress record to cover the requested work."""
    if requested_ticks <= 0:
        raise ValueError('Simulation acceptance requires a positive tick count')
    ticks=re.findall(r'^(?:tick=|Multiplayer ticks=)([0-9]+)\b',check.get('stdout',''),re.MULTILINE)
    check['completed_ticks']=int(ticks[-1]) if ticks else None
    check['progress_reported']=check['completed_ticks'] is not None
    check['requested_ticks']=requested_ticks
    check['passed']=bool(check['passed'] and check['exit_code']==0 and
        check['completed_ticks'] is not None and check['completed_ticks']>=requested_ticks and
        'Script fault:' not in check.get('stderr','') and
        'Script fault:' not in check.get('stdout',''))
    return check['passed']


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--runtime',type=Path,required=True)
    p.add_argument('--probe',type=Path,required=True)
    p.add_argument('--base-cache',type=Path,required=True)
    p.add_argument('--project-root',type=Path,default=Path(__file__).resolve().parents[1])
    p.add_argument('--output',type=Path,required=True)
    p.add_argument('--timeout',type=int,default=180)
    p.add_argument('--resources',action='store_true',help='Also inspect bitmap frames, first audio permutations and facial blocks')
    p.add_argument('--inspection-only',action='store_true',help='Inspect native cache readers without running simulation or rendering')
    a=p.parse_args();scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert a.timeout>0 and not a.output.resolve().is_relative_to(scratch)
    a.output.mkdir(parents=True,exist_ok=True)
    catalog=workshop.scan();report=dict(schema='H2_ALL_INSTALLED_WORKSHOP_SWEEP_V1',
        completed=False,all_checks_passed=False,gameplay_qualified=False,scope=__doc__,
        runtime_checks_requested=not a.inspection_only,resource_checks_requested=a.resources,
        runtime=dict(path=str(a.runtime.resolve()),**workshop.pin(a.runtime)),
        probe=dict(path=str(a.probe.resolve()),**workshop.pin(a.probe)),catalog_errors=catalog['errors'],mods=[])
    def save():
        summary=Counter();requirements=Counter();materials=Counter()
        for mod in report['mods']:
            for row in mod['maps']:
                summary[row['status']]+=1
                for need in row.get('inspection',{}).get('script_requirements',[]):requirements[need]+=1
                for name,values in row.get('inspection',{}).get('materials',{}).get('templates',{}).items():
                    if values['without_native_passes'] or values['gap_messages']:materials[name]+=1
        report['summary']=dict(map_status=dict(summary),script_requirements=dict(requirements.most_common()),
                               material_templates_with_gaps=dict(materials.most_common()))
        compact=copy.deepcopy(report)
        for mod in compact['mods']:
            for row in mod['maps']:
                detail=a.output/'maps'/(mod['id']+'-'+hashlib.sha256(row['map'].encode()).hexdigest()[:16]+'.json')
                if row['status']!='running':
                    if not detail.exists():
                        detail.parent.mkdir(exist_ok=True)
                        raw=json.dumps(row,indent=2)+'\n'
                        if len(raw.encode())>2*1024*1024:raise ValueError('Per-map diagnostic exceeds2MiB')
                        detail.write_text(raw)
                    row['detail_report']=str(detail)
                for check in row['checks'].values():
                    for field in ['stdout','stderr']:check[field]=check.get(field,'')[-800:]
                inspection=row.get('inspection',{})
                for value in inspection.get('materials',{}).get('templates',{}).values():
                    value['without_native_passes_count']=len(value.pop('without_native_passes'))
                if 'material_errors' in inspection:
                    inspection['material_error_count']=len(inspection.pop('material_errors'))
        data=json.dumps(compact,indent=2)+'\n'
        if len(data.encode())>16*1024*1024:raise ValueError('Sweep report exceeds16MiB diagnostic budget')
        temp=a.output/'sweep.json.tmp';temp.write_text(data);temp.replace(a.output/'sweep.json')
    save()
    for mod in catalog['mods']:
        item=Path(mod['directory']);entry=dict(id=mod['id'],title=mod['title'],metadata_error=mod['error'],maps=[])
        report['mods'].append(entry)
        missions={r['scenario']:r for r in mod['missions']}
        maps=workshop.source_owned(item,'maps')
        files=sorted((path for path in maps.iterdir() if path.is_file() and path.suffix.casefold()=='.map'),key=lambda p:p.name.casefold()) if maps.is_dir() else []
        names={path.stem.casefold() for path in files}
        for name,mission in missions.items():
            if name.casefold() not in names:
                entry['maps'].append(dict(map=name+'.map',status='missing_declared_map',checks={}))
        for source in files:
            row=dict(map=source.name,status='running',checks={});entry['maps'].append(row);save()
            mission=next((r for name,r in missions.items() if name.casefold()==source.stem.casefold()),None)
            if mission is None:
                row.update(status='not_declared_in_catalog',reason='Map is inventoried statically but cannot be selected through the production Mods menu');save();continue
            with tempfile.TemporaryDirectory(prefix='map-',dir=scratch) as folder:
                case=Path(folder);env={key:value for key,value in os.environ.items() if not key.startswith('HALO2_')}
                root=a.project_root.resolve(strict=True)
                env['H2_INSTALLATION_LAYOUT']=json.dumps(dict(schema='H2_INSTALLATION_LAYOUT_V1',data=str(root),resources=str(root)))
                env.setdefault('SDL_VIDEODRIVER','x11')
                for key in ['HALO2_TEST_SCRATCH','TMPDIR','TMP','TEMP','XDG_DATA_HOME','XDG_CONFIG_HOME','XDG_CACHE_HOME','MESA_SHADER_CACHE_DIR','__GL_SHADER_DISK_CACHE_PATH']:
                    env[key]=str(case/key)
                    Path(env[key]).mkdir()
                try:
                    before=workshop.pin(source)
                    row['source']=before
                    prepared=workshop.prepare_cached(item,mission['scenario'],a.base_cache,case/'prepared')
                    row['source']=before;row['campaign']=mission['campaign']
                    inspect=case/'inspection.json'
                    row['checks']['inspection']=run([a.probe,prepared['map'],prepared['shared_maps'],prepared['prepared_root'],
                        'campaign' if mission['campaign'] else 'multiplayer',inspect,*(['--resources'] if a.resources else [])],env,a.timeout)
                    if inspect.is_file():
                        if inspect.stat().st_size>2*1024*1024:raise ValueError('Per-map native inspection exceeds2MiB')
                        row['inspection']=json.loads(inspect.read_text())
                    if not a.inspection_only:
                        common=[a.runtime,'--map',prepared['map'],'--shared-maps',prepared['shared_maps'],'--mute']
                        mode=['--campaign'] if mission['campaign'] else ['--multiplayer','--local-players','1']
                        row['checks']['simulation']=run([*common,*mode,'--simulate','90'],env,a.timeout)
                        simulation=row['checks']['simulation']
                        completed_simulation(simulation,90)
                        row['checks']['gameplay_render']=run([*common,*mode,'--frames','4','--graphics-mode','Original','--hidden',
                            '--viewport','640','480'],env,a.timeout)
                        row['checks']['preview_render']=run([*common,'--mcc-preview','--prepared-mcc-cache-root',prepared['prepared_root'],
                            '--frames','4','--graphics-mode','Original','--hidden','--viewport','640','480'],env,a.timeout)
                        for kind in ['gameplay_render','preview_render']:
                            check=row['checks'][kind]
                            check['four_frames_reported']=bool(re.search(r'(?:Rendered 4 frames|Multiplayer frames=4\b)',check['stdout']))
                            check['passed'] &= check['four_frames_reported']
                    row['source_unchanged']=before==workshop.pin(source)
                    inputs=json.loads((Path(prepared['prepared_root'])/'workshop-inputs.json').read_text())
                    row['resource_sources_unchanged']=all(workshop.pin(Path(path))==value for path,value in inputs.get('resources',{}).items())
                    row['status']='bounded_checks_passed' if row['source_unchanged'] and row['resource_sources_unchanged'] and all(r['passed'] for r in row['checks'].values()) else 'checks_failed'
                    inspection=row.get('inspection',{})
                    if row['status']=='bounded_checks_passed' and (inspection.get('script_requirements') or
                        inspection.get('materials',{}).get('without_native_passes') or inspection.get('material_errors') or
                        any(not r['loaded'] or r.get('limitations') for r in inspection.get('content',[]))):
                        row['status']='checks_passed_with_inspection_gaps'
                    if a.inspection_only:
                        row['status']={'bounded_checks_passed':'native_inspection_passed',
                            'checks_passed_with_inspection_gaps':'native_inspection_passed_with_gaps',
                            'checks_failed':'native_inspection_failed'}[row['status']]
                except Exception as error:
                    row.update(status='preparation_or_inspection_failed',error=str(error))
                    if 'source' in row:row['source_unchanged']=row['source']==workshop.pin(source)
            row['scratch_removed']=not case.exists();save()
            print(json.dumps(dict(mod=entry['title'],map=source.name,status=row['status'],
                failures=[key for key,value in row['checks'].items() if not value['passed']])),flush=True)
    report['completed']=True
    report['runtime_unchanged']=report['runtime']['sha256']==workshop.pin(a.runtime)['sha256']
    accepted='native_inspection_passed' if a.inspection_only else 'bounded_checks_passed'
    report['all_checks_passed']=not report['catalog_errors'] and all(not m['metadata_error'] and all(r['status']==accepted for r in m['maps']) for m in report['mods'])
    save()
    return 0 if report['all_checks_passed'] else 1


if __name__=='__main__':raise SystemExit(main())

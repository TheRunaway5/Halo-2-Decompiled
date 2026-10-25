#!/usr/bin/env python3
"""Extend the selected runtime's map proof only after real submitted-frame checks.

The executable, resources and prior qualification remain immutable. Publication
uses a new proof and an atomic guarded selection update after normal admission.
"""
import argparse,copy,json,os,uuid
from pathlib import Path
from build_launcher_engine import atomic_json,digest,smoke
from gameplay_runtime_selection import selected_gameplay_runtime,validate_arguments
ROOT=Path(__file__).resolve().parents[1]
def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--map',default='elongation.map');ap.add_argument('--output',type=Path,required=True);a=ap.parse_args()
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert Path(a.map).name==a.map and a.map.endswith('.map')
    selection=ROOT/'gameplay-runtime-selection.json';before=selection.read_bytes();record=selected_gameplay_runtime(ROOT)
    assert record and a.map not in record['maps'],'Preserve an already qualified map'
    runtime=ROOT/record['runtime'];runtime_pin=digest(runtime);assert runtime_pin==record['runtime_sha256']
    proof=ROOT/record['qualification'];proof_pin=digest(proof);prior=json.loads(proof.read_text())
    maps=ROOT/record['cache']/'halo2/h2_maps_win64_dx11';map_pin=digest(maps/a.map)
    out=a.output.resolve();assert out.is_relative_to(ROOT) and not out.exists();out.mkdir(parents=True)
    report=copy.deepcopy(prior);report.update(status='qualifying-additional-map',passed=False,
        parent_qualification=str(proof.relative_to(ROOT)),parent_qualification_sha256=proof_pin,
        additional_map=a.map,additional_map_sha256=map_pin,additional_map_checks=[],original_runtime_unchanged=True)
    if 'publication' in report:report['parent_publication']=report.pop('publication')
    for count in (1,2,4):
        case=smoke(runtime,maps,a.map,count,owner,180,ROOT,record.get('video_driver',''))
        report['additional_map_checks'].append(case);atomic_json(out/'qualification.json',report)
        if not case['passed']:raise RuntimeError('Additional map failed actual renderer qualification')
    assert digest(runtime)==runtime_pin and digest(proof)==proof_pin and digest(maps/a.map)==map_pin
    assert selection.read_bytes()==before,'Another task changed selection; preserve it'
    report['smoke'].extend(report['additional_map_checks'])
    report['requested_smoke_cases'].extend(dict(map=a.map,local_players=n) for n in (1,2,4))
    report.update(passed=True,status='qualified-additional-map',runtime_installed=True)
    atomic_json(out/'qualification.json',report)
    candidate=copy.deepcopy(record);candidate['maps'][a.map]=map_pin
    candidate['qualification']=(out/'qualification.json').relative_to(ROOT).as_posix();candidate['qualification_sha256']=digest(out/'qualification.json')
    temporary=out/'selection.json';atomic_json(temporary,candidate)
    admitted=selected_gameplay_runtime(ROOT,selection_name=temporary.relative_to(ROOT).as_posix())
    validate_arguments(ROOT,admitted,['--map',str(maps/a.map),'--shared-maps',str(maps)])
    assert selection.read_bytes()==before and digest(runtime)==runtime_pin
    pending=selection.with_name('.combat-map-selection-'+uuid.uuid4().hex)
    try:
        atomic_json(pending,candidate);assert selection.read_bytes()==before
        os.replace(pending,selection)
    finally:pending.unlink(missing_ok=True)
    assert selected_gameplay_runtime(ROOT)['maps'][a.map]==map_pin
    atomic_json(out/'publication.json',dict(passed=True,runtime=record['runtime'],runtime_sha256=runtime_pin,map=a.map,map_sha256=map_pin,actual_submitted_frame_checks=3,previous_qualification_preserved=digest(proof)==proof_pin))
    print(json.dumps(dict(passed=True,runtime=record['runtime'],map=a.map,submitted_frame_checks=3)),flush=True);return 0
if __name__=='__main__':raise SystemExit(main())

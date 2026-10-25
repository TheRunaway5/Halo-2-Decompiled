#!/usr/bin/env python3
"""Guard and publish this investigation's already qualified launcher candidate."""
import argparse,json,os
from pathlib import Path
from build_launcher_engine import atomic_json,digest,publication_identity,publish_runtime_files,source_hashes
from gameplay_runtime_selection import selected_gameplay_runtime
ROOT=Path(__file__).resolve().parents[1]
NAMES=('Halo2-Launcher','Halo2-Title-Launcher','gameplay-runtime-selection.json','frontend-runtime-selection.json')
def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--capture-baseline',action='store_true');ap.add_argument('--report',type=Path);a=ap.parse_args()
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    baseline=owner/'combat-effect-candidate-publication-baseline.json'
    if a.capture_baseline:
        assert not baseline.exists(),'Preserve the original publication baseline'
        atomic_json(baseline,{name:publication_identity(ROOT/name) for name in NAMES})
        print('Recorded current launcher publication identities without changing selection',flush=True);return 0
    assert a.report and baseline.is_file()
    path=a.report.resolve(strict=True);report=json.loads(path.read_text())
    assert report['passed'] and report['qualified_candidate'] and report['status']=='prepared' and not report['runtime_installed']
    destination=(ROOT/report['runtime']).parent.resolve(strict=True)
    assert destination.parent==ROOT/'bin/engine-builds'
    record=json.loads((destination/'selection.json').read_text())
    selected_gameplay_runtime(ROOT,selection_name=(destination/'selection.json').relative_to(ROOT).as_posix())
    assert record['runtime_sha256']==report['runtime_sha256']==digest(ROOT/record['runtime'])
    assert source_hashes(owner/'source')==json.loads((destination/'source-hashes.json').read_text())
    expected={name:tuple(value) if value is not None else None for name,value in json.loads(baseline.read_text()).items()}
    assert all(publication_identity(ROOT/name)==expected[name] for name in NAMES),'Another task changed publication; preserve its selection'
    frontend_path=destination/'frontend-selection.json'
    frontend=json.loads(frontend_path.read_text()) if frontend_path.is_file() else None
    publish_runtime_files(ROOT,destination,owner,record,frontend,report,expected)
    report.update(status='published',runtime_installed=True)
    atomic_json(path,report);print(json.dumps(dict(runtime=record['runtime'],runtime_sha256=record['runtime_sha256'],published=True)),flush=True);return 0
if __name__=='__main__':raise SystemExit(main())

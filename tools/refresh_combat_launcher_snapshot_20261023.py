#!/usr/bin/env python3
"""Refresh this investigation's owned build to one exact current production snapshot.

No checkout, reference, installed runtime or other scratch owner is modified.
Changed compilation inputs receive fresh timestamps before official validation.
"""
import json,os,shutil,sys,tempfile
from pathlib import Path
from build_launcher_engine import freeze_source,source_hashes

ROOT=Path(__file__).resolve().parents[1]
def main():
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    source=owner/'source';follow=ROOT/'analysis/combat-presentation-audit-20261022/repair-followup-20261023'
    previous=json.loads((follow/'source-hashes.json').read_text())
    if '--recover-interrupted' in sys.argv:
        # The failed build recorded the exact partially refreshed input set.
        # Authenticate it before resuming; never silently adopt unknown edits.
        receipt=ROOT/'analysis/combat-presentation-audit-20261022/repair-launcher-phased-object-audio-20261023/source-hashes.json'
        previous=json.loads(receipt.read_text())
    assert not source.is_symlink() and source_hashes(source)==previous
    changes={}
    with tempfile.TemporaryDirectory(prefix='launcher-source-refresh-',dir=owner) as temporary:
        fresh=Path(temporary)/'source';current=freeze_source(ROOT,fresh)
        # Never write through a borrowed authored-data alias.
        data=source/'data'
        if data.is_symlink():
            assert data.resolve()==(ROOT/'data').resolve()
            data.unlink();shutil.copytree(fresh/'data',data)
        for relative in sorted(previous.keys()|current.keys()):
            if previous.get(relative)==current.get(relative):continue
            target=source/relative
            assert target.parent.resolve().is_relative_to(source.resolve())
            if relative not in current:target.unlink()
            else:
                assert not target.is_symlink()
                target.parent.mkdir(parents=True,exist_ok=True)
                if target.exists():target.unlink()
                target.write_bytes((fresh/relative).read_bytes())
            changes[relative]=dict(before=previous.get(relative),after=current.get(relative))
        # Freeze the corresponding build helpers and top-level test sources.
        # Their borrowed fixture/helper directories stay read-only aliases.
        for path in (fresh/'tools').rglob('*'):
            if not path.is_file():continue
            target=source/'tools'/path.relative_to(fresh/'tools')
            if target.exists() and target.read_bytes()==path.read_bytes():continue
            assert not target.is_symlink() and target.parent.resolve().is_relative_to(source.resolve())
            target.parent.mkdir(parents=True,exist_ok=True)
            if target.exists():target.unlink()
            target.write_bytes(path.read_bytes())
        for path in fresh.iterdir():
            if path.is_file() and path.name!='CMakeLists.txt':
                target=source/path.name
                if not target.exists() or target.read_bytes()!=path.read_bytes():
                    if target.exists():target.unlink()
                    target.write_bytes(path.read_bytes())
        for path in (fresh/'tests').iterdir():
            if path.is_file() and not path.is_symlink():
                target=source/'tests'/path.name
                if not target.exists() or target.read_bytes()!=path.read_bytes():
                    if target.exists():target.unlink()
                    target.write_bytes(path.read_bytes())
        assert source_hashes(source)==current and source_hashes(ROOT)==current
    (follow/'source-hashes.json').write_text(json.dumps(current,indent=2,sort_keys=True)+'\n')
    admission=follow/'diagnostic-admission.json';value=json.loads(admission.read_text())
    role={'role':'Exact current production snapshot for launcher delivery; diagnostic binaries invalidated',
          'receipt':str(follow/'launcher-source-refresh.json')}
    value.update(build_completed=False,invalidated_by=__doc__,source_snapshot_role=role)
    (follow/'snapshot-role.json').write_text(json.dumps(role,indent=2)+'\n')
    admission.write_text(json.dumps(value,indent=2)+'\n')
    receipt=dict(scope=__doc__,changes=changes,source_files=len(current),
        production_unchanged=source_hashes(ROOT)==current,private_refresh_tree_removed=not Path(temporary).exists(),
        launcher_build_or_gameplay_verified=False)
    (follow/'launcher-source-refresh.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps({k:v for k,v in receipt.items() if k!='changes'}),flush=True)
    return 0
if __name__=='__main__':raise SystemExit(main())

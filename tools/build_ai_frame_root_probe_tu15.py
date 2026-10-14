#!/usr/bin/env python3
"""Build a strict AI-frame replay with one retained current-root memory/header ABI.

The standalone link uses current source memory, snapshot storage and the
complete AI module. No frozen game libraries or supplied game-service returns
are linked. Original input banks and independent hardware remain required.
"""
import argparse,json,shutil,subprocess
from pathlib import Path
from audit_rendering import file_sha

ROOT=Path(__file__).resolve().parents[1]

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output',type=Path,required=True)
    ap.add_argument('--module',type=Path,required=True);a=ap.parse_args()
    out=a.output.resolve();assert not out.exists();source=out/'source';source.mkdir(parents=True)
    originals=[p for p in (ROOT/'include').rglob('*') if p.is_file()]
    originals+=list((ROOT/'src').glob('*.hpp'))
    originals+=[ROOT/'src'/p for p in ('source_runtime_memory.cpp','source_runtime_snapshot_bytes.cpp','cache_file_snapshot.cpp','source_runtime_write_protection.inc')]
    originals+=[ROOT/'tools/replay_native_ai_frame_tu15.cpp']
    snapshot={}
    for p in originals:
        q=source/p.relative_to(ROOT);q.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(p,q)
        snapshot[str(p.relative_to(ROOT))]=file_sha(q)
    module=a.module.resolve();shutil.copy2(module,source/'module.cpp')
    common=['c++','-std=c++20','-O3','-DNDEBUG','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(source/'include'),'-I'+str(source/'src')]
    files=[source/'module.cpp',source/'tools/replay_native_ai_frame_tu15.cpp',*[source/'src'/p for p in ('source_runtime_memory.cpp','source_runtime_snapshot_bytes.cpp','cache_file_snapshot.cpp')]]
    commands=[common+['-c',str(p),'-o',str(out/(p.stem+'.o'))] for p in files]
    commands.append(['c++',*[str(out/(p.stem+'.o')) for p in files],'-pthread','-o',str(out/'replay-native')])
    with (out/'build.log').open('w') as log:
        for command in commands:subprocess.run(command,stdout=log,stderr=subprocess.STDOUT,check=True)
    drift=[p for p,h in snapshot.items() if file_sha(ROOT/p)!=h]
    report=dict(scope=__doc__,tool_sha256=file_sha(Path(__file__)),commands=commands,
        original_module=str(module),module_sha256=file_sha(source/'module.cpp'),source_snapshot_sha256=snapshot,
        root_drift_after_compile=drift,objects_sha256={p.name:file_sha(p) for p in out.glob('*.o')},
        binary_sha256=file_sha(out/'replay-native'),passed=not drift)
    (out/'build.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    assert not drift,drift
    print('Built current-root AI frame',report['binary_sha256'])

if __name__=='__main__':main()

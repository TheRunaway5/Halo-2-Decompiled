#!/usr/bin/env python3
"""Build a strict AI-frame replay with one retained current-root memory/header ABI.

The standalone link uses current source memory, snapshot storage and the
complete AI module. No frozen game libraries or supplied game-service returns
are linked. Original input banks and independent hardware remain required.
"""
import argparse,json,shlex,shutil,subprocess
from pathlib import Path
from audit_rendering import file_sha

ROOT=Path(__file__).resolve().parents[1]

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output',type=Path,required=True)
    ap.add_argument('--reuse-build',type=Path,help='Reuse objects only after exact compiler/dependency admission')
    ap.add_argument('--api',choices=('frame','actor','object-material','recorded-animation'),default='frame')
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
    if a.api=='actor':commands[1].insert(1,'-DHALO2_REPLAY_AI_ACTOR_TU15=1')
    if a.api=='object-material':commands[1].insert(1,'-DHALO2_REPLAY_OBJECT_MATERIAL_TU15=1')
    if a.api=='recorded-animation':commands[1].insert(1,'-DHALO2_REPLAY_RECORDED_ANIMATION_TU15=1')
    commands.append(['c++',*[str(out/(p.stem+'.o')) for p in files],'-pthread','-o',str(out/'replay-native')])
    previous_path=a.reuse_build.resolve() if a.reuse_build else None
    previous=json.loads((previous_path/'build.json').read_text()) if previous_path else None
    dependencies={};dependency_commands=[];executed=[];reused=[]
    def normalize(command,directory):
        return [v.replace(str(directory/'source'),'${SOURCE}').replace(str(directory),'${BUILD}') for v in command]
    with (out/'build.log').open('w') as log:
        for p,command in zip(files,commands):
            depfile=out/(p.stem+'.d');object_=out/(p.stem+'.o')
            flags=command[:command.index('-c')]
            dependency_command=flags+['-MM','-MF',str(depfile),'-MT',str(object_),str(p)]
            dependency_commands.append(dependency_command)
            subprocess.run(dependency_command,stdout=log,stderr=subprocess.STDOUT,check=True)
            paths=shlex.split(depfile.read_text().replace('\\\n',' ').partition(':')[2])
            required={}
            for name in paths:
                dependency=Path(name).resolve();relative=str(dependency.relative_to(source))
                digest=file_sha(dependency);required[relative]=digest
                assert digest==(file_sha(source/'module.cpp') if relative=='module.cpp' else snapshot[relative])
            dependencies[p.stem]=required
            admitted=previous is not None
            if admitted:
                for name,digest in required.items():
                    prior=(previous_path/'source'/name)
                    expected=previous['module_sha256'] if name=='module.cpp' else previous['source_snapshot_sha256'].get(name)
                    if not prior.is_file() or expected!=digest or file_sha(prior)!=digest:
                        admitted=False;break
            if admitted:
                prior_commands=[c for c in previous['commands'] if '-c' in c and Path(c[c.index('-o')+1]).name==object_.name]
                admitted=len(prior_commands)==1 and normalize(prior_commands[0],previous_path)==normalize(command,out)
            if admitted:
                prior=previous_path/object_.name
                assert file_sha(prior)==previous['objects_sha256'][object_.name]
                shutil.copy2(prior,object_);reused.append(dict(object=object_.name,build=str(previous_path),sha256=file_sha(prior)))
            else:
                subprocess.run(command,stdout=log,stderr=subprocess.STDOUT,check=True);executed.append(command)
        subprocess.run(commands[-1],stdout=log,stderr=subprocess.STDOUT,check=True);executed.append(commands[-1])
    # Only used headers and sources can change this replay's actual ABI. Keep
    # all other shared-root drift as observations without rejecting the build.
    copied_drift=[p for p,h in snapshot.items() if file_sha(ROOT/p)!=h]
    required_root={p for required in dependencies.values() for p in required if p!='module.cpp'}
    drift=sorted(set(copied_drift)&required_root)
    for required in dependencies.values():
        for p,digest in required.items():assert file_sha(source/p)==digest
    report=dict(scope=__doc__,tool_sha256=file_sha(Path(__file__)),commands=commands,api=a.api,
        original_module=str(module),module_sha256=file_sha(source/'module.cpp'),source_snapshot_sha256=snapshot,
        root_drift_after_compile=drift,copied_root_drift_after_compile=copied_drift,
        compiler_dependency_sha256=dependencies,dependency_commands=dependency_commands,
        dependency_files_sha256={p.name:file_sha(p) for p in out.glob('*.d')},
        executed_commands=executed,reused_objects=reused,
        reuse_build_report_sha256=file_sha(previous_path/'build.json') if previous_path else None,
        objects_sha256={p.name:file_sha(p) for p in out.glob('*.o')},
        binary_sha256=file_sha(out/'replay-native'),passed=not drift)
    (out/'build.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    assert not drift,drift
    print('Built current-root AI frame',report['binary_sha256'])

if __name__=='__main__':main()

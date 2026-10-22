#!/usr/bin/env python3
"""Build the AI-frame replay against the coherent pinned v4 ABI."""
import argparse,json,shutil,subprocess
from pathlib import Path
from audit_rendering import file_sha
ROOT=Path(__file__).resolve().parents[1]

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True);ap.add_argument('--release',action='store_true');ap.add_argument('--module',type=Path,required=True);a=ap.parse_args();out=a.output.resolve();assert not out.exists();out.mkdir(parents=True)
    base=ROOT/'analysis/weapons-tu15-goal-20261013';pinned=json.loads((base/'charge-effect-source-fixed-v2/build.json').read_text());v4=ROOT/'.local/sandbox-tu15-source-20261014-v4'
    for p,h in {**pinned['linked_libraries_sha256'],**pinned['override_objects'],**pinned['override_source_files']}.items():assert file_sha(Path(p))==h,p
    provenance=json.loads((base/'wraith-source-provenance-v1.json').read_text());checkpoint=Path(provenance['source_checkpoint']);assert file_sha(checkpoint)==provenance['source_checkpoint_sha256'];manifest=json.loads(checkpoint.read_text())
    headers={p:h for p,h in manifest['source_sha256'].items() if p.startswith('include/') or (p.startswith('src/') and p.endswith('.hpp'))}
    for p,h in headers.items():assert file_sha(v4/p)==h,p
    source=out/'source';source.mkdir();(source/'include/halo2').mkdir(parents=True)
    copies={a.module.resolve():source/'particle.cpp',ROOT/'tools/replay_native_ai_frame_tu15.cpp':source/'replay.cpp',ROOT/'include/halo2/source_ai_frame_tu15.hpp':source/'include/halo2/source_ai_frame_tu15.hpp'}
    # The current shared lowerer emits this read-only observer declaration.
    # None of this module's bodies contains its101EA0 contact call site.
    observer=ROOT/'include/halo2/source_projectile_effect_observer.hpp'
    if 'halo2/source_projectile_effect_observer.hpp' in (a.module.resolve()).read_text():
        copies[observer]=source/'include/halo2/source_projectile_effect_observer.hpp'
    for name in ('physics_contact_runtime_internal.hpp','physics_world_runtime_internal.hpp'):
        copies[ROOT/'.local/weapon-charge-effect-source-20261014'/name]=source/name
    for p,q in copies.items():shutil.copyfile(p,q)
    common=['c++','-std=c++20']+(['-O3','-DNDEBUG'] if a.release else ['-O1'])+['-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(source/'include'), '-I'+str(source/'src'),'-I'+str(v4/'include'),'-I'+str(v4/'src'),'-isystem','/usr/include/SDL2']
    commands=[common+['-c',str(source/'particle.cpp'),'-o',str(out/'particle.o')],common+['-c',str(source/'replay.cpp'),'-o',str(out/'replay.o')]]
    link=list(pinned['link']);index=next(i for i,p in enumerate(link) if p.endswith('/capture.o'));link[index:index+1]=[str(out/'replay.o'),str(out/'particle.o')];link[link.index('-o')+1]=str(out/'replay-native');commands.append(link)
    with (out/'build.log').open('w') as log:
        for command in commands:subprocess.run(command,cwd=ROOT/'build-sandbox-tu15-20261014-v4',stdout=log,stderr=subprocess.STDOUT,check=True)
    metadata=dict(scope=__doc__,tool_sha256=file_sha(Path(__file__)),release=a.release,compile=commands[:-1],link=link,source_checkpoint_sha256=file_sha(checkpoint),matching_headers_verified=len(headers),linked_libraries_sha256=pinned['linked_libraries_sha256'],override_objects=pinned['override_objects'],source_files_sha256={str(q.relative_to(out)):file_sha(q) for q in copies.values()},original_source_files_sha256={str(p):file_sha(p) for p in copies},objects_sha256={p.name:file_sha(p) for p in out.glob('*.o')},binary_sha256=file_sha(out/'replay-native'))
    (out/'build.json').write_text(json.dumps(metadata,indent=2,sort_keys=True)+'\n');print('Built native AI frame',metadata['binary_sha256'])
if __name__=='__main__':main()

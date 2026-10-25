#!/usr/bin/env python3
"""Admit only this repair's presentation owners to the existing owned test cohort."""
import difflib,hashlib,json,os
from pathlib import Path
from build_launcher_engine import source_hashes
ROOT=Path(__file__).resolve().parents[1]
FILES=['cmake/MultiplayerContinuation.cmake','cmake/SourceLensDraws.cmake',
'include/halo2/source_lens_frame_world_tu15.hpp','include/halo2/source_execution_lens.hpp',
'include/halo2/source_lens_query_frame_tu15.hpp','include/halo2/source_lens_draw_frame_tu15.hpp',
'include/halo2/native_lens_presentation_tu15.hpp','src/runtime/source_lens_frame_world_tu15.cpp',
'src/runtime/source_execution_lens.cpp','src/rendering/effects/source_lens_query_frame_tu15.cpp',
'src/rendering/effects/source_lens_draw_frame_tu15.cpp','src/rendering/effects/native_lens_presentation_tu15.cpp',
'src/rendering/effects/native_client_lens_operands_tu15.inc']
def main():
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);source=owner/'source'
    follow=ROOT/'analysis/combat-presentation-audit-20261022/repair-followup-20261023'
    old=json.loads((follow/'source-hashes.json').read_text());assert source_hashes(source)==old
    changes={}
    for name in FILES+['tests/native_lens_presentation_tu15_tests.cpp',
            'include/halo2/native_weapon_projectile_scene.hpp','src/gameplay/weapons/native_weapon_projectile_scene.cpp']:
        target=source/name;raw=(ROOT/name).read_bytes()
        if target.exists() and target.read_bytes()==raw:continue
        target.parent.mkdir(parents=True,exist_ok=True);assert not target.is_symlink()
        before=hashlib.sha256(target.read_bytes()).hexdigest() if target.exists() else None
        target.write_bytes(raw);changes[name]=dict(before=before,after=hashlib.sha256(raw).hexdigest())
    name='src/gameplay/multiplayer/multiplayer_runtime.cpp';target=source/name
    before=target.read_bytes();old_lines=before.decode().splitlines(keepends=True);root_lines=(ROOT/name).read_text().splitlines(keepends=True)
    projected=[];selected=[];excluded=[]
    for tag,i,j,k,l in difflib.SequenceMatcher(None,old_lines,root_lines,autojunk=False).get_opcodes():
        new=''.join(root_lines[k:l]);own=tag!='equal' and 'native_lens' in new
        projected.extend(root_lines[k:l] if own else old_lines[i:j])
        if tag!='equal':(selected if own else excluded).append(dict(before_lines=[i+1,j],after_lines=[k+1,l]))
    if 'native_lens_resources' not in before.decode():assert len(selected)==8,selected
    raw=''.join(projected).encode()
    if raw!=before:
        target.write_bytes(raw);changes[name]=dict(before=hashlib.sha256(before).hexdigest(),after=hashlib.sha256(raw).hexdigest(),lens_hunks=selected,unrelated_root_hunks_excluded=excluded)
    pins=source_hashes(source);(follow/'source-hashes.json').write_text(json.dumps(pins,indent=2,sort_keys=True)+'\n')
    admission=follow/'diagnostic-admission.json';a=json.loads(admission.read_text());a.update(build_completed=False,invalidated_by=__doc__);admission.write_text(json.dumps(a,indent=2)+'\n')
    role=dict(role='Frozen production snapshot plus scoped client-lens repair; unrelated current ROOT changes excluded; no launcher or joined-gameplay acceptance yet')
    (follow/'snapshot-role.json').write_text(json.dumps(role,indent=2)+'\n')
    out=ROOT/'analysis/combat-presentation-audit-20261022/repair-client-lens';p=out/'source-refresh.json';n=2
    while p.exists():p=out/f'source-refresh-{n}.json';n+=1
    p.write_text(json.dumps(dict(scope=__doc__,changes=changes,launcher_or_client_gameplay_verified=False),indent=2)+'\n')
    print(json.dumps(dict(changed=list(changes),receipt=str(p))),flush=True)
if __name__=='__main__':main()

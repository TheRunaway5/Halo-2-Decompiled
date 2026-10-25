#!/usr/bin/env python3
"""Give the two component GPU tests their actual owned build shader inputs."""
import hashlib,json,os
from pathlib import Path
from build_launcher_engine import source_hashes
ROOT=Path(__file__).resolve().parents[1]
def sha(data):return hashlib.sha256(data).hexdigest()
def main():
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);source=owner/'source'
    follow=ROOT/'analysis/combat-presentation-audit-20261022/repair-followup-20261023'
    pins=json.loads((follow/'source-hashes.json').read_text());assert source_hashes(source)==pins;changes={}
    def save(relative,data):
        path=source/relative;before=path.read_bytes()
        if before!=data:path.write_bytes(data);changes[relative]=dict(before=sha(before),after=sha(data))
    for relative in ('tests/projectile_glow_gpu_tests.cpp','tests/camouflage_material_gpu_tests.cpp'):
        save(relative,(ROOT/relative).read_bytes())
    relative='CMakeLists.txt';text=(source/relative).read_text()
    anchor='        add_executable(projectile_glow_gpu_tests tests/projectile_glow_gpu_tests.cpp)';assert text.count(anchor)==1
    lines='\n'.join('        target_compile_definitions('+target+' PRIVATE HALO2_SHADER_DIR="${CMAKE_CURRENT_BINARY_DIR}/shaders")'
        for target in ('projectile_glow_gpu_tests','camouflage_material_gpu_tests'))
    if lines not in text:save(relative,text.replace(anchor,anchor+'\n'+lines).encode())
    updated=source_hashes(source);assert {p for p in updated if updated[p]!=pins.get(p)}=={p for p in changes if p in updated}
    (follow/'source-hashes.json').write_text(json.dumps(updated,indent=2,sort_keys=True)+'\n')
    path=follow/'diagnostic-admission.json';value=json.loads(path.read_text());value.update(build_completed=False,invalidated_by=__doc__);path.write_text(json.dumps(value,indent=2)+'\n')
    receipt=dict(scope=__doc__,changes=changes,private_renderer_result_or_shader_supplied=False,
        actual_compiled_shader_bank=str(owner/'build/shaders'),production_unchanged=source_hashes(source)==updated)
    (follow/'gpu-shader-input-refresh.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt),flush=True)
if __name__=='__main__':main()

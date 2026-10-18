#!/usr/bin/env python3
"""Run an authenticated captured-particle graphics probe with real validation."""
import argparse
import json
import os
from pathlib import Path
import resource
import subprocess
import time

from audit_rendering import file_sha

SHADERS=('particle.vert.spv','particle.geom.spv','particle.frag.spv',
    'particle_directional.vert.spv','particle_perpendicular.vert.spv',
    'particle_model.vert.spv','particle_model.geom.spv','particle_distortion.vert.spv')


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build',type=Path,required=True)
    parser.add_argument('--input',type=Path,required=True)
    parser.add_argument('--shaders',type=Path,required=True)
    parser.add_argument('--validation-layer',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    args.build=args.build.absolute();args.input=args.input.absolute()
    args.shaders=args.shaders.absolute();args.validation_layer=args.validation_layer.absolute()
    args.output=args.output.absolute()
    build=json.loads((args.build/'build.json').read_text())
    data=json.loads(args.input.read_text())
    binary=args.build/'capture'
    assert build['build_passed'] and file_sha(binary)==build['binary_sha256']
    assert file_sha(Path(build['source']))==build['source_sha256']
    assert data['passed'] and not data['source_state_mutated'] and not data['expanded_bank_written']
    assert file_sha(Path(data['map']))==data['map_sha256']
    assert file_sha(Path(data['source_draw_file']))==data['source_draw_sha256']
    pins={str(args.shaders/name):file_sha(args.shaders/name) for name in SHADERS}
    pins[str(binary)]=file_sha(binary);pins[str(args.input)]=file_sha(args.input)
    pins[str(args.build/'build.json')]=file_sha(args.build/'build.json')
    pins[str(Path(data['map']))]=data['map_sha256']
    pins[str(Path(data['source_draw_file']))]=data['source_draw_sha256']
    for name,digest in build['linked_files_sha256'].items():
        assert file_sha(Path(name))==digest,name
        pins[name]=digest
    provenance=args.validation_layer/'provenance.json'
    layer=json.loads(provenance.read_text())
    manifest=args.validation_layer/'layer/VkLayer_khronos_validation.json'
    declaration=json.loads(manifest.read_text())['layer']
    library=Path(declaration['library_path'])
    assert declaration['name']=='VK_LAYER_KHRONOS_validation'
    assert file_sha(library)==layer['library_sha256']
    for path in (provenance,manifest,library):pins[str(path)]=file_sha(path)
    command=[str(binary),str(args.input),str(args.shaders),str(args.output)]
    log=Path(str(args.output)+'.log');receipt=Path(str(args.output)+'-receipt.json')
    assert not args.output.exists() and not log.exists() and not receipt.exists()
    environment=os.environ.copy();environment['VK_LAYER_PATH']=str(args.validation_layer/'layer')
    started=time.monotonic()
    with log.open('w') as output:
        completed=subprocess.run(command,stdout=output,stderr=subprocess.STDOUT,env=environment,
            preexec_fn=lambda:resource.setrlimit(resource.RLIMIT_CORE,(0,0)))
    assert all(file_sha(Path(name))==digest for name,digest in pins.items())
    result=dict(command=command,returncode=completed.returncode,seconds=time.monotonic()-started,
        input_manifest_sha256=file_sha(args.input),binary_sha256=build['binary_sha256'],
        build_sha256=file_sha(args.build/'build.json'),all_pinned_inputs_unchanged=True,
        linked_files_sha256=build['linked_files_sha256'],shader_files_sha256={str(args.shaders/name):pins[str(args.shaders/name)] for name in SHADERS},
        validation_layer_provenance_sha256=file_sha(provenance),validation_layer_library_sha256=file_sha(library),
        log_sha256=file_sha(log),core_dumps_disabled=True,tool_sha256=file_sha(Path(__file__)),
        full_weapon_parity=False,physical_xbox_verified=False)
    if (args.output/'report.json').exists():
        result['report_sha256']=file_sha(args.output/'report.json')
    receipt.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({name:result[name] for name in ('returncode','seconds','binary_sha256','all_pinned_inputs_unchanged')}))
    raise SystemExit(completed.returncode)


if __name__=='__main__':main()

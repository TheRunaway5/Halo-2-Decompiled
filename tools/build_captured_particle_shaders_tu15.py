#!/usr/bin/env python3
"""Compile current production particle shaders with authenticated disc programs.

Keep an immutable snapshot of every included GLSL input. The two exercised
Needler program identities must match the original map-byte evidence.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess

from audit_rendering import file_sha

ROOT=Path(__file__).resolve().parents[1]
STAGES={
    'particle.vert.spv':('particle.vert',None),
    'particle.geom.spv':('particle.geom',None),
    'particle.frag.spv':('particle.frag',None),
    'particle_directional.vert.spv':('particle.vert','PARTICLE_DIRECTIONAL'),
    'particle_perpendicular.vert.spv':('particle.vert','PARTICLE_PERPENDICULAR'),
    'particle_model.vert.spv':('particle.vert','PARTICLE_MODEL'),
    'particle_model.geom.spv':('particle.geom','PARTICLE_MODEL'),
    'particle_distortion.vert.spv':('particle.vert','PARTICLE_DISTORTION'),
}


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--draw-evidence',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();args.output=args.output.absolute()
    assert not args.output.exists();source=args.output/'source';source.mkdir(parents=True)
    manifest=ROOT/'shaders/native-programs/particles.json'
    catalog=json.loads(manifest.read_text());evidence=json.loads(args.draw_evidence.read_text())
    assert catalog['schema']=='H2_NATIVE_PARTICLE_SOURCES_V1' and evidence['passed']
    pins={str(manifest):file_sha(manifest),str(ROOT/'CMakeLists.txt'):file_sha(ROOT/'CMakeLists.txt')};programs={};identities=[]
    for entry in catalog['entries']:
        path=(manifest.parent/entry['source']).resolve()
        assert path.is_relative_to(manifest.parent) and file_sha(path)==entry['source_sha256']
        assert not entry['expected_vertices_embedded'] and path.name not in programs
        programs[path.name]=path;identities.append(entry)
    for row in evidence['source_programs']:
        match=[e for e in identities if e['classification']==row['classification'] and e.get('variant','standard')=='standard']
        assert len(match)==1 and match[0]['original_program_sha256']==row['source_sha256']
        assert match[0]['instructions']==row['instructions']
    copied={}

    def snapshot(path):
        path=path.resolve();name=path.name
        if name in copied:
            assert copied[name]==str(path);return
        copied[name]=str(path);raw=path.read_bytes();pins[str(path)]=file_sha(path)
        assert pins[str(path)]==hashlib.sha256(raw).hexdigest()
        (source/name).write_bytes(raw)
        for include in re.findall(r'^\s*#include\s+"([^"]+)"',raw.decode(),re.MULTILINE):
            assert Path(include).name==include
            child=ROOT/'shaders'/include
            snapshot(child if child.is_file() else programs[include])

    for name,_ in STAGES.values():snapshot(ROOT/'shaders'/name)
    compiler=Path(shutil.which('glslangValidator')).resolve()
    validator_name=shutil.which('spirv-val');validator=Path(validator_name).resolve() if validator_name else None
    commands=[];passed=True
    with (args.output/'build.log').open('w') as log:
        for output,(name,define) in STAGES.items():
            command=[str(compiler),'-V']+(['-D'+define+'=1'] if define else [])
            command+=['-I'+str(source),str(source/name),'-o',str(args.output/output)]
            commands.append(command)
            if subprocess.run(command,stdout=log,stderr=subprocess.STDOUT).returncode:passed=False;break
            if validator:
                command=[str(validator),str(args.output/output)];commands.append(command)
                if subprocess.run(command,stdout=log,stderr=subprocess.STDOUT).returncode:passed=False;break
    root_drift={name:file_sha(Path(name)) for name,digest in pins.items() if file_sha(Path(name))!=digest}
    report=dict(passed=passed and not root_drift,scope=__doc__,commands=commands,
        compiler=str(compiler),compiler_sha256=file_sha(compiler),
        spirv_validator=str(validator) if validator else None,
        spirv_validator_sha256=file_sha(validator) if validator else None,
        original_source_sha256=pins,snapshot_source_sha256={name:file_sha(source/name) for name in copied},
        root_drift=root_drift,exercised_original_program_bytes_authenticated=True,
        shader_files_sha256={name:file_sha(args.output/name) for name in STAGES if (args.output/name).exists()},
        original_programs=identities,draw_evidence_sha256=file_sha(args.draw_evidence),
        tool_sha256=file_sha(Path(__file__)),full_weapon_parity=False,physical_xbox_verified=False)
    (args.output/'build.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(passed=report['passed'],compiled_shaders=len(report['shader_files_sha256']),
        snapshot_files=len(copied),root_drift=bool(root_drift))))
    raise SystemExit(0 if report['passed'] else 1)


if __name__=='__main__':main()

#!/usr/bin/env python3
"""Stage the declared production shaders, rejecting executable interpreters."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil

from native_renderer_shaders import AOT_PROFILE, AOT_PACKAGE_SCHEMA, native_module_contract

ALIASES={name+'.vert.spv':'particle.vert' for name in
         ('particle_directional','particle_distortion','particle_model','particle_perpendicular')}
ALIASES['particle_model.geom.spv']='particle.geom'
ALIASES['path_tracing/lighting.frag.spv']='path_tracing/world.frag'

def relative_shader(name):
    if not isinstance(name,str):raise ValueError('Production shader name is not a string')
    path=Path(name)
    if path.is_absolute() or '..' in path.parts or path.suffix!='.spv' or path.as_posix()!=name:
        raise ValueError('Production shader path is not canonical inside its owner')
    return path

def expected_inventory(build,read):
    cache=build/'CMakeCache.txt'
    values=re.findall(r'^CMAKE_HOME_DIRECTORY:INTERNAL=(.+)$',read(cache).decode(),re.MULTILINE)
    if len(values)!=1:raise ValueError('Build has no unique frozen source owner')
    source=Path(values[0])
    if not source.is_absolute() or source.resolve(strict=True)!=source:
        raise ValueError('Frozen source owner is not canonical')
    policy=source/'cmake/NativeRendererProductionShaders.cmake'
    text=re.sub(r'#[^\n]*','',read(policy).decode())
    definitions=re.findall(r'\bset\s*\(\s*native_renderer_fixed_shaders\s+([^)]*)\)',text,re.DOTALL)
    if len(definitions)!=1:raise ValueError('Production source policy lacks one complete fixed family')
    fixed=definitions[0].split()
    if not fixed or len(fixed)!=len(set(fixed)):raise ValueError('Production source policy duplicates or omits its fixed family')
    expected={}
    for name in fixed:
        relative_shader(name)
        # A literal fixed module needs an actual production GLSL source.
        # Named particle variants are compile definitions of these two files.
        origin=source/'shaders'/ALIASES.get(name,name[:-4])
        read(origin)
        expected[name]=None
    specifications=(
        ('native-world-kernel-catalog/compiled.json','HALO2_NATIVE_WORLD_KERNEL_COMPILED_V1','stages','native_world/'),
        ('native-fragment-catalog/compiled.json','HALO2_COMPILED_NATIVE_FRAGMENT_KERNELS_V1','modules','native-fragments/'))
    for name,schema,field,prefix in specifications:
        manifest=json.loads(read(build/name))
        if manifest.get('schema')!=schema or not isinstance(manifest.get(field),list):
            raise ValueError('Production native compiler manifest has another schema')
        if field=='modules' and manifest.get('passed')is not True:
            raise ValueError('Native fragment compilation did not pass')
        if field=='stages' and manifest.get('runtime_instruction_interpreter')is not False:
            raise ValueError('Native world compilation retained an instruction interpreter')
        for row in manifest[field]:
            path=row.get('path');relative_shader(path)
            if not path.startswith(prefix) or path in expected:
                raise ValueError('Native compiler duplicated or changed a program owner')
            size,pin=row.get('bytes'),row.get('sha256')
            if type(size)is not int or not 0<size<=32*1024*1024 or not isinstance(pin,str) or not re.fullmatch('[0-9a-f]{64}',pin):
                raise ValueError('Native compiler manifest lacks a complete immutable module identity')
            expected[path]=(size,pin)
    return expected

def stage_production_shaders(build, output):
    build=Path(build).resolve(strict=True);output=Path(output)
    if output.exists() or output.is_symlink():raise ValueError('Production shader staging requires a new owner')
    pins={}
    def read(path):
        if not path.is_file() or path.is_symlink() or path.resolve(strict=True)!=path:
            raise ValueError('Missing direct production source or shader: '+str(path))
        if path.stat().st_size>64*1024*1024:raise ValueError('Production ownership input exceeds its bound')
        raw=path.read_bytes();stat=path.stat()
        pin=(stat.st_dev,stat.st_ino,stat.st_size,stat.st_mtime_ns,hashlib.sha256(raw).hexdigest())
        if path in pins and pins[path]!=pin:raise ValueError('Production ownership input changed during preparation')
        pins[path]=pin
        return raw
    expected=expected_inventory(build,read)
    inventory=build/'production-shaders.txt'
    names=read(inventory).decode().splitlines()
    if not names or names!=sorted(set(names)):
        raise ValueError('Invalid production shader inventory')
    for name in names:relative_shader(name)
    if set(names)!=set(expected):raise ValueError('Generated production shader inventory differs from its authored/compiler owners')
    rows=[]
    for name in names:
        source=build/'shaders'/relative_shader(name)
        raw=read(source);pin=hashlib.sha256(raw).hexdigest()
        if expected[name]is not None and expected[name]!=(len(raw),pin):
            raise ValueError('Production shader differs from its native compiler identity: '+name)
        contract=native_module_contract(raw,name)
        rows.append({'path':name,'bytes':len(raw),'sha256':pin,
                     'native_contract':contract})
    output.mkdir(parents=True,exist_ok=False)
    try:
        for row in rows:
            target=output/row['path'];target.parent.mkdir(parents=True,exist_ok=True)
            shutil.copy2(build/'shaders'/row['path'],target)
            if hashlib.sha256(target.read_bytes()).hexdigest()!=row['sha256']:
                raise ValueError('Production shader changed during copy')
        for path in tuple(pins):read(path)
        for row in rows:
            if hashlib.sha256((output/row['path']).read_bytes()).hexdigest()!=row['sha256']:
                raise ValueError('Production shader output changed during staging')
        report={'schema':AOT_PACKAGE_SCHEMA,'profile':AOT_PROFILE,'passed':True,
                'runtime_instruction_interpreter':False,
                'inventory_sha256':pins[inventory][-1],
                'ownership_pins':{str(path):{'bytes':pin[2],'sha256':pin[-1]} for path,pin in pins.items()},
                'modules':rows,'module_count':len(rows)}
        (output/'production-shaders.json').write_text(json.dumps(report,indent=2)+'\n')
        return report
    except BaseException:
        shutil.rmtree(output)
        raise

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--build-dir',type=Path,required=True)
    p.add_argument('--output-dir',type=Path,required=True)
    args=p.parse_args();result=stage_production_shaders(args.build_dir,args.output_dir)
    print(json.dumps({'passed':True,'module_count':result['module_count']}))

if __name__=='__main__':main()

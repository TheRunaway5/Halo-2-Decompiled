#!/usr/bin/env python3
"""Compile complete authored fragment identities into fixed Vulkan modules.

The original evaluators are expanded only by this build tool. Program control
words become literals before compilation and optimization; only color data
remains in the unchanged uniform layout. Runtime selection compares full keys.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import struct
import subprocess
import sys
import time

from native_world_fragment_kernel import (lower_combiner_program,
    lower_fragment_program, build_fragment_source)
from compile_native_world_kernels import atomic
from audit_native_renderer_kernels import audit_spirv

COLORS=frozenset((*range(10,26),43,44))
FAMILIES={
    'world':'world.frag','world-fixed':'world.frag','world-emblem':'world.frag','world-source-depth':'world.frag',
    'hud':'hud.frag','menu':'menu_world.frag','menu-temporal':'menu_world.frag',
    'particle':'particle.frag','scene-copy':'scene_copy.frag',
    'scope-copy':'scope_copy.frag','lens':'source_lens_draw.frag',
    'contrail':'source_contrail.frag','decal':'source_decal.frag',
    'light-volume':'light_volume.frag',
    'pstencil-normal':'pstencil/normal_accumulation.frag',
    'pstencil-normal-utility':'pstencil/normal_light_utility.frag',
    'pstencil-texture':'pstencil/texture_accumulation.frag',
    'pstencil-texture-source-depth':'pstencil/texture_accumulation.frag',
    'pstencil-light-utility':'pstencil/light_utility.frag',
    'pstencil-dependent-light':'pstencil/dependent_light.frag',
    'pstencil-dependent-light-source-depth':'pstencil/dependent_light.frag',
    'fog':'fog.frag','bloom':'bloom.frag','patchy-noise':'patchy_noise.frag',
    'patchy-composite':'patchy_composite.frag','bsp-patchy-noise':'bsp_patchy_noise.frag',
    'bsp-patchy-composite':'bsp_patchy_composite.frag',
    'cinematic-crossfade':'cinematic_crossfade.frag',
}

def controls(value):
    if not isinstance(value,list) or len(value)!=60 or any(type(x)is not int or not 0<=x<=0xffffffff for x in value):
        raise ValueError('Native fragment requires all 60 uint32 source words')
    return [0 if i in COLORS else x for i,x in enumerate(value)]

def key(words):
    return hashlib.sha256(struct.pack('<60I',*words)).hexdigest()

def entries(paths):
    result={}
    for path in paths:
        raw=json.loads(path.read_bytes())
        if raw.get('schema')!='HALO2_NATIVE_FRAGMENT_PROGRAMS_V1':
            raise ValueError('Unknown native fragment registry schema: '+str(path))
        for entry in raw['entries']:
            family=entry['family']
            if family not in FAMILIES:raise ValueError('Unknown native fragment family: '+family)
            words=controls(entry['controls']);identity=(family,key(words))
            if not entry.get('provenance'):raise ValueError('Native fragment lacks source provenance')
            row={'family':family,'controls':words,'key':identity[1],
                 'path':f'native-fragments/{family}/{identity[1]}.frag.spv'}
            result[identity]=row
            # The source W-depth variant uses the same complete authored
            # combiner identity; only its raster/depth adapter differs.
            if family=='world':
                depth={**row,'family':'world-source-depth',
                    'path':f'native-fragments/world-source-depth/{identity[1]}.frag.spv'}
                result[('world-source-depth',identity[1])]=depth
            if family in ('pstencil-texture','pstencil-dependent-light'):
                depth_family=family+'-source-depth'
                result[(depth_family,identity[1])]={**row,'family':depth_family,
                    'path':f'native-fragments/{depth_family}/{identity[1]}.frag.spv'}
            if family=='menu':
                temporal={**row,'family':'menu-temporal',
                    'path':f'native-fragments/menu-temporal/{identity[1]}.frag.spv'}
                result[('menu-temporal',identity[1])]=temporal
    if not result:raise ValueError('Native fragment registry has no source-owned programs')
    return [result[k] for k in sorted(result)]

def inline_source(path,shader_root,words,stack=(),overrides=None,*,preserve_source_operations=False):
    """Preserve family wrappers while replacing the executable evaluators."""
    if path in stack:raise ValueError('Recursive shader include')
    source=overrides[path] if overrides and path in overrides else path.read_text()
    def replace(match):
        name=match.group(1)
        if name=='world_combiner_specialization.glsl':return ''
        if name=='combiner.glsl':
            prefix='''#ifndef H2_COMBINER_CONSTANT
#ifdef H2_COMBINER_WORD
#define H2_COMBINER_CONSTANT(index) argb(H2_COMBINER_WORD(index))
#else
#define H2_COMBINER_CONSTANT(index) argb(material.program[(index)/4u][(index)%4u])
#endif
#endif
'''
            return prefix+lower_combiner_program(words,
                color_expression=lambda i:f'H2_COMBINER_CONSTANT({i}u)',
                preserve_source_operations=preserve_source_operations)
        if name=='texture_program.glsl':
            lowered=lower_fragment_program(words)
            modes=lowered['modes']
            return '''#ifndef TEXTURE_COORDINATE_W
#define TEXTURE_COORDINATE_W(i) stageCoordinates[i].w
#endif
#ifndef TEXTURE_VOLUME
#define TEXTURE_VOLUME(i) false
#endif
#ifndef TEXTURE_PROJECTED_2D
#define TEXTURE_PROJECTED_2D(i) fetchStage2D(i,stageCoordinates[i].xy/TEXTURE_COORDINATE_W(i))
#endif
'''+f'uint textureMode(uint stage){{const uvec4 modes=uvec4({",".join(str(x)+"u" for x in modes)});return modes[stage];}}\n'+lowered['texture_glsl']
        included=path.parent/name
        if not included.is_file():included=shader_root/name
        if not included.is_file():raise ValueError('Missing source shader include: '+name)
        return inline_source(included,shader_root,words,(*stack,path),overrides,
            preserve_source_operations=preserve_source_operations)
    source=re.sub(r'^\s*#include\s+"([^"]+)"\s*$',replace,source,flags=re.MULTILINE)
    return source

def source_for(shader_root,row):
    family=row['family'];words=row['controls']
    if family.startswith('world'):
        source=build_fragment_source((shader_root/'world.frag').read_text(),
            lower_fragment_program(words),authored_vertex=family!='world-fixed')
        if family=='world-emblem':
            source=source.replace('#version 450','#version 450\n#define H2_EMBLEM_W_DEPTH 1',1)
        if family=='world-source-depth':
            source=source.replace('#version 450','#version 450\n#define H2_SOURCE_WORLD_DEPTH 1',1)
        return source
    path=shader_root/FAMILIES[family];overrides={}
    if family in ('pstencil-texture','pstencil-texture-source-depth'):
        # This source adapter has two independently authored combiner phases.
        # Lower each phase separately; never retain its runtime word callback.
        original_source=(shader_root/'pstencil/texture_accumulation_2003_program.glsl').read_text()
        original=[int(x,16) for x in re.findall(r'0x([0-9a-fA-F]+)u',original_source)]
        if len(original)!=60 or hashlib.sha256(struct.pack('<60I',*original)).hexdigest()!='cba50a5a3d7fe85d954df37d2cb0d575253907b77dbb4baa442e40c449f6f848':
            raise ValueError('Original builtin3 complete program identity changed')
        fixed=lower_combiner_program(original,color_expression=lambda i:f'argb({original[i]}u)')
        for old,new in (('runCombiners','runOriginalCombiners'),('registers','originalRegisters'),('argb','originalArgb')):
            fixed=re.sub(r'\b'+old+r'\b',new,fixed)
        wrapper=path.read_text()
        wrapper=re.sub(r'^uint textureAccumulationWord\(uint index\);\s*$','',wrapper,flags=re.MULTILINE)
        wrapper=wrapper.replace('#define H2_COMBINER_WORD(index) textureAccumulationWord(index)','').replace('#undef H2_COMBINER_WORD','')
        wrapper=wrapper.replace('#include "pstencil/texture_accumulation_2003_program.glsl"','')
        wrapper=re.sub(r'bool originalPhase=false;\s*uint textureAccumulationWord\(uint index\)\s*\{[^}]+\}','',wrapper)
        wrapper=wrapper.replace('originalPhase=true;','')
        wrapper=wrapper.replace('pixel=runCombiners(samples,colorFactor,','pixel=runOriginalCombiners(samples,colorFactor,')
        wrapper=wrapper.replace('void main() {',fixed+'\nvoid main() {')
        overrides[path]=wrapper
    source=inline_source(path,shader_root,words,overrides=overrides,
        preserve_source_operations=family=='fog')
    if family in ('pstencil-texture-source-depth','pstencil-dependent-light-source-depth'):
        source=source.replace('#version 450','#version 450\n#define H2_SOURCE_WORLD_DEPTH 1',1)
    if family.startswith('pstencil-'):
        source=re.sub(r'\bmaterial\.flags\.w\s*!=\s*0u','true',source)
    if family in ('menu','menu-temporal'):
        if family=='menu-temporal':
            source=source.replace('#version 450','#version 450\n#define H2_NATIVE_RASTER_OFFSET 1',1)
        source=re.sub(r'struct MenuWorldPixelProgramView\s*\{[^}]+\};\s*MenuWorldPixelProgramView menuWorldPixelProgram;','',source)
        source=source.replace('#define material menuWorldPixelProgram','').replace('#undef material','')
        source=re.sub(r'H2_FIXED_STAGE_UNROLL\s+for\(uint i=0u;i<15u;\+\+i\)\s*menuWorldPixelProgram\.program\[i\]=uvec4\([^;]+;', '',source)
        # The volume projection adapter reads this one control word directly
        # in the shared template; its complete mode identity is fixed here.
        source=source.replace('material.program[13][2]',str(words[54])+'u')
    def literal(match):
        index=int(match.group(1))
        if not 0<=index<60:raise ValueError('Fragment word read escapes source definition')
        return f'material.program[{index//4}][{index%4}]' if index in COLORS else f'{words[index]}u'
    source=re.sub(r'\bword\s*\(\s*(\d+)u?\s*\)',literal,source)
    if re.search(r'\bword\s*\(|\bMENU_WORLD_PROGRAM_WORD\s*\(',source):
        # The unused hook definition may remain; no executable hook call may.
        executable='\n'.join(line for line in source.splitlines() if not line.lstrip().startswith('#'))
        if re.search(r'\bword\s*\(|\bMENU_WORLD_PROGRAM_WORD\s*\(',executable):
            raise ValueError('Family retained runtime program decoding: '+family)
    return source

def run(argv):
    result=subprocess.run([str(x) for x in argv],capture_output=True,text=True)
    if result.returncode:raise RuntimeError('Native fragment build failed: '+(result.stdout+result.stderr)[-6000:])

def write_header(path,rows):
    header=['#pragma once','#include <array>','#include <cstdint>','#include <string_view>',
        'namespace halo2::generated_native_fragment_kernels {',
        'struct Entry{std::string_view family;std::array<std::uint32_t,60> controls;std::string_view path;};',
        f'inline constexpr std::array<Entry,{len(rows)}> entries{{{{']
    for row in rows:
        header.append('{"'+row['family']+'",{{'+','.join(str(x)+'u' for x in row['controls'])+'}},"'+row['path']+'"},')
    header+=['}};','}']
    atomic(path,('\n'.join(header)+'\n').encode())

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--registry',type=Path,action='append',required=True)
    p.add_argument('--shader-root',type=Path,required=True)
    p.add_argument('--shader-output-dir',type=Path,required=True)
    p.add_argument('--header',type=Path)
    p.add_argument('--manifest',type=Path)
    p.add_argument('--compiler',type=Path)
    p.add_argument('--optimizer',type=Path)
    p.add_argument('--validator',type=Path)
    p.add_argument('--list-outputs',action='store_true')
    p.add_argument('--header-only',action='store_true')
    args=p.parse_args();rows=entries(args.registry)
    if args.list_outputs:
        print(';'.join(str(args.shader_output_dir/r['path']) for r in rows));return
    if args.header_only:
        if not args.header:p.error('Catalog generation requires --header')
        write_header(args.header,rows)
        # The header-only CMake rule has this content-stable file as OUTPUT.
        # Complete the rule even when the identical catalog was revalidated.
        completed_ns=time.time_ns();os.utime(args.header,ns=(completed_ns,completed_ns))
        return
    if not all((args.header,args.manifest,args.compiler,args.optimizer,args.validator)):
        p.error('Compilation requires output header,manifest and complete compiler toolchain')
    generated=args.header.parent/'sources';generated.mkdir(parents=True,exist_ok=True)
    manifest=[]
    for row in rows:
        name=row['family']+'-'+row['key']
        source=generated/(name+'.frag');temporary=generated/(name+'.spv')
        output=args.shader_output_dir/row['path'];output.parent.mkdir(parents=True,exist_ok=True)
        atomic(source,source_for(args.shader_root,row).encode())
        run([args.compiler,'-V','--target-env','vulkan1.3','-I'+str(args.shader_root),source,'-o',temporary])
        run([args.optimizer,'--preserve-interface','-O',temporary,'-o',output])
        run([args.validator,'--target-env','vulkan1.3',output])
        contract=audit_spirv(output.read_bytes())
        if not contract['passed']:raise ValueError('Compiled fragment retains instruction reads: '+str(contract))
        manifest.append({**row,'sha256':hashlib.sha256(output.read_bytes()).hexdigest(),'bytes':output.stat().st_size,'native_contract':contract})
    write_header(args.header,rows)
    atomic(args.manifest,(json.dumps({'schema':'HALO2_COMPILED_NATIVE_FRAGMENT_KERNELS_V1','passed':True,'modules':manifest},indent=2)+'\n').encode())
    # CMake uses this manifest as the successful completion output. Identical
    # revalidation must still become newer than its changed dependencies.
    # Publish one completion time only after every module passed its audits;
    # the content-stable catalog header retains its existing timestamp.
    completed_ns=time.time_ns()
    for path in [args.manifest,*(args.shader_output_dir/row['path'] for row in rows)]:
        os.utime(path,ns=(completed_ns,completed_ns))

if __name__=='__main__':main()

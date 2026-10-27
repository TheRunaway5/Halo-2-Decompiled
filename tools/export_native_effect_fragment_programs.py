#!/usr/bin/env python3
"""Export finite effect/copy fragment controls from actual native TU1.5 producers.

Explicit source maintenance, never a normal-build or runtime dependency. Run
through tools/run_test.py so the compiler/probe products have an owned lifetime.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess

ROOT=Path(__file__).resolve().parents[1]
FILES=(
    'tools/export_native_effect_fragment_programs.cpp',
    'src/rendering/source_scene_copy.cpp', 'src/rendering/scope_copy.cpp',
    'src/rendering/effects/source_decal_state.cpp',
    'src/rendering/effects/source_contrail_state.cpp',
    'src/ui/hud/motion_sensor.cpp', 'include/halo2/motion_sensor.hpp',
    'include/halo2/source_decal_pass.hpp', 'include/halo2/source_lens_program_tu15.hpp')
SITES={'scene-copy':'TU1.5 352D0, retail352E0 four-tap program',
       'scope-copy':'TU1.5/retail2B000 copy program',
       'lens':'Authenticated original2DEE0 literal stores; source_lens_program_tu15.hpp',
       'decal':'TU1.5 43A30 and43D13/43DA9 complete layers0..5',
       'world-fixed':'TU1.5 43A30 projected layers2..5, NativeWorldEffects fixed-input world material consumer',
       'hud':'TU1.5 25E5C0 final motion sensor program bind25ED28; retail2548F0 bind255058, opacity1',
       'contrail':'TU1.5 52D30 literal stores; finite blend/properties/declaration branches'}
ROUTING_FILES={'world-fixed':('src/rendering/effects/world_effects.cpp',),
               'hud':('src/ui/hud/multiplayer_hud.cpp',)}
ROUTING_BLOCKS={'world-fixed':(
    ('src/rendering/renderer/renderer.cpp',
     '    void make_native_forward_pipeline(ForwardPipeline& p) {',
     '        std::array<VkShaderModule,3> modules{};'),
    ('src/rendering/renderer/renderer.cpp',
     '        const auto draw_decals=[&](const std::vector<Draw>& list) {',
     '        auto draw_texture_mapping_overlays =')),
    'hud':(('src/rendering/renderer/renderer.cpp',
     '        for(unsigned material=0;material<hud_materials.size();++material)if(hud_materials[material]) {',
     '            const auto hud_fragment=shader(std::string(path).c_str());'),)}

def routing_evidence(family):
    result=[]
    for name,begin,end in ROUTING_BLOCKS.get(family,()):
        source=(ROOT/name).read_text()
        first=source.index(begin);last=source.index(end,first)
        result.append({'source':name,'begin':begin,'end_exclusive':end,
                       'sha256':hashlib.sha256(source[first:last].encode()).hexdigest()})
    return result

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--report',type=Path,required=True)
    args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    binary=scratch/'export-native-effect-fragments'
    command=['c++','-std=c++20','-O1','-ffp-contract=off','-ffunction-sections','-fdata-sections',
             '-I'+str(ROOT/'include'),*[str(ROOT/name) for name in FILES if name.endswith('.cpp')],
             '-Wl,--gc-sections','-o',str(binary)]
    result=subprocess.run(command,capture_output=True,text=True)
    if result.returncode:
        raise RuntimeError(result.stderr[-6000:])
    result=subprocess.run([str(binary)],capture_output=True,text=True,check=True)
    rows=json.loads(result.stdout)
    pins={name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest() for name in FILES}
    entries={}
    for row in rows:
        words=row['words']
        if len(words)!=60 or any(type(word)!=int or not 0<=word<=0xffffffff for word in words):
            raise ValueError('Native producer emitted an invalid complete program')
        controls=[0 if 10<=i<=25 or i in (43,44) else word for i,word in enumerate(words)]
        key=(row['family'],tuple(controls))
        if key not in entries:
            source_files={**pins,**{name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest()
                for name in ROUTING_FILES.get(row['family'],())}}
            entries[key]={'family':row['family'],'controls':controls,
                          'provenance':{'authority':SITES[row['family']],
                                        'source_files':source_files,'producer_arguments':[]}}
            routes=routing_evidence(row['family'])
            if routes:entries[key]['provenance']['runtime_routes']=routes
        entries[key]['provenance']['producer_arguments'].append(row['arguments'])
    registry={'schema':'HALO2_NATIVE_FRAGMENT_PROGRAMS_V1','entries':list(entries.values())}
    encoded=(json.dumps(registry,indent=2)+'\n').encode()
    if len(encoded)>1024*1024 or args.output.is_symlink() or args.report.is_symlink():
        raise ValueError('Export output exceeds its bounded source owner')
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_bytes(encoded)
    receipt={'passed':True,'producer_cases':len(rows),'unique_programs':len(entries),
             'counts':{family:sum(row['family']==family for row in entries.values()) for family in SITES},
             'source_files':pins,'registry':str(args.output),'registry_sha256':hashlib.sha256(encoded).hexdigest(),
             'scratch_owner':str(scratch),'compiler_products_lifecycle':'Removed by enclosing tools/run_test.py'}
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps({key:receipt[key] for key in ('passed','producer_cases','unique_programs','counts')}))

if __name__=='__main__':main()

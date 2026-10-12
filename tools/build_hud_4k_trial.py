#!/usr/bin/env python3
"""Link the isolated HUD trial against the installed runtime's verified objects.

Rebuild the HUD units and the current compatible UI pack reader. Copy the base runtime
objects, libraries and headers for reproducibility, retain their hashes, and
leave ordinary launchers pointed at their existing runtime. No class layouts
or other runtime translation units are changed by this trial.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(command):
    subprocess.run([str(x) for x in command], check=True)


def link_binary(target, inputs):
    temporary=target.with_suffix('.linking')
    run(['c++','-O3','-o',temporary,*inputs])
    temporary.replace(target)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--base-build',type=Path,required=True)
    parser.add_argument('--output',type=Path,default=ROOT/'build-hud-4k-trial')
    parser.add_argument('--overlay-source',type=Path,default=ROOT/'analysis/multiplayer/hud-4k/verified-source')
    args=parser.parse_args();base=args.base_build.resolve();out=args.output.resolve()
    overlay=args.overlay_source.resolve()
    cache=(base/'CMakeCache.txt').read_text()
    source=Path(re.search(r'^CMAKE_HOME_DIRECTORY:INTERNAL=(.+)$',cache,re.M).group(1))
    ninja=(base/'build.ninja').read_text()
    block=ninja.split('build halo2_native: ',1)[1].split('\n\n',1)[0]
    libraries=re.search(r'^  LINK_LIBRARIES = (.+)$',block,re.M).group(1).split()
    objects=block.split(' | ',1)[0].split()[1:]
    out.mkdir(parents=True,exist_ok=True)
    shutil.copytree(source/'include',out/'include',dirs_exist_ok=True)
    shutil.copyfile(overlay/'include/halo2/motion_sensor.hpp',out/'include/halo2/motion_sensor.hpp')
    shutil.copyfile(overlay/'include/halo2/frontend_font_sampling.hpp',out/'include/halo2/frontend_font_sampling.hpp')
    shutil.copytree(base/'shaders',out/'shaders',dirs_exist_ok=True)
    inputs={str(overlay/'include/halo2/motion_sensor.hpp'):sha(overlay/'include/halo2/motion_sensor.hpp'),
            str(overlay/'include/halo2/frontend_font_sampling.hpp'):sha(overlay/'include/halo2/frontend_font_sampling.hpp')};link=[]
    for name in objects+libraries:
        path=Path(name)
        if path.is_absolute():
            link.append(path);continue
        original=base/path;target=out/'base'/path
        target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(original,target)
        inputs[str(original)]=sha(original);link.append(target)
    common=['c++','-O3','-DNDEBUG','-std=c++20','-I'+str(out/'include')]
    built=[]
    for name in ('motion_sensor','multiplayer_hud','frontend_ui_assets','frontend_font','frontend_font_sampling','multiplayer_scoreboard'):
        target=out/(name+'.o');original=overlay/'src'/(name+'.cpp')
        run(common+['-ffp-contract=off','-c',original,'-o',target]);built.append(target)
        inputs[str(original)]=sha(original)
    link_binary(out/'halo2_native',[*built,*link])
    # An independent, unchanged pre-trial implementation checks the default
    # radar byte-for-byte. Only its public symbol is renamed for this executable.
    original=(source/'src/motion_sensor.cpp').read_text()
    routine=original[original.index('Texture source_motion_sensor_composite('):original.rfind('} // namespace halo2')]
    routine=routine.replace('source_motion_sensor_composite(', 'baseline_motion_sensor_composite(',1)
    oracle=out/'baseline_motion_sensor.cpp'
    oracle.write_text('#include "halo2/motion_sensor.hpp"\n#include <algorithm>\n#include <cmath>\nnamespace halo2 {\n'+routine+'}\n')
    oracle_object=out/'baseline_motion_sensor.o';run(common+['-ffp-contract=off','-c',oracle,'-o',oracle_object])
    library_link=link[len(objects):]
    run(common+['-c',overlay/'tools/hud_4k_codec.cpp','-o',out/'hud_4k_codec.o'])
    link_binary(out/'hud_4k_codec',[out/'hud_4k_codec.o',*built,*library_link])
    flags=subprocess.check_output(['pkg-config','--cflags','sdl2'],text=True).split()
    for name in ('hud_4k_tests','hud_vista_artwork_tests','hud_gameplay_tests','hud_render_tests','hud_vista_crosshair_tests','frontend_font_sampling_tests'):
        target=out/(name+'.o')
        run(common+flags+['-DHALO2_SHADER_DIR="'+str(out/'shaders')+'"','-c',overlay/'tests'/(name+'.cpp'),'-o',target])
        additional=[oracle_object] if name=='hud_4k_tests' else []
        link_binary(out/name,[target,*additional,*built,*library_link])
    report=dict(base_runtime=str(base/'halo2_native'),base_runtime_sha256=sha(base/'halo2_native'),
                source_snapshot=str(source),overlay_source=str(overlay),inputs=inputs,trial_runtime_sha256=sha(out/'halo2_native'),
                modified_units=['motion_sensor.cpp','multiplayer_hud.cpp','frontend_font.cpp','frontend_font_sampling.cpp','multiplayer_scoreboard.cpp'],
                current_pack_compatibility_unit='frontend_ui_assets.cpp',class_layout_changes=False)
    (out/'build-provenance.json').write_text(json.dumps(report,indent=2)+'\n')
    print(out/'halo2_native')


if __name__=='__main__':
    main()

#!/usr/bin/env python3
"""Compile the current live-world translation unit with its recorded build flags."""
import argparse
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
from original_xbox_pose_case import ROOT,sha


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);output=args.output.absolute();output.mkdir(exist_ok=False)
    flags=ROOT/'build-multiplayer/CMakeFiles/halo2_systems.dir/flags.make'
    values={}
    for line in flags.read_text().splitlines():
        if line.startswith(('CXX_DEFINES =','CXX_INCLUDES =','CXX_FLAGS =')):
            key,value=line.split('=',1);values[key.strip()]=shlex.split(value)
    source=ROOT/'src/physics_live_world_tu15.cpp'
    inputs=[source,ROOT/'src/source_sound_cache_io_tu15.cpp',ROOT/'include/halo2/source_sound_cache_io_tu15.hpp',
        ROOT/'src/source_sound_cache_query_tu15_generated.cpp',ROOT/'include/halo2/source_sound_cache_query_tu15.hpp',
        ROOT/'src/source_sound_scalar_tu15_generated.cpp',ROOT/'include/halo2/source_sound_scalar_tu15.hpp',
        ROOT/'src/source_sound_listener_selection_tu15_generated.cpp',ROOT/'include/halo2/source_sound_listener_selection_tu15.hpp',
        ROOT/'src/source_sound_gain_tu15_generated.cpp',ROOT/'include/halo2/source_sound_gain_tu15.hpp',
        ROOT/'src/source_sound_random_scalar_tu15_generated.cpp',ROOT/'include/halo2/source_sound_random_scalar_tu15.hpp',
        ROOT/'src/source_sound_properties_tu15_generated.cpp',ROOT/'include/halo2/source_sound_properties_tu15.hpp',
        ROOT/'src/source_sound_random_short_tu15_generated.cpp',ROOT/'include/halo2/source_sound_random_short_tu15.hpp',
        ROOT/'src/source_sound_range_selection_tu15_generated.cpp',ROOT/'include/halo2/source_sound_range_selection_tu15.hpp',
        ROOT/'src/source_sound_permutation_tu15_generated.cpp',ROOT/'include/halo2/source_sound_permutation_tu15.hpp',
        ROOT/'src/source_sound_binding_tu15_generated.cpp',ROOT/'include/halo2/source_sound_binding_tu15.hpp',
        ROOT/'src/source_sound_chunk_readiness_tu15_generated.cpp',ROOT/'include/halo2/source_sound_chunk_readiness_tu15.hpp',
        ROOT/'cmake/PhysicsLiveWorldTu15.cmake']
    pins={str(path):sha(path)for path in inputs}
    command=['c++',*values['CXX_DEFINES'],*values['CXX_INCLUDES'],*values['CXX_FLAGS'],'-fsyntax-only',str(source)]
    (output/'attempt.json').write_text(json.dumps(dict(runner_scratch=str(scratch),command=command),indent=2)+'\n')
    result=subprocess.run(command,capture_output=True,text=True,cwd=scratch,timeout=180)
    (output/'compile.log').write_text((result.stdout+result.stderr)[-65536:])
    for path,pin in pins.items():assert sha(path)==pin,path
    report=dict(passed=result.returncode==0,command=command,exit_code=result.returncode,
        source_files_sha256=pins,baseline_flags_sha256=sha(flags),current_root_translation_unit_compiled=True,
        syntax_only=True,full_application_link_or_runtime_verified=False,runner_scratch=str(scratch),
        persistent_objects_or_binaries_retained=False,full_weapon_parity=False,tool_sha256=sha(Path(__file__)))
    (output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    for path in [Path(__file__),*inputs]:shutil.copyfile(path,output/path.name)
    print(json.dumps(dict(passed=report['passed'],exit_code=result.returncode,runner_scratch=str(scratch))),flush=True)
    assert report['passed'],(result.stdout+result.stderr)[-3000:]


if __name__=='__main__':main()

#!/usr/bin/env python3
"""Link a fixed-cadence timing probe from one owned native application build.

Use tools/run_test.py; --build must belong to that same live scratch owner.
No production object, executable or dependency file is replaced.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess


def sha(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build',type=Path,required=True)
    parser.add_argument('--report',type=Path,required=True)
    args=parser.parse_args()
    if 'HALO2_TEST_SCRATCH' not in os.environ:
        parser.error('use tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    build=args.build.resolve(strict=True)
    if not build.is_relative_to(scratch):
        parser.error('the build must belong to this active scratch owner')
    target=build/'CMakeFiles/halo2_native.dir'
    link=shlex.split((target/'link.txt').read_text())
    flags={}
    for line in (target/'flags.make').read_text().splitlines():
        if line.startswith(('CXX_DEFINES =','CXX_INCLUDES =','CXX_FLAGS =')):
            key,value=line.split('=',1);flags[key.strip()]=shlex.split(value)
    runtime_object=target/'src/gameplay/multiplayer/multiplayer_runtime.cpp.o'
    names=[line.split()[-1] for line in subprocess.check_output(['nm','-g','--defined-only',str(runtime_object)],text=True).splitlines() if line.split()]
    decoded=subprocess.check_output(['c++filt'],input='\n'.join(names)+'\n',text=True).splitlines()
    matches=[name for name,decoded_name in zip(names,decoded) if decoded_name==
             'halo2::run_multiplayer(halo2::MultiplayerOptions const&, halo2::MultiplayerRuntimeOutcome&)']
    if len(matches)!=1:
        raise RuntimeError('the frozen runtime has no unique expected multiplayer entry ABI')
    symbol=matches[0]
    context_object=build/'CMakeFiles/halo2_first_person_context_tu15.dir/src/gameplay/animation/source_first_person_context_tu15.cpp.o'
    fp_names=[line.split()[-1] for line in subprocess.check_output(['nm','-g','--defined-only',str(context_object)],text=True).splitlines() if line.split()]
    fp_decoded=subprocess.check_output(['c++filt'],input='\n'.join(fp_names)+'\n',text=True).splitlines()
    fp_matches=[name for name,decoded_name in zip(fp_names,fp_decoded)
                if decoded_name.startswith('halo2::SourceFirstPersonContextTu15::adopt_local(')]
    if len(fp_matches)!=1:
        raise RuntimeError('the frozen runtime has no unique expected first-person adoption ABI')
    fp_symbol=fp_matches[0]
    source=Path(__file__).with_name('native_cache_publication_probe.cpp')
    output=build/'halo2_native_fixed_tick_probe'
    obj=build/'native_cache_publication_probe.o'
    compile_command=[link[0],*flags['CXX_DEFINES'],*flags['CXX_INCLUDES'],*flags['CXX_FLAGS'],
                     '-DHALO2_NATIVE_CACHE_REAL_SYMBOL="__real_'+symbol+'"',
                     '-DHALO2_NATIVE_CACHE_WRAP_SYMBOL="__wrap_'+symbol+'"',
                     '-DHALO2_NATIVE_FP_REAL_SYMBOL="__real_'+fp_symbol+'"',
                     '-DHALO2_NATIVE_FP_WRAP_SYMBOL="__wrap_'+fp_symbol+'"',
                     '-c',str(source),'-o',str(obj)]
    # Retain the original main and every native object/library; intercept only
    # its external call into the multiplayer application. Leave link.d alone.
    link=[arg for arg in link if not arg.startswith('-Wl,--dependency-file=')]
    link[link.index('-o')+1]=str(output)
    link.extend([str(obj),'-Wl,--wrap='+symbol,'-Wl,--wrap='+fp_symbol])
    record=dict(passed=False,production_runtime=str(build/'halo2_native'),
                production_sha256=sha(build/'halo2_native'),source_sha256=sha(source),
                helper_sha256=sha(Path(__file__)),commands=[compile_command,link],
                probe=str(output),controlled_tick_cadence=True,render_fraction=0.5,session_random_seed=1)
    args.report.parent.mkdir(parents=True,exist_ok=True)
    try:
        for command in (compile_command,link):
            subprocess.run(command,cwd=build,check=True)
        record.update(passed=True,probe_sha256=sha(output))
    finally:
        args.report.write_text(json.dumps(record,indent=2)+'\n')
    print('Built owned fixed-tick probe:',output,flush=True)


if __name__=='__main__':
    main()

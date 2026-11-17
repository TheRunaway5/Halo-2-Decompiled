#!/usr/bin/env python3
"""Replay exact installed 107140 in one copied Primary partition and the actual R31 link."""
import argparse
import json
import os
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import sys
import time

from build_launcher_engine import freeze_source, source_hashes
from native_metadata import check_budget, load
from native_object_record_ratio_replacements import BODY, INCLUDE, replace_object_record_ratio
from specialize_native_operands import specialize
from validate_native_object_record_ratio_public_tu15 import sha
from validate_native_replacement_snapshot import auxiliary_source_hashes, freeze_test_code, test_code_hashes

ROOT=Path(__file__).resolve().parents[1]
UNIT='src/physics/physics_live_support_tu15_generated.cpp'
DRIVER='tests/native_tu15_object_record_ratio_public_tests.cpp'
COMPARE='tools/validate_native_object_record_ratio_public_tu15.py'
FIXTURE='tools/validate_native_object_record_ratio_frame_tu15.py'
HEADERS=('include/halo2/native_tu15_object_record_ratio_kernel.hpp',
    'src/gameplay/objects/native_tu15_object_record_ratio_ordered.hpp',
    'src/gameplay/objects/native_tu15_object_record_ratio_source_frame.hpp',
    'src/gameplay/objects/native_tu15_object_record_ratio_source_owner.hpp')


def inventory(root):
    return {**source_hashes(root),**test_code_hashes(root),**auxiliary_source_hashes(root)}


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('linked-donor','linked-inventory','xbe','output'):
        parser.add_argument('--'+name,type=Path,required=True)
    args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Use tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    donor=args.linked_donor.resolve();build=donor/'build';source=scratch/'source';libs=scratch/'libraries'
    output=args.output.resolve();output.mkdir(parents=True,exist_ok=False);libs.mkdir()
    report=dict(passed=False,complete_build=False,execution_accounting_proved=False,
                owned_scratch=str(scratch),retained_large_artifacts=[],stage='copying',commands={})

    def save(): (output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')

    def run(label,command,timeout=1200):
        report['stage']=label;report['commands'][label]=list(map(str,command));save()
        start=time.monotonic()
        with (output/(label+'.log')).open('w') as log:
            p=subprocess.Popen(command,cwd=libs,stdout=log,stderr=subprocess.STDOUT)
            try:
                while True:
                    try: result=p.wait(timeout=30);break
                    except subprocess.TimeoutExpired:
                        elapsed=int(time.monotonic()-start);print(label,elapsed,flush=True)
                        if elapsed>timeout:raise TimeoutError(label)
            finally:
                if p.poll() is None:p.terminate();p.wait()
        if result:
            raise ValueError(label+' failed: '+(output/(label+'.log')).read_text()[-6500:])

    try:
        old=load(args.linked_inventory)
        freeze_source(donor/'source',source);freeze_test_code(donor/'source',source)
        if inventory(source)!=old:
            raise ValueError('Independent R31 source copy differs from its inventory')
        current=(ROOT/UNIT).read_text();actual,receipt=specialize(current)
        if receipt.get('native_object_record_ratio_entries')!=['107140']:
            raise ValueError('Live 107140 prepared binding missing')
        old_prepared=build/'native-operands/physics/physics_live_support_tu15_generated.cpp'
        proposed=replace_object_record_ratio(old_prepared.read_text())
        target=BODY.search(proposed)[0]
        if target!=BODY.search(actual)[0]:
            raise ValueError('The R31 proposed leaf differs from the exact installed prepared leaf')
        partition=build/'native-translations/physics_live_support_tu15_generated'
        leaf=re.compile(r'^void run_107140\(WorldState &s\) \{.*?^\}',re.M|re.S)
        bodies=[(path,path.read_text()) for path in partition.glob('body_*.cpp')]
        chosen=[(path,body) for path,body in bodies if leaf.search(body)]
        if len(chosen)!=1:
            raise ValueError('Missing or ambiguous original Primary 107140 shard')
        shard,old_text=chosen[0];match=leaf.search(old_text)
        if not old_text.startswith('#include "common.hpp"\n') or old_text.count('void run_107140(')!=1:
            raise ValueError('Unexpected actual R31 Primary shard shape')
        overlay=old_text[:match.start()]+target.removeprefix('static ')+old_text[match.end():]
        new_text=overlay.replace('#include "common.hpp"\n','#include "common.hpp"\n'+INCLUDE,1)
        local=scratch/'primary';local.mkdir()
        (local/shard.name).write_text(new_text)
        shutil.copyfile(partition/'common.hpp',local/'common.hpp')
        for name in (*HEADERS,DRIVER,COMPARE,FIXTURE):
            path=source/name;path.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/name,path)
        pins=inventory(source)
        if {key for key in old.keys()|pins.keys() if old.get(key)!=pins.get(key)}!=set((*HEADERS,DRIVER,COMPARE,FIXTURE)):
            raise ValueError('Unexpected source overlay dependency')
        report.update(old_shard_sha256=sha(shard),original_common_sha256=sha(local/'common.hpp'),
                      installed_primary_sha256=sha(ROOT/UNIT),replacement_shard_sha256=sha(local/shard.name),
                      exact_other_shard_bytes=True,proof_scope='one exact prepared routine; old scope counters removed only for that routine')
        recipe=build/'CMakeFiles/native_projectile_function_query_public_tests.dir/link.txt'
        link=shlex.split(recipe.read_text());report['link_recipe_sha256']=sha(recipe)
        inputs=[];archives={};system={}
        for arg in link[link.index('-o')+2:]:
            if arg=='-ldl':inputs.append(arg);continue
            prefix='-Wl,@' if arg.startswith('-Wl,@') else ''
            path=build/arg.removeprefix(prefix)
            if path.is_relative_to(build):
                key=str(path.relative_to(build))
                if key not in archives:
                    digest=sha(path);dest=libs/key;dest.parent.mkdir(parents=True,exist_ok=True)
                    shutil.copy2(path,dest)
                    if sha(dest)!=digest or sha(path)!=digest:
                        raise ValueError('R31 link input drift: '+key)
                    archives[key]=digest
                inputs.append(prefix+str(libs/key))
            elif path.is_file() and not prefix:
                system[str(path)]=sha(path);inputs.append(arg)
            else:
                raise ValueError('Unexpected link input: '+arg)
        report.update(copied_r31_inputs=archives,system_inputs=system)
        flag_paths={label:build/f'CMakeFiles/{name}.dir/flags.make' for label,name in
                    (('owner','halo2_physics_world_runtime'),('driver','native_projectile_function_query_public_tests'))}
        flag_inputs={label:path.read_text() for label,path in flag_paths.items()}
        report['flag_inputs_sha256']={label:sha(path) for label,path in flag_paths.items()}
        print('107140_LINK_INPUTS_READY',scratch,flush=True)
        for label,cpp in (('owner',local/shard.name),('driver',source/DRIVER)):
            actual_flags=[arg.replace(str(donor/'source'),str(source)) for line in flag_inputs[label].splitlines()
                if line.startswith(('CXX_DEFINES =','CXX_INCLUDES =','CXX_FLAGS ='))
                for arg in shlex.split(line.partition('=')[2])]
            if label=='driver':actual_flags.append('-I'+str(source/'tests'))
            dep=scratch/(label+'.d')
            run(label,[link[0],*actual_flags,'-c',cpp,'-o',scratch/(label+'.o'),'-MD','-MF',dep])
            dependencies={}
            for arg in shlex.split(dep.read_text().replace(chr(92)+'\n',' ').split(':',1)[1]):
                path=Path(arg).resolve()
                if path.is_relative_to(source):
                    key=str(path.relative_to(source));dependencies[key]=sha(path)
                    if dependencies[key]!=pins.get(key):raise ValueError('Compiled dependency drift: '+key)
                elif path.is_relative_to(donor):
                    raise ValueError('Compiler borrowed removed R31 owner dependency: '+str(path))
            report[label+'_dependencies']=dependencies
        binary,link_map=scratch/'primary-public',scratch/'link.map'
        run('link',[*link[:3],'-Wl,-Map='+str(link_map),scratch/'driver.o',scratch/'owner.o','-o',binary,*inputs])
        if f'libhalo2_physics_world_runtime.a({shard.name}.o)' in link_map.read_text():
            raise ValueError('The linker extracted the old Primary shard')
        report.update(older_primary_shard_unused=True,linked_binary_sha256=sha(binary))
        run('actual-public',[sys.executable,'-B',source/COMPARE,'--xbe',args.xbe.resolve(),
                            '--native',binary,'--output',output/'public'])
        report['copied_source_unchanged']=inventory(source)==pins
        report['system_inputs_unchanged']=all(sha(Path(p))==h for p,h in system.items())
        report.update(passed=report['copied_source_unchanged'] and report['system_inputs_unchanged'],stage='complete')
    except BaseException as error:
        report['error']=repr(error);raise
    finally:
        save();check_budget(output)
    return 0 if report['passed'] else 1


if __name__=='__main__':
    raise SystemExit(main())

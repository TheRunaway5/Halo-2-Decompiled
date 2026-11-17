#!/usr/bin/env python3
"""Replay the proposed exact 1046C0 shard against independent R32 World linkage."""
import argparse
import hashlib
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
import native_object_ratio_caller_replacements as binding
from validate_native_replacement_snapshot import auxiliary_source_hashes, freeze_test_code, test_code_hashes

ROOT=Path(__file__).resolve().parents[1]
UNIT='src/physics/physics_live_support_tu15_generated.cpp'
DRIVER='tests/native_tu15_object_ratio_caller_world_tests.cpp'
COMPARE='tools/validate_native_object_ratio_caller_world_tu15.py'
TOOL='tools/native_object_ratio_caller_replacements.py'
TEST_TARGET='CMakeFiles/native_object_function_query_public_tests.dir'
OWNER_TARGET='CMakeFiles/halo2_physics_world_runtime.dir'
ARCHIVE='libhalo2_physics_world_runtime.a'
FIXTURES=('tools/validate_native_object_ratio_caller_middle_frame_tu15.py',
          'tools/native_object_ratio_combat_fixtures.py','tools/native_object_ratio_high_fixtures.py',
          'tools/native_object_ratio_low_fixtures.py')
HEADER_PATTERN='native_tu15_object_ratio_caller*'
PARTITION=Path('native-translations/physics_live_support_tu15_generated')


def sha(path):
    with path.open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()


def inventory(path):
    return {**source_hashes(path),**test_code_hashes(path),**auxiliary_source_hashes(path)}


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('linked-donor','linked-inventory','map','xbe','output'):
        parser.add_argument('--'+name,type=Path,required=True)
    parser.add_argument('--inherited-only',action='store_true')
    args=parser.parse_args()
    driver='tests/native_tu15_object_function_query_public_tests.cpp' if args.inherited_only else DRIVER
    compare='tools/validate_native_object_ratio_caller_inherited_tu15.py' if args.inherited_only else COMPARE
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Use tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    donor=args.linked_donor.resolve(strict=True)
    build=donor/'build'
    output=args.output.resolve()
    output.mkdir(parents=True,exist_ok=False)
    copied,source=scratch/'build',scratch/'source'
    copied.mkdir()
    proof=dict(passed=False,stage='copying',owned_scratch=str(scratch),retained_large_artifacts=[],
               coherent_runtime=False,commands={},linked_donor=str(donor))

    def save():
        (output/'report.json').write_text(json.dumps(proof,indent=2,sort_keys=True)+'\n')

    def copy(src,dst):
        digest=sha(src);dst.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(src,dst)
        if dst.is_symlink() or sha(dst)!=digest or sha(src)!=digest:
            raise ValueError('Independent input copy changed: '+str(src))
        return digest

    def run(label,command,timeout=900,env=None):
        proof['commands'][label]=list(map(str,command));proof['stage']=label;save()
        print('1046C0_WORLD_LINK',label,flush=True)
        with (scratch/(label+'.log')).open('w') as out:
            try:
                subprocess.run(command,cwd=copied,stdout=out,stderr=subprocess.STDOUT,
                               check=True,timeout=timeout,env=env)
            finally:
                out.flush();(output/(label+'.log')).write_bytes(Path(out.name).read_bytes()[-12000:])

    def relocate(value):
        return value.replace(str(donor/'source'),str(source)).replace(str(build),str(copied))

    try:
        pins=load(args.linked_inventory)
        freeze_source(donor/'source',source);freeze_test_code(donor/'source',source)
        if inventory(source)!=pins:raise ValueError('R32 source does not match its immutable inventory')
        proof['donor_inventory_storage_sha256']=sha(args.linked_inventory)
        overlay=(driver,compare,TOOL,*FIXTURES,*(
            str(path.relative_to(ROOT)) for directory in ('include/halo2','src/gameplay/objects')
            for path in sorted((ROOT/directory).glob(HEADER_PATTERN+'.hpp'))))
        live_pins={name:copy(ROOT/name,source/name) for name in overlay}
        if inventory(source)!=(pins|live_pins):raise ValueError('Unexpected frozen source overlay')
        proof['overlay_pins']=live_pins
        canonical=source/UNIT
        original_raw=canonical.read_text()
        if binding.digest(binding.BODY.search(original_raw)[0])!=binding.PINS[False]:
            raise ValueError('R32 actual canonical 1046C0 body changed')
        prepared=build/'native-operands/physics/physics_live_support_tu15_generated.cpp'
        prepared_text=prepared.read_text()
        base=binding.BODY.search(prepared_text)[0]
        if binding.digest(base)!=binding.PINS[True]:raise ValueError('R32 prepared 1046C0 body changed')
        candidate=binding.replace_object_ratio_caller(prepared_text)
        if binding.remove_object_ratio_caller(candidate)!=prepared_text:
            raise ValueError('Exact new prepared parent did not reverse the complete unit')
        proof['exact_candidate_prepared_body_sha256']=binding.digest(binding.BODY.search(candidate)[0])
        shape=re.compile(r'^void run_1046c0\(WorldState &s\) \{.*?^\}',re.S|re.M)
        candidates=[]
        for path in (build/PARTITION).glob('body_*.cpp'):
            text=path.read_text()
            if match:=shape.search(text):candidates.append((path,text,match))
        if len(candidates)!=1:raise ValueError('Expected one authentic 1046C0 partition')
        path,text,match=candidates[0]
        prologue=re.match(r'^void run_1046c0\(WorldState &s\) \{\nstatic ::halo2::execution_accounting::Counter [^\n]+\n'
                          r'::halo2::execution_accounting::Scope [^\n]+\n\n',match[0])
        if not prologue or not text.startswith('#include "common.hpp"\n'):
            raise ValueError('Missing authentic accounting scope or partition header')
        tail=match[0][prologue.end():]
        event=r'\{::halo2::execution_accounting::record_literal\([^)]*\);(s\.instruction_literal\(0x[0-9a-f]+u\);)\}'
        canonical_tail,events=re.subn(event,r'\1',tail)
        if events!=406 or binding.HEADER+'\n'+canonical_tail!=base:
            raise ValueError('Donor accounting did not exactly project to prepared 1046C0')
        changed_tail=binding.arithmetic(tail)
        projected=binding.HEADER+binding.prefix(True)+'\n'+re.sub(event,r'\1',changed_tail)
        if projected!=binding.BODY.search(candidate)[0]:
            raise ValueError('Candidate prepared projection differs from the rebuilt accounting body')
        replacement=prologue[0]+binding.prefix(True).lstrip('\n')+'\n'+changed_tail
        rebuilt=text[:match.start()]+replacement+text[match.end():]
        if rebuilt.replace(replacement,match[0],1)!=text:
            raise ValueError('Complete original shard did not reverse')
        directory=scratch/'primary';directory.mkdir()
        shard=directory/path.name
        shard.write_text(rebuilt.replace('#include "common.hpp"\n','#include "common.hpp"\n'+binding.INCLUDE,1))
        common=copy(build/PARTITION/'common.hpp',directory/'common.hpp')
        proof.update(archived_shard_sha256=sha(path),candidate_shard_sha256=sha(shard),common_sha256=common,
                     untouched_shard_neighbors=True,original_accounting_scope_retained=True,original_retained_sites=events)
        recipe_path=build/TEST_TARGET/'link.txt'
        recipe=shlex.split(recipe_path.read_text())
        proof['actual_world_test_link_recipe_sha256']=sha(recipe_path)
        args_link=[];archive_pins={};system_pins={}
        for arg in recipe[recipe.index('-o')+2:]:
            if arg=='-ldl':args_link.append(arg);continue
            prefix='-Wl,@' if arg.startswith('-Wl,@') else ''
            name=build/arg.removeprefix(prefix)
            if name.is_relative_to(build):
                key=str(name.relative_to(build));local=copied/key
                if key not in archive_pins:
                    start=time.monotonic()
                    while not name.is_file():
                        if not donor.is_dir():raise ValueError('Donor ended before independent inputs were copied')
                        if time.monotonic()-start>900:raise TimeoutError('Waiting on donor '+key)
                        time.sleep(5)
                    if name.suffix=='.a':subprocess.run(['ar','t',name],stdout=subprocess.DEVNULL,check=True,timeout=30)
                    archive_pins[key]=copy(name,local)
                if prefix and '/.local/test-scratch/' in local.read_text():
                    raise ValueError('Unexpected borrowed scratch dependency in response file')
                args_link.append(prefix+str(local))
            elif name.is_file() and not prefix:
                system_pins[str(name)]=sha(name);args_link.append(arg)
            else:raise ValueError('Unexpected World link input: '+arg)
        proof.update(archive_pins=archive_pins,system_pins=system_pins)
        flags={}
        for role,target in (('owner',OWNER_TARGET),('driver',TEST_TARGET)):
            location=Path(target)/'flags.make'
            proof[role+'_flag_pin']=copy(build/location,copied/location)
            flags[role]=[relocate(arg) for line in (copied/location).read_text().splitlines()
                         if line.startswith(('CXX_DEFINES =','CXX_INCLUDES =','CXX_FLAGS ='))
                         for arg in shlex.split(line.partition('=')[2])]
        xbe=scratch/'default.xbe';map_path=scratch/'coagulation.map'
        proof['xbe_pin']=copy(args.xbe.resolve(),xbe)
        proof['map_pin']=copy(args.map.resolve(),map_path)
        print('1046C0_WORLD_INPUTS_READY',scratch,flush=True)
        for role,cpp in (('owner',shard),('driver',source/driver)):
            dep=scratch/(role+'.d')
            run('compile-'+role,[recipe[0],*flags[role],'-c',cpp,'-o',scratch/(role+'.o'),'-MD','-MF',dep])
            dependencies={}
            for token in shlex.split(dep.read_text().replace(chr(92)+'\n',' ').split(':',1)[1]):
                resolved=(copied/token).resolve(strict=True)
                if resolved.is_relative_to(source):
                    relative=str(resolved.relative_to(source))
                    expected=(pins|live_pins).get(relative)
                    if sha(resolved)!=expected:raise ValueError('Source dependency drift: '+relative)
                    dependencies[relative]=expected
                elif '/.local/test-scratch/' in str(resolved) and not resolved.is_relative_to(scratch):
                    raise ValueError('Unexpected borrowed compiler dependency: '+str(resolved))
            proof[role+'_dependencies']=dependencies
        binary,link_map,link_dep=scratch/'1046c0-world',scratch/'link.map',scratch/'link.d'
        link_flags=[arg for arg in recipe[1:recipe.index('-o')] if not arg.endswith('.o') and
                    not arg.startswith('-Wl,--dependency-file=')]
        symbols=subprocess.check_output(['nm','-g','--defined-only',scratch/'owner.o'],text=True).splitlines()
        symbols=[line.split()[-1] for line in symbols if re.search(r'\b[TW]\s+\S*run_1046c0\S*$',line)]
        if len(symbols)!=1:raise ValueError('Expected one exact candidate 1046C0 symbol')
        symbol=symbols[0]
        run('link',[recipe[0],*link_flags,'-Wl,-Map='+str(link_map),'-Wl,--dependency-file='+str(link_dep),
            '-Wl,--trace-symbol='+symbol,scratch/'driver.o',scratch/'owner.o','-o',binary,*args_link])
        definitions=[line for line in (output/'link.log').read_text().splitlines() if 'definition of '+symbol in line]
        if len(definitions)!=1 or str(scratch/'owner.o') not in definitions[0]:
            raise ValueError('Linker did not define 1046C0 from the exact candidate object: '+str(definitions))
        proof['link_symbol_definition']=definitions[0]
        lines=[line for line in link_dep.read_text().replace(chr(92)+'\n',' ').splitlines()
               if line.strip() and not line.lstrip().startswith('#')]
        dependencies={(copied/token).resolve(strict=True) for token in shlex.split(lines[0].split(':',1)[1])}
        if any('/.local/test-scratch/' in str(p) and not p.is_relative_to(scratch) for p in dependencies):
            raise ValueError('Unexpected borrowed link dependency')
        if scratch/'owner.o' not in dependencies or copied/'libhalo2_systems.a' not in dependencies:
            raise ValueError('Missing actual replacement shard or World archive from link')
        proof.update(older_partition_unused=True,binary_sha256=sha(binary),link_map_sha256=sha(link_map))
        run('actual-world',[sys.executable,'-B',source/compare,'--binary',binary,'--map',map_path,'--xbe',xbe,
                           '--report',output/'world.json',*(() if args.inherited_only else ('--accounting',))],timeout=1500)
        if not json.loads((output/'world.json').read_bytes())['passed']:
            raise ValueError('World comparison failed')
        if inventory(source)!=(pins|live_pins) or any(sha(Path(name))!=value for name,value in system_pins.items()):
            raise ValueError('Independent source or system inputs changed')
        proof.update(passed=True,stage='complete',actual_live_world=True)
    except BaseException as error:
        proof['error']=repr(error);raise
    finally:
        save();check_budget(output)
    return 0


if __name__=='__main__':
    raise SystemExit(main())

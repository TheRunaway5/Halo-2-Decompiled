#!/usr/bin/env python3
"""Verify all three linked 3EB10 owners using independent R32 archive copies."""
import argparse
from concurrent.futures import ThreadPoolExecutor
import json
import os
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import sys

from build_launcher_engine import freeze_source
import native_indexed_datum_gate_replacements as binding
from native_metadata import check_budget, load
from replay_native_object_ratio_caller_link_tu15 import inventory, sha
from validate_native_indexed_datum_gate_frame_tu15 import MAPPACK_SHA, PROFILE, corpus
from validate_native_replacement_snapshot import freeze_test_code
from oracle_smg_firing_1_1 import Xbe

ROOT = Path(__file__).resolve().parents[1]
DRIVER = 'tests/native_tu15_indexed_datum_gate_linked_tests.cpp'
WORLD_DRIVER = 'tests/native_tu15_particle_draw_indexed_gate_world_tests.cpp'
FILTER_DRIVER = 'tests/native_tu15_particle_type_filter_linked_tests.cpp'
TEST = 'CMakeFiles/native_object_function_query_public_tests.dir'
WORLD = 'CMakeFiles/halo2_physics_world_runtime.dir'
SYSTEMS = 'CMakeFiles/halo2_systems.dir'
UNITS = {
    'primary': ('src/physics/physics_live_support_tu15_generated.cpp', WORLD, 'run_3eb10', 'tu15'),
    'particle-frame': ('src/rendering/effects/source_effect_particle_frame_tu15_generated.cpp',
                       WORLD, 'source_run_effect_particle_frame_tu15', 'tu15'),
    'death-startup': ('src/gameplay/units/death_startup_runtime_generated.cpp',
                      SYSTEMS, 'source_run_death_startup_bodies_1_1', 'map-pack11'),
}
PARTITION = Path('native-translations/physics_live_support_tu15_generated')
BODY = re.compile(r'^((?:\[\[gnu::noinline\]\] )?(?:static )?void run_3eb10\([^)]*\) \{).*?^\}', re.M|re.S)
PROLOGUE = re.compile(r'^\nstatic ::halo2::execution_accounting::Counter [^\n]+\n'
                      r'::halo2::execution_accounting::Scope [^\n]+\n\n')
EVENT = re.compile(r'\{::halo2::execution_accounting::record_literal\(halo2_execution_counter_8ef3\);'
                   r'(s\.instruction_literal\(0x[0-9a-f]+u\);|'
                   r'if\(s\.services\.instruction\)\{s\.services\.instruction\(0x[0-9a-f]+u\);\})\}')


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('donor','inventory','xbe','mappack','output'):
        parser.add_argument('--'+name,type=Path,required=True)
    parser.add_argument('--map',type=Path,help='Compare the actual inherited ParticleDraw caller on this OG map')
    parser.add_argument('--particle-type-filter',action='store_true',help='Replay the separate ParticleDraw 17DE60 owner')
    args=parser.parse_args()
    if args.map and args.particle_type_filter:
        parser.error('The inherited gate and direct particle filter are separate fixtures')
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Use tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    donor=args.donor.resolve(strict=True);build=donor/'build'
    output=args.output.resolve();output.mkdir(parents=True,exist_ok=False)
    copied,source=scratch/'build',scratch/'source'
    copied.mkdir()
    filter_owner=args.particle_type_filter
    symbol_id='17de60' if filter_owner else '3eb10'
    site_count=27 if filter_owner else 25
    reference_env='HALO2_REFERENCE_PARTICLE_TYPE_FILTER' if filter_owner else 'HALO2_REFERENCE_INDEXED_DATUM_GATE'
    units=UNITS
    if filter_owner:
        import native_particle_type_filter_replacements as filter_binding
        from validate_native_particle_type_filter_frame_tu15 import KERNEL as filter_kernel, corpus as filter_inputs
        units={'particle-draw':('src/rendering/effects/source_effect_particle_draw_tu15_generated.cpp',
                                WORLD,'source_run_effect_particle_draw_tu15','tu15')}
    proof=dict(passed=False,owned_scratch=str(scratch),coherent_runtime=False,commands={},
               linked_donor=str(donor),retained_large_artifacts=[])

    def save():
        (output/'report.json').write_text(json.dumps(proof,indent=2,sort_keys=True)+'\n')

    def copy(src,dst):
        digest=sha(src);dst.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(src,dst)
        if dst.is_symlink() or sha(dst)!=digest or sha(src)!=digest:
            raise ValueError('Input copy changed: '+str(src))
        return digest

    def run(label, command, *, env=None, capture=False):
        proof['commands'][label]=list(map(str,command))
        print('3EB10_LINK',label,flush=True)
        log=scratch/(label+'.log')
        succeeded=False
        with log.open('w') as out:
            try:
                subprocess.run(command,cwd=copied,stdout=out,stderr=subprocess.STDOUT,
                               check=True,timeout=1200,env=env)
                succeeded=True
            finally:
                out.flush()
                if not capture or not succeeded:
                    (output/(label+'.log')).write_bytes(log.read_bytes()[-12000:])
        return log

    def relocate(value):
        return value.replace(str(donor/'source'),str(source)).replace(str(build),str(copied))

    def owner_flags(target, artifact):
        rules=(build/target/'build.make').read_text()
        relative=artifact.relative_to(build).as_posix()
        marker=f' -c {artifact}'
        rows=[line.strip() for line in rules.splitlines() if marker in line and ' -MD ' in line]
        if len(rows)!=1:
            raise ValueError('Missing actual compile recipe for '+relative)
        flags=(build/target/'flags.make').read_text()
        definitions={line.partition('=')[0].strip():line.partition('=')[2].strip()
                     for line in flags.splitlines() if line.startswith(('CXX_DEFINES =','CXX_INCLUDES =','CXX_FLAGS ='))}
        command=rows[0].split(' -MD ',1)[0]
        for key,value in definitions.items():command=command.replace(f'$({key})',value)
        if '$(' in command:
            raise ValueError('Unexpanded generated CMake compile recipe')
        return shlex.split(relocate(command))

    try:
        pins=load(args.inventory)
        freeze_source(donor/'source',source);freeze_test_code(donor/'source',source)
        if inventory(source)!=pins:
            raise ValueError('R32 original source inventory changed')
        proof['donor_manifest_storage_sha256']=sha(args.inventory)
        test_source=FILTER_DRIVER if filter_owner else WORLD_DRIVER if args.map else DRIVER
        folder,pattern,kernel=('src/rendering/effects','native_tu15_particle_type_filter_source*.hpp',filter_kernel
            ) if filter_owner else ('src/gameplay/objects','native_tu15_indexed_datum_gate_source*.hpp',PROFILE.kernel)
        extras=(test_source,kernel,*(str(path.relative_to(ROOT)) for path in (ROOT/folder).glob(pattern)))
        extra_pins={name:copy(ROOT/name,source/name) for name in extras}
        if inventory(source)!=(pins|extra_pins):
            raise ValueError('Unexpected independent source overlay')
        proof['overlay_pins']=extra_pins
        recipe_path=build/TEST/'link.txt'
        recipe=shlex.split(recipe_path.read_text())
        proof['actual_world_link_recipe_sha256']=sha(recipe_path)
        archives,systems,link_args={},{},[]
        for arg in recipe[recipe.index('-o')+2:]:
            if arg=='-ldl':
                link_args.append(arg);continue
            prefix='-Wl,@' if arg.startswith('-Wl,@') else ''
            origin=build/arg.removeprefix(prefix)
            if origin.is_relative_to(build):
                name=origin.relative_to(build);local=copied/name
                if str(name) not in archives:archives[str(name)]=copy(origin,local)
                if prefix and '/.local/test-scratch/' in local.read_text():
                    raise ValueError('World link response file borrows owned scratch')
                link_args.append(prefix+str(local))
            elif origin.is_file() and not prefix:
                systems[str(origin)]=sha(origin);link_args.append(arg)
            else:
                raise ValueError('Unexpected World link input: '+arg)
        proof.update(archives=archives,systems=systems)
        print('3EB10_LINK independent-archives',len(archives),flush=True)
        candidates={}
        for owner,(unit,target,symbol,image) in units.items():
            death=owner=='death-startup';primary=owner=='primary'
            canonical=source/unit
            if not death:
                canonical=build/'native-operands'/unit.removeprefix('src/')
            whole=canonical.read_text()
            authenticated=filter_binding.authenticate(whole) if filter_owner else binding.authenticate(whole)
            if not authenticated or authenticated[3] is not None:
                raise ValueError('Expected archived retained owner '+owner)
            _,old,literal,_=authenticated[:4]
            if filter_owner:
                anchor,include=filter_binding.ANCHOR,filter_binding.INCLUDE
                header=filter_binding.HEADER
                prefix=filter_binding.prefix(literal)
                shape=re.compile(r'^('+re.escape(header)+r').*?^\}',re.M|re.S)
            else:
                _,_,_,_,anchor,image_name=authenticated
                include=binding.INCLUDE;header=binding.DEATH_HEADER if death else binding.WORLD_HEADER
                prefix=binding.prefix(literal,image_name);shape=BODY
            if primary:
                location=build/PARTITION
                copies=[path for path in location.glob('body_*.cpp') if BODY.search(path.read_text())]
                for path in location.glob('*.hpp'):copy(path,copied/PARTITION/path.name)
            else:
                copies=list((build/'native-execution').glob('*/'+unit))
            if len(copies)!=1:
                raise ValueError('Missing unique actual instrumented source for '+owner)
            actual=copies[0];text=actual.read_text()
            rows=list(shape.finditer(text))
            if len(rows)!=1:
                raise ValueError('Instrumented 3EB10 source shape changed')
            body=rows[0];tail=body[0][len(body[1]):]
            scope=PROLOGUE.match(tail)
            if not scope or f'"{symbol_id}",{site_count}u,0u' not in scope[0]:
                raise ValueError('Missing original retained accounting scope')
            projection,count=EVENT.subn(r'\1',tail[scope.end():])
            if count!=site_count or header+'\n'+projection!=old:
                raise ValueError('R32 instrumentation does not project to the exact canonical owner '+owner)
            transport=tail[scope.end():]
            if filter_owner:
                transport=filter_binding.arithmetic(transport)
                projected=header+prefix+'\n'+EVENT.sub(r'\1',transport)
                expected=filter_binding.BODY.search(filter_binding.replace_particle_type_filter(whole))[0]
                if projected!=expected:
                    raise ValueError('Instrumented 17DE60 differs from the exact staged source')
            changed=body[1]+scope[0]+prefix.lstrip('\n')+'\n'+transport
            include_anchor='#include "common.hpp"\n' if primary else anchor
            inserted=(text[:body.start()]+changed+text[body.end():])
            if inserted.count(include_anchor)!=1 or inserted.replace(changed,body[0],1)!=text:
                raise ValueError('Candidate changed another retained body')
            inserted=inserted.replace(include_anchor,include_anchor+include,1)
            cpp=copied/actual.relative_to(build);cpp.parent.mkdir(parents=True,exist_ok=True)
            cpp.write_text(inserted)
            if inserted.replace(include,'',1).replace(changed,body[0],1)!=text:
                raise ValueError('Candidate did not reverse to the whole archived source')
            obj=scratch/(owner+'.o')
            candidates[owner]=(owner_flags(target,actual),cpp,obj,symbol,image)
            proof.setdefault('owners',{})[owner]=dict(canonical_sha256=sha(canonical),
                archived_sha256=sha(actual),candidate_sha256=sha(cpp),literal_transport=literal)
        flags=(build/TEST/'flags.make').read_text()
        driver_flags=[relocate(arg) for line in flags.splitlines()
                      if line.startswith(('CXX_DEFINES =','CXX_INCLUDES =','CXX_FLAGS ='))
                      for arg in shlex.split(line.partition('=')[2])]
        driver=scratch/'driver.o'
        jobs=[('compile-'+owner,[*flags,'-c',cpp,'-o',obj]) for owner,(flags,cpp,obj,_,_) in candidates.items()]
        jobs.append(('compile-driver',[recipe[0],*driver_flags,'-c',source/test_source,'-o',driver]))
        with ThreadPoolExecutor(max_workers=4) as pool:
            futures=[pool.submit(run,*job) for job in jobs]
            for future in futures:future.result()
        symbols={}
        for owner,(_,_,obj,fragment,_) in candidates.items():
            lines=subprocess.check_output(['nm','-g','--defined-only',obj],text=True).splitlines()
            matches=[line.split()[-1] for line in lines if fragment in line and re.search(r'\b[TW]\s+',line)]
            if len(matches)!=1:
                raise ValueError('Missing exact linked symbol: '+owner)
            symbols[owner]=matches[0]
        link_flags=[x for x in recipe[1:recipe.index('-o')] if not x.endswith('.o') and
                    not x.startswith('-Wl,--dependency-file=')]
        binaries={name:scratch/name for name in ('archived','candidate')}
        run('link-archived',[recipe[0],*link_flags,driver,'-o',binaries['archived'],*link_args])
        log=run('link-candidate',[recipe[0],*link_flags,*('-Wl,--trace-symbol='+s for s in symbols.values()),
            driver,*(value[2] for value in candidates.values()),'-o',binaries['candidate'],*link_args])
        for owner,symbol in symbols.items():
            hits=[line for line in log.read_text().splitlines() if 'definition of '+symbol in line]
            if len(hits)!=1 or str(candidates[owner][2]) not in hits[0]:
                raise ValueError('Wrong linked 3EB10 owner for '+owner)
            proof['owners'][owner]['linked_definition']=hits[0]
        xbe=scratch/'tu15.xbe';mappack=scratch/'map-pack.xbe'
        if copy(args.xbe.resolve(),xbe)!=PROFILE.image_sha256 or copy(args.mappack.resolve(),mappack)!=MAPPACK_SHA:
            raise ValueError('Wrong original fixture images')
        fixture=scratch/'inputs.json'
        expected=None
        if args.map:
            import validate_native_particle_draw_indexed_gate_tu15 as parent
            from validate_native_projectile_function_query_frame_tu15 import boundary_packet
            world_map=scratch/'coagulation.map'
            proof['map_sha256']=copy(args.map.resolve(),world_map)
            if proof['map_sha256']!='1de37727dbb9c51aa08e03bfea86c0641ad67250dba28c5dcb3ba3dadaaf93fa':
                raise ValueError('Actual linked World fixture requires original Coagulation')
            image=Xbe(xbe);rows=parent.corpus(image);original_dir=scratch/'original';original_dir.mkdir()
            original,proof['original_parent_authority']=parent.observe(image,rows,original_dir)
            expected=[]
            for row,case in zip(rows,original['cases'],strict=True):
                traced=[event['entry'] for event in case['calls']]
                if not case['completed'] or traced[-1]!=0x1772af or traced.count(0x3eb10)!=1 or traced.count(0x177157)!=1:
                    raise ValueError('Wrong original inherited gate route')
                expected.append(dict(name=row['name'],expected=boundary_packet(case,row,traced[-1]),stores=case['writes'],
                    banks=[bank for bank in case['banks'] if bank['address'] in (0x61000000,0x62000000)]))
            fixture.write_text(json.dumps(rows,separators=(',',':')))
        elif filter_owner:
            fixture.write_text(json.dumps(filter_inputs(Xbe(xbe)),separators=(',',':')))
        else:
            fixture.write_text(json.dumps(dict(tu15=corpus(Xbe(xbe)),mappack11=corpus(Xbe(mappack),True)),
                                          separators=(',',':')))
        proof['fixture_sha256']=sha(fixture)
        packet_pins={}
        for mode,version,reference in (('archived','archived',False),
                                       ('native','candidate',False),('reference','candidate',True)):
            env=os.environ.copy();counter=scratch/(mode+'-accounting.json')
            env['HALO2_NATIVE_EXECUTION_REPORT']=str(counter)
            env.pop(reference_env,None)
            if reference:env[reference_env]='1'
            save_dir=scratch/('save-'+mode)
            world_args=[world_map,xbe,save_dir] if args.map else []
            packets=run('actual-'+mode,[binaries[version],*world_args,fixture],env=env,capture=True)
            packet_pins[mode]=sha(packets);rows=json.loads(packets.read_text())
            accounting=json.loads(counter.read_text())
            if args.map:
                if rows!=expected:
                    for n,(actual,wanted) in enumerate(zip(rows,expected,strict=True)):
                        if actual!=wanted:
                            diff=[key for key in wanted if actual[key]!=wanted[key]]
                            raise ValueError('Actual World original inherited output differs in '+mode+
                                ' row '+str(n)+' fields '+str(diff)+
                                ' ABI actual='+str(actual['expected'])+' original='+str(wanted['expected']))
                    raise ValueError('Actual World original inherited output shape differs')
                sums={}
                for values in accounting['rows']:
                    row=dict(zip(accounting['row_fields'],values,strict=True))
                    if row['symbol'] in ('1770c0','3eb10'):
                        unit=accounting['units'][row['unit']][0]
                        sums[row['symbol'],unit]=row
                key=('3eb10',UNITS['primary'][0])
                parent_key=('1770c0','src/rendering/effects/source_effect_particle_draw_tu15_generated.cpp')
                if set(sums)!={key,parent_key}:
                    raise ValueError('Wrong inherited gate source selection: '+repr(tuple(sums)))
                parent_count,gate_count=sums[parent_key],sums[key]
                if any(row['entries']!=64 or row['returns']!=64 or row['unwinds'] for row in (parent_count,gate_count)):
                    raise ValueError('Wrong inherited original parent and child counts')
                if (gate_count['literal_statements']==0)!=(mode=='native'):
                    raise ValueError('Wrong inherited child retained accounting')
                proof.setdefault('inherited_accounting',{})[mode]=dict(parent=parent_count,child=gate_count)
                continue
            for owner,(unit,_,_,image) in units.items():
                expected=[row for row in rows if row['owner']==owner]
                stops=sum(row['stopped'] for row in expected)
                counts=[row for vals in accounting['rows']
                        if (row:=dict(zip(accounting['row_fields'],vals,strict=True)))['symbol']==symbol_id and
                        accounting['units'][row['unit']][0]==unit and accounting['image_names'][row['image']]==image]
                if len(counts)!=1 or len(expected)!=297 or stops==0:
                    raise ValueError('Missing actual linked 3EB10 accounting '+owner)
                count=counts[0]
                if (count['entries'],count['returns'],count['unwinds'],count['literal_sites'])!=(297,297-stops,stops,site_count):
                    raise ValueError('Unexpected actual linked 3EB10 entries or exits '+owner)
                retained=(count['body_entries'],count['literal_statements'])
                if (mode=='native' and retained!=(0,0)) or (mode!='native' and min(retained)<=0):
                    raise ValueError('Wrong retained accounting for '+owner+' '+mode)
                proof['owners'][owner].setdefault('accounting',{})[mode]=count
            if filter_owner:
                genuine=[row for values in accounting['rows']
                         if (row:=dict(zip(accounting['row_fields'],values,strict=True)))['symbol']=='bcdf0' and
                         accounting['units'][row['unit']][0]==units['particle-draw'][0]]
                if len(genuine)!=1 or genuine[0]['entries']!=297 or genuine[0]['returns']!=297:
                    raise ValueError('Missing genuine linked BCDF0 child returns')
                proof.setdefault('genuine_child_accounting',{})[mode]=genuine[0]
        if len(set(packet_pins.values()))!=1:
            raise ValueError('Archived, native and reference linked packets differ')
        if inventory(source)!=(pins|extra_pins):
            raise ValueError('Independent R32 source changed')
        proof.update(passed=True,linked_packet_sha256=packet_pins,actual_exports=not bool(args.map),
                     actual_inherited_world=bool(args.map))
    except BaseException as error:
        proof['error']=repr(error)
        raise
    finally:
        save();check_budget(output)
    return 0


if __name__=='__main__':
    raise SystemExit(main())

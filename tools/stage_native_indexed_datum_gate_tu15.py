#!/usr/bin/env python3
"""Stage, configure and verify the three exact 3EB10 source bindings."""
import argparse
from concurrent.futures import ThreadPoolExecutor
import difflib
import json
import os
from pathlib import Path
import re
import subprocess
import sys

from build_launcher_engine import freeze_source
from collect_native_production_closure import prepare_peer_source
import native_indexed_datum_gate_replacements as binding
from native_metadata import check_budget, retain_bytes
from replay_native_object_ratio_caller_link_tu15 import inventory, sha
from stage_native_object_ratio_caller_tu15 import replace_one
from validate_native_indexed_datum_gate_source_tu15 import CONFIG
from validate_native_replacement_snapshot import freeze_test_code

ROOT=Path(__file__).resolve().parents[1]
NAMES=('original_tu15_native_indexed_datum_gate_frame','original_mappack11_native_indexed_datum_gate_frame',
       'native_indexed_datum_gate_source','native_indexed_datum_gate_linked')
MODULE='cmake/NativeIndexedDatumGate.cmake'


def stage(source):
    changes={}

    def edit(path,fn):
        target=source/path;old=target.read_text();new=fn(old)
        if old==new:
            raise ValueError('No exact staged change: '+path)
        changes[path]=(old,new);target.write_text(new)

    def add(path,old,new):
        edit(path,lambda text:replace_one(text,old,new))

    for unit,_,_,_ in CONFIG.values():
        edit(unit,binding.replace_indexed_datum_gate)
    primary='tools/generate_physics_live_support_tu15.py'
    def primary_generator(text):
        text=replace_one(text,'install_object_function_scalar=False):',
                         'install_object_function_scalar=False,install_indexed_datum_gate=False):')
        anchor='    if install_codec_copy_rows:\n'
        text=replace_one(text,anchor,'''    if install_indexed_datum_gate:
        if paired['entries'].get('0003eb10')!=0x3eb10:
            raise ValueError('Primary 3EB10 is absent from the authenticated entry list')
        from native_indexed_datum_gate_replacements import replace_indexed_datum_gate
        generated=replace_indexed_datum_gate(generated)
'''+anchor)
        return replace_one(text,'install_object_function_scalar=True)',
                           'install_object_function_scalar=True,install_indexed_datum_gate=True)')
    edit(primary,primary_generator)
    add('tools/generate_effect_particle_frame_tu15.py',
        '    a.output.write_text(replace_effect_reference_element(source))',
        '''    from native_indexed_datum_gate_replacements import replace_indexed_datum_gate
    a.output.write_text(replace_indexed_datum_gate(replace_effect_reference_element(source)))''')
    add('tools/generate_unit_animation_selection_runtime_1_1.py',
        '    if not a.output.exists() or a.output.read_text()!=source:',
        '''    if RUNTIME_FUNCTION=='source_run_death_startup_bodies_1_1':
        from native_indexed_datum_gate_replacements import replace_indexed_datum_gate
        source=replace_indexed_datum_gate(source)
    if not a.output.exists() or a.output.read_text()!=source:''')
    add('tools/specialize_native_operands.py',"""    receipt['specialized_sha256']=digest(output)""",
        """    indexed_datum_entries=[]
    if 'native_indexed_datum_gate_owner_detail' in text or 'native_tu15_indexed_datum_gate_source_owner.hpp' in text:
        import native_indexed_datum_gate_replacements as indexed_datum
        output=indexed_datum.retag_specialized(text,output)
        indexed_datum_entries=indexed_datum.installed_entries(output)
    receipt['native_indexed_datum_gate_entries']=indexed_datum_entries
    receipt['specialized_sha256']=digest(output)""")
    add('cmake/NativeOperandSpecialization.cmake','native_object_ratio_caller_replacements.py ',
        'native_object_ratio_caller_replacements.py native_indexed_datum_gate_replacements.py ')
    add('CMakeLists.txt','include(cmake/NativeTu15ObjectRatioCaller.cmake)',
        'include(cmake/NativeTu15ObjectRatioCaller.cmake)\ninclude('+MODULE+')')
    def catalog(text):
        text=replace_one(text,"    ('178070',", "    ('3eb10','native_indexed_datum_gate_replacements','replace_indexed_datum_gate'),\n    ('178070',")
        anchor="        elif name=='native_codec_copy_replacements':\n            choices="
        text=replace_one(text,anchor,"""        elif name=='native_indexed_datum_gate_replacements':
            choices=[module.prefix(literal,image) for _,_,_,image in module.CONTEXTS
                     for literal in (False,True) if module.prefix(literal,image) in text]
            if not choices:continue
            prefix=choices[0]
"""+anchor)
        anchor="            elif name=='native_object_ratio_caller_replacements':\n                rebound="
        return replace_one(text,anchor,"""            elif name=='native_indexed_datum_gate_replacements':
                rebound=bind(module.remove_indexed_datum_gate(text))
"""+anchor)
    edit('tools/native_source_prefixes.py',catalog)
    def coordinator(text):
        anchor="               'original_tu15_native_object_ratio_caller_frame|"
        text=replace_one(text,anchor,"               '"+'|'.join(NAMES)+"|'\n"+anchor)
        text=replace_one(text,"            '--target', 'native_object_ratio_caller_tests',",
            "            '--target', 'native_indexed_datum_gate_linked_tests',\n            '--target', 'native_object_ratio_caller_tests',")
        anchor="            '--retain-report', 'native-object-ratio-caller-frame/report.json',"
        return replace_one(text,anchor,''.join("            '--retain-report', '"+file+"',\n" for file in (
            'native-indexed-datum-gate-tu15/report.json','native-indexed-datum-gate-mappack11/report.json',
            'native-indexed-datum-gate-source/report.json','native-indexed-datum-gate-linked.json'))+anchor)
    edit('tools/validate_native_replacement_snapshot.py',coordinator)
    return changes


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--xbe',type=Path,required=True)
    args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Use tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();source=scratch/'source'
    output=args.output.resolve();output.mkdir(parents=True,exist_ok=False)
    proof=dict(passed=False,installed=False,owned_scratch=str(scratch),retained_large_artifacts=[])

    def run(label,command,*,env=None):
        print('3EB10_STAGE',label,flush=True)
        result=subprocess.run(command,cwd=source,capture_output=True,text=True,timeout=1100,env=env)
        (output/(label+'.log')).write_text((result.stdout+result.stderr)[-12000:])
        if result.returncode:
            raise ValueError(label+' failed: '+(result.stdout+result.stderr)[-6000:])
        return result.stdout

    try:
        freeze_source(ROOT,source);freeze_test_code(ROOT,source)
        before=inventory(source);changes=stage(source)
        after=inventory(source)
        if set(k for k in after if before.get(k)!=after[k])!=set(changes):
            raise ValueError('Unexpected staged file inventory changes')
        proof['paths']={path:dict(before=before[path],after=after[path]) for path in changes}
        patch=''.join(''.join(difflib.unified_diff(a.splitlines(keepends=True),b.splitlines(keepends=True),
            fromfile='a/'+path,tofile='b/'+path)) for path,(a,b) in changes.items())
        proposal=output/'proposal.patch';proposal.write_text(patch)
        proof['proposal_sha256']=sha(proposal)
        manifest=json.dumps(after,indent=2,sort_keys=True).encode()+b'\n'
        proof['source_manifest']=retain_bytes(manifest,output/'source-hashes.json',output)
        tracked=(MODULE,'tools/native_indexed_datum_gate_replacements.py',
            'tools/validate_native_indexed_datum_gate_frame_tu15.py','tools/validate_native_indexed_datum_gate_source_tu15.py',
            'tools/validate_native_indexed_datum_gate_linked_tu15.py',
            *(k for k in before if 'native_tu15_indexed_datum_gate' in k))
        proof['inputs']={name:before[name] for name in tracked}
        env={**os.environ,'PYTHONPATH':str(source/'tools'),'CARGO_NET_OFFLINE':'true'}
        check=scratch/'check.py'
        check.write_text("""
from pathlib import Path
import hashlib,json,subprocess,sys
import native_indexed_datum_gate_replacements as b
from native_source_prefixes import authenticated_prefixes
from specialize_native_operands import specialize
from validate_native_indexed_datum_gate_source_tu15 import CONFIG
from native_object_function_query_replacements import installed_entries as c9070
from native_object_function_scalar_replacements import installed_entries as bcb70
from native_object_ratio_caller_replacements import installed_entries as ratio
root=Path.cwd();scratch=Path(sys.argv[1]);report={}
for label,(unit,gen,_,_) in CONFIG.items():
    fresh=scratch/(label+'-fresh.cpp')
    cmd=[sys.executable,'-B',root/'tools'/gen,'--output',fresh]
    cmd+=['--directory',scratch/'particle-input'] if label=='Particle' else ['--manifest',scratch/(label+'.json')]
    subprocess.run(cmd,check=True,stdout=subprocess.DEVNULL)
    for cohort,raw in (('current',(root/unit).read_text()),('fresh',fresh.read_text())):
        formats=[('raw',raw)]
        if label!='Death':
            prepared,receipt=specialize(raw)
            assert receipt['native_indexed_datum_gate_entries']==['3eb10']
            assert specialize(prepared)[0]==prepared
            assert b.remove_indexed_datum_gate(prepared)==specialize(b.remove_indexed_datum_gate(raw))[0]
            formats.append(('prepared',prepared))
        for form,text in formats:
            canonical=b.remove_indexed_datum_gate(text)
            assert b.installed_entries(text)==['3eb10'] and b.replace_indexed_datum_gate(canonical)==text
            if label=='Primary':
                assert c9070(text)==['c9070'] and bcb70(text)==['bcb70'] and ratio(text)==['1046c0']
            catalog=authenticated_prefixes(root,root/unit,text)
            assert catalog['3eb10']['authenticated'],catalog['3eb10']
            marker=b.prefix(form=='prepared','MapPack11' if label=='Death' else 'TitleUpdate15')
            image='TitleUpdate15' if label=='Death' else 'MapPack11'
            bad=(text.replace(marker,marker+marker,1),text.replace(marker,b.prefix(form=='prepared',image),1),
                 text.replace(marker,b.prefix(form!='prepared','MapPack11' if label=='Death' else 'TitleUpdate15'),1),
                 text.replace(marker,'',1),text.replace('s.flags.logic((s.reg(0,1,0))^(s.reg(0,1,0)),1),1,0);',
                    's.flags.logic(1u,1),1,0);',1))
            changed=bad[:4]+(text.replace(b.BODY.search(text)[0],b.BODY.search(text)[0].replace('0x3eb5au','0x3eb5bu'),1),)
            for defect in changed:
                try:b.installed_entries(defect)
                except ValueError:pass
                else:raise ValueError('3EB10 binding accepted unknown source')
            key=label+'-'+cohort+'-'+form;out=scratch/(key+'.cpp')
            out.write_text(text)
            report[key]={'sha256':hashlib.sha256(text.encode()).hexdigest(),'refusals':len(changed)}
print(json.dumps(report))
""")
        forms=json.loads(run('exact-source-catalog',[sys.executable,'-B',check,scratch],env=env))
        if len(forms)!=10:raise ValueError('Missing actual 3EB10 source form')
        proof['forms']=forms
        jobs=[(Path(compiler).name+'-'+name,[compiler,'-std=c++20','-fsyntax-only','-w',
              '-I'+str(source/'include'),'-I'+str(source/'src'),scratch/(name+'.cpp')])
              for compiler in ('g++','clang++') for name in forms]
        with ThreadPoolExecutor(max_workers=3) as pool:
            futures=[pool.submit(run,*job) for job in jobs]
            for future in futures:future.result()
        proof['whole_syntax_checks']=len(jobs)
        owner=scratch/'registration';owner.mkdir()
        dependency,dependency_inputs=prepare_peer_source(ROOT/'build-perf-4k60/_seed/halo2_datachannel-src',owner)
        run('configure',[sys.executable,'-B',source/'tools/run_tests.py','--build-worker','--source',source,
            '--output',output/'cmake','--build-only','--target','help',
            '--cmake-arg=-DHALO2_NATIVE_EXECUTION_ACCOUNTING=ON','--cmake-arg=-DHALO2_STAGE_CUSTOM_ASSETS=OFF',
            '--cmake-arg=-DHALO2_TEST_ASSET_SOURCE_ROOT='+str(ROOT),
            '--cmake-arg=-DHALO2_TU15_GAMEPLAY_XBE='+str(args.xbe.resolve()),
            '--cmake-arg=-DFETCHCONTENT_SOURCE_DIR_HALO2_DATACHANNEL='+str(dependency),
            '--cmake-arg=-DFETCHCONTENT_FULLY_DISCONNECTED=ON'],env={**env,'HALO2_TEST_SCRATCH':str(owner)})
        tests={row['name']:row['command'] for row in json.loads(subprocess.check_output(
            ['ctest','--test-dir',owner/'build','--show-only=json-v1'],cwd=source,text=True,timeout=120))['tests']}
        for name in NAMES:
            if name not in tests:raise ValueError('Missing actual CTest registration '+name)
        run('selector',[sys.executable,'-B',source/'tests/test_native_cohort_test_filter.py',
            '--report',output/'cohort-filter.json'],env=env)
        selected=json.loads(subprocess.check_output([sys.executable,'-B','-c',
            'import json;from validate_native_replacement_snapshot import TEST_NAMES,TEST_FILTER;print(json.dumps([TEST_NAMES,TEST_FILTER]))'],
            cwd=source,text=True,env=env,timeout=60))
        if any(name not in selected[0] or not re.fullmatch(selected[1],name) or re.fullmatch(selected[1],name+'-extra') for name in NAMES):
            raise ValueError('Exact CTest selector differs')
        proof['registration']={name:tests[name] for name in NAMES}
        proof['dependency_inputs']=dependency_inputs
        if any(sha(ROOT/name)!=value for name,value in before.items() if name in set(changes)|set(tracked)):
            raise ValueError('Live 3EB10 stage preimages changed')
        proof.update(passed=True,live_preimages_match=True)
    except Exception as error:
        proof['error']=repr(error)
        raise
    finally:
        (output/'report.json').write_text(json.dumps(proof,indent=2,sort_keys=True)+'\n')
        check_budget(output)
    return 0


if __name__=='__main__':
    raise SystemExit(main())

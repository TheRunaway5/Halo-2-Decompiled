#!/usr/bin/env python3
"""Qualify a complete private 1046C0 source insertion without editing the checkout."""
import argparse
import difflib
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

from build_launcher_engine import freeze_source, source_hashes
from native_metadata import load, retain_bytes
from validate_native_replacement_snapshot import auxiliary_source_hashes, freeze_test_code, test_code_hashes

ROOT=Path(__file__).resolve().parents[1]
PRIMARY='src/physics/physics_live_support_tu15_generated.cpp'
GEN='tools/generate_physics_live_support_tu15.py'
RECORD='tools/native_object_record_ratio_replacements.py'
BINDER='tools/native_object_ratio_caller_replacements.py'
SPECIALIZER='tools/specialize_native_operands.py'
PREFIXES='tools/native_source_prefixes.py'
CMAKE='cmake/NativeOperandSpecialization.cmake'
OWNER_NAMES=('native_object_ratio_caller_replacements','native_object_ratio_caller_owner_detail',
             'native_object_ratio_caller_detail','native_tu15_object_ratio_caller_source_owner.hpp')


def digest(data):
    return hashlib.sha256(data.encode() if isinstance(data,str) else data).hexdigest()


def inventory(root):
    return {**source_hashes(root),**test_code_hashes(root),**auxiliary_source_hashes(root)}


def replace_one(text,old,new):
    if text.count(old)!=1:raise ValueError('Unexpected private insertion anchor: '+old[:160])
    return text.replace(old,new,1)


def stage_edits(source,registration):
    sys.path.insert(0,str(source/'tools'))
    import native_object_ratio_caller_replacements as binding
    changes={}
    def edit(name,update):
        text=(source/name).read_text()
        after=update(text)
        if after==text:raise ValueError('Empty proposed change: '+name)
        changes[name]=(changes[name][0] if name in changes else text,after)
        (source/name).write_text(after)
    edit(RECORD,lambda text:replace_one(text,
        '    if tuple(flight.digest(value) for value in (body,parents[0]))!=PINS[literal]:',
        '''    parent=parents[0]
    if 'native_object_ratio_caller_owner_detail' in parent:
        from native_object_ratio_caller_replacements import canonical_parent
        parent=canonical_parent(parent)
    if tuple(flight.digest(value) for value in (body,parent))!=PINS[literal]:'''))
    def generator(text):
        text=replace_one(text,'install_object_function_query=False):',
                         'install_object_function_query=False,install_object_ratio_caller=False):')
        anchor='    if install_codec_copy_rows:\n'
        text=replace_one(text,anchor,'''    if install_object_ratio_caller:
        if not install_object_record_ratio or paired['entries'].get('001046c0')!=0x1046c0:
            raise ValueError('Primary 1046C0 and its actual record child must be authenticated')
        from native_object_ratio_caller_replacements import replace_object_ratio_caller
        generated=replace_object_ratio_caller(generated)
'''+anchor)
        return replace_one(text,'install_object_function_query=True)',
                           'install_object_function_query=True,install_object_ratio_caller=True)')
    edit(GEN,generator)
    def specializer(text):
        anchor="    receipt['native_object_record_ratio_entries']=record_ratio_entries\n"
        return replace_one(text,anchor,anchor+'''    ratio_caller_entries=[]
    if any(marker in text for marker in ('native_tu15_object_ratio_caller_source_owner.hpp',
            'native_object_ratio_caller_owner_detail','native_object_ratio_caller_detail')):
        caller_spec=importlib.util.spec_from_file_location('native_object_ratio_caller_binding',
            Path(__file__).with_name('native_object_ratio_caller_replacements.py'))
        caller=importlib.util.module_from_spec(caller_spec)
        caller_spec.loader.exec_module(caller)
        caller.installed_entries(text)
        output=caller.retag_specialized(text,output)
        ratio_caller_entries=caller.installed_entries(output)
    receipt['native_object_ratio_caller_entries']=ratio_caller_entries
''')
    edit(SPECIALIZER,specializer)
    edit(CMAKE,lambda text:replace_one(text,' native_object_record_ratio_replacements.py ',
                                           ' native_object_record_ratio_replacements.py native_object_ratio_caller_replacements.py '))
    def prefixes(text):
        anchor="    ('107140','native_object_record_ratio_replacements','replace_object_record_ratio'),\n"
        text=replace_one(text,anchor,anchor+
            "    ('1046c0','native_object_ratio_caller_replacements','replace_object_ratio_caller'),\n")
        text=replace_one(text,"'native_object_record_ratio_replacements','native_vehicle_render_root_replacements'",
                         "'native_object_record_ratio_replacements','native_object_ratio_caller_replacements','native_vehicle_render_root_replacements'")
        anchor="            if name=='native_object_table_ratio_replacements':\n"
        text=replace_one(text,anchor,'''            if name=='native_object_ratio_caller_replacements':
                symbols=('src/rendering/effects/source_effect_compiled_symbols_tu15.cpp','src/rendering/effects/source_object_function_symbols_tu15.inc')
                evidence['validation_source_sha256']={p:hashlib.sha256((Path(root)/p).read_bytes()).hexdigest()for p in symbols}
                evidence['retained_fallback_scope']='Exact TU1.5 1046C0 retains all 406 original callbacks, independent FC510, 107140 and CombatAction child routes, and scoped observed x86 arithmetic flags. MapPack11 and unsupported x86 backends stay retained.'
'''+anchor)
        anchor="            elif name=='native_object_table_ratio_replacements':\n"
        return replace_one(text,anchor,'''            elif name=='native_object_ratio_caller_replacements':
                rebound=bind(module.remove_object_ratio_caller(text))
'''+anchor)
    edit(PREFIXES,prefixes)
    def older_record(text):
        anchor='    raw = [binding.remove_object_record_ratio(text) for text in raw]'
        return replace_one(text,anchor,'''    from native_object_ratio_caller_replacements import remove_object_ratio_caller
    raw = [binding.remove_object_record_ratio(remove_object_ratio_caller(text)) for text in raw]''')
    edit('tools/validate_native_object_record_ratio_source_tu15.py',older_record)
    def prior_selector(text):
        anchor='    variants=raw+[specialize(text)[0] for text in raw]'
        return replace_one(text,anchor,'''    from native_object_ratio_caller_replacements import remove_object_ratio_caller
    raw=[remove_object_ratio_caller(text) for text in raw]
'''+anchor)
    edit('tools/validate_native_object_ratio_caller_dispatch_source_tu15.py',prior_selector)
    def source_tests(text):
        anchor="""    if require_binding:
        raise ValueError('1046C0 has no installed native default')"""
        text=replace_one(text,anchor,'    from native_object_ratio_caller_replacements import installed_entries, remove_object_ratio_caller')
        anchor='    variants=raw+[specialize(text)[0] for text in raw]'
        return replace_one(text,anchor,'''    if require_binding:
        for unit in raw:
            installed_entries(unit)
            installed_entries(specialize(unit)[0])
    raw=[remove_object_ratio_caller(text) for text in raw]
'''+anchor)
    edit('tools/validate_native_object_ratio_caller_source_tu15.py',source_tests)
    if registration:
        registration_edits(edit)
    edit(PRIMARY,binding.replace_object_ratio_caller)
    if binding.remove_object_ratio_caller(changes[PRIMARY][1])!=changes[PRIMARY][0]:
        raise ValueError('Whole Primary exact inverse failed')
    return changes


def registration_edits(edit):
    def inherited_accounting(text):
        text=replace_one(text,'def accounting(path, reference, complete, stopped, inherited):',
            'def accounting(path, reference, complete, stopped, inherited, native_ratio=False):')
        a='''    if (child['entries'], child['body_entries'], child['returns'], child['unwinds']) != (
        expected, expected, expected - stopped, stopped) or child['literal_statements'] < expected:
        raise ValueError(f'C9070 lost its separately retained 1046C0 accounting: {child}')'''
        b='''    if (child['literal_sites'], child['entries'], child['returns'], child['unwinds']) != (
        406, expected, expected - stopped, stopped):
        raise ValueError(f'C9070 lost its separate 1046C0 owner accounting: {child}')
    if native_ratio:
        if child['body_entries'] or child['literal_statements'] or (
            child['returns_without_body'], child['unwinds_without_body']) != (expected - stopped, stopped):
            raise ValueError(f'C9070 executed the retained 1046C0 body by default: {child}')
    elif child['body_entries'] != expected or child['literal_statements'] < expected:
        raise ValueError(f'C9070 lost its separately retained 1046C0 accounting: {child}')'''
        text=replace_one(text,a,b)
        a="    parser.add_argument('--accounting', action='store_true')"
        text=replace_one(text,a,a+"\n    parser.add_argument('--native-1046c0', action='store_true')")
        text=replace_one(text,"accounting(counter, reference, complete, stopped, mode == 'inherited')",
            "accounting(counter, reference, complete, stopped, mode == 'inherited', args.native_1046c0)")
        return replace_one(text,'retained_1046c0=True','retained_1046c0=not args.native_1046c0')
    edit('tools/validate_native_object_function_query_public_tu15.py',inherited_accounting)
    edit('cmake/NativeTu15ObjectFunctionQuery.cmake',lambda text:replace_one(text,
        'native-object-function-query-public.json" ${_halo2_c9070_accounting})',
        'native-object-function-query-public.json" --native-1046c0 ${_halo2_c9070_accounting})'))
    edit('CMakeLists.txt',lambda text:replace_one(text,'include(cmake/NativeTu15ObjectFunctionQuery.cmake)\n',
        'include(cmake/NativeTu15ObjectFunctionQuery.cmake)\ninclude(cmake/NativeTu15ObjectRatioCaller.cmake)\n'))
    def coordinator(text):
        a="               'native_object_function_query_(binding|public)|'\n"
        text=replace_one(text,a,a+"               'original_tu15_native_object_ratio_caller_frame|native_object_ratio_caller_(source|world|inherited)|'\n")
        a="            '--target', 'native_object_function_query_tests',\n"
        text=replace_one(text,a,a+"            '--target', 'native_object_ratio_caller_tests',\n")
        a="            '--retain-report', 'native-object-function-query-public.json',\n"
        return replace_one(text,a,a+"""            '--retain-report', 'native-object-ratio-caller-frame/report.json',
            '--retain-report', 'native-object-ratio-caller-source/report.json',
            '--retain-report', 'native-object-ratio-caller-world.json',
            '--retain-report', 'native-object-ratio-caller-inherited.json',
""")
    edit('tools/validate_native_replacement_snapshot.py',coordinator)
    edit(PREFIXES,lambda text:replace_one(text,'the retained 1046C0 inherited route',
                                             'the independent 1046C0 inherited route'))
    def catalog(text):
        text=replace_one(text,"for entry in ('c9070','d3950','107140'):",
                              "for entry in ('c9070','d3950','107140','1046c0'):")
        text=replace_one(text,"            self.assertIn('retained 1046C0',proof['retained_fallback_scope'])\n            self.assertNotIn('1046c0',rows)",
            """            self.assertIn('independent 1046C0',proof['retained_fallback_scope'])
            import native_object_ratio_caller_replacements as caller
            self.assertEqual(caller.replace_object_ratio_caller(caller.remove_object_ratio_caller(text)),text)
            wrong_caller=text.replace(caller.prefix(literal),caller.prefix(not literal),1)
            self.assertFalse(authenticated_prefixes(ROOT,path,wrong_caller)['1046c0']['authenticated'])""")
        return text
    edit('tests/test_native_migration_inventory.py',catalog)


def structural(source,scratch,output,changes,reuse=None):
    driver=scratch/'check-private-binding.py'
    driver.write_text("""
import hashlib,json,sys
from pathlib import Path
root,scratch=map(Path,sys.argv[1:])
sys.path.insert(0,str(root/'tools'))
import native_object_ratio_caller_replacements as caller
import native_object_record_ratio_replacements as record
import native_object_function_query_replacements as c9070
from specialize_native_operands import specialize
unit=root/'src/physics/physics_live_support_tu15_generated.cpp'
result={}
for label,path in (('current',unit),('fresh',scratch/'fresh.cpp')):
    raw=path.read_text()
    assert caller.installed_entries(raw)==['1046c0'] and record.installed_entries(raw)==['107140']
    assert c9070.installed_entries(raw)==['c9070']
    canonical=caller.remove_object_ratio_caller(raw)
    prepared,receipt=specialize(raw)
    assert receipt['native_object_ratio_caller_entries']==['1046c0']
    assert receipt['native_object_record_ratio_entries']==['107140']
    assert receipt['native_object_function_query_entries']==['c9070']
    assert caller.remove_object_ratio_caller(prepared)==specialize(canonical)[0]
    again,repeat=specialize(prepared)
    assert again==prepared and repeat['native_object_ratio_caller_entries']==['1046c0']
    assert record.installed_entries(prepared)==['107140'] and c9070.installed_entries(prepared)==['c9070']
    count=0
    for fmt,bound in (('raw',raw),('prepared',prepared)):
        assert caller.replace_object_ratio_caller(caller.remove_object_ratio_caller(bound))==bound
        body=caller.BODY.search(bound)[0]
        installed_tag=fmt=='prepared'
        marker=caller.prefix(installed_tag)
        replacements=(
            (marker,marker+marker),
            (marker,caller.prefix(not installed_tag)),
            (marker,''),
            ('0x10481au);run_107140(s);','0x10481bu);run_107140(s);'),
            ('(0x46a178u),4','(0x46a179u),4'),
            ('shift_definition_index<true>(s);','shift_definition_index<false>(s);'),
        )
        for before,after in replacements:
            assert before in body
            changed=bound.replace(body,body.replace(before,after,1),1)
            for check in (caller.installed_entries,caller.replace_object_ratio_caller,caller.remove_object_ratio_caller):
                try:check(changed)
                except ValueError:count+=1
                else:raise ValueError('Missed exact owner refusal for '+before)
        cpp=scratch/(label+'-'+fmt+'.cpp');cpp.write_text(bound)
    result[label]={'raw':caller.digest(raw),'prepared':caller.digest(prepared),'strict_refusals':count}
print(json.dumps(result,sort_keys=True))
""")
    stages=[]
    def run(name,argv,timeout=1200):
        print('1046C0_PRIVATE_BINDING',name,flush=True)
        completed=subprocess.run(argv,cwd=source,capture_output=True,text=True,timeout=timeout)
        (output/(name+'.log')).write_text((completed.stdout+completed.stderr)[-15000:])
        stages.append(dict(name=name,command=list(map(str,argv)),exit_code=completed.returncode))
        if completed.returncode:raise ValueError(name+' failed: '+(completed.stdout+completed.stderr)[-5500:])
        return completed.stdout
    run('generation',[sys.executable,'-B',source/GEN,'--output',scratch/'fresh.cpp','--manifest',scratch/'generation.json'])
    result=json.loads(run('exact-inverse',[sys.executable,'-B',driver,source,scratch]))
    if reuse:
        if result!=reuse['formats'] or reuse.get('whole_cpp_syntax_passed')!=8:
            raise ValueError('The new generated forms differ from the earlier syntax inputs')
        return dict(formats=result,commands=stages,source_units=len(changes),
                    whole_cpp_syntax_passed=8,syntax_replayed=False)
    for compiler in ('g++','clang++'):
        for label in ('current','fresh'):
            for fmt in ('raw','prepared'):
                name=label+'-'+fmt
                run(compiler+'-'+name,[compiler,'-std=c++20','-fsyntax-only','-w','-I'+str(source/'include'),
                                      '-I'+str(source/'src'),scratch/(name+'.cpp')])
    return dict(formats=result,commands=stages,source_units=len(changes),whole_cpp_syntax_passed=8)


def check_registration(source,scratch,output,xbe):
    from collect_native_production_closure import prepare_peer_source
    owner=scratch/'registration';owner.mkdir()
    dependency,dependency_inputs=prepare_peer_source(ROOT/'build-perf-4k60/_seed/halo2_datachannel-src',owner)
    args=[sys.executable,'-B',source/'tools/run_tests.py','--build-worker','--source',source,
          '--output',output/'cmake','--build-only','--target','help',
          '--cmake-arg=-DHALO2_NATIVE_EXECUTION_ACCOUNTING=ON','--cmake-arg=-DHALO2_STAGE_CUSTOM_ASSETS=OFF',
          '--cmake-arg=-DHALO2_TEST_ASSET_SOURCE_ROOT='+str(ROOT),'--cmake-arg=-DHALO2_TU15_GAMEPLAY_XBE='+str(xbe),
          '--cmake-arg=-DFETCHCONTENT_SOURCE_DIR_HALO2_DATACHANNEL='+str(dependency),
          '--cmake-arg=-DFETCHCONTENT_FULLY_DISCONNECTED=ON']
    env={**os.environ,'HALO2_TEST_SCRATCH':str(owner),'CARGO_NET_OFFLINE':'true'}
    print('1046C0_PRIVATE_BINDING configured-selector',flush=True)
    configured=subprocess.run(args,cwd=source,env=env,capture_output=True,text=True,timeout=600)
    if configured.returncode:raise ValueError('Private CMake configuration failed: '+configured.stdout[-6000:]+configured.stderr[-3000:])
    result=subprocess.run(['ctest','--test-dir',owner/'build','--show-only=json-v1'],
                          cwd=source,capture_output=True,text=True,check=True,timeout=120)
    tests={row['name']:row for row in json.loads(result.stdout)['tests']}
    wanted=('original_tu15_native_object_ratio_caller_frame','native_object_ratio_caller_source',
            'native_object_ratio_caller_world','native_object_ratio_caller_inherited')
    if any(name not in tests for name in (*wanted,'native_object_function_query_public')):
        raise ValueError('Missing real CTest registration')
    selected=subprocess.check_output([sys.executable,'-B','-c',
        'import json;from validate_native_replacement_snapshot import TEST_FILTER,TEST_NAMES;print(json.dumps([TEST_FILTER,TEST_NAMES]))'],
        cwd=source,text=True,timeout=60,env={**env,'PYTHONPATH':str(source/'tools')})
    pattern,names=json.loads(selected)
    if not all(re.fullmatch(pattern,name) and name in names and not re.fullmatch(pattern,name+'-extra') for name in wanted):
        raise ValueError('Cohort selection failed exact registration')
    if '--native-1046c0' not in tests['native_object_function_query_public']['command']:
        raise ValueError('Registered C9070 check lost independent native 1046C0 verification')
    test_results={}
    for label,args in (
        ('catalog',[sys.executable,'-B',source/'tests/test_native_migration_inventory.py',
            'NativeMigrationInventoryTests.test_object_function_query_catalog_preserves_genuine_siblings']),
        ('cohort-filter',[sys.executable,'-B',source/'tests/test_native_cohort_test_filter.py',
            '--report',output/'cohort-filter.json']),
    ):
        run=subprocess.run(args,cwd=source,env=env,capture_output=True,text=True,timeout=1000)
        (output/(label+'.log')).write_text((run.stdout+run.stderr)[-16000:])
        if run.returncode:raise ValueError(label+' failed: '+(run.stdout+run.stderr)[-6000:])
        test_results[label]=True
    return dict(actual_ctest_names=list(wanted),focused_tests=test_results,
                cohort_names=len(names),commands={name:tests[name]['command'] for name in wanted},
                dependency_commit_inputs=dependency_inputs,full_configured_build_run=False)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('donor-source','donor-inventory','output'):
        parser.add_argument('--'+name,type=Path,required=True)
    parser.add_argument('--registration',action='store_true')
    parser.add_argument('--prior-syntax',type=Path)
    parser.add_argument('--prior-forms',type=Path)
    parser.add_argument('--tu15-xbe',type=Path)
    args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Use tools/run_test.py')
    if args.registration and not args.tu15_xbe:parser.error('--registration needs the original TU1.5 image')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    source=scratch/'source';output=args.output.resolve()
    output.mkdir(parents=True,exist_ok=False)
    proof=dict(passed=False,installed=False,owned_scratch=str(scratch),retained_large_artifacts=[])
    def save():
        (output/'report.json').write_text(json.dumps(proof,indent=2,sort_keys=True)+'\n')
    try:
        donor=args.donor_source.resolve(strict=True)
        before=load(args.donor_inventory)
        freeze_source(donor,source);freeze_test_code(donor,source)
        if inventory(source)!=before:raise ValueError('Frozen R32 source changed during independent copy')
        overlay=[str(path.relative_to(ROOT)) for folder in ('include/halo2','src/gameplay/objects','tests','tools')
                 for path in sorted((ROOT/folder).glob('*object_ratio_caller*')) if path.is_file() and path.suffix in ('.hpp','.cpp','.py')]
        if args.registration:overlay.append('cmake/NativeTu15ObjectRatioCaller.cmake')
        live={}
        for name in overlay:
            origin=ROOT/name
            data=origin.read_bytes()
            target=source/name;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(data)
            if origin.read_bytes()!=data:raise ValueError('Private proof input changed during copy: '+name)
            live[name]=digest(data)
        if inventory(source)!=(before|live):raise ValueError('Unexpected independent 1046C0 overlay')
        changes=stage_edits(source,args.registration)
        proof['inputs']=live
        proof['paths']={name:dict(before=digest(pair[0]),after=digest(pair[1])) for name,pair in sorted(changes.items())}
        patch=''.join(''.join(difflib.unified_diff(a.splitlines(keepends=True),b.splitlines(keepends=True),
                       fromfile='a/'+name,tofile='b/'+name,n=3)) for name,(a,b) in sorted(changes.items()))
        (output/'proposal.patch').write_text(patch)
        proof['proposal_sha256']=digest(patch)
        proof['live_preimages_match']={name:digest((ROOT/name).read_bytes())==digest(a) for name,(a,_) in changes.items()}
        expected=before|live|{name:digest(b) for name,(_,b) in changes.items()}
        if inventory(source)!=expected:raise ValueError('Private proposal altered an unauthenticated input')
        encoded=(json.dumps(expected,sort_keys=True,indent=2)+'\n').encode()
        proof['manifest_sha256']=digest(encoded)
        proof['manifest_storage']=retain_bytes(encoded,output/'source-hashes.json',output)
        proof['stage']='structural-validation';save()
        previous=None
        if args.prior_syntax:
            previous=json.loads(args.prior_syntax.read_bytes())
            compiled_headers={name:pin for name,pin in live.items() if name.endswith(('.hpp','.cpp'))}
            prior_headers={name:pin for name,pin in previous['inputs'].items() if name.endswith(('.hpp','.cpp'))}
            if compiled_headers!=prior_headers or any(
                proof['paths'][name]!=previous['paths'][name] for name in (PRIMARY,GEN,RECORD,SPECIALIZER)
            ):
                raise ValueError('The staged C++ dependencies differ from the earlier syntax inputs')
            proof['reused_syntax_report_sha256']=digest(args.prior_syntax.read_bytes())
        if args.prior_forms:
            earlier=json.loads(args.prior_forms.read_bytes())
            same_inputs={name:pin for name,pin in live.items() if name!=str(Path(__file__).relative_to(ROOT))}
            prior_inputs={name:pin for name,pin in earlier['inputs'].items() if name!=str(Path(__file__).relative_to(ROOT))}
            if (proof['paths']!=earlier['paths'] or same_inputs!=prior_inputs or
                proof['proposal_sha256']!=earlier['proposal_sha256'] or earlier.get('whole_cpp_syntax_passed')!=8):
                raise ValueError('Exact source or generated-form inputs changed since the prior run')
            proof.update(formats=earlier['formats'],whole_cpp_syntax_passed=8,syntax_replayed=False,
                         reused_generation_and_syntax_report_sha256=digest(args.prior_forms.read_bytes()))
        else:
            proof.update(structural(source,scratch,output,changes,previous))
        if args.registration:proof['registration']=check_registration(source,scratch,output,args.tu15_xbe.resolve())
        if inventory(source)!=expected:raise ValueError('Private structural validation changed source')
        proof.update(passed=True,stage='complete',source_unchanged=True)
    except BaseException as error:
        proof['error']=repr(error);raise
    finally:
        save()
    return 0


if __name__=='__main__':
    raise SystemExit(main())

#!/usr/bin/env python3
"""Authenticate fresh FC510 routing and compile complete real owner units."""
import argparse,hashlib,json,os,re,shlex,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report',type=Path,required=True)
    parser.add_argument('--compiler',default='clang++')
    parser.add_argument('--require-binding',action='store_true')
    args=parser.parse_args();os.chdir(ROOT);scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    import native_object_parent_mode_replacements as binding
    from native_object_parent_mode_inputs import UNIT,EFFECT,HELPER,PUBLIC
    from specialize_native_operands import specialize
    from validate_native_model_marker_lookup_generation_tu15 import CHILD_WRAPPER
    tools={str(p.relative_to(ROOT)):sha(p)for p in(ROOT/'tools').rglob('*.py')}
    headers={p.resolve():sha(p)for tree in('src','include')for p in(ROOT/tree).rglob('*')if p.suffix in('.hpp','.h','.inc')}
    originals={name:(ROOT/name).read_text()for name in(UNIT,EFFECT,HELPER,PUBLIC)}
    pins={name:sha(ROOT/name)for name in originals}
    report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],source_before=pins,execution_qualified_separately=True)
    try:
        fresh=scratch/'primary.cpp';metadata=scratch/'generator-tools.json'
        command=[sys.executable,'-B',str(ROOT/'tools/generate_physics_live_support_tu15.py'),'--output',str(fresh),'--manifest',str(scratch/'primary.json')]
        done=subprocess.run([sys.executable,'-B','-c',CHILD_WRAPPER,str(metadata),*command[2:]],capture_output=True,text=True,timeout=300)
        if done.returncode:raise ValueError('Full Primary generator failed '+done.stderr[-7000:])
        source=fresh.read_text();match,body=binding.authenticate(source)
        canonical_match,canonical_body=binding.authenticate(originals[UNIT])
        if body!=canonical_body:raise ValueError('Fresh FC510 original body differs')
        installed=binding.PREFIX in match[0]and binding.INCLUDE in source
        if args.require_binding and(not installed or binding.replace_object_parent_mode(originals[UNIT])!=originals[UNIT]):
            raise ValueError('Fresh/current Primary native FC510 default missing')
        bound=binding.replace_object_parent_mode(source)
        if binding.replace_object_parent_mode(bound)!=bound:raise ValueError('Fresh FC510 installer not idempotent')
        prepared,receipt=specialize(bound)
        if binding.PREFIX not in binding.BODY.search(prepared)[0]:raise ValueError('Prepared FC510 default missing')
        if receipt['native_bsp_surface_2d_actual_owner_entries']!=['2a0230']or receipt['native_bsp_surface_polygon_actual_owner_entries']!=['1e6e80']:
            raise ValueError('Earlier actual BSP owner routing lost')
        rejected=0
        resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)[0]
        dispatch=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)[0]
        for changed in(
            source.replace(match[0],match[0].replace('0x12cu','0x128u',1),1),
            source.replace(resolver,resolver.replace('return p;','return p+1u;',1),1),
            source.replace(dispatch,dispatch.replace('run_fc510(s);','run_fc280(s);',1),1),
            bound.replace(binding.PREFIX,binding.EFFECT_PREFIX,1)):
            if changed==source:raise ValueError('Missing fresh FC510 mutation witness')
            try:binding.replace_object_parent_mode(changed)
            except ValueError:rejected+=1
            else:raise ValueError('Fresh FC510 complete source mutation admitted')
        helper=binding.replace_effect(originals[HELPER],originals[EFFECT])
        if args.require_binding and(helper!=originals[HELPER]or originals[EFFECT].count(binding.INCLUDE)!=1):
            raise ValueError('Actual Effect FC510 prefix/include missing')
        if binding.replace_effect(helper,originals[EFFECT])!=helper:raise ValueError('Actual Effect installer not idempotent')
        current_effect_body=binding.effect_body(originals[HELPER])
        for changed_helper,changed_cpp in(
            (originals[HELPER].replace(current_effect_body,current_effect_body.replace('0x12c','0x128',1),1),originals[EFFECT]),
            (originals[HELPER],originals[EFFECT].replace('entry==0xfc510u','entry==0xfc511u',1)),
            (helper.replace(binding.EFFECT_PREFIX,binding.PREFIX,1),originals[EFFECT])):
            if changed_helper==originals[HELPER]and changed_cpp==originals[EFFECT]:raise ValueError('Missing Effect mutation witness')
            try:binding.replace_effect(changed_helper,changed_cpp)
            except ValueError:rejected+=1
            else:raise ValueError('Changed actual Effect owner admitted')
        public=binding.replace_value(originals[PUBLIC])
        if args.require_binding and public!=originals[PUBLIC]:raise ValueError('Public parent-mode DATA facade missing')
        if binding.replace_value(public)!=public:raise ValueError('Public installer not idempotent')
        fresh.write_text(bound);prepared_path=scratch/'primary-prepared.cpp';prepared_path.write_text(prepared)
        (scratch/Path(HELPER).name).write_text(helper)
        effect_path=scratch/'effect.cpp';effect_path.write_text(originals[EFFECT]if binding.INCLUDE in originals[EFFECT]else binding.INCLUDE+originals[EFFECT])
        public_path=scratch/'public.cpp';public_path.write_text(public)
        syntax=[];dependencies=set()
        for path in(fresh,prepared_path,effect_path,public_path):
            dep=path.with_suffix('.d')
            cmd=[args.compiler,'-std=c++20','-fsyntax-only','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),'-I'+str(ROOT/'src/gameplay/weapons'),'-MD','-MF',str(dep),str(path)]
            done=subprocess.run(cmd,capture_output=True,text=True,timeout=300)
            if done.returncode:raise ValueError('Whole owner CPP syntax failed '+done.stderr[-7000:])
            syntax.append(dict(command=cmd,sha256=sha(path),passed=True))
            dependencies.update(Path(value).resolve()for value in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]))
        used={str(Path(value).relative_to(ROOT))for value in json.loads(metadata.read_text())if Path(value).is_relative_to(ROOT/'tools')}
        used.update(str(Path(module.__file__).resolve().relative_to(ROOT))for module in tuple(sys.modules.values())if getattr(module,'__file__',None)and Path(module.__file__).resolve().is_relative_to(ROOT/'tools'))
        tool_pins={name:tools[name]for name in sorted(used)}
        header_pins={str(path.relative_to(ROOT)):headers[path]for path in dependencies if path in headers}
        for name,pin in(*pins.items(),*tool_pins.items(),*header_pins.items()):
            if sha(ROOT/name)!=pin:raise ValueError('FC510 generation input changed '+name)
        report.update(passed=True,command=command,primary_body=binding.BODY_PIN,effect_body=binding.EFFECT_PIN,public_predecessor=binding.PUBLIC_PIN,generated_prefix_installed=installed,idempotent=True,strict_source_mutants_rejected=rejected,whole_cpp_syntax=syntax,preparation_tools=tool_pins,header_hashes=header_pins,primary_whole_unit_equal=bound==originals[UNIT],scope='Complete authenticated Primary/Effect/public ownership, fresh raw/prepared routing and standalone syntax; no descendant or engine-wide closure')
    except BaseException as error:report['error']=repr(error);raise
    finally:
        args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    return 0
if __name__=='__main__':raise SystemExit(main())

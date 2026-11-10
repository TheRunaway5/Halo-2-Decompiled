#!/usr/bin/env python3
"""Regenerate complete Shadow/ParticleDraw owners and authenticate both preparations."""
import argparse,contextlib,hashlib,io,json,os,re,runpy,shlex,subprocess,sys
from pathlib import Path
import native_geometry_visibility_replacements as binding
ROOT=Path(__file__).resolve().parents[1]
UNITS=('src/rendering/shadows/source_shadow_frame_tu15_generated.cpp','src/rendering/effects/source_effect_particle_draw_tu15_generated.cpp')
PREPARED_PINS=('35359d8fbcae2b0690ec51c41a6c7361d154563788279cc8e06a5155864ed3ff','d0961ab782a90f30f2ae990343a21e0a4dec8546d10194429d1b7b4042a6cab2')
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def invoke(command):
    saved=sys.argv
    try:
        sys.argv=command[2:]
        with contextlib.redirect_stdout(io.StringIO()):
            try:runpy.run_path(command[2],run_name='__main__')
            except SystemExit as result:
                if result.code not in(None,0):raise
    finally:sys.argv=saved
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--report',type=Path,required=True);parser.add_argument('--compiler',default='clang++');parser.add_argument('--require-binding',action='store_true');args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);tools={p.resolve():sha(p)for p in(ROOT/'tools').rglob('*.py')};headers={p.resolve():sha(p)for tree in('src','include')for p in(ROOT/tree).rglob('*')if p.suffix in('.hpp','.h','.inc')};canonical={unit:sha(ROOT/unit)for unit in UNITS}
    report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],units=[])
    try:
        from generated_routine_ownership import function_bodies
        from specialize_native_operands import specialize
        for index,unit in enumerate(UNITS):
            directory=scratch/str(index);directory.mkdir();raw=directory/('shadow-generated.cpp'if not index else'particle.cpp')
            command=[sys.executable,'-B',str(ROOT/'tools'/('generate_shadow_frame_tu15.py'if not index else'generate_effect_particle_draw_tu15.py'))]
            manifest=directory/'generation.json'
            if not index:
                command+=['--directory',str(directory/'generation'),'--output',str(raw)]
                entries=sorted(set(re.findall(r'^static void run_([0-9a-f]+)\(WorldState &\);$',(ROOT/unit).read_text(),re.M)))
                for entry in entries:command+=['--additional-entry','0x'+entry]
                manifest=directory/'generation/generation.json'
            else:
                command+=['--output',str(raw),'--directory',str(directory/'generation')]
                manifest=directory/'generation/generation.json'
            invoke(command)
            source=raw.read_text();current=(ROOT/unit).read_text();bound=binding.replace_geometry_visibility(source)
            if args.require_binding and bound!=source:raise ValueError('Fresh complete geometry admission default missing')
            if binding.replace_geometry_visibility(bound)!=bound:raise ValueError('Fresh complete geometry admission binding nonidempotent')
            equal=source==current
            if not index and not equal:raise ValueError('Whole Shadow regeneration is not byte-identical')
            if not equal:raise ValueError('Whole renderer generation is not byte-identical '+unit)
            correspondence=dict(whole_unit_equal=True)
            controls=[]
            match,original=binding.authenticate(source)
            resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)[0]
            dispatcher=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)[0]
            for name,mutated in(('body',source.replace(match[0],match[0].replace('0x2a7cu','0x2a78u',1),1)),('resolver',source.replace(resolver,resolver.replace('return p;','return p+1;',1),1)),('dispatcher',source.replace(dispatcher,dispatcher.replace('{','{if(false)return;',1),1))):
                try:binding.replace_geometry_visibility(mutated)
                except ValueError:controls.append(dict(name=name,rejected=True))
                else:raise ValueError('Fresh whole geometry admission source mutation survived '+name)
            prepared,receipt=specialize(bound);body=next(body for sig,body in function_bodies(prepared)if sig[1]=='135970')
            if binding.digest(body)!=PREPARED_PINS[index]:raise ValueError('Complete prepared geometry admission body drift')
            if specialize(prepared)[0]!=prepared:raise ValueError('Whole geometry admission repeat specialization is not idempotent')
            raw.write_text(bound);output=directory/'prepared.cpp';output.write_text(prepared);dependencies=set();syntax=[]
            for source_path in(raw,output):
                dep=source_path.with_suffix('.d');check=[args.compiler,'-std=c++20','-fsyntax-only','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-MD','-MF',str(dep),str(source_path)]
                checked=subprocess.run(check,capture_output=True,text=True,timeout=240)
                if checked.returncode:raise ValueError('Whole geometry admission owner syntax failed: '+checked.stderr[-6500:])
                dependencies.update(Path(x).resolve()for x in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]));syntax.append(dict(command=check,passed=True,source_sha256=sha(source_path)))
            generation=json.loads(manifest.read_text())
            report['units'].append(dict(source=unit,correspondence=correspondence,body_count=len(list(function_bodies(source))),original_body_sha256=binding.digest(binding.authenticate(source)[1]),prepared_body_sha256=PREPARED_PINS[index],raw_prepared_syntax=syntax,header_hashes={str(p.relative_to(ROOT)):headers[p]for p in dependencies if p in headers},fresh_authentication_controls=controls,generation=dict(manifest_sha256=sha(manifest),manifest_bytes=manifest.stat().st_size,original_sha256=generation.get('original_sha256'),correspondence_sha256=generation.get('correspondence_sha256'),generated_cpp_sha256=generation.get('generated_cpp_sha256'))))
        used={Path(module.__file__).resolve()for module in tuple(sys.modules.values())if getattr(module,'__file__',None)};used.add(Path(__file__).resolve())
        changed=[str(p.relative_to(ROOT))for p in used if p in tools and sha(p)!=tools[p]]
        if changed:raise ValueError('Geometry admission generation preparation drift: '+', '.join(changed))
        for row in report['units']:
            for name,pin in row['header_hashes'].items():
                if sha(ROOT/name)!=pin:raise ValueError('Geometry admission generation header drift '+name)
        for unit,pin in canonical.items():
            if sha(ROOT/unit)!=pin:raise ValueError('Geometry admission canonical source drift '+unit)
        report.update(passed=True,canonical_sha256=canonical,preparation_tools_sha256={str(p.relative_to(ROOT)):tools[p]for p in used if p in tools})
    except Exception as error:report['error']=str(error)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({key:report[key]for key in('passed','error')if key in report}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())

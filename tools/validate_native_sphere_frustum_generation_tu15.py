#!/usr/bin/env python3
"""Regenerate the authenticated particle unit and verify native classifier routing."""
import argparse,contextlib,difflib,hashlib,io,json,os,platform,runpy,shlex,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
UNIT='src/rendering/effects/source_effect_particle_draw_tu15_generated.cpp'
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--compiler',default='clang++');args=p.parse_args()
 if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
 if sys.platform!='linux' or platform.machine().lower()not in('x86_64','amd64','i386','i686'):return 77
 scratch=Path(os.environ['HALO2_TEST_SCRATCH']);generated=scratch/'particle.cpp';directory=scratch/'generation'
 # Snapshot before executing any preparation code. Only actually imported
 # local tools and compiler-reported source dependencies enter the receipt.
 tool_start={path.resolve():sha(path)for path in(ROOT/'tools').rglob('*.py')}
 header_start={path.resolve():sha(path)for tree in('include','src')for path in(ROOT/tree).rglob('*')if path.suffix in('.h','.hpp','.inc')}
 from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
 if not NEW_DEFAULT.is_file():return 77
 if sha(NEW_DEFAULT)!=NEW_PIN:raise ValueError('Wrong particle generator original image')
 canonical=(ROOT/UNIT).read_text();canonical_sha=hashlib.sha256(canonical.encode()).hexdigest()
 from native_sphere_frustum_replacements import PREFIX,replace_sphere_frustum,BODY_PIN,canonical_body
 from generated_routine_ownership import function_bodies
 from specialize_native_operands import specialize
 from model_lighting_leaves_native_replacements import prefix as lighting_prefix
 body=lambda source,entry:next(value for signature,value in function_bodies(source)if int(signature[1],16)==entry)
 if replace_sphere_frustum(canonical)!=canonical or PREFIX not in body(canonical,0x184690):raise ValueError('Classifier production binding missing')
 command=[sys.executable,'-B',str(ROOT/'tools/generate_effect_particle_draw_tu15.py'),'--output',str(generated),'--directory',str(directory)]
 original_argv=sys.argv
 try:
  sys.argv=command[2:]
  with contextlib.redirect_stdout(io.StringIO()):runpy.run_path(command[2],run_name='__main__')
 finally:sys.argv=original_argv
 raw=generated.read_text()
 if replace_sphere_frustum(raw)!=raw or PREFIX not in body(raw,0x184690):raise ValueError('Fresh generator omitted native classifier')
 if hashlib.sha256(canonical_body(body(raw,0x184690)).encode()).hexdigest()!=BODY_PIN:raise ValueError('Fresh classifier original continuation drift')
 if body(raw,0x184690)!=body(canonical,0x184690):raise ValueError('Fresh classifier body differs from bound production')
 raw_bodies={int(signature[1],16):value for signature,value in function_bodies(raw)}
 canonical_bodies={int(signature[1],16):value for signature,value in function_bodies(canonical)}
 changed_bodies=sorted(hex(entry)for entry in raw_bodies.keys()|canonical_bodies.keys()if raw_bodies.get(entry)!=canonical_bodies.get(entry))
 # The prior particle lighting native owner must survive this shared seam.
 if lighting_prefix(0x2c390)not in body(raw,0x2c390):raise ValueError('Fresh generation lost prior2C390 lighting binding')
 specialized,receipt=specialize(raw);prepared=scratch/'particle-specialized.cpp';prepared.write_text(specialized)
 if PREFIX not in body(specialized,0x184690)or's.instruction_literal('not in body(specialized,0x184690):raise ValueError('Prepared classifier gate/retained continuation missing')
 syntax=[];dependencies=set()
 for path in(generated,prepared):
  dep=path.with_suffix('.d');cmd=[args.compiler,'-std=c++20','-fsyntax-only','-fno-fast-math','-ffp-contract=off','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-MD','-MF',str(dep),str(path)]
  result=subprocess.run(cmd,capture_output=True,text=True)
  if result.returncode:raise ValueError('Fresh particle syntax failed: '+result.stderr[-6000:])
  syntax.append(dict(command=cmd,passed=True,source_sha256=sha(path)))
  for name in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
   candidate=Path(name).resolve()
   if candidate in header_start:dependencies.add(candidate)
 tools={Path(module.__file__).resolve()for module in tuple(sys.modules.values())if getattr(module,'__file__',None)}
 tools.update((Path(command[2]),Path(__file__).resolve()))
 used_tools={str(path.relative_to(ROOT)):tool_start[path]for path in tools if path in tool_start}
 used_headers={str(path.relative_to(ROOT)):header_start[path]for path in dependencies}
 for name,pin in(used_tools|used_headers|{UNIT:canonical_sha}).items():
  if sha(ROOT/name)!=pin:raise ValueError('Classifier generation dependency drift: '+name)
 manifest=json.loads((directory/'generation.json').read_text())
 report=dict(passed=True,scratch_owner=str(scratch),original_sha256=NEW_PIN,canonical_sha256=canonical_sha,
             raw_sha256=sha(generated),whole_unit_byte_equal=raw==canonical,
             other_changed_function_bodies=changed_bodies,
             whole_unit_diff=''.join(difflib.unified_diff(canonical.splitlines(True),raw.splitlines(True),fromfile='production',tofile='fresh',n=2))[:20000],
             classifier_body_identical=True,native_prefix_preserved=True,previous_lighting_prefix_preserved=True,
             raw_and_specialized_syntax=syntax,preparation_tools_sha256=used_tools,source_dependencies_sha256=used_headers,
             generation_command=command,generation_manifest_sha256=sha(directory/'generation.json'),
             generated_entries=len(manifest.get('entries',[])),specialization_receipt=receipt,
             scope='Routing and regeneration evidence. Classifier execution has an independent unchanged-i386 ABI proof; other unit bodies remain separately qualified.',retained_large_artifacts=[])
 if raw!=canonical:report['passed']=False
 args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(passed=report['passed'],whole_unit_byte_equal=raw==canonical,tool_inputs=len(used_tools),header_inputs=len(used_headers))))
 if not report['passed']:raise ValueError('Fresh particle unit differs from current production; bounded diagnostic retained')
 return 0
if __name__=='__main__':raise SystemExit(main())

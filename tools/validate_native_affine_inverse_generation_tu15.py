#!/usr/bin/env python3
"""Rebuild all five inverse contexts and compile complete owning units."""
import argparse,contextlib,difflib,hashlib,io,json,os,re,runpy,shlex,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
MOTION='analysis/multiplayer/sandbox-source/vehicle-motion-20261011/correspondence-verified/report.json'
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def invoke(command):
 old=sys.argv
 try:
  sys.argv=command[2:]
  with contextlib.redirect_stdout(io.StringIO()):runpy.run_path(command[2],run_name='__main__')
 finally:sys.argv=old
def main():
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--report',type=Path,required=True);parser.add_argument('--compiler',default='clang++');args=parser.parse_args()
 os.chdir(ROOT);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
 import native_affine_inverse_replacements as binding
 from native_affine_inverse_inputs import UNITS,PUBLIC,SHARED_BODY,unique_function
 from specialize_native_operands import specialize
 from validate_native_camera_instance_bitmap_generation_tu15 import generate_ai
 tools={p.resolve():sha(p)for p in(ROOT/'tools').rglob('*.py')}
 headers={p.resolve():sha(p)for tree in('src','include')for p in(ROOT/tree).rglob('*')if p.suffix in('.hpp','.h','.inc')}
 import native_affine_inverse_animation_replacements as animation
 canonical={unit:sha(ROOT/unit)for unit in(*UNITS,PUBLIC,animation.UNIT)}
 report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],units=[])
 try:
  for name,unit in zip(('ai','map-world','world','motion'),UNITS,strict=True):
   row=dict(owner=name,unit=unit);report['units'].append(row);generated=scratch/(name+'.cpp');current=(ROOT/unit).read_text()
   if name=='ai':command,provenance=generate_ai(scratch);row['original_reference_provenance']=provenance
   else:
    if name=='world':
     frozen=scratch/'world-current.cpp';frozen.write_text(current)
     command=[sys.executable,'-B',str(ROOT/'tools/fix_physics_world_observation_tu15.py'),'--source',str(frozen),'--output',str(generated),'--manifest',str(scratch/(name+'.json'))]
    elif name=='motion':command=[sys.executable,'-B',str(ROOT/'tools/generate_vehicle_motion_20261011.py'),'--correspondence',str(ROOT/MOTION),'--output',str(generated),'--manifest',str(scratch/(name+'.json'))]
    else:command=[sys.executable,'-B',str(ROOT/'tools/generate_map_world_support_tu15.py'),'--output',str(generated),'--manifest',str(scratch/(name+'.json'))]
    invoke(command)
   source=generated.read_text();match,body,mode,_=binding.authenticate(source)
   if name=='motion':
    import native_motion_fst_c1_corrections as correction
    if correction.correct(source)!=source or correction.correct(current)!=current:raise ValueError('Qualified Motion C1 corrections missing')
    row['qualified_motion_c1']=dict(sites=len(correction.SITES),whole_original_baseline=correction.BASELINE,original_reversal_equal=correction.canonical_source(source)==correction.canonical_source(current))
   if binding.prefix(mode)not in match[0]or binding.replace_affine_inverse(source)!=source:raise ValueError('Fresh inverse default/idempotence missing '+name)
   canonical_match,canonical_body,_,_=binding.authenticate(current)
   if binding.canonical_body(canonical_match[0])!=canonical_body:raise ValueError('Canonical inverse body ownership changed')
   prepared,receipt=specialize(source);prepared_path=scratch/(name+'-prepared.cpp');prepared_path.write_text(prepared)
   if binding.prefix(mode)not in binding.BODY.search(prepared)[0]:raise ValueError('Prepared inverse default missing '+name)
   syntax=[];dependencies=set()
   for artifact in(generated,prepared_path):
    dep=artifact.with_suffix('.d');command_syntax=[args.compiler,'-std=c++20','-fsyntax-only','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-MD','-MF',str(dep),str(artifact)]
    done=subprocess.run(command_syntax,capture_output=True,text=True,timeout=300)
    if done.returncode:raise ValueError('Whole '+name+' unit syntax failed '+done.stderr[-6000:])
    dependencies.update(Path(x).resolve()for x in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]));syntax.append(dict(command=command_syntax,passed=True,sha256=sha(artifact)))
   if name in('world','motion')and source!=current:
    delta=''.join(difflib.unified_diff(current.splitlines(True),source.splitlines(True),fromfile='canonical',tofile='generated'))
    if len(delta.encode())<100000:row['bounded_diff']=delta
    raise ValueError('Complete inverse '+name+' generator changed canonical bytes')
   row.update(passed=True,generation_command=command,full_body=sha_text(body),whole_unit_equal=source==current,raw_prepared_standalone_syntax=syntax,header_hashes={str(p.relative_to(ROOT)):headers[p]for p in dependencies if p in headers},actual_owner_groups={k:v for k,v in receipt.items()if k.endswith('actual_owner_entries')})
  unit=animation.UNIT;current=(ROOT/unit).read_text();generated=scratch/'animation.cpp'
  command=[sys.executable,'-B',str(ROOT/'tools/generate_unit_animation_selection_runtime_1_1.py'),'--output',str(generated),'--manifest',str(scratch/'animation.json')]
  invoke(command);source=generated.read_text();match,body,state,public_entry=animation.authenticate(source)
  if animation.PREFIX not in match[0]or animation.replace_affine_inverse_animation(source)!=source:raise ValueError('Fresh fixed-1.1 inverse default missing')
  animation.authenticate(current);dep=generated.with_suffix('.d')
  syntax_command=[args.compiler,'-std=c++20','-fsyntax-only','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-MD','-MF',str(dep),str(generated)]
  done=subprocess.run(syntax_command,capture_output=True,text=True,timeout=300)
  if done.returncode:raise ValueError('Whole fixed-1.1 animation syntax failed '+done.stderr[-6000:])
  dependencies={Path(x).resolve()for x in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1])}
  if source!=current:
   report['animation_bounded_diff']=''.join(difflib.unified_diff(current.splitlines(True),source.splitlines(True),fromfile='canonical',tofile='generated'))[:30000]
   raise ValueError('Complete fixed-1.1 animation generator changed canonical bytes')
  report['units'].append(dict(owner='animation-fixed1.1',unit=unit,passed=True,generation_command=command,whole_unit_equal=True,complete_body=animation.BODY_PIN,complete_state=animation.STATE_PIN,complete_public_entry=animation.PUBLIC_PIN,whole_standalone_syntax=True,header_hashes={str(p.relative_to(ROOT)):headers[p]for p in dependencies if p in headers},operand_preparation='Fixed literal1.1 owner; selected-operand preparation is inapplicable'))
  public=(ROOT/PUBLIC).read_text()
  if unique_function(public,'inverse_frame')!=SHARED_BODY:raise ValueError('Installed shared inverse public body missing')
  done=subprocess.run([args.compiler,'-std=c++20','-fsyntax-only','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),str(ROOT/PUBLIC)],capture_output=True,text=True,timeout=180)
  if done.returncode:raise ValueError('Standalone public inverse CPP syntax failed '+done.stderr[-6000:])
  used={Path(module.__file__).resolve()for module in tuple(sys.modules.values())if getattr(module,'__file__',None)};used.add(Path(__file__).resolve())
  report['preparation_tools_sha256']={str(p.relative_to(ROOT)):tools[p]for p in used if p in tools};report['canonical_sha256']=canonical
  for row in report['units']:
   for name,pin in row['header_hashes'].items():assert sha(ROOT/name)==pin,'Generation header drift '+name
  for name,pin in report['preparation_tools_sha256'].items():assert sha(ROOT/name)==pin,'Generation tool drift '+name
  for name,pin in canonical.items():assert sha(ROOT/name)==pin,'Canonical source drift '+name
  report.update(passed=True,public_standalone_syntax=True,qualification_scope='Routing/regeneration/syntax; numerical and provider proofs are separate')
 except BaseException as error:report['error']=repr(error);raise
 finally:args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
 return 0
def sha_text(text):return hashlib.sha256(text.encode()).hexdigest()
if __name__=='__main__':raise SystemExit(main())

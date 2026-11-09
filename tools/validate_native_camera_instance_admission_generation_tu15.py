#!/usr/bin/env python3
"""Regenerate complete16B810 owners with genuine references and standalone syntax."""
import argparse,contextlib,fcntl,hashlib,importlib.util,io,json,os,re,runpy,shlex,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()

def generate_ai(scratch):
 # Existing retained originals stay in place. Only missing authenticated bank
 # images are reconstructed briefly into this owner's scratch, then removed.
 # Frozen source trees reference the original analysis bank through a symlink;
 # containment/member ownership uses that canonical authenticated root.
 base=(ROOT/'analysis/weapons-tu15-goal-20261013').resolve();store=base/'retained-bank-chunks-v1';helper=base/'bank-chunk-retention-v1.py'
 assert sha(helper)=='4f7d77fc78baa82d3503c6d7ccb47ed933fc5bff2748f46a93e946afc6d64303'
 spec=importlib.util.spec_from_file_location('camera_exact_bank_recovery',helper);decode=importlib.util.module_from_spec(spec);spec.loader.exec_module(decode)
 index=json.loads((store/'index.json').read_text());refs=json.loads((base/'ai-frame-native-functions-v58/generation.json').read_text())['references']
 refs.append(dict(report=str(base/'hunter-public-ranged-script-target-api-original-v1/report.json')));expected={};recovered=[]
 for item in refs:
  manifest=Path(item['report']).parent/'hardware-step-reference.json'
  if manifest.exists():
   step=Path(json.loads(manifest.read_text())['hardware_step_directory']);proof=json.loads((step/'report.json').read_text())
   for name in('hardware-trace.bin','hardware-stores.bin','hardware-exit.bin','hardware-after.bin'):expected[(step/name).resolve()]=proof['files_sha256'][name]
 import validate_ai_hardware_step_reference_tu15 as gate
 original=gate.file_sha;cache={}
 def resolve(path):
  path=Path(path).resolve()
  if path.exists():return original(path)
  if path in cache:return cache[path]
  if path not in expected or path.name!='hardware-after.bin'or not path.is_relative_to(base):return original(path)
  member=str(path.relative_to(base));entry=index['files'][member];assert entry['raw_sha256']==expected[path]
  descriptor=store/entry['manifest'];assert original(descriptor)==entry['manifest_sha256'];manifest=json.loads(descriptor.read_text());assert manifest['member']==member and manifest['raw_sha256']==expected[path]
  output=scratch/'recovered-after.bin'
  try:
   with output.open('wb')as stream:decode.reconstruct(manifest,stream)
   actual=original(output);assert actual==expected[path]and output.stat().st_size==entry['raw_bytes'];cache[path]=actual
   recovered.append(dict(logical_path=str(path),manifest=str(descriptor),manifest_sha256=entry['manifest_sha256'],bytes=output.stat().st_size,sha256=actual));return actual
  finally:output.unlink(missing_ok=True)
 gate.file_sha=resolve
 command=['tools/generate_ai_frame_tu15.py','--output',str(scratch/'ai.cpp'),'--directory',str(scratch/'ai-generation')]+sum((['--reference',str(Path(x['report']).parent)]for x in refs),[])
 argv=sys.argv
 try:
  with(store/'pack.lock').open('rb')as lock:
   fcntl.flock(lock,fcntl.LOCK_SH);sys.argv=command
   with contextlib.redirect_stdout(io.StringIO()):runpy.run_path(str(ROOT/command[0]),run_name='__main__')
 finally:sys.argv=argv;gate.file_sha=original
 return command,dict(reference_count=len(refs),recovered=recovered,originals_modified=False,raw_banks_retained=False)

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--compiler',default='clang++');p.add_argument('--units',nargs='+',choices=('primary','ai','death'),default=('primary','ai','death'));args=p.parse_args()
 os.chdir(ROOT);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,scratch_owner=str(scratch),retained_large_artifacts=[],units=[])
 tools_before={path.resolve():sha(path)for path in(ROOT/'tools').rglob('*.py')};headers_before={path.resolve():sha(path)for tree in('src','include')for path in(ROOT/tree).rglob('*')if path.suffix in('.hpp','.h','.inc')}
 from native_camera_instance_admission_inputs import UNITS
 import native_camera_instance_admission_replacements as binding
 from specialize_native_operands import specialize
 pins={path:sha(ROOT/path)for path in UNITS}
 try:
  for name,unit in zip(('primary','ai','death'),UNITS,strict=True):
   if name not in args.units:continue
   generated=scratch/(name+'.cpp');row=dict(name=name,unit=unit);report['units'].append(row)
   if name=='ai':command,provenance=generate_ai(scratch);row['references']=provenance
   else:
    script='generate_physics_live_support_tu15.py'if name=='primary'else'generate_death_cycle_runtime_tu15.py';flag='--manifest'if name=='primary'else'--directory'
    command=[sys.executable,'-B',str(ROOT/'tools'/script),'--output',str(generated),flag,str(scratch/(name+'-generation'))]
    argv=sys.argv
    try:
     sys.argv=command[2:]
     with contextlib.redirect_stdout(io.StringIO()):runpy.run_path(command[2],run_name='__main__')
    finally:sys.argv=argv
   row['generation_command']=command;source=generated.read_text();canonical=(ROOT/unit).read_text();assert binding.replace_camera_instance_admission(source)==source
   match,original=binding.authenticate(source);assert binding.PREFIX in match[0];canonical_match,canonical_original=binding.authenticate(canonical)
   resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)[0]
   dispatcher=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)[0]
   controls=(source.replace(match[0],match[0].replace('0x98u','0x99u',1),1),
     source.replace(resolver,resolver.replace('return p;','return p+1u;',1),1),
     source.replace(dispatcher,dispatcher.replace('run_16b810(s);','run_16c000(s);',1),1),
     source.replace(dispatcher,dispatcher.replace('{switch(target){','{switch(target){case 0x16b811u:run_16b810(s);return;',1),1))
   # Canonical selected-if contexts use their exact existing source tests;
   # these fresh contexts must be complete literal switches.
   for changed in controls:
    assert changed!=source,'Missing fresh source mutation witness'
    try:binding.replace_camera_instance_admission(changed)
    except ValueError:pass
    else:raise ValueError('Fresh16B810 binder admitted body/resolver/dispatch drift')
   row['fresh_source_drift_rejections']=len(controls)

   assert binding.digest(original)in binding.BODY_PINS and binding.PREFIX in canonical_match[0]
   prepared,receipt=specialize(source);body=binding.BODY.search(prepared)[0];assert binding.PREFIX in body and 's.instruction_literal('in body and binding.digest(body)==binding.PREPARED_OUTPUT_PIN
   specialized=scratch/(name+'-prepared.cpp');specialized.write_text(prepared);deps=set();syntax=[]
   for artifact in(generated,specialized):
    dep=artifact.with_suffix('.d');cmd=[args.compiler,'-std=c++20','-fsyntax-only','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-MD','-MF',str(dep),str(artifact)]
    result=subprocess.run(cmd,capture_output=True,text=True,timeout=240);assert result.returncode==0,result.stderr[-6000:]
    syntax.append(dict(command=cmd,passed=True,sha256=sha(artifact)))
    deps.update(Path(x).resolve()for x in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]))
   row.update(passed=True,native_prefix_and_idempotency=True,original_body=binding.digest(original),prepared_body=binding.digest(body),whole_unit_equal=source==canonical,raw_and_specialized_syntax=syntax,imported_native_owner_groups={k:v for k,v in receipt.items()if k.endswith('actual_owner_entries')},headers_sha256={str(path.relative_to(ROOT)):headers_before[path]for path in deps if path in headers_before})
   if name=='primary':assert receipt['native_bsp_surface_2d_actual_owner_entries']==['2a0230']and receipt['native_bsp_surface_polygon_actual_owner_entries']==['1e6e80']
  public=ROOT/'src/physics/native_tu15_camera_instanced_query.cpp';assert binding.replace_public(public.read_text())==public.read_text()
  result=subprocess.run([args.compiler,'-std=c++20','-fsyntax-only','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),str(public)],capture_output=True,text=True,timeout=180);assert result.returncode==0,result.stderr[-6000:]
  report['public_standalone_cpp_syntax']=True
  used={Path(module.__file__).resolve()for module in tuple(sys.modules.values())if getattr(module,'__file__',None)};used.add(Path(__file__).resolve())
  report['preparation_tools_sha256']={str(path.relative_to(ROOT)):tools_before[path]for path in used if path in tools_before};report['canonical_sha256']=pins
  for row in report['units']:
   for path,pin in row['headers_sha256'].items():assert sha(ROOT/path)==pin,'Generation header drift '+path
  for path,pin in report['preparation_tools_sha256'].items():assert sha(ROOT/path)==pin,'Generation tool drift '+path
  for path,pin in pins.items():assert sha(ROOT/path)==pin,'Canonical unit drift '+path
  report['passed']=True
 except BaseException as error:report['error']=repr(error);raise
 finally:args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
 return 0
if __name__=='__main__':raise SystemExit(main())

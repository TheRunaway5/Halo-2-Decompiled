#!/usr/bin/env python3
"""Freeze independent original/native sound-owner checks in a new directory."""
import argparse, hashlib, json, os, shlex, shutil, subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
OWNED=('include/halo2/sound_object_request.hpp','src/sound_object_request.cpp',
 'tests/sound_object_request_tests.cpp','tools/research_sound_object_request_u5.py',
 'tools/oracle_sound_object_request_1_1.py','tools/export_sound_object_request_fixtures.py',
 'tools/audit_sound_object_request_native.py','docs/MP_SOUND_OBJECT_REQUEST_NATIVE_INTEGRATION.md')
COMPILE=('src/sound_object_request.cpp','src/fp_sound_request.cpp','tests/sound_object_request_tests.cpp')
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def save(p,x):
 raw=(json.dumps(x,sort_keys=True,indent=2)+'\n').encode();p.parent.mkdir(parents=True,exist_ok=True)
 if p.exists():assert p.read_bytes()==raw,('Frozen output differs',str(p))
 else:p.write_bytes(raw)
def command(argv,cwd=ROOT,env=None):
 result=subprocess.run(argv,cwd=cwd,env=env,text=True,capture_output=True)
 row=dict(argv=list(map(str,argv)),cwd=str(cwd),returncode=result.returncode,stdout=result.stdout,stderr=result.stderr)
 assert result.returncode==0,row
 return row
def compare(first,second):
 a={p.relative_to(first):p for p in first.rglob('*')if p.is_file()};b={p.relative_to(second):p for p in second.rglob('*')if p.is_file()}
 assert a.keys()==b.keys(),(first,second,'File set differs')
 for p in a:assert a[p].read_bytes()==b[p].read_bytes(),('Independent source repeat differs',p)
 return len(a)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--directory',type=Path,required=True);a=p.parse_args();out=a.directory.resolve()
 prior=ROOT/'analysis/multiplayer/integration/fp-sound-request-native/final-manifest.json'
 before=json.loads(prior.read_text())['files']
 for path,h in before.items():assert sha(ROOT/path)==h,path
 original=compare(out/'measured-canonical',out/'repeat');static=compare(out/'u5',out/'u5-repeat')
 r=json.loads((out/'measured-canonical/report.json').read_text());mapping=json.loads((out/'u5/report.json').read_text())
 for c in r['cases']:
  assert c['roots']==c['roots_after'] and c['shared_local_rng_before_hex']==c['shared_local_rng_after_hex']
  for store in c['stores']:assert int(store['instruction'],16)in {int(line.split()[0],16)for file in(out/'measured-canonical/instructions').glob('*.asm')for line in file.read_text().splitlines()}
 fixtures=command([str(ROOT/'.tools/python/bin/python'),'tools/export_sound_object_request_fixtures.py','--report',str(out/'measured-canonical/report.json'),'--providers',str(out/'u5/providers.json'),'--output',str(out/'repeat-fixtures.tsv')])
 assert (out/'fixtures.tsv').read_bytes()==(out/'repeat-fixtures.tsv').read_bytes()
 # Snapshot only actual compiler dependencies. No unrelated current library,
 # release, renderer, shader or asset archive is rebuilt or presumed fresh.
 dependency=command(['g++','-std=c++20','-MM','-Iinclude',*COMPILE])
 tokens=shlex.split(dependency['stdout'].replace('\\\n',' '))
 paths={token for token in tokens if not token.endswith(':')};paths.update(OWNED)
 snapshots={path:sha(ROOT/path)for path in paths}
 source=out/'native/source'
 for path,h in snapshots.items():
  destination=source/path;destination.parent.mkdir(parents=True,exist_ok=True)
  assert not destination.exists();shutil.copy2(ROOT/path,destination);assert sha(destination)==h
 for path,h in snapshots.items():assert sha(ROOT/path)==h,('Shared compilation dependency drift',path)
 runs=[]
 for name,compiler,flags in(('normal','g++',['-O2']),('clang-asan','clang++',['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer'])):
  directory=out/'native'/name;directory.mkdir(parents=True,exist_ok=True);binary=directory/'sound_object_request_tests'
  compile_row=command([compiler,'-std=c++20','-ffp-contract=off','-fno-fast-math','-Wall','-Wextra','-Wpedantic',*flags,'-I'+str(source/'include'),*[str(source/path)for path in COMPILE],'-o',str(binary)])
  save(directory/'compile.json',compile_row)
  env=os.environ.copy();env.update(ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
  run=command([str(binary),str(out/'fixtures.tsv')],env=env);run['sanitizer_environment']={k:env[k]for k in('ASAN_OPTIONS','UBSAN_OPTIONS')};run['binary_sha256']=sha(binary)
  save(directory/'run.json',run);runs.append(dict(name=name,returncode=run['returncode'],stdout=run['stdout'],binary_sha256=sha(binary)))
 for path,h in before.items():assert sha(ROOT/path)==h,path
 audit=dict(schema='source-sound-object-request-native-audit-v1',prior_manifest_sha256=sha(prior),prior_pins_verified=len(before),
  independent_original_files=original,independent_static_files=static,paired_complete_bodies=len(mapping['owners']),
  rows_per_image=sum(q['rows']for q in mapping['owners']),actual_cases=len(r['cases']),
  accepted=sum(q['result_al']!=0 for q in r['cases']),ordered_stores=sum(len(q['stores'])for q in r['cases']),compression_controls=len(r['compression_controls']),
  fixture_reexport=fixtures,compiler_dependencies=dependency,snapshots=snapshots,native_runs=runs,
  explicit_limit='All original owning executions are1.1, static complete bodies separately match authenticated1.5. Actual fixtures have physicalNONE, unit activity predicate false, ordinary weapon header; physical/PVS-remap branches are byte-derived complete implementations without positive execution here. No native runtime or submit/scheduler/voice ownership is claimed.')
 save(out/'audit.json',audit)
 pins={str(path.relative_to(ROOT)):sha(path)for path in out.rglob('*')if path.is_file()}
 pins.update({path:sha(ROOT/path)for path in OWNED})
 save(out/'checkpoint-manifest.json',dict(schema='source-sound-object-request-native-freeze-v1',files=pins,audit_sha256=sha(out/'audit.json'),prior_manifest_sha256=sha(prior),native_runtime_connected=False))
 print(json.dumps(dict(pins=len(pins),audit_sha256=sha(out/'audit.json'),manifest_sha256=sha(out/'checkpoint-manifest.json'),native_runs=runs)))
if __name__=='__main__':main()

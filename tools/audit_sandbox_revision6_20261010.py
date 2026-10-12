#!/usr/bin/env python3
"""Read-only hash and completed-check audit for the local revision6 delivery."""
import argparse,hashlib,json
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]

def digest(path):
 h=hashlib.sha256()
 with path.open('rb')as file:
  for part in iter(lambda:file.read(1024*1024),b''):h.update(part)
 return h.hexdigest()

def audit(root,manifest):
 data=json.loads(manifest.read_text());errors=[];checked={}
 def check(label,base,entries):
  checked[label]=len(entries)
  for name,pin in entries.items():
   path=base/name
   if not path.is_file()or digest(path)!=pin:errors.append(f'{label}: {name}')
 delivery=manifest.parent;bank=delivery.parent
 check('runtime_files',delivery,data['runtime_files'])
 check('build_shader_files',root,data['required_build_shader_files'])
 check('evidence_files',root,data['evidence'])
 check('external_input_files',root,data['external_inputs'])
 source_manifest=bank/'source-manifest.json'
 if digest(source_manifest)!=data['source_manifest_sha256']:errors.append('source manifest')
 sources=json.loads(source_manifest.read_text())
 check('source_files',bank/'source',sources['files'])
 for label in ('cpu_checks','gpu_checks','asan_checks','muzzle_checks','audio_checks','launcher_checks'):
  rows=data[label];checked[label]=len(rows)
  for record in rows:
   if record['exit_code']!=0:errors.append(f'failed {label}: {record["name"]}')
   binary=Path(record['binary'])
   if not binary.is_file()or digest(binary)!=record['binary_sha256']:errors.append(f'changed {label}: {record["name"]}')
   if str(record['log'])not in data['evidence']:errors.append(f'unpinned log: {record["name"]}')
 launcher=Path(data['launcher']);target=Path(data['launcher_target'])
 if not launcher.is_symlink()or launcher.resolve()!=target.resolve():errors.append('installed sandbox launcher target')
 checked['mismatches']=errors;checked['manifest_sha256']=digest(manifest)
 return checked

def main():
 parser=argparse.ArgumentParser(description=__doc__)
 parser.add_argument('--root',type=Path,default=ROOT)
 parser.add_argument('--manifest',type=Path)
 args=parser.parse_args();root=args.root.resolve()
 manifest=args.manifest or root/'analysis/multiplayer/integration/sandbox-release-r6-20261010/delivery/manifest.json'
 result=audit(root,manifest);print(json.dumps(result,indent=2,sort_keys=True))
 raise SystemExit(bool(result['mismatches']))
if __name__=='__main__':main()

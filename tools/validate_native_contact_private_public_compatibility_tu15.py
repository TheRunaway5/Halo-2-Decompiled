#!/usr/bin/env python3
"""Re-run an existing public component proof against an owned source overlay.

The small source tree borrows unchanged sources/assets by symlink and copies
the selected immutable overlay plus includes. No canonical file is modified.
The child proof's evidence domain remains unchanged; this compatibility run
does not turn its original CPU children into a native-only production proof.
"""
import argparse,hashlib,importlib,json,os,shutil,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--overlay',type=Path,required=True);p.add_argument('--module',required=True);p.add_argument('--receipt',type=Path,required=True);a,arguments=p.parse_known_args();scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();source=scratch/'source';source.mkdir();include=source/'include';shutil.copytree(ROOT/'include',include);(source/'src/physics').mkdir(parents=True)
 for path in(ROOT/'src/physics').iterdir():
  (source/'src/physics'/path.name).symlink_to(path,target_is_directory=path.is_dir())
 for name in ('tools','tests','assets','analysis','docs','.tools'):(source/name).symlink_to(ROOT/name,target_is_directory=True)
 manifest=json.loads((a.overlay/'source-pins.json').read_text());pins={}
 for rel,proof in manifest['files'].items():
  path=a.overlay/'staged'/rel;assert sha(path)==proof['after'];target=source/rel;target.parent.mkdir(parents=True,exist_ok=True)
  if target.is_symlink():target.unlink()
  shutil.copyfile(path,target);pins[rel]=sha(path)
 module=importlib.import_module(a.module)
 # Imported validators use their common ROOT module variable. Every path
 # remains a real owned source path; file I/O and compiler tools are ordinary.
 canonical_root=ROOT
 def redirect(value):
  if isinstance(value,Path)and value.is_relative_to(canonical_root):return source/value.relative_to(canonical_root)
  if isinstance(value,str)and value.startswith(str(canonical_root)+'/'):return str(source/Path(value).relative_to(canonical_root))
  if isinstance(value,tuple):return tuple(redirect(x)for x in value)
  if isinstance(value,list):return[redirect(x)for x in value]
  return value
 for item in list(sys.modules.values()):
  if getattr(item,'ROOT',None)==canonical_root:item.ROOT=source
  if hasattr(item,'FILES'):item.FILES=redirect(item.FILES)
  filename=getattr(item,'__file__',None)
  if filename and Path(filename).is_relative_to(canonical_root/'tools'):item.__file__=str(source/'tools'/Path(filename).name)
 assert module.ROOT==source,'Public proof did not select the owned source overlay'
 sys.argv=[str(source/'tools'/f'{a.module}.py'),*arguments];result=None
 try:result=module.main();return result
 finally:
  a.receipt.parent.mkdir(parents=True,exist_ok=True);a.receipt.write_text(json.dumps(dict(scope=__doc__,module=a.module,arguments=arguments,result=result,source_root=str(source),owned_scratch=str(scratch),overlay_manifest_sha256=sha(a.overlay/'source-pins.json'),overlay_source_hashes=pins,canonical_source_writes=False,retained_large_artifacts=[],temporary_allocated_bytes_before_cleanup=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file()and not p.is_symlink())),indent=2)+'\n')
if __name__=='__main__':raise SystemExit(main())

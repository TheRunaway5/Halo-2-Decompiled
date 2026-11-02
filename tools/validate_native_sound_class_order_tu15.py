#!/usr/bin/env python3
"""Replay a pinned physical TU1.5 reference through the real sound registry."""
import argparse,gzip,hashlib,json,os,subprocess
from pathlib import Path
FIXTURE_SHA256='60791a30db855db59ca33542b3f9f5a32f2cbace7d88d917cf3e2b351fab058e'
XBE_SHA256='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
def main():
 parser=argparse.ArgumentParser(description=__doc__)
 parser.add_argument('--native',type=Path,required=True);parser.add_argument('--fixture',type=Path,required=True)
 args=parser.parse_args();scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
 packed=args.fixture.read_bytes()
 if hashlib.sha256(packed).hexdigest()!=FIXTURE_SHA256:raise ValueError('Original sound-class fixture changed')
 raw=gzip.decompress(packed)
 if len(raw)>2*1024*1024:raise ValueError('Original sound-class fixture exceeded its bounded size')
 reference=json.loads(raw)
 if reference['xbe_sha256']!=XBE_SHA256 or reference['instruction_patches'] or reference['supplied_original_outcomes']:
  raise ValueError('Original sound-class provenance changed')
 if len(reference['cases'])!=108 or len(reference['metadata']['sites'])!=17:
  raise ValueError('Original sound-class coverage changed')
 destination=scratch/'sound-class-original.json';destination.write_bytes(raw)
 result=subprocess.run([str(args.native.resolve(strict=True)),str(destination)],check=False,timeout=80)
 return result.returncode
if __name__=='__main__':raise SystemExit(main())

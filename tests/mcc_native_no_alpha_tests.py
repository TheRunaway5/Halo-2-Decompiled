"""Run the bounded complete NoAlpha fixture under CTest's owned lifecycle."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import zlib

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--repo-root',type=Path,required=True);p.add_argument('--test-executable',type=Path,required=True);p.add_argument('--native-root');p.add_argument('--evidence');a=p.parse_args()
    directory=a.repo_root/'tests/fixtures';metadata=json.loads((directory/'mcc-native-no-alpha-v1.json').read_bytes())
    if metadata['schema']!='H2_MCC_NATIVE_NO_ALPHA_REGRESSION_V1' or metadata['source_policy_sha256']!=hashlib.sha256((a.repo_root/'tools/data/mcc-native-no-alpha.json').read_bytes()).hexdigest():raise ValueError('NoAlpha source policy changed')
    name=metadata['compressed_file']
    if Path(name).name!=name or not 0<metadata['compressed_bytes']<=128*1024 or not 0<metadata['decoded_bytes']<=2*1024*1024:raise ValueError('Invalid bounded NoAlpha fixture extent')
    packed=(directory/name).read_bytes()
    if len(packed)!=metadata['compressed_bytes'] or hashlib.sha256(packed).hexdigest()!=metadata['compressed_sha256']:raise ValueError('Packed NoAlpha source fixture changed')
    decoder=zlib.decompressobj();raw=decoder.decompress(packed,metadata['decoded_bytes']+1)
    if len(raw)!=metadata['decoded_bytes'] or not decoder.eof or decoder.unused_data or decoder.unconsumed_tail or hashlib.sha256(raw).hexdigest()!=metadata['decoded_sha256']:raise ValueError('Complete NoAlpha source fixture changed')
    fixture=Path(os.environ['HALO2_TEST_SCRATCH'])/'no-alpha.json';fixture.write_bytes(raw)
    return subprocess.run([str(a.test_executable.resolve(strict=True)),str(fixture)],check=False).returncode
if __name__=='__main__':raise SystemExit(main())

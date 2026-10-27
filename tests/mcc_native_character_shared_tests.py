"""Run the bounded literal two-profile fixture under the owned CTest lifecycle."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import zlib

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo-root',type=Path,required=True)
    parser.add_argument('--test-executable',type=Path,required=True)
    parser.add_argument('--native-root')
    parser.add_argument('--evidence')
    args=parser.parse_args()
    directory=args.repo_root/'tests/fixtures'
    metadata=json.loads((directory/'mcc-native-character-shared-v1.json').read_bytes())
    if metadata['schema']!='H2_MCC_NATIVE_CHARACTER_SHARED_REGRESSION_V1' or metadata['source_policy_sha256']!=hashlib.sha256((args.repo_root/'tools/data/mcc-native-character-shared.json').read_bytes()).hexdigest():
        raise ValueError('Character fixture source policy changed')
    name=metadata['compressed_file']
    if Path(name).name!=name or not 0<metadata['compressed_bytes']<=128*1024 or not 0<metadata['decoded_bytes']<=2*1024*1024:
        raise ValueError('Invalid bounded character fixture extent')
    packed=(directory/name).read_bytes()
    if len(packed)!=metadata['compressed_bytes'] or hashlib.sha256(packed).hexdigest()!=metadata['compressed_sha256']:
        raise ValueError('Complete packed character fixture changed')
    decoder=zlib.decompressobj();raw=decoder.decompress(packed,metadata['decoded_bytes']+1)
    if len(raw)!=metadata['decoded_bytes'] or not decoder.eof or decoder.unused_data or decoder.unconsumed_tail or hashlib.sha256(raw).hexdigest()!=metadata['decoded_sha256']:
        raise ValueError('Complete decoded character fixture changed')
    owner=Path(os.environ['HALO2_TEST_SCRATCH']);fixture=owner/'character-shared.json';fixture.write_bytes(raw)
    return subprocess.run([str(args.test_executable.resolve(strict=True)),str(fixture)],check=False).returncode

if __name__=='__main__':
    raise SystemExit(main())

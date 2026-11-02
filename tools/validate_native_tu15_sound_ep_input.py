#!/usr/bin/env python3
"""Compose actual native GP resources, EP input windows and stereo output."""
import argparse,gzip,hashlib,json,os,struct,subprocess
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
for name in ('probe','vectors','reference','report'):p.add_argument('--'+name,type=Path,required=True)
a=p.parse_args();Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);compressed=a.vectors.read_bytes();raw=gzip.decompress(compressed)
ref=json.loads(a.reference.read_text());assert hashlib.sha256(compressed).hexdigest()==ref['GP_output_fixture_sha256']and hashlib.sha256(raw).hexdigest()==ref['GP_output_fixture_raw_sha256']
assert struct.unpack_from('<2I',raw)==(0x47504f31,36)and len(raw)==8+36*(1+6+3712+1024)*4
r=subprocess.run([str(a.probe.resolve()),str(a.reference.resolve())],input=raw,capture_output=True,timeout=60);assert r.returncode==0 and not r.stderr,(r.returncode,r.stderr.decode())
result=json.loads(r.stdout);assert result['passed']and result['original_EP_returns']==66 and result['original_input_reads']==330
result.update(reference_sha256=hashlib.sha256(a.reference.read_bytes()).hexdigest(),GP_fixture_sha256=hashlib.sha256(compressed).hexdigest(),probe_sha256=hashlib.sha256(a.probe.read_bytes()).hexdigest(),runtime_original_program_loaded=False,physical_clock_or_audible_parity_verified=False,retained_large_artifacts=[])
a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))

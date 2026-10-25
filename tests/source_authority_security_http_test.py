"""Denied-journal checkpoint repair plus real typed peer replay acceptance."""
import os
from pathlib import Path
import subprocess
import sys

root=Path(__file__).resolve().parents[1]
scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
for index, binary in enumerate(sys.argv[1:]):
    env=dict(os.environ,HALO2_PUBLISHER_TEST_OUTPUT=str(scratch/f'authority-security-{index}'))
    if index==0:env['HALO2_PUBLISHER_SECURITY_REPAIR']='1'
    else:env.pop('HALO2_PUBLISHER_SECURITY_REPAIR',None)
    result=subprocess.run([sys.executable,str(root/'tests/source_authority_publisher_http_test.py'),binary],
        cwd=root,env=env,timeout=315)
    if result.returncode:raise SystemExit(result.returncode)

"""Exercise the actual Rust bootstrap from a relocated native executable."""
import os
from pathlib import Path
import shutil
import subprocess
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from package_eos_client import package

root = Path(os.environ['HALO2_TEST_SCRATCH'])
binary = root / Path(sys.argv[1]).name
shutil.copy2(sys.argv[1], binary)
# The copied executable still uses its build-time RPATH for the official SDK.
env = {k: v for k, v in os.environ.items() if not k.startswith(('H2_EOS_', 'H2_ONLINE_TRANSPORT'))}
def check(expected, **overrides):
    result = subprocess.run([str(binary)], env=env | overrides, capture_output=True, text=True)
    assert result.returncode == (1 if expected is None else 0), result.stderr
    if expected is not None:
        assert 'EOS_BOOTSTRAP ' + expected in result.stdout.splitlines(), result.stdout
check('0 0')
source = root / 'private-input.env'
source.write_text(''.join(k + '=' + v + '\n' for k,v in {
    'H2_EOS_PRODUCT_ID': 'a'*32, 'H2_EOS_SANDBOX_ID': 'b'*32,
    'H2_EOS_DEPLOYMENT_ID': 'c'*32, 'H2_EOS_CLIENT_ID': 'untrusted-client',
    'H2_EOS_CLIENT_SECRET': 'shared-client-value', 'H2_EOS_CACHE_DIR': '/build-machine-only',
}.items()))
sidecar = root / 'eos-client.env'
package(source, sidecar)
assert 'CACHE_DIR' not in sidecar.read_text()
assert sidecar.stat().st_mode & 0o777 == 0o600
check('1 1')
check('0 0', H2_ONLINE_TRANSPORT='direct', H2_EOS_PROFILES='0')
check('1 1', H2_EOS_CONFIG=str(source), H2_ONLINE_TRANSPORT='eos', H2_EOS_PROFILES='1')
check(None, H2_EOS_CONFIG=str(root / 'missing.env'))
sidecar.write_text('H2_EOS_SERVER_CLIENT_SECRET=must-never-ship\n')
check(None)
try:
    package(sidecar, root / 'rejected.env')
except ValueError:
    pass
else:
    raise AssertionError('Trusted server setting entered a game release')
print('PASS relocated Rust bootstrap, packaged defaults, environment overrides and trusted-config rejection')

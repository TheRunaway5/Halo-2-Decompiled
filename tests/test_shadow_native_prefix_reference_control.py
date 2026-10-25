#!/usr/bin/env python3
"""The unchanged policy must pass numeric checks and fail native-work admission."""
import os
import subprocess
import sys

environment = dict(os.environ)
environment[sys.argv[3] if len(sys.argv)>3 else 'HALO2_REFERENCE_SHADOW_NATIVE_PREFIXES'] = '1'
result = subprocess.run([sys.argv[1], sys.argv[2], '--quick', '--expect-reference', '--require-native'],
                        env=environment, capture_output=True, text=True, timeout=30)
if result.returncode != 1 or 'NUMERICAL_PASS ' not in result.stdout or 'NATIVE_REQUIRED:' not in result.stderr:
    raise SystemExit(f'Wrong red control: exit={result.returncode}\n{result.stdout}\n{result.stderr}')
print('PASS: reference numerical output unchanged; actual native-work gate is red')

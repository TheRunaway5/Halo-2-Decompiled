#!/usr/bin/env python3
"""Verify engine resource placement and execution in an owned bundle fixture."""
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys


def main():
    app = Path(sys.argv[1]).resolve(strict=True)
    contents = app / 'Contents'
    assert not (contents / 'MacOS/data').exists(), 'Tag data was staged in the executable directory'
    assert (contents / 'Resources/data/tags/variants/resource-fixture.json').read_text() == '{"source_tag_probe":17}\n'
    assert (contents / 'Resources/shaders/staged-marker.txt').read_text() == 'Resource copy control.\n'
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    relocated = scratch / 'Relocated engine Ω.app'
    shutil.copytree(app, relocated, symlinks=True)
    for bundle in (app, relocated):
        subprocess.run(['codesign', '--verify', '--deep', '--strict', str(bundle)], check=True)
        for arch in ('arm64', 'x86_64'):
            subprocess.run(['/usr/bin/arch', '-' + arch, str(bundle / 'Contents/MacOS/Halo 2')],
                           cwd=scratch, check=True)
    report = {'executed_architectures': ['arm64', 'x86_64'], 'executions': 4,
              'strict_signature_verified': True, 'legacy_code_directory_data': False,
              'relocated_resource_lookup': True, 'scope': 'Actual CMake engine resource staging and signing; synthetic executable/dependency, no gameplay or GPU acceptance.'}
    (app.parent / 'engine-resources.json').write_text(json.dumps(report, indent=2)+'\n')
    print('PASS engine tag/shader resources, relocation and strict signatures on both architectures')


if __name__ == '__main__':
    main()

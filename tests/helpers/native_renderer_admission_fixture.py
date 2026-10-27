"""Controlled verifier exit status for compiled launcher routing fixtures.

The real verifier and SPIR-V packages have separate admission tests. These
recorders only test whether the launcher calls and obeys that verifier; importing
the copied module still exposes the actual legacy capability reader functions.
"""
from pathlib import Path


def install_renderer_admission_fixture(repository, destination):
    source = Path(repository) / 'tools/native_renderer_shaders.py'
    prefix = '''import sys as _fixture_sys
if __name__ == '__main__' and '--verify-aot-package' in _fixture_sys.argv:
    import json as _fixture_json
    from pathlib import Path as _FixturePath
    _fixture_root = _FixturePath(__file__).resolve().parents[1]
    (_fixture_root/'renderer-admission-call.json').write_text(_fixture_json.dumps(_fixture_sys.argv[1:]))
    raise SystemExit(1 if (_fixture_root/'reject-renderer-admission').exists() else 0)
'''
    Path(destination).write_text(prefix + source.read_text())

"""Actual vanilla prompt install plus the C++ mixed-view HUD consumer."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
import controller_prompt_assets

with tempfile.TemporaryDirectory(prefix='vanilla-prompts-', dir=os.environ.get('HALO2_TEST_SCRATCH')) as scratch:
    cache = Path(scratch)
    controller_prompt_assets.prepare(ROOT, cache)
    result = subprocess.run([sys.argv[1], sys.argv[2], str(cache)], check=False)
raise SystemExit(result.returncode)

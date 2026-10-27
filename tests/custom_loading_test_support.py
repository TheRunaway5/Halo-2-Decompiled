"""Small declared loading art sources; no claim of rendered DDS validity."""
import hashlib
import json
from pathlib import Path
import shutil


def custom_loading_fixture(root):
    from mcc_loading_assets import CUSTOM
    source = Path(root) / CUSTOM
    source.mkdir(parents=True, exist_ok=True)
    files = {}
    for frame in (3, 4):
        for suffix, origin in (('svg', 'authored-vector'), ('dds', 'rasterized-authored-vector')):
            name = f'loading-{frame:02d}.{suffix}'
            data = ('synthetic authored loading source ' + name).encode()
            (source / name).write_bytes(data)
            files[name] = dict(bytes=len(data), sha256=hashlib.sha256(data).hexdigest(), origin=origin)
    (source / 'profile.json').write_text(json.dumps(dict(schema='H2_AUTHORED_LOADING_ASSETS_V1',
        files=files, runtime_admitted=False, state='authored-needs-review')))
    return source


def permitted_source_policy_fixture(root):
    """Copy only authenticated policy metadata; never discover/extract media."""
    from h2ek_loading_ambience import POLICY
    target = Path(root) / POLICY
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(Path(__file__).resolve().parents[1] / POLICY, target)
    return target

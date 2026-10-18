"""Read exact original GPU observations, including losslessly compacted JSON."""
import gzip
import hashlib
import json
from pathlib import Path


def load_json(path, pins=None):
    path = Path(path).absolute()
    actual = path if path.is_file() else path.with_name(path.name + '.gz')
    raw = actual.read_bytes()
    if pins is not None:
        pins[str(actual)] = hashlib.sha256(raw).hexdigest()
    if actual != path:
        raw = gzip.decompress(raw)
        manifest = actual.parent / 'compression.json'
        record = json.loads(manifest.read_text())['files'][path.name]
        assert record['compressed_file'] == actual.name
        assert record['raw_sha256'] == hashlib.sha256(raw).hexdigest()
        assert record['compressed_sha256'] == hashlib.sha256(actual.read_bytes()).hexdigest()
        if pins is not None:
            pins[str(manifest)] = hashlib.sha256(manifest.read_bytes()).hexdigest()
    return json.loads(raw)

"""Content hashes reused only while the same file identity/stat stays unchanged."""
import hashlib
from pathlib import Path

class StatCheckedSha256:
    def __init__(self):
        self.cache = {}

    @staticmethod
    def signature(info):
        return (info.st_dev, info.st_ino, info.st_size, info.st_mtime_ns, info.st_ctime_ns)

    def __call__(self, path):
        path = Path(path).resolve(strict=True)
        before = self.signature(path.stat())
        cached = self.cache.get(path)
        if cached is not None and cached[0] == before:
            return cached[1]
        digest = hashlib.sha256()
        with path.open('rb') as stream:
            for block in iter(lambda: stream.read(1024 * 1024), b''):
                digest.update(block)
        if self.signature(path.stat()) != before:
            raise RuntimeError(f'Original evidence file changed during hashing: {path}')
        value = digest.hexdigest()
        self.cache[path] = (before, value)
        return value

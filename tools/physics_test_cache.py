"""Prepared MCC input admission and bounded physics-test result retention."""
from pathlib import Path
import hashlib
import json
import lzma
import shutil

from weapon_test_cache import mcc_runtime_inputs, require_mcc_map


def runtime_cache(root, maps=None):
    directory, receipt = mcc_runtime_inputs(root)
    directory = directory.resolve(strict=True)
    if maps is not None and Path(maps).resolve(strict=True) != directory:
        raise ValueError("Physics runtime maps must belong to the selected prepared MCC cache")
    return directory, receipt


def runtime_map(maps, selected):
    path = require_mcc_map(selected)
    if path.parent != Path(maps).resolve(strict=True):
        raise ValueError("Physics runtime map is outside the selected prepared MCC cache")
    return path


def retain_results(source, destination, *, video=False):
    """Copy bounded reports; raw banks/build products stay in owned scratch."""
    source, destination = Path(source).resolve(), Path(destination).resolve()
    if destination.is_relative_to(source) or source.is_relative_to(destination):
        raise ValueError("Retained results must be separate from owned test scratch")
    destination.mkdir(parents=True, exist_ok=False)
    retained, omitted = [], []
    report_bytes = 0
    for path in sorted(source.rglob('*')):
        if not path.is_file() or path.is_symlink():
            continue
        # Do not follow a symlink in any parent of the selected file.
        if any(parent.is_symlink() for parent in path.parents if parent != source and source in parent.parents):
            continue
        size = path.stat().st_size
        small = path.suffix in ('.json', '.jsonl', '.log', '.txt') and size <= 1024 * 1024
        media = video and path.name in ('window.mp4', 'video-sample.png')
        # Long live input/body traces can cross the individual report limit.
        # Keep their exact bytes compressed within the same bounded report
        # budget, rather than losing the diagnostic at that size boundary.
        if (path.name in ('ticks.jsonl', 'player-body-frames.json',
                         'authority-phantom-phase.jsonl', 'authority-velocity-stores.jsonl',
                         'authority-machine-state.jsonl') and
                1024 * 1024 < size <= 8 * 1024 * 1024):
            original = path.read_bytes()
            packed = lzma.compress(original)
            if len(packed) <= 1024 * 1024 and report_bytes + len(packed) <= 8 * 1024 * 1024:
                target = destination / (str(path.relative_to(source)) + '.xz')
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes(packed)
                if lzma.decompress(target.read_bytes()) != original:
                    raise RuntimeError("Compressed native trace changed during retention")
                retained.append(dict(path=str(target), bytes=len(packed),
                    sha256=hashlib.sha256(packed).hexdigest(), compression='xz',
                    original_bytes=size, original_sha256=hashlib.sha256(original).hexdigest()))
                report_bytes += len(packed)
                continue
        if not media and (not small or report_bytes + size > 8 * 1024 * 1024):
            omitted.append(dict(path=str(path.relative_to(source)), bytes=size))
            continue
        target = destination / path.relative_to(source)
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(path, target)
        with path.open('rb') as stream:
            before = hashlib.file_digest(stream, 'sha256').hexdigest()
        with target.open('rb') as stream:
            after = hashlib.file_digest(stream, 'sha256').hexdigest()
        if before != after:
            raise RuntimeError("Retained physics result changed during copying")
        retained.append(dict(path=str(target), bytes=size, sha256=after))
        if not media:
            report_bytes += size
    receipt = dict(retained=retained, omitted_temporary_outputs=omitted,
                   retained_bytes=sum(row['bytes'] for row in retained),
                   video_retention_requested=bool(video),
                   scratch_removed_by='tools/run_test.py after the owning process exits')
    (destination / 'retention.json').write_text(json.dumps(receipt, indent=2) + '\n')
    return receipt

"""Locate implementation inputs for tools that identify modules by filename.

Build definitions should use explicit subsystem paths. Native generators also
exchange module basenames, so their discovery must handle the categorized tree
and reject ambiguous owners.
"""
from pathlib import Path


def source_file(root: Path, module: str) -> Path:
    directory = Path(root) / 'src'
    relative = Path(module)
    if relative.is_absolute() or '..' in relative.parts:
        raise ValueError(f'Expected an implementation module below src/: {module}')
    if len(relative.parts) > 1:
        result = directory / relative
        if result.is_file():
            return result
        raise FileNotFoundError(result)
    matches = sorted(path for path in directory.rglob(module) if path.is_file())
    if not matches:
        raise FileNotFoundError(f'No implementation module {module} below {directory}')
    if len(matches) != 1:
        raise ValueError(f'Ambiguous implementation module {module}: {matches}')
    return matches[0]

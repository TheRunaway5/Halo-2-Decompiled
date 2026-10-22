"""Select a locally validated menu/runtime update while preserving reference bundles."""
import hashlib
import json
from pathlib import PurePosixPath


def selected_runtime(root, reference_manifest):
    selection = root / '.local/pstencil-runtime-selection.json'
    if not selection.is_file():
        return reference_manifest
    chosen = json.loads(selection.read_text())
    if chosen.get('schema') != 'H2_LOCAL_PSTENCIL_RUNTIME_SELECTION_V1':
        raise SystemExit('Unsupported local pStencil runtime selection')
    for key in ('runtime', 'shader_directory', 'qualification'):
        path = PurePosixPath(chosen[key])
        if path.is_absolute() or '..' in path.parts:
            raise SystemExit('Local pStencil runtime escaped its project owner')
    proof = root / chosen['qualification']
    if hashlib.sha256(proof.read_bytes()).hexdigest() != chosen['qualification_sha256']:
        raise SystemExit('Local pStencil menu qualification changed')
    qualification = json.loads(proof.read_text())
    if not qualification.get('passed') or qualification.get('runtime_sha256') != chosen['runtime_sha256']:
        raise SystemExit('Local pStencil runtime has no passed menu qualification')
    for name,digest in chosen.get('upscaler_binaries',{}).items():
        path=PurePosixPath(name)
        if path.is_absolute() or '..' in path.parts or not name.startswith(str(PurePosixPath(chosen['runtime']).parent / 'upscalers')+'/'):
            raise SystemExit('Local upscaler escaped its runtime owner')
        if hashlib.sha256((root/name).read_bytes()).hexdigest()!=digest:
            raise SystemExit('Local upscaler binary changed')
    # Reference executable, maps and UI assets keep their independently pinned owners.
    result=reference_manifest | {key: chosen[key] for key in
        ('runtime', 'runtime_sha256', 'shader_directory', 'shaders', 'verification', 'live_switch', 'fullscreen_modes')}
    return result | {key:chosen[key] for key in ('upscaling','upscaler_binaries') if key in chosen}

"""Export actual post-vertex GPU outputs for previously authenticated weapon draws.

Read-only RenderDoc replay. These are original software through xemu/OpenGL,
not physical Xbox results or native outputs. Run using the disposable runner.
"""
import hashlib
import json
import os
from pathlib import Path
import runpy
import shutil
import traceback
import renderdoc as rd

ROOT = Path('/home/eric/Developer/Halo-2-Decompiled')
OUT = Path(os.environ['WEAPON_GPU_OUT'])
OUT.mkdir(parents=True, exist_ok=False)
HELPER = ROOT / 'analysis/multiplayer-shading/appearance-residual-replay/replay.py'
plain = runpy.run_path(str(HELPER))['plain']
shutil.copyfile(HELPER, OUT / 'helper-source.py')


def sha(path):
    with path.open('rb') as source:
        return hashlib.file_digest(source, 'sha256').hexdigest()


def blob(raw):
    raw = bytes(raw); digest = hashlib.sha256(raw).hexdigest()
    path = OUT / 'blobs' / (digest + '.bin'); path.parent.mkdir(exist_ok=True)
    if not path.exists(): path.write_bytes(raw)
    assert sha(path) == digest
    return dict(file=str(path.relative_to(OUT)), bytes=len(raw), sha256=digest)


status = 1
try:
    capture = Path(os.environ['WEAPON_GPU_RDC'])
    parts = Path(os.environ['WEAPON_GPU_PARTS'])
    geometry_path = Path(os.environ['WEAPON_GPU_GEOMETRY'])
    geometry = json.loads(geometry_path.read_text())
    assert geometry['passed'] and geometry['original_capture_sha256'] == sha(capture)
    cap = rd.OpenCaptureFile()
    assert cap.OpenFile(str(capture), '', None) == rd.ResultCode.Succeeded
    options = rd.ReplayOptions(); options.optimisation = rd.ReplayOptimisationLevel.Fastest
    result, ctrl = cap.OpenCapture(options, None)
    assert result == rd.ResultCode.Succeeded, str(result)
    rows = []
    for owner in geometry['geometry_owners']:
        event = owner['event']
        (OUT / 'progress.txt').write_text(f'Exporting actual original VS output for event{event}\n')
        ctrl.SetFrameEvent(event, True)
        pipe = ctrl.GetPipelineState()
        mesh = ctrl.GetPostVSData(0, 0, rd.MeshDataStage.VSOut)
        assert mesh.vertexResourceId != rd.ResourceId.Null() and mesh.vertexByteStride > 0
        raw = bytes(ctrl.GetBufferData(mesh.vertexResourceId, mesh.vertexByteOffset, mesh.numIndices * mesh.vertexByteStride))
        assert raw and len(raw) % mesh.vertexByteStride == 0
        row = dict(event=event, model=owner['model'], section=owner['section'], part=owner['part'],
                   mesh=plain(mesh), outputs=plain(pipe.GetShaderReflection(rd.ShaderStage.Vertex).outputSignature),
                   viewport=plain(pipe.GetViewport(0)), vertices=blob(raw), exported_vertex_rows=len(raw)//mesh.vertexByteStride)
        if mesh.indexResourceId != rd.ResourceId.Null():
            assert mesh.indexByteStride in (2,4)
            indices = bytes(ctrl.GetBufferData(mesh.indexResourceId, mesh.indexByteOffset, mesh.numIndices * mesh.indexByteStride))
            assert len(indices) == mesh.numIndices * mesh.indexByteStride
            row['indices'] = blob(indices)
        rows.append(row)
    report = dict(passed=True, scope=__doc__, capture_sha256=sha(capture), parts_sha256=sha(parts),
        tool_sha256=sha(Path(os.environ['WEAPON_GPU_TOOL'])), helper_sha256=sha(HELPER),
        geometry_sha256=sha(geometry_path), draws=rows, actual_original_GPU_outputs=True,
        native_outputs_compared=False, physical_xbox_verified=False, full_weapon_parity=False)
    (OUT / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    ctrl.Shutdown(); cap.Shutdown(); status = 0
except Exception:
    (OUT / 'error.txt').write_text(traceback.format_exc())
os._exit(status)

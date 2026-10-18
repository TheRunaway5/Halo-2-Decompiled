"""Run in qrenderdoc: retain candidate original first-person draw inputs.

Exact authored index sequences locate candidates; they alone do not establish
draw ownership, a complete source/GPU boundary or native framebuffer parity.
Readbacks share content hashes. No guest or native runtime is executed here.
"""
import hashlib
import json
import os
from pathlib import Path
import runpy
import shutil
import struct
import traceback
import renderdoc as rd

ROOT = Path('/home/eric/Developer/Halo-2-Decompiled')
OUT = Path(os.environ['WEAPON_GPU_OUT'])
OUT.mkdir(parents=True, exist_ok=False)
HELPER = ROOT / 'analysis/multiplayer-shading/appearance-residual-replay/replay.py'
plain = runpy.run_path(str(HELPER))['plain']
shutil.copy2(HELPER, OUT / 'helper-source.py')


def progress(message):
    with (OUT / 'progress.txt').open('a') as stream:
        stream.write(message + '\n')


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def blob(raw, suffix='.bin'):
    raw = bytes(raw)
    key = hashlib.sha256(raw).hexdigest()
    path = OUT / 'blobs' / (key + suffix)
    path.parent.mkdir(exist_ok=True)
    if not path.exists():
        path.write_bytes(raw)
    assert digest(path) == key
    return dict(file=str(path.relative_to(OUT)), bytes=len(raw), sha256=key)


def flatten(actions):
    for action in actions:
        yield action
        yield from flatten(action.children)


status = 1
try:
    capture = Path(os.environ['WEAPON_GPU_RDC'])
    parts_path = Path(os.environ['WEAPON_GPU_PARTS'])
    parts = json.loads(parts_path.read_text())['parts']
    counts = {p['index_count'] for p in parts}
    progress('opening original capture')
    cap = rd.OpenCaptureFile()
    assert cap.OpenFile(str(capture), '', None) == rd.ResultCode.Succeeded
    options = rd.ReplayOptions()
    options.optimisation = rd.ReplayOptimisationLevel.Fastest
    result, ctrl = cap.OpenCapture(options, None)
    assert result == rd.ResultCode.Succeeded, str(result)
    actions = list(flatten(ctrl.GetRootActions()))
    draws = [a for a in actions if a.flags & rd.ActionFlags.Drawcall]
    (OUT / 'actions.json').write_text(json.dumps([dict(event=a.eventId,
        name=str(a.customName), indices=a.numIndices, instances=a.numInstances,
        index_offset=a.indexOffset, base_vertex=a.baseVertex, flags=str(a.flags))
        for a in actions], indent=2) + '\n')
    buffers = {str(b.resourceId): b for b in ctrl.GetBuffers()}
    textures = {str(t.resourceId): t for t in ctrl.GetTextures()}
    (OUT / 'textures.json').write_text(json.dumps([plain(t) for t in textures.values()], indent=2) + '\n')
    candidates = [a for a in draws if a.numIndices in counts]
    progress(f'{len(actions)} actions, {len(draws)} draws, {len(candidates)} index-count candidates')
    exported = []
    for action in candidates:
        event = action.eventId
        progress(f'exporting original draw {event}')
        ctrl.SetFrameEvent(event, True)
        pipe = ctrl.GetPipelineState()
        index = pipe.GetIBuffer()
        record = dict(event=event, action=plain(action),
            vertex_inputs=plain(pipe.GetVertexInputs()), vertex_buffers=plain(pipe.GetVBuffers()),
            index_buffer=plain(index), viewport=plain(pipe.GetViewport(0)),
            topology=str(pipe.GetPrimitiveTopology()),
            outputs=plain(pipe.GetOutputTargets()), depth=plain(pipe.GetDepthTarget()),
            pipeline=plain(ctrl.GetGLPipelineState()), index_only_candidates=[])
        actual_indices = []
        if index.resourceId != rd.ResourceId.Null() and index.byteStride in (2, 4):
            raw = bytes(ctrl.GetBufferData(index.resourceId,
                index.byteOffset + action.indexOffset * index.byteStride,
                action.numIndices * index.byteStride))
            record['indices'] = blob(raw)
            assert len(raw) == action.numIndices * index.byteStride
            actual_indices = list(struct.unpack('<' + ('H' if index.byteStride == 2 else 'I') * action.numIndices, raw))
            record['index_only_candidates'] = [dict(model=p['model'], name=p['name'],
                section=p['section'], part=p['part'], shader=p['shader'], shader_name=p['shader_name'])
                for p in parts if p['index_u16'] == actual_indices]
        record['stages'] = {}
        for stage in (rd.ShaderStage.Vertex, rd.ShaderStage.Fragment):
            reflection = pipe.GetShaderReflection(stage)
            if reflection is None:
                continue
            shader = pipe.GetShader(stage)
            item = dict(shader=str(shader), source=blob(reflection.rawBytes, '.glsl'),
                        input_signature=plain(reflection.inputSignature), blocks=[])
            for slot, block in enumerate(reflection.constantBlocks):
                binding = pipe.GetConstantBlock(stage, slot, 0).descriptor
                values = ctrl.GetCBufferVariableContents(pipe.GetGraphicsPipelineObject(), shader,
                    stage, pipe.GetShaderEntryPoint(stage), slot, binding.resource,
                    binding.byteOffset, binding.byteSize)
                item['blocks'].append(dict(slot=slot, block=plain(block), values=plain(values)))
            record['stages'][str(stage)] = item
        record['vertex_data'] = []
        if actual_indices and record['index_only_candidates']:
            for slot, bound in enumerate(pipe.GetVBuffers()):
                if bound.resourceId == rd.ResourceId.Null() or not bound.byteStride:
                    continue
                description = buffers[str(bound.resourceId)]
                available = description.length - bound.byteOffset
                size = min(available, (max(actual_indices) + max(0, action.baseVertex) + 1) * bound.byteStride)
                assert 0 < size <= 16 * 1024 * 1024
                record['vertex_data'].append(dict(slot=slot, binding=plain(bound),
                    data=blob(ctrl.GetBufferData(bound.resourceId, bound.byteOffset, size))))
        (OUT / f'event-{event}.json').write_text(json.dumps(record, indent=2) + '\n')
        exported.append(dict(event=event, indices=action.numIndices,
                             index_only_candidates=record['index_only_candidates']))
    report = dict(passed=True, scope=__doc__, capture_sha256=digest(capture),
        parts_sha256=digest(parts_path), tool_sha256=digest(Path(os.environ['WEAPON_GPU_TOOL'])),
        helper_sha256=digest(HELPER), api=plain(ctrl.GetAPIProperties()),
        action_count=len(actions), draw_count=len(draws), present_count=sum(bool(a.flags & rd.ActionFlags.Present) for a in actions),
        candidates=exported, actual_draw_owner_verified=False,
        GPU_source_boundary_verified=False, native_pixels_compared=False, full_weapon_parity=False)
    (OUT / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    progress('original candidate export complete')
    status = 0
except Exception:
    (OUT / 'error.txt').write_text(traceback.format_exc())
os._exit(status)

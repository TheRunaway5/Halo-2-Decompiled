#!/usr/bin/env python3
"""Read-only RenderDoc extraction of original Xbox radar render targets.

Run with qrenderdoc --python and RADAR_CAPTURE / RADAR_CAPTURE_OUT set.
The capture must come from the original game, not a native reconstruction.
"""
import hashlib
import json
import os
from pathlib import Path
import traceback
import renderdoc as rd


def plain(value, depth=0):
    if value is None or isinstance(value, (str, int, float, bool)):
        return value
    if isinstance(value, rd.ResourceId) or depth > 6:
        return str(value)
    if isinstance(value, (bytes, bytearray)):
        return dict(bytes=len(value), sha256=hashlib.sha256(value).hexdigest())
    if isinstance(value, (list, tuple)) or type(value).__name__.startswith('rdcarray'):
        return [plain(v, depth + 1) for v in value]
    result = {}
    for key in dir(value):
        if key.startswith('_') or key in ('this', 'thisown', 'rawBytes'):
            continue
        try:
            v = getattr(value, key)
            if not callable(v):
                result[key] = plain(v, depth + 1)
        except Exception:
            pass
    return result or str(value)


def walk(actions):
    for action in actions:
        yield action
        yield from walk(action.children)


out = Path(os.environ['RADAR_CAPTURE_OUT'])
out.mkdir(parents=True, exist_ok=True)
(out / 'started.txt').write_text('Read-only original capture extraction started\n')
try:
    capture = Path(os.environ['RADAR_CAPTURE'])
    file = rd.OpenCaptureFile()
    assert file.OpenFile(str(capture), '', None) == rd.ResultCode.Succeeded
    result, replay = file.OpenCapture(rd.ReplayOptions(), None)
    assert result == rd.ResultCode.Succeeded, result
    textures = {str(t.resourceId): t for t in replay.GetTextures()}
    rows = []
    actions = [a for a in walk(replay.GetRootActions()) if a.flags & rd.ActionFlags.Drawcall]
    # A synchronized gameplay frame includes the radar immediately before
    # its final HUD. For long captures the caller can select its last frame.
    actions = actions[-int(os.environ.get('RADAR_DRAW_LIMIT', str(len(actions)))):]
    target_events = {u.eventId for t in textures.values()
                     if t.width == 64 and t.height == 64
                     for u in replay.GetUsage(t.resourceId)
                     if u.usage == rd.ResourceUsage.ColorTarget}
    actions = [a for a in actions if a.eventId in target_events]
    if 'RADAR_TARGET_LIMIT' in os.environ:
        actions = actions[:int(os.environ['RADAR_TARGET_LIMIT'])]
    (out / 'target-events.json').write_text(json.dumps([a.eventId for a in actions]))
    for action in actions:
        if not action.flags & rd.ActionFlags.Drawcall:
            continue
        replay.SetFrameEvent(action.eventId, False)
        pipe = replay.GetPipelineState()
        targets = pipe.GetOutputTargets()
        if not targets:
            continue
        target = textures.get(str(targets[0].resource))
        if target is None or target.width != 64 or target.height != 64:
            continue
        event = action.eventId
        row = dict(event=event, count=action.numIndices, target=plain(target),
                   resources=plain(pipe.GetReadOnlyResources(rd.ShaderStage.Fragment)),
                   samplers=plain(pipe.GetSamplers(rd.ShaderStage.Fragment)))
        row['vertex_inputs'] = plain(pipe.GetVertexInputs())
        row['vertex_buffers'] = plain(pipe.GetVBuffers())
        for index, binding in enumerate(pipe.GetVBuffers()):
            if binding.resourceId == rd.ResourceId.Null():
                continue
            data = bytes(replay.GetBufferData(binding.resourceId, binding.byteOffset,
                         min(binding.byteSize, max(4096, action.numIndices * binding.byteStride))))
            (out / f'{event}-vertex-buffer-{index}.bin').write_bytes(data)
        for stage, label in ((rd.ShaderStage.Vertex, 'vertex'),
                             (rd.ShaderStage.Fragment, 'fragment')):
            reflection = pipe.GetShaderReflection(stage)
            row[label] = plain(reflection)
            if reflection is not None:
                (out / f'{event}-{label}.bin').write_bytes(bytes(reflection.rawBytes))
                row[label + '_constants'] = []
                for index, block in enumerate(reflection.constantBlocks):
                    binding = pipe.GetConstantBlock(stage, index, 0)
                    descriptor = binding.descriptor
                    values = replay.GetCBufferVariableContents(pipe.GetGraphicsPipelineObject(),
                        pipe.GetShader(stage), stage, pipe.GetShaderEntryPoint(stage), index,
                        descriptor.resource, descriptor.byteOffset, descriptor.byteSize)
                    row.setdefault(label + '_uniform_values', []).append(dict(
                        block=block.name, values=plain(values)))
                    if binding.descriptor.resource == rd.ResourceId.Null():
                        continue
                    data = bytes(replay.GetBufferData(descriptor.resource,
                                 descriptor.byteOffset, descriptor.byteSize))
                    name = f'{event}-{label}-constants-{index}.bin'
                    (out / name).write_bytes(data)
                    row[label + '_constants'].append(dict(block=plain(block),
                                                          binding=plain(binding), file=name))
        row['pipeline'] = plain(replay.GetVulkanPipelineState() if
            replay.GetAPIProperties().pipelineType == rd.GraphicsAPI.Vulkan else
            replay.GetGLPipelineState())
        readbacks = (True, False) if os.environ.get('RADAR_READBACK', 'all') == 'all' else (
            (False,) if action == actions[-1] else ())
        for before in readbacks:
            replay.SetFrameEvent(event - 1 if before else event, True)
            raw = bytes(replay.GetTextureData(target.resourceId, rd.Subresource()))
            name = f'{event}-' + ('before' if before else 'after')
            (out / (name + '.bin')).write_bytes(raw)
            save = rd.TextureSave()
            save.resourceId = target.resourceId
            save.destType = rd.FileType.PNG
            save.alpha = rd.AlphaMapping.Preserve
            assert replay.SaveTexture(save, str(out / (name + '.png'))) == rd.ResultCode.Succeeded
        rows.append(row)
        (out / f'{event}-draw.json').write_text(json.dumps(row, indent=2) + '\n')
        (out / 'progress.json').write_text(json.dumps(dict(events=[r['event'] for r in rows])))
    (out / 'capture.json').write_text(json.dumps(dict(
        capture=str(capture), sha256=hashlib.sha256(capture.read_bytes()).hexdigest(),
        boundary='Original game executed in xemu; emulator GPU output, not physical NV2A.',
        draws=rows), indent=2) + '\n')
    replay.Shutdown()
    file.Shutdown()
except Exception:
    (out / 'error.txt').write_text(traceback.format_exc())
os._exit(0)

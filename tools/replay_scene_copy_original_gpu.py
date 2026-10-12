"""Offline replay only; run in qrenderdoc --python with SCENE_COPY_GPU_SESSION."""
from pathlib import Path
import hashlib, json, os, runpy, traceback
import renderdoc as rd

ROOT = Path('/home/eric/Developer/Halo-2-Decompiled')
TOOL = ROOT / 'tools/replay_scene_copy_original_gpu.py'
SESSION = Path(os.environ['SCENE_COPY_GPU_SESSION']).resolve()
PARENT = ROOT / 'analysis/multiplayer/game-variants/scene-copy-original-gpu-20261012'
assert SESSION.parent == PARENT
OUT = SESSION / 'replay'
OUT.mkdir(exist_ok=False)
HELPER = ROOT / 'analysis/multiplayer-shading/appearance-residual-replay/replay.py'
helpers = runpy.run_path(str(HELPER))
plain, blob, digest = (helpers[key] for key in ['plain', 'blob', 'file_digest'])

def save(name, value):
    (OUT / name).write_text(json.dumps(value, indent=2) + '\n')

def main():
    cap = ctrl = None
    report = {'capture': str(SESSION / 'scene-copy_capture.rdc'), 'stage': 'hashing',
              'guest_IO': False, 'shader_replacements': False}
    try:
        report['capture_sha256'] = digest(Path(report['capture']))
        report['tool_sha256'] = digest(TOOL)
        report['helper_sha256'] = digest(HELPER)
        report['stage'] = 'opening-file'; save('progress.json', report)
        cap = rd.OpenCaptureFile()
        result = cap.OpenFile(report['capture'], '', None)
        assert result == rd.ResultCode.Succeeded, str(result)
        report['stage'] = 'opening-replay'; save('progress.json', report)
        result, ctrl = cap.OpenCapture(rd.ReplayOptions(), None)
        assert result == rd.ResultCode.Succeeded, str(result)
        report['stage'] = 'inventory'; save('progress.json', report)
        actions = helpers['actions_of'](ctrl.GetRootActions())
        report['action_count'] = len(actions)
        textures = {str(t.resourceId): t for t in ctrl.GetTextures()}
        report['textures'] = [dict(id=str(t.resourceId), width=t.width, height=t.height,
            format=t.format.Name(), bytes=t.byteSize, mips=t.mips) for t in textures.values()]
        targets = [t for t in textures.values() if t.width == 320 and t.height == 240]
        report['targets320x240'] = []
        candidates = set()
        for texture in targets:
            usages = list(ctrl.GetUsage(texture.resourceId))
            report['targets320x240'].append(dict(resource=str(texture.resourceId),
                usage=[dict(event=u.eventId, usage=str(u.usage)) for u in usages]))
            candidates.update(u.eventId for u in usages if u.eventId in actions and
                              actions[u.eventId].flags & rd.ActionFlags.Drawcall)
        save('inventory.json', report)
        report['draws'] = []
        for event in sorted(candidates):
            ctrl.SetFrameEvent(event, True)
            pipe = ctrl.GetPipelineState()
            outputs = list(pipe.GetOutputTargets())
            target_ids = {str(t.resourceId) for t in targets}
            if not any(str(o.resource) in target_ids for o in outputs):
                continue
            action = actions[event]
            packet = {'event': event, 'action': plain(action),
                'viewport': plain(pipe.GetViewport(0)), 'scissor': plain(pipe.GetScissor(0)),
                'outputs': plain(outputs), 'depth': plain(pipe.GetDepthTarget()),
                'topology': str(pipe.GetPrimitiveTopology()), 'input': plain(pipe.GetVertexInputs()),
                'gl': plain(ctrl.GetGLPipelineState()), 'samplers': plain(pipe.GetSamplers(rd.ShaderStage.Fragment))}
            for stage, name in [(rd.ShaderStage.Vertex, 'vertex'), (rd.ShaderStage.Fragment, 'fragment')]:
                reflection = pipe.GetShaderReflection(stage)
                if reflection:
                    (OUT / f'{event}-{name}.glsl').write_bytes(bytes(reflection.rawBytes))
                    packet[name] = helpers['blocks'](ctrl, pipe, stage, OUT, event)
            packet['sampled'] = []
            for used in pipe.GetReadOnlyResources(rd.ShaderStage.Fragment):
                texture = textures.get(str(used.descriptor.resource))
                item = {'binding': plain(used)}
                if texture and not used.access.staticallyUnused:
                    item.update({'texture': plain(texture), **blob(OUT,
                        f'{event}-sample-{str(texture.resourceId).split("::")[-1]}.bin',
                        ctrl.GetTextureData(texture.resourceId, rd.Subresource()))})
                packet['sampled'].append(item)
            mesh = ctrl.GetPostVSData(0, 0, rd.MeshDataStage.VSOut)
            packet['postvs'] = {'description': plain(mesh)}
            if mesh.vertexResourceId != rd.ResourceId.Null():
                packet['postvs'].update(blob(OUT, f'{event}-postvs.bin', ctrl.GetBufferData(
                    mesh.vertexResourceId, mesh.vertexByteOffset, mesh.numIndices * mesh.vertexByteStride)))
            packet['surfaces'] = []
            for output in outputs:
                if str(output.resource) not in target_ids:
                    continue
                for label, at in [('before', event - 1), ('after', event)]:
                    ctrl.SetFrameEvent(at, True)
                    packet['surfaces'].append({'label': label, 'resource': str(output.resource),
                        **blob(OUT, f'{event}-{label}-target.bin',
                            ctrl.GetTextureData(output.resource, rd.Subresource()))})
                    image = rd.TextureSave(); image.resourceId = output.resource
                    image.destType = rd.FileType.PNG
                    ctrl.SaveTexture(image, str(OUT / f'{event}-{label}-target.png'))
            save(f'{event}-draw.json', packet)
            report['draws'].append({'event': event, 'file': f'{event}-draw.json',
                                   'indices': action.numIndices})
            save('progress.json', report)
        report['stage'] = 'complete'
    except Exception:
        report['stage'] = 'failed'
        report['error'] = traceback.format_exc()
    finally:
        save('report.json', report)
        if ctrl: ctrl.Shutdown()
        if cap: cap.Shutdown()
    os._exit(0)

main()
